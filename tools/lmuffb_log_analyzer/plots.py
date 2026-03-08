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
    output_path: Optional[str] = None,
    show: bool = True,
    status_callback = None
) -> str:
    """
    Generate 4-panel time-series plot for slope detection analysis.
    """
    if status_callback: status_callback("Initializing plot...")
    fig, axes = plt.subplots(4, 1, figsize=(14, 12), sharex=True)
    fig.suptitle('Slope Detection Analysis - Time Series', fontsize=14, fontweight='bold')
    
    # Downsample for performance
    if status_callback: status_callback("Downsampling data...")
    plot_df = _downsample_df(df)
    time = plot_df['Time'] if 'Time' in plot_df.columns else np.arange(len(plot_df)) * 0.01
    
    # Panel 1: Inputs (Lat G and Slip Angle)
    if status_callback: status_callback("Rendering Panel 1 (Inputs)...")
    ax1 = axes[0]
    ax1.plot(time, plot_df['LatAccel'] / 9.81, label='Lateral G', color='#2196F3', alpha=0.8)
    ax1.set_ylabel('Lateral G', color='#2196F3')
    ax1.tick_params(axis='y', labelcolor='#2196F3')
    _safe_legend(ax1, loc='upper left')
    ax1.grid(True, alpha=0.3)
    
    ax1_twin = ax1.twinx()
    if 'calc_slip_angle_front' in plot_df.columns:
        ax1_twin.plot(time, plot_df['calc_slip_angle_front'], label='Slip Angle', 
                      color='#FF9800', alpha=0.8)
    ax1_twin.set_ylabel('Slip Angle (rad)', color='#FF9800')
    ax1_twin.tick_params(axis='y', labelcolor='#FF9800')
    _safe_legend(ax1_twin, loc='upper right')
    ax1.set_title('Inputs: Lateral G and Slip Angle')
    
    # Panel 2: Derivatives
    if status_callback: status_callback("Rendering Panel 2 (Derivatives)...")
    ax2 = axes[1]
    if 'dG_dt' in plot_df.columns:
        ax2.plot(time, plot_df['dG_dt'], label='dG/dt', color='#2196F3', alpha=0.8)
    if 'dAlpha_dt' in plot_df.columns:
        ax2.plot(time, plot_df['dAlpha_dt'], label='dAlpha/dt', color='#FF9800', alpha=0.8)
        ax2.axhline(0.02, color='#F44336', linestyle='--', alpha=0.5, label='Threshold (0.02)')
        ax2.axhline(-0.02, color='#F44336', linestyle='--', alpha=0.5)
    ax2.set_ylabel('Derivative')
    _safe_legend(ax2, loc='upper right')
    ax2.grid(True, alpha=0.3)
    ax2.set_title('Derivatives: dG/dt and dAlpha/dt')
    
    # Panel 3: Slope
    if status_callback: status_callback("Rendering Panel 3 (Slope)...")
    ax3 = axes[2]
    if 'SlopeCurrent' in plot_df.columns:
        ax3.plot(time, plot_df['SlopeCurrent'], label='Slope (dG/dAlpha)', color='#9C27B0', linewidth=0.8)
        ax3.axhline(-0.3, color='#F44336', linestyle='--', alpha=0.5, label='Neg Threshold (-0.3)')
        ax3.axhline(0, color='#4CAF50', linestyle='-', alpha=0.3)
    ax3.set_ylabel('Slope (G/rad)')
    ax3.set_ylim(-15, 15)  # Clamp for visibility
    _safe_legend(ax3, loc='upper right')
    ax3.grid(True, alpha=0.3)
    ax3.set_title('Calculated Slope (dG/dAlpha)')
    
    # Panel 4: Grip Output
    if status_callback: status_callback("Rendering Panel 4 (Output)...")
    ax4 = axes[3]
    grip_col = 'GripFactor' if 'GripFactor' in plot_df.columns else 'SlopeSmoothed'
    if grip_col in plot_df.columns:
        ax4.plot(time, plot_df[grip_col], label='Grip Factor', color='#4CAF50', linewidth=1.0)
        ax4.axhline(0.2, color='#9E9E9E', linestyle='--', alpha=0.5, label='Floor (0.2)')
        ax4.axhline(1.0, color='#9E9E9E', linestyle='--', alpha=0.5)
    ax4.set_ylabel('Grip Factor')
    ax4.set_xlabel('Time (s)')
    ax4.set_ylim(0, 1.1)
    _safe_legend(ax4, loc='upper right')
    ax4.grid(True, alpha=0.3)
    ax4.set_title('Output: Grip Factor')
    
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
    Scatter plot of Slip Angle vs Lateral G (tire curve visualization).
    """
    if status_callback: status_callback("Initializing tire curve plot...")
    fig, ax = plt.subplots(figsize=(10, 8))
    
    slip_col = 'calc_slip_angle_front' if 'calc_slip_angle_front' in df.columns else None
    if slip_col is None:
        return ""
    
    # Downsample for performance (crucial for ax.scatter)
    if status_callback: status_callback("Downsampling data...")
    plot_df = _downsample_df(df)
    
    slip = np.abs(plot_df[slip_col])
    lat_g = np.abs(plot_df['LatAccel'] / 9.81) if 'LatAccel' in plot_df.columns else None
    
    if lat_g is None:
        return ""
    
    # Color by speed
    speed = plot_df['Speed'] * 3.6 if 'Speed' in plot_df.columns else None
    
    if status_callback: status_callback("Rendering scatter plot...")
    scatter = ax.scatter(slip, lat_g, c=speed, cmap='viridis', alpha=0.3, s=2)
    
    ax.set_xlabel('Slip Angle (rad)')
    ax.set_ylabel('Lateral G')
    ax.set_title('Tire Curve: Slip Angle vs Lateral G')
    ax.grid(True, alpha=0.3)
    
    if speed is not None:
        cbar = plt.colorbar(scatter, ax=ax)
        cbar.set_label('Speed (km/h)')
    
    # Mark the theoretical peak region
    ax.axvline(0.08, color='#F44336', linestyle='--', alpha=0.5, label='Typical Peak (~0.08 rad)')
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
