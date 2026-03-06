# Implementation Plan - Expand Log Analyser (#253)

## Context
Issue #253 "Expand the Log Analyser, Add more stats and plots" requires enhancing the Python-based log analyzer tool to provide deeper diagnostics, particularly regarding Yaw Kick stability, frame consistency (dt), and signal clipping. This follows investigation into "strange pulls" with stiff setups (e.g., LMP2 at Silverstone).

### Design Rationale
The log analyzer is the primary tool for offline debugging of FFB physics. Adding these specific plots and metrics will allow developers and advanced users to definitively identify issues like physics aliasing, threshold thrashing, and numerical instability without needing to modify and re-run the C++ application.

## Analysis
The current analyzer focuses heavily on Slope Detection. The new requirements shift focus towards:
1.  **Temporal Stability:** Monitoring `delta_time` for frame drops.
2.  **Spectral Analysis:** Using FFT to identify high-frequency chatter in yaw acceleration.
3.  **Component Contribution:** Quantifying how much individual effects (Yaw Kick, Rear Torque) contribute to the total FFB.
4.  **Non-linear Effects:** Visualizing "Threshold Thrashing" where a noisy signal oscillates around a binary gate.
5.  **Correlation:** Linking suspension events (velocity, bottoming) to FFB spikes.

### Design Rationale
By moving these calculations to the Python analyzer, we keep the C++ hot-path lean while still gaining high-fidelity insights from the recorded 400Hz telemetry.

## Proposed Changes

### 1. New Analyzer Module: `yaw_analyzer.py`
*   **File:** `tools/lmuffb_log_analyzer/analyzers/yaw_analyzer.py`
*   **Functions:**
    *   `analyze_yaw_dynamics(df, metadata)`: Calculates Threshold Crossing Rate, Yaw Kick Contribution %, and rolling means.
    *   `calculate_fft(signal, fs=400)`: Computes the magnitude spectrum of yaw acceleration.
    *   `calculate_suspension_velocity(df)`: Derives velocity from `RawSuspDeflection`.
    *   `analyze_clipping(df)`: Quantifies clipping per component.

### 2. Visualization Enhancements: `plots.py`
*   **File:** `tools/lmuffb_log_analyzer/plots.py`
*   **New Plots:**
    *   `plot_yaw_diagnostic`: Time-series of raw vs smoothed yaw accel, threshold line, and yaw kick FFB.
    *   `plot_system_health`: `DeltaTime` vs `Time` and clipping indicators.
    *   `plot_threshold_thrashing`: Zoomed view of threshold crossings.
    *   `plot_suspension_yaw_correlation`: Scatter plot of Susp Velocity vs Yaw Accel.
    *   `plot_bottoming_diagnostic`: Ride height vs Yaw Kick FFB.
    *   `plot_yaw_fft`: Spectral magnitude of yaw acceleration.
    *   `plot_clipping_summary`: Bar chart or timeline of clipping events.

### 3. Reporting Update: `reports.py`
*   **File:** `tools/lmuffb_log_analyzer/reports.py`
*   **Updates:** Add "YAW KICK & STABILITY" and "SYSTEM HEALTH & CLIPPING" sections.

### 4. CLI Integration: `cli.py`
*   **File:** `tools/lmuffb_log_analyzer/cli.py`
*   **Updates:** Integrate the new analyzer and ensure `--all` generates the new plots.

### Design Rationale
Decoupling the analysis logic (analyzers/) from the visualization (plots.py) ensures the codebase remains maintainable and testable.

## Test Plan
1.  **Unit Tests:** Add `tools/lmuffb_log_analyzer/tests/test_yaw_analyzer.py` to verify math (FFT, rolling means, derivative).
2.  **Integration Test:** Run the analyzer against `sample_log.csv` (or a generated binary log) to ensure no crashes during plot generation.
3.  **Manual Verification:** Visually inspect generated plots to ensure labels, scales, and data mappings are correct.

### Design Rationale
Automated tests for math ensure reliability, while visual inspection is necessary for UI/UX aspects of the plots.

## Additional Questions
- Should we support both CSV and Binary for all new plots? (Yes, `loader.py` handles the mapping, so it should be transparent).
- What is the preferred window size for rolling means? (Issue states window=100 for 400Hz data, which is 0.25s).

## Implementation Notes
- **Yaw Dynamics Analyzer:** Implemented `yaw_analyzer.py` with 400Hz logic for threshold crossing rates, rolling means, and FFB contribution percentages.
- **Spectral Analysis:** Integrated `numpy.fft` for yaw acceleration spectral analysis, highlighting driver (1-3Hz) vs suspension (10-25Hz) frequency bands.
- **Plotting Suite:** Added 7 new diagnostic plots to `plots.py`, including system health (dt/clipping), bottoming diagnostics, and thrashing analysis.
- **CLI & Reporting:** Fully integrated into `cli.py` and `reports.py`. High thrashing/clipping issues are now automatically flagged in the text report.
- **Review Feedback:** Addressed `TypeError` where `None` values from analyzers were compared with floats in the reporting layer.
- **Version Increment:** Bumped to v0.7.130.
- **Tests:** Added `test_yaw_analyzer.py`. All 15 unit tests pass.
- **Challenges:** Mapping `RawGameYawAccel` to the correct derivation overlay required careful handling of twin axes in `matplotlib`.
