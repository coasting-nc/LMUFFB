import pytest
import pandas as pd
import numpy as np
from pathlib import Path
from lmuffb_log_analyzer.plots import (
    plot_slope_timeseries, 
    plot_slip_vs_latg, 
    plot_dalpha_histogram,
    plot_load_estimation_diagnostic
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
        'RawLoadFL': np.full(len(t), 2000.0),
        'RawLoadFR': np.full(len(t), 2000.0),
        'RawLoadRL': np.full(len(t), 2000.0),
        'RawLoadRR': np.full(len(t), 2000.0),
        'ApproxLoadFL': np.full(len(t), 2100.0),
        'ApproxLoadFR': np.full(len(t), 2100.0),
        'ApproxLoadRL': np.full(len(t), 2100.0),
        'ApproxLoadRR': np.full(len(t), 2100.0)
    }
    return pd.DataFrame(data)

def test_plot_generation(sample_df, tmp_path):
    # Test slope timeseries
    ts_path = tmp_path / "timeseries.png"
    result = plot_slope_timeseries(sample_df, output_path=str(ts_path), show=False)
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
