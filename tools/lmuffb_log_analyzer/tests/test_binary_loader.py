import pytest
import pandas as pd
import numpy as np
import struct
from pathlib import Path
from lmuffb_log_analyzer.loader import load_log, load_bin
from lmuffb_log_analyzer.models import SessionMetadata

# v0.7.170: 129 floats (added approx_load_fl/fr/rl/rr)
NUM_FLOATS = 129

def create_mock_bin(path: Path):
    """Create a mock binary log file for testing"""
    header = (
        "# LMUFFB Telemetry Log v1.1\n"
        "# App Version: 0.7.129\n"
        "# Driver: Binary Test\n"
        "# Vehicle: Mock Car\n"
        "# Track: Mock Track\n"
        "# Gain: 1.0\n"
        "# Understeer Effect: 0.5\n"
        "# SoP Effect: 0.5\n"
        "# Slope Detection: Enabled\n"
        "# [DATA_START]\n"
    )

    # Matching LOG_FRAME_DTYPE exactly (v0.7.129)
    # 2 doubles (16) + 123 floats (492) + 3 uint8 (3) = 511 bytes

    # Frame 1: recognizable values
    floats_1 = [0.0] * NUM_FLOATS
    floats_1[0] = 50.0 # speed

    frame_1 = struct.pack("<dd" + "f"*NUM_FLOATS + "BBB",
        100.0, 0.0025, # timestamp, dt
        *floats_1,
        1, 0, 0 # clipping, warn_bits, marker
    )

    # Frame 2: marker set
    floats_2 = [0.0] * NUM_FLOATS
    floats_2[0] = 51.0 # speed

    frame_2 = struct.pack("<dd" + "f"*NUM_FLOATS + "BBB",
        100.0025, 0.0025, # timestamp, dt
        *floats_2,
        0, 0, 1 # clipping, warn_bits, marker
    )

    with open(path, 'wb') as f:
        f.write(header.encode('utf-8'))
        f.write(frame_1)
        f.write(frame_2)

def create_mock_lz4_bin(path: Path):
    """Create a mock compressed binary log file for testing"""
    header = (
        "# LMUFFB Telemetry Log v1.1\n"
        "# App Version: 0.7.129\n"
        "# Compression: LZ4\n"
        "# Driver: LZ4 Test\n"
        "# Vehicle: Mock Car\n"
        "# Track: Mock Track\n"
        "# [DATA_START]\n"
    )

    floats = [0.0] * NUM_FLOATS
    floats[0] = 60.0 # speed

    # Create two blocks of data
    frames_block1 = struct.pack("<dd" + "f"*NUM_FLOATS + "BBB",
        200.0, 0.0025, # timestamp, dt
        *floats,
        0, 0, 0 # clipping, warn_bits, marker
    ) * 10 # 10 frames in block 1

    frames_block2 = struct.pack("<dd" + "f"*NUM_FLOATS + "BBB",
        201.0, 0.0025, # timestamp, dt
        *floats,
        0, 0, 0 # clipping, warn_bits, marker
    ) * 5 # 5 frames in block 2

    import lz4.block
    compressed1 = lz4.block.compress(frames_block1, store_size=False)
    compressed2 = lz4.block.compress(frames_block2, store_size=False)

    with open(path, 'wb') as f:
        f.write(header.encode('utf-8'))
        # Write block 1: 8-byte header [compressed, uncompressed]
        f.write(struct.pack("<II", len(compressed1), len(frames_block1)))
        f.write(compressed1)
        # Write block 2: 8-byte header
        f.write(struct.pack("<II", len(compressed2), len(frames_block2)))
        f.write(compressed2)

def test_load_bin_log(tmp_path):
    bin_path = tmp_path / "test_log.bin"
    create_mock_bin(bin_path)

    metadata, df = load_log(str(bin_path))

    # Check metadata
    assert metadata.driver_name == "Binary Test"
    assert metadata.vehicle_name == "Mock Car"
    assert metadata.slope_enabled is True

    # Check dataframe
    assert len(df) == 2
    assert "Time" in df.columns
    assert "Speed" in df.columns
    assert "Marker" in df.columns

    assert df.iloc[0]["Time"] == 100.0
    assert df.iloc[0]["Speed"] == 50.0
    assert df.iloc[0]["Clipping"] == 1
    assert df.iloc[0]["Marker"] == 0

    assert df.iloc[1]["Time"] == 100.0025
    assert df.iloc[1]["Speed"] == 51.0
    assert df.iloc[1]["Clipping"] == 0
    assert df.iloc[1]["Marker"] == 1

def test_load_bin_alignment(tmp_path):
    """Verify that fields are correctly aligned with C++ struct re-ordering"""
    bin_path = tmp_path / "test_align.bin"

    header = "# [DATA_START]\n"

    # Mapping based on v0.7.138 order
    dtype_list = [
        ('timestamp', 'f8'), ('delta_time', 'f8'),
        ('speed', 'f4'), ('lat_accel', 'f4'), ('long_accel', 'f4'), ('yaw_rate', 'f4'),
        ('steering', 'f4'), ('throttle', 'f4'), ('brake', 'f4'),
        ('raw_steering', 'f4'), ('raw_throttle', 'f4'), ('raw_brake', 'f4'),
        ('raw_lat_accel', 'f4'), ('raw_long_accel', 'f4'), ('raw_game_yaw_accel', 'f4'),
        ('raw_game_shaft_torque', 'f4'), ('raw_game_gen_torque', 'f4'),
        ('raw_load_fl', 'f4'), ('raw_load_fr', 'f4'), ('raw_load_rl', 'f4'), ('raw_load_rr', 'f4'),
        ('raw_slip_vel_lat_fl', 'f4'), ('raw_slip_vel_lat_fr', 'f4'), ('raw_slip_vel_lat_rl', 'f4'), ('raw_slip_vel_lat_rr', 'f4'),
        ('raw_slip_vel_long_fl', 'f4'), ('raw_slip_vel_long_fr', 'f4'), ('raw_slip_vel_long_rl', 'f4'), ('raw_slip_vel_long_rr', 'f4'),
        ('raw_ride_height_fl', 'f4'), ('raw_ride_height_fr', 'f4'), ('raw_ride_height_rl', 'f4'), ('raw_ride_height_rr', 'f4'),
        ('raw_susp_deflection_fl', 'f4'), ('raw_susp_deflection_fr', 'f4'), ('raw_susp_deflection_rl', 'f4'), ('raw_susp_deflection_rr', 'f4'),
        ('raw_susp_force_fl', 'f4'), ('raw_susp_force_fr', 'f4'), ('raw_susp_force_rl', 'f4'), ('raw_susp_force_rr', 'f4'),
        ('raw_brake_pressure_fl', 'f4'), ('raw_brake_pressure_fr', 'f4'), ('raw_brake_pressure_rl', 'f4'), ('raw_brake_pressure_rr', 'f4'),
        ('raw_rotation_fl', 'f4'), ('raw_rotation_fr', 'f4'), ('raw_rotation_rl', 'f4'), ('raw_rotation_rr', 'f4'),
        ('slip_angle_fl', 'f4'), ('slip_angle_fr', 'f4'), ('slip_angle_rl', 'f4'), ('slip_angle_rr', 'f4'),
        ('slip_ratio_fl', 'f4'), ('slip_ratio_fr', 'f4'), ('slip_ratio_rl', 'f4'), ('slip_ratio_rr', 'f4'),
        ('grip_fl', 'f4'), ('grip_fr', 'f4'), ('grip_rl', 'f4'), ('grip_rr', 'f4'),
        ('load_fl', 'f4'), ('load_fr', 'f4'), ('load_rl', 'f4'), ('load_rr', 'f4'),
        ('ride_height_fl', 'f4'), ('ride_height_fr', 'f4'), ('ride_height_rl', 'f4'), ('ride_height_rr', 'f4'),
        ('susp_deflection_fl', 'f4'), ('susp_deflection_fr', 'f4'), ('susp_deflection_rl', 'f4'), ('susp_deflection_rr', 'f4'),
        ('calc_slip_angle_front', 'f4'), ('calc_slip_angle_rear', 'f4'), ('calc_grip_front', 'f4'), ('calc_grip_rear', 'f4'),
        ('grip_delta', 'f4'), ('calc_rear_lat_force', 'f4'),
        ('smoothed_yaw_accel', 'f4'), ('lat_load_norm', 'f4'),
        ('dG_dt', 'f4'), ('dAlpha_dt', 'f4'), ('slope_current', 'f4'), ('slope_raw_unclamped', 'f4'),
        ('slope_numerator', 'f4'), ('slope_denominator', 'f4'), ('hold_timer', 'f4'), ('input_slip_smoothed', 'f4'),
        ('slope_smoothed', 'f4'), ('confidence', 'f4'),
        ('surface_type_fl', 'f4'), ('surface_type_fr', 'f4'), ('slope_torque', 'f4'), ('slew_limited_g', 'f4'),
        ('session_peak_torque', 'f4'), ('long_load_factor', 'f4'), ('structural_mult', 'f4'), ('vibration_mult', 'f4'),
        ('steering_angle_deg', 'f4'), ('steering_range_deg', 'f4'), ('debug_freq', 'f4'), ('tire_radius', 'f4'),
        ('ffb_total', 'f4'), ('ffb_base', 'f4'), ('ffb_understeer_drop', 'f4'), ('ffb_oversteer_boost', 'f4'),
        ('ffb_sop', 'f4'), ('ffb_rear_torque', 'f4'), ('ffb_scrub_drag', 'f4'), ('ffb_yaw_kick', 'f4'),
        ('ffb_gyro_damping', 'f4'), ('ffb_road_texture', 'f4'), ('ffb_slide_texture', 'f4'), ('ffb_lockup_vibration', 'f4'),
        ('ffb_spin_vibration', 'f4'), ('ffb_bottoming_crunch', 'f4'), ('ffb_abs_pulse', 'f4'), ('ffb_soft_lock', 'f4'),
        ('extrapolated_yaw_accel', 'f4'), ('derived_yaw_accel', 'f4'),
        ('ffb_shaft_torque', 'f4'), ('ffb_gen_torque', 'f4'), ('ffb_grip_factor', 'f4'), ('speed_gate', 'f4'),
        ('front_load_peak_ref', 'f4'),
        ('approx_load_fl', 'f4'), ('approx_load_fr', 'f4'), ('approx_load_rl', 'f4'), ('approx_load_rr', 'f4'),
        ('physics_rate', 'f4'),
        ('clipping', 'u1'), ('warn_bits', 'u1'), ('marker', 'u1')
    ]

    data = np.zeros(1, dtype=dtype_list)

    data['timestamp'] = 123.456
    data['speed'] = 88.0
    data['surface_type_fl'] = 5.0
    data['marker'] = 1

    with open(bin_path, 'wb') as f:
        f.write(header.encode('utf-8'))
        f.write(data.tobytes())

    _, df = load_log(str(bin_path))

    assert df.iloc[0]["Time"] == 123.456
    assert df.iloc[0]["Speed"] == 88.0
    assert df.iloc[0]["SurfaceFL"] == 5.0
    assert df.iloc[0]["Marker"] == 1

def test_load_lz4_bin_log(tmp_path):
    bin_path = tmp_path / "test_lz4_log.bin"
    create_mock_lz4_bin(bin_path)

    metadata, df = load_log(str(bin_path))

    # Check metadata
    assert metadata.driver_name == "LZ4 Test"

    # Check dataframe
    assert len(df) == 15 # 10 + 5 frames
    assert df.iloc[0]["Time"] == 200.0
    assert df.iloc[0]["Speed"] == 60.0
    assert df.iloc[10]["Time"] == 201.0
