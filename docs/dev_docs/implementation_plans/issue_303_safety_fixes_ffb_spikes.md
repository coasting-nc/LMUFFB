# Implementation Plan - Issue #303: Safety fixes for FFB Spikes

This plan outlines the implementation of enhanced safety mechanisms to prevent and log Force Feedback (FFB) spikes, jolts, and unexpected behaviors in LMUFFB.

## Context
Users have reported occasional FFB spikes or violent jolts, particularly during transitions or game stutters. This task aims to implement robust detection and mitigation strategies.

**Issue:** [Safety fixes for FFB Spikes #303](https://github.com/coasting-nc/LMUFFB/issues/303)

## Design Rationale
The core philosophy is "Safety First". When a signal is untrusted (due to lost frames or state transitions), we must temporarily degrade the FFB quality (smoothing/capping) to protect the user and their hardware. Reliability is ensured by monitoring both game state (`mControl`) and signal integrity (frame timing). Logging provides the necessary transparency to diagnose the root causes of reported "jolts".

## Reference Documents
- `src/main.cpp`: FFB loop and state gating.
- `src/FFBEngine.cpp`: FFB calculation and `ApplySafetySlew`.
- `docs/dev_docs/investigations/session transition.md`: Background on state transitions.

## Codebase Analysis Summary
- **FFBThread (main.cpp)**: The primary loop where telemetry is fetched and passed to the engine. It handles high-level muting based on `mControl`.
- **FFBEngine (FFBEngine.cpp/h)**: Implements the physics and the existing `ApplySafetySlew`.
- **GameConnector (GameConnector.cpp/h)**: Provides the state machine for `mControl` and `inRealtime`.

### Impacted Functionalities:
- **State Transition Handling**: Detecting when `mControl` changes.
- **Frame Timing Monitoring**: Detecting gaps in `mElapsedTime`.
- **Safety Slew Limiting**: Dynamically adjusting limits based on safety state.
- **Logging**: Adding detailed diagnostic prints for safety events and soft lock.

### Design Rationale:
These modules form the backbone of the FFB pipeline and state detection. Modifying them allows us to intercept untrusted data at the source (main loop) and mitigate its impact during processing (engine).

## FFB Effect Impact Analysis
| Effect | Technical Changes | User Perspective |
| :--- | :--- | :--- |
| **All Effects** | Will be subject to temporary gain reduction and increased smoothing during safety windows. | Temporary "muffled" feel during transitions or stutters, preventing violent jolts. |
| **Soft Lock** | Engagement and influence will be logged. Preserved during pause/garage but suppressed if AI takes control. | Consistent safety at the steering limits with better diagnostic visibility. |

### Design Rationale:
Temporary attenuation is preferred over abrupt muting as it maintains some level of "feel" while ensuring safety. Logging the transitions helps confirm if spikes are tied to game-driven control changes.

## Proposed Changes

1.  **Update `src/FFBEngine.h` to define `SafetyMonitor` and constants.**
    - Define a `SafetyMonitor` struct to track timers and previous states (`m_last_mControl`, `m_safety_timer`, `m_spike_counter`, `m_tock_timer`).
    - Add constants for `SAFETY_WINDOW_DURATION` (2.0s), `SAFETY_GAIN_REDUCTION` (0.5x), `SAFETY_SLEW_WINDOW` (200.0), and `SPIKE_DETECTION_THRESHOLD` (500.0).
    - Add a public method `TriggerSafetyWindow(const char* reason)`.
    - **Verification:** Use `read_file` to confirm the new struct and constants are present.

2.  **Modify `src/FFBEngine.cpp` constructor to initialize `SafetyMonitor`.**
    - Ensure `m_last_mControl` is initialized to a neutral value (e.g., -2).
    - **Verification:** Use `read_file` on `src/FFBEngine.cpp` to check the constructor.

3.  **Update `FFBEngine::calculate_force` in `src/FFBEngine.cpp` to implement transition detection and safety mitigation.**
    - Detect `mControl` transitions (e.g., PLAYER <-> AI/NONE) and call `TriggerSafetyWindow`.
    - Add logging for FFB muting/unmuting when `allowed` state or `mControl` changes, showing the exact moment and reason.
    - If `m_safety_timer > 0`, apply `SAFETY_GAIN_REDUCTION` and an extra EMA smoothing pass to the final `norm_force`.
    - Decrement `m_safety_timer` by `dt`.
    - **Verification:** Use `read_file` to verify the logic integration in `calculate_force`.

4.  **Enhance `FFBEngine::ApplySafetySlew` in `src/FFBEngine.cpp`.**
    - Implement tighter slew limits (`SAFETY_SLEW_WINDOW`) when `m_safety_timer > 0`.
    - Detect high-slew spikes: if `std::abs(delta) / dt > SPIKE_DETECTION_THRESHOLD`, increment `m_spike_counter`.
    - If `m_spike_counter` exceeds a threshold (e.g., 5 frames), log a warning "[Safety] High Spike Detected: Rate=%.1f" and reset the counter.
    - **Verification:** Use `read_file` to verify the slew rate limiting and logging logic.

5.  **Modify `src/main.cpp` to detect lost frames in `FFBThread`.**
    - After `CopyTelemetry`, compare `pPlayerTelemetry->mElapsedTime` with a `last_et` variable.
    - If `(curr_et - last_et) > (data->mDeltaTime * 1.5)`, call `g_engine.TriggerSafetyWindow("Lost Frames")`.
    - **Verification:** Use `read_file` to confirm the lost frame detection is correctly placed in the FFB loop.

6.  **Implement "Full Tock" detection in `src/FFBEngine.cpp`.**
    - At the end of `calculate_force`, check if `std::abs(data->mUnfilteredSteering) > 0.95` and `std::abs(norm_force) > 0.8`.
    - If true, increment `m_tock_timer` by `dt`. If it exceeds 1.0s, log "[Safety] Full Tock Detected" (throttled to once per 5 seconds) and reset timer.
    - **Verification:** Use `read_file` to verify the full tock detection logic.

7.  **Implement Soft Lock logging in `src/SteeringUtils.cpp`.**
    - In `calculate_soft_lock`, log when the effect first engages (`abs_steer > 1.0` transition) and when it significantly influences the wheel force (e.g., `abs(ctx.soft_lock_force) > 5.0 Nm`).
    - Use flags to ensure logs are not spammed (only on transitions or significant changes).
    - **Verification:** Use `read_file` to confirm logging in `calculate_soft_lock`.

8.  **Create unit tests in `tests/test_issue_303_safety.cpp`.**
    - Write tests simulating `mControl` transitions, lost frames (using `SetLastElapsedTime`), and high-slew spikes.
    - Assert that force outputs are correctly mitigated (capped/smoothed) during safety windows.
    - **Verification:** Run `cmake --build build` to ensure the new tests compile.

9.  **Add user guide for safety feature testing and debugging.**
    - Create `docs/user_guides/Safety_Features_Testing_Guide.md`.
    - Describe expected log entries for jolts, stutters, and transitions.
    - Explain how users can verify if a perceived spike was caught by the safety logic.
    - **Verification:** Confirm the file exists and has correct content.

10. **Increment version and update changelog.**
    - Update `VERSION` to `0.7.157`.
    - Update `src/Version.h` to match.
    - Add an entry to `CHANGELOG.md` describing the safety fixes.
    - **Verification:** Use `read_file` to confirm version updates.

11. **Ensure build and run all tests.**
    - Run `cmake --build build` to ensure a clean build.
    - Run all project tests using `./build/tests/run_combined_tests` and ensure all tests (including new ones) pass.
    - **Verification:** Confirm final test output shows 0 failures.

12. **Complete pre-commit steps.**
    - Complete pre-commit steps to ensure proper testing, verification, review, and reflection are done.

13. **Submit the changes.**
    - Submit the changes with a descriptive commit message.

## Design Rationale (Proposed Changes)
- **Safety Window:** A time-based window ensures that even if telemetry stabilizes quickly, we don't immediately return to full power, allowing any residual "jitter" to settle.
- **Slew Rate Capping:** Tighter slew rates during transitions are the most effective way to prevent the "hammering" effect of digital spikes.
- **Logging:** Moving from silent mitigation to logged mitigation allows developers to correlate user complaints with specific code-triggered safety events. Throttling/flagging prevents log spam.

## Test Plan (TDD-Ready)
- **Design Rationale**: Tests must prove that the safety window is active, gain is reduced, and slew is capped when triggered by various conditions.

1.  `test_safety_mcontrol_transition`: Verify that changing `mControl` triggers a safety window with reduced gain and tighter slew.
2.  `test_safety_lost_frames`: Simulate a large jump in `mElapsedTime` and verify "Safety Mode" activation.
3.  `test_spike_detection_logging`: Verify that a synthetic spike triggers a log entry (mocked logger).
4.  `test_full_tock_detection`: Verify that holding the wheel at lock with high force triggers a log entry.

## Deliverables
- [ ] Modified `FFBEngine.h/cpp`
- [ ] Modified `main.cpp`
- [ ] Modified `SteeringUtils.cpp`
- [ ] New test file `tests/test_issue_303_safety.cpp`
- [ ] Updated `VERSION` and `CHANGELOG.md`
- [ ] Updated Implementation Notes (Unforeseen Issues, Deviations, etc.)

## Implementation Notes
- **Transition Detection**: The signature of `FFBEngine::calculate_force` was updated to include `signed char mControl`. This allows the engine to detect control handovers within the high-frequency physics pipeline and trigger the safety window immediately.
- **Safety Mode Persistence**: A `safety_smoothed_force` state variable was added to `SafetyMonitor` to ensure that the extra smoothing pass during the safety window is continuous and doesn't "snap" to zero or the target force on activation.
- **Lost Frame Sensitivity**: The threshold for lost frame detection was set to `1.5x` the expected delta-time. This ensures that minor jitter doesn't trigger safety mode, but actual game hitches (lost frames) are caught.
- **Throttled Logging**: Full Tock and Soft Lock influence logging use timers and flags to ensure that the debug log remains readable and isn't flooded with identical entries during sustained events.
- **Test Integrity**: Updated `tests/test_ffb_common.h` and existing coverage tests (`test_coverage_boost_v2.cpp`) to handle the new safety state, ensuring that residual timers from one test don't affect the assertions of another.
