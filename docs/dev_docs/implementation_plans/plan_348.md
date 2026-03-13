# Implementation Plan - Issue #348: Tire Grip Estimation Diagnostics

## Context
This plan outlines the implementation of a comprehensive diagnostic suite to evaluate the accuracy of the tire grip estimation algorithms (Friction Circle fallback and Slope Detection) in the LMUFFB project. This involves logging key parameters from the C++ FFB engine and implementing simulation/comparison logic in the Python log analyzer.

**GitHub Issue:** [#348 Add one or more plots to diagnose how good the tire grip estimate is](https://github.com/coasting-nc/LMUFFB/issues/348)

## Design Rationale
The core problem is that the FFB engine normally skips its fallback approximations when the game provides valid grip data. To evaluate these approximations, we need to either log them regardless (Shadow Mode) or simulate them in the analysis tool. This plan does both:
1.  **Logging Config:** We log the user's `optimal_slip_angle/ratio` so the Python tool can exactly replicate the Friction Circle math.
2.  **Shadow Mode:** We ensure Slope Detection runs in the background even for unencrypted cars, allowing a direct "Ground Truth" comparison against the game's actual grip data.
3.  **Visual Proof:** New plots will overlay the estimation against the ground truth, providing immediate visual feedback on the algorithm's performance and tuning needs.

## Reference Documents
- `docs/dev_docs/github_issues/348.md`

## Codebase Analysis Summary

### Current Architecture
- **FFBEngine (C++)**: Calculates force and handles grip estimation fallbacks in `GripLoadEstimation.cpp`.
- **AsyncLogger (C++)**: Handles high-frequency binary logging of telemetry and algorithm state.
- **Log Analyzer (Python)**: Parses binary logs, performs statistical analysis, and generates diagnostic plots.

### Impacted Functionalities
- **AsyncLogger Header**: Needs to include new session-level parameters.
- **FFB Loop (main.cpp)**: Needs to pass these parameters to the logger.
- **Grip Estimation Logic**: Needs to be updated to support "Shadow Mode" for slope detection.
- **Python Models/Loader**: Need to handle the new header fields.
- **Python Analyzers/Plots**: New logic for grip estimation comparison and improved slope diagnostics.

**Design Rationale:** These modules form the end-to-end telemetry pipeline. Modifications are required at the source (FFBEngine), the transport (AsyncLogger), and the consumer (Log Analyzer) to enable the new diagnostic capabilities.

## FFB Effect Impact Analysis
This change is primarily diagnostic and does not alter the *behavior* of FFB effects for the user during normal gameplay. However, "Shadow Mode" ensures that even when not actively used for FFB, the slope detection state is correctly updated and logged.

| Effect | Technical Changes | User-facing Changes |
| :--- | :--- | :--- |
| **Grip Estimation** | Slope detection now runs in "Shadow Mode" for all cars. | None (FFB feel remains identical). |
| **Diagnostics** | New analyzer and plots in the Python tool. | Enhanced log analysis reports and new diagnostic plots. |

**Design Rationale:** By running the algorithm in "Shadow Mode", we maintain physics-based realism by not interfering with the primary FFB path when good data is available, while still gaining the data needed for validation.

## Proposed Changes

### 1. C++: Log Slip Thresholds
**Files:** `src/AsyncLogger.h`, `src/main.cpp`
- Add `optimal_slip_angle` and `optimal_slip_ratio` to `SessionInfo`.
- Update `AsyncLogger::WriteHeader` to write these to the log file header.
- Update `FFBThread` in `main.cpp` to populate these values when starting a log.

**Design Rationale:** These are static session-level constants required for the Python tool to replicate the Friction Circle fallback math exactly as it would happen in C++.

### 2. C++: Shadow Mode for Slope Detection
**File:** `src/GripLoadEstimation.cpp`
- Move the call to `calculate_slope_grip` outside the `result.value < 0.0001` conditional block in `calculate_axle_grip`.
- Use the pre-calculated `slope_grip_estimate` if the fallback is actually triggered.

**Design Rationale:** Calculating slope detection in the background is computationally cheap and provides the "Shadow Mode" data needed to compare the algorithm's output against the game's raw grip data for tuning.

### 3. Python: Update Models & Loader
**Files:** `tools/lmuffb_log_analyzer/models.py`, `tools/lmuffb_log_analyzer/loader.py`
- Update `SessionMetadata` pydantic model.
- Update `_parse_header` to read the new fields with sensible defaults for backward compatibility.

### 4. Python: Grip Estimation Analyzer
**File:** `tools/lmuffb_log_analyzer/analyzers/grip_analyzer.py` (New)
- Implement `analyze_grip_estimation` to simulate Friction Circle math and calculate correlation/error stats against `GripFL/FR`.

**Design Rationale:** This allows evaluating the static Friction Circle fallback even when it wasn't used in-game.

### 5. Python: Upgrade Slope Analyzer
**File:** `tools/lmuffb_log_analyzer/analyzers/slope_analyzer.py`
- Add correlation and false positive rate calculations against raw game grip.

### 6. Python: New and Upgraded Plots
**File:** `tools/lmuffb_log_analyzer/plots.py`
- Add `plot_grip_estimation_diagnostic`.
- Rewrite `plot_slope_timeseries` to include Torque-Slope, Confidence, and Raw Grip overlay.

### 7. Python: CLI and Reports Hookup
**Files:** `tools/lmuffb_log_analyzer/cli.py`, `tools/lmuffb_log_analyzer/reports.py`
- Integrate new diagnostics into the text report and CLI output.
- Add the new grip diagnostic plot to the `--plot-all` sequence.

### 8. Version and Changelog
- Increment version in `VERSION`.
- Update `CHANGELOG_DEV.md`.

**Version Increment Rule:** Increment by +1 to the rightmost number.

## Test Plan (TDD-Ready)

### C++ Verification
1.  **Build Check**: Ensure project builds on Linux.
2.  **Unit Test (FFBEngine)**: Verify `calculate_axle_grip` still uses raw grip when available, but updates slope detection state.
    - Test: `FFBEngineTestAccess::GetSlopeSmoothedOutput` should change even when raw grip is valid.

### Python Verification
1.  **Loader Test**: Verify it correctly parses the new header fields from a (manually created) log file.
2.  **Analyzer Test**: Create a synthetic `DataFrame` with known slip and grip values. Verify the Friction Circle simulation matches expected mathematical results.
3.  **Plot Test**: Verify `plot_grip_estimation_diagnostic` and `plot_slope_timeseries` execute without errors.

**Design Rationale:** These tests provide coverage for both the data generation (C++) and data analysis (Python) parts of the feature, ensuring end-to-end integrity.

## Deliverables
- [ ] Modified `src/AsyncLogger.h`, `src/main.cpp`, `src/GripLoadEstimation.cpp`.
- [ ] Modified `tools/lmuffb_log_analyzer/` files (`models.py`, `loader.py`, `plots.py`, `cli.py`, `reports.py`, `analyzers/slope_analyzer.py`).
- [ ] New `tools/lmuffb_log_analyzer/analyzers/grip_analyzer.py`.
- [ ] Updated `VERSION` and `CHANGELOG_DEV.md`.
- [ ] Implementation Plan updated with notes.
- [ ] Code review records.

## Implementation Notes

### Unforeseen Issues
- The Python test suite (`test_plots.py`) required updates as the signature of `plot_slope_timeseries` changed (it now requires `metadata` to handle the "DISABLED" watermark).
- Some Python tests needed additional columns in synthetic DataFrames to support the new diagnostic overlays.

### Plan Deviations
- Added `tests/test_issue_348_shadow_mode.cpp` to verify the C++ "Shadow Mode" logic, as planned in the TDD cycle.
- Explicitly updated `tools/lmuffb_log_analyzer/tests/test_plots.py` to cover the new `plot_grip_estimation_diagnostic`.

### Challenges Encountered
- Replicating the C++ Friction Circle logic in Python required careful attention to the normalization steps and the safety floor (0.2).
- Ensuring "Shadow Mode" didn't impact FFB was verified by ensuring `result.value` is only overwritten if the fallback condition is met.

### Recommendations for Future Plans
- When changing plot function signatures, always include a step to update corresponding visualization tests.
