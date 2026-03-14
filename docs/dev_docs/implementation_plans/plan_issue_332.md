# Implementation Plan - Issue #332: Fix regression introduced after version 0.7.152

## Context
Issue #332 reports two critical regressions affecting usability:
1.  **FFB Loss on Player Join/Leave:** In multiplayer, when players join or leave, the FFB gain drops significantly and takes time to recover.
2.  **Wheel Lock in Pits:** When returning to the pits or being in the garage, the wheel often goes to full lock and stays there until the user goes back on track.

## Design Rationale
### 1. FFB Loss (Safety Window Over-sensitivity)
The FFB engine implements a "Safety Window" that reduces gain to 30% for 2 seconds whenever a "Control Transition" is detected. Currently, any change in the `mControl` variable (which indicates if the player, AI, or nobody is in control) triggers this window.
In multiplayer, rFactor 2's shared memory often reorders the vehicle list when players join or leave. If the `playerVehicleIdx` or the corresponding `mControl` value flickers momentarily during this reordering, the engine triggers a safety reset.
**Solution:** We will implement a "debounced" or more specific transition check. Specifically, transitions to `ControlMode::NONE` (-1) followed by a quick return to `ControlMode::PLAYER` (0) should probably not trigger a full 2-second safety window, or at least we should be more selective about which transitions are "dangerous".

### 2. Wheel Lock in Pits (Soft Lock Persistence)
The engine preserves `soft_lock_force` even when FFB is muted (e.g., in the garage) to ensure the wheel doesn't jump if the user is past the lock. However, if the game reports invalid or extreme steering values (e.g., during a teleport to the pits), the Soft Lock engages. Because the user is in the garage, they cannot easily "steer back" to unlock it if the simulated rack is pinned.
**Solution:** Disable Soft Lock specifically when `mInGarageStall` is true. When in a garage stall, the car is effectively on jacks/stationary, and the physical steering rack shouldn't be simulated by the FFB engine.

## Codebase Analysis Summary
-   **FFBEngine.cpp / FFBEngine.h:** Contains the `calculate_force` loop, safety window logic, and `IsFFBAllowed`.
-   **main.cpp:** Handles the high-level coordination and calls `calculate_force`.
-   **SteeringUtils.cpp:** Implements `calculate_soft_lock`.

## FFB Effect Impact Analysis
-   **Soft Lock:** Will be disabled in the garage stall. This improves usability by preventing "stuck" wheels in the pits. Realism is unaffected as the car isn't being driven in the garage stall.
-   **Safety Window:** Will be less prone to false positives during multiplayer joins/leaves. This improves the "feel" by preventing unexpected gain drops during races.

## Proposed Changes
### 1. `src/FFBEngine.h`
-   No changes expected.

### 2. `src/FFBEngine.cpp`
-   Modify `calculate_force` to refine `mControl` transition logic.
    -   Ignore transitions to/from `ControlMode::NONE` (-1) if they are transient?
    -   Or, only trigger "Control Transition" safety if transitioning TO or FROM `ControlMode::AI`.
-   Modify the FFB mute block to also mute `soft_lock_force` if the player is in a garage stall.

### 3. `src/SteeringUtils.cpp`
-   Potentially add a check for `mInGarageStall` (though passing it through `calculate_force` is cleaner).

### 4. `src/main.cpp`
-   Ensure `scoring.mInGarageStall` is correctly passed or handled.

## Parameter Synchronization Checklist
N/A (No new settings)

## Version Increment Rule
-   Increment `VERSION` to `0.7.183`.

## Test Plan
### 1. test_issue_332_mcontrol_glitch
-   **Scenario:**
    1.  Frame 1: `mControl = 0` (Player) -> Normal FFB.
    2.  Frame 2: `mControl = -1` (None) -> Muted.
    3.  Frame 3: `mControl = 0` (Player) -> Should NOT have 70% gain reduction if it was a transient glitch.
-   **Assertions:** Check `m_safety.safety_timer` is still 0.0.

### 2. test_issue_332_soft_lock_pits
-   **Scenario:**
    1.  Set `mInGarageStall = true`.
    2.  Set `mUnfilteredSteering = 1.5`.
    3.  Call `calculate_force`.
-   **Assertions:** `total_output` should be 0.0, despite being past the lock.

## Deliverables
-   Modified `src/FFBEngine.cpp`
-   New test file `tests/test_issue_332.cpp`
-   Updated `VERSION`
-   Updated `CHANGELOG_DEV.md`

## Implementation Notes
### Unforeseen Issues
- The initial reproduction test revealed that both `TriggerSafetyWindow` inside `calculate_force` (via `mControl` transition) and the `FFB Unmuted` trigger were causing the gain drop. Both needed to be refined to ignore `ControlMode::NONE (-1)`.

### Plan Deviations
- Added a check in the `FFB Unmuted` logic as well, which was not explicitly detailed in the original design rationale but became obvious during test refinement.
- Incremented version to 0.7.183 and updated both dev and user changelogs.

### Challenges
- Simulating the exact multiplayer "glitch" requires careful ordering of `calculate_force` calls in the test case to ensure `last_mControl` and `last_allowed` states are correctly exercised.

### Suggestions for the Future
- Consider a more general "debounce" for all safety triggers if other telemetry flickers are found to cause issues.
- The `inGarage` flag could be used for other garage-specific logic if needed (e.g., muting road textures even if speed is artificially high).
