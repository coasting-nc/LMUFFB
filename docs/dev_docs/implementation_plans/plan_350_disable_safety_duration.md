# Implementation Plan - Disable by default "Safety Duration" time window #350

This plan outlines the changes required to disable the "Safety Duration" (FFB safety mitigation window) by default by setting its default value to 0 seconds across the engine, presets, and configuration loading.

## User Request
Disable by default "Safety Duration" time window #350
Link: https://github.com/coasting-nc/LMUFFB/issues/350

## Design Rationale (MANDATORY)
The "Safety Duration" feature was originally intended to protect users from dangerous FFB spikes by muting or reducing FFB for a period after a spike or system stutter is detected. However, users have reported that stutters caused by players joining or leaving online servers trigger this safety window unnecessarily, leading to a loss of FFB during driving when no actual danger is present. Setting the default to 0 effectively disables this mitigation window while keeping the detection logic active (for logging/diagnostics), providing a better user experience in multiplayer without compromising perceived safety.

## Proposed Steps

1.  **Modify `src/ffb/FFBEngine.h`**
    - Change `DEFAULT_SAFETY_WINDOW_DURATION` from `2.0f` to `0.0f`.
    - Verify change with `read_file`.
2.  **Modify `src/ffb/FFBSafetyMonitor.h`**
    - Change the default value of `m_safety_window_duration` from `2.0f` to `0.0f`.
    - Verify change with `read_file`.
3.  **Modify `src/core/Config.cpp`**
    - In `LoadPresets()`, find the "T300 v0.7.164" preset and change `p.safety_window_duration = 2.0f;` to `p.safety_window_duration = 0.0f;`.
    - Verify change with `read_file`.
4.  **Update existing tests**
    - Modify `tests/test_issue_316_safety_gui.cpp` to expect `0.0f` instead of `2.0f` in `test_safety_gui_defaults`.
    - Verify change with `read_file`.
5.  **Add new verification test**
    - Add a new test case to `tests/test_issue_316_safety_gui.cpp` (or a new test file) that iterates through all built-in presets in `Config::presets` and asserts that their `safety_window_duration` is `0.0f`.
    - Verify change with `read_file`.
6.  **Run all tests**
    - Compile and run all tests using `./build/tests/run_combined_tests` to ensure no regressions and that the new logic is correct.
7.  **Update Version and Changelog**
    - Increment the version in `VERSION` (0.7.187 -> 0.7.188).
    - Update `CHANGELOG_DEV.md` with a description of the change.
    - Verify changes with `read_file`.
8.  **Record Independent Code Review**
    - Request a code review and save it to `docs/dev_docs/code_reviews/review_iteration_1.md`.
    - Address any feedback if necessary.
9.  **Complete pre-commit steps**
    - Complete pre-commit steps to ensure proper testing, verification, review, and reflection are done.
10. **Submit the changes**
    - Submit the changes using the submission tool.

## FFB Effect Impact Analysis
| Effect | Technical Changes | User-facing Changes |
| :--- | :--- | :--- |
| **FFB Safety** | `safety_timer` will initialize to 0 or expire immediately if triggered with a 0s duration. | FFB will no longer be muted for 2 seconds after a stutter or spike detection. |

## Parameter Synchronization Checklist
- [x] Declaration in FFBEngine.h (member variable): `m_safety_window_duration` already exists.
- [x] Declaration in Preset struct (Config.h): `safety_window_duration` already exists.
- [x] Entry in `Preset::Apply()`: Already exists.
- [x] Entry in `Preset::UpdateFromEngine()`: Already exists.
- [x] Entry in `Config::Save()`: Already exists.
- [x] Entry in `Config::Load()`: Already exists.
- [x] Validation logic: `(std::max)(0.0f, ...)` already exists in `Config::Load` and `Preset::Apply`.

## Test Plan (TDD-Ready)
- `test_safety_gui_defaults`: Update to assert `0.0f`.
- `test_built_in_presets_safety_disabled`: New test that checks `Config::presets` after `Config::LoadPresets()`.
- Run all combined tests.

## Deliverables
- [x] Modified `src/ffb/FFBEngine.h`
- [x] Modified `src/ffb/FFBSafetyMonitor.h`
- [x] Modified `src/ffb/FFBSafetyMonitor.cpp`
- [x] Modified `src/core/Config.cpp`
- [x] Modified `tests/test_issue_316_safety_gui.cpp`
- [x] Modified `tests/test_issue_303_safety.cpp`
- [x] Modified `tests/test_issue_314_safety_v2.cpp`
- [x] Modified `tests/test_coverage_boost_v7.cpp`
- [x] Modified `tests/test_coverage_boost_v8.cpp`
- [x] Updated `VERSION`
- [x] Updated `CHANGELOG_DEV.md`
- [x] `docs/dev_docs/github_issues/350_disable_by_default_safety_duration.md`
- [x] `docs/dev_docs/code_reviews/review_iteration_1.md`
- [x] Updated Implementation Plan with notes.

## Implementation Notes

### Unforeseen Issues
- Setting the default to `0.0f` caused several existing safety regression tests to fail. These tests were designed to verify the 2-second mitigation window logic.
- **Resolution**: Updated all affected tests to explicitly set `m_safety_window_duration = 2.0f` before triggering safety events. This preserves the verification of the mitigation logic while allowing the global default to be 0.

### Plan Deviations
- Added a modification to `src/ffb/FFBSafetyMonitor.cpp` to explicitly zero the `safety_timer` when `m_safety_window_duration` is near zero. This ensures that even if a trigger occurs, the mitigation window does not stay active for a single frame or due to floating point precision issues.

### Challenges
- Ensuring all built-in presets were correctly accounted for. The new `test_built_in_presets_safety_disabled` test was instrumental in verifying this across all 23+ built-in presets.

### Recommendations
- Consider making the safety detection itself a separate toggle in the future, although current users only complained about the *duration* of the lockout, not the diagnostic logs.
