# Implementation Plan - Issue #184: Soft Lock Stationary

## Context
Soft Lock provides physical resistance at the steering limits. Currently, it only works when the car is in a "realtime" session and the player is in control. Users have reported that it should work when the car is stationary as well (including in the garage/pits before taking control).

## Reference Documents
- GitHub Issue #184
- `src/FFBEngine.cpp`, `src/main.cpp`, `src/SteeringUtils.cpp`

## Codebase Analysis Summary
- **Current Architecture:** FFB calculation is driven by `FFBThread` in `main.cpp`. It only calls `FFBEngine::calculate_force` if `in_realtime` is true and `IsFFBAllowed` returns true.
- **Impacted Functionalities:**
    - `FFBThread` in `main.cpp`: Gating logic needs to be more permissive for safety effects like Soft Lock.
    - `FFBEngine::calculate_force`: Needs to handle a "muted" or "not allowed" state where only safety effects are calculated.

## FFB Effect Impact Analysis
| Effect | Technical Changes | User Perspective |
| :--- | :--- | :--- |
| **Soft Lock** | Will be calculated even when `allowed` is false. | Works in garage and when AI is driving. |
| **Game FFB** | Muted when `allowed` is false. | No unwanted forces in garage (Fixes #185). |
| **Lateral Effects** | Muted when `allowed` is false. | No SoP or Yaw effects when not driving. |
| **Tactile Effects** | Muted when `allowed` is false. | No vibrations when not driving. |

## Proposed Changes

### 1. `src/FFBEngine.h`
- Update `calculate_force` signature:
  ```cpp
  double calculate_force(const TelemInfoV01* data, const char* vehicleClass = nullptr, const char* vehicleName = nullptr, float genFFBTorque = 0.0f, bool allowed = true);
  ```

### 2. `src/FFBEngine.cpp`
- Update `calculate_force` implementation:
    - If `allowed` is `false`, zero out `output_force` (the structural part).
    - If `allowed` is `false`, zero out all `ctx` effects EXCEPT `soft_lock_force`.
    - Ensure `calculate_gyro_damping` is still called (to update `m_steering_velocity_smoothed`) but its output `ctx.gyro_force` is zeroed if `!allowed`.

### 3. `src/main.cpp`
- Modify `FFBThread` loop:
    - Remove `in_realtime` check from the outer `if`.
    - Calculate `allowed` based on `in_realtime && g_engine.IsFFBAllowed(...)`.
    - Pass `allowed` to `calculate_force`.
    - Ensure `should_output` is true if we have valid telemetry, even if `!allowed`.

## Parameter Synchronization Checklist
N/A (No new settings)

## Version Increment Rule
Increment version to v0.7.78.

## Test Plan
- **New Test Case:** `test_soft_lock_stationary_not_allowed`
    - Set `allowed = false`.
    - Set `steer = 1.1`.
    - Verify `force` is correctly calculated for soft lock.
- **New Test Case:** `test_other_forces_muted_when_not_allowed`
    - Set `allowed = false`.
    - Set `raw_torque = 1.0`, `speed = 50.0`.
    - Verify `force` is 0.0 (except for soft lock).
- **Regression:** Run full test suite.

## Deliverables
- [x] Modified `src/FFBEngine.h`
- [x] Modified `src/FFBEngine.cpp`
- [x] Modified `src/main.cpp`
- [x] New tests in `tests/test_issue_184_repro.cpp`
- [x] Updated `VERSION` and `src/Version.h` (via CMake)
- [x] Updated `CHANGELOG_DEV.md`

## Implementation Notes
### Unforeseen Issues
- None. The implementation followed the plan smoothly.

### Plan Deviations
- None.

### Challenges Encountered
- Determining the exact meaning of "stationary" from the issue. Exploration of the code and telemetry flags revealed that the main gating factor was `in_realtime` and `IsFFBAllowed`, which prevent Soft Lock in the garage and during AI control.

### Recommendations for Future Plans
- Continue decoupling safety effects from dynamic physics effects to allow for better hardware protection in all game states.
