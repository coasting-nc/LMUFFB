
#### The Problem: Diagnostics and Health Checks for Longitudinal Load

The problem is that I cannot feel the Longitudinal load FFB effecton the wheel. I tried to max out the settings, with 1000% for longitudinal load and and max SoP scale. 

I also tried to isolate it from other effects, setting their slider to zero or low values; but I've kept Steering Shaft Gain at around 50% in order to be able to feel the longitudinal load (since the longitudinal load is applied as a multiplier to the base steering force, not as an independent additive force). I also had the "Pure Passthrough" option disabled.

I want to add diagnostic plots and reports and stats for longitudinal load  to the log analyser as health checks and for issues like the one above where I cannot feel it.

#### Python Log Analyzer: New Longitudinal Diagnostics
We will add a new plot to visualize the raw load, the calculated multiplier, and how it impacts the final FFB. We will also add warnings for encrypted data.

**Update `tools/lmuffb_log_analyzer/analyzers/lateral_analyzer.py`:**
Add this new function to the bottom of the file to extract longitudinal stats and encrypted data warnings:
```python
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
    results['missing_data_warnings'] =[]
    if 'WarnBits' in df.columns:
        any_warn = df['WarnBits'].max()
        if any_warn & 0x01:
            results['missing_data_warnings'].append("Tire Load (mTireLoad) data is missing/encrypted. Using kinematic fallback.")
        if any_warn & 0x02:
            results['missing_data_warnings'].append("Tire Grip (mGripFract) data is missing/encrypted. Using slip-based fallback.")
            
    return results
```

**Update `tools/lmuffb_log_analyzer/plots.py`:**
Add the new diagnostic plot function:
```python
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
```

**Update `tools/lmuffb_log_analyzer/reports.py`:**
Import the new analyzer and add the sections to the text report:
```python
from .analyzers.lateral_analyzer import analyze_lateral_dynamics, analyze_longitudinal_dynamics

def generate_text_report(metadata: SessionMetadata, df: pd.DataFrame) -> str:
    # ... existing code ...
    lateral_results = analyze_lateral_dynamics(df, metadata)
    long_results = analyze_longitudinal_dynamics(df, metadata) # <--- ADD THIS
    
    # ... existing report appends ...
    
    # ---> ADD THIS BLOCK AFTER LATERAL LOAD ANALYSIS <---
    report.append("LONGITUDINAL LOAD ANALYSIS")
    report.append("-" * 20)
    if long_results.get('long_load_factor_mean') is not None:
        report.append(f"Multiplier Mean:     {long_results['long_load_factor_mean']:.2f}x")
        report.append(f"Multiplier Range:    {long_results['long_load_factor_min']:.2f}x to {long_results['long_load_factor_max']:.2f}x")
        if not long_results.get('long_load_active', True):
            report.append("Status:              INACTIVE (Multiplier is stuck/clamped)")
        else:
            report.append("Status:              ACTIVE")
    report.append("")

    if long_results.get('missing_data_warnings'):
        report.append("DATA WARNINGS (ENCRYPTED/MISSING TELEMETRY)")
        report.append("-" * 20)
        for warn in long_results['missing_data_warnings']:
            report.append(f"  [!] {warn}")
        report.append("")
        
    # ... existing issues block ...
    
    # ---> ADD THIS TO THE ISSUES LIST <---
    if long_results.get('straight_brake_issue'):
        all_issues.append("LONGITUDINAL LOAD: Braking in a straight line produces near-zero FFB because the effect is a multiplier on base steering force (which is zero when driving straight). Turn the wheel slightly to feel the weight transfer.")
```

**Update `tools/lmuffb_log_analyzer/cli.py`:**
Import the plot and add it to the `_run_plots` function:
```python
from .plots import (
    # ... existing imports ...
    plot_lateral_diagnostic,
    plot_longitudinal_diagnostic # <--- ADD THIS
)

def _run_plots(metadata, df, output_dir, logfile_stem, plot_all=False):
    # ... existing code ...
            # Lateral Diagnostic
            lat_path = output_path / f"{logfile_stem}_lateral_diag.png"
            plot_lateral_diagnostic(df, metadata, str(lat_path), show=False, status_callback=update_status)
            console.print(f"  [OK] Created: {lat_path}")

            # ---> ADD THIS BLOCK <---
            # Longitudinal Diagnostic
            long_path = output_path / f"{logfile_stem}_longitudinal_diag.png"
            plot_longitudinal_diagnostic(df, metadata, str(long_path), show=False, status_callback=update_status)
            console.print(f"  [OK] Created: {long_path}")
```