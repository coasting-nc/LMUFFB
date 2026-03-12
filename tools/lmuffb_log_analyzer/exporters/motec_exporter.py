import numpy as np
import pandas as pd
import struct
import os
import xml.etree.ElementTree as ET
from datetime import datetime
from pathlib import Path

class MotecExporter:
    def __init__(self):
        # Default channel mapping: (LMU Name, MoTeC Name, Unit, Multiplier, Divisor, Offset, Decimals)
        # Scaling: displayed = (raw * mult / div / 10^dec) + offset
        # We target i16 for most channels
        self.channel_configs = [
            ('Time', 'Time', 's', 1, 1, 0, 3),
            ('Speed', 'Speed', 'km/h', 1, 1, 0, 1),
            ('LatAccel', 'G Force Lat', 'G', 1, 1, 0, 3),
            ('LongAccel', 'G Force Long', 'G', 1, 1, 0, 3),
            ('YawRate', 'Yaw Rate', 'deg/s', 1, 1, 0, 3),
            ('Steering', 'Steering Angle', 'deg', 1, 1, 0, 1),
            ('Throttle', 'Throttle Pos', '%', 100, 1, 0, 1),
            ('Brake', 'Brake Pos', '%', 100, 1, 0, 1),
            ('FFBTotal', 'FFB Total', 'Nm', 1, 1, 0, 3),
            ('LoadFL', 'Wheel Load FL', 'N', 1, 1, 0, 0),
            ('LoadFR', 'Wheel Load FR', 'N', 1, 1, 0, 0),
            ('LoadRL', 'Wheel Load RL', 'N', 1, 1, 0, 0),
            ('LoadRR', 'Wheel Load RR', 'N', 1, 1, 0, 0),
            ('SlipAngleFL', 'Slip Angle FL', 'deg', 1, 1, 0, 3),
            ('SlipAngleFR', 'Slip Angle FR', 'deg', 1, 1, 0, 3),
            ('SlipAngleRL', 'Slip Angle RL', 'deg', 1, 1, 0, 3),
            ('SlipAngleRR', 'Slip Angle RR', 'deg', 1, 1, 0, 3),
            ('GripFL', 'Grip FL', '%', 100, 1, 0, 1),
            ('GripFR', 'Grip FR', '%', 100, 1, 0, 1),
            ('GripRL', 'Grip RL', '%', 100, 1, 0, 1),
            ('GripRR', 'Grip RR', '%', 100, 1, 0, 1),
        ]

    def export(self, metadata, df, output_path, target_freq=100):
        """Export session data to .ld and .ldx files"""
        path = Path(output_path)

        # 1. Resample data
        resampled_df = self._resample(df, target_freq)

        # 2. Write .ld file
        self._write_ld(metadata, resampled_df, path, target_freq)

        # 3. Write .ldx file
        self._write_ldx(metadata, df, path.with_suffix(".ldx"))

    def _resample(self, df, target_freq):
        """Resample variable-rate data to fixed-rate using linear interpolation"""
        start_time = df['Time'].iloc[0]
        end_time = df['Time'].iloc[-1]

        duration = end_time - start_time
        num_samples = int(duration * target_freq) + 1

        new_times = np.linspace(start_time, start_time + (num_samples - 1) / target_freq, num_samples)

        resampled_data = {'Time': new_times}

        for col, _, _, _, _, _, _ in self.channel_configs:
            if col == 'Time': continue
            if col in df.columns:
                resampled_data[col] = np.interp(new_times, df['Time'], df[col])
            else:
                resampled_data[col] = np.zeros(num_samples)

        return pd.DataFrame(resampled_data)

    def _write_ld(self, metadata, df, path, freq):
        """Binary serialization for MoTeC .ld format"""
        num_samples = len(df)
        num_channels = len(self.channel_configs)

        # Prepare data first to know sizes
        packed_data = []
        channel_data_pointers = []

        current_data_offset = 0 # Relative to start of data block

        for col, _, _, mult, div, off, dec in self.channel_configs:
            # displayed = (raw * mult / div / 10^dec) + offset
            # raw = (displayed - offset) * (10^dec) * div / mult
            raw_data = (df[col].values - off) * (10**dec) * div / mult
            raw_data = np.clip(raw_data, -32768, 32767).astype(np.int16)

            packed_data.append(raw_data.tobytes())
            channel_data_pointers.append(current_data_offset)
            current_data_offset += len(raw_data) * 2

        # Header structure (0x664 bytes)
        # We use placeholders for pointers and fill them later
        header = bytearray(0x664)

        struct.pack_into("<I", header, 0x00, 0x40) # File Marker
        # 0x08: Channel Metadata Pointer (placeholder)
        # 0x0C: Channel Data Pointer (placeholder)

        struct.pack_into("<I", header, 0x46, 12345) # Device Serial
        struct.pack_into("<8s", header, 0x4A, b"ADL\0\0\0\0\0") # Device Type
        struct.pack_into("<H", header, 0x52, 420) # Device Version
        struct.pack_into("<I", header, 0x56, num_channels) # Number of Channels

        date_str = metadata.timestamp.strftime("%d/%m/%Y").encode('ascii')
        time_str = metadata.timestamp.strftime("%H:%M:%S").encode('ascii')
        struct.pack_into("<16s", header, 0x5E, date_str)
        struct.pack_into("<16s", header, 0x7E, time_str)

        struct.pack_into("<64s", header, 0x9E, metadata.driver_name.encode('utf-8')[:63])
        struct.pack_into("<64s", header, 0xDE, metadata.vehicle_name.encode('utf-8')[:63])
        struct.pack_into("<64s", header, 0x15E, metadata.track_name.encode('utf-8')[:63])

        struct.pack_into("<I", header, 0x5DE, 0xc81a4) # Pro Flag

        # Write to file
        with open(path, "wb") as f:
            f.write(header)

            # Channel Metadata Block
            metadata_start = 0x664
            data_start = metadata_start + (num_channels * 0x70)

            struct.pack_into("<I", header, 0x08, metadata_start)
            struct.pack_into("<I", header, 0x0C, data_start)

            f.seek(0)
            f.write(header)

            f.seek(metadata_start)
            for i, (col, name, unit, mult, div, off, dec) in enumerate(self.channel_configs):
                next_ptr = metadata_start + (i + 1) * 0x70 if i < num_channels - 1 else 0
                chan_data_ptr = data_start + channel_data_pointers[i]

                chan_header = bytearray(0x70)
                struct.pack_into("<I", chan_header, 0x00, next_ptr)
                struct.pack_into("<I", chan_header, 0x04, chan_data_ptr)
                struct.pack_into("<I", chan_header, 0x08, num_samples)
                struct.pack_into("<H", chan_header, 0x0C, 0x0001)
                struct.pack_into("<H", chan_header, 0x0E, 0x0003)
                struct.pack_into("<H", chan_header, 0x10, 2)
                struct.pack_into("<H", chan_header, 0x12, freq)
                struct.pack_into("<h", chan_header, 0x14, mult)
                struct.pack_into("<h", chan_header, 0x16, div)
                struct.pack_into("<h", chan_header, 0x18, off)
                struct.pack_into("<h", chan_header, 0x1A, dec)
                struct.pack_into("<32s", chan_header, 0x1C, name.encode('utf-8')[:31])
                struct.pack_into("<8s", chan_header, 0x3C, name[:7].encode('utf-8'))
                struct.pack_into("<12s", chan_header, 0x44, unit.encode('utf-8')[:11])
                f.write(chan_header)

            f.seek(data_start)
            for block in packed_data:
                f.write(block)

    def _write_ldx(self, metadata, df, path):
        """Generate XML sidecar for MoTeC"""
        root = ET.Element("LDXFile", ValidationCode="0", Version="1.0")

        layers = ET.SubElement(root, "Layers")
        layer = ET.SubElement(layers, "Layer")
        marker_group = ET.SubElement(layer, "MarkerGroup", Name="Beacons", Index="0")

        # Add start marker
        ET.SubElement(marker_group, "Marker", Version="100", EntityID="1", Time="0.000000")

        # Check for user markers in the dataframe
        if 'Marker' in df.columns:
            markers = df[df['Marker'] > 0]
            for _, row in markers.iterrows():
                ET.SubElement(marker_group, "Marker", Version="100", EntityID="1", Time=f"{row['Time']:.6f}")

        details = ET.SubElement(root, "Details")
        detail_map = [
            ("Driver", metadata.driver_name),
            ("Vehicle", metadata.vehicle_name),
            ("Venue", metadata.track_name),
            ("Event", f"LMUFFB Session {metadata.timestamp.strftime('%Y-%m-%d %H:%M:%S')}"),
            ("Session", "Practice")
        ]

        for d_id, d_val in detail_map:
            ET.SubElement(details, "String", Id=d_id, Value=d_val)

        # XML pretty printing
        tree = ET.ElementTree(root)
        ET.indent(tree, space="  ", level=0)
        tree.write(path, encoding="utf-8", xml_declaration=True)
