import pandas as pd
import numpy as np
from typing import Dict, Any
from ..models import SessionMetadata
from ..utils import safe_corrcoef, find_invalid_signals

def analyze_grip_estimation(df: pd.DataFrame, metadata: SessionMetadata) -> Dict[str, Any]:
    results = {}
    results['issues'] = []

    cols =['GripFL', 'GripFR', 'SlipAngleFL', 'SlipAngleFR', 'SlipRatioFL', 'SlipRatioFR', 'Speed']
    if not all(c in df.columns for c in cols):
        return results

    # Check for invalid signals
    invalid_signals = find_invalid_signals(df, cols)
    if invalid_signals:
        results['issues'].append(f"Invalid values (NaN/Inf) detected in: {', '.join(invalid_signals)}")

    raw_front_grip = (df['GripFL'] + df['GripFR']) / 2.0

    if raw_front_grip.std() < 0.001:
        results['status'] = "ENCRYPTED"
        return results

    # Determine which load channels to use (Raw or Approx)
    if 'LoadFL' in df.columns and df['LoadFL'].max() > 100:
        load_fl = df['LoadFL']
        load_fr = df['LoadFR']
    else:
        load_fl = df['ApproxLoadFL']
        load_fr = df['ApproxLoadFR']

    # Estimate static front load (average between 2 and 15 m/s)
    speed_mask = (df['Speed'] > 2.0) & (df['Speed'] < 15.0)
    if speed_mask.any():
        static_load = ((load_fl[speed_mask] + load_fr[speed_mask]) / 2.0).mean()
    else:
        static_load = ((load_fl + load_fr) / 2.0).quantile(0.05)
    
    static_load = max(static_load, 1000.0)

    # Apply 50ms EMA smoothing to the load to match C++ curb-strike rejection
    # alpha = dt / (tau + dt) = 0.0025 / (0.050 + 0.0025) = 0.0476
    load_fl_smooth = load_fl.ewm(alpha=0.0476, adjust=False).mean()
    load_fr_smooth = load_fr.ewm(alpha=0.0476, adjust=False).mean()

    # Simulate NEW C++ Dynamic Load-Sensitive Friction Circle
    def calc_wheel_grip(slip_angle, slip_ratio, current_load):
        # 1. Dynamic Load Sensitivity
        load_ratio = np.clip(current_load / static_load, 0.25, 4.0)
        dynamic_slip_angle = metadata.optimal_slip_angle * np.power(load_ratio, 0.333)
        
        # 2. Friction Circle
        lat_metric = np.abs(slip_angle) / dynamic_slip_angle
        long_metric = np.abs(slip_ratio) / metadata.optimal_slip_ratio
        combined = np.sqrt(lat_metric**2 + long_metric**2)
        
        # 3. Continuous Falloff with 5% Asymptote
        min_sliding_grip = 0.05
        grip = min_sliding_grip + ((1.0 - min_sliding_grip) / (1.0 + (combined**4)))
        return grip

    approx_fl = calc_wheel_grip(df['SlipAngleFL'], df['SlipRatioFL'], load_fl_smooth)
    approx_fr = calc_wheel_grip(df['SlipAngleFR'], df['SlipRatioFR'], load_fr_smooth)
    
    approx_grip = (approx_fl + approx_fr) / 2.0
    approx_grip = np.clip(approx_grip, 0.0, 1.0)

    df['SimulatedApproxGrip'] = approx_grip

    # Statistical Analysis (Only during slip events)
    slip_mask = (raw_front_grip < 0.98) | (approx_grip < 0.98)

    if slip_mask.sum() > 50:
        error = approx_grip[slip_mask] - raw_front_grip[slip_mask]
        results['status'] = "VALID"
        results['mean_error_during_slip'] = float(np.abs(error).mean())
        results['std_error_during_slip'] = float(error.std())

        results['correlation'] = float(safe_corrcoef(raw_front_grip[slip_mask], approx_grip[slip_mask]))

        false_positives = (approx_grip < 0.9) & (raw_front_grip > 0.98)
        results['false_positive_rate'] = float(false_positives.mean() * 100.0)
    else:
        results['status'] = "NO_SLIP_EVENTS"

    return results