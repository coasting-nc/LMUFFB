import pandas as pd
import numpy as np
from typing import Dict, Any
from ..models import SessionMetadata

def analyze_grip_estimation(df: pd.DataFrame, metadata: SessionMetadata) -> Dict[str, Any]:
    """
    Simulates the C++ Friction Circle fallback and compares it to Raw Telemetry Grip.
    """
    results = {}
    cols =['GripFL', 'GripFR', 'SlipAngleFL', 'SlipAngleFR', 'SlipRatioFL', 'SlipRatioFR']
    if not all(c in df.columns for c in cols):
        return results

    # 1. Calculate Raw Front Axle Grip
    raw_front_grip = (df['GripFL'] + df['GripFR']) / 2.0

    # Check if raw grip is actually valid (not encrypted/flatlined)
    if raw_front_grip.std() < 0.001:
        results['status'] = "ENCRYPTED"
        return results

    # Simulate NEW C++ Continuous Friction Circle with Sliding Floor
    def calc_wheel_grip(slip_angle, slip_ratio):
        lat_metric = np.abs(slip_angle) / metadata.optimal_slip_angle
        long_metric = np.abs(slip_ratio) / metadata.optimal_slip_ratio
        combined = np.sqrt(lat_metric**2 + long_metric**2)
        
        min_sliding_grip = 0.05
        # Continuous falloff that asymptotes to min_sliding_grip
        grip = min_sliding_grip + ((1.0 - min_sliding_grip) / (1.0 + (combined**4)))
        return grip

    approx_fl = calc_wheel_grip(df['SlipAngleFL'], df['SlipRatioFL'])
    approx_fr = calc_wheel_grip(df['SlipAngleFR'], df['SlipRatioFR'])
    
    approx_grip = (approx_fl + approx_fr) / 2.0
    approx_grip = np.clip(approx_grip, 0.0, 1.0) # Removed 0.2 floor

    df['SimulatedApproxGrip'] = approx_grip

    # 3. Statistical Analysis (Only during slip events)
    # We only care about accuracy when the car is actually sliding (Grip < 0.98)
    slip_mask = (raw_front_grip < 0.98) | (approx_grip < 0.98)

    if slip_mask.sum() > 50:
        error = approx_grip[slip_mask] - raw_front_grip[slip_mask]
        results['status'] = "VALID"
        results['mean_error_during_slip'] = float(error.mean())
        results['std_error_during_slip'] = float(error.std())

        # Pearson correlation
        if raw_front_grip[slip_mask].std() > 0 and approx_grip[slip_mask].std() > 0:
            results['correlation'] = float(np.corrcoef(raw_front_grip[slip_mask], approx_grip[slip_mask])[0, 1])
        else:
            results['correlation'] = 0.0

        # False Positive Rate: Approx says sliding (<0.9), but Raw says gripping (>0.98)
        false_positives = (approx_grip < 0.9) & (raw_front_grip > 0.98)
        results['false_positive_rate'] = float(false_positives.mean() * 100.0)
    else:
        results['status'] = "NO_SLIP_EVENTS"

    return results
