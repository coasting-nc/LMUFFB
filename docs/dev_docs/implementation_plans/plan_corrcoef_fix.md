# Implementation Plan - Robust Correlation Calculation

## Context
The Log Analyzer tool is encountering `RuntimeWarning: invalid value encountered in divide` in `numpy`'s `corrcoef` function. This happens when one or both input arrays have zero variance (all values are identical), which is common in telemetry logs when certain features are disabled or the car is stationary.

## Design Rationale
To ensure the analyzer is robust and produces accurate results without triggering warnings:
1.  **Explicit Variance Check:** Before calling `np.corrcoef`, we will calculate the standard deviation of both input arrays. If either `std` is zero (or below a tiny epsilon), the correlation will be reported as `0.0` as requested by the user.
2.  **Centralized Helper:** Instead of repeating this logic in 6 different places, we will create a helper function `safe_corrcoef` in a new utility module.
3.  **TDD approach:** We will write tests that explicitly feed constant data into the analyzers to ensure no warnings are triggered and the output is `0.0`.

## Codebase Analysis Summary
### Impacted Functionalities
- **Analysis Modules (`slope_analyzer.py`, `lateral_analyzer.py`, `grip_analyzer.py`)**: These use correlation to compare simulated vs raw data.
- **Plotting Module (`plots.py`)**: Uses correlation to display metrics in plot text boxes and titles.

### Design Rationale
A centralized `safe_corrcoef` function is preferred over ad-hoc checks to ensure consistency and maintainability.

## Proposed Changes

### 1. tools/lmuffb_log_analyzer/utils.py (NEW)
- Implement `safe_corrcoef(x, y)`:
    - Return `0.0` if `len(x) < 2`.
    - Return `0.0` if `np.std(x) == 0` or `np.std(y) == 0`.
    - Otherwise return `np.corrcoef(x, y)[0, 1]`.

### 2. tools/lmuffb_log_analyzer/analyzers/*.py
- Replace `np.corrcoef(...)` with `safe_corrcoef(...)`.

### 3. tools/lmuffb_log_analyzer/plots.py
- Replace `np.corrcoef(...)` with `safe_corrcoef(...)`.

## Test Plan (TDD-Ready)
### Design Rationale
We need to prove that the "zero variance" case is handled gracefully without warnings.

### Test Cases
1.  **`test_safe_corrcoef_constant_data`**: Pass `[1, 1, 1]` and `[1, 2, 3]` to `safe_corrcoef`. Assert it returns `0.0` and no warnings are raised.
2.  **`test_grip_analyzer_zero_variance`**: Run `analyze_grip_estimation` with a DataFrame where `GripFL` is constant. Assert `results['correlation'] == 0.0`.
3.  **`test_plots_zero_variance`**: Call `plot_load_estimation` with constant data. Assert no warnings are raised.

## Deliverables
- [ ] New file `tools/lmuffb_log_analyzer/utils.py`.
- [ ] Modified `plots.py`, `slope_analyzer.py`, `lateral_analyzer.py`, `grip_analyzer.py`.
- [ ] New test file `tools/lmuffb_log_analyzer/tests/test_corrcoef_robustness.py`.
- [x] Updated `CHANGELOG_DEV.md`.
- [x] Implementation Notes.

## Implementation Notes
- **Unforeseen Issues:** None. The `pydantic` validation error in the test suite was resolved by providing all required fields in the mock `SessionMetadata`.
- **Plan Deviations:** None.
- **Challenges Encountered:** None.
- **Recommendations for Future Plans:** Consider creating a global `math_utils.py` for the log analyzer if more robust statistical helpers are needed in the future.
