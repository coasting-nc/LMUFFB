# Implementation Plan - Issue #355: Fix Suspension Force Interpretation

**Issue:** #355 - Fix Lockup Vibration and Suspension Bottoming with correct interpretation of mSuspForce as the pushrod/spring load (not the wheel load)

## 1. Design Rationale
The core of this issue is a physical misunderstanding of `mSuspForce`. In LMU (and rF2), `mSuspForce` is the force measured at the suspension's spring/damper (pushrod), which is linked to the wheel via a **Motion Ratio (MR)**.
- **Physics Reasoning:** Force at Wheel = Force at Pushrod * Motion Ratio. Without this normalization, effects using `mSuspForce` or its derivative are scaled incorrectly depending on the car's geometry.
- **Reliability:** By using `approximate_load()` (which now accounts for MR) as a fallback for `mTireLoad`, we ensure that safety-critical features like "Grounded Check" and "Bottoming Safety" work even when telemetry is encrypted (returning 0 for tire load).
- **Architectural Integrity:** Centralizing MR and unsprung weight values in `VehicleUtils` removes duplication and magic numbers from the physics engine, providing a single source of truth for car-class-specific geometry.

## 2. Reference Documents
- GitHub Issue: `docs/dev_docs/github_issues/issue_355.md`

## 3. Codebase Analysis Summary

### Current Architecture
- `FFBEngine`: Main physics loop and effect calculation.
- `GripLoadEstimation.cpp`: Contains `approximate_load` and `approximate_rear_load`.
- `VehicleUtils.cpp`: Handles vehicle classification and class-based defaults (e.g., seed loads).

### Impacted Functionalities
1.  **`FFBEngine::calculate_lockup_vibration`**: The `is_grounded` check currently uses `mSuspForce > 50.0`.
2.  **`FFBEngine::calculate_suspension_bottoming`**:
    - Method 1 (Impulse) uses raw `mSuspForce` derivative.
    - Safety Fallback uses `mTireLoad` directly without fallback for encrypted data.
3.  **`FFBEngine::approximate_load` & `approximate_rear_load`**: Currently contain hardcoded switch-case for MR and unsprung weight.

### Design Rationale for Impact Zone
These specific areas are where the software makes assumptions about the physical meaning of `mSuspForce`. By correcting these points, we ensure the entire pipeline treats suspension forces consistently.

## 4. FFB Effect Impact Analysis

| Effect | Technical Changes | User-Facing Change |
| :--- | :--- | :--- |
| **Lockup Vibration** | Use `approximate_load()` for grounding check. | More reliable vibration behavior on curbs/jumps and consistent across car classes. |
| **Suspension Bottoming** | Normalize impulse by MR; Add approximation fallback to safety trigger. | Prototypes (Hypercars) will stop "false-triggering" bottoming on minor bumps. Encrypted cars will now correctly trigger bottoming thumps. |

### Design Rationale for FFB Feel
Prototypes often felt "harsh" or "broken" in previous versions because their 0.5 MR doubled the apparent impulse of every bump. Normalizing this ensures a 100kN/s wheel hit feels like 100kN/s regardless of whether it's a GT3 or a Cadillac Hypercar.

## 5. Proposed Changes

### A. Vehicle Metadata Centralization (`src/VehicleUtils.h/cpp`)
- Implement `GetMotionRatioForClass(ParsedVehicleClass vclass)`
- Implement `GetUnsprungWeightForClass(ParsedVehicleClass vclass, bool is_rear)`
- **Design Rationale:** Encourages modularity. If we add more car classes in the future, we only update `VehicleUtils`.

### B. Load Estimation Refactoring (`src/GripLoadEstimation.cpp`)
- Modify `approximate_load` and `approximate_rear_load` to call the new `VehicleUtils` helpers.
- **Design Rationale:** Reduces code duplication between front and rear load approximation logic.

### C. Lockup Vibration Grounding Check (`src/FFBEngine.cpp`)
- Update `calculate_lockup_vibration` to use `ctx.frame_warn_load ? approximate_load(w) : w.mTireLoad` for the `is_grounded` check.
- **Design Rationale:** `mTireLoad` is always the preferred "truth" if available. The approximation is an accurate physical model for when the game hides the truth.

### D. Suspension Bottoming Normalization (`src/FFBEngine.cpp`)
- Update `calculate_suspension_bottoming` Method 1 to multiply `dForce` by MR.
- Update the Safety Trigger to use `approximate_load` when `ctx.frame_warn_load` is active.
- **Design Rationale:** Ensures consistent "thump" intensity across all cars.

### Version Increment Rule
- Increment `VERSION` and `src/Version.h` (if applicable) by the smallest possible increment (e.g., 0.7.174 -> 0.7.175).

## 6. Test Plan (TDD-Ready)

### New Unit Tests in `tests/test_ffb_physics.cpp`
1.  **`Test_MotionRatio_Values`**:
    - **Inputs:** `ParsedVehicleClass::HYPERCAR`, `ParsedVehicleClass::GT3`.
    - **Expected Outputs:** 0.50 and 0.65 respectively.
2.  **`Test_LoadApproximation_Consistency`**:
    - **Inputs:** `mSuspForce = 10000N` for a Hypercar.
    - **Expected:** `(10000 * 0.5) + 400 = 5400N`.
3.  **`Test_Bottoming_Impulse_Normalization`**:
    - **Scenario:** Simulate a 200kN/s pushrod impulse on a Hypercar (MR 0.5).
    - **Expected:** Normalized wheel impulse is 100kN/s. Effect should just barely trigger (threshold 100kN/s).
4.  **`Test_Bottoming_Safety_Fallback`**:
    - **Scenario:** `mTireLoad = 0`, `mSuspForce = 40000N` (extreme load).
    - **Expected:** `approximate_load` returns ~20400N (for GT3). This exceeds `m_static_front_load * 2.5` (~12000N), triggering the safety thump.

### Design Rationale for Tests
These tests cover the mathematical core of the fix. By verifying the MR and the normalized impulse calculation, we prove that the "hyper-sensitivity" bug is resolved before even running the code.

## 7. Deliverables
- [x] Modified `src/VehicleUtils.h` & `src/VehicleUtils.cpp`
- [x] Modified `src/GripLoadEstimation.cpp`
- [x] Modified `src/FFBEngine.cpp`
- [x] New/Updated tests in `tests/test_ffb_physics_issue_355.cpp`
- [x] Updated `VERSION` and `CHANGELOG_DEV.md`
- [x] Updated Implementation Plan with final notes.

## 8. Implementation Notes

### Unforeseen Issues
- **Registration Hack**: Unit tests in `tests/test_ffb_physics_issue_355.cpp` required explicit inclusion in `tests/CMakeLists.txt` to be recognized by the test runner.
- **State Seeding**: `m_prev_susp_force` needed an explicit update in the post-calculation block of `FFBEngine::calculate_force` to ensure Method 1 (Impulse) worked correctly in subsequent frames.

### Plan Deviations
- Added `mGripFract` clarification comment in `src/GripLoadEstimation.cpp` as requested by user.
- Added "CRITICAL" warning comments in `InternalsPlugin.hpp` and `FFBEngine.cpp` to prevent future misuse of `mSuspForce`.

### Challenges
- Simulated 400Hz upsampling and 100Hz telemetry transitions in unit tests to accurately verify derivative-based effects (Bottoming Impulse).

### Recommendations
- Future vehicle classes should have their Motion Ratio and Unsprung Weight added to `VehicleUtils.cpp` immediately to maintain FFB consistency.
