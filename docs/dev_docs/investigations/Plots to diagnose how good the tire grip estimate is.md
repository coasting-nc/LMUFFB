
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

