
## Plots to diagnose how good the tire grip estimate is

### 1. Are the current Grip diagnostics enough?
**No.** Currently, the log analyzer only checks if the raw grip channel (`mGripFract`) is flatlined (encrypted/missing). It does **not** evaluate how accurate our fallback approximation is compared to the real physics.

### 2. How does Grip differ from Load for FFB purposes?
Unlike Tire Load (which the FFB engine normalizes into a **Dynamic Ratio**), Tire Grip is an **Absolute Multiplier** ranging from `0.0` to `1.0`. 
*   `1.0` = Full grip (Base FFB is untouched).
*   `< 1.0` = Sliding (Base FFB is reduced to simulate understeer/oversteer).

Because it is an absolute multiplier, **absolute accuracy matters**. Specifically, the FFB engine cares about two things:
1.  **Onset Timing:** Does the approximation detect the *start* of the slide at the exact same moment the real tire loses grip?
2.  **Depth of Loss:** Does the approximation drop to `0.5` when the real tire drops to `0.5`?

### 3. The Challenge & The Solution
In the C++ code, for performance reasons, the FFB engine **skips** calculating the fallback approximation if the game provides valid raw grip data. This means the log file doesn't contain the approximation to compare against!

**The Solution:** We can perfectly simulate the C++ "Friction Circle" fallback algorithm in Python using the logged telemetry (`SlipAngle`, `SlipRatio`, `Speed`). By doing this, we can overlay the "Simulated Fallback" on top of the "Raw Game Grip" to prove how accurate our math is.

Here is how we implement this comprehensive Grip Diagnostic suite.

---

### Step 1: Log the Slip Thresholds from C++
To simulate the math in Python, the log file needs to know what `optimal_slip_angle` and `optimal_slip_ratio` the user had configured.

**File: `src/AsyncLogger.h`**
Add the variables to the `SessionInfo` struct and the header writer:
```cpp
// Inside struct SessionInfo (around line 140)
    float optimal_slip_angle; // NEW
    float optimal_slip_ratio; // NEW
};

// Inside AsyncLogger::WriteHeader (around line 280)
        m_file << "# Optimal Slip Angle: " << info.optimal_slip_angle << "\n";
        m_file << "# Optimal Slip Ratio: " << info.optimal_slip_ratio << "\n";
        m_file << "# ========================\n";
```

**File: `src/main.cpp`**
Pass the settings to the logger:
```cpp
// Inside FFBThread(), where AsyncLogger::Get().Start() is called (around line 100)
                            info.optimal_slip_angle = g_engine.m_optimal_slip_angle; // NEW
                            info.optimal_slip_ratio = g_engine.m_optimal_slip_ratio; // NEW
                            AsyncLogger::Get().Start(info, Config::m_log_path);
```

---

### Step 2: Update Python Models & Loader
**File: `tools/lmuffb_log_analyzer/models.py`**
```python
    # Add to SessionMetadata
    optimal_slip_angle: float = 0.10
    optimal_slip_ratio: float = 0.12
```

**File: `tools/lmuffb_log_analyzer/loader.py`**
```python
# Inside _parse_header (around line 260)
        optimal_slip_angle=float(header_data.get('optimal_slip_angle', 0.10)),
        optimal_slip_ratio=float(header_data.get('optimal_slip_ratio', 0.12)),
    )
```

---

### Step 3: Create the Grip Analyzer
Create a new file to simulate the C++ math and generate statistics.

**File: `tools/lmuffb_log_analyzer/analyzers/grip_analyzer.py`**
```python
import pandas as pd
import numpy as np
from typing import Dict, Any
from ..models import SessionMetadata

def analyze_grip_estimation(df: pd.DataFrame, metadata: SessionMetadata) -> Dict[str, Any]:
    """
    Simulates the C++ Friction Circle fallback and compares it to Raw Telemetry Grip.
    """
    results = {}
    cols =['GripFL', 'GripFR', 'CalcSlipAngleFront', 'SlipRatioFL', 'SlipRatioFR']
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
        results['correlation'] = float(np.corrcoef(raw_front_grip[slip_mask], approx_grip[slip_mask])[0, 1])
        
        # False Positive Rate: Approx says sliding (<0.9), but Raw says gripping (>0.98)
        false_positives = (approx_grip < 0.9) & (raw_front_grip > 0.98)
        results['false_positive_rate'] = float(false_positives.mean() * 100.0)
    else:
        results['status'] = "NO_SLIP_EVENTS"

    return results
```

---

### Step 4: Create the Visual Plot
**File: `tools/lmuffb_log_analyzer/plots.py`**
Add this function to visually prove the approximation matches the game engine.
```python
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
    time = plot_df['Time']
    raw_front_grip = (plot_df['GripFL'] + plot_df['GripFR']) / 2.0
    approx_grip = plot_df['SimulatedApproxGrip']

    # Panel 1: Time Series Overlay
    ax1 = axes[0]
    ax1.plot(time, raw_front_grip, label='Raw Game Grip (Truth)', color='#2196F3', linewidth=2.0, alpha=0.8)
    ax1.plot(time, approx_grip, label='Approximated Grip (Fallback)', color='#F44336', linestyle='--', linewidth=1.5)
    
    ax1.set_ylabel('Grip Fraction (0.0 - 1.0)')
    ax1.set_ylim(0.0, 1.1)
    ax1.set_title('Front Axle Grip Loss Events')
    ax1.grid(True, alpha=0.3)
    _safe_legend(ax1, loc='lower right')

    # Panel 2: Error Delta
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
```

---

### Step 5: Hook it into the CLI and Report
**File: `tools/lmuffb_log_analyzer/cli.py`**
```python
from .analyzers.grip_analyzer import analyze_grip_estimation
from .plots import plot_grip_estimation_diagnostic # Add to imports

def _run_analyze(metadata, df, verbose=False):
    # ... existing code ...
    grip_est_results = analyze_grip_estimation(df, metadata)
    
    if grip_est_results.get('status') == "VALID":
        table5 = Table(title="Grip Estimation Accuracy (Friction Circle Fallback)")
        table5.add_column("Metric", style="cyan")
        table5.add_column("Value", style="green")
        table5.add_column("Status", style="yellow")

        corr = grip_est_results['correlation']
        table5.add_row("Correlation (r)", f"{corr:.3f}", "LOW" if corr < 0.75 else "OK")
        table5.add_row("Mean Error (Sliding)", f"{grip_est_results['mean_error_during_slip']:.3f}", "INFO")
        
        fpr = grip_est_results['false_positive_rate']
        table5.add_row("False Positive Rate", f"{fpr:.1f}%", "HIGH" if fpr > 5.0 else "OK")
        
        console.print(table5)

def _run_plots(metadata, df, output_dir, logfile_stem, plot_all=False):
    # ... existing plots ...
    if plot_all:
        # Grip Estimation Diagnostic
        grip_diag_path = output_path / f"{logfile_stem}_grip_estimation.png"
        plot_grip_estimation_diagnostic(df, metadata, str(grip_diag_path), show=False, status_callback=update_status)
        console.print(f"  [OK] Created: {grip_diag_path}")
```

**File: `tools/lmuffb_log_analyzer/reports.py`**
```python
from .analyzers.grip_analyzer import analyze_grip_estimation

def generate_text_report(metadata: SessionMetadata, df: pd.DataFrame) -> str:
    # ... existing code ...
    grip_est_results = analyze_grip_estimation(df, metadata)

    if grip_est_results:
        report.append("GRIP ESTIMATION ACCURACY (Friction Circle Fallback)")
        report.append("-" * 20)
        status = grip_est_results.get('status')
        if status == "ENCRYPTED":
            report.append("Status: Cannot evaluate (Raw telemetry is encrypted/missing).")
        elif status == "NO_SLIP_EVENTS":
            report.append("Status: Cannot evaluate (No significant sliding occurred).")
        else:
            report.append(f"Correlation (r):       {grip_est_results['correlation']:.3f}")
            report.append(f"Mean Error (Sliding):  {grip_est_results['mean_error_during_slip']:.3f}")
            report.append(f"False Positive Rate:   {grip_est_results['false_positive_rate']:.1f}%")
            
            if grip_est_results['correlation'] > 0.85:
                report.append("Evaluation:            EXCELLENT (Fallback closely matches game engine)")
            elif grip_est_results['correlation'] > 0.70:
                report.append("Evaluation:            GOOD (Fallback is viable)")
            else:
                report.append("Evaluation:            POOR (Consider tuning Optimal Slip Angle/Ratio)")
        report.append("")
```

### Why this is incredibly powerful:
By simulating the C++ math in Python, you can now take a log file from a car with *unencrypted* telemetry, run the analyzer, and **visually see exactly how good your fallback math is**. 

If the "Approximated Grip" line drops too early compared to the "Raw Game Grip", the report will flag a high "False Positive Rate", telling you immediately that the `Optimal Slip Angle` setting needs to be increased for that car class.


## Plots and Diagnostics for Slope Detection

Looking at the codebase and the *intent* of the Slope Detection algorithm, **our current diagnostics are definitely missing some crucial pieces.**

Here is an evaluation of what is missing and how we should upgrade the log analyzer to properly evaluate the Slope Detection algorithm.

### What is missing from the current Slope Diagnostics?

1. **The "Ground Truth" Comparison:** Just like we did for the Friction Circle, we need to overlay the Slope Detection's output (`Grip Factor`) against the game's actual `Raw Front Grip`. The whole point of Slope Detection is to be *more accurate* than the static Friction Circle. We need a visual and statistical way to prove that it actually matches the game's physics better.
2. **The Torque-Based Anticipation (Pneumatic Trail):** In v0.7.40, you added a massive feature to the C++ code: `m_slope_torque_current`. It looks at the steering shaft torque vs. steering angle to anticipate grip loss *before* the lateral G-forces drop. **This is completely missing from the current plots.** We need to see if the Torque-Slope actually drops before the G-Slope!
3. **The Confidence Gate:** The C++ code calculates a `Confidence` multiplier based on how fast the slip angle is changing. If confidence is low, it ignores the slope drop. We need to visualize this to understand why the algorithm might be ignoring a slide.
4. **"Disabled" Awareness:** The plots should explicitly warn you if you are looking at a log where the feature was turned off, so you don't waste time analyzing flat lines.

---

### The Upgrades

Here is how we rewrite the Python analyzer to make the Slope Detection diagnostics incredibly powerful.

#### 1. Update the Slope Plot (`plots.py`)
We will rewrite `plot_slope_timeseries` to include the Torque-Slope, the Confidence multiplier, the Raw Grip overlay, and a "DISABLED" watermark.

**File: `tools/lmuffb_log_analyzer/plots.py`**
Replace the existing `plot_slope_timeseries` function:

```python
def plot_slope_timeseries(
    df: pd.DataFrame, 
    metadata: SessionMetadata, # NEW: Pass metadata to check if enabled
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
    
    plt.tight_layout()
    
    if output_path:
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        plt.close(fig)
        return output_path
    if show: plt.show()
    return ""
```
*(Note: You will need to update the call in `cli.py` to pass `metadata` to `plot_slope_timeseries(df, metadata, ...)`).*

#### 2. Update the Statistical Analysis (`slope_analyzer.py`)
We need to add a correlation check to see if the Slope Detection is actually matching the game's raw grip.

**File: `tools/lmuffb_log_analyzer/analyzers/slope_analyzer.py`**
```python
def analyze_slope_stability(df: pd.DataFrame, metadata, threshold: float = 0.02) -> Dict[str, Any]:
    results = {}
    
    # ... existing basic stats ...

    # NEW: Correlation with Ground Truth (Raw Grip)
    grip_col = 'GripFactor' if 'GripFactor' in df.columns else 'SlopeSmoothed'
    if grip_col in df.columns and 'GripFL' in df.columns and 'GripFR' in df.columns:
        raw_front_grip = (df['GripFL'] + df['GripFR']) / 2.0
        
        # Only evaluate correlation when the car is actually sliding
        slip_mask = (raw_front_grip < 0.98) | (df[grip_col] < 0.98)
        if slip_mask.sum() > 50:
            results['slope_grip_correlation'] = float(np.corrcoef(raw_front_grip[slip_mask], df.loc[slip_mask, grip_col])[0, 1])
            
            # False Positive Rate: Slope says sliding (<0.9), but Raw says gripping (>0.98)
            false_positives = (df[grip_col] < 0.9) & (raw_front_grip > 0.98)
            results['false_positive_rate'] = float(false_positives.mean() * 100.0)
        else:
            results['slope_grip_correlation'] = None
            results['false_positive_rate'] = None

    # ... existing issue detection ...

    if not metadata.slope_enabled:
        results['issues'].append("SLOPE DETECTION WAS DISABLED. Metrics reflect Friction Circle fallback, not Slope math.")
    elif results.get('slope_grip_correlation') is not None and results['slope_grip_correlation'] < 0.6:
        results['issues'].append(f"POOR GRIP CORRELATION ({results['slope_grip_correlation']:.2f}) - Slope detection is not matching game physics.")

    return results
```
*(Note: Update `cli.py` and `reports.py` to pass `metadata` into `analyze_slope_stability(df, metadata)`).*

### Why these additions are critical:

1. **Proving the "Pneumatic Trail" Theory:** By plotting `SlopeTorque` (Cyan) against `SlopeCurrent` (Purple), you will visually see if the steering torque drops *before* the lateral G-forces drop. If it does, your v0.7.40 fusion logic is a massive success. If they drop at the exact same time, the torque anticipation isn't adding much value.
2. **Validating the Confidence Gate:** If you feel a slide in-game but the FFB doesn't drop, you can look at Panel 3. If the green `Confidence` area is at `0.0` during the slide, you know your `slope_alpha_threshold` is set too high, preventing the algorithm from trusting the data.
3. **Objective Superiority:** By comparing the `slope_grip_correlation` of a log using Slope Detection vs. a log using the Friction Circle fallback, you can mathematically prove which algorithm provides a more accurate representation of the game's physics engine.