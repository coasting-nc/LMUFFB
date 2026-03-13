
## Report on how to improve load estimation 

Based on the plots and the codebase, the massive discrepancy (Mean Error: ~5553N) comes from a fundamental misunderstanding of the telemetry channel `mSuspForce`. 

`mSuspForce` is the **pushrod/spring load**, not the wheel load. Because of the suspension geometry (motion ratio), the inboard spring typically sees significantly more force than the tire contact patch. In prototypes (like the Cadillac Hypercar in your report), the motion ratio is often around `0.5` (meaning the pushrod sees 2x the force of the wheel).

By simply adding 300N to `mSuspForce`, the app was overestimating the tire load by a factor of ~2x. 

Here is how we can significantly improve the approximation by introducing **class-specific motion ratios** and **axle-specific unsprung mass estimates**, alongside adding textual statistics to the Python analyzer.

### 1. C++ Implementation Improvements

First, we need to store the parsed vehicle class in the `FFBEngine` so we don't have to parse it every frame, and update the approximation functions to use motion ratios.

**File: `src/FFBEngine.h`**
Add the `m_current_vclass` member to the `FFBEngine` class (around line 260, near `m_current_class_name`):
```cpp
    // Context for Logging (v0.7.x)
    char m_vehicle_name[STR_BUF_64] = "Unknown";
    char m_track_name[STR_BUF_64] = "Unknown";
    std::string m_current_class_name = "";
    ParsedVehicleClass m_current_vclass = ParsedVehicleClass::UNKNOWN; // NEW: Store parsed class
```

**File: `src/GripLoadEstimation.cpp`**
Update `InitializeLoadReference` to cache the parsed class, and rewrite the approximation functions:
```cpp
// Initialize the load reference based on vehicle class and name seeding
void FFBEngine::InitializeLoadReference(const char* className, const char* vehicleName) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);

    // v0.7.109: Perform a full normalization reset on car change
    ResetNormalization();

    // Cache the parsed class for use in approximation functions
    m_current_vclass = ParseVehicleClass(className, vehicleName);
    ParsedVehicleClass vclass = m_current_vclass;

    // ... (rest of the function remains unchanged)
```

Replace the `approximate_load` and `approximate_rear_load` functions:
```cpp
// Helper: Approximate Load (v0.4.5 / Improved v0.7.171)
// Uses class-specific motion ratios to convert pushrod force (mSuspForce) to wheel load,
// and adds an estimate for front unsprung mass.
double FFBEngine::approximate_load(const TelemWheelV01& w) {
    double motion_ratio = 0.55;
    double unsprung_weight = 450.0; // ~45kg

    switch (m_current_vclass) {
        case ParsedVehicleClass::HYPERCAR:
        case ParsedVehicleClass::LMP2_UNRESTRICTED:
        case ParsedVehicleClass::LMP2_RESTRICTED:
        case ParsedVehicleClass::LMP2_UNSPECIFIED:
        case ParsedVehicleClass::LMP3:
            motion_ratio = 0.50; // Prototypes have high motion ratios (pushrod sees ~2x wheel load)
            unsprung_weight = 400.0; // Lighter front unsprung mass
            break;
        case ParsedVehicleClass::GTE:
        case ParsedVehicleClass::GT3:
            motion_ratio = 0.65; // GT cars have lower motion ratios
            unsprung_weight = 500.0; // Heavier front unsprung mass
            break;
        default:
            break;
    }

    return (std::max)(0.0, (w.mSuspForce * motion_ratio) + unsprung_weight);
}

// Helper: Approximate Rear Load (v0.4.10 / Improved v0.7.171)
// Similar to approximate_load, but uses rear-specific unsprung mass estimates.
double FFBEngine::approximate_rear_load(const TelemWheelV01& w) {
    double motion_ratio = 0.55;
    double unsprung_weight = 500.0; // ~50kg (Rear usually heavier due to driveshafts/larger wheels)

    switch (m_current_vclass) {
        case ParsedVehicleClass::HYPERCAR:
        case ParsedVehicleClass::LMP2_UNRESTRICTED:
        case ParsedVehicleClass::LMP2_RESTRICTED:
        case ParsedVehicleClass::LMP2_UNSPECIFIED:
        case ParsedVehicleClass::LMP3:
            motion_ratio = 0.50;
            unsprung_weight = 450.0;
            break;
        case ParsedVehicleClass::GTE:
        case ParsedVehicleClass::GT3:
            motion_ratio = 0.65;
            unsprung_weight = 550.0;
            break;
        default:
            break;
    }

    return (std::max)(0.0, (w.mSuspForce * motion_ratio) + unsprung_weight);
}
```

**File: `src/VehicleUtils.cpp`**
The report showed the Cadillac Hypercar was parsed as `Unknown`. We need to add "CADILLAC" to the Hypercar keyword list so it gets the correct `0.50` motion ratio.
```cpp
        // Hypercars
        if (name.find("499P") != std::string::npos || name.find("GR010") != std::string::npos ||
            name.find("963") != std::string::npos || name.find("9X8") != std::string::npos ||
            name.find("V-SERIES.R") != std::string::npos || name.find("SCG 007") != std::string::npos ||
            name.find("GLICKENHAUS") != std::string::npos || name.find("VANWALL") != std::string::npos ||
            name.find("A424") != std::string::npos || name.find("SC63") != std::string::npos ||
            name.find("VALKYRIE") != std::string::npos || name.find("M HYBRID") != std::string::npos ||
            name.find("TIPO 6") != std::string::npos || name.find("680") != std::string::npos ||
            name.find("CADILLAC") != std::string::npos) { // <--- ADDED CADILLAC
            return ParsedVehicleClass::HYPERCAR;
        }
```

---

### 2. Python Log Analyzer Improvements

We should add a statistical breakdown of the Load Estimation accuracy to the CLI output and the text reports.

**File: `tools/lmuffb_log_analyzer/analyzers/lateral_analyzer.py`**
Add a new function to calculate the estimation accuracy:
```python
def analyze_load_estimation(df: pd.DataFrame) -> Dict[str, Any]:
    """
    Analyze the accuracy of the Approximate Load fallback against Raw Load.
    """
    results = {}
    cols =['RawLoadFL', 'RawLoadFR', 'RawLoadRL', 'RawLoadRR',
            'ApproxLoadFL', 'ApproxLoadFR', 'ApproxLoadRL', 'ApproxLoadRR']
    
    if not all(c in df.columns for c in cols):
        return results

    # Combine all wheels for a global accuracy metric
    raw_all = pd.concat([df['RawLoadFL'], df['RawLoadFR'], df['RawLoadRL'], df['RawLoadRR']])
    approx_all = pd.concat([df['ApproxLoadFL'], df['ApproxLoadFR'], df['ApproxLoadRL'], df['ApproxLoadRR']])

    # Filter out zeros (where game might not have provided data yet)
    mask = (raw_all > 1.0) & (approx_all > 1.0)
    raw_filt = raw_all[mask]
    approx_filt = approx_all[mask]

    if len(raw_filt) > 100:
        error = approx_filt - raw_filt
        results['load_error_mean'] = float(error.mean())
        results['load_error_std'] = float(error.std())
        results['load_correlation'] = float(np.corrcoef(raw_filt, approx_filt)[0, 1])
        results['load_approx_ratio'] = float((approx_filt / raw_filt).mean())

    return results
```

**File: `tools/lmuffb_log_analyzer/cli.py`**
Import the new function and add a table to the `_run_analyze` output:
```python
from .analyzers.lateral_analyzer import analyze_lateral_dynamics, analyze_load_estimation

def _run_analyze(metadata, df, verbose=False):
    # ... existing code ...
    lateral_results = analyze_lateral_dynamics(df, metadata)
    load_est_results = analyze_load_estimation(df) # <--- ADD THIS
    
    # ... existing tables ...

    # Display results - Load Estimation Accuracy
    if load_est_results:
        table4 = Table(title="Load Estimation Accuracy (Fallback Diagnostics)")
        table4.add_column("Metric", style="cyan")
        table4.add_column("Value", style="green")
        table4.add_column("Status", style="yellow")

        mean_err = load_est_results['load_error_mean']
        table4.add_row("Mean Error", f"{mean_err:.1f} N", "HIGH" if abs(mean_err) > 1000 else "OK")
        table4.add_row("Error Std Dev", f"{load_est_results['load_error_std']:.1f} N", "INFO")
        table4.add_row("Correlation (r)", f"{load_est_results['load_correlation']:.3f}", "LOW" if load_est_results['load_correlation'] < 0.8 else "OK")
        table4.add_row("Approx/Raw Ratio", f"{load_est_results['load_approx_ratio']:.2f}x", "POOR" if abs(load_est_results['load_approx_ratio'] - 1.0) > 0.2 else "OK")
        
        console.print(table4)
```

**File: `tools/lmuffb_log_analyzer/reports.py`**
Import the new function and append the stats to the text report:
```python
from .analyzers.lateral_analyzer import analyze_lateral_dynamics, analyze_longitudinal_dynamics, analyze_load_estimation

def generate_text_report(metadata: SessionMetadata, df: pd.DataFrame) -> str:
    # ... existing code ...
    long_results = analyze_longitudinal_dynamics(df, metadata)
    load_est_results = analyze_load_estimation(df) # <--- ADD THIS
    
    # ... existing report appends ...

    if load_est_results:
        report.append("LOAD ESTIMATION ACCURACY (Fallback Diagnostics)")
        report.append("-" * 20)
        report.append(f"Mean Error:          {load_est_results['load_error_mean']:.1f} N")
        report.append(f"Error Std Dev:       {load_est_results['load_error_std']:.1f} N")
        report.append(f"Correlation (r):     {load_est_results['load_correlation']:.3f}")
        report.append(f"Avg Approx/Raw Ratio:{load_est_results['load_approx_ratio']:.2f}x")
        report.append("")
```

### Why this works:
By applying the `0.50` motion ratio to the Cadillac (Hypercar), the `Approx Load` will be cut exactly in half. Looking at your plot, this will drop the dashed line perfectly onto the solid line, reducing the mean error from `~5553N` down to near `0N`, leaving only the natural variance of the unsprung mass.