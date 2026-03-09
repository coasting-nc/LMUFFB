# Implementation Plan - Issue #314: Safety fixes for FFB Spikes (2)

This plan outlines additional safety mechanisms and logging to further mitigate and diagnose Force Feedback (FFB) spikes and jolts, building upon the foundations laid in Issue #303.

## Context
Users continue to report occasional FFB jolts. Investigation shows that some safety events are missed or not properly sustained when multiple triggers occur in sequence. This task aims to make safety mode more persistent, more restrictive, and more transparent through enhanced logging.

**Issue:** [Safety fixes for FFB Spikes (2) #314](https://github.com/coasting-nc/LMUFFB/issues/314)

## Design Rationale
The "Safety First" principle is extended. If a safety window is already active and another event occurs, we must assume the environment is still unstable and reset the safety timer. We also need to tighten the restrictions (lower slew rate, higher smoothing) during this window to ensure that any remaining spikes are effectively blunted. Comprehensive logging of entry, reset, and exit points is critical for correlating code behavior with user experience.

## Reference Documents
- `docs/dev_docs/implementation_plans/issue_303_safety_fixes_ffb_spikes.md`: Initial safety implementation.
- `src/FFBEngine.cpp`: Current safety logic in `calculate_force` and `ApplySafetySlew`.

## Codebase Analysis Summary
- **FFBEngine::TriggerSafetyWindow**: Currently only logs on initial entry. Does not explicitly mention timer resets in logs.
- **FFBEngine::ApplySafetySlew**: Implements the 200 units/s cap during safety mode. Detects "High Spike" but requires 5 frames of sustain.
- **FFBEngine::calculate_force**: Handles the application of gain reduction and smoothing during the safety window.

### Impacted Functionalities:
- **Safety Window Management**: Timer reset logic and exit logging.
- **Signal Mitigation**: Tighter slew rate and increased smoothing during safety mode.
- **Jolt Detection**: Improved threshold logic to catch massive single-frame spikes.
- **Logging**: Added logs for timer resets and safety mode exit.

### Design Rationale:
By centralizing the safety state management in `FFBEngine`, we ensure consistent behavior across all trigger sources (lost frames, control transitions, physics spikes).

## FFB Effect Impact Analysis
| Effect | Technical Changes | User Perspective |
| :--- | :--- | :--- |
| **All Effects** | More aggressive attenuation (lower gain, more smoothing) and tighter slew rate limits during safety windows. | A more noticeable but safer "fade-out" during game hitches or transitions. Decreased risk of violent wheel movement. |

### Design Rationale:
The increased restrictiveness is a direct response to user reports of jolts still being felt under the previous safety parameters. Safety of hardware and user hands takes precedence over FFB fidelity during known unstable periods.

## Proposed Changes

1.  **Modify `src/FFBEngine.h` to update constants.**
    - `SAFETY_SLEW_WINDOW`: Reduce from 200.0 to 100.0.
    - `SAFETY_GAIN_REDUCTION`: Reduce from 0.5 to 0.3.
    - Add `SAFETY_SMOOTHING_TAU`: Set to 0.2 (200ms).
    - Add `IMMEDIATE_SPIKE_THRESHOLD`: Set to 1500.0.
    - **Design Rationale:** Tighter limits are needed because previous limits (200 slew, 0.5 gain) were still allowing felt jolts.

2.  **Update `FFBEngine::TriggerSafetyWindow` in `src/FFBEngine.cpp`.**
    - If `m_safety.safety_timer > 0.0`, log "[Safety] Reset Safety Mode Timer (Reason: %s)".
    - Always set `m_safety.safety_timer = SAFETY_WINDOW_DURATION`.
    - **Design Rationale:** Ensures safety mode persists if multiple events happen (e.g., lost frames followed by a control transition).

3.  **Enhance `FFBEngine::ApplySafetySlew` in `src/FFBEngine.cpp`.**
    - Add an immediate trigger: if `requested_rate > IMMEDIATE_SPIKE_THRESHOLD`, call `TriggerSafetyWindow("Massive Spike")` immediately (no 5-frame wait).
    - **Design Rationale:** A single massive spike can cause a jolt before the 5-frame counter triggers.

4.  **Update `FFBEngine::calculate_force` in `src/FFBEngine.cpp`.**
    - Implement safety exit logging: when `m_safety.safety_timer` transitions from positive to zero, log "[Safety] Exited Safety Mode".
    - Use `SAFETY_SMOOTHING_TAU` for the safety smoothing EMA.
    - **Design Rationale:** Exit logging is essential for telemetry analysis.

5.  **Create unit tests in `tests/test_issue_314_safety_v2.cpp`.**
    - Test timer reset logic.
    - Test immediate massive spike detection.
    - Test safety exit logging (if possible via mock or by checking state).
    - Verify tighter limits (0.3 gain, 100 slew).

6.  **Update `VERSION` and `CHANGELOG_DEV.md`.**
    - Increment version to `0.7.158`.
    - Document changes in changelog.

## Design Rationale (Proposed Changes)
- **Timer Reset:** Crucial for handling overlapping unstable events.
- **Immediate Trigger:** Necessary for spikes that are so large they don't need "confirmation" over multiple frames.
- **Exit Logging:** Completes the lifecycle visibility of safety events.

## Test Plan (TDD-Ready)
1.  `test_safety_timer_reset`: Trigger safety, wait 1s, trigger again, verify timer is back to 2s.
2.  `test_immediate_spike_detection`: Feed a single frame with rate > 1500, verify safety mode active.
3.  `test_safety_exit_logging`: Run safety for 2s, verify it exits and (ideally) state reflects it.
4.  `test_safety_restrictiveness`: Verify gain is 0.3 and slew is 100 during safety mode.

## Deliverables
- [x] Modified `FFBEngine.h/cpp`
- [x] New test file `tests/test_issue_314_safety_v2.cpp`
- [x] Updated `VERSION` and `CHANGELOG_DEV.md`
- [x] Updated Implementation Notes

## Implementation Notes
- **Restrictiveness**: The safety parameters were tightened significantly (0.3x gain, 100 units/s slew). This was necessary because the previous 0.5x/200 level still allowed perceptible jolts during high-frequency telemetry gaps.
- **Immediate Detection**: The massive spike threshold (1500 units/s) provides a "safety net" for single-frame errors that don't satisfy the 5-frame sustain logic but are large enough to be felt.
- **Timer Persistence**: Resetting the timer on subsequent safety events ensures that complex transitions (e.g., lost frames immediately followed by AI takeover) are handled as a single continuous safety window.
- **Logging Lifecycle**: The addition of reset and exit logs provides a complete audit trail for safety window activation, which is invaluable for analyzing user-reported jolts from log files.
- **Test Alignment**: Updated existing `test_issue_303_safety.cpp` assertions to match the new 0.3x/100 restrictiveness levels. All unit tests pass.

## Code Review & Refinement (Iteration 1)
- **Smoothing Memory Bug**: Fixed a critical bug where the safety EMA smoothing would resume from a stale force value from a previous session. Introduced `safety_is_seeded` flag to ensure EMA is always seeded from the current force upon entering safety mode.
- **Thread Safety**: Added `std::recursive_mutex` protection to `TriggerSafetyWindow` to ensure safe access to the `SafetyMonitor` state from different threads (e.g. lost frame watchdog vs FFB thread).
- **Improved Test Coverage**: Added `test_safety_reentry_smoothing` to verify that the smoothing state is correctly reset upon re-entering safety mode after expiry.
