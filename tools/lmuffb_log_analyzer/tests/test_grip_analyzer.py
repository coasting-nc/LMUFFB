import pytest
import pandas as pd
import numpy as np
from lmuffb_log_analyzer.analyzers.grip_analyzer import analyze_grip_estimation
from lmuffb_log_analyzer.models import SessionMetadata
from datetime import datetime

@pytest.fixture
def base_metadata():
    return SessionMetadata(
        log_version="1.2",
        timestamp=datetime.now(),
        app_version="0.7.172",
        driver_name="Test",
        vehicle_name="TestCar",
        track_name="TestTrack",
        gain=1.0,
        understeer_effect=1.0,
        sop_effect=1.0,
        optimal_slip_angle=0.10,
        optimal_slip_ratio=0.12,
        slope_enabled=False,
        slope_sensitivity=0.5,
        slope_threshold=-0.3
    )

def test_analyze_grip_estimation_basic(base_metadata):
    # Create synthetic data
    t = np.arange(0, 1, 0.01)
    # Case 1: Perfect grip
    df = pd.DataFrame({
        'GripFL': np.full(len(t), 1.0),
        'GripFR': np.full(len(t), 1.0),
        'CalcSlipAngleFront': np.full(len(t), 0.0),
        'SlipAngleFL': np.full(len(t), 0.0),
        'SlipAngleFR': np.full(len(t), 0.0),
        'SlipRatioFL': np.full(len(t), 0.0),
        'SlipRatioFR': np.full(len(t), 0.0)
    })

    results = analyze_grip_estimation(df, base_metadata)
    # std is 0, so should be ENCRYPTED if we don't handle constant 1.0
    # Actually, constant 1.0 has std 0.0
    assert results['status'] == "ENCRYPTED"

def test_analyze_grip_estimation_sliding(base_metadata):
    t = np.arange(0, 2, 0.01) # 200 samples

    # Simulate a slide
    # slip_angle goes from 0 to 0.2 (peak at 0.1)
    slip_angle = np.linspace(0, 0.2, len(t))

    # Ground truth: continuous 0.05 + 0.95 / (1.0 + x^4)
    min_sliding_grip = 0.05
    raw_grip = min_sliding_grip + ((1.0 - min_sliding_grip) / (1.0 + (slip_angle/0.1)**4))

    df = pd.DataFrame({
        'GripFL': raw_grip,
        'GripFR': raw_grip,
        'CalcSlipAngleFront': slip_angle,
        'SlipAngleFL': slip_angle,
        'SlipAngleFR': slip_angle,
        'SlipRatioFL': np.zeros(len(t)),
        'SlipRatioFR': np.zeros(len(t))
    })

    results = analyze_grip_estimation(df, base_metadata)
    assert results['status'] == "VALID"
    assert results['correlation'] > 0.99
    assert abs(results['mean_error_during_slip']) < 0.01
    assert 'SimulatedApproxGrip' in df.columns

def test_analyze_grip_estimation_encrypted(base_metadata):
    t = np.arange(0, 1, 0.01)
    df = pd.DataFrame({
        'GripFL': np.zeros(len(t)), # Flatlined at 0
        'GripFR': np.zeros(len(t)),
        'CalcSlipAngleFront': np.random.randn(len(t)),
        'SlipAngleFL': np.zeros(len(t)),
        'SlipAngleFR': np.zeros(len(t)),
        'SlipRatioFL': np.zeros(len(t)),
        'SlipRatioFR': np.zeros(len(t))
    })

    results = analyze_grip_estimation(df, base_metadata)
    assert results['status'] == "ENCRYPTED"
