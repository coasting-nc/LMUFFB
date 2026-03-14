import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from pathlib import Path
from typing import Optional, List
from .models import SessionMetadata
from .analyzers.yaw_analyzer import get_fft, calculate_suspension_velocity

def _safe_legend(ax, loc='upper right'):
    """Only show legend if there are labeled artists."""
    handles, labels = ax.get_legend_handles_labels()
    if labels:
        ax.legend(loc=loc)

MAX_PLOT_POINTS = 20000

def _downsample_df(df: pd.DataFrame, max_points: int = MAX_PLOT_POINTS) -> pd.DataFrame:
    """Downsample dataframe for plotting if it exceeds max_points."""
    if len(df) <= max_points:
        return df
    
    # We want to preserve the temporal order, so we use step-based downsampling
    # instead of random sampling for time-series plots.
    step = len(df) // max_points
    return df.iloc[::step].copy()

def plot_slope_timeseries(
    df: pd.DataFrame,
    metadata: SessionMetadata,
    output_path: Optional[str] = None,
    show: bool = True,
    status_callback = None
) -> str:
    """
    Generate comprehensive time-series plot for slope detection analysis.
    """
    if status_callback: status_callback("Initializing plot...")
    fig, axes = plt.subplots(4, 1, figsize=(14, 14), sharex=True)
    fig.suptitle('Slope Detection Analysis (Dynamic Grip Estimation)', fontsize=14, fontweight='bold')

    plot_df = _downsample_df(df)
    time = plot_df['Time'] if 'Time' in plot_df.columns else np.arange(len(plot_df)) * 0.01

    # Watermark if disabled
    if not metadata.slope_enabled:
        for ax in axes:
            ax.text(0.5, 0.5, 'SLOPE DETECTION DISABLED IN THIS SESSION',
                    transform=ax.transAxes, fontsize=24, color='red',
                    alpha=0.2, ha='center', va='center', fontweight='bold')

    # Panel 1: Inputs (Lat G and Slip Angle)
    ax1 = axes[0]
    ax1.plot(time, plot_df['LatAccel'] / 9.81, label='Lateral G', color='#2196F3', alpha=0.8)
    ax1.set_ylabel('Lateral G', color='#2196F3')
    ax1.tick_params(axis='y', labelcolor='#2196F3')
    _safe_legend(ax1, loc='upper left')
    ax1.grid(True, alpha=0.3)

    ax1_twin = ax1.twinx()
    if 'CalcSlipAngleFront' in plot_df.columns:
        ax1_twin.plot(time, plot_df['CalcSlipAngleFront'], label='Slip Angle', color='#FF9800', alpha=0.8)
    ax1_twin.set_ylabel('Slip Angle (rad)', color='#FF9800')
    ax1_twin.tick_params(axis='y', labelcolor='#FF9800')
    _safe_legend(ax1_twin, loc='upper right')
    ax1.set_title('Physical Inputs: Lateral G and Slip Angle')

    # Panel 2: The Two Slopes (G-Based vs Torque-Based)
    ax2 = axes[1]
    if 'SlopeCurrent' in plot_df.columns:
        ax2.plot(time, plot_df['SlopeCurrent'], label='G-Based Slope (Lateral Saturation)', color='#9C27B0', linewidth=1.2)
    if 'SlopeTorque' in plot_df.columns:
        ax2.plot(time, plot_df['SlopeTorque'], label='Torque-Based Slope (Pneumatic Trail)', color='#00BCD4', linewidth=1.2, alpha=0.8)

    ax2.axhline(metadata.slope_threshold, color='#F44336', linestyle='--', alpha=0.5, label=f'Neg Threshold ({metadata.slope_threshold})')
    ax2.axhline(0, color='black', linestyle='-', alpha=0.3)
    ax2.set_ylabel('Slope Value')
    ax2.set_ylim(-15, 15)  # Clamp for visibility
    _safe_legend(ax2, loc='upper right')
    ax2.grid(True, alpha=0.3)
    ax2.set_title('Calculated Slopes (Torque should drop BEFORE G-Slope)')

    # Panel 3: Confidence Gate
    ax3 = axes[2]
    if 'Confidence' in plot_df.columns:
        ax3.fill_between(time, 0, plot_df['Confidence'], color='#4CAF50', alpha=0.3, label='Confidence Multiplier')
        ax3.plot(time, plot_df['Confidence'], color='#4CAF50', linewidth=1.0)
    if 'dAlpha_dt' in plot_df.columns:
        ax3_twin = ax3.twinx()
        ax3_twin.plot(time, plot_df['dAlpha_dt'].abs(), color='#FF9800', alpha=0.5, linewidth=0.8, label='abs(dAlpha/dt)')
        ax3_twin.axhline(metadata.slope_alpha_threshold or 0.02, color='red', linestyle=':', alpha=0.5, label='Alpha Threshold')
        ax3_twin.set_ylabel('Slip Rate (rad/s)', color='#FF9800')
        _safe_legend(ax3_twin, loc='upper right')

    ax3.set_ylabel('Confidence (0-1)')
    ax3.set_ylim(0, 1.1)
    _safe_legend(ax3, loc='upper left')
    ax3.grid(True, alpha=0.3)
    ax3.set_title('Algorithm Confidence (Requires active steering/slip changes)')

    # Panel 4: Output vs Ground Truth
    ax4 = axes[3]
    if 'GripFL' in plot_df.columns and 'GripFR' in plot_df.columns:
        raw_front_grip = (plot_df['GripFL'] + plot_df['GripFR']) / 2.0
        ax4.plot(time, raw_front_grip, label='Raw Game Grip (Truth)', color='#2196F3', linewidth=2.0, alpha=0.6)

    grip_col = 'GripFactor' if 'GripFactor' in plot_df.columns else 'SlopeSmoothed'
    if grip_col in plot_df.columns:
        ax4.plot(time, plot_df[grip_col], label='Slope Estimated Grip', color='#F44336', linewidth=1.5, linestyle='--')
        ax4.axhline(0.2, color='#9E9E9E', linestyle='--', alpha=0.5, label='Safety Floor (0.2)')

    ax4.set_ylabel('Grip Fraction')
    ax4.set_xlabel('Time (s)')
    ax4.set_ylim(0, 1.1)
    _safe_legend(ax4, loc='lower right')
    ax4.grid(True, alpha=0.3)
    ax4.set_title('Final Output: Estimated Grip vs Actual Game Grip')
    
    # Add markers if present
    if 'Marker' in plot_df.columns:
        marker_times = time[plot_df['Marker'] == 1]
        if len(marker_times) > 0:
            if status_callback: status_callback(f"Adding {len(marker_times)} markers...")
            for ax in axes:
                # Use vlines for performance instead of multiple axvline calls
                ax.vlines(marker_times, -100, 100, color='#E91E63', linestyle='-', alpha=0.7, linewidth=2, transform=ax.get_xaxis_transform())
    
    if status_callback: status_callback("Finalizing layout...")
    plt.tight_layout()
    
    if output_path:
        if status_callback: status_callback(f"Saving to {Path(output_path).name}...")
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        return output_path
    
    if show:
        plt.show()
    
    return ""


def plot_lateral_diagnostic(
    df: pd.DataFrame,
    metadata: SessionMetadata,
    output_path: Optional[str] = None,
    show: bool = True,
    status_callback = None
) -> str:
    """
    Diagnostic plot for Lateral Load effect.
    Panels: Input (G vs Load), Force Decomposition, G vs Load scatter.
    """
    cols = ['LatAccel', 'LatLoadNorm', 'FFBSoP']
    if not all(c in df.columns for c in cols):
        return ""

    if status_callback: status_callback("Initializing Lateral diagnostic plot...")
    fig = plt.figure(figsize=(14, 12))
    gs = fig.add_gridspec(3, 2)
    fig.suptitle('SoP Lateral & Load Transfer Diagnostic', fontsize=14, fontweight='bold')

    plot_df = _downsample_df(df)
    time = plot_df['Time']

    # Panel 1: Inputs (Time Series)
    if status_callback: status_callback("Rendering Panel 1 (Inputs)...")
    ax1 = fig.add_subplot(gs[0, :])
    ax1.plot(time, plot_df['LatAccel'] / 9.81, label='Lateral G', color='#2196F3', alpha=0.6)
    ax1.set_ylabel('Lateral G', color='#2196F3')
    ax1.tick_params(axis='y', labelcolor='#2196F3')
    ax1.grid(True, alpha=0.3)

    ax1_twin = ax1.twinx()
    ax1_twin.plot(time, plot_df['LatLoadNorm'], label='Smoothed Load Transfer', color='#FF9800', alpha=0.8)
    if 'RawLoadFL' in df.columns and 'RawLoadFR' in df.columns:
        total_load = plot_df['RawLoadFL'] + plot_df['RawLoadFR']
        raw_lt = (plot_df['RawLoadFL'] - plot_df['RawLoadFR']) / (total_load + 1e-6)
        ax1_twin.plot(time, raw_lt, label='Raw Load Transfer', color='#9E9E9E', alpha=0.3, linewidth=0.5)

    ax1_twin.set_ylabel('Norm. Load Transfer [-1, 1]', color='#FF9800')
    ax1_twin.tick_params(axis='y', labelcolor='#FF9800')
    ax1.set_title('Inputs: Lateral Acceleration vs. Tire Load Transfer')

    # Combine legends
    lines, labels = ax1.get_legend_handles_labels()
    lines2, labels2 = ax1_twin.get_legend_handles_labels()
    ax1.legend(lines + lines2, labels + labels2, loc='upper right')

    # Panel 2: Force Decomposition (Time Series)
    if status_callback: status_callback("Rendering Panel 2 (Decomposition)...")
    ax2 = fig.add_subplot(gs[1, :], sharex=ax1)

    load_part = plot_df['LatLoadNorm'] * metadata.lat_load_effect * metadata.sop_scale
    g_part = plot_df['FFBSoP'] - load_part

    ax2.fill_between(time, 0, g_part, color='#2196F3', alpha=0.3, label='G-Force Component')
    ax2.fill_between(time, g_part, plot_df['FFBSoP'], color='#FF9800', alpha=0.3, label='Lateral Load Component')
    ax2.plot(time, plot_df['FFBSoP'], color='black', linewidth=1.0, label='Total SoP Force')

    ax2.set_ylabel('Force (Nm)')
    ax2.set_xlabel('Time (s)')
    ax2.set_title('SoP Force Decomposition')
    ax2.grid(True, alpha=0.3)
    ax2.legend(loc='upper right')

    # Panel 3: Scatter Plot (G vs Load)
    if status_callback: status_callback("Rendering Panel 3 (Balance)...")
    ax3 = fig.add_subplot(gs[2, 0])
    ax3.scatter(plot_df['LatAccel'] / 9.81, plot_df['LatLoadNorm'],
                c=plot_df['Speed'], cmap='viridis', alpha=0.2, s=5)
    ax3.set_xlabel('Lateral G')
    ax3.set_ylabel('Norm. Load Transfer')
    ax3.set_title('Correlation: Lat G vs Load Transfer')
    ax3.grid(True, alpha=0.3)

    # Panel 4: Distribution
    if status_callback: status_callback("Rendering Panel 4 (Distribution)...")
    ax4 = fig.add_subplot(gs[2, 1])
    ax4.hist(df['LatLoadNorm'], bins=50, color='#FF9800', alpha=0.7)
    ax4.set_xlabel('Norm. Load Transfer')
    ax4.set_ylabel('Frequency')
    ax4.set_title('Load Transfer Distribution')
    ax4.grid(True, alpha=0.3)

    plt.tight_layout()

    if output_path:
        if status_callback: status_callback(f"Saving to {Path(output_path).name}...")
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        return output_path

    if show:
        plt.show()

    return ""


def plot_yaw_diagnostic(
    df: pd.DataFrame,
    threshold: float = 1.68,
    output_path: Optional[str] = None,
    show: bool = True,
    status_callback = None
) -> str:
    """
    Comprehensive Yaw Kick diagnostic plot (Issue #253).
    Panels: Yaw Accel (Raw/Smoothed/Threshold), Yaw Rate vs Accel, Yaw Kick FFB.
    """
    cols = ['RawGameYawAccel', 'SmoothedYawAccel', 'FFBYawKick', 'YawRate']
    if not all(c in df.columns for c in cols):
        return ""

    if status_callback: status_callback("Initializing Yaw diagnostic plot...")
    fig, axes = plt.subplots(3, 1, figsize=(14, 15), sharex=True)
    fig.suptitle('Yaw Kick Dynamics Diagnostic', fontsize=14, fontweight='bold')

    plot_df = _downsample_df(df)
    time = plot_df['Time']

    # Panel 1: Yaw Acceleration & Threshold
    ax1 = axes[0]
    ax1.plot(time, plot_df['RawGameYawAccel'], label='Raw Yaw Accel', color='#9E9E9E', alpha=0.4, linewidth=0.5)
    if 'ExtrapolatedYawAccel' in plot_df.columns:
        ax1.plot(time, plot_df['ExtrapolatedYawAccel'], label='Extrapolated Yaw Accel', color='#2196F3', alpha=0.4, linewidth=0.5)
    if 'DerivedYawAccel' in plot_df.columns:
        ax1.plot(time, plot_df['DerivedYawAccel'], label='Derived Yaw Accel', color='#9C27B0', alpha=0.4, linewidth=0.5)

    ax1.plot(time, plot_df['SmoothedYawAccel'], label='Smoothed Yaw Accel', color='#F44336', linewidth=1.0)
    ax1.axhline(threshold, color='black', linestyle='--', alpha=0.6, label=f'Threshold ({threshold})')
    ax1.axhline(-threshold, color='black', linestyle='--', alpha=0.6)
    ax1.set_ylabel('Accel (rad/s²)')
    ax1.set_title('Yaw Acceleration & Binary Threshold')
    ax1.grid(True, alpha=0.3)
    _safe_legend(ax1)

    # Panel 2: Yaw Rate vs Accel Overlay
    ax2 = axes[1]
    ax2.plot(time, plot_df['YawRate'], label='Yaw Rate', color='#2196F3', linewidth=1.0)
    ax2.set_ylabel('Rate (rad/s)', color='#2196F3')
    ax2.tick_params(axis='y', labelcolor='#2196F3')

    ax2_twin = ax2.twinx()
    ax2_twin.plot(time, plot_df['SmoothedYawAccel'], label='Smoothed Yaw Accel', color='#F44336', alpha=0.5, linewidth=0.8)
    ax2_twin.set_ylabel('Accel (rad/s²)', color='#F44336')
    ax2_twin.tick_params(axis='y', labelcolor='#F44336')

    ax2.set_title('Overlay: Yaw Rate vs Smoothed Yaw Accel')
    ax2.grid(True, alpha=0.3)

    # Combined legend for twin axes
    lines, labels = ax2.get_legend_handles_labels()
    lines2, labels2 = ax2_twin.get_legend_handles_labels()
    ax2.legend(lines + lines2, labels + labels2, loc='upper right')

    # Panel 3: Yaw Kick FFB
    ax3 = axes[2]
    ax3.plot(time, plot_df['FFBYawKick'], label='Yaw Kick FFB', color='#4CAF50', linewidth=1.0)
    ax3.axhline(0, color='black', linestyle='-', alpha=0.3)
    ax3.set_ylabel('Force (Nm)')
    ax3.set_xlabel('Time (s)')
    ax3.set_title('Yaw Kick FFB Contribution')
    ax3.grid(True, alpha=0.3)
    _safe_legend(ax3)

    plt.tight_layout()

    if output_path:
        if status_callback: status_callback(f"Saving to {Path(output_path).name}...")
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        return output_path
    if show: plt.show()
    return ""


def plot_load_estimation_diagnostic(
    df: pd.DataFrame,
    output_path: Optional[str] = None,
    show: bool = True,
    status_callback = None
) -> str:
    cols =['RawLoadFL', 'RawLoadFR', 'RawLoadRL', 'RawLoadRR',
            'ApproxLoadFL', 'ApproxLoadFR', 'ApproxLoadRL', 'ApproxLoadRR', 'Speed']
    if not all(c in df.columns for c in cols):
        return ""

    if status_callback: status_callback("Initializing Load Estimation diagnostic plot...")

    # Create a large 4x2 grid
    fig = plt.figure(figsize=(16, 20))
    gs = fig.add_gridspec(4, 2)
    fig.suptitle('Tire Load Estimation: Absolute vs Dynamic Ratio', fontsize=16, fontweight='bold')

    plot_df = _downsample_df(df)
    time = plot_df['Time']

    # --- Helper function to plot individual wheels ---
    def plot_wheel(ax, raw_col, approx_col, title):
        ax.plot(time, plot_df[raw_col], label='Raw Load', color='#2196F3', alpha=0.8, linewidth=1.5)
        ax.plot(time, plot_df[approx_col], label='Approx Load', color='#F44336', alpha=0.8, linestyle='--', linewidth=1.5)
        ax.set_title(title)
        ax.set_ylabel('Load (N)')
        ax.grid(True, alpha=0.3)
        _safe_legend(ax, loc='upper right')

    # Row 1: Front Wheels (Absolute)
    plot_wheel(fig.add_subplot(gs[0, 0]), 'RawLoadFL', 'ApproxLoadFL', 'Front Left (FL) Absolute Load')
    plot_wheel(fig.add_subplot(gs[0, 1]), 'RawLoadFR', 'ApproxLoadFR', 'Front Right (FR) Absolute Load')

    # Row 2: Rear Wheels (Absolute)
    plot_wheel(fig.add_subplot(gs[1, 0]), 'RawLoadRL', 'ApproxLoadRL', 'Rear Left (RL) Absolute Load')
    plot_wheel(fig.add_subplot(gs[1, 1]), 'RawLoadRR', 'ApproxLoadRR', 'Rear Right (RR) Absolute Load')

    # --- Helper function to plot Dynamic Ratios ---
    def plot_dynamic_ratio(ax, raw_avg, approx_avg, title):
        # Mimic C++ static load learning (average load between 2 and 15 m/s)
        static_mask = (plot_df['Speed'] > 2.0) & (plot_df['Speed'] < 15.0)
        if static_mask.any():
            raw_static = raw_avg[static_mask].mean()
            approx_static = approx_avg[static_mask].mean()
        else:
            raw_static = np.percentile(raw_avg, 5)
            approx_static = np.percentile(approx_avg, 5)

        # Prevent division by zero
        raw_static = max(raw_static, 1.0)
        approx_static = max(approx_static, 1.0)

        raw_ratio = raw_avg / raw_static
        approx_ratio = approx_avg / approx_static

        # Calculate metrics for the text box
        valid_mask = (raw_avg > 1.0) & (approx_avg > 1.0)
        if valid_mask.sum() > 100:
            ratio_error_mean = np.abs(approx_ratio[valid_mask] - raw_ratio[valid_mask]).mean()
            ratio_corr = np.corrcoef(raw_ratio[valid_mask], approx_ratio[valid_mask])[0, 1]
            stats_text = f"Dynamic Corr: {ratio_corr:.3f}\nMean Error: {ratio_error_mean:.3f}x"
        else:
            stats_text = "Stats: N/A (Insufficient Data)"

        ax.plot(time, raw_ratio, label='Raw Ratio (x)', color='#2196F3', linewidth=1.5)
        ax.plot(time, approx_ratio, label='Approx Ratio (x)', color='#F44336', linestyle='--', linewidth=1.5)
        ax.axhline(1.0, color='black', linestyle=':', alpha=0.5, label='Static Weight (1.0x)')

        ax.set_title(title)
        ax.set_ylabel('Multiplier (x Static)')
        ax.grid(True, alpha=0.3)
        _safe_legend(ax, loc='upper right')

        # Add stats text box in the upper left
        props = dict(boxstyle='round', facecolor='white', alpha=0.8, edgecolor='gray')
        ax.text(0.02, 0.95, stats_text, transform=ax.transAxes, fontsize=10,
                verticalalignment='top', bbox=props)

    # Row 3: Dynamic Ratios (The FFB Truth)
    raw_front = (plot_df['RawLoadFL'] + plot_df['RawLoadFR']) / 2.0
    approx_front = (plot_df['ApproxLoadFL'] + plot_df['ApproxLoadFR']) / 2.0
    plot_dynamic_ratio(fig.add_subplot(gs[2, 0]), raw_front, approx_front, 'Front Axle Dynamic Ratio (What FFB Feels)')

    raw_rear = (plot_df['RawLoadRL'] + plot_df['RawLoadRR']) / 2.0
    approx_rear = (plot_df['ApproxLoadRL'] + plot_df['ApproxLoadRR']) / 2.0
    plot_dynamic_ratio(fig.add_subplot(gs[2, 1]), raw_rear, approx_rear, 'Rear Axle Dynamic Ratio (What FFB Feels)')

    # --- Row 4: Restored Statistical Plots ---
    raw_all = pd.concat([plot_df['RawLoadFL'], plot_df['RawLoadFR'], plot_df['RawLoadRL'], plot_df['RawLoadRR']])
    approx_all = pd.concat([plot_df['ApproxLoadFL'], plot_df['ApproxLoadFR'], plot_df['ApproxLoadRL'], plot_df['ApproxLoadRR']])

    # Filter out zeros (where game might not have provided data yet)
    mask = (raw_all > 1.0) & (approx_all > 1.0)
    raw_filt = raw_all[mask]
    approx_filt = approx_all[mask]

    # Panel 7: Correlation Scatter
    ax_corr = fig.add_subplot(gs[3, 0])
    if len(raw_filt) > 0:
        correlation = np.corrcoef(raw_filt, approx_filt)[0, 1]
        ax_corr.scatter(raw_filt, approx_filt, alpha=0.1, s=2, color='#9C27B0')

        # Identity line
        lims = [
            np.min([ax_corr.get_xlim()[0], ax_corr.get_ylim()[0]]),
            np.max([ax_corr.get_xlim()[1], ax_corr.get_ylim()[1]]),
        ]
        ax_corr.plot(lims, lims, 'k-', alpha=0.5, zorder=0, label='Identity Line')
        ax_corr.set_title(f'Absolute Correlation (All Wheels)\nr = {correlation:.3f}')
    ax_corr.set_xlabel('Raw Load (N)')
    ax_corr.set_ylabel('Approx Load (N)')
    ax_corr.grid(True, alpha=0.3)

    # Panel 8: Error Distribution
    ax_err = fig.add_subplot(gs[3, 1])
    if len(raw_filt) > 0:
        error_filt = approx_filt - raw_filt
        ax_err.hist(error_filt, bins=50, color='#607D8B', alpha=0.7, edgecolor='white')
        mean_err = error_filt.mean()
        std_err = error_filt.std()
        ax_err.axvline(mean_err, color='red', linestyle='--', label=f'Mean: {mean_err:.1f}N')
        ax_err.set_title(f'Absolute Error Distribution (Approx - Raw)\nStd Dev: {std_err:.1f}N')
        ax_err.legend()
    ax_err.set_xlabel('Error (N)')
    ax_err.set_ylabel('Frequency')
    ax_err.grid(True, alpha=0.3)

    plt.tight_layout()

    if output_path:
        if status_callback: status_callback(f"Saving to {Path(output_path).name}...")
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        return output_path
    if show: plt.show()
    return ""


def plot_grip_estimation_diagnostic(
    df: pd.DataFrame,
    metadata: SessionMetadata,
    output_path: Optional[str] = None,
    show: bool = True,
    status_callback = None
) -> str:
    if 'SimulatedApproxGrip' not in df.columns or 'GripFL' not in df.columns:
        return ""

    if status_callback: status_callback("Initializing Grip Estimation plot...")
    fig, axes = plt.subplots(2, 1, figsize=(14, 10), sharex=True)
    fig.suptitle('Tire Grip Estimation: Raw Telemetry vs Friction Circle Fallback', fontsize=14, fontweight='bold')

    plot_df = _downsample_df(df)
    time = plot_df['Time'] if 'Time' in plot_df.columns else np.arange(len(plot_df)) * 0.01
    raw_front_grip = (plot_df['GripFL'] + plot_df['GripFR']) / 2.0
    approx_grip = plot_df['SimulatedApproxGrip']

    # --- Calculate Stats for the Text Box ---
    slip_mask = (raw_front_grip < 0.98) | (approx_grip < 0.98)
    if slip_mask.sum() > 50:
        error_array = approx_grip[slip_mask] - raw_front_grip[slip_mask]
        mean_err = error_array.mean()
        if raw_front_grip[slip_mask].std() > 0 and approx_grip[slip_mask].std() > 0:
            corr = np.corrcoef(raw_front_grip[slip_mask], approx_grip[slip_mask])[0, 1]
        else:
            corr = 0.0
        stats_text = f"During Slip Events:\nCorrelation (r): {corr:.3f}\nMean Error: {mean_err:.3f}"
    else:
        stats_text = "Not enough slip events\nto calculate stats."

    # --- Panel 1: Time Series Overlay ---
    ax1 = axes[0]
    ax1.plot(time, raw_front_grip, label='Raw Game Grip (Truth)', color='#2196F3', linewidth=2.0, alpha=0.8)
    ax1.plot(time, approx_grip, label='Approximated Grip (Fallback)', color='#F44336', linestyle='--', linewidth=1.5)

    ax1.set_ylabel('Grip Fraction (0.0 - 1.0)')
    ax1.set_ylim(0.0, 1.1)
    ax1.set_title('Front Axle Grip Loss Events')
    ax1.grid(True, alpha=0.3)
    
    # Add Stats Text Box
    props = dict(boxstyle='round', facecolor='white', alpha=0.8, edgecolor='gray')
    ax1.text(0.02, 0.15, stats_text, transform=ax1.transAxes, fontsize=10,
            verticalalignment='bottom', bbox=props)

    # Add Slip Angle Overlay on Twin Axis
    if 'CalcSlipAngleFront' in plot_df.columns:
        ax1_twin = ax1.twinx()
        ax1_twin.plot(time, np.abs(plot_df['CalcSlipAngleFront']), label='Absolute Slip Angle', color='#FF9800', alpha=0.4, linewidth=1.0)
        ax1_twin.axhline(metadata.optimal_slip_angle, color='#FF9800', linestyle=':', alpha=0.6, label=f'Optimal Threshold ({metadata.optimal_slip_angle})')
        ax1_twin.set_ylabel('Slip Angle (rad)', color='#FF9800')
        ax1_twin.tick_params(axis='y', labelcolor='#FF9800')
        ax1_twin.set_ylim(0, 0.25) # Keep scale consistent with scatter plot
        
        # Combine legends
        lines, labels = ax1.get_legend_handles_labels()
        lines2, labels2 = ax1_twin.get_legend_handles_labels()
        ax1.legend(lines + lines2, labels + labels2, loc='lower right')
    else:
        _safe_legend(ax1, loc='lower right')

    # --- Panel 2: Error Delta ---
    ax2 = axes[1]
    error = approx_grip - raw_front_grip
    ax2.fill_between(time, 0, error, where=(error > 0), color='#F44336', alpha=0.3, label='Over-estimating Grip')
    ax2.fill_between(time, 0, error, where=(error < 0), color='#2196F3', alpha=0.3, label='Under-estimating Grip')
    ax2.plot(time, error, color='black', linewidth=0.5)

    ax2.axhline(0, color='black', linestyle='-', alpha=0.5)
    ax2.set_ylabel('Error Delta')
    ax2.set_xlabel('Time (s)')
    ax2.set_ylim(-0.5, 0.5)
    ax2.set_title('Approximation Error (Closer to 0 is better)')
    ax2.grid(True, alpha=0.3)
    _safe_legend(ax2, loc='upper right')

    plt.tight_layout()

    if output_path:
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        return output_path
    if show: plt.show()
    return ""


def plot_system_health(
    df: pd.DataFrame,
    output_path: Optional[str] = None,
    show: bool = True,
    status_callback = None
) -> str:
    """
    Plot DeltaTime and Clipping indicators to detect frame drops and saturation.
    """
    if 'DeltaTime' not in df.columns:
        return ""

    if status_callback: status_callback("Initializing System Health plot...")
    fig, axes = plt.subplots(2, 1, figsize=(14, 8), sharex=True)
    fig.suptitle('System Health: Timing & Clipping', fontsize=14, fontweight='bold')

    plot_df = _downsample_df(df)
    time = plot_df['Time']

    # Panel 1: DeltaTime
    ax1 = axes[0]
    ax1.plot(time, plot_df['DeltaTime'], color='#673AB7', linewidth=0.8)
    ax1.axhline(0.0025, color='#4CAF50', linestyle='--', alpha=0.6, label='Target (400Hz)')
    ax1.set_ylabel('DeltaTime (s)')
    ax1.set_title('Time Consistency (Look for spikes/drops)')
    ax1.grid(True, alpha=0.3)
    _safe_legend(ax1)

    # Panel 2: Clipping
    ax2 = axes[1]
    if 'Clipping' in plot_df.columns:
        ax2.fill_between(time, 0, plot_df['Clipping'], color='#F44336', alpha=0.3, label='Clipping Active')

    # Overlay total FFB to see context
    if 'FFBTotal' in plot_df.columns:
        ax2_twin = ax2.twinx()
        ax2_twin.plot(time, plot_df['FFBTotal'], color='black', alpha=0.5, linewidth=0.5, label='Total FFB')
        ax2_twin.set_ylabel('Total FFB (Nm)')
        _safe_legend(ax2_twin, loc='upper right')

    ax2.set_ylabel('Status (Binary)')
    ax2.set_ylim(-0.1, 1.1)
    ax2.set_xlabel('Time (s)')
    ax2.set_title('FFB Clipping Events')
    ax2.grid(True, alpha=0.3)
    _safe_legend(ax2, loc='upper left')

    plt.tight_layout()

    if output_path:
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        return output_path
    if show: plt.show()
    return ""


def plot_threshold_thrashing(
    df: pd.DataFrame,
    threshold: float = 1.68,
    window_sec: float = 3.0,
    output_path: Optional[str] = None,
    show: bool = True,
    status_callback = None
) -> str:
    """
    Zoomed-in plot showing Raw Yaw vs Smoothed Yaw vs Threshold (Issue #253).
    Finds the period with highest threshold crossing rate.
    """
    cols = ['RawGameYawAccel', 'SmoothedYawAccel', 'Time']
    if not all(c in df.columns for c in cols):
        return ""

    if status_callback: status_callback("Detecting thrashing window...")

    # Calculate crossings in rolling window to find "noisiest" part
    fs = 400.0
    window_samples = int(window_sec * fs)

    raw_yaw = df['RawGameYawAccel'].values
    above = (np.abs(raw_yaw) > threshold).astype(int)
    crossings = np.abs(np.diff(above))

    crossing_counts = pd.Series(crossings).rolling(window=window_samples).sum()
    max_idx = crossing_counts.idxmax()

    if pd.isna(max_idx):
        start_idx, end_idx = 0, min(window_samples, len(df)-1)
    else:
        start_idx = max(0, max_idx - window_samples)
        end_idx = min(len(df)-1, max_idx)

    plot_df = df.iloc[start_idx:end_idx]
    time = plot_df['Time']

    fig, ax = plt.subplots(figsize=(12, 6))
    ax.plot(time, plot_df['RawGameYawAccel'], label='Raw Yaw Accel', color='#9E9E9E', alpha=0.4, linewidth=0.7)
    ax.plot(time, plot_df['SmoothedYawAccel'], label='Smoothed Yaw Accel', color='#F44336', linewidth=1.5)
    ax.axhline(threshold, color='black', linestyle='--', alpha=0.8, label=f'Threshold ({threshold})')
    ax.axhline(-threshold, color='black', linestyle='--')

    ax.set_title(f'Threshold Thrashing Analysis (Zoomed {window_sec}s)')
    ax.set_xlabel('Time (s)')
    ax.set_ylabel('Yaw Accel (rad/s²)')
    ax.grid(True, alpha=0.3)
    _safe_legend(ax)

    if output_path:
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        return output_path
    if show: plt.show()
    return ""


def plot_suspension_yaw_correlation(
    df: pd.DataFrame,
    output_path: Optional[str] = None,
    show: bool = True,
    status_callback = None
) -> str:
    """
    Scatter plot of Suspension Velocity vs Raw Yaw Acceleration.
    """
    df_vel = calculate_suspension_velocity(df)
    vel_cols = [c for c in df_vel.columns if 'Velocity' in c]
    if not vel_cols or 'RawGameYawAccel' not in df.columns:
        return ""

    if status_callback: status_callback("Rendering suspension correlation...")

    plot_df = _downsample_df(df_vel, max_points=10000)

    fig, axes = plt.subplots(2, 2, figsize=(14, 12))
    fig.suptitle('Correlation: Suspension Velocity vs Raw Yaw Acceleration', fontsize=14, fontweight='bold')

    axes = axes.flatten()
    for i, col in enumerate(vel_cols):
        ax = axes[i]
        scatter = ax.scatter(plot_df[col], plot_df['RawGameYawAccel'].abs(),
                            alpha=0.2, s=5, c=plot_df['Speed'], cmap='plasma')
        ax.set_title(col.replace('RawSuspVelocity', ''))
        ax.set_xlabel('Susp Velocity (m/s)')
        ax.set_ylabel('abs(Raw Yaw Accel)')
        ax.grid(True, alpha=0.3)

    plt.tight_layout(rect=[0, 0.03, 1, 0.95])

    if output_path:
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        return output_path
    if show: plt.show()
    return ""


def plot_bottoming_diagnostic(
    df: pd.DataFrame,
    output_path: Optional[str] = None,
    show: bool = True,
    status_callback = None
) -> str:
    """
    Overlay plot of Ride Height vs Yaw Kick FFB.
    """
    rh_cols = ['RawRideHeightFL', 'RawRideHeightFR', 'RawRideHeightRL', 'RawRideHeightRR']
    if not any(c in df.columns for c in rh_cols) or 'FFBYawKick' not in df.columns:
        return ""

    if status_callback: status_callback("Rendering bottoming diagnostic...")
    plot_df = _downsample_df(df)
    time = plot_df['Time']

    fig, ax1 = plt.subplots(figsize=(14, 8))
    for col in rh_cols:
        if col in plot_df.columns:
            ax1.plot(time, plot_df[col], label=col.replace('RawRideHeight', ''), alpha=0.7, linewidth=0.8)

    ax1.set_ylabel('Ride Height (m)')
    ax1.set_title('Bottoming Out vs Yaw Kick FFB')
    ax1.grid(True, alpha=0.3)
    _safe_legend(ax1, loc='upper left')

    ax2 = ax1.twinx()
    ax2.plot(time, plot_df['FFBYawKick'], color='black', linewidth=1.2, label='Yaw Kick FFB')
    ax2.set_ylabel('Yaw Kick FFB (Nm)')
    _safe_legend(ax2, loc='upper right')

    if output_path:
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        return output_path
    if show: plt.show()
    return ""


def plot_yaw_fft(
    df: pd.DataFrame,
    output_path: Optional[str] = None,
    show: bool = True,
    status_callback = None
) -> str:
    """
    Plot Frequency Spectrum (FFT) of Yaw Acceleration.
    """
    if 'RawGameYawAccel' not in df.columns:
        return ""

    if status_callback: status_callback("Calculating FFT...")
    signal = df['RawGameYawAccel'].values
    fft_res = get_fft(signal, fs=400.0)

    fig, ax = plt.subplots(figsize=(12, 6))
    ax.semilogy(fft_res['freqs'], fft_res['magnitude'], color='#E91E63', linewidth=0.8)
    ax.set_title('Frequency Spectrum of Raw Yaw Acceleration')
    ax.set_xlabel('Frequency (Hz)')
    ax.set_ylabel('Magnitude (Log Scale)')
    ax.grid(True, alpha=0.3, which='both')

    # Highlight regions of interest
    ax.axvspan(1, 3, color='green', alpha=0.1, label='Driver/Handling (1-3Hz)')
    ax.axvspan(10, 25, color='orange', alpha=0.1, label='Suspension (10-25Hz)')
    ax.axvspan(50, 100, color='red', alpha=0.1, label='Aliasing/Noise (50Hz+)')

    _safe_legend(ax)

    if output_path:
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        return output_path
    if show: plt.show()
    return ""


def plot_pull_detector(
    df: pd.DataFrame,
    output_path: Optional[str] = None,
    show: bool = True,
    status_callback = None
) -> str:
    """
    Generate DC Offset / Pull Detector plot (Issue #271).
    Calculates a 0.5s rolling average of Smoothed Yaw Accel and Yaw Kick FFB.
    """
    cols = ['SmoothedYawAccel', 'FFBYawKick']
    if not all(c in df.columns for c in cols):
        return ""

    if status_callback: status_callback("Calculating rolling averages for Pull Detector...")

    # 0.5s window at 400Hz = 200 samples
    window = 200

    # Use full dataframe for rolling average to avoid edge artifacts from downsampling
    # then downsample the result
    plot_df = df.copy()
    plot_df['YawAccel_RA'] = plot_df['SmoothedYawAccel'].rolling(window=window, center=True).mean()
    plot_df['YawKick_RA'] = plot_df['FFBYawKick'].rolling(window=window, center=True).mean()

    if status_callback: status_callback("Downsampling for plot...")
    plot_df = _downsample_df(plot_df)
    time = plot_df['Time']

    fig, ax1 = plt.subplots(figsize=(14, 8))
    fig.suptitle('DC Offset / Pull Detector (0.5s Rolling Average)', fontsize=14, fontweight='bold')

    ax1.plot(time, plot_df['YawAccel_RA'], label='Smoothed Yaw Accel (Rolling Avg)', color='#F44336', linewidth=1.5)
    ax1.axhline(0, color='black', linestyle='-', alpha=0.3)
    ax1.set_ylabel('Yaw Accel (rad/s²)', color='#F44336')
    ax1.tick_params(axis='y', labelcolor='#F44336')
    ax1.grid(True, alpha=0.3)

    ax2 = ax1.twinx()
    ax2.plot(time, plot_df['YawKick_RA'], label='Yaw Kick FFB (Rolling Avg)', color='#4CAF50', linewidth=1.5)
    ax2.set_ylabel('Force (Nm)', color='#4CAF50')
    ax2.tick_params(axis='y', labelcolor='#4CAF50')

    ax1.set_xlabel('Time (s)')

    # Combined legend
    lines, labels = ax1.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()
    ax1.legend(lines + lines2, labels + labels2, loc='upper right')

    plt.tight_layout()

    if output_path:
        if status_callback: status_callback(f"Saving to {Path(output_path).name}...")
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        return output_path
    if show: plt.show()
    return ""


def plot_unopposed_force(
    df: pd.DataFrame,
    output_path: Optional[str] = None,
    show: bool = True,
    status_callback = None
) -> str:
    """
    Generate Unopposed Force Overlay plot (Issue #271).
    Overlays GripFactor and FFBYawKick.
    """
    cols = ['GripFactor', 'FFBYawKick']
    if not all(c in df.columns for c in cols):
        return ""

    if status_callback: status_callback("Rendering Unopposed Force plot...")
    plot_df = _downsample_df(df)
    time = plot_df['Time']

    fig, ax1 = plt.subplots(figsize=(14, 8))
    fig.suptitle('Unopposed Force Analysis: Grip Factor vs Yaw Kick', fontsize=14, fontweight='bold')

    # Panel 1: Grip Factor (Background fill)
    ax1.fill_between(time, 0, plot_df['GripFactor'], color='#2196F3', alpha=0.2, label='Grip Factor')
    ax1.plot(time, plot_df['GripFactor'], color='#2196F3', linewidth=0.8, alpha=0.5)
    ax1.set_ylabel('Grip Factor (0.0 - 1.0)', color='#2196F3')
    ax1.set_ylim(0, 1.1)
    ax1.tick_params(axis='y', labelcolor='#2196F3')
    ax1.grid(True, alpha=0.3)

    # Panel 2: Yaw Kick (Foreground)
    ax2 = ax1.twinx()
    ax2.plot(time, plot_df['FFBYawKick'], label='Yaw Kick FFB', color='#F44336', linewidth=1.0)
    ax2.set_ylabel('Yaw Kick Force (Nm)', color='#F44336')
    ax2.tick_params(axis='y', labelcolor='#F44336')

    ax1.set_xlabel('Time (s)')

    # Combined legend
    lines, labels = ax1.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()
    ax1.legend(lines + lines2, labels + labels2, loc='upper right')

    plt.tight_layout()

    if output_path:
        if status_callback: status_callback(f"Saving to {Path(output_path).name}...")
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        return output_path
    if show: plt.show()
    return ""


def plot_clipping_components(
    df: pd.DataFrame,
    output_path: Optional[str] = None,
    show: bool = True,
    status_callback = None
) -> str:
    """
    Bar chart showing which components contribute most during clipping events.
    """
    if 'Clipping' not in df.columns or not df['Clipping'].any():
        return ""

    components = [
        'FFBBase', 'FFBUndersteerDrop', 'FFBOversteerBoost', 'FFBSoP',
        'FFBRearTorque', 'FFBScrubDrag', 'FFBYawKick', 'FFBGyroDamping',
        'FFBRoadTexture', 'FFBSlideTexture', 'FFBLockupVibration',
        'FFBSpinVibration', 'FFBBottomingCrunch', 'FFBABSPulse', 'FFBSoftLock'
    ]

    if status_callback: status_callback("Analyzing clipping contribution...")

    clipping_df = df[df['Clipping'] == 1]
    means = {}
    for comp in components:
        if comp in df.columns:
            means[comp.replace('FFB', '')] = clipping_df[comp].abs().mean()

    # Sort by mean contribution
    sorted_means = dict(sorted(means.items(), key=lambda x: x[1], reverse=True))

    fig, ax = plt.subplots(figsize=(12, 6))
    bars = ax.bar(sorted_means.keys(), sorted_means.values(), color='#FF5722')
    ax.set_title('Mean Component Magnitude during Clipping Events')
    ax.set_ylabel('Mean Absolute Force (Nm)')
    plt.xticks(rotation=45, ha='right')
    ax.grid(True, alpha=0.3, axis='y')

    if output_path:
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        return output_path
    if show: plt.show()
    return ""

def plot_slip_vs_latg(
    df: pd.DataFrame,
    output_path: Optional[str] = None,
    show: bool = True,
    status_callback = None
) -> str:
    """
    Scatter plot of Slip Angle vs Lateral G.
    Uses speed-banded envelopes to isolate Mechanical Grip from Aerodynamic Grip.
    """
    if status_callback: status_callback("Initializing tire curve plot...")
    
    if 'CalcSlipAngleFront' in df.columns:
        slip_col = 'CalcSlipAngleFront'
    elif 'calc_slip_angle_front' in df.columns:
        slip_col = 'calc_slip_angle_front'
    else:
        slip_col = None
    
    fig, ax = plt.subplots(figsize=(14, 9))
    
    # Filter out very low speeds (parking lot maneuvers)
    plot_df = df[df['Speed'] * 3.6 > 20.0].copy()
    downsampled_df = _downsample_df(plot_df, max_points=15000)
    
    slip = np.abs(downsampled_df[slip_col])
    lat_g = np.abs(downsampled_df['LatAccel'] / 9.81)
    speed_kmh = downsampled_df['Speed'] * 3.6
    
    scatter = ax.scatter(slip, lat_g, c=speed_kmh, cmap='viridis', alpha=0.15, s=5, label='Raw Telemetry')
    
    bins = np.arange(0, 0.25, 0.005)
    plot_df['SlipBin'] = pd.cut(np.abs(plot_df[slip_col]), bins)
    
    # Define Speed Bands to isolate Aero vs Mechanical grip
    speed_series = plot_df['Speed'] * 3.6
    bands = {
        'Low Speed (<120 km/h) [Mechanical]': (speed_series >= 20) & (speed_series < 120),
        'Med Speed (120-180 km/h) [Mixed]': (speed_series >= 120) & (speed_series < 180),
        'High Speed (>180 km/h) [Aero]': (speed_series >= 180)
    }
    colors = {
        'Low Speed (<120 km/h) [Mechanical]': '#2196F3', # Blue
        'Med Speed (120-180 km/h) [Mixed]': '#FF9800',   # Orange
        'High Speed (>180 km/h) [Aero]': '#F44336'       # Red
    }
    
    for name, mask in bands.items():
        band_df = plot_df[mask]
        if len(band_df) < 100: continue
        
        binned_envelope = band_df.groupby('SlipBin')['LatAccel'].apply(
            lambda x: np.percentile(np.abs(x), 95) if len(x) > 5 else np.nan
        ) / 9.81
        
        bin_centers = binned_envelope.index.categories.mid
        valid_idx = ~binned_envelope.isna()
        
        x_valid = bin_centers[valid_idx].astype(float).values
        y_valid = binned_envelope[valid_idx].astype(float).values
        
        if len(x_valid) > 5:
            y_smooth = pd.Series(y_valid).rolling(window=5, center=True, min_periods=1).mean().values
            ax.plot(x_valid, y_smooth, color=colors[name], linewidth=3, label=f'{name} Envelope')
            
            # Gradient Analysis for Knee
            dy = np.gradient(y_smooth, x_valid)
            early_mask = x_valid < 0.08
            if early_mask.any():
                max_dy = np.max(dy[early_mask])
                # Knee is where slope drops to 20% of max, after 0.03 rad
                knee_candidates = np.where((dy < max_dy * 0.20) & (x_valid > 0.03))[0]
                
                if len(knee_candidates) > 0:
                    knee_idx = knee_candidates[0]
                    ax.plot(x_valid[knee_idx], y_smooth[knee_idx], marker='*', color=colors[name], 
                            markersize=16, markeredgecolor='black', 
                            label=f'{name[:9]} Knee ~{x_valid[knee_idx]:.3f} rad')

    ax.set_xlabel('Absolute Slip Angle (rad)')
    ax.set_ylabel('Absolute Lateral G')
    ax.set_title('Physical Tire Curve by Speed Band (Isolating Aero vs Mechanical Grip)')
    ax.set_xlim(0, 0.25)
    ax.grid(True, alpha=0.3)
    
    cbar = plt.colorbar(scatter, ax=ax)
    cbar.set_label('Speed (km/h)')
    
    _safe_legend(ax)
    plt.tight_layout()
    
    if output_path:
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        return output_path
    if show: plt.show()
    return ""

def plot_slip_ratio_vs_long_g(
    df: pd.DataFrame,
    output_path: Optional[str] = None,
    show: bool = True,
    status_callback = None
) -> str:
    """
    Scatter plot of Slip Ratio vs Longitudinal G during braking.
    Uses speed-banded envelopes to isolate Aero Drag from Mechanical Braking.
    """
    cols =['SlipRatioFL', 'SlipRatioFR', 'LongAccel', 'Brake', 'Speed', 'Steering']
    if not all(c in df.columns for c in cols):
        return ""

    if status_callback: status_callback("Rendering Longitudinal Tire Curve...")
    
    fig, ax = plt.subplots(figsize=(14, 9))
    
    # Filter for straight-line braking
    brake_mask = (df['Brake'] > 0.05) & (df['Speed'] * 3.6 > 20.0) & (np.abs(df['Steering']) < 0.1)
    plot_df = df[brake_mask].copy()
    
    if len(plot_df) < 50:
        if status_callback: status_callback("Not enough straight-line braking data.")
        plt.close(fig)
        return ""

    plot_df['AvgFrontSlipRatio'] = (np.abs(plot_df['SlipRatioFL']) + np.abs(plot_df['SlipRatioFR'])) / 2.0
    downsampled_df = _downsample_df(plot_df, max_points=15000)
    
    slip_ratio = downsampled_df['AvgFrontSlipRatio']
    long_g = np.abs(downsampled_df['LongAccel'] / 9.81)
    speed_kmh = downsampled_df['Speed'] * 3.6
    
    scatter = ax.scatter(slip_ratio, long_g, c=speed_kmh, cmap='plasma', alpha=0.2, s=5, label='Braking Telemetry')
    
    bins = np.arange(0, 0.30, 0.005)
    plot_df['RatioBin'] = pd.cut(plot_df['AvgFrontSlipRatio'], bins)
    
    # Speed Bands for Braking (Aero drag adds massive Long G at high speeds)
    speed_series = plot_df['Speed'] * 3.6
    bands = {
        'Low/Med Speed (<150 km/h) [Mechanical]': (speed_series < 150),
        'High Speed (>150 km/h) [Aero + Mech]': (speed_series >= 150)
    }
    colors = {
        'Low/Med Speed (<150 km/h) [Mechanical]': '#00BCD4', # Cyan
        'High Speed (>150 km/h) [Aero + Mech]': '#E91E63'    # Pink
    }
    
    for name, mask in bands.items():
        band_df = plot_df[mask]
        if len(band_df) < 50: continue
        
        binned_envelope = band_df.groupby('RatioBin')['LongAccel'].apply(
            lambda x: np.percentile(np.abs(x), 95) if len(x) > 3 else np.nan
        ) / 9.81
        
        bin_centers = binned_envelope.index.categories.mid
        valid_idx = ~binned_envelope.isna()
        
        x_valid = bin_centers[valid_idx].astype(float).values
        y_valid = binned_envelope[valid_idx].astype(float).values
        
        if len(x_valid) > 5:
            y_smooth = pd.Series(y_valid).rolling(window=5, center=True, min_periods=1).mean().values
            ax.plot(x_valid, y_smooth, color=colors[name], linewidth=3, label=f'{name} Envelope')
            
            dy = np.gradient(y_smooth, x_valid)
            early_mask = x_valid < 0.10
            if early_mask.any():
                max_dy = np.max(dy[early_mask])
                knee_candidates = np.where((dy < max_dy * 0.20) & (x_valid > 0.03))[0]
                
                if len(knee_candidates) > 0:
                    knee_idx = knee_candidates[0]
                    ax.plot(x_valid[knee_idx], y_smooth[knee_idx], marker='*', color=colors[name], 
                            markersize=16, markeredgecolor='black', 
                            label=f'{name[:13]} Knee ~{x_valid[knee_idx]:.3f}')

    ax.set_xlabel('Absolute Slip Ratio (Front Axle Avg)')
    ax.set_ylabel('Absolute Longitudinal G (Braking)')
    ax.set_title('Longitudinal Tire Curve by Speed Band (Isolating Aero Drag)')
    ax.set_xlim(0, 0.30)
    ax.grid(True, alpha=0.3)
    
    cbar = plt.colorbar(scatter, ax=ax)
    cbar.set_label('Speed (km/h)')
    
    _safe_legend(ax)
    plt.tight_layout()
    
    if output_path:
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        return output_path
    if show: plt.show()
    return ""

    
def plot_dalpha_histogram(
    df: pd.DataFrame,
    output_path: Optional[str] = None,
    show: bool = True,
    status_callback = None
) -> str:
    """
    Histogram of dAlpha/dt values (shows when slope calculation is "active").
    """
    if 'dAlpha_dt' not in df.columns:
        return ""
    
    if status_callback: status_callback("Rendering dAlpha histogram...")
    fig, ax = plt.subplots(figsize=(10, 6))
    
    dalpha = df['dAlpha_dt'].values
    
    # Create histogram
    ax.hist(dalpha, bins=100, color='#2196F3', alpha=0.7, edgecolor='white')
    
    # Mark the threshold
    threshold = 0.02
    ax.axvline(threshold, color='#F44336', linestyle='--', linewidth=2, label=f'Threshold (+{threshold})')
    ax.axvline(-threshold, color='#F44336', linestyle='--', linewidth=2, label=f'Threshold (-{threshold})')
    
    # Calculate percentages
    above_threshold = (np.abs(dalpha) > threshold).mean() * 100
    
    ax.set_xlabel('dAlpha/dt (rad/s)')
    ax.set_ylabel('Frequency')
    ax.set_title(f'Distribution of dAlpha/dt\n{above_threshold:.1f}% of frames above threshold (active calculation)')
    _safe_legend(ax)
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    if output_path:
        if status_callback: status_callback(f"Saving to {Path(output_path).name}...")
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        return output_path
    
    if show:
        plt.show()
    
    return ""


def plot_slope_correlation(
    df: pd.DataFrame,
    output_path: Optional[str] = None,
    show: bool = True,
    status_callback = None
) -> str:
    """
    Scatter plot of dAlpha/dt vs SlopeCurrent to detect numerical instability.
    """
    if 'dAlpha_dt' not in df.columns or 'SlopeCurrent' not in df.columns:
        return ""

    if status_callback: status_callback("Rendering slope correlation plot...")
    # Downsample if too large for performance
    if len(df) > 20000:
        plot_df = df.sample(n=20000, random_state=42)
    else:
        plot_df = df

    fig, ax = plt.subplots(figsize=(10, 8))

    ax.scatter(plot_df['dAlpha_dt'], plot_df['SlopeCurrent'],
               alpha=0.1, s=10, color='#9C27B0')

    # Annotate thresholds
    ax.axvline(0.02, color='#F44336', linestyle='--', alpha=0.5, label='Threshold (0.02)')
    ax.axvline(-0.02, color='#F44336', linestyle='--', alpha=0.5)

    ax.set_xlabel('dAlpha/dt (rad/s)')
    ax.set_ylabel('Slope (G/rad)')
    ax.set_title('Instability Check: dAlpha/dt vs SlopeCurrent')
    ax.set_ylim(-50, 50)  # Focus on the relevant range, even if outliers exist
    ax.grid(True, alpha=0.3)
    _safe_legend(ax)

    plt.tight_layout()

    if output_path:
        if status_callback: status_callback(f"Saving to {Path(output_path).name}...")
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        return output_path
    
    if show:
        plt.show()
    
    return ""


def plot_longitudinal_diagnostic(
    df: pd.DataFrame,
    metadata: SessionMetadata,
    output_path: Optional[str] = None,
    show: bool = True,
    status_callback = None
) -> str:
    """
    Diagnostic plot for Longitudinal Load Transfer.
    Panels: Inputs (Pedals/Speed), Load & Multiplier, FFB Output Impact.
    """
    cols =['LoadFL', 'LoadFR', 'LongitudinalLoadFactor', 'FFBBase', 'FFBTotal', 'Speed', 'Brake', 'Throttle']
    if not all(c in df.columns for c in cols):
        return ""

    if status_callback: status_callback("Initializing Longitudinal diagnostic plot...")
    fig, axes = plt.subplots(3, 1, figsize=(14, 12), sharex=True)
    fig.suptitle('Longitudinal Load Transfer Diagnostic', fontsize=14, fontweight='bold')

    plot_df = _downsample_df(df)
    time = plot_df['Time']

    # Panel 1: Inputs (Pedals & Speed)
    ax1 = axes[0]
    ax1.plot(time, plot_df['Brake'], label='Brake', color='#F44336', alpha=0.8)
    ax1.plot(time, plot_df['Throttle'], label='Throttle', color='#4CAF50', alpha=0.8)
    ax1.set_ylabel('Pedal Input (0-1)')
    ax1.grid(True, alpha=0.3)
    _safe_legend(ax1, loc='upper left')

    ax1_twin = ax1.twinx()
    ax1_twin.plot(time, plot_df['Speed'] * 3.6, label='Speed (km/h)', color='#2196F3', alpha=0.6, linestyle='--')
    ax1_twin.set_ylabel('Speed (km/h)', color='#2196F3')
    _safe_legend(ax1_twin, loc='upper right')
    ax1.set_title('Driver Inputs & Speed')

    # Panel 2: Front Load & Multiplier
    ax2 = axes[1]
    front_load = (plot_df['LoadFL'] + plot_df['LoadFR']) / 2.0
    ax2.plot(time, front_load, label='Avg Front Load (N)', color='#9C27B0', alpha=0.8)
    ax2.set_ylabel('Load (N)', color='#9C27B0')
    ax2.grid(True, alpha=0.3)
    _safe_legend(ax2, loc='upper left')

    ax2_twin = ax2.twinx()
    ax2_twin.plot(time, plot_df['LongitudinalLoadFactor'], label='Long. Load Multiplier', color='#FF9800', linewidth=1.5)
    ax2_twin.axhline(1.0, color='black', linestyle='--', alpha=0.5)
    ax2_twin.set_ylabel('Multiplier (x)', color='#FF9800')
    _safe_legend(ax2_twin, loc='upper right')
    ax2.set_title('Tire Load & Calculated Multiplier')

    # Panel 3: FFB Output Impact
    ax3 = axes[2]
    ax3.plot(time, plot_df['FFBBase'], label='Base FFB (Nm)', color='#9E9E9E', alpha=0.6)
    ax3.plot(time, plot_df['FFBTotal'], label='Total FFB Output (Nm)', color='#2196F3', linewidth=1.0)

    if 'Clipping' in plot_df.columns:
        ax3.fill_between(time, -15, 15, where=plot_df['Clipping']==1, color='#F44336', alpha=0.2, label='Clipping')

    ax3.set_ylabel('Force (Nm)')
    ax3.set_xlabel('Time (s)')
    ax3.grid(True, alpha=0.3)
    _safe_legend(ax3, loc='upper right')
    ax3.set_title('FFB Output & Clipping')

    plt.tight_layout()

    if output_path:
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        return output_path
    if show: plt.show()
    return ""


def plot_raw_telemetry_health(
    df: pd.DataFrame,
    output_path: Optional[str] = None,
    show: bool = True,
    status_callback = None
) -> str:
    """
    Diagnostic plot to verify if raw telemetry channels are encrypted/missing.
    Plots all 4 tire loads and all 4 tire grips.
    """
    cols =['RawLoadFL', 'RawLoadFR', 'RawLoadRL', 'RawLoadRR', 
            'GripFL', 'GripFR', 'GripRL', 'GripRR']
    if not all(c in df.columns for c in cols):
        return ""

    if status_callback: status_callback("Rendering Raw Telemetry Health plot...")
    fig, axes = plt.subplots(2, 1, figsize=(14, 10), sharex=True)
    fig.suptitle('Raw Telemetry Health (Encryption / Missing Data Check)', fontsize=14, fontweight='bold')

    plot_df = _downsample_df(df)
    time = plot_df['Time']

    # Panel 1: Tire Loads
    ax1 = axes[0]
    ax1.plot(time, plot_df['RawLoadFL'], label='FL Load', color='#F44336', alpha=0.7, linewidth=1)
    ax1.plot(time, plot_df['RawLoadFR'], label='FR Load', color='#4CAF50', alpha=0.7, linewidth=1)
    ax1.plot(time, plot_df['RawLoadRL'], label='RL Load', color='#2196F3', alpha=0.7, linewidth=1)
    ax1.plot(time, plot_df['RawLoadRR'], label='RR Load', color='#FF9800', alpha=0.7, linewidth=1)

    ax1.set_ylabel('Raw Tire Load (N)')
    ax1.set_title('Raw Tire Load (mTireLoad) - Should fluctuate dynamically if not encrypted')
    ax1.grid(True, alpha=0.3)
    _safe_legend(ax1, loc='upper right')

    # Panel 2: Tire Grips
    ax2 = axes[1]
    ax2.plot(time, plot_df['GripFL'], label='FL Grip', color='#F44336', alpha=0.7, linewidth=1)
    ax2.plot(time, plot_df['GripFR'], label='FR Grip', color='#4CAF50', alpha=0.7, linewidth=1)
    ax2.plot(time, plot_df['GripRL'], label='RL Grip', color='#2196F3', alpha=0.7, linewidth=1)
    ax2.plot(time, plot_df['GripRR'], label='RR Grip', color='#FF9800', alpha=0.7, linewidth=1)

    ax2.set_ylabel('Raw Grip Fraction (0-1)')
    ax2.set_xlabel('Time (s)')
    ax2.set_title('Raw Tire Grip (mGripFract) - Should fluctuate dynamically if not encrypted')
    ax2.grid(True, alpha=0.3)
    _safe_legend(ax2, loc='upper right')

    plt.tight_layout()

    if output_path:
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        return output_path
    if show: plt.show()
    return ""


def plot_true_tire_curve(
    df: pd.DataFrame,
    metadata: SessionMetadata,
    output_path: Optional[str] = None,
    show: bool = True,
    status_callback = None
) -> str:
    """
    Scatter plot of Raw Grip vs Slip Angle with Binned Mean and Theoretical Math Overlay.
    """
    cols =['GripFL', 'SlipAngleFL', 'SlipRatioFL', 'Speed']
    if not all(c in df.columns for c in cols):
        return ""

    if status_callback: status_callback("Rendering True Tire Curve plot...")
    
    # Filter out low speeds where slip angles go to infinity
    plot_df = df[df['Speed'] > 5.0].copy()
    
    # Extract pure lateral points (low longitudinal slip) to see the clean curve
    pure_lat_mask = np.abs(plot_df['SlipRatioFL']) < 0.02
    pure_lat_df = plot_df[pure_lat_mask]

    fig, ax = plt.subplots(figsize=(12, 8))
    
    # 1. Scatter Plot (Background)
    downsampled_df = _downsample_df(plot_df, max_points=15000)
    scatter = ax.scatter(
        np.abs(downsampled_df['SlipAngleFL']), 
        downsampled_df['GripFL'], 
        c=np.abs(downsampled_df['SlipRatioFL']), 
        cmap='coolwarm', 
        alpha=0.2, 
        s=10,
        label='Raw Telemetry Points'
    )
    
    # 2. Binned Mean (The Game's Actual Curve)
    if len(pure_lat_df) > 100:
        # Create bins every 0.005 rad
        bins = np.arange(0, 0.25, 0.005)
        pure_lat_df['SlipBin'] = pd.cut(np.abs(pure_lat_df['SlipAngleFL']), bins)
        binned_mean = pure_lat_df.groupby('SlipBin')['GripFL'].mean()
        bin_centers = binned_mean.index.categories.mid
        
        ax.plot(bin_centers, binned_mean.values, color='black', linewidth=3, label='Binned Mean (Game Physics)')

    # 3. Theoretical Curve (Our C++ Math)
    x_theory = np.linspace(0, 0.25, 100)
    
    min_sliding_grip = 0.05
    combined_slip = x_theory / metadata.optimal_slip_angle
    y_theory = min_sliding_grip + ((1.0 - min_sliding_grip) / (1.0 + combined_slip**4))
    
    ax.plot(x_theory, y_theory, color='#00E676', linewidth=3, linestyle='--', 
            label=f'C++ Math (Optimal = {metadata.optimal_slip_angle} rad, Min = {min_sliding_grip})')

    ax.set_xlabel('Absolute Slip Angle (rad)')
    ax.set_ylabel('Raw Game Grip Fraction (0.0 - 1.0)')
    ax.set_title('True Tire Curve: Game Physics vs C++ Approximation')
    ax.set_xlim(0, 0.25)
    ax.set_ylim(0, 1.05)
    ax.grid(True, alpha=0.3)
    
    cbar = plt.colorbar(scatter, ax=ax)
    cbar.set_label('Absolute Slip Ratio (Braking/Accel)')
    
    ax.legend(loc='upper right')

    plt.tight_layout()

    if output_path:
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        return output_path
    if show: plt.show()
    return ""
 