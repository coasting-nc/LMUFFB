# Implementation Plan - Improvements related to new session state detection (#269)

**Issue**: #269 - Improvements related to new session state detection implemented in v0.7.136

## Context
Following the implementation of a robust state machine in `GameConnector` (Issue #267), we now need to leverage this state to improve telemetry logging, GUI feedback, and FFB gating. Specifically, we want to ensure FFB is active only when appropriate while maintaining the soft-lock effect, and provide the user with clear information about the detected game state.

### Design Rationale
The robust state machine provides reliable `m_sessionActive` and `m_inRealtime` flags. We will extend this to include the player's control state (`m_playerControl`). This allows for more precise FFB gating and better-delimited telemetry logs. The GUI update will improve transparency, helping users understand why FFB might be active or muted. By allowing `FFBEngine::calculate_force` to handle the `allowed` flag internally, we can preserve the Soft Lock effect even when general FFB is muted (e.g., in menus or under AI control).

## Codebase Analysis Summary
### Impacted Functionalities:
1.  **GameConnector**: Central state repository. Now tracks `m_playerControl` to distinguish between human and AI control.
2.  **GuiLayer_Common.cpp**: UI feedback. Displays new state information in the "System Health" header of the Debug window.
3.  **main.cpp (FFBThread)**: Execution logic. Uses new states for logging triggers and FFB gating.
4.  **FFBEngine**: Physics engine. Its `calculate_force` method already supports gated FFB while preserving the Soft Lock effect if `allowed=false` is passed.

### Design Rationale
`GameConnector` is the "source of truth" for game state. GUI and `FFBThread` are consumers. `FFBEngine` is the executor. This separation of concerns is maintained while improving the coordination between them.

## FFB Effect Impact Analysis
### Technical Perspective:
- **Affected Effects**: All effects are muted when not driving or not in control. Soft Lock is preserved when not driving or not in control.
- **Source Files**: `FFBEngine.cpp`, `main.cpp`.
- **Changes**: Logic in `main.cpp` no longer zero-sum outputs when not in realtime, but passes the appropriate `allowed` flag to `calculate_force`.

### User Perspective (FFB Feel & UI):
- **Feel**: User will still feel the physical limits of the wheel (Soft Lock) when in menus or under AI control, which improves safety and realism.
- **UI**: Clear feedback on why FFB is on/off via the "System Health" section.

## Proposed Changes

### 1. Update `src/GameConnector.h` and `src/GameConnector.cpp`
- **Design Rationale**: Track the player's control state to distinguish human vs AI control.
- **Changes**:
    - Add `m_playerControl` atomic variable to `GameConnector`.
    - Add `GetPlayerControl()` accessor.
    - Initialize `m_playerControl` in `TryConnect`.
    - Update `m_playerControl` in `CheckTransitions`.

### 2. Update `src/GuiLayer_Common.cpp`
- **Design Rationale**: Provide transparency to the user about the detected session and player state.
- **Changes**:
    - Display "Sim Status", "Session Type", "Player State", and "Control" in the "System Health" header of the Debug window.
    - Use human-readable labels:
        - Session: 0="Test Day", 1-4="Practice", 5-8="Qualifying", 9="Warmup", 10-13="Race".
        - Control: -1="Nobody", 0="Player", 1="AI", 2="Remote", 3="Replay".
    - Repurpose "START LOGGING" / "STOP LOG" buttons to toggle `Config::m_auto_start_logging` to ensure consistent lifecycle management (files created only when driving).

### 3. Refine `src/main.cpp` (FFBThread)
- **Design Rationale**: Precise control over FFB gating and logging start/stop triggers.
- **Changes**:
    - Pass `in_realtime_phys && (playerControl == 0)` as the `full_allowed` flag to `g_engine.calculate_force`.
    - Remove the unconditional zeroing of `force_physics` when `!in_realtime_phys`.
    - Update `should_start_log` to trigger on `is_driving && !was_driving`.
    - Update `should_stop_log` to trigger on `(!is_driving && was_driving) || (!is_session_active && was_driving)`.
    - Ensure logging can be manually toggled via `Config::m_auto_start_logging` even if driving has already started.

### 4. Refine `src/AsyncLogger.h`
- **Design Rationale**: Reset file size counter for each new log file.
- **Changes**:
    - Reset `m_file_size_bytes` to 0 in `AsyncLogger::Start`.

### 5. Version Increment Rule
- Version numbers in `VERSION` and `src/Version.h` will be incremented by the smallest possible increment (+1 to the rightmost number): `0.7.136` -> `0.7.137`.

## Test Plan (TDD-Ready)
### Design Rationale:
Verify the state transitions and FFB gating logic using mocks since the actual game environment isn't available on Linux.

- **Test File**: `tests/test_issue_269_state_detection.cpp`
- **Test Cases**:
    1. `test_issue_269_control_state_detection`: Verify `m_playerControl` updates correctly from Shared Memory.
    2. `test_issue_269_soft_lock_preservation`: Verify that `FFBEngine::calculate_force` returns non-zero torque (Soft Lock) when `allowed=false` and steering is beyond the limit, while physics torque is zeroed.

## Deliverables
- [x] Modified `src/GameConnector.h`
- [x] Modified `src/GameConnector.cpp`
- [x] Modified `src/GuiLayer_Common.cpp`
- [x] Modified `src/main.cpp`
- [x] New `tests/test_issue_269_state_detection.cpp`
- [x] Updated `VERSION` & `CHANGELOG_DEV.md`
- [x] Implementation Notes (to be updated in this plan)

## Implementation Notes

### Unforeseen Issues
- Discovered that several existing unit tests (`test_transition_logging.cpp`, `test_issue_267_state_detection.cpp`) implemented their own `GameConnectorTestAccessor` which conflicted with the one added to `test_ffb_common.h`. Resolved by removing local definitions and unifying the accessor.
- Experienced compilation errors due to `GameConnector` being an incomplete type in some headers. Fixed by moving include orders and using appropriate namespace/class forward declarations.

### Plan Deviations
- None significant. The core logic of using `allowed=false` to preserve soft-lock while muting physics torque worked as intended without modifications to `FFBEngine`.

### Challenges
- Ensuring all unit tests (over 400) still passed after refactoring the common test headers was the most time-consuming part.

### Recommendations
- The state machine in `GameConnector` is now quite robust. Future improvements could involve adding more SME events (like pit requests) to the GUI for even better transparency.

### Metadata Population and Regression Analysis
During code review, it was identified that moving the logging start logic to the `FFBThread` required re-populating `SessionInfo` metadata (vehicle class, brand, and track name) to avoid regressions where logs were created with incomplete header information.

- **Fix**: The `main.cpp` FFB loop now correctly initializes `SessionInfo` by performing vehicle class and brand parsing immediately before starting the `AsyncLogger`.
- **Verification**: Verified that `.bin` filenames generated in `main.cpp` correctly include the brand and class (e.g., `_Ferrari_Hypercar_`), matching the behavior of manual GUI-triggered logging. The `test_issue_257_log_filename_format` confirms that the filename format remains consistent.
- **Review 2/3 Alignment**: Review 2 identified the metadata omission as a "nitpick" regression. Review 3 confirmed that the re-population logic in `main.cpp` correctly addressed this, ensuring 100% metadata parity between manual and automated logging paths.
