import pytest
import pandas as pd
import numpy as np
from lmuffb_log_analyzer.analyzers.slope_analyzer import (
    analyze_slope_stability, 
    detect_oscillation_events
)

@pytest.fixture
def stable_df():
    """A stable telemetry dataframe"""
    data = {
        'Time': np.arange(0, 10, 0.01),
        'SlopeCurrent': np.full(1000, 1.0),
        'SlopeSmoothed': np.full(1000, 1.0),
        'GripFactor': np.full(1000, 1.0),
        'dAlpha_dt': np.full(1000, 0.05),
        'Speed': np.full(1000, 30.0), # 108 km/h
        'LatAccel': np.full(1000, 9.81),
        'calc_slip_angle_front': np.full(1000, 0.01)
    }
    return pd.DataFrame(data)

@pytest.fixture
def unstable_df():
    """An unstable telemetry dataframe with oscillations and floor hits"""
    t = np.arange(0, 10, 0.01)
    slope = np.sin(t * 20) * 10 # Rapid oscillation
    
    # Force some floor hits
    grip = np.full(1000, 1.0)
    grip[100:200] = 0.2
    
    data = {
        'Time': t,
        'SlopeCurrent': slope,
        'SlopeSmoothed': grip,
        'GripFactor': grip,
        'dAlpha_dt': np.full(1000, 0.05),
        'Speed': np.full(1000, 30.0),
        'LatAccel': np.full(1000, 9.81),
        'calc_slip_angle_front': np.full(1000, 0.01)
    }
    return pd.DataFrame(data)

def test_analyze_slope_stability_stable(stable_df):
    results = analyze_slope_stability(stable_df)
    assert results['slope_std'] < 1.0
    assert results['active_percentage'] == 100.0
    assert results['floor_percentage'] == 0.0
    assert len(results['issues']) == 0

def test_analyze_slope_stability_unstable(unstable_df):
    results = analyze_slope_stability(unstable_df)
    assert results['slope_std'] > 5.0
    assert results['floor_percentage'] > 0.0
    assert any("HIGH SLOPE VARIANCE" in issue for issue in results['issues'])
    assert any("FREQUENT FLOOR HITS" in issue for issue in results['issues'])
    
    # New metrics checks
    assert 'zero_crossing_rate' in results
    assert 'binary_residence' in results
    assert 'derivative_energy_ratio' in results

    # New metrics checks
    assert 'zero_crossing_rate' in results
    assert 'binary_residence' in results
    assert 'derivative_energy_ratio' in results

    # New metrics checks
    assert 'zero_crossing_rate' in results
    assert 'binary_residence' in results
    assert 'derivative_energy_ratio' in results

    # New metrics checks
    assert 'zero_crossing_rate' in results
    assert 'binary_residence' in results
    assert 'derivative_energy_ratio' in results

def test_detect_oscillation_events(unstable_df):
    events = detect_oscillation_events(unstable_df, threshold=2.0)
    assert len(events) >= 1
    assert events[0]['duration'] > 0
    assert events[0]['amplitude'] > 2.0

def test_detect_singularities():
    from lmuffb_log_analyzer.analyzers.slope_analyzer import detect_singularities
    data = {
        'SlopeCurrent': [100.0, 5.0, -100.0, 1.0],
        'dAlpha_dt': [0.01, 0.01, 0.04, 0.1],
    }
    df = pd.DataFrame(data)
    count, worst = detect_singularities(df, slope_thresh=10.0, alpha_rate_thresh=0.05)
    assert count == 2
    assert abs(worst) == 100.0
