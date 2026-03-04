# Implementation Plan - Issue #235: FFB still present while in garage and gets worse in main menu

## Context
User reports that FFB vibrations and "grinding" are present in the garage and worsen in the main menu as of v0.7.114. This is a potential regression or incomplete fix for Issue #185.

## Analysis
- **Issue #185 Recap**: Introduced muting in `calculate_force` when `allowed` is false, but preserved Soft Lock force (> 0.1 Nm).
- **Issue #235 Observation**: "Gets worse in main menu".
- **Hypothesis**:
    1.  **Filter Pollution**: When transitioning from driving to menu/garage, high-frequency filters (Holt-Winters, extrapolators) might retain session residuals or oscillate on noisy menu telemetry.
    2.  **Weak Soft Lock Gate**: The 0.1 Nm threshold for Soft Lock might be too low, allowing steering jitter/noise to trigger `min_force` amplification or small vibrations.
    3.  **Staleness Timeout**: If the heartbeat only uses `mElapsedTime`, the system might not refresh in menus when the game is paused but the user is moving the wheel (or vice-versa, it might stay active when it should sleep).

## Design Rationale
- **Steering Heartbeat**: Adding `mUnfilteredSteering` to the `GameConnector` heartbeat ensures that Soft Lock remains responsive in menus if the user moves the wheel, but allows the system to enter a "stale" (muted) state if everything is static.
- **Filter Reset on Transition**: Unconditionally resetting up-samplers and smoothing filters when FFB is disabled (garage/menu transition) ensures a clean state and eliminates session residuals.
- **Tightened Soft Lock Gate**: Requiring both `abs_steer > 1.0` and `force > 0.5 Nm` ensures that only intentional physical rack limits bypass the garage mute, rejecting noise.

## Proposed Changes

### `src/GameConnector.h` / `.cpp`
- Add `m_lastSteering` to track steering movement.
- Update `CopyTelemetry` to refresh update timer if steering changes > 0.0001.

### `src/FFBEngine.h` / `.cpp`
- Add `m_was_allowed` to detect transitions.
- In `calculate_force`, if `m_was_allowed && !allowed`, call `Reset()` on all up-samplers and zero out smoothed states.
- Tighten `significant_soft_lock` check.

### `src/main.cpp`
- Verify existing menu muting logic (confirmed: it slews to 0.0 when not in realtime).

## Test Plan
- Update `tests/test_issue_185_fix.cpp` with `test_issue_235_garage_noise` to simulate menu jitter.
- Verify transition resets via unit tests.
- Run full regression suite.

## Implementation Notes
- Build and test on Linux (Headless).
- Ensure no regressions in Soft Lock feel during actual driving.
