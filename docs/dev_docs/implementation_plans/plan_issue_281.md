# Implementation Plan - Issue #281: Fix FFB Spikes on Driving State Transition

## Context
Issue #281 reports FFB spikes when switching from `IsPlayerActivelyDriving() == true` to `false`. This typically occurs when pausing the game, returning to the garage, or when AI takes control. The root cause is that while most physics are muted, certain effects (like Soft Lock) might persist and use frozen telemetry if not gated correctly at the final output stage.

## Design Rationale
- **User Safety**: Sudden jumps in FFB are a safety risk.
- **Selective Gating**: By using `mControl != 0` as the gate for total force suppression, we allow Soft Lock to remain active in the garage and during pause (where the player is still "in control" of the car's wheel, even if not driving), while ensuring all force is smoothly slewed to zero when the player is truly no longer in control (AI, Replay, or Main Menu).
- **Physics-Based Smoothing**: Using the existing `ApplySafetySlew` ensures that the transition to zero force (when `mControl != 0`) is handled over ~10-20ms, preventing a "clunk".

## Codebase Analysis Summary
- **Main Loop**: `src/main.cpp` uses `is_driving` (from `IsPlayerActivelyDriving()`) to determine `full_allowed`.
- **Muting Logic**: `FFBEngine::calculate_force` mutes most physics if `!allowed`, but preserves `soft_lock_force`.
- **Current Gating**: v0.7.148 added `if (!is_driving) force_physics = 0.0;` which successfully stopped the spikes but also disabled Soft Lock in the garage and during pause.
- **Improved Gating**: Replacing the gate with `if (scoring.mControl != 0)` targets the actual "no longer in control" state.

## FFB Effect Impact Analysis
| Effect | Technical Changes Needed | User-facing Changes |
| :--- | :--- | :--- |
| **Soft Lock** | Will remain active in garage/pause (`mControl == 0`). Zeroed when `mControl != 0`. | Soft Lock protection persists in garage and pause. |
| **Other effects** | Muted when `!is_driving` via `full_allowed` in `calculate_force`. | No change. |

## Proposed Changes

### `src/main.cpp`
- Modify the gating logic in `FFBThread`:
  ```cpp
  // Replace:
  if (!is_driving) force_physics = 0.0;
  // With:
  if (scoring.mControl != 0) force_physics = 0.0;
  ```
- This ensures that if the player is in control (even if paused or in garage), the engine's calculated force (which will be just Soft Lock if `!is_driving`) is passed through. If AI/Remote takes over, force is targeted to zero.

### Metadata & Documentation
- Increment version to `0.7.153` in `VERSION`.
- Add entry to `CHANGELOG_DEV.md`.

## Test Plan
- **Updated Test**: `tests/test_issue_281_spikes.cpp`
- **Scenario 1: Garage/Pause (`is_driving = false`, `mControl = 0`)**
  - Setup: Wheel beyond lock.
  - Expectation: `force_physics` should be Soft Lock (~1.0), NOT zero.
- **Scenario 2: AI Takeover (`mControl = 1`)**
  - Setup: Wheel beyond lock.
  - Expectation: `force_physics` should target 0.0 and slew towards it.
- **Design Rationale**: Proves that Soft Lock is preserved where useful but muted where dangerous.

## Deliverables
- [x] Modified `src/main.cpp`
- [x] Modified `tests/test_issue_281_spikes.cpp`
- [x] Updated `VERSION`
- [x] Updated `CHANGELOG_DEV.md`

## Implementation Notes
- **Update (Iteration 1)**: The initial fix in v0.7.148 was too aggressive. Using `mControl` as the primary gate for total suppression is more refined as it respects the player's presence in the cockpit.
- **Update (Iteration 2)**: Refined the gate to use `scoring.mControl != 0`. This targets only the states where the player is not in control (AI, Replay, etc.), allowing Soft Lock to remain functional when stationary or paused while still preventing transition spikes when AI takes over.
- **Build/Test**: Verified with `tests/test_issue_281_spikes.cpp`. Building on Linux required `-DBUILD_HEADLESS=ON` due to missing GLFW3.

## CI Trigger Investigation
The initial submission did not trigger the `.github/workflows/windows-build-and-test.yml` workflow.
- **Analysis**: The workflow contains a `paths-ignore` for `**.md` files. While the submission included non-MD files (`VERSION`, `src/main.cpp`, etc.), it is possible that the CI system evaluates changes based on the primary content or that the agent's submission mechanism (API-based push) did not satisfy the trigger conditions for "push" in this specific repository configuration.
- **Verification**: Confirmed that `src/main.cpp` and `tests/test_issue_281_spikes.cpp` are correctly modified and staged. These files should have triggered the build as they are outside the `paths-ignore` scope.
- **Action**: Performing a manual commit and push attempt via the environment to ensure the remote receives the non-MD changes.
