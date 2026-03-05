import pytest
import pandas as pd
import numpy as np
import struct
from pathlib import Path
from lmuffb_log_analyzer.loader import load_log, load_bin
from lmuffb_log_analyzer.models import SessionMetadata

def create_mock_bin(path: Path):
    """Create a mock binary log file for testing"""
    header = (
        "# LMUFFB Telemetry Log v1.0\n"
        "# App Version: 0.7.126\n"
        "# Driver: Binary Test\n"
        "# Vehicle: Mock Car\n"
        "# Track: Mock Track\n"
        "# Gain: 1.0\n"
        "# Understeer Effect: 0.5\n"
        "# SoP Effect: 0.5\n"
        "# Slope Detection: Enabled\n"
        "# [DATA_START]\n"
    )

    # Matching LOG_FRAME_DTYPE exactly (v0.7.126 re-aligned)
    # double(8)*2 + float(4)*61 + uint8(1)*2 = 16 + 244 + 2 = 262 bytes

    # We'll use struct to pack a single frame
    # d d f...f B B
    # Total floats = 61.

    # Frame 1: recognizable values
    # speed is index 0 in float block (3rd field)
    # surface_type_fl is index 45 in float block (48th field: 2d + 7proc + 6raw + 10algo + 8susp + 4calc + 4yaw + 10slope + 1surf = 61-11-2 = 48th float?)
    # Let's be precise:
    # doubles: 0, 1
    # speed: 2 (float index 0)
    # ...
    # raw_steering: 9 (float index 7)
    # ...
    # slip_angle_fl: 15 (float index 13)
    # ...
    # surface_type_fl: 50 (float index 48)

    floats_1 = [0.0] * 61
    floats_1[0] = 50.0 # speed
    floats_1[48] = 5.0 # surface_type_fl

    frame_1 = struct.pack("<dd" + "f"*61 + "BB",
        100.0, 0.0025, # timestamp, dt
        *floats_1,
        1, 0 # clipping, marker
    )

    # Frame 2: marker set
    floats_2 = [0.0] * 61
    floats_2[0] = 51.0 # speed

    frame_2 = struct.pack("<dd" + "f"*61 + "BB",
        100.0025, 0.0025, # timestamp, dt
        *floats_2,
        0, 1 # clipping, marker
    )

    with open(path, 'wb') as f:
        f.write(header.encode('utf-8'))
        f.write(frame_1)
        f.write(frame_2)

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

    # Mapping based on v0.7.126 re-aligned order (61 floats)
    dtype_list = [
        ('timestamp', 'f8'), ('delta_time', 'f8'),
        ('speed', 'f4'), ('lat_accel', 'f4'), ('long_accel', 'f4'), ('yaw_rate', 'f4'),
        ('steering', 'f4'), ('throttle', 'f4'), ('brake', 'f4'),
        ('raw_steering', 'f4'), ('raw_lat_accel', 'f4'), ('raw_load_fl', 'f4'), ('raw_load_fr', 'f4'),
        ('raw_slip_vel_fl', 'f4'), ('raw_slip_vel_fr', 'f4'),
        ('slip_angle_fl', 'f4'), ('slip_angle_fr', 'f4'), ('slip_ratio_fl', 'f4'), ('slip_ratio_fr', 'f4'),
        ('grip_fl', 'f4'), ('grip_fr', 'f4'), ('load_fl', 'f4'), ('load_fr', 'f4'), ('load_rl', 'f4'), ('load_rr', 'f4'),
        ('ride_height_fl', 'f4'), ('ride_height_fr', 'f4'), ('ride_height_rl', 'f4'), ('ride_height_rr', 'f4'),
        ('susp_deflection_fl', 'f4'), ('susp_deflection_fr', 'f4'), ('susp_deflection_rl', 'f4'), ('susp_deflection_rr', 'f4'),
        ('calc_slip_angle_front', 'f4'), ('calc_grip_front', 'f4'), ('calc_grip_rear', 'f4'), ('grip_delta', 'f4'),
        ('raw_yaw_accel', 'f4'), ('smoothed_yaw_accel', 'f4'), ('ffb_yaw_kick', 'f4'), ('lat_load_norm', 'f4'),
        ('dG_dt', 'f4'), ('dAlpha_dt', 'f4'), ('slope_current', 'f4'), ('slope_raw_unclamped', 'f4'),
        ('slope_numerator', 'f4'), ('slope_denominator', 'f4'), ('hold_timer', 'f4'), ('input_slip_smoothed', 'f4'),
        ('slope_smoothed', 'f4'), ('confidence', 'f4'),
        ('surface_type_fl', 'f4'), ('surface_type_fr', 'f4'), ('slope_torque', 'f4'), ('slew_limited_g', 'f4'),
        ('ffb_total', 'f4'), ('ffb_base', 'f4'), ('ffb_shaft_torque', 'f4'), ('ffb_gen_torque', 'f4'),
        ('ffb_sop', 'f4'), ('ffb_grip_factor', 'f4'), ('speed_gate', 'f4'), ('load_peak_ref', 'f4'),
        ('clipping', 'u1'), ('marker', 'u1')
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
