# Implementation Plan - Fix Soft Lock Weakness and Normalization Issue (#181)

## Context
Issue #181 reports that the Soft Lock feature is too weak, especially when the car is moving. Analysis reveals that this is because Soft Lock is currently treated as a structural force and normalized by the learned session peak torque. As peak torque increases during driving, the relative strength of the Soft Lock (which is calculated in absolute Nm) decreases.

## Reference Documents
- GitHub Issue #181: Soft lock too weak and only working when car is standing still
- `tests/test_issue_181_soft_lock_weakness.cpp`: Regression test confirming the behavior.

## Codebase Analysis Summary
- **Current Architecture**: `FFBEngine::calculate_force` sums various force components. Structural forces (understeer, SoP, gyro, soft lock) are summed and then normalized by `m_session_peak_torque`.
- **Impacted Functionalities**:
    - `FFBEngine::calculate_force`: Summation logic will be modified.
    - `SteeringUtils.cpp`: `calculate_soft_lock` implementation (verified, but might need comment updates).
    - `tests/test_ffb_soft_lock.cpp`: Existing tests might need adjustment due to scaling changes.

## FFB Effect Impact Analysis
| Effect | Technical Changes | User-Facing Impact |
| :--- | :--- | :--- |
| **Soft Lock** | Move from normalized `structural_sum` to absolute `texture_sum_nm`. | Becomes significantly stronger and consistent regardless of how fast or hard the car is driven. |

## Proposed Changes

### 1. `src/FFBEngine.cpp`
- **Modify `FFBEngine::calculate_force`**:
    - Remove `ctx.soft_lock_force` from the `structural_sum` calculation (around line 500).
    - Add `ctx.soft_lock_force` to the `texture_sum_nm` calculation (around line 510).
    - This ensures it is scaled by `1.0 / m_wheelbase_max_nm` instead of `1.0 / m_session_peak_torque`.

### 2. `VERSION` and `src/Version.h`
- Increment version from `0.7.76` to `0.7.77`.

## Test Plan (TDD-Ready)
- **`test_soft_lock_normalization_consistency`**:
    - **Description**: Verify that Soft Lock output is identical for different session peak torques.
    - **Inputs**: Steer = 1.001, Session Peak = 1.0 vs 50.0.
    - **Expected Output**: Both should return the same force (e.g., ~33% for a 15Nm wheel).
- **`test_soft_lock_strength_regression`**:
    - **Description**: Ensure Soft Lock reaches full force at reasonable excess.
    - **Inputs**: Steer = 1.01 (1% excess), Stiffness = 100%.
    - **Expected Output**: 1.0 (Full force).

## Deliverables
- [x] Modified `src/FFBEngine.cpp`
- [x] Updated `tests/test_issue_181_soft_lock_weakness.cpp`
- [x] Updated `VERSION` (Note: `src/Version.h` is auto-generated)
- [x] Implementation Notes

## Implementation Notes
- **Issue Resolved**: Soft Lock was being weakened by session peak torque normalization.
- **Fix**: Relocated Soft Lock to the absolute Nm scaling group (Textures).
- **Result**: Soft Lock is now consistently strong and independent of learned peaks. Verified by tests to reach 100% force at 1% steering excess.
- **Deviations**: Reverted manual edits to `src/Version.h` as the file is auto-generated in the build directory. Updated tests to use the standard `TEST_CASE` macro for auto-registration. Corrected hallucinated v0.7.76 changelog entries and adjusted the current version to 0.7.76.
- **Iterative Review Process**:
    - **Iteration 1**: Initial review of the move to absolute Nm scaling. Feedback noted missing `Version.h` macro and artifact storage.
    - **Iteration 2**: Review after fixing test macro, truncating `src/Version.h`, and adding artifacts.
    - **Iteration 3**: Review before submission, identifying remaining nitpicks in artifact storage.
    - **Iteration 4**: Final review after removing hallucinated entries and storing all iterative reviews. Solution rated #Correct#.
- **Recommendations**: Monitor user feedback regarding damping. The increased effective strength might make damping feel more aggressive too.
