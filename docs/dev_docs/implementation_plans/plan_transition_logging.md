# Implementation Plan - Log Game Transitions to Debug Log (Issue #245)

This plan addresses the requirement to log game state transitions (entering driving, pausing, AI control, etc.) to the debug log file for better diagnostics, without cluttering the console output.

## Design Rationale
Transition periods in sim racing (loading, session changes, pausing) are common sources of Force Feedback bugs and "wheel-snapping" jolts. By maintaining a discrete, timestamped trace of these transitions in the `lmuffb_debug.log`, we can correlate FFB anomalies with specific game events. Using a file-only logging mechanism prevents performance-degrading console spam while ensuring critical diagnostic data is preserved for post-crash analysis.

## Context
The application currently logs certain events to both the file and console. High-frequency or menu-heavy transitions can be noisy for the user but are invaluable for developers. We need a way to capture these transitions silently.

## Proposed Changes

### 1. Logger Enhancement (`src/Logger.h`)
- **Change:** Add `LogFile(const char* fmt, ...)` and `_LogFileNoLock(const std::string& message)` methods.
- **Rationale:** Decoupling file logging from console printing allows us to record background state changes without visual noise.
- **Implementation:** Similar to `Log`, but `_LogFileNoLock` will omit `std::cout`.

### 2. Transition Tracking Logic (`src/GameConnector.h/cpp`)
- **Technical Change:** Add a `struct TransitionState` to `GameConnector` to store previous values of:
    - `mOptionsLocation` (ApplicationStateV01)
    - `mInRealtime` (ScoringInfoV01)
    - `mGamePhase` (ScoringInfoV01)
    - `mSession` (ScoringInfoV01)
    - `mControl` (VehicleScoringInfoV01)
    - `mPitState` (VehicleScoringInfoV01)
- **Technical Change:** Implement `CheckTransitions(const SharedMemoryObjectOut& current)` in `GameConnector`.
- **Design Rationale:** `GameConnector::CopyTelemetry` is the central point where data is ingested. Checking transitions here ensures we don't miss any state changes, even if they happen between physics frames (though unlikely given the rates).
- **Update Frequency Analysis:**
    - **Safe to Log:** Changes in the variables listed above.
    - **Do NOT Log:** `mElapsedTime`, `mDeltaTime`, `mCurrentET`, or any per-frame telemetry like `mSteeringShaftTorque`.

### 3. Integration Point (`src/GameConnector.cpp`)
- **Change:** Call `CheckTransitions` inside `CopyTelemetry` after successfully locking and copying data.
- **Rationale:** Ensures we only check for transitions when we have a valid, fresh copy of the shared memory.

### 4. Version and Changelog
- Increment version to `0.7.121`.
- Update `CHANGELOG_DEV.md` and `USER_CHANGELOG.md`.

## Test Plan

### 1. Transition Logic Test (`tests/test_transition_logging.cpp`)
- **Description:** Verifies that changing each tracked variable triggers exactly one log entry in the file and zero in the console.
- **Test Cases:**
    - Initialize with default state.
    - Change `mOptionsLocation` from 2 to 3. Verify log: `[Transition] OptionsLocation: 2 -> 3`.
    - Change `mGamePhase` to 9. Verify log: `[Transition] GamePhase: 5 -> 9 (Paused)`.
    - Change `mControl` from 0 to 1. Verify log: `[Transition] Control: 0 -> 1 (AI)`.
    - Verify that multiple calls with the same state do NOT produce duplicate logs.
- **Design Rationale:** Proves that the "Edge Detection" logic works correctly and handles mapping of magic numbers to readable strings where appropriate.

### 2. Regression Testing
- Run `./build/tests/run_combined_tests` to ensure no impact on existing FFB or telemetry logic.

## Deliverables
- [ ] Modified `src/Logger.h`
- [ ] Modified `src/GameConnector.h`
- [ ] Modified `src/GameConnector.cpp`
- [ ] New `tests/test_transition_logging.cpp`
- [ ] Updated `VERSION`
- [ ] Updated `CHANGELOG_DEV.md`
- [ ] Updated `USER_CHANGELOG.md`
- [ ] Implementation Notes in this plan.
