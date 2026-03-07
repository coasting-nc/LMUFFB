# Implementation Plan - Issue #225: Fix detection of LMP2 (WEC) restricted car class

The goal of this task is to ensure that the LMU FFB engine correctly identifies the LMP2 WEC restricted class, ensuring consistent FFB load normalization.

- **Issue:** #225 "Fix detection of LMP2 (WEC) restricted car class"
- **Status:** Completed

## Design Rationale

### Context
In Le Mans Ultimate (LMU), different car classes have different physical characteristics, specifically regarding their maximum aerodynamic and mechanical loads. The FFB engine uses a "Seed Load" (in Newtons) to perform initial force normalization. If a car is identified as the wrong class, the wrong Seed Load is applied, leading to inconsistent FFB feel across different cars in the same session.

### Analysis
The current implementation of `ParseVehicleClass` in `src/VehicleUtils.cpp` identifies the string "LMP2" (when not accompanied by "ELMS" or "WEC") as `LMP2_UNSPECIFIED`. However, according to the issue report and LMU's internal scoring data, the string `"LMP2"` explicitly refers to the restricted WEC specification, while `"LMP2_ELMS"` refers to the unrestricted ELMS specification.

- **Current Behavior:** `"LMP2"` -> `LMP2_UNSPECIFIED` (8000N)
- **Correct Behavior:** `"LMP2"` -> `LMP2_RESTRICTED` (7500N)

### Proposed Changes
The fix involves updating the hierarchical string matching in `ParseVehicleClass`. Since "LMP2" is the base string for both "LMP2" (WEC) and "LMP2_ELMS", we should check for "ELMS" first, and if not found, treat "LMP2" as the restricted WEC version.

### Test Plan
We will use Test-Driven Development (TDD).
1. Update `tests/test_vehicle_utils.cpp` to expect `LMP2_RESTRICTED` for the input `"LMP2"`.
2. Confirm the test fails.
3. Update `src/VehicleUtils.cpp`.
4. Confirm the test passes.

## Codebase Analysis Summary

### Current Architecture
The vehicle classification logic is encapsulated in `VehicleUtils.h/cpp`. It provides `ParseVehicleClass`, which takes `className` and `vehicleName` (from Shared Memory) and returns a `ParsedVehicleClass` enum.

### Impacted Functionalities
- **Vehicle Classification:** `ParseVehicleClass` will return `LMP2_RESTRICTED` instead of `LMP2_UNSPECIFIED` for the "LMP2" string.
- **FFB Load Normalization:** The `FFBEngine` uses the result of `ParseVehicleClass` to call `GetDefaultLoadForClass`. Changing the returned enum will change the `m_car_max_torque_nm` seed value.

### Design Rationale for Impact Zone
The impact is localized to `VehicleUtils.cpp`. This is the single source of truth for class identification in the project, making it the safest and most maintainable place for the fix.

## FFB Effect Impact Analysis

| Effect | Technical Change | User Perspective |
| :--- | :--- | :--- |
| **Structural FFB** | Seed load changes from 8000N to 7500N. | LMP2 WEC cars will feel slightly stronger/more active because they are normalized against a lower peak load. |

### Design Rationale for FFB Feel
Reducing the seed load for LMP2 WEC cars from 8000N to 7500N correctly reflects their restricted performance. In an absolute Nm pipeline, this means the normalization multiplier will be slightly higher, preserving the "heaviness" of the steering rack even with restricted aero.

## Proposed Changes

### 1. `src/VehicleUtils.cpp`
- **Modify `ParseVehicleClass`**:
  - Update the LMP2 block to return `LMP2_RESTRICTED` if "LMP2" is found and "ELMS"/"DERESTRICTED" is not.

### 2. `VERSION`
- **Update**: Increment to `0.7.115`.

### 3. `CHANGELOG_DEV.md` & `USER_CHANGELOG.md`
- **Update**: Document the fix for Issue #225.

## Test Plan (TDD-Ready)

### Design Rationale for Tests
The unit tests in `tests/test_vehicle_utils.cpp` provide immediate feedback on string parsing without requiring the full engine or game telemetry. By updating the existing assertions, we ensure that the new logic is verified against the project's standards.

### Test Cases
- **Updated Test:** `ASSERT_EQ((int)ParseVehicleClass("LMP2", ""), (int)ParsedVehicleClass::LMP2_RESTRICTED);` (Was `LMP2_UNSPECIFIED`)
- **Regression Test:** Ensure `ParseVehicleClass("LMP2 ELMS", "")` still returns `LMP2_UNRESTRICTED`.

## Deliverables
- [x] Modified `src/VehicleUtils.cpp`
- [x] Updated `tests/test_vehicle_utils.cpp`
- [x] Updated `VERSION` (0.7.115)
- [x] Updated `CHANGELOG_DEV.md` and `USER_CHANGELOG.md`
- [x] Implementation Plan (this document)

## Implementation Notes
The fix was implemented following TDD principles.
- **Unforeseen Issues:** Several existing physics and coverage tests were also expecting `LMP2_UNSPECIFIED` (8000N) for the "LMP2" class string. These tests had to be updated to match the new correct behavior of 7500N.
- **Plan Deviations:** The plan was expanded to include updating other relevant test files (`test_ffb_load_normalization.cpp` and `test_ffb_coverage_target.cpp`) that were broken by the change in `VehicleUtils`.
- **Challenges:** The initial test run failed due to a missing dependency (`glfw3`) in the Linux environment, but this was resolved by using the `BUILD_HEADLESS=ON` flag which correctly handles the mock environment.
- **Code Reviews:** Iteration 1 highlighted missing documentation files (VERSION, changelogs, review records) and an incomplete implementation plan. These were addressed in the final iteration.
- **Recommendations:** For future car class additions, ensure all related unit tests are updated to prevent regressions in physical constants verification.
