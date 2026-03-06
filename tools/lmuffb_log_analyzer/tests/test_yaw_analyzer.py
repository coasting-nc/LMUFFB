import pandas as pd
import numpy as np
import pytest
from tools.lmuffb_log_analyzer.analyzers.yaw_analyzer import (
    analyze_yaw_dynamics,
    calculate_suspension_velocity,
    analyze_clipping,
    get_fft
)

@pytest.fixture
def sample_df():
    """Create a dummy dataframe with necessary columns."""
    t = np.arange(0, 1.0, 0.0025)  # 400Hz, 1s
    n = len(t)

    df = pd.DataFrame({
        'Time': t,
        'Speed': np.full(n, 30.0),  # > 27.8
        'CalcSlipAngleFront': np.zeros(n),
        'RawGameYawAccel': np.sin(2 * np.pi * 50 * t) * 2.0, # 50Hz noise, amp 2.0 (> 1.68)
        'SmoothedYawAccel': np.sin(2 * np.pi * 5 * t) * 1.0,
        'FFBYawKick': np.sin(2 * np.pi * 5 * t) * 0.5,
        'FFBRearTorque': np.zeros(n),
        'FFBTotal': np.ones(n),
        'RawSuspDeflectionFL': np.linspace(0, 0.1, n),
        'Clipping': np.zeros(n, dtype=int),
        'YawRate': np.cos(2 * np.pi * 5 * t)
    })

    # Add some clipping
    df.loc[10:20, 'Clipping'] = 1
    df.loc[10:20, 'FFBBase'] = 10.0

    return df

def test_analyze_yaw_dynamics(sample_df):
    results = analyze_yaw_dynamics(sample_df, threshold=1.68)

    assert 'threshold_crossing_rate' in results
    # With 50Hz sine and 1.68 threshold, it should cross many times
    assert results['threshold_crossing_rate'] > 0
    assert 'yaw_kick_contribution_pct' in results
    assert 'straightaway_yaw_kick_mean' in results
    assert results['yaw_accel_nan_count'] == 0

def test_calculate_suspension_velocity(sample_df):
    df_vel = calculate_suspension_velocity(sample_df)
    assert 'RawSuspVelocityFL' in df_vel.columns
    # Constant gradient of 0.1 over 1.0s should be ~0.1
    assert pytest.approx(df_vel['RawSuspVelocityFL'].mean(), 0.01) == 0.1

def test_analyze_clipping(sample_df):
    results = analyze_clipping(sample_df)
    assert 'total_clipping_pct' in results
    assert results['total_clipping_pct'] > 0
    assert 'FFBBase' in results['clipping_component_means']
    assert results['clipping_component_means']['FFBBase'] == 10.0

def test_get_fft():
    t = np.arange(0, 1.0, 0.0025)
    f_target = 50.0
    signal = np.sin(2 * np.pi * f_target * t)

    res = get_fft(signal, fs=400.0)

    assert len(res['freqs']) > 0
    # Peak should be at 50Hz
    peak_freq = res['freqs'][np.argmax(res['magnitude'])]
    assert pytest.approx(peak_freq, abs=1.0) == f_target
