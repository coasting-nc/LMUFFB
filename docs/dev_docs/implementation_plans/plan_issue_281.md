# Implementation Plan - Issue #281: Fix FFB Spikes on Driving State Transition

## Context
Issue #281 reports FFB spikes when switching from `IsPlayerActivelyDriving() == true` to `false`. This typically occurs when pausing the game or when AI takes control. The root cause is that while most physics are muted, the Soft Lock effect persists and may use frozen telemetry, causing a "punch" if the wheel was turned when the transition occurred.

## Design Rationale
- **User Safety**: Sudden jumps in FFB are a safety risk and degrade the user experience.
- **Architectural Consistency**: Gating the final physics force in the main loop ensures that all components (including Soft Lock) respect the driving state while allowing the slew rate limiter to handle the transition.
- **Physics-Based Smoothing**: Using the existing `ApplySafetySlew` with the `restricted` flag (100 units/s) ensures that the wheel relaxes over ~10-20ms rather than snapping, which is physically safer for hardware.

## Codebase Analysis Summary
- **Main Loop**: `src/main.cpp` calculates `is_driving` using `GameConnector::Get().IsPlayerActivelyDriving()`.
- **Muting Logic**: It currently relies on `calculate_force` with `full_allowed = false` to mute physics.
- **Soft Lock Exception**: `FFBEngine::calculate_force` explicitly preserves `soft_lock_force` even when `allowed` is false.
- **Impact Zone**: The transition in `main.cpp` where `is_driving` becomes false is where the spike occurs.

## FFB Effect Impact Analysis
| Effect | Technical Changes Needed | User-facing Changes |
| :--- | :--- | :--- |
| **Soft Lock** | Will be explicitly zeroed in `main.cpp` when `is_driving` is false. | Wheel relaxes in menus/pause even if beyond lock. Stays active in garage. |
| **All other effects** | Already muted when `!is_driving` (via `full_allowed`). | Smoother transition to zero due to slew limiting. |

## Proposed Changes

### `src/main.cpp`
- Inside `FFBThread`, after `g_engine.calculate_force(...)`:
  ```cpp
  if (!is_driving) force_physics = 0.0;
  ```
- This ensures that when not actively driving (Paused, AI, or in UI), the target force is always zero, but it is still passed through `ApplySafetySlew` to ensure a smooth ramp.

### Metadata & Documentation
- Increment version to `0.7.148` in `VERSION`.
- Add entry to `CHANGELOG_DEV.md`.

## Test Plan
- **New Test File**: `tests/test_issue_281_spikes.cpp`
- **Test Case**: `test_issue_281_transition_smoothing`
  - Setup: Mock a car beyond lock (Steering = 1.1) to trigger Soft Lock.
  - Step 1: `is_driving = true`, `full_allowed = false` (Garage scenario). Verify FFB is ~-1.0 (Soft Lock).
  - Step 2: Set `is_driving = false` (Pause scenario).
  - Step 3: Run one frame of the FFB loop logic. Verify FFB has decreased but is NOT zero (slewing).
  - Step 4: Run multiple frames. Verify FFB reaches 0.0.
- **Design Rationale**: This test mimics the high-frequency FFB loop and proves that the main loop logic correctly smooths the transition to zero force even when internal engine components are still producing torque.

## Deliverables
- [x] Modified `src/main.cpp`
- [x] New `tests/test_issue_281_spikes.cpp`
- [x] Updated `VERSION`
- [x] Updated `CHANGELOG_DEV.md`
- [x] Final Implementation Notes in this plan.

## Implementation Notes
- **Encountered Issues**: Initially, the test scenario 1 for "Active Driving" needed a loop of frames to reach the target force because of the slew rate limiter. This was adjusted in the final test code.
- **Plan Deviations**: None. The implementation followed the proposed logic exactly.
- **Challenges**: Ensuring that Soft Lock still works in the garage stall while being muted in menus required careful checking of the `is_driving` predicate vs `full_allowed`.
- **Recommendations**: The current `restricted` slew rate of 100 units/s provides a 10ms relaxation time at 1000Hz. If users still find this too sudden, a separate `MENU_TRANSITION_SLEW` could be introduced, but 10ms is generally considered safe and smooth.
