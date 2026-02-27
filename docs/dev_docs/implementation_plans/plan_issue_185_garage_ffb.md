# Implementation Plan - Issue #185: Some FFB active when "in the garage"

## Context
User reported that while in the garage view, there is a small amount of FFB active (wheel slowly rotates and has a "grinding" feeling). This should be zero when the car is stationary in the garage.

## Proposed Changes

### `src/FFBEngine.cpp`
- In `calculate_force`, when `allowed` is `false`, ensure the final `norm_force` is exactly `0.0` unless `soft_lock_force` is active.
- Specifically, the `min_force` logic should not be applied to the structural/tactile sum if it was muted by the `!allowed` condition, unless there is a significant intentional force like Soft Lock.
- I will move the `min_force` application to only apply if `allowed` is true OR if `soft_lock_force` is non-zero.

## Test Plan
- Create `tests/test_issue_185_repro.cpp` to reproduce the issue.
- Verify that `calculate_force` with `allowed=false` and `steer=0` returns non-zero if `m_min_force` is set.
- Fix the code and verify the test passes (returns 0.0).

## Deliverables
- `src/FFBEngine.cpp` modified.
- `tests/test_issue_185_fix.cpp` created.
- `VERSION` updated to `0.7.85`.
- `CHANGELOG_DEV.md` updated.

## Implementation Notes

### Unforeseen Issues
- Initially created both `test_issue_185_repro.cpp` and `test_issue_185_fix.cpp` with identical test case names within the same namespace, which caused a build failure due to duplicate symbols.

### Plan Deviations
- Consolidated all tests into `tests/test_issue_185_fix.cpp` and deleted the `repro` file to resolve the build conflict.
- Explicitly verified that Soft Lock still functions even when FFB is muted in the garage, provided it exceeds a 0.1 Nm threshold.

### Challenges
- Reproducing the "grinding" feel in a unit test environment required carefully selecting torque values that fall between the minimum force threshold and the noise floor.

### Recommendations
- Future FFB muting logic should continue to follow the "Safety First" pattern by preserving Soft Lock (rack limits) even when environmental forces are disabled.

## Analysis of "Snap to Center" on "Drive"

The user reported a "very sudden snap turn back to center when going back into the car" (clicking the "Drive" button), which has "almost claimed a few fingers".

### Cause Analysis
This "snap" is a direct consequence of the wheel having rotated away from center while in the garage stall. The reported "slow rotation" in the garage (caused by telemetry noise being amplified by `min_force`) puts the wheel at an offset (e.g., 180 degrees). When the user clicks "Drive", the FFB is suddenly re-enabled. The game's physics (specifically the Self-Aligning Torque of the stationary tires) immediately sees a large steering offset and generates a massive force to return the wheel to center. Because the transition was instantaneous, the wheel "snaps".

### How this PR addresses it
1.  **Prevents the Offset**: By muting FFB and bypassing `min_force` while in the garage stall (`mInGarageStall`), the wheel will no longer "slowly rotate" or "grind". It will remain exactly where the user left it (usually center).
2.  **Eliminates the Jolt**: Since the wheel is no longer offset when the user clicks "Drive", there is no large error for the physics engine to correct, thus no "snap".
3.  **Safety Buffer**: The preservation of Soft Lock ensures that even if the user manually turns the wheel to the rack limit while in the menu, they will still feel the physical stop, preventing them from unknowingly holding the wheel against a high-tension limit that would release upon clicking "Drive".
4.  **Existing Slew Protection**: The engine already possesses a `ApplySafetySlew` mechanism (v0.7.49). While this limits the rate of change, a jump from 0% to 100% force can still occur very quickly (one or two frames at 400Hz). By ensuring the target force starts at 0 (due to lack of offset), this slew limiter will only have to handle small adjustments rather than a full-scale correction.

By fixing the root cause (unwanted movement in the garage), the dangerous side effect (the snap-back) is also resolved.
