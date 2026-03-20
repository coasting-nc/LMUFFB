# Implementation Plan - Issue #426: Pause/Menu Spikes when the wheel is off-center

## Context
Users experience loud mechanical "clacks" or spikes when pausing the game or entering the menu if the steering wheel is not centered. This is caused by the Force Feedback (FFB) torque dropping to zero too abruptly (within ~10ms), resulting in a sudden release of motor tension that high-torque Direct Drive wheels translate into a physical shock.

## Design Rationale
The core philosophy of LMUFFB is "Reliability and Safety". While muting FFB when the player is not in control is a safety requirement, doing so via a near-instantaneous step function introduces mechanical instability. By transitioning from a 100.0 units/sec slew rate to a 2.0 units/sec slew rate during restricted states, we transform a "spike" into a "fade", preserving both the safety of muting the motor and the physical integrity of the user's hardware. A 0.5s fade-out time (1.0 / 2.0) is a well-established sweet spot in audio and haptics for "soft-muting" without perceived lag in the UI transition.

## Reference Documents
- GitHub Issue: [Pause/Menu Spikes when the wheel is off-center #426](https://github.com/coasting-nc/LMUFFB/issues/426)
- Existing Safety Logic: `src/ffb/FFBSafetyMonitor.cpp`

## Codebase Analysis Summary
- **Affected Component:** `FFBSafetyMonitor` class.
- **Impact zone:** The `ApplySafetySlew` function which uses `SAFETY_SLEW_RESTRICTED` when the `restricted` flag is true. This flag is typically set when `FFBEngine::calculate_force` determines that FFB should be muted (e.g., during pause, pits, or AI control).
- **Data Flow:** `FFBEngine` calls `FFBSafetyMonitor::ApplySafetySlew` in every frame of the high-frequency loop. When `allowed` is false, it passes `target_force = 0.0` and `restricted = true`.

## FFB Effect Impact Analysis
| Effect | Technical Change | User Perspective |
| :--- | :--- | :--- |
| **All Effects** | Slew rate limit during "muted" states reduced from 100.0 to 2.0. | FFB will smoothly fade out over 0.5s when pausing or entering pits, instead of snapping to zero. |

**Design Rationale:** This change affects the *global* output behavior during state transitions. It does not alter the physics of individual effects like understeer or road texture during active driving.

## Proposed Changes

1.  **Modify `src/ffb/FFBSafetyMonitor.h`**:
    -   Update `SAFETY_SLEW_RESTRICTED` constant.
    -   **Design Rationale:** Hardcoding this as a lower constant is the simplest and most reliable fix. While it could be a user setting, 0.5s is a universally safe default for this specific "uncontrolled" state transition.

2.  **Update `VERSION`**:
    -   Increment from `0.7.204` to `0.7.205`.

3.  **Update `CHANGELOG_DEV.md`**:
    -   Add entry for Issue #426 fix.

## Test Plan (TDD-Ready)

### 1. Reproduction & Verification Test: `test_issue_426_slew_rate`
- **Goal:** Prove that the restricted slew rate is significantly slower than the normal slew rate and follows the 0.5s fade-out target.
- **Test File:** `tests/test_issue_426_spikes.cpp`
- **Scenarios:**
    - **Scenario A (Normal Slew):** Start at 1.0, target 0.0, `restricted = false`. Expected: Delta per frame (at 400Hz/0.0025s) is `1000 * 0.0025 = 2.5`. Reaches 0.0 in 1 frame.
    - **Scenario B (Restricted Slew - New):** Start at 1.0, target 0.0, `restricted = true`. Expected: Delta per frame is `2.0 * 0.0025 = 0.005`. Reaches 0.0 in exactly 200 frames (0.5s).
- **Design Rationale:** By measuring the exact delta per frame, we verify the math behind the slew limit directly.

### 2. Regression Testing
- Run `./build/tests/run_combined_tests` to ensure existing safety window and spike detection logic (which also use `ApplySafetySlew`) still function correctly.

## Implementation Steps

1.  **Create Reproduction Test:** Implement `tests/test_issue_426_spikes.cpp` and verify it fails (or shows the fast 10ms drop).
2.  **Apply Fix:** Modify `src/ffb/FFBSafetyMonitor.h`.
3.  **Build:** Run `cmake --build build`.
4.  **Verify Fix:** Run the new test and ensure it passes with the 0.5s fade duration.
5.  **Iterative Review:** Request code review, save to `docs/dev_docs/code_reviews/issue_426_review_iteration_1.md`.
6.  **Finalize Docs:** Update `VERSION` and `CHANGELOG_DEV.md`.
7.  **Pre-Commit:** Complete pre-commit steps to ensure proper testing, verification, review, and reflection are done.
8.  **Submit:** Submit the changes.

## Deliverables
- [ ] Modified `src/ffb/FFBSafetyMonitor.h`
- [ ] New `tests/test_issue_426_spikes.cpp`
- [ ] Updated `VERSION` and `CHANGELOG_DEV.md`
- [ ] Implementation Plan with final notes
- [ ] Code review records in `docs/dev_docs/code_reviews/`

## Implementation Notes (Final)
- **Unforeseen Issues:** The test runner's `--filter` flag uses substring matching, which could pick up multiple tests (e.g., `Issue314` and `Issue316` if filtering for `Issue3`). Explicitly used `--tag=Slew` or more specific filter names to isolate the new test during development.
- **Plan Deviations:** None. The planned 2.0 slew rate (0.5s fade) was confirmed as ideal by the reproduction test.
- **Challenges:** Reproducing the 10ms spike in a unit test required careful accounting for the 400Hz `dt` (0.0025s) used by the `FFBSafetyMonitor`.
- **Recommendations:** In the future, we might consider making the "Fade-out Duration" a user-configurable setting in the GUI, but 0.5s is a very safe and standard default.
