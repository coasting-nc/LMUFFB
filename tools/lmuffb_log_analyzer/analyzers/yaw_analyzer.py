import pandas as pd
import numpy as np
from typing import Dict, List, Any, Optional

def analyze_yaw_dynamics(df: pd.DataFrame, threshold: float = 1.68) -> Dict[str, Any]:
    """
    Analyze Yaw Kick dynamics, threshold crossings, and contributions.
    """
    results = {}

    # Use mapped names (CamelCase) as per loader.py
    yaw_kick_col = 'FFBYawKick'
    rear_torque_col = 'FFBRearTorque'
    raw_yaw_col = 'RawGameYawAccel'
    smoothed_yaw_col = 'SmoothedYawAccel'
    speed_col = 'Speed'
    slip_col = 'CalcSlipAngleFront'
    total_ffb_col = 'FFBTotal'
    time_col = 'Time'

    # 1. Rolling Mean (window=100) of ffb_yaw_kick and ffb_rear_torque
    if yaw_kick_col in df.columns:
        df = df.copy()
        df['FFBYawKick_rolling'] = df[yaw_kick_col].rolling(window=100, center=True).mean()
        results['ffb_yaw_kick_rolling_mean'] = float(df['FFBYawKick_rolling'].mean())
        results['ffb_yaw_kick_rolling_std'] = float(df['FFBYawKick_rolling'].std())

    if rear_torque_col in df.columns:
        if yaw_kick_col not in df.columns: df = df.copy()
        df['FFBRearTorque_rolling'] = df[rear_torque_col].rolling(window=100, center=True).mean()
        results['ffb_rear_torque_rolling_mean'] = float(df['FFBRearTorque_rolling'].mean())
        results['ffb_rear_torque_rolling_std'] = float(df['FFBRearTorque_rolling'].std())

    # 2. Straightaway Analysis for "Constant Pull"
    straight_mask = None
    if speed_col in df.columns and slip_col in df.columns:
        straight_mask = (df[speed_col] > 27.8) & (df[slip_col].abs() < 0.02)

    if straight_mask is not None and straight_mask.any():
        if 'FFBYawKick_rolling' in df.columns:
            results['straightaway_yaw_kick_mean'] = float(df.loc[straight_mask, 'FFBYawKick_rolling'].mean())
        if 'FFBRearTorque_rolling' in df.columns:
            results['straightaway_rear_torque_mean'] = float(df.loc[straight_mask, 'FFBRearTorque_rolling'].mean())

    # 3. Threshold Crossing Rate (Hz)
    if raw_yaw_col in df.columns:
        raw_yaw = df[raw_yaw_col].values
        abs_yaw = np.abs(raw_yaw)
        above = (abs_yaw > threshold).astype(int)
        # Crossing from < threshold to > threshold
        crossings = np.count_nonzero(np.diff(above) == 1)
        duration = df[time_col].iloc[-1] - df[time_col].iloc[0] if len(df) > 1 else 0
        results['threshold_crossing_rate'] = float(crossings / duration) if duration > 0 else 0.0
    else:
        results['threshold_crossing_rate'] = None

    # 4. Yaw Kick Contribution %
    if yaw_kick_col in df.columns and total_ffb_col in df.columns:
        sum_yaw = df[yaw_kick_col].abs().sum()
        sum_total = df[total_ffb_col].abs().sum()
        results['yaw_kick_contribution_pct'] = float((sum_yaw / sum_total) * 100) if sum_total > 0 else 0.0
    else:
        results['yaw_kick_contribution_pct'] = None

    # 5. Stats on SmoothedYawAccel
    if smoothed_yaw_col in df.columns:
        yaw = df[smoothed_yaw_col]
        results['yaw_accel_min'] = float(yaw.min())
        results['yaw_accel_max'] = float(yaw.max())
        results['yaw_accel_nan_count'] = int(yaw.isna().sum())
        results['yaw_accel_inf_count'] = int(np.isinf(yaw).sum())

    return results

def calculate_suspension_velocity(df: pd.DataFrame) -> pd.DataFrame:
    """
    Calculate suspension velocity: d(susp_deflection) / dt.
    """
    df = df.copy()
    cols = ['RawSuspDeflectionFL', 'RawSuspDeflectionFR', 'RawSuspDeflectionRL', 'RawSuspDeflectionRR']
    time_col = 'Time'

    if time_col not in df.columns:
        return df

    # To handle potential duplicate timestamps that cause division by zero in np.gradient,
    # we first calculate the gradient on unique timestamps and then map it back.
    df_unique = df.drop_duplicates(subset=[time_col])

    if len(df_unique) < 2:
        for col in cols:
            if col in df.columns:
                df[col.replace('Deflection', 'Velocity')] = 0.0
        return df

    for col in cols:
        if col in df.columns:
            vel_col = col.replace('Deflection', 'Velocity')

            # Calculate gradient on unique timestamps
            with np.errstate(divide='ignore', invalid='ignore'):
                unique_vel = np.gradient(df_unique[col], df_unique[time_col])

            # Map back to original dataframe
            vel_series = pd.Series(unique_vel, index=df_unique.index)
            df[vel_col] = vel_series.reindex(df.index, method='ffill').fillna(0.0)

    return df

def analyze_clipping(df: pd.DataFrame) -> Dict[str, Any]:
    """
    Analyze clipping events for FFB components.
    """
    results = {}

    if 'Clipping' in df.columns:
        results['total_clipping_pct'] = float(df['Clipping'].mean() * 100)

    components = [
        'FFBBase', 'FFBUndersteerDrop', 'FFBOversteerBoost', 'FFBSoP',
        'FFBRearTorque', 'FFBScrubDrag', 'FFBYawKick', 'FFBGyroDamping',
        'FFBRoadTexture', 'FFBSlideTexture', 'FFBLockupVibration',
        'FFBSpinVibration', 'FFBBottomingCrunch', 'FFBABSPulse', 'FFBSoftLock'
    ]

    if 'Clipping' in df.columns and df['Clipping'].any():
        clipping_df = df[df['Clipping'] == 1]
        results['clipping_component_means'] = {}
        for comp in components:
            if comp in df.columns:
                results['clipping_component_means'][comp] = float(clipping_df[comp].abs().mean())

    return results

def get_fft(signal: np.ndarray, fs: float = 400.0) -> Dict[str, np.ndarray]:
    """
    Calculate FFT of a signal. Returns frequencies and magnitudes.
    """
    if len(signal) < 2:
        return {'freqs': np.array([]), 'magnitude': np.array([])}

    # Detrend to remove DC offset
    signal_detrended = signal - np.mean(signal)

    n = len(signal_detrended)
    freqs = np.fft.rfftfreq(n, d=1/fs)
    magnitude = np.abs(np.fft.rfft(signal_detrended)) / n * 2

    return {
        'freqs': freqs,
        'magnitude': magnitude
    }
