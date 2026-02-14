import pandas as pd
import numpy as np
from typing import Dict, List, Any

def analyze_slope_stability(df: pd.DataFrame, threshold: float = 0.02) -> Dict[str, Any]:
    """
    Analyze the stability of slope detection algorithm.
    """
    results = {}
    
    # Basic slope statistics
    slope = df['SlopeCurrent']
    results['slope_mean'] = float(slope.mean())
    results['slope_std'] = float(slope.std())
    results['slope_min'] = float(slope.min())
    results['slope_max'] = float(slope.max())
    results['slope_range'] = (float(slope.min()), float(slope.max()))
    results['slope_variance'] = float(slope.var())
    
    # Percentage of time slope is actively calculated
    if 'dAlpha_dt' in df.columns:
        active_mask = np.abs(df['dAlpha_dt']) > threshold
        results['active_percentage'] = float(active_mask.mean() * 100)
    else:
        results['active_percentage'] = None
    
    # Percentage of time at grip floor (0.2)
    grip_col = 'GripFactor' if 'GripFactor' in df.columns else 'SlopeSmoothed'
    if grip_col in df.columns:
        floor_mask = df[grip_col] <= 0.21
        results['floor_percentage'] = float(floor_mask.mean() * 100)
    else:
        results['floor_percentage'] = None
    
    # Grip on straights analysis
    straight_mask = (
        (df['Speed'] > 27.8) &  # > 100 km/h
        (np.abs(df.get('calc_slip_angle_front', 0)) < 0.02)
    )
    if straight_mask.any():
        results['grip_on_straights_mean'] = float(df.loc[straight_mask, grip_col].mean())
        results['grip_on_straights_std'] = float(df.loc[straight_mask, grip_col].std())
    else:
        results['grip_on_straights_mean'] = None
        results['grip_on_straights_std'] = None
    
    # Signal Quality Metrics
    # 1. Zero-Crossing Rate (Hz)
    if 'SlopeCurrent' in df.columns:
        # Count sign changes
        diffs = np.diff(np.sign(df['SlopeCurrent']))
        crossings = np.count_nonzero(diffs)
        duration = df['Time'].iloc[-1] - df['Time'].iloc[0] if 'Time' in df.columns else len(df) * 0.01
        results['zero_crossing_rate'] = float(crossings / duration) if duration > 0 else 0.0
    else:
        results['zero_crossing_rate'] = None

    # 2. Binary State Residence
    if grip_col in df.columns:
        binary_mask = (df[grip_col] <= 0.25) | (df[grip_col] >= 0.95)
        results['binary_residence'] = float(binary_mask.mean() * 100)
    else:
        results['binary_residence'] = None

    # 3. Derivative Energy Ratio
    if 'dG_dt' in df.columns and 'dAlpha_dt' in df.columns:
        std_alpha = df['dAlpha_dt'].std()
        results['derivative_energy_ratio'] = float(df['dG_dt'].std() / std_alpha) if std_alpha > 0 else 0.0
    else:
        results['derivative_energy_ratio'] = None

    # Issue detection
    results['issues'] = []
    
    if results['slope_std'] > 5.0:
        results['issues'].append(
            f"HIGH SLOPE VARIANCE ({results['slope_std']:.2f}) - Algorithm may be unstable"
        )
    
    if results.get('floor_percentage', 0) > 5.0:
        results['issues'].append(
            f"FREQUENT FLOOR HITS ({results['floor_percentage']:.1f}%) - Algorithm too aggressive"
        )
    
    if results.get('active_percentage') is not None and results['active_percentage'] < 30.0:
        results['issues'].append(
            f"LOW ACTIVE PERCENTAGE ({results['active_percentage']:.1f}%) - Slope rarely calculated"
        )
    
    if results.get('grip_on_straights_mean') is not None and results['grip_on_straights_mean'] < 0.9:
        results['issues'].append(
            f"LOW GRIP ON STRAIGHTS ({results['grip_on_straights_mean']:.2f}) - Slope stuck at negative"
        )

    if results.get('zero_crossing_rate', 0) > 5.0:
        results['issues'].append(
            f"HIGH SIGNAL NOISE ({results['zero_crossing_rate']:.1f} Hz) - Slope signal is jittery"
        )

    return results

def detect_oscillation_events(
    df: pd.DataFrame, 
    column: str = 'SlopeCurrent',
    threshold: float = 5.0,
    min_duration: float = 0.1
) -> List[Dict[str, Any]]:
    """
    Detect periods where a signal oscillates rapidly between extremes.
    """
    events = []
    
    if column not in df.columns:
        return events
    
    signal = df[column].values
    time = df['Time'].values if 'Time' in df.columns else np.arange(len(signal)) * 0.01
    
    # Calculate rolling std to detect high-variance periods
    window = 50  # 0.5 seconds at 100Hz
    rolling_std = pd.Series(signal).rolling(window, center=True).std().values
    
    # Vectorized search for events
    # Find frames where std exceeds threshold
    std_above = (rolling_std > threshold).astype(int)
    # Detect starts and ends (1 = starts, -1 = ends)
    # prepend/append 0 ensures we catch events leading to/from the edges
    diff = np.diff(std_above, prepend=0, append=0)
    starts = np.where(diff == 1)[0]
    ends = np.where(diff == -1)[0]
    
    for s, e in zip(starts, ends):
        # Index e is the point just after the event, adjust to inclusive range [s, e-1]
        actual_end = e - 1
        duration = time[actual_end] - time[s]
        
        if duration >= min_duration:
            events.append({
                'start_time': float(time[s]),
                'end_time': float(time[actual_end]),
                'duration': float(duration),
                'amplitude': float(np.abs(signal[s:e]).max()),
                'frame_start': int(s),
                'frame_end': int(actual_end)
            })
    
    return events

def analyze_grip_correlation(df: pd.DataFrame) -> Dict[str, float]:
    """
    Analyze correlation between calculated grip and expected physics.
    """
    results = {}
    
    grip_col = 'GripFactor' if 'GripFactor' in df.columns else 'SlopeSmoothed'
    
    if grip_col in df.columns:
        # Correlation with absolute slip angle
        if 'calc_slip_angle_front' in df.columns:
            slip = np.abs(df['calc_slip_angle_front'])
            results['grip_vs_slip_correlation'] = float(-df[grip_col].corr(slip))
        
        # Correlation with lateral G
        if 'LatAccel' in df.columns:
            lat_g = np.abs(df['LatAccel'])
            results['grip_vs_latg_correlation'] = float(df[grip_col].corr(lat_g))
    
    return results

def detect_singularities(
    df: pd.DataFrame,
    slope_thresh: float = 10.0,
    alpha_rate_thresh: float = 0.05
) -> (int, float):
    """
    Detect "Singularity Events" (high slope with low slip rate).
    """
    if 'SlopeCurrent' not in df.columns or 'dAlpha_dt' not in df.columns:
        return 0, 0.0

    mask = (np.abs(df['SlopeCurrent']) > slope_thresh) & (np.abs(df['dAlpha_dt']) < alpha_rate_thresh)
    count = int(mask.sum())
    worst = float(df.loc[mask, 'SlopeCurrent'].abs().max()) if count > 0 else 0.0

    return count, worst
