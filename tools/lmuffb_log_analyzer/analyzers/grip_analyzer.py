import pandas as pd
import numpy as np
from typing import Dict, Any
from ..models import SessionMetadata

def analyze_grip_estimation(df: pd.DataFrame, metadata: SessionMetadata) -> Dict[str, Any]:
    """
    Simulates the C++ Friction Circle fallback and compares it to Raw Telemetry Grip.
    """
    results = {}
    cols = ['GripFL', 'GripFR', 'CalcSlipAngleFront', 'SlipRatioFL', 'SlipRatioFR']
    if not all(c in df.columns for c in cols):
        return results

    # 1. Calculate Raw Front Axle Grip
    raw_front_grip = (df['GripFL'] + df['GripFR']) / 2.0

    # Check if raw grip is actually valid (not encrypted/flatlined)
    if raw_front_grip.std() < 0.001:
        results['status'] = "ENCRYPTED"
        return results

    # 2. Simulate C++ Friction Circle Approximation
    lat_metric = df['CalcSlipAngleFront'].abs() / metadata.optimal_slip_angle
    avg_ratio = (df['SlipRatioFL'].abs() + df['SlipRatioFR'].abs()) / 2.0
    long_metric = avg_ratio / metadata.optimal_slip_ratio

    combined_slip = np.sqrt(lat_metric**2 + long_metric**2)

    # C++ Logic: 1.0 / (1.0 + excess * 2.0)
    approx_grip = np.where(combined_slip > 1.0, 1.0 / (1.0 + (combined_slip - 1.0) * 2.0), 1.0)
    approx_grip = np.clip(approx_grip, 0.2, 1.0) # C++ safety floor

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
