import pandas as pd
import numpy as np
import lz4.block
import struct
from pathlib import Path
from typing import Tuple, Optional
from datetime import datetime
from .models import SessionMetadata

# Define the LogFrame dtype matching C++ LogFrame struct exactly (v0.7.126)
LOG_FRAME_DTYPE = np.dtype([
    ('timestamp', np.float64),
    ('delta_time', np.float64),

    # Vehicle State (Grouped as in C++ struct)
    ('speed', np.float32),
    ('lat_accel', np.float32),
    ('long_accel', np.float32),
    ('yaw_rate', np.float32),

    # Driver Inputs
    ('steering', np.float32),
    ('throttle', np.float32),
    ('brake', np.float32),

    # Front Axle - Raw Telemetry (100Hz)
    ('raw_steering', np.float32),
    ('raw_lat_accel', np.float32),
    ('raw_load_fl', np.float32),
    ('raw_load_fr', np.float32),
    ('raw_slip_vel_fl', np.float32),
    ('raw_slip_vel_fr', np.float32),

    # Front Axle - Processed (400Hz)
    ('slip_angle_fl', np.float32),
    ('slip_angle_fr', np.float32),
    ('slip_ratio_fl', np.float32),
    ('slip_ratio_fr', np.float32),
    ('grip_fl', np.float32),
    ('grip_fr', np.float32),
    ('load_fl', np.float32),
    ('load_fr', np.float32),
    ('load_rl', np.float32),
    ('load_rr', np.float32),

    # Suspension & Ride Height
    ('ride_height_fl', np.float32),
    ('ride_height_fr', np.float32),
    ('ride_height_rl', np.float32),
    ('ride_height_rr', np.float32),
    ('susp_deflection_fl', np.float32),
    ('susp_deflection_fr', np.float32),
    ('susp_deflection_rl', np.float32),
    ('susp_deflection_rr', np.float32),

    # Calculated values
    ('calc_slip_angle_front', np.float32),
    ('calc_grip_front', np.float32),
    ('calc_grip_rear', np.float32),
    ('grip_delta', np.float32),

    # Yaw Kick Analysis (Issue #241)
    ('raw_yaw_accel', np.float32),
    ('smoothed_yaw_accel', np.float32),
    ('ffb_yaw_kick', np.float32),
    ('lat_load_norm', np.float32),

    # Slope Detection Specific
    ('dG_dt', np.float32),
    ('dAlpha_dt', np.float32),
    ('slope_current', np.float32),
    ('slope_raw_unclamped', np.float32),
    ('slope_numerator', np.float32),
    ('slope_denominator', np.float32),
    ('hold_timer', np.float32),
    ('input_slip_smoothed', np.float32),
    ('slope_smoothed', np.float32),
    ('confidence', np.float32),

    # Accuracy Tools / Advanced Slope
    ('surface_type_fl', np.float32),
    ('surface_type_fr', np.float32),
    ('slope_torque', np.float32),
    ('slew_limited_g', np.float32),

    # FFB Output
    ('ffb_total', np.float32),
    ('ffb_base', np.float32),
    ('ffb_shaft_torque', np.float32),
    ('ffb_gen_torque', np.float32),
    ('ffb_sop', np.float32),
    ('ffb_grip_factor', np.float32),
    ('speed_gate', np.float32),
    ('load_peak_ref', np.float32),

    ('clipping', np.uint8),
    ('marker', np.uint8)
])

def load_log(filepath: str) -> Tuple[SessionMetadata, pd.DataFrame]:
    """
    Load lmuFFB telemetry log file (Binary or CSV).
    
    Returns:
        Tuple of (SessionMetadata, DataFrame with telemetry data)
    """
    path = Path(filepath)
    if not path.exists():
        raise FileNotFoundError(f"Log file not found: {filepath}")
    
    # Check if binary (via extension)
    if path.suffix == '.bin':
        return load_bin(filepath)
    else:
        # Fallback to CSV
        return load_csv(filepath)

def load_csv(filepath: str) -> Tuple[SessionMetadata, pd.DataFrame]:
    path = Path(filepath)
    # Parse header comments
    metadata = _parse_header(path)
    
    # Find data start line (first non-comment line)
    data_start = 0
    with open(path, 'r') as f:
        for i, line in enumerate(f):
            if not line.startswith('#'):
                data_start = i
                break
    
    # Load CSV data
    df = pd.read_csv(filepath, skiprows=data_start)
    
    # Ensure clipping/marker are int
    if 'Clipping' in df.columns:
        df['Clipping'] = df['Clipping'].astype(int)
    if 'Marker' in df.columns:
        df['Marker'] = df['Marker'].astype(int)

    return metadata, df

def load_bin(filepath: str) -> Tuple[SessionMetadata, pd.DataFrame]:
    """Load binary telemetry log file (supports LZ4 and uncompressed)"""
    path = Path(filepath)
    metadata = _parse_header(path)

    with open(path, 'rb') as f:
        # Robustly find [DATA_START]\n marker
        # We read a chunk that should contain the entire header
        chunk = f.read(8192)
        marker = b'[DATA_START]\n'
        offset = chunk.find(marker)

        if offset == -1:
            raise ValueError("Binary log file missing [DATA_START] marker in first 8KB")

        # Jump to start of data
        data_pos = offset + len(marker)
        f.seek(data_pos)

        # Check for LZ4 compression (indicated by metadata or checking first 4 bytes for magic/size)
        is_lz4 = False
        with open(path, 'r', errors='ignore') as f_meta:
            for line in f_meta:
                if "Compression: LZ4" in line:
                    is_lz4 = True
                    break

        if is_lz4:
            all_data = []
            while True:
                size_bytes = f.read(8)
                if not size_bytes or len(size_bytes) < 8:
                    break
                compressed_size, uncompressed_size = struct.unpack("<II", size_bytes)
                compressed_data = f.read(compressed_size)
                if len(compressed_data) < compressed_size:
                    break
                uncompressed_data = lz4.block.decompress(compressed_data, uncompressed_size=uncompressed_size)
                all_data.append(np.frombuffer(uncompressed_data, dtype=LOG_FRAME_DTYPE))
            data = np.concatenate(all_data) if all_data else np.array([], dtype=LOG_FRAME_DTYPE)
        else:
            # Read the rest of the file into a numpy array
            data = np.fromfile(f, dtype=LOG_FRAME_DTYPE)

    df = pd.DataFrame(data)

    # Rename columns to match CSV for consistency (CamelCase)
    mapping = {
        'timestamp': 'Time',
        'delta_time': 'DeltaTime',
        'speed': 'Speed',
        'lat_accel': 'LatAccel',
        'long_accel': 'LongAccel',
        'yaw_rate': 'YawRate',
        'steering': 'Steering',
        'throttle': 'Throttle',
        'brake': 'Brake',
        'slip_angle_fl': 'SlipAngleFL',
        'slip_angle_fr': 'SlipAngleFR',
        'slip_ratio_fl': 'SlipRatioFL',
        'slip_ratio_fr': 'SlipRatioFR',
        'grip_fl': 'GripFL',
        'grip_fr': 'GripFR',
        'load_fl': 'LoadFL',
        'load_fr': 'LoadFR',
        'load_rl': 'LoadRL',
        'load_rr': 'LoadRR',
        'ride_height_fl': 'RideHeightFL',
        'ride_height_fr': 'RideHeightFR',
        'ride_height_rl': 'RideHeightRL',
        'ride_height_rr': 'RideHeightRR',
        'susp_deflection_fl': 'SuspDeflectionFL',
        'susp_deflection_fr': 'SuspDeflectionFR',
        'susp_deflection_rl': 'SuspDeflectionRL',
        'susp_deflection_rr': 'SuspDeflectionRR',
        'calc_slip_angle_front': 'CalcSlipAngle',
        'calc_grip_front': 'CalcGripFront',
        'calc_grip_rear': 'CalcGripRear',
        'grip_delta': 'GripDelta',
        'raw_yaw_accel': 'RawYawAccel',
        'smoothed_yaw_accel': 'SmoothedYawAccel',
        'ffb_yaw_kick': 'FFBYawKick',
        'lat_load_norm': 'LatLoadNorm',
        'dG_dt': 'dG_dt',
        'dAlpha_dt': 'dAlpha_dt',
        'slope_current': 'SlopeCurrent',
        'slope_raw_unclamped': 'SlopeRaw',
        'slope_numerator': 'SlopeNum',
        'slope_denominator': 'SlopeDenom',
        'hold_timer': 'HoldTimer',
        'input_slip_smoothed': 'InputSlipSmooth',
        'slope_smoothed': 'SlopeSmoothed',
        'confidence': 'Confidence',
        'surface_type_fl': 'SurfaceFL',
        'surface_type_fr': 'SurfaceFR',
        'slope_torque': 'SlopeTorque',
        'slew_limited_g': 'SlewLimitedG',
        'ffb_total': 'FFBTotal',
        'ffb_base': 'FFBBase',
        'ffb_shaft_torque': 'FFBShaftTorque',
        'ffb_gen_torque': 'FFBGenTorque',
        'ffb_sop': 'FFBSoP',
        'ffb_grip_factor': 'GripFactor',
        'speed_gate': 'SpeedGate',
        'load_peak_ref': 'LoadPeakRef',
        'raw_steering': 'RawSteering',
        'raw_lat_accel': 'RawLatAccel',
        'raw_load_fl': 'RawLoadFL',
        'raw_load_fr': 'RawLoadFR',
        'raw_slip_vel_fl': 'RawSlipVelFL',
        'raw_slip_vel_fr': 'RawSlipVelFR',
        'clipping': 'Clipping',
        'marker': 'Marker'
    }

    # Apply mapping
    df.rename(columns=mapping, inplace=True)

    return metadata, df

def _parse_datetime(date_str: str) -> datetime:
    """Parse datetime from log header"""
    try:
        return datetime.strptime(date_str, "%Y-%m-%d %H:%M:%S")
    except ValueError:
        return datetime.now()

def _safe_float(val: Optional[str]) -> Optional[float]:
    """Safely convert string to float"""
    if val is None or val.lower() == 'none' or val == '':
        return None
    try:
        return float(val)
    except ValueError:
        return None

def _parse_header(path: Path) -> SessionMetadata:
    """Extract metadata from header comments"""
    header_data = {}
    
    # Read as bytes to handle potential binary content after header
    with open(path, 'rb') as f:
        for line_bytes in f:
            try:
                line = line_bytes.decode('utf-8', errors='ignore').strip()
            except:
                continue

            if not line.startswith('#'):
                break
            
            line = line.lstrip('# ').strip()
            if ':' in line:
                key, value = line.split(':', 1)
                header_data[key.strip().lower().replace(' ', '_')] = value.strip()
            elif 'LMUFFB Telemetry Log' in line:
                parts = line.split(':')
                if len(parts) > 1:
                    header_data['log_version'] = parts[1].strip()

    return SessionMetadata(
        log_version=header_data.get('lmuffb_telemetry_log', 'unknown'),
        timestamp=_parse_datetime(header_data.get('date', '')),
        app_version=header_data.get('app_version', 'unknown'),
        driver_name=header_data.get('driver', 'Unknown'),
        vehicle_name=header_data.get('vehicle', 'Unknown'),
        track_name=header_data.get('track', 'Unknown'),
        gain=float(header_data.get('gain', 1.0)),
        understeer_effect=float(header_data.get('understeer_effect', 1.0)),
        sop_effect=float(header_data.get('sop_effect', 1.0)),
        slope_enabled=header_data.get('slope_detection', '').lower() == 'enabled',
        slope_sensitivity=float(header_data.get('slope_sensitivity', 0.5)),
        slope_threshold=float(header_data.get('slope_threshold', -0.3)),
        slope_alpha_threshold=_safe_float(header_data.get('slope_alpha_threshold')),
        slope_decay_rate=_safe_float(header_data.get('slope_decay_rate')),
    )
