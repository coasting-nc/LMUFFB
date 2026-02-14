import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from pathlib import Path
from typing import Optional

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
