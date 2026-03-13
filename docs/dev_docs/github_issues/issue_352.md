# Upgrading the Diagnostics Plots for Tire load estimates #352

**Status:** Open
**Opened by:** coasting-nc
**Date:** Mar 13, 2026

## Description

**coasting-nc:**
Question
Here are the updated plots and diagnostics after the code changes related to tire load estimates. I notice that the raw load and estimate load are in two separate plots, which makes it harder to compare. They should be in the same plot. Additionally, we should have a plot for each of the 4 tires.

Additionally, some plots from the previous version have been removed (eg. the one with the absolute error and another one), I think we should restore them.

I think we should also put Dynamic Correlation and Dynamic Ratio Error values in the plot, as a legend, for convenience.

## Upgrading the Diagnostics Plot

**coasting-nc:**
You are completely right about the plots. Having them separated makes it hard to read, and we need to see all 4 wheels, plus restore the correlation and error distribution charts.

Let's rewrite the `plot_load_estimation_diagnostic` function to generate a massive, comprehensive 8-panel grid:
* Row 1: Front Left (Overlaid) | Front Right (Overlaid)
* Row 2: Rear Left (Overlaid) | Rear Right (Overlaid)
* Row 3: Front Axle Dynamic Ratio | Rear Axle Dynamic Ratio
* Row 4: Correlation Scatter | Error Distribution

File: `tools/lmuffb_log_analyzer/plots.py`
Replace the existing `plot_load_estimation_diagnostic` function with this updated version:
```python
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

        ax.plot(time, raw_ratio, label='Raw Ratio (x)', color='#2196F3', linewidth=1.5)
        ax.plot(time, approx_ratio, label='Approx Ratio (x)', color='#F44336', linestyle='--', linewidth=1.5)
        ax.axhline(1.0, color='black', linestyle=':', alpha=0.5, label='Static Weight (1.0x)')

        ax.set_title(title)
        ax.set_ylabel('Multiplier (x Static)')
        ax.grid(True, alpha=0.3)
        _safe_legend(ax, loc='upper right')

    # Row 3: Dynamic Ratios (What FFB Feels)
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
        ax_corr.set_title(f'Correlation (All Wheels)\nr = {correlation:.3f}')
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
```

## Question 2

**coasting-nc:**
I think we should also put Dynamic Correlation and Dynamic Ratio Error values in the plot, as a legend, for convenience.

## Answer 2

**coasting-nc:**
That is a great idea. Having those metrics directly on the plot makes it much easier to evaluate the approximation at a glance without having to cross-reference the text report.

We can calculate the correlation and mean error directly inside the plotting helper and display them in a clean text box (like a secondary legend) in the upper-left corner of the Dynamic Ratio panels.

Here is the updated `plot_load_estimation_diagnostic` function.

File: `tools/lmuffb_log_analyzer/plots.py`
Replace the existing `plot_load_estimation_diagnostic` function with this version:
```python
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
```

What this does:
1. Calculates the metrics on the fly: Inside the `plot_dynamic_ratio` helper, it calculates the exact same `ratio_corr` and `ratio_error_mean` that the text report uses.
2. Adds a Text Box: It uses `ax.text` with a bounding box (bbox) to draw a clean, semi-transparent white box in the upper-left corner (0.02, 0.95 in axes coordinates).
3. Avoids Clutter: By putting it in the upper-left, it stays out of the way of the main legend (which is anchored to the upper-right).
