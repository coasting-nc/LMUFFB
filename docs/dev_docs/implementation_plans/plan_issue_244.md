# Implementation Plan - Improve logging of game transitions to debug log file (Issue #244)

This plan addresses the requirement to ensure all console output is captured in the debug log and to improve the monitoring of game transitions to ensure no state changes are missed, especially during menu navigation.

## Design Rationale
Reliable logging is the backbone of remote diagnostics. The current system has two gaps: some console prints bypass the log file, and the state transition logic misses certain events (especially menu-related ones and high-level engine events). By centralizing all output through the `Logger` and expanding the edge-detection logic in `GameConnector` to include all `SME_*` events and robust menu state tracking, we create a high-fidelity "black box" recorder for the application.

## Context
The application uses `Logger::Log` for most things, but many modules still use `std::cout`, `std::cerr`, or `fprintf`. Additionally, `GameConnector::CheckTransitions` was recently added but lacks coverage for many of the `SME_*` events defined in the LMU Shared Memory specification.

## Proposed Changes

### 1. Centralize Logging (`src/Logger.h`, various files)
- **Change:** Systematically replace all instances of `std::cout`, `std::cerr`, `printf`, and `fprintf` with calls to `Logger::Get().Log(...)` or `Logger::Get().LogFile(...)` where appropriate.
- **Change:** Update `Logger` to ensure it correctly handles all severity levels if needed, though the current `Log` (console+file) and `LogFile` (file only) are sufficient for the requirements.
- **Rationale:** Requirement 1 explicitly states that anything in the console must also be in the log file. Centralizing through `Logger` guarantees this.

### 2. Expand Transition Monitoring (`src/GameConnector.h/cpp`)
- **Technical Change:** Update `TransitionState` struct to include flags for all relevant `SME_*` events:
    - `SME_ENTER`, `SME_EXIT`
    - `SME_STARTUP`, `SME_SHUTDOWN`
    - `SME_LOAD`, `SME_UNLOAD`
    - `SME_START_SESSION`, `SME_END_SESSION`
    - `SME_ENTER_REALTIME`, `SME_EXIT_REALTIME`
    - `SME_INIT_APPLICATION`, `SME_UNINIT_APPLICATION`
- **Technical Change:** Enhance `CheckTransitions` to detect these events. Since these are likely transient flags in the Shared Memory, we will log them whenever they are non-zero in the current frame.
- **Technical Change:** Improve `mOptionsLocation` logging to ensure it captures all menu transitions accurately.
- **Design Rationale:** Requirement 3 and 4 specify that we must monitor all transitions defined in `lmu_sm_transitions.md`. Many of these are `SME_*` events that are not currently tracked.

### 3. Version and Changelog
- Increment version to `0.7.123`.
- Update `CHANGELOG_DEV.md` and `USER_CHANGELOG.md`.

## Test Plan

### 1. Transition Logic Test (`tests/test_transition_logging.cpp`)
- **Description:** Expand the existing test to cover the new `SME_*` events.
- **Test Cases:**
    - Mock `SharedMemoryObjectOut` with various `events[]` flags set.
    - Verify `Logger` receives the expected transition strings.
    - Verify `mOptionsLocation` transitions are correctly identified.
- **Design Rationale:** Verifies that the expanded edge detection correctly identifies and logs the new events.

### 2. Logging Consistency Test (Manual/Audit)
- **Description:** Audit the codebase to ensure no direct `std::cout`/`std::cerr` remains.
- **Rationale:** Ensures compliance with Requirement 1.

## Deliverables
- [ ] Modified `src/Logger.h`
- [ ] Modified `src/GameConnector.h`
- [ ] Modified `src/GameConnector.cpp`
- [ ] Modified various files (removing `std::cout`/`std::cerr`)
- [ ] Updated `tests/test_transition_logging.cpp`
- [ ] Updated `VERSION`
- [ ] Updated `CHANGELOG_DEV.md`
- [ ] Updated `USER_CHANGELOG.md`

## Additional Questions
- Are `SME_*` events in Shared Memory "sticky" (remain 1 until read) or "transient" (1 for only one game update)? *Assuming transient, but polling at 400Hz should catch them if the game is at 100Hz.*
- Should we capture `std::cout` from 3rd party libraries? *The requirement says "Each print that goes into the console", but we only have control over our code. We will focus on our own codebase.*

## Implementation Notes
*(To be filled during development)*
