import pytest
import pandas as pd
import numpy as np
import struct
import os
import xml.etree.ElementTree as ET
from pathlib import Path
from datetime import datetime
from lmuffb_log_analyzer.models import SessionMetadata
from lmuffb_log_analyzer.exporters.motec_exporter import MotecExporter

@pytest.fixture
def mock_telemetry():
    """Create 1 second of mock telemetry at 400Hz"""
    n_samples = 400
    times = np.linspace(0, 1.0, n_samples)
    df = pd.DataFrame({
        'Time': times,
        'Speed': np.linspace(0, 100, n_samples),
        'LatAccel': np.sin(times * 2 * np.pi) * 2.0,
        'Steering': np.linspace(-90, 90, n_samples),
        'Throttle': np.linspace(0, 1, n_samples),
        'Brake': np.zeros(n_samples),
        'Marker': np.zeros(n_samples, dtype=int),
        'YawRate': np.zeros(n_samples),
        'FFBTotal': np.zeros(n_samples),
        'LoadFL': np.zeros(n_samples),
        'LoadFR': np.zeros(n_samples),
        'LoadRL': np.zeros(n_samples),
        'LoadRR': np.zeros(n_samples),
        'SlipAngleFL': np.zeros(n_samples),
        'SlipAngleFR': np.zeros(n_samples),
        'SlipAngleRL': np.zeros(n_samples),
        'SlipAngleRR': np.zeros(n_samples),
        'GripFL': np.zeros(n_samples),
        'GripFR': np.zeros(n_samples),
        'GripRL': np.zeros(n_samples),
        'GripRR': np.zeros(n_samples),
    })

    metadata = SessionMetadata(
        log_version="1.1",
        timestamp=datetime(2026, 3, 12, 14, 30, 0),
        app_version="0.7.167",
        driver_name="Test Driver",
        vehicle_name="Test Car",
        track_name="Test Track",
        gain=1.0,
        understeer_effect=1.0,
        sop_effect=1.0,
        slope_enabled=True,
        slope_sensitivity=0.5,
        slope_threshold=-0.3
    )

    return metadata, df

def test_motec_exporter_files_created(mock_telemetry, tmp_path):
    metadata, df = mock_telemetry
    output_ld = tmp_path / "test_export.ld"

    exporter = MotecExporter()
    exporter.export(metadata, df, str(output_ld))

    assert output_ld.exists()
    assert output_ld.with_suffix(".ldx").exists()

def test_ld_header_magic_numbers(mock_telemetry, tmp_path):
    metadata, df = mock_telemetry
    output_ld = tmp_path / "test_header.ld"

    exporter = MotecExporter()
    exporter.export(metadata, df, str(output_ld))

    with open(output_ld, "rb") as f:
        # File Marker
        f.seek(0x00)
        assert struct.unpack("<I", f.read(4))[0] == 0x40

        # Pro Logging Flag
        f.seek(0x5DE)
        assert struct.unpack("<I", f.read(4))[0] == 0xc81a4

def test_ld_channel_linked_list(mock_telemetry, tmp_path):
    metadata, df = mock_telemetry
    output_ld = tmp_path / "test_list.ld"

    exporter = MotecExporter()
    exporter.export(metadata, df, str(output_ld))

    with open(output_ld, "rb") as f:
        # Get pointer to first channel
        f.seek(0x08)
        chan_ptr = struct.unpack("<I", f.read(4))[0]

        channel_count = 0
        while chan_ptr != 0:
            channel_count += 1
            f.seek(chan_ptr)
            next_ptr = struct.unpack("<I", f.read(4))[0]

            # Sanity check to avoid infinite loop
            if channel_count > 200:
                pytest.fail("Too many channels or circular linked list")
            chan_ptr = next_ptr

        assert channel_count >= 5 # We expect at least the basic channels

def test_ld_data_scaling(mock_telemetry, tmp_path):
    metadata, df = mock_telemetry
    # Set a constant speed for easier verification
    df['Speed'] = 72.0 # 72 km/h
    output_ld = tmp_path / "test_scaling.ld"

    exporter = MotecExporter()
    exporter.export(metadata, df, str(output_ld))

    with open(output_ld, "rb") as f:
        # Find Speed channel
        f.seek(0x08)
        chan_ptr = struct.unpack("<I", f.read(4))[0]

        speed_found = False
        while chan_ptr != 0:
            f.seek(chan_ptr)
            next_ptr = struct.unpack("<I", f.read(4))[0]
            data_ptr = struct.unpack("<I", f.read(4))[0]
            num_samples = struct.unpack("<I", f.read(4))[0]
            f.seek(chan_ptr + 0x0E) # Go to data_type
            data_type = struct.unpack("<H", f.read(2))[0]
            sample_size = struct.unpack("<H", f.read(2))[0]
            sample_rate = struct.unpack("<H", f.read(2))[0]
            multiplier = struct.unpack("<h", f.read(2))[0]
            divisor = struct.unpack("<h", f.read(2))[0]
            offset = struct.unpack("<h", f.read(2))[0]
            decimals = struct.unpack("<h", f.read(2))[0]
            name = f.read(32).decode('utf-8').strip('\x00')

            if "Speed" in name:
                speed_found = True
                f.seek(data_ptr)
                raw_val = struct.unpack("<h", f.read(2))[0]

                # displayed_value = (raw_value * multiplier / divisor / (10^decimal_places)) + offset
                displayed = (raw_val * multiplier / divisor / (10**decimals)) + offset
                assert displayed == pytest.approx(72.0, abs=0.1)
                break
            chan_ptr = next_ptr

        assert speed_found

def test_ldx_xml_content(mock_telemetry, tmp_path):
    metadata, df = mock_telemetry
    output_ld = tmp_path / "test_xml.ld"

    exporter = MotecExporter()
    exporter.export(metadata, df, str(output_ld))

    ldx_path = output_ld.with_suffix(".ldx")
    tree = ET.parse(ldx_path)
    root = tree.getroot()

    assert root.tag == "LDXFile"

    # Check details
    details = root.find("Details")
    assert details is not None

    found_driver = False
    for string in details.findall("String"):
        if string.get("Id") == "Driver":
            assert string.get("Value") == "Test Driver"
            found_driver = True

    assert found_driver
