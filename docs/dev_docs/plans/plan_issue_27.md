# Implementation Plan - Fix FFB Active after Crash and Negative Value Crashes (Issue #27)

## Context
Address two critical stability issues:
1. FFB remains active with strong forces when the game crashes, freezes, or exits a driving session unexpectedly.
2. The application can crash if certain settings (likely `lockup_gamma` or slope detection parameters) are set to negative values via manual config editing.

## Reference Documents
- GitHub Issue #27: https://github.com/coasting-nc/LMUFFB/issues/27
- Issue Messages: `docs/issue_27_messages.md`
- FFB Formulas: `docs/dev_docs/references/FFB_formulas.md`

## Codebase Analysis Summary
- **Telemetry Processing**: `GameConnector::CopyTelemetry` handles reading from shared memory.
- **FFB Loop**: `FFBThread` in `main.cpp` executes at 400Hz and sends forces to hardware.
- **Configuration**: `Preset::Apply` and `Config::Load` handle setting physical parameters.
- **Vibration Effects**: `FFBEngine` uses several oscillators (`pow`, `sin`) which are sensitive to parameter ranges.

## FFB Effect Impact Analysis
- **Affected Effects**: All effects (Base Torque, SoP, Textures) are impacted by the safety muting.
- **User Perspective**:
  - Improved safety: The wheel will no longer stay loaded when the game crashes.
  - Improved robustness: Setting nonsensical values in the config won't cause the app to disappear (crash).

## Proposed Changes

### 1. FFB Heartbeat / Staleness Detection
- **GameConnector.h/cpp**:
  - Added `m_lastElapsedTime` and `m_lastUpdateLocalTime` heartbeat members.
  - `CopyTelemetry` now updates these when `mElapsedTime` advances.
  - Added `IsStale(timeoutMs)` method.
- **main.cpp**:
  - `FFBThread` now calls `IsStale(100)` and mutes force if true.

### 2. Robust Parameter Validation
- **Config.h**:
  - Added `Preset::Validate()` method to centralized safety clamping.
  - Clamped `lockup_gamma`, `notch_q`, `max_torque_ref`, `optimal_slip_angle`, and 40+ other parameters.
- **Config.cpp**:
  - Updated `Load` and `ParsePresetLine` to call `Validate()`.

### 3. Versioning & Changelog
- Incremented version to `0.7.16`.
- Updated `CHANGELOG_DEV.md`.

## Test Plan (TDD)

### New Tests in `tests/test_ffb_stability.cpp`
1. **`test_negative_parameter_safety`**: Verifies `Preset::Apply` (via `Validate`) clamps dangerous values.
2. **`test_config_load_validation`**: Verifies `Config::Load` clamps values from a malformed file.
3. **`test_engine_robustness_to_static_telemetry`**: Verifies `FFBEngine` stays stable with frozen data.

## Deliverables
- [x] Modified `src/GameConnector.h` and `src/GameConnector.cpp`
- [x] Modified `src/main.cpp`
- [x] Modified `src/Config.h` and `src/Config.cpp`
- [x] Modified `VERSION` and `src/Version.h`
- [x] Modified `CHANGELOG_DEV.md`
- [x] New test file `tests/test_ffb_stability.cpp`
- [x] Implementation Plan updated with Implementation Notes.

## Verification of Reported Issues

| Issue Reported | Resolution |
| :--- | :--- |
| **FFB remains active after server disconnect** | Addressed by `GameConnector::IsStale()`. If telemetry heartbeat (`mElapsedTime`) stops for >100ms, `FFBThread` in `main.cpp` now explicitly mutes all forces. |
| **FFB continues after pausing the game** | Addressed by both `in_realtime` check and `IsStale()` watchdog. Pausing stops the physics engine, triggering the watchdog. |
| **Wheel snaps and stays locked on crash** | Addressed by the 100ms watchdog. The wheel is now released (force 0.0) almost immediately after the game process stops updating shared memory. |
| **App crashes from manual config edits (negative values)** | Addressed by `Preset::Validate()`. All physical parameters are now rigorously clamped to safe ranges during both startup loading and preset switching, preventing division-by-zero or `pow()` domain errors. |

## Implementation Notes

### Unforeseen Issues
- **Bottoming Oscillator**: Discovered that the `test_engine_robustness_to_static_telemetry` failed initially because `Suspension Bottoming` was active by default (due to 0 ride height in basic telemetry) and its phase advances even with static data. Updated the test to explicitly disable bottoming for constant force verification.

### Plan Deviations
- **Added `Preset::Validate()`**: Instead of putting clamping logic directly in `Apply`, I created a separate `Validate()` method to be used by both `Apply` and `Config::Load` (DRY principle).

### Challenges
- **Linux Testing of GameConnector**: Since `GameConnector.cpp` uses Win32 API, it cannot be compiled or tested in the Linux sandbox. Relied on code review and unit tests of the hardware-agnostic `FFBEngine` logic.

### Recommendations
- **Watchdog Timeout**: The 100ms timeout for staleness is conservative. It could be made configurable in `config.ini` in the future if users experience false positives during extreme stuttering.
