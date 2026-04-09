import pytest
import numpy as np
import pandas as pd
import warnings
from datetime import datetime
from lmuffb_log_analyzer.utils import safe_corrcoef
from lmuffb_log_analyzer.analyzers.grip_analyzer import analyze_grip_estimation
from lmuffb_log_analyzer.models import SessionMetadata

def test_safe_corrcoef_edge_cases():
    # Constant data should return 0.0 and NOT trigger RuntimeWarning
    x = np.array([1, 1, 1])
    y = np.array([1, 2, 3])

    with warnings.catch_warnings(record=True) as w:
        warnings.simplefilter("always")
        res = safe_corrcoef(x, y)
        assert res == 0.0
        # Check that no RuntimeWarning was triggered
        for warning in w:
            assert not issubclass(warning.category, RuntimeWarning)

    # Empty or single-element data
    assert safe_corrcoef(np.array([]), np.array([])) == 0.0
    assert safe_corrcoef(np.array([1]), np.array([1])) == 0.0

    # Normal data should still work
    x_norm = np.array([1, 2, 3])
    y_norm = np.array([2, 4, 6])
    assert pytest.approx(safe_corrcoef(x_norm, y_norm)) == 1.0

def test_grip_analyzer_robustness():
    # Create mock data where one channel is constant
    df = pd.DataFrame({
        'GripFL': [1.0, 1.0, 1.0, 1.0],
        'GripFR': [1.0, 1.0, 1.0, 1.0],
        'SlipAngleFL': [0.1, 0.2, 0.3, 0.4],
        'SlipAngleFR': [0.1, 0.2, 0.3, 0.4],
        'SlipRatioFL': [0.01, 0.02, 0.03, 0.04],
        'SlipRatioFR': [0.01, 0.02, 0.03, 0.04],
        'Speed': [50.0, 50.0, 50.0, 50.0],
        'LoadFL': [2000.0, 2000.0, 2000.0, 2000.0],
        'LoadFR': [2000.0, 2000.0, 2000.0, 2000.0],
    })

    metadata = SessionMetadata(
        log_version="v1.2",
        timestamp=datetime.now(),
        app_version="v0.7.276",
        driver_name="Test",
        vehicle_name="Test",
        track_name="Test",
        gain=1.0,
        understeer_effect=1.0,
        sop_effect=1.0,
        slope_enabled=True,
        slope_sensitivity=0.5,
        slope_threshold=0.02,
        optimal_slip_angle=0.15,
        optimal_slip_ratio=0.1,
        lat_load_effect=0.5,
        sop_scale=1.0
    )

    # This should not raise warnings and should return correlation 0.0
    with warnings.catch_warnings(record=True) as w:
        warnings.simplefilter("always")
        results = analyze_grip_estimation(df, metadata)
        # In this mock, raw_front_grip is constant 1.0 -> correlation should be 0.0
        if results.get('status') == 'VALID':
            assert results['correlation'] == 0.0

        for warning in w:
            if "invalid value encountered in divide" in str(warning.message):
                pytest.fail("RuntimeWarning triggered in correlation calculation")
