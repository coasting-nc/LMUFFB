import pytest
import pandas as pd
import numpy as np
from lmuffb_log_analyzer.analyzers.lateral_analyzer import analyze_load_estimation
from lmuffb_log_analyzer.reports import generate_text_report
from lmuffb_log_analyzer.models import SessionMetadata
from datetime import datetime

def test_analyze_load_estimation_ratio():
    # Create dummy dataframe
    # Raw Static Load approx 4000
    # Approx Static Load approx 2000 (Incorrect but consistent MR)
    # Dynamics: load increases by 50%
    data = {
        'Time': np.linspace(0, 10, 1000),
        'Speed': np.concatenate([np.ones(500) * 10, np.ones(500) * 30]),
        'RawLoadFL': np.concatenate([np.ones(500) * 4000, np.ones(500) * 6000]),
        'RawLoadFR': np.concatenate([np.ones(500) * 4000, np.ones(500) * 6000]),
        'RawLoadRL': np.ones(1000) * 4000,
        'RawLoadRR': np.ones(1000) * 4000,
        'ApproxLoadFL': np.concatenate([np.ones(500) * 2000, np.ones(500) * 3000]),
        'ApproxLoadFR': np.concatenate([np.ones(500) * 2000, np.ones(500) * 3000]),
        'ApproxLoadRL': np.ones(1000) * 2000,
        'ApproxLoadRR': np.ones(1000) * 2000,
    }
    df = pd.DataFrame(data)

    results = analyze_load_estimation(df)

    # Absolute error should be large (~2000)
    assert abs(results['load_error_mean']) > 1500

    # Ratio error should be near 0 because both have same dynamic shape (1.0x -> 1.5x)
    assert results['ratio_error_mean'] < 0.01
    assert results['ratio_correlation'] > 0.99

def test_report_with_load_estimation():
    metadata = SessionMetadata(
        log_version="v1.2",
        timestamp=datetime.now(),
        app_version="0.7.171",
        driver_name="Test",
        vehicle_name="Test",
        track_name="Test",
        gain=1.0,
        understeer_effect=1.0,
        sop_effect=1.0,
        slope_enabled=True,
        slope_sensitivity=0.5,
        slope_threshold=-0.3,
        dynamic_normalization=True,
        auto_load_normalization=False
    )

    data = {
        'Time': [0, 1],
        'Speed': [10, 10],
        'RawLoadFL': [4000, 4000],
        'RawLoadFR': [4000, 4000],
        'RawLoadRL': [4000, 4000],
        'RawLoadRR': [4000, 4000],
        'ApproxLoadFL': [2000, 2000],
        'ApproxLoadFR': [2000, 2000],
        'ApproxLoadRL': [2000, 2000],
        'ApproxLoadRR': [2000, 2000],
    }
    df = pd.DataFrame(data)

    # Add dummy cols needed by other analyzers in report
    df['LatAccel'] = 0
    df['YawRate'] = 0
    df['FFBTotal'] = 0
    df['SlopeCurrent'] = 0
    df['SlopeSmoothed'] = 1.0

    report = generate_text_report(metadata, df)

    assert "Dynamic Norm (Torque): Enabled" in report
    assert "Auto Load Norm (Vib):  Disabled" in report
    # Note: our dummy data only has 2 rows, analyze_load_estimation needs > 100
    # Let's expand it

    df_large = pd.concat([df] * 100).reset_index()
    report_large = generate_text_report(metadata, df_large)
    assert "LOAD ESTIMATION ACCURACY" in report_large
    assert "Absolute Mean Error:" in report_large
