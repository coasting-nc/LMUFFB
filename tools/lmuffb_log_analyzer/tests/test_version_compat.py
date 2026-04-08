import pytest
import struct
import numpy as np
from pathlib import Path
import pandas as pd
from lmuffb_log_analyzer.loader import load_log, LOG_FRAME_DTYPE_V11, LOG_FRAME_DTYPE_V12

def create_mock_v11(path: Path):
    header = "# LMUFFB Telemetry Log v1.1\n# [DATA_START]\n"
    # Create 10 frames
    data = np.zeros(10, dtype=LOG_FRAME_DTYPE_V11)
    data['timestamp'] = np.arange(10) * 0.0025
    data['speed'] = 50.0

    with open(path, 'wb') as f:
        f.write(header.encode('utf-8'))
        f.write(data.tobytes())

def create_mock_v12(path: Path):
    header = "# LMUFFB Telemetry Log v1.2\n# [DATA_START]\n"
    # Create 10 frames
    data = np.zeros(10, dtype=LOG_FRAME_DTYPE_V12)
    data['timestamp'] = np.arange(10) * 0.0025
    data['speed'] = 60.0
    data['ffb_stationary_damping'] = 0.5

    with open(path, 'wb') as f:
        f.write(header.encode('utf-8'))
        f.write(data.tobytes())

def test_v11_v12_compatibility(tmp_path):
    v11_path = tmp_path / "test_v11.bin"
    v12_path = tmp_path / "test_v12.bin"

    create_mock_v11(v11_path)
    create_mock_v12(v12_path)

    # Test V11
    meta11, df11 = load_log(str(v11_path))
    assert meta11.log_version == "v1.1"
    assert len(df11) == 10
    assert df11.iloc[0]['Speed'] == 50.0
    # V11 shouldn't have StationaryDamping in the binary,
    # but the DataFrame might have it as NaN or missing if not mapped?
    # Actually our mapping includes it, so for V11 it might not exist in the source 'data' array.
    assert 'FFBStationaryDamping' not in df11.columns or pd.isna(df11.iloc[0]['FFBStationaryDamping'])

    # Test V12
    meta12, df12 = load_log(str(v12_path))
    assert meta12.log_version == "v1.2"
    assert len(df12) == 10
    assert df12.iloc[0]['Speed'] == 60.0
    assert df12.iloc[0]['FFBStationaryDamping'] == 0.5

def test_autodetect_fallback(tmp_path):
    # Create a file with v1.2 data but v1.1 header (simulating a common dev error)
    path = tmp_path / "test_mismatch.bin"
    header = "# LMUFFB Telemetry Log v1.1\n# [DATA_START]\n"
    data = np.zeros(10, dtype=LOG_FRAME_DTYPE_V12)

    with open(path, 'wb') as f:
        f.write(header.encode('utf-8'))
        f.write(data.tobytes())

    # Should still load because of the % itemsize check
    meta, df = load_log(str(path))
    assert len(df) == 10
