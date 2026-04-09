import pandas as pd
import numpy as np
from typing import Dict, Any
from ..models import SessionMetadata
from ..utils import safe_corrcoef, find_invalid_signals

def analyze_lateral_dynamics(df: pd.DataFrame, metadata: SessionMetadata) -> Dict[str, Any]:
    """
    Analyze SoP Lateral components and Load Transfer.
    """
    results = {}
    results['issues'] = []

    # Check for invalid signals
    invalid_signals = find_invalid_signals(df, ['LatLoadNorm', 'RawLatLoadNorm', 'LatAccel', 'FFBSoP'])
    if invalid_signals:
        results['issues'].append(f"Invalid values (NaN/Inf) detected in: {', '.join(invalid_signals)}")

    # 1. Raw Load Transfer
    if 'RawLoadFL' in df.columns and 'RawLoadFR' in df.columns:
        total_load = df['RawLoadFL'] + df['RawLoadFR']
        # Normalized raw load transfer [-1, 1]
        raw_lt = np.where(total_load > 1.0, (df['RawLoadFL'] - df['RawLoadFR']) / total_load, 0.0)
        df['RawLatLoadNorm'] = raw_lt

        results['raw_load_transfer_min'] = float(raw_lt.min())
        results['raw_load_transfer_max'] = float(raw_lt.max())
        results['raw_load_transfer_std'] = float(raw_lt.std())

    # 2. Compare Smoothed vs Raw Load Transfer
    if 'LatLoadNorm' in df.columns and 'RawLatLoadNorm' in df.columns:
        results['load_transfer_correlation'] = float(safe_corrcoef(df['LatLoadNorm'], df['RawLatLoadNorm']))
        # Calculate lag? (Maybe too complex for now)

    # 3. Decompose FFBSoP
    # SoP = (LatAccel_smoothed * sop_effect + LatLoadNorm * lat_load_effect) * sop_scale * gain
    # Note: We don't have LatAccel_smoothed in logs (except if we derive it from LatAccel and sop_smoothing)
    # But we have FFBSoP (total SoP force)

    if 'FFBSoP' in df.columns:
        sop_total = df['FFBSoP'].values
        results['ffb_sop_mean_abs'] = float(np.abs(sop_total).mean())

        # Estimate components if we have enough metadata
        if 'LatLoadNorm' in df.columns:
            # We assume FFBSoP already includes m_gain and m_sop_scale
            # FFBEngine.cpp:
            # double sop_base = (m_sop_lat_g_smoothed * m_sop_effect + m_sop_load_smoothed * m_lat_load_effect) * m_sop_scale;
            # ctx.sop_base_force = sop_base;
            # ...
            # double di_structural = norm_structural * (m_target_rim_nm / wheelbase_max_safe);
            # double norm_force = (di_structural + di_texture) * m_gain;

            # This makes exact decomposition hard because of the global gain and hardware scaling.
            # However, we can calculate the RATIO of contributions.

            # Normalized components (before gain and scale)
            # component_g = m_sop_lat_g_smoothed * m_sop_effect
            # component_load = m_sop_load_smoothed * m_lat_load_effect

            # Since we don't have m_sop_lat_g_smoothed, we can't perfectly subtract.
            # But we HAVE m_sop_load_smoothed (logged as LatLoadNorm).

            # Contribution from load = LatLoadNorm * lat_load_effect * sop_scale
            # (Note: AsyncLogger logs FFBSoP BEFORE final normalization and gain, wait...)
            # FFBEngine.cpp: snap.sop_force = (float)ctx.sop_unboosted_force; // unboosted = (smoothed_G * effect + smoothed_Load * effect) * scale
            # LogFrame: frame.ffb_sop = (float)ctx.sop_base_force; // sop_base = same as unboosted if no oversteer boost

            load_contribution = df['LatLoadNorm'] * metadata.lat_load_effect * metadata.sop_scale
            results['load_contribution_mean_abs'] = float(np.abs(load_contribution).mean())

            # If we assume FFBSoP = G_part + Load_part
            # G_part = FFBSoP - Load_part
            g_contribution = df['FFBSoP'] - load_contribution
            results['g_contribution_mean_abs'] = float(np.abs(g_contribution).mean())

            total_sum_abs = results['load_contribution_mean_abs'] + results['g_contribution_mean_abs']
            if total_sum_abs > 0:
                results['load_contribution_pct'] = (results['load_contribution_mean_abs'] / total_sum_abs) * 100
                results['g_contribution_pct'] = (results['g_contribution_mean_abs'] / total_sum_abs) * 100
            else:
                results['load_contribution_pct'] = 0.0
                results['g_contribution_pct'] = 0.0

    # 4. Correlation with Lateral G
    if 'LatAccel' in df.columns and 'LatLoadNorm' in df.columns:
        results['lat_g_vs_load_correlation'] = float(safe_corrcoef(df['LatAccel'], df['LatLoadNorm']))

    return results

def analyze_load_estimation(df: pd.DataFrame) -> Dict[str, Any]:
    """
    Analyze the accuracy of the Approximate Load fallback, focusing on Dynamic Ratios.
    """
    results = {}
    cols =['RawLoadFL', 'RawLoadFR', 'RawLoadRL', 'RawLoadRR',
            'ApproxLoadFL', 'ApproxLoadFR', 'ApproxLoadRL', 'ApproxLoadRR', 'Speed']

    if not all(c in df.columns for c in cols):
        return results

    # Combine front wheels for analysis
    raw_front = (df['RawLoadFL'] + df['RawLoadFR']) / 2.0
    approx_front = (df['ApproxLoadFL'] + df['ApproxLoadFR']) / 2.0

    mask = (raw_front > 1.0) & (approx_front > 1.0)

    if mask.sum() > 100:
        # 1. Absolute Error (For reference)
        error = approx_front[mask] - raw_front[mask]
        results['load_error_mean'] = float(error.mean())

        # 2. DYNAMIC RATIO ANALYSIS (What FFB actually cares about)
        # Mimic C++ logic: learn static load between 2 and 15 m/s
        static_mask = mask & (df['Speed'] > 2.0) & (df['Speed'] < 15.0)

        if static_mask.any():
            raw_static = raw_front[static_mask].mean()
            approx_static = approx_front[static_mask].mean()
        else:
            # Fallback if no slow driving occurred
            raw_static = np.percentile(raw_front[mask], 5)
            approx_static = np.percentile(approx_front[mask], 5)

        if raw_static > 1.0 and approx_static > 1.0:
            # Calculate the dynamic multipliers (e.g., 1.5x static weight)
            raw_ratio = raw_front[mask] / raw_static
            approx_ratio = approx_front[mask] / approx_static

            # How closely does the approximation match the real dynamic shape?
            ratio_error = np.abs(approx_ratio - raw_ratio)
            results['ratio_error_mean'] = float(ratio_error.mean())
            results['ratio_correlation'] = float(safe_corrcoef(raw_ratio, approx_ratio))

    return results

def analyze_longitudinal_dynamics(df: pd.DataFrame, metadata: SessionMetadata) -> Dict[str, Any]:
    results = {}
    if 'LongitudinalLoadFactor' in df.columns:
        results['long_load_factor_mean'] = float(df['LongitudinalLoadFactor'].mean())
        results['long_load_factor_max'] = float(df['LongitudinalLoadFactor'].max())
        results['long_load_factor_min'] = float(df['LongitudinalLoadFactor'].min())
        
        # Check if the multiplier is stuck (indicates uncalibrated static load)
        if results['long_load_factor_max'] - results['long_load_factor_min'] < 0.05:
            results['long_load_active'] = False
        else:
            results['long_load_active'] = True
            
    # Check for the "Straight Line Braking" physical limitation
    if 'Brake' in df.columns and 'Steering' in df.columns and 'FFBTotal' in df.columns:
        straight_brake_mask = (df['Brake'] > 0.5) & (df['Steering'].abs() < 0.05)
        if straight_brake_mask.any():
            mean_ffb = df.loc[straight_brake_mask, 'FFBTotal'].abs().mean()
            if mean_ffb < 1.0:
                results['straight_brake_issue'] = True

    # Check for missing/encrypted data using WarnBits
    # FIX: Require the warning to be active for >50% of the session to avoid false positives from curb strikes/spawns
    results['missing_data_warnings'] = []
    if 'WarnBits' in df.columns:
        warn_load_pct = (df['WarnBits'] & 0x01).astype(bool).mean()
        warn_grip_pct = (df['WarnBits'] & 0x02).astype(bool).mean()
        
        if warn_load_pct > 0.5:
            results['missing_data_warnings'].append(f"Tire Load (mTireLoad) is missing/encrypted ({warn_load_pct*100:.1f}% of session). Using kinematic fallback.")
        if warn_grip_pct > 0.5:
            results['missing_data_warnings'].append(f"Tire Grip (mGripFract) is missing/encrypted ({warn_grip_pct*100:.1f}% of session). Using slip-based fallback.")
            
    return results
