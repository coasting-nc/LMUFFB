import pytest
import pandas as pd
import numpy as np
from pathlib import Path
from lmuffb_log_analyzer.plots import (
    plot_slope_timeseries, 
    plot_slip_vs_latg, 
    plot_dalpha_histogram,
    plot_load_estimation_diagnostic,
    plot_grip_estimation_diagnostic
)
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
        slope_enabled=True,
        slope_sensitivity=0.5,
        slope_threshold=-0.3
    )

@pytest.fixture
def sample_df():
    t = np.arange(0, 10, 0.01)
    data = {
        'Time': t,
        'LatAccel': np.sin(t) * 10,
        'Speed': np.full(len(t), 30.0),
        'calc_slip_angle_front': np.cos(t) * 0.1,
        'SlopeCurrent': np.sin(t * 10) * 5,
        'SlopeSmoothed': np.ones(len(t)),
        'GripFactor': np.ones(len(t)),
        'dAlpha_dt': np.zeros(len(t)),
        'Marker': np.zeros(len(t)),
        'dG_dt': np.zeros(len(t)),
        'RawLoadFL': 2000.0 + np.sin(t) * 100,
        'RawLoadFR': 2000.0 + np.cos(t) * 100,
        'RawLoadRL': 2000.0 + np.sin(t * 0.5) * 100,
        'RawLoadRR': 2000.0 + np.cos(t * 0.5) * 100,
        'ApproxLoadFL': 2100.0 + np.sin(t) * 90,
        'ApproxLoadFR': 2100.0 + np.cos(t) * 90,
        'ApproxLoadRL': 2100.0 + np.sin(t * 0.5) * 90,
        'ApproxLoadRR': 2100.0 + np.cos(t * 0.5) * 90
    }
    return pd.DataFrame(data)

def test_plot_generation(sample_df, base_metadata, tmp_path):
    # Add columns required for improved plots
    sample_df['CalcSlipAngleFront'] = sample_df['calc_slip_angle_front']
    sample_df['GripFL'] = sample_df['GripFactor']
    sample_df['GripFR'] = sample_df['GripFactor']
    sample_df['SimulatedApproxGrip'] = sample_df['GripFactor']

    # Test slope timeseries
    ts_path = tmp_path / "timeseries.png"
    result = plot_slope_timeseries(sample_df, base_metadata, output_path=str(ts_path), show=False)
    assert Path(result).exists()
    
    # Test tire curve
    tc_path = tmp_path / "tire_curve.png"
    result = plot_slip_vs_latg(sample_df, output_path=str(tc_path), show=False)
    assert Path(result).exists()
    
    # Test histogram
    h_path = tmp_path / "hist.png"
    result = plot_dalpha_histogram(sample_df, output_path=str(h_path), show=False)
    assert Path(result).exists()

    # Test load estimation diagnostic
    load_path = tmp_path / "load_diag.png"
    result = plot_load_estimation_diagnostic(sample_df, output_path=str(load_path), show=False)
    assert Path(result).exists()

    # Test grip estimation diagnostic
    grip_path = tmp_path / "grip_diag.png"
    result = plot_grip_estimation_diagnostic(sample_df, base_metadata, output_path=str(grip_path), show=False)
    assert Path(result).exists()

def test_plot_slip_vs_latg_legacy(sample_df, tmp_path):
    # Do NOT add CalcSlipAngleFront to simulate a legacy CSV where the column is named calc_slip_angle_front
    if 'CalcSlipAngleFront' in sample_df.columns:
        sample_df = sample_df.drop(columns=['CalcSlipAngleFront'])
        
    tc_path = tmp_path / "tire_curve_legacy.png"
    result = plot_slip_vs_latg(sample_df, output_path=str(tc_path), show=False)
    assert Path(result).exists()
    assert result == str(tc_path)
