# Implementation Plan - Issue #174: Stuck FFB in Menu

## Context
Issue #174 reports that FFB (specifically a strong force or "punching") persists when the game is interrupted by entering a menu or going back to the garage. This is caused by the "Soft Lock" feature remaining active when the game is not in real-time, using potentially frozen or non-representative telemetry from the last active frame.

## Reference Documents
- GitHub Issue #174: "if the ffb is interrupted(menu,or back to garage) the wheel keeps punching to left or right side and dont stop until u are back in the car"
- `docs/dev_docs/implementation_plans/plan_issue_184.md` (Related Soft Lock stationary support)

## Codebase Analysis Summary
- **Current Architecture**: The application runs a 400Hz FFB loop in `src/main.cpp`. It connects to LMU via shared memory using `GameConnector`. Telemetry and scoring info are copied into a local buffer.
- **FFB Thread Logic**: The thread currently checks `!is_stale && g_localData.telemetry.playerHasVehicle`. If true, it determines `full_allowed` (based on `scoring.mControl == 0`, `mIsPlayer`, etc.) and calls `g_engine.calculate_force`.
- **FFB Engine**: `FFBEngine::calculate_force` uses the `allowed` parameter to mute most forces, but it explicitly preserves `soft_lock_force` (v0.7.78).
- **Issue Root Cause**: When entering the Esc menu, `in_realtime` becomes false. However, `playerHasVehicle` and `playerVehicleIdx` remain valid. The FFB loop continues to call `calculate_force` with `full_allowed = false` (because `in_realtime` is false). Since `calculate_force` preserves Soft Lock even when `!allowed`, if the wheel was at a high steering angle when pausing, the Soft Lock force "punches" back and stays active because telemetry is frozen or the engine state isn't reset.

## FFB Effect Impact Analysis
| Effect | Technical Changes Needed | User-facing Changes |
| :--- | :--- | :--- |
| **Soft Lock** | Will be muted in `main.cpp` when `in_realtime` is false. | Wheel relaxes when entering menus. No "stuck" forces. |
| **All other effects** | Already muted when `!allowed` (which includes `!in_realtime`). | No change; already silent in menus. |

## Proposed Changes

### `src/main.cpp`
- In `FFBThread`, update the conditional for calling `calculate_force`.
- Wrap the `calculate_force` call with an `if (in_realtime)` check.
- Ensure that if `!in_realtime`, `force` is explicitly set to `0.0` before the safety slew limiter.

### Version Increment Rule
- Increment version to `0.7.108` in `VERSION` and `CHANGELOG_DEV.md`.

## Parameter Synchronization Checklist
N/A - No new parameters added.

## Initialization Order Analysis
N/A - No changes to header dependencies or constructors.

## Test Plan (TDD-Ready)
### Unit/Integration Tests
- **Test Case**: `test_issue_174_menu_muting` in `tests/test_issue_174_repro.cpp`.
- **Setup**:
    - Mock telemetry where `playerHasVehicle = true`.
    - Set `in_realtime = false`.
    - Set `unfiltered_steering = 1.1` (should trigger soft lock).
- **Execution**: Run a simulation of the `FFBThread` logic or a dedicated test harness that mimics it.
- **Expected Result**: `force == 0.0`.
- **Baseline**: Current code (v0.7.107) should produce a non-zero force in this scenario.

### Boundary Conditions
- **Transition**: `in_realtime` toggling from `true` to `false`. Verify that `ApplySafetySlew` correctly ramps the force down instead of a hard jump.

## Deliverables
- [ ] Modified `src/main.cpp`
- [ ] New `tests/test_issue_174_repro.cpp`
- [ ] Updated `tests/CMakeLists.txt`
- [ ] Updated `VERSION`
- [ ] Updated `CHANGELOG_DEV.md`
- [ ] Updated Implementation Plan with notes.

## Implementation Notes
- **Strategy**: The fix involves explicitly setting the FFB force to 0.0 when `in_realtime` is false in the main FFB loop (`src/main.cpp`).
- **Safety**: The force override happens *before* `ApplySafetySlew`, ensuring that the wheel relaxes smoothly when entering a menu, rather than snapping to zero.
- **Side Effects**: `calculate_force` is still called even when not in real-time. This is intentional to keep the engine's internal state updated, but the resulting force is discarded in favor of 0.0 if the game is paused/in-menu.
- **Verification**: Verified that Soft Lock remains active in real-time "not allowed" states (like stationary garage or AI driving) but is correctly zeroed out in the pause menu.

### Unforeseen Issues
- The initial implementation attempt in Review 1 incorrectly moved `should_output = true` inside a conditional, which would have caused the force to "freeze" on some hardware instead of relaxing. This was corrected in Iteration 2 by explicitly setting `force = 0.0` while keeping `should_output = true`.

### Plan Deviations
- Added `VERSION` and `CHANGELOG_DEV.md` updates to the implementation loop as requested by standard "Fixer" workflows.

### CI Failure: Deprecated Artifact Actions & Permissions
During the PR scan, the `scan-pr / scan-pr` job failed multiple times.
1.  **Deprecated Version**: Failed because of `actions/upload-artifact@v3` usage.
    - **Cause**: The `.github/workflows/osv-scanner.yml` workflow used `google/osv-scanner-action` v1.7.1, which internally depended on the deprecated v3 artifact action. GitHub now automatically fails such requests (see [GitHub Blog](https://github.blog/changelog/2024-04-16-deprecation-notice-v3-of-the-artifact-actions/)).
    - **Resolution**: Updated `.github/workflows/osv-scanner.yml` to use `google/osv-scanner-action@v2.3.3`.
2.  **Missing Permissions**: Failed after updating to v2.3.3 because of insufficient permissions.
    - **Cause**: The newer version of the scanner action requires `actions: read` permission to function, which was missing from the workflow's `permissions` block.
    - **Resolution**: Added `actions: read` to the `permissions` section in `.github/workflows/osv-scanner.yml`.

- **Recommendations**: Regularly audit and update GitHub Action versions to their latest stable releases and ensure the corresponding workflow permissions meet the requirements of the updated actions to prevent sudden CI breakage.

### Recommendations
- Future improvements could consider a "Slow Relax" mode for menu entry if the current 1000 units/s slew is still too fast for some users, though it currently matches "Normal" driving safety levels.
