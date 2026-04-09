# Implementation Plan - Fix Log Analyzer NaN Warnings

## Context
The lmuFFB Log Analyzer triggers `RuntimeWarning: invalid value encountered in divide` during correlation calculations when telemetry data contains `NaN`, `Inf`, or zero-variance signals. Additionally, the user needs to know the origin of these invalid values to diagnose potential data recording issues.

## Design Rationale
- **Reliability:** Correlation calculations are prone to numerical instability. Using a wrapper that explicitly checks for `NaN`, `Inf`, and zero-variance ensures the tool remains stable and provides a clean output (0.0) when a valid correlation cannot be determined.
- **Traceability:** Identifying the specific telemetry column containing invalid data is crucial for debugging the C++ engine's logging logic or identifying game-side encryption/changes.
- **Physics-based Reasoning:** A correlation of `nan` in FFB diagnostics usually indicates a lack of signal activity (e.g., car stationary) or a catastrophic failure in calculation. Returning `0.0` prevents UI/Report breakage while the added warnings inform the user of the data quality issue.

## Codebase Analysis Summary
- **Impacted Modules:**
    - `tools/lmuffb_log_analyzer/utils.py`: Contains `safe_corrcoef`. Needs enhancement to handle non-finite values and suppress NumPy internal warnings.
    - `tools/lmuffb_log_analyzer/analyzers/lateral_analyzer.py`: Triggers the warning in `analyze_lateral_dynamics` due to `nan` in load transfer correlation.
    - `tools/lmuffb_log_analyzer/analyzers/grip_analyzer.py` & `slope_analyzer.py`: Use `safe_corrcoef` and should benefit from improved reporting.
- **Data Flow:** Telemetry is loaded into a Pandas DataFrame. Analyzers extract columns as NumPy arrays and pass them to `safe_corrcoef`.

## Proposed Changes

### 1. Enhanced Utilities (`tools/lmuffb_log_analyzer/utils.py`)
- **`safe_corrcoef(x, y)`**:
    - Convert inputs to NumPy arrays.
    - Check for `NaN` or `Inf` using `np.isfinite()`. If any non-finite values are present, return `0.0`.
    - Use `np.errstate(divide='ignore', invalid='ignore')` to suppress warnings during the internal division in `np.corrcoef`.
    - Maintain existing length and zero-variance checks.
- **`find_invalid_signals(df, columns)`**:
    - New helper to return a list of column names containing `NaN` or `Inf`.

### 2. Analyzer Updates
- **`lateral_analyzer.py`**:
    - Use `find_invalid_signals` on `LatLoadNorm`, `RawLatLoadNorm`, and `LatAccel`.
    - If invalid signals are found, add a warning to the results.
- **`slope_analyzer.py` & `grip_analyzer.py`**:
    - Add checks for invalid signals in primary inputs (e.g., `Slope`, `GripFL/FR`).
    - Integrate findings into the `issues` list in the returned results.

### 3. CLI/Report Integration
- Ensure that the "Origin of NaN" warnings are included in the final text report and printed to the console during `analyze` and `batch` commands.

## Version Increment Rule
- Increment `VERSION` and `src/Version.h` by +1 (smallest increment).
- Current version is likely `0.7.276` (from logs) or whatever is in the `VERSION` file.
- **Note:** I will check the `VERSION` file during implementation.

## Test Plan (TDD-Ready)
### Design Rationale
Tests will use synthetic DataFrames with injected "bad" data to ensure the analyzer doesn't crash and correctly identifies the problematic columns.

### Test Cases (`tools/lmuffb_log_analyzer/tests/test_nan_robustness.py`)
1. **`test_safe_corrcoef_nan_inf`**:
   - Input: Arrays with `np.nan` and `np.inf`.
   - Expected Output: `0.0`.
   - Verification: No `RuntimeWarning` caught.
2. **`test_find_invalid_signals`**:
   - Input: DataFrame with some "clean" columns and some containing `NaN`.
   - Expected Output: List of "dirty" column names.
3. **`test_lateral_analyzer_nan_reporting`**:
   - Input: Mock telemetry with `NaN` in `LatLoadNorm`.
   - Expected Output: Results dict contains a warning about `LatLoadNorm` containing invalid values.
4. **`test_slope_analyzer_nan_robustness`**:
   - Input: Mock telemetry with `Inf` in `Slope`.
   - Expected Output: Status is still returned, issues include "invalid values in Slope".

## Deliverables
- [ ] Updated `utils.py` with robust `safe_corrcoef` and `find_invalid_signals`.
- [ ] Updated analyzers with error reporting.
- [ ] New test file `test_nan_robustness.py`.
- [ ] Updated `VERSION` and `src/Version.h`.
- [ ] Updated `CHANGELOG_DEV.md`.
- [ ] Implementation Notes added to this plan.

## Implementation Notes
- **Challenge:** The `RuntimeWarning` from `np.corrcoef` was triggered even when `safe_corrcoef` checked for `np.std(x) == 0`. This is because standard deviation might be non-zero due to precision errors but small enough to trigger division warnings internally in NumPy when normalized. Added `np.errstate` suppression and explicit finiteness checks to resolve this.
- **Traceability:** Integrated `find_invalid_signals` across all major analyzers (Slope, Grip, Lateral).
- **Bug Found:** Discovered that several analyzers were using `df['col'].corr(df['other'])` directly, bypassing the `safe_corrcoef` helper entirely. Refactored these to use the robust helper.
- **Test Coverage:** Created `test_nan_robustness.py` which verifies handling of `NaN`, `Inf`, and zero-variance inputs without triggering warnings. Verified with provided real-world log file.
- **Report Fix:** Enhanced `reports.py` to prevent `KeyError` when correlation metrics are missing due to invalid data or lack of slip events, ensuring the report generates successfully even with bad telemetry.
