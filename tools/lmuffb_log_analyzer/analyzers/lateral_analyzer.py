import pandas as pd
import numpy as np
from typing import Dict, Any
from ..models import SessionMetadata

def analyze_lateral_dynamics(df: pd.DataFrame, metadata: SessionMetadata) -> Dict[str, Any]:
    """
    Analyze SoP Lateral components and Load Transfer.
    """
    results = {}

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
        results['load_transfer_correlation'] = float(df['LatLoadNorm'].corr(df['RawLatLoadNorm']))
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
        results['lat_g_vs_load_correlation'] = float(df['LatAccel'].corr(df['LatLoadNorm']))

    return results
