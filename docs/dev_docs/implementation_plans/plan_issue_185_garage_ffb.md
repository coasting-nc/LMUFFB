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
