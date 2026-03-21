# Implementation Plan - Redesign Preset System Phase 1 (GripEstimationConfig)

This document describes the plan and progress for refactoring the Grip Estimation configuration into a grouped structure.

## Proposed Changes

### 1. Define `GripEstimationConfig` Struct
- Location: `src/ffb/FFBConfig.h`
- Members:
    - `float optimal_slip_angle`
    - `float optimal_slip_ratio`
    - `float slip_angle_smoothing`
    - `float chassis_inertia_smoothing`
    - `bool load_sensitivity_enabled`
    - `float grip_smoothing_steady`
    - `float grip_smoothing_fast`
    - `float grip_smoothing_sensitivity`
- Methods:
    - `bool Equals(const GripEstimationConfig& o, float eps = 0.0001f) const`
    - `void Validate()`

### 2. Update `Preset` Struct
- Location: `src/core/Config.h`
- Replace loose variables with `GripEstimationConfig grip_estimation`.
- Update `Apply()`, `UpdateFromEngine()`, `Equals()`, and `Validate()`.
- Update fluent setters.

### 3. Update `FFBEngine` Class
- Location: `src/ffb/FFBEngine.h`
- Replace loose variables with `GripEstimationConfig m_grip_estimation`.

### 4. Find and Replace
- `src/ffb/FFBEngine.cpp`
- `src/core/Config.cpp`
- `src/gui/GuiLayer_Common.cpp`
- Other files as identified by `grep`.

## Safety Testing (TDD)
- Create `tests/test_refactor_grip_estimation.cpp`.
- **Consistency Test:** Verify `final_force` output remains identical.
- **Round-Trip Test:** Verify `Preset <-> Engine` synchronization.
- **Validation Test:** Verify clamping logic.

## Progress Journal
- [x] Create safety tests.
- [x] Define `GripEstimationConfig` struct.
- [x] Update `Preset` and `FFBEngine`.
- [x] Fix compiler errors and find-and-replace.
- [x] Verify all tests pass.
- [x] Increment version and update changelog.

## Encountered Issues
- **Widespread impact in Tests:** Grip estimation variables are heavily used across many unit tests. Used `sed` to perform bulk updates in the `tests/` directory.
- **main.cpp dependency:** `src/core/main.cpp` used several grip estimation members for telemetry logging, requiring updates.
- **Source Split File:** `src/physics/GripLoadEstimation.cpp` (a split from `FFBEngine.cpp`) also required updates for several members.
- **Test Failure:** `test_analyzer_bundling_integrity` failed initially due to a missing build artifact. Building the `LMUFFB` target resolved the issue, as it populates the `tools` directory.

## Deviations from the Plan
- Performed more bulk updates in the `tests/` directory than initially anticipated due to the high usage of grip estimation parameters in regression tests.

## Suggestions for the Future
- (To be filled)
