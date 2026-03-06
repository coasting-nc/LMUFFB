# Implementation Plan - Robust Session and State Detection (#267)

**Issue**: #267 - Implement robust session and state detection

## Context
The goal is to implement a robust state machine within `GameConnector` to accurately track the game session and the player's "Realtime" (driving) status. This is crucial for:
1.  **Safety**: Ensuring FFB is only active when the player is physically in the car and in control.
2.  **Telemetry Integrity**: Starting and stopping telemetry logs at the correct moments to avoid capturing garbage data from menus or replays.
3.  **Reliability**: Handling session transitions (e.g., Qualifying to Race) gracefully without losing state or outputting incorrect forces during buffer wipes.

### Design Rationale
The current implementation relies heavily on polling individual flags (like `mInRealtime`) which can be unreliable during transitions when shared memory buffers are partially updated or wiped. By leveraging the event-driven flags (`SME_*`) provided by LMU's shared memory, we can implement a more robust edge-detection system. This "Hybrid" approach (Event + Polling) ensures that we don't miss transitions while still having a fallback if an event is missed (unlikely with shared memory, but good for robustness).

## Codebase Analysis Summary
### Impacted Functionalities:
1.  **GameConnector**: The core interface to LMU shared memory. It will now host the primary state machine.
2.  **Main Loop (main.cpp)**: The FFB thread and logging control logic will be refactored to use the new robust state machine instead of raw polled values.
3.  **FFBEngine**: `IsFFBAllowed` remains as a secondary check, but the primary "is driving" gate moves to `GameConnector`.

### Design Rationale:
`GameConnector` is the natural home for this logic as it already performs the `CopyTelemetry` and `CheckTransitions` tasks. Moving the state management here centralizes the "source of truth" for the game's operational state.

## FFB Effect Impact Analysis
### Technical Perspective:
- **Affected Effects**: All effects are indirectly affected by the global FFB gate.
- **Source Files**: `GameConnector.h/cpp`, `main.cpp`, `FFBEngine.cpp`.
- **Changes**: Refactoring the conditional logic in the `FFBThread`.
- **Interactions**: Improved synchronization between logging and FFB state.

### User Perspective (FFB Feel & UI):
- **Feel**: FFB will start and stop more cleanly when entering/exiting the car.
- **Safety**: Reduced risk of sudden torque spikes when jumping between menus and the cockpit.
- **Telemetry**: Logs will be more accurately delimited, making analysis in the `lmuffb_log_analyzer` more straightforward.

## Proposed Changes

### 1. Update `src/GameConnector.h`
- **Design Rationale**: Add atomic state variables to the singleton to provide a thread-safe "source of truth" for session and realtime status.
- **Member Variables**:
    - `std::atomic<bool> m_sessionActive{false};` (Track loaded)
    - `std::atomic<bool> m_inRealtime{false};` (Cockpit active)
    - `std::atomic<long> m_currentSessionType{-1};`
    - `std::atomic<unsigned char> m_currentGamePhase{255};`
- **Accessors**:
    - `bool IsSessionActive() const`
    - `bool IsInRealtime() const`
    - `long GetSessionType() const`
    - `unsigned char GetGamePhase() const`

### 2. Update `src/GameConnector.cpp`
- **Design Rationale**: Implement initialization in `TryConnect` and robust transition detection in `CheckTransitions` to keep the internal state in sync with the game.
- **Logic**:
    - **`TryConnect`**: Initialize `m_sessionActive`, `m_inRealtime`, `m_currentSessionType`, and `m_currentGamePhase` from the initial SM poll.
    - **`CheckTransitions`**:
        - Update `m_sessionActive` on `SME_START_SESSION`, `SME_END_SESSION`, `SME_UNLOAD`.
        - Update `m_inRealtime` on `SME_ENTER_REALTIME`, `SME_EXIT_REALTIME`.
        - Also ensure `SME_END_SESSION` and `SME_UNLOAD` clear `m_inRealtime`.
        - Update `m_currentSessionType` on `mSession` change.
        - Update `m_currentGamePhase` on `mGamePhase` change.

### 3. Implement Tests `tests/test_issue_267_state_detection.cpp`
- **Design Rationale**: Verify the state machine logic using mocks since the actual game environment isn't available on Linux.
- **Components**:
    - `MockSharedMemory` to simulate LMU SM behavior.
    - Test cases: `TestInitialConnectionInMenu`, `TestInitialConnectionInCockpit`, `TestTransitionCockpitEntryExit`, `TestSessionEndClearsRealtime`, `TestSessionTypeChange`.

### 4. Verify `GameConnector` with new tests
- **Design Rationale**: Ensure the core state logic is correct before integrating it into the main loop.
- **Actions**: Compile and run `./build/tests/run_combined_tests --filter Issue267`.

### 5. Refactor `src/main.cpp`
- **Design Rationale**: Transition the FFB thread and logging control to use the robust state machine.
- **Logic**:
    - Use `GameConnector::Get().IsInRealtime()` as the primary FFB gate.
    - Update the auto-logging trigger to use `IsInRealtime()` and detect session type changes for restarts.

### 6. Verify `main.cpp` changes
- **Design Rationale**: Ensure the application still builds and passes all tests after refactoring.
- **Actions**: Compile the project and run all tests.

### 7. Finalize Changes
- **Design Rationale**: Update versioning and documentation to reflect the fix.
- **Tasks**:
    - Increment `VERSION` (+1 rightmost digit).
    - Update `CHANGELOG_DEV.md`.
    - Update `src/Version.h` (regenerated by CMake, but ensure it reflects the change).

### 8. Complete pre-commit steps
- **Design Rationale**: Ensure proper testing, verification, review, and reflection are done.

## Initialization Order Analysis
- `GameConnector` is a singleton initialized on first `Get()`.
- `TryConnect` is called in `main()` and potentially periodically.
- No circular dependencies are introduced as we are only using existing LMU SM structures.

## Version Increment Rule
Version numbers in `VERSION` and `src/Version.h` will be incremented by the smallest possible increment (+1 to the rightmost number).

## Test Plan (TDD-Ready)
### Design Rationale:
Since we cannot run the game on Linux, we will rely on a comprehensive mock of the Shared Memory interface to simulate transitions and verify the state machine's correctness.

- **Mock Object**: Create a `MockSharedMemory` that allows pushing sequences of `SharedMemoryObjectOut` states and triggering `events[]`.
- **Test File**: `tests/test_issue_267_state_detection.cpp`
- **Test Cases (Baseline + 5 new tests)**:
    1.  `TestInitialConnectionInMenu`: Verify states are `false` when connecting to a clean SM.
    2.  `TestInitialConnectionInCockpit`: Verify states are `true` when connecting while `mInRealtime` already set.
    3.  `TestTransitionCockpitEntryExit`: Simulate `SME_ENTER_REALTIME` / `SME_EXIT_REALTIME` sequence.
    4.  `TestSessionEndClearsRealtime`: Verify that `SME_END_SESSION` force-clears `m_inRealtime` even if `SME_EXIT_REALTIME` was missed.
    5.  `TestSessionTypeChange`: Verify `m_currentSessionType` updates and can be detected for log restarts.

## Deliverables
- [ ] Modified `src/GameConnector.h`
- [ ] Modified `src/GameConnector.cpp`
- [ ] Modified `src/main.cpp`
- [ ] New `tests/test_issue_267_state_detection.cpp`
- [ ] Updated `VERSION` & `CHANGELOG_DEV.md`
- [ ] Updated `src/Version.h`

## Additional Questions
- **Q: Does `SME_START_SESSION` fire immediately upon loading completion, or when the player enters the garage?**
  - **A**: Based on investigation reports, it fires when the track has loaded and the simulation is active (garage).
- **Q: Are there cases where `mInRealtime` is true but `SME_ENTER_REALTIME` didn't fire?**
  - **A**: Unlikely, but the initialization logic in `TryConnect` handles the case where the app starts mid-session.
