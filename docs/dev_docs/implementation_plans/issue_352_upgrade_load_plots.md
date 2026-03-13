# Implementation Plan - Upgrading the Diagnostics Plots for Tire load estimates #352

## Context
The current tire load estimation diagnostic plots are insufficient for detailed analysis. They separate raw and estimated loads into different panels, only show one axle, and lack statistical summaries (correlation, error distribution) that were present in earlier versions or are now desired for better at-a-glance evaluation.

### Design Rationale
- **Context:** Accurate tire load estimation is critical for the suspension-based fallback model used when primary telemetry is missing. Visualizing the accuracy across all wheels and axles, specifically in terms of "Dynamic Ratio" (which is what influences FFB), allows for precise tuning of motion ratios and unsprung mass offsets.

## Analysis
The user wants a comprehensive 8-panel grid that includes:
- Overlay of Raw vs. Approx load for all 4 wheels.
- Dynamic Ratio plots for both axles (Front/Rear).
- Restored Correlation Scatter and Error Distribution plots.
- Inclusion of metrics (Correlation, Mean Error) directly on the plots.

### Design Rationale
- **Analysis:** By overlaying raw and approx loads, we can easily spot phase shifts or damping issues. The dynamic ratio panels are essential because they represent the actual signal scaling applied in the FFB engine. Restoring statistical plots provides a quantitative measure of global accuracy.

## Proposed Changes

### 1. Update Plot Layout
- Change `plot_load_estimation_diagnostic` to use a `4x2` grid.
- Increase figure size to `(16, 20)` to accommodate the 8 panels.

### 2. Implement `plot_wheel` Helper
- Overlays `RawLoad` and `ApproxLoad` for a specific wheel.
- Uses distinct colors and line styles for clarity.

### 3. Implement `plot_dynamic_ratio` Helper
- Mimics C++ static load learning to normalize signals.
- Calculates and displays `Dynamic Corr` and `Mean Error` in an in-plot text box.

### 4. Restore Statistical Panels
- **Correlation Scatter:** Plots raw vs. approx for all wheels to show global linearity.
- **Error Distribution:** Histogram of `Approx - Raw` to identify bias or variance issues.

### Design Rationale
- **Proposed Changes:** These changes directly address the user's requirements for a "massive, comprehensive 8-panel grid". Using helper functions reduces code duplication and ensures consistency across panels. The in-plot metrics improve usability by providing quantitative feedback without requiring external tools.

## Test Plan

### Automated Tests
- Run `PYTHONPATH=tools python -m pytest tools/lmuffb_log_analyzer/tests/test_plots.py`.
- Ensure `test_plot_generation` passes, confirming no regressions in the plotting pipeline.

### Manual Verification
- Verify the code logic in `plots.py` matches the requested implementation in Issue #352.
- Confirm that the output file is created successfully.

### Design Rationale
- **Test Plan:** Since this is a GUI/Plotting change on a headless Linux environment, automated regression tests combined with rigorous code review are the most reliable verification methods.

## Implementation Notes
- **Iteration 1:** Initial implementation followed the user's suggested code but accidentally deleted `plot_longitudinal_diagnostic` due to a bad search-and-replace or merge-diff application.
- **Iteration 2:** Restored the deleted function and verified imports ( `Path` was already imported). Updated `VERSION` and `CHANGELOG_DEV.md`.

## Additional Questions
None at this time.
