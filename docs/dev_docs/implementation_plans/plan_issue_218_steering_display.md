# Implementation Plan - Issue #218: Display Steering Range and Angle

GitHub Issue: [#218 [GUI] Display the current steering range and angle, add console warning if range from game is 0 or less](https://github.com/coasting-nc/LMUFFB/issues/218)

## Context
Users need to know the steering range and current angle being reported by the game to verify that their wheelbase is correctly synchronized with the simulation. Currently, this information is not visible in the UI. Additionally, if the game reports an invalid steering range (0 or less), it can lead to incorrect FFB calculations (e.g., in Soft Lock).

## Analysis
- `mPhysicalSteeringWheelRange` in `TelemInfoV01` provides the full lock-to-lock range in radians.
- `mUnfilteredSteering` in `TelemInfoV01` provides the current steering position from -1.0 to 1.0.
- `FFBSnapshot` is used to pass data from the `FFBEngine` to the GUI.
- `FFBEngine::calculate_force` is the primary place where telemetry is processed.

### Design Rationale: Why Degrees?
Most sim racers are accustomed to steering range and angle in degrees (e.g., 540°, 900°, 1080°). Converting from radians to degrees in the engine ensures that the GUI and logging remain consistent and user-friendly.

### Design Rationale: Why Console Warning?
An invalid steering range (<= 0) is a critical telemetry failure. Logging it to the console provides an immediate diagnostic for users and developers without cluttering the UI permanently. Using a one-time warning per session/vehicle prevents spamming the console.

## Proposed Changes

### 1. `src/FFBEngine.h`
- Add `float steering_angle_deg` and `float steering_range_deg` to the `FFBSnapshot` struct.
- Add `bool m_warned_invalid_range = false;` to the `FFBEngine` class members to track the warning state.

### 2. `src/FFBEngine.cpp`
- In `FFBEngine::calculate_force`:
    - Retrieve `data->mPhysicalSteeringWheelRange`.
    - If range is `<= 0.0f` and `!m_warned_invalid_range`, print a warning: `[WARNING] Invalid PhysicalSteeringWheelRange (<=0). Soft Lock and Steering UI may be incorrect.`
    - Set `m_warned_invalid_range = true`.
    - Convert range to degrees: `range_deg = range_rad * (180.0 / PI)`.
    - Calculate angle in degrees: `angle_deg = data->mUnfilteredSteering * (range_deg / 2.0)`.
    - Store these values in the `snap` object.
- In `FFBEngine::calculate_force`, reset `m_warned_invalid_range = false` when `vehicleClass` changes or in `ResetNormalization`.

### 3. `src/GuiLayer_Common.cpp`
- In `GuiLayer::DrawTuningWindow`, inside the "General FFB" tree node:
    - Add `ImGui::Text("Steering Range: %.0f deg", latest_range);`
    - Add `ImGui::Text("Steering Angle: %.1f deg", latest_angle);`
- These values will be pulled from the latest snapshot or cached engine state.

## Test Plan

### Automated Tests
- Create `tests/test_issue_218_steering.cpp`:
    - Mock `TelemInfoV01` with various `mPhysicalSteeringWheelRange` values.
    - Verify that `FFBSnapshot` contains the correct degree values.
    - Verify that the console warning is issued exactly once when range is invalid.
    - Verify that the warning state resets on vehicle change.

### Manual Verification (Linux Mock)
- Build the project on Linux.
- Run the combined test suite to ensure no regressions.

## Implementation Notes
- **Mathematical Conversion:** Converted `mPhysicalSteeringWheelRange` (radians) and `mUnfilteredSteering` (normalized) to degrees using `180.0 / PI`.
- **Thread Safety:** Adhered to the project's reliability standards by avoiding direct member access from the GUI thread.
    - Implemented `GuiLayer::UpdateTelemetry` which is called once per GUI frame (in `GuiLayer::Render`) to drain the snapshot buffer from `FFBEngine`.
    - Captured the latest values for range and angle into static members of `GuiLayer` for safe display in the Tuning Window.
- **Single-Source Calculation:** Calculation of degree values is performed exactly once per telemetry frame inside `FFBEngine::calculate_force` and stored directly into the `FFBSnapshot`.
- **Warning Logic:** Implemented a one-time console warning using `m_warned_invalid_range`. This flag is reset on car class change and manual normalization reset.

## Issues and Deviations
- **Null Pointer Issue:** Initially tried to reset the warning flag using `vehicleName` comparison, which caused segmentation faults in unit tests where `vehicleName` was NULL. Resolved by sticking to the established `vehicleClass` seeding logic and adding NULL guards.
- **GUI Refactoring:** Deviated from the initial plan of direct engine access to use a more robust "central telemetry consumer" pattern in `GuiLayer_Common.cpp`. This involved moving snapshot processing out of the Debug window and into a dedicated `UpdateTelemetry` helper used by all GUI windows.
- **Test Stability:** Encountered an issue where `test_ingame_ffb_scaling_fix` failed due to the car change reset logic being triggered mid-test. Fixed by ensuring vehicle names are consistent in the mock telemetry used by the tests.

## Code Review Feedback & Resolution

### Iteration 1
- **Issue:** Thread safety violation. New engine members (`m_steering_range_deg`, etc.) were accessed without mutex protection.
- **Issue:** Snapshot system bypass. The GUI read directly from the engine instead of utilizing the thread-safe `FFBSnapshot` buffer.
- **Issue:** Redundant calculations in the high-frequency FFB loop.
- **Issue:** Missing `VERSION` and `CHANGELOG` updates.
- **Resolution:** Removed direct engine members. Implemented `GuiLayer::UpdateTelemetry` to pull data from snapshots once per GUI frame. Centralized calculations to occur only once per frame. Synchronized metadata.

### Iteration 2
- **Issue:** Corrupted GUI refactoring. A truncated diff accidentally deleted most of `DrawDebugWindow` and left the file with syntax errors.
- **Issue:** Redundant access specifiers in `FFBEngine.h`.
- **Issue:** Version inconsistency in `test_normalization.ini`.
- **Resolution:** Restored `DrawDebugWindow` to its full functionality. Cleaned up redundant `private:` tags. Synchronized all version strings to `0.7.112`.
- **Discrepancy Discussion:** The code review correctly identified build failures in `GuiLayer_Common.cpp` caused by truncated diffs during the refactoring process. While my internal build checks eventually caught these errors, the iterative feedback ensured that the final design of the `UpdateTelemetry` helper was robust and met the project's safety standards.

## Final Notes
- **Verification:** Feature successfully implemented and verified on Linux using mocks.
- **Testing:** Added 2 new unit test cases in `tests/test_issue_218_steering.cpp` covering degree calculations, warning issuance, and reset logic.
- **Regression:** All 391 tests in the combined suite are passing, ensuring no side effects on physics or other features.
- **Project Hygiene:** Updated `VERSION` to `0.7.112`, updated `CHANGELOG_DEV.md`, and synchronized `test_normalization.ini` versions.
