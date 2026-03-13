# Implementation Plan - Issue #345: Improve unsprung mass approximation in approximate_load and approximate_rear_load

**GitHub Issue:** [#345 Improve unsprung mass approximation in approximate_load and approximate_rear_load](https://github.com/coasting-nc/LMUFFB/issues/345)

## Context
The current tire load estimation fallback (`approximate_load`) uses a fixed 300N offset to account for unsprung mass. Investigation (see `docs/dev_docs/investigations/improve tire load estimation.md`) revealed that this significantly overestimates loads, especially for prototypes, because it ignores the suspension motion ratio (pushrod force vs. wheel load). This plan implements class-specific motion ratios and axle-specific unsprung mass estimates to improve accuracy.

## Design Rationale
- **Physics Accuracy**: Race cars typically have motion ratios significantly less than 1.0 (e.g., 0.5 for prototypes). By applying these ratios to the suspension force, we correctly estimate the load at the wheel contact patch.
- **Dynamic Ratio Priority**: While absolute Newtons are important for diagnostics, the FFB engine primarily relies on the ratio between current load and static load. Accurate motion ratios ensure the *shape* of the load curve (weight transfer and aero) is preserved, which is what the FFB feel depends on.
- **Car Classification**: Adding "CADILLAC" to the Hypercar parser ensures that one of the most common prototypes in LMU receives the correct physics parameters.
- **Enhanced Diagnostics**: Logging normalization settings and calculating "Dynamic Ratio Error" in the Python analyzer provides proof that the fallback is viable for FFB even if absolute Newtons have some residual error.

## Reference Documents
- [Improve Tire Load Estimation Investigation](../../investigations/improve%20tire%20load%20estimation.md)
- [Issue #342: Load Estimation Diagnostics](issue_342_load_estimation_diagnostic.md) (Baseline for diagnostics)

## Codebase Analysis Summary

### Current Architecture
- **FFBEngine**: Stores vehicle state and handles physics.
- **GripLoadEstimation.cpp**: Implements the load approximation logic.
- **VehicleUtils.cpp**: Handles car class identification.
- **AsyncLogger.h**: Handles binary telemetry logging.

### Impacted Functionalities
1. **Vehicle Identification**: `ParseVehicleClass` needs to be more comprehensive.
2. **Load Approximation**: `approximate_load` and `approximate_rear_load` need class-aware logic.
3. **Session Logging**: Header needs to include normalization toggles for better context in analysis.
4. **Log Analysis (Python)**: Needs to evaluate dynamic ratios and report on fallback health.

### Design Rationale
- Caching `m_current_vclass` in `FFBEngine` avoids repeated string parsing in the high-frequency physics loop (400Hz).
- Using a switch statement in approximation functions allows for easy tuning of parameters per class without complex lookup tables.

## FFB Effect Impact Analysis
| Effect | Technical Change | User Perspective |
|--------|------------------|------------------|
| Road Texture | Scaled by improved load | More consistent texture feel across different cars. |
| Scrub Drag | Scaled by improved load | More realistic resistance during heavy braking/turning. |
| Lockup Vibration | Scaled by improved load | Better feedback of tire status under load. |
| Understeer Drop | Dependent on load/grip | More stable understeer transitions in encrypted content. |

### Design Rationale
By improving the load estimate, all load-dependent effects will operate within their intended mathematical windows, preventing over-saturation or "numbness" in cars with high downforce/motion ratios.

## Proposed Changes

### 1. C++: FFBEngine Header Updates
- **File:** `src/FFBEngine.h`
  - Add `ParsedVehicleClass m_current_vclass = ParsedVehicleClass::UNKNOWN;` to `FFBEngine` class.

### 2. C++: Vehicle Identification Updates
- **File:** `src/VehicleUtils.cpp`
  - Update `ParseVehicleClass` to include "CADILLAC" in the HYPERCAR keyword list.

### 3. C++: Load Approximation Logic
- **File:** `src/GripLoadEstimation.cpp`
  - Update `InitializeLoadReference` to cache `m_current_vclass`.
  - Rewrite `approximate_load` and `approximate_rear_load` to use `m_current_vclass` for motion ratios and axle-specific unsprung weights.
    - Prototypes (Hypercar/LMP): MR ~0.50, Front Unsprung ~400N, Rear ~450N.
    - GTs (GTE/GT3): MR ~0.65, Front Unsprung ~500N, Rear ~550N.
    - Default: MR ~0.55, Front Unsprung ~450N, Rear ~500N.

### 4. C++: Logging Enhancements
- **File:** `src/AsyncLogger.h`
  - Update `SessionInfo` struct to include `dynamic_normalization` and `auto_load_normalization`.
  - Update `WriteHeader` to log these new fields.
- **File:** `src/main.cpp`
  - Populate the new `SessionInfo` fields before starting the logger.

### 5. Python: Log Analyzer Upgrades
- **File:** `tools/lmuffb_log_analyzer/models.py` & `loader.py`
  - Add support for parsing the new header fields.
- **File:** `tools/lmuffb_log_analyzer/analyzers/lateral_analyzer.py`
  - Implement `analyze_load_estimation` to calculate **Dynamic Ratio Error** (Current/Static) and correlation.
- **File:** `tools/lmuffb_log_analyzer/plots.py`
  - Update `plot_load_estimation_diagnostic` to show the **Dynamic Ratio Comparison** panel.
- **File:** `tools/lmuffb_log_analyzer/reports.py`
  - Update text report to include fallback health status and normalization settings.

### Version Increment Rule
- Increment version in `VERSION` to `0.7.171`.

## Test Plan
### Design Rationale
We must verify that the new class-based logic correctly assigns motion ratios and that the approximation remains stable.

1. **Unit Test (C++)**: `tests/test_issue_345_load_approx.cpp`
   - Test `approximate_load` with different `m_current_vclass` settings.
   - Assert that Hypercar returns lower values than GT3 for the same `mSuspForce`.
   - Assert that Rear load is slightly higher than Front load for the same force (due to unsprung mass).
2. **Regression Test (Python)**:
   - Run `test_loader.py` to ensure the updated header doesn't break parsing of old logs.
3. **Manual Verification (Linux)**:
   - Compile and run combined tests: `./build/tests/run_combined_tests`.

## Parameter Synchronization Checklist
N/A - No new user-configurable sliders. These are internal physics improvements.

## Deliverables
- [ ] Modified `src/FFBEngine.h`
- [ ] Modified `src/VehicleUtils.cpp`
- [ ] Modified `src/GripLoadEstimation.cpp`
- [ ] Modified `src/AsyncLogger.h`
- [ ] Modified `src/main.cpp`
- [ ] Modified Python tools (`models.py`, `loader.py`, `lateral_analyzer.py`, `plots.py`, `reports.py`)
- [ ] New C++ test file `tests/test_issue_345_load_approx.cpp`
- [ ] Updated `VERSION` and `CHANGELOG_DEV.md`
- [ ] Implementation Notes in this plan.
