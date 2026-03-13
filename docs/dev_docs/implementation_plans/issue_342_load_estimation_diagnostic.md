# Implementation Plan - Issue #342: Tire Load Estimation Diagnostic Plots

**GitHub Issue:** [#342 Add one or more plots to diagnose how good the tire load estimate is (approximate_load)](https://github.com/coasting-nc/LMUFFB/issues/342)

## Context
The `approximate_load` function is used as a fallback when primary tire load telemetry is missing (e.g., encrypted DLC in LMU/rFactor 2). Its current implementation is a simple linear approximation: `mSuspForce + 300.0`. We need to verify how accurate this estimate is by comparing it against real tire load data in cases where both are available.

## Design Rationale
To properly diagnose and improve the load estimation, we need high-fidelity data comparison. By logging both the game's reported tire load and our approximation simultaneously, we can perform offline analysis to find correlations, offsets, and dynamic discrepancies.

## Codebase Analysis Summary

### Current Architecture
- **FFBEngine**: Handles physics calculations. `approximate_load` and `approximate_rear_load` are member functions.
- **AsyncLogger**: Captures `LogFrame` structures at 400Hz and writes them to a binary file.
- **Log Analyzer (Python)**: Parses these binary files and generates diagnostic plots.

### Impacted Functionalities
1. **Logging System**: `LogFrame` structure needs new fields to store the approximated loads.
2. **FFB Calculation Loop**: `calculate_force` in `FFBEngine.cpp` must compute these approximations every frame for logging purposes, regardless of whether the fallback is currently active.
3. **Log Analyzer**: Needs to handle the updated binary format and provide new visualization tools.

### Design Rationale
- **Binary Format Change**: Adding fields to `LogFrame` is necessary because we need per-wheel approximation data to match the per-wheel raw data. We increment the log version to maintain traceability.
- **Continuous Approximation**: We calculate the approximation even when real data is available so we have a ground truth for comparison.

## FFB Effect Impact Analysis
This task is primarily diagnostic and does not directly change FFB feel yet. However, the findings might lead to improvements in `approximate_load`, which would improve FFB feel for users of encrypted content.

## Proposed Changes

### 1. C++: Extend Logging
- **File:** `src/AsyncLogger.h`
  - Add `float approx_load_fl, approx_load_fr, approx_load_rl, approx_load_rr;` to `LogFrame` struct.
- **File:** `src/AsyncLogger.cpp` (or `AsyncLogger.h` inline)
  - Update `WriteHeader` to increment version to `v1.2`.
- **File:** `src/FFBEngine.cpp`
  - In `calculate_force`, populate the new `LogFrame` fields:
    - `frame.approx_load_fl = (float)approximate_load(fl);`
    - `frame.approx_load_fr = (float)approximate_load(fr);`
    - `frame.approx_load_rl = (float)approximate_rear_load(upsampled_data->mWheel[2]);`
    - `frame.approx_load_rr = (float)approximate_rear_load(upsampled_data->mWheel[3]);`

### 2. Python: Update Loader
- **File:** `tools/lmuffb_log_analyzer/loader.py`
  - Update `LOG_FRAME_DTYPE` with the new fields.
  - Update `mapping` dictionary.

### 3. Python: Implement Diagnostic Plot
- **File:** `tools/lmuffb_log_analyzer/plots.py`
  - Implement `plot_load_estimation_diagnostic`.
  - Panels:
    - Time-series Comparison (Raw vs Approx) for Front/Rear.
    - Error distribution (Histogram of `Approx - Raw`).
    - Correlation Scatter Plot with R² value.
- **File:** `tools/lmuffb_log_analyzer/cli.py`
  - Add `plot_load_estimation_diagnostic` to the `plots` command.

### 4. Code Documentation
- **File:** `src/GripLoadEstimation.cpp`
  - Add comments explaining the findings from the analysis (to be completed after running analysis on any available logs, or based on initial review).

## Test Plan
### Design Rationale
Since we cannot run the game, we will use unit tests to verify that the approximation logic is consistent and that the logging system correctly captures the values.

1. **Unit Test (C++)**: `tests/test_issue_342_logging.cpp`
   - Verify that `approximate_load` returns expected values for given `mSuspForce`.
   - Verify that `LogFrame` size is correct.
2. **Integration Test (Python)**:
   - Create a dummy binary file with the new format.
   - Verify that `loader.py` can parse it without errors.

## Parameter Synchronization Checklist
N/A - No new user-configurable parameters are added in this phase.

## Deliverables
- [ ] Modified `src/AsyncLogger.h`
- [ ] Modified `src/FFBEngine.cpp`
- [ ] Modified `tools/lmuffb_log_analyzer/loader.py`
- [ ] Modified `tools/lmuffb_log_analyzer/plots.py`
- [ ] Modified `tools/lmuffb_log_analyzer/cli.py`
- [ ] New/Updated C++ tests.
- [ ] Implementation Notes in this plan.

## Implementation Notes
(To be filled during development)
