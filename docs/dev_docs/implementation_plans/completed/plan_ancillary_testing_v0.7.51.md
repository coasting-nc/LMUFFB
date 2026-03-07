# Implementation Plan - Ancillary Code Testing Safety Net

This plan outlines the implementation of a comprehensive test suite for "ancillary" code in `FFBEngine.h` (mathematical utilities, performance statistics, and vehicle metadata parsing). This suite serves as a zero-regression safety net before extracting these functions into separate modules.

## Context
As part of the refactoring proposal `FFBEngine_Ancillary_Extraction.md`, we need to ensure that generic utilities currently residing in `FFBEngine.h` are perfectly tested. This allows us to move them later with high confidence that no logic has changed.

## Reference Documents
- Proposal: `docs/dev_docs/proposals/FFBEngine_Ancillary_Extraction.md`
- FFB Engine Source: `src/FFBEngine.h`

## Codebase Analysis Summary
The code to be tested is currently located in `src/FFBEngine.h` as either standalone structs or member functions of the `FFBEngine` class.

### Impacted Functionalities
1.  **Math & Signal Utilities**: `BiquadNotch`, `smoothstep`, `inverse_lerp`, `calculate_sg_derivative`, `apply_slew_limiter`, `apply_adaptive_smoothing`.
2.  **Performance Tracking**: `ChannelStats`.
3.  **Vehicle Mapping**: `ParseVehicleClass`, `GetDefaultLoadForClass`.

## Proposed Changes

### 1. Documentation & Code Cleanup
- **File**: `src/FFBEngine.h`
- **Action**: Add Doxygen-style comments to `BiquadNotch` explaining its purpose (Bi-quad direct form II) and its role in filtering steering wheel oscillation and road noise.

### 2. New Test Suite: Math Utils
- **File**: `tests/test_math_utils.cpp`
- **Logic**:
    - Implement independent tests for `BiquadNotch`, focusing on stability and state management.
    - Implement exhaustive tests for `inverse_lerp` and `smoothstep` including degenerate cases.
    - Implement buffer state tests for `calculate_sg_derivative`.

### 3. New Test Suite: Perf Stats
- **File**: `tests/test_perf_stats.cpp`
- **Logic**:
    - Test `ChannelStats` Min/Max/Avg logic.
    - Verify interval resets vs. session persistence.

### 4. New Test Suite: Vehicle Utils
- **File**: `tests/test_vehicle_utils.cpp`
- **Logic**:
    - Test `ParseVehicleClass` with a wide variety of strings (case sensitivity, substring matching).
    - Verify that every class in `ParsedVehicleClass` has a valid default load.

## Version Increment Rule
- The version in `VERSION` and `src/Version.h` will be incremented from `0.7.50` to `0.7.51`.

## Test Plan (TDD-Ready)

### Test Suite 1: Math Utils (`tests/test_math_utils.cpp`)
- `test_biquad_notch_stability`:
    - Input: Impulse, step, and extreme noise (1e6).
    - Expected: Output remains finite and settles correctly.
- `test_inverse_lerp_behavior`:
    - Input: `min=0, max=10, val=15` -> Expected `1.0`.
    - Input: `min=10, max=0, val=5` -> Expected `0.5` (inverse logic).
    - Input: `min=5, max=5, val=5` -> Expected `1.0` (zero-range safety).
- `test_sg_derivative_ramp`:
    - Input: A linear ramp of values.
    - Expected: Derivative exactly matches the ramp slope (accounting for `dt`).
- `test_sg_derivative_buffer_states`:
    - Boundary: Empty buffer, 1-sample buffer, half-full, exactly full, wrapped-around.
    - Expected: Returns `0.0` until `window` is reached; handles wraparound correctly.

### Test Suite 2: Perf Stats (`tests/test_perf_stats.cpp`)
- `test_channel_stats_average`:
    - Input: Series `[10, 20, 30]`.
    - Expected: `Avg() == 20.0`.
- `test_channel_stats_resets`:
    - Action: Update, `ResetInterval()`, Update new values.
    - Expected: `session_max` captures both intervals; `interval_sum` only captures the second.

### Test Suite 3: Vehicle Utils (`tests/test_vehicle_utils.cpp`)
- `test_vehicle_class_parsing_keywords`:
    - Inputs: `"LMP2 ELMS"`, `"GTE Pro"`, `"GT3 Gen 2"`, `"Random Car"`.
    - Expected: Map to `LMP2_UNRESTRICTED`, `GTE`, `GT3`, `UNKNOWN` respectively.
- `test_vehicle_class_case_insensitivity`:
    - Inputs: `"gt3"`, `"GT3"`, `"Gt3"`.
    - Expected: All map to `GT3`.

## Deliverables
- [ ] Updated `src/FFBEngine.h` (comments only).
- [ ] `tests/test_math_utils.cpp`.
- [ ] `tests/test_perf_stats.cpp`.
- [ ] `tests/test_vehicle_utils.cpp`.
- [ ] Projects builds successfully.
- [ ] All tests (old + new) pass.
- [ ] Version incremented to `0.7.51`.

## Implementation Notes
- Use `FFBEngineTestAccess` for accessing the private/protected math methods in `FFBEngine`.
- Ensure new files are added to `tests/CMakeLists.txt`.
