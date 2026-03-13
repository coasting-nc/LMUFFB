import pandas as pd
import numpy as np
import lz4.block
import struct
from pathlib import Path
from typing import Tuple, Optional
from datetime import datetime
from .models import SessionMetadata

# Define the LogFrame dtype matching C++ LogFrame struct exactly (v0.7.129)
# 2 doubles (16) + 123 floats (492) + 3 uint8 (3) = 511 bytes
LOG_FRAME_DTYPE = np.dtype([
    ('timestamp', np.float64),
    ('delta_time', np.float64),

    # --- PROCESSED 400Hz DATA (Smooth) ---
    ('speed', np.float32),
    ('lat_accel', np.float32),
    ('long_accel', np.float32),
    ('yaw_rate', np.float32),

    ('steering', np.float32),
    ('throttle', np.float32),
    ('brake', np.float32),

    # --- RAW 100Hz GAME DATA (Step-function) ---
    ('raw_steering', np.float32),
    ('raw_throttle', np.float32),
    ('raw_brake', np.float32),
    ('raw_lat_accel', np.float32),
    ('raw_long_accel', np.float32),
    ('raw_game_yaw_accel', np.float32),
    ('raw_game_shaft_torque', np.float32),
    ('raw_game_gen_torque', np.float32),

    ('raw_load_fl', np.float32),
    ('raw_load_fr', np.float32),
    ('raw_load_rl', np.float32),
    ('raw_load_rr', np.float32),

    ('raw_slip_vel_lat_fl', np.float32),
    ('raw_slip_vel_lat_fr', np.float32),
    ('raw_slip_vel_lat_rl', np.float32),
    ('raw_slip_vel_lat_rr', np.float32),

    ('raw_slip_vel_long_fl', np.float32),
    ('raw_slip_vel_long_fr', np.float32),
    ('raw_slip_vel_long_rl', np.float32),
    ('raw_slip_vel_long_rr', np.float32),

    ('raw_ride_height_fl', np.float32),
    ('raw_ride_height_fr', np.float32),
    ('raw_ride_height_rl', np.float32),
    ('raw_ride_height_rr', np.float32),

    ('raw_susp_deflection_fl', np.float32),
    ('raw_susp_deflection_fr', np.float32),
    ('raw_susp_deflection_rl', np.float32),
    ('raw_susp_deflection_rr', np.float32),

    ('raw_susp_force_fl', np.float32),
    ('raw_susp_force_fr', np.float32),
    ('raw_susp_force_rl', np.float32),
    ('raw_susp_force_rr', np.float32),

    ('raw_brake_pressure_fl', np.float32),
    ('raw_brake_pressure_fr', np.float32),
    ('raw_brake_pressure_rl', np.float32),
    ('raw_brake_pressure_rr', np.float32),

    ('raw_rotation_fl', np.float32),
    ('raw_rotation_fr', np.float32),
    ('raw_rotation_rl', np.float32),
    ('raw_rotation_rr', np.float32),

    # --- ALGORITHM STATE (400Hz) ---
    ('slip_angle_fl', np.float32),
    ('slip_angle_fr', np.float32),
    ('slip_angle_rl', np.float32),
    ('slip_angle_rr', np.float32),

    ('slip_ratio_fl', np.float32),
    ('slip_ratio_fr', np.float32),
    ('slip_ratio_rl', np.float32),
    ('slip_ratio_rr', np.float32),

    ('grip_fl', np.float32),
    ('grip_fr', np.float32),
    ('grip_rl', np.float32),
    ('grip_rr', np.float32),

    ('load_fl', np.float32),
    ('load_fr', np.float32),
    ('load_rl', np.float32),
    ('load_rr', np.float32),

    ('ride_height_fl', np.float32),
    ('ride_height_fr', np.float32),
    ('ride_height_rl', np.float32),
    ('ride_height_rr', np.float32),

    ('susp_deflection_fl', np.float32),
    ('susp_deflection_fr', np.float32),
    ('susp_deflection_rl', np.float32),
    ('susp_deflection_rr', np.float32),

    ('calc_slip_angle_front', np.float32),
    ('calc_slip_angle_rear', np.float32),
    ('calc_grip_front', np.float32),
    ('calc_grip_rear', np.float32),
    ('grip_delta', np.float32),
    ('calc_rear_lat_force', np.float32),

    ('smoothed_yaw_accel', np.float32),
    ('lat_load_norm', np.float32),

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

    ('surface_type_fl', np.float32),
    ('surface_type_fr', np.float32),
    ('slope_torque', np.float32),
    ('slew_limited_g', np.float32),

    ('session_peak_torque', np.float32),
    ('long_load_factor', np.float32),
    ('structural_mult', np.float32),
    ('vibration_mult', np.float32),
    ('steering_angle_deg', np.float32),
    ('steering_range_deg', np.float32),
    ('debug_freq', np.float32),
    ('tire_radius', np.float32),

    # --- FFB COMPONENTS (400Hz) ---
    ('ffb_total', np.float32),
    ('ffb_base', np.float32),
    ('ffb_understeer_drop', np.float32),
    ('ffb_oversteer_boost', np.float32),
    ('ffb_sop', np.float32),
    ('ffb_rear_torque', np.float32),
    ('ffb_scrub_drag', np.float32),
    ('ffb_yaw_kick', np.float32),
    ('ffb_gyro_damping', np.float32),
    ('ffb_road_texture', np.float32),
    ('ffb_slide_texture', np.float32),
    ('ffb_lockup_vibration', np.float32),
    ('ffb_spin_vibration', np.float32),
    ('ffb_bottoming_crunch', np.float32),
    ('ffb_abs_pulse', np.float32),
    ('ffb_soft_lock', np.float32),

    ('extrapolated_yaw_accel', np.float32),
    ('derived_yaw_accel', np.float32),

    ('ffb_shaft_torque', np.float32),
    ('ffb_gen_torque', np.float32),
    ('ffb_grip_factor', np.float32),
    ('speed_gate', np.float32),
    ('front_load_peak_ref', np.float32),

    ('approx_load_fl', np.float32),
    ('approx_load_fr', np.float32),
    ('approx_load_rl', np.float32),
    ('approx_load_rr', np.float32),

    # --- SYSTEM (400Hz) ---
    ('physics_rate', np.float32),
    ('clipping', np.uint8),
    ('warn_bits', np.uint8),
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
        # Robustly find [DATA_START] marker
        chunk = f.read(8192)
        marker = b'[DATA_START]'
        offset = chunk.find(marker)

        if offset == -1:
            raise ValueError("Binary log file missing [DATA_START] marker in first 8KB")

        # Jump to start of data (skip marker and the following newline)
        newline_pos = chunk.find(b'\n', offset)
        if newline_pos == -1:
            raise ValueError("Binary log file missing newline after [DATA_START] marker")

        data_pos = newline_pos + 1
        f.seek(data_pos)

        # Check for LZ4 compression (indicated by metadata)
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

    # Rename columns to match legacy CSV for consistency (CamelCase)
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
        'raw_steering': 'RawSteering',
        'raw_throttle': 'RawThrottle',
        'raw_brake': 'RawBrake',
        'raw_lat_accel': 'RawLatAccel',
        'raw_long_accel': 'RawLongAccel',
        'raw_game_yaw_accel': 'RawGameYawAccel',
        'raw_game_shaft_torque': 'RawGameShaftTorque',
        'raw_game_gen_torque': 'RawGameGenTorque',
        'raw_load_fl': 'RawLoadFL',
        'raw_load_fr': 'RawLoadFR',
        'raw_load_rl': 'RawLoadRL',
        'raw_load_rr': 'RawLoadRR',
        'raw_slip_vel_lat_fl': 'RawSlipVelLatFL',
        'raw_slip_vel_lat_fr': 'RawSlipVelLatFR',
        'raw_slip_vel_lat_rl': 'RawSlipVelLatRL',
        'raw_slip_vel_lat_rr': 'RawSlipVelLatRR',
        'raw_slip_vel_long_fl': 'RawSlipVelLongFL',
        'raw_slip_vel_long_fr': 'RawSlipVelLongFR',
        'raw_slip_vel_long_rl': 'RawSlipVelLongRL',
        'raw_slip_vel_long_rr': 'RawSlipVelLongRR',
        'raw_ride_height_fl': 'RawRideHeightFL',
        'raw_ride_height_fr': 'RawRideHeightFR',
        'raw_ride_height_rl': 'RawRideHeightRL',
        'raw_ride_height_rr': 'RawRideHeightRR',
        'raw_susp_deflection_fl': 'RawSuspDeflectionFL',
        'raw_susp_deflection_fr': 'RawSuspDeflectionFR',
        'raw_susp_deflection_rl': 'RawSuspDeflectionRL',
        'raw_susp_deflection_rr': 'RawSuspDeflectionRR',
        'raw_susp_force_fl': 'RawSuspForceFL',
        'raw_susp_force_fr': 'RawSuspForceFR',
        'raw_susp_force_rl': 'RawSuspForceRL',
        'raw_susp_force_rr': 'RawSuspForceRR',
        'raw_brake_pressure_fl': 'RawBrakePressureFL',
        'raw_brake_pressure_fr': 'RawBrakePressureFR',
        'raw_brake_pressure_rl': 'RawBrakePressureRL',
        'raw_brake_pressure_rr': 'RawBrakePressureRR',
        'raw_rotation_fl': 'RawRotationFL',
        'raw_rotation_fr': 'RawRotationFR',
        'raw_rotation_rl': 'RawRotationRL',
        'raw_rotation_rr': 'RawRotationRR',
        'slip_angle_fl': 'SlipAngleFL',
        'slip_angle_fr': 'SlipAngleFR',
        'slip_angle_rl': 'SlipAngleRL',
        'slip_angle_rr': 'SlipAngleRR',
        'slip_ratio_fl': 'SlipRatioFL',
        'slip_ratio_fr': 'SlipRatioFR',
        'slip_ratio_rl': 'SlipRatioRL',
        'slip_ratio_rr': 'SlipRatioRR',
        'grip_fl': 'GripFL',
        'grip_fr': 'GripFR',
        'grip_rl': 'GripRL',
        'grip_rr': 'GripRR',
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
        'calc_slip_angle_front': 'CalcSlipAngleFront',
        'calc_slip_angle_rear': 'CalcSlipAngleRear',
        'calc_grip_front': 'CalcGripFront',
        'calc_grip_rear': 'CalcGripRear',
        'grip_delta': 'GripDelta',
        'calc_rear_lat_force': 'CalcRearLatForce',
        'smoothed_yaw_accel': 'SmoothedYawAccel',
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
        'session_peak_torque': 'SessionPeakTorque',
        'long_load_factor': 'LongitudinalLoadFactor',
        'structural_mult': 'StructuralMult',
        'vibration_mult': 'VibrationMult',
        'steering_angle_deg': 'SteeringAngleDeg',
        'steering_range_deg': 'SteeringRangeDeg',
        'debug_freq': 'DebugFreq',
        'tire_radius': 'TireRadius',
        'ffb_total': 'FFBTotal',
        'ffb_base': 'FFBBase',
        'ffb_understeer_drop': 'FFBUndersteerDrop',
        'ffb_oversteer_boost': 'FFBOversteerBoost',
        'ffb_sop': 'FFBSoP',
        'ffb_rear_torque': 'FFBRearTorque',
        'ffb_scrub_drag': 'FFBScrubDrag',
        'ffb_yaw_kick': 'FFBYawKick',
        'ffb_gyro_damping': 'FFBGyroDamping',
        'ffb_road_texture': 'FFBRoadTexture',
        'ffb_slide_texture': 'FFBSlideTexture',
        'ffb_lockup_vibration': 'FFBLockupVibration',
        'ffb_spin_vibration': 'FFBSpinVibration',
        'ffb_bottoming_crunch': 'FFBBottomingCrunch',
        'ffb_abs_pulse': 'FFBABSPulse',
        'ffb_soft_lock': 'FFBSoftLock',
        'extrapolated_yaw_accel': 'ExtrapolatedYawAccel',
        'derived_yaw_accel': 'DerivedYawAccel',
        'ffb_shaft_torque': 'FFBShaftTorque',
        'ffb_gen_torque': 'FFBGenTorque',
        'ffb_grip_factor': 'GripFactor',
        'speed_gate': 'SpeedGate',
        'front_load_peak_ref': 'FrontLoadPeakRef',
        'approx_load_fl': 'ApproxLoadFL',
        'approx_load_fr': 'ApproxLoadFR',
        'approx_load_rl': 'ApproxLoadRL',
        'approx_load_rr': 'ApproxLoadRR',
        'physics_rate': 'PhysicsRate',
        'clipping': 'Clipping',
        'warn_bits': 'WarnBits',
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
        car_class=header_data.get('car_class', 'Unknown'),
        car_brand=header_data.get('car_brand', 'Unknown'),
        track_name=header_data.get('track', 'Unknown'),
        gain=float(header_data.get('gain', 1.0)),
        understeer_effect=float(header_data.get('understeer_effect', 1.0)),
        sop_effect=float(header_data.get('sop_effect', 1.0)),
        lat_load_effect=float(header_data.get('lateral_load_effect', 0.0)),
        long_load_effect=float(header_data.get('long_load_effect', 0.0)),
        sop_scale=float(header_data.get('sop_scale', 1.0)),
        sop_smoothing=float(header_data.get('sop_smoothing', 0.0)),
        slope_enabled=header_data.get('slope_detection', '').lower() == 'enabled',
        slope_sensitivity=float(header_data.get('slope_sensitivity', 0.5)),
        slope_threshold=float(header_data.get('slope_threshold', -0.3)),
        slope_alpha_threshold=_safe_float(header_data.get('slope_alpha_threshold')),
        slope_decay_rate=_safe_float(header_data.get('slope_decay_rate')),
        dynamic_normalization=header_data.get('dynamic_normalization', '').lower() == 'enabled',
        auto_load_normalization=header_data.get('auto_load_normalization', '').lower() == 'enabled',
    )
