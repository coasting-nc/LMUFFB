# Implementation Plan - Issue #274: Fix issues in session and player state detection

## Context
Issue #274 identifies that the application incorrectly displays Sim: "Main Menu" even when disconnected from LMU. Furthermore, session detection (Track Loaded vs Main Menu) and driving state detection (In Menu vs Driving) can be unreliable if certain Shared Memory Events (SME) are missed.

## Design Rationale
The core philosophy is to move from an event-only state machine to a hybrid model that uses both events and state polling.
1. **Connection State:** The UI should explicitly distinguish between "Disconnected" and "Connected (Main Menu)". We will use `GameConnector::IsConnected()` to drive this.
2. **Session State:** While `SME_START_SESSION` and `SME_END_SESSION` are useful for logging transitions, the "Ground Truth" for whether a session is active is whether `mTrackName` in the scoring buffer is non-empty. We will use this as a robust fallback.
3. **Realtime State:** `mInRealtime` in the scoring buffer is the definitive source for whether the user is in the cockpit or at the monitor. We will ensure this is always synchronized during the transition check, even if `SME_ENTER_REALTIME` was somehow missed.

## Reference Documents
- GitHub Issue: [Fix issues in session and player state detection #274](https://github.com/coasting-nc/LMUFFB/issues/274)
- Investigation Report: `docs/dev_docs/investigations/session transition.md`
- Existing state machine logic: `src/GameConnector.cpp`, `src/HealthMonitor.h`, `src/GuiLayer_Common.cpp`.

## Codebase Analysis Summary
### Impacted Functionalities
1. **`HealthMonitor.h` (`HealthStatus` struct):**
   - Current: Tracks `session_active`, `session_type`, `is_realtime`, `player_control`.
   - Change: Add `is_connected`.
   - Rationale: The GUI relies on `HealthStatus` for its display logic. Including connection status allows for a unified "Health" check.

2. **`GuiLayer_Common.cpp` (`DrawDebugWindow`):**
   - Current: Displays "Sim: Track Loaded" if `hs.session_active` is true, otherwise "Main Menu".
   - Change: First check `hs.is_connected`. If false, display "Disconnected". If true, then check `hs.session_active`.
   - Rationale: Provides accurate feedback to the user about why data might not be appearing.

3. **`GameConnector.cpp` (`CheckTransitions`):**
   - Current: Updates `m_sessionActive` and `m_inRealtime` primarily based on `generic.events`.
   - Change: Add polling-based synchronization. If `scoring.mTrackName[0] != '\0'`, `m_sessionActive` must be true. If `scoring.mInRealtime != 0`, `m_inRealtime` must be true.
   - Rationale: Events in Shared Memory can sometimes be missed or arrive out of order during rapid transitions. Polling the state fields ensures the internal state eventually converges to the truth.

## Proposed Changes

### 1. `src/HealthMonitor.h`
- Add `bool is_connected = false;` to `HealthStatus`.
- Update `HealthMonitor::Check` signature: `static HealthStatus Check(..., bool isConnected = false, ...)`
- Assign `status.is_connected = isConnected;`

### 2. `src/main.cpp`
- In `FFBThread`, update the call to `HealthMonitor::Check` to pass `GameConnector::Get().IsConnected()`.

### 3. `src/GuiLayer_Common.cpp`
- In `DrawDebugWindow`, update the "System Health" section:
  ```cpp
  if (!hs.is_connected) {
      ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Sim: Disconnected from LMU");
  } else {
      bool active = hs.session_active;
      ImGui::TextColored(active ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
          "Sim: %s", active ? "Track Loaded" : "Main Menu");
  }
  ```

### 4. `src/GameConnector.cpp`
- In `CheckTransitions(const SharedMemoryObjectOut& current)`:
  - Add robust polling:
    ```cpp
    // Robust Fallback: Polling State
    if (scoring.mTrackName[0] != '\0') {
        m_sessionActive = true;
    } else {
        // Only set to false if we didn't just get a START_SESSION event in this same frame
        // (though usually mTrackName is populated when the event is fired)
        m_sessionActive = false;
    }
    m_inRealtime = (scoring.mInRealtime != 0);
    ```
  - Keep the event-based logging as it is useful for the debug log, but let the polled state take precedence for the atomic flags.

### 5. `VERSION` and `CHANGELOG_DEV.md`
- Increment version: `0.7.138` -> `0.7.139` (or similar depending on current).
- Document: "Fixed session and player state detection reliability (Issue #274)".

## Test Plan

### Design Rationale
We need to verify that the state machine correctly handles transitions even when events are missing, and that the connection status is correctly reported.

### Test Cases
1. **`test_issue_274_robust_session_fallback`**:
   - Initialize `GameConnector` with `m_sessionActive = false`.
   - Provide a `SharedMemoryObjectOut` with NO events but a non-empty `mTrackName`.
   - Call `CheckTransitions`.
   - Assert `gc.IsSessionActive()` is true.

2. **`test_issue_274_robust_realtime_fallback`**:
   - Initialize `GameConnector` with `m_inRealtime = false`.
   - Provide a `SharedMemoryObjectOut` with NO events but `mInRealtime = 1`.
   - Call `CheckTransitions`.
   - Assert `gc.IsInRealtime()` is true.

3. **`test_issue_274_disconnected_status`**:
   - Mock a disconnected state (if possible via the test accessor).
   - Verify `HealthMonitor::Check` reflects `is_connected = false`.

### Execution
Build: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build`
Run: `./build/tests/run_combined_tests --filter=state_machine`

## Deliverables
- [ ] Modified `src/HealthMonitor.h`
- [ ] Modified `src/GuiLayer_Common.cpp`
- [ ] Modified `src/GameConnector.cpp`
- [ ] Modified `src/main.cpp`
- [ ] Updated `VERSION`
- [ ] Updated `CHANGELOG_DEV.md`
- [ ] New/Updated tests in `tests/test_issue_274_state_reliability.cpp` (or added to `test_issue_267_state_detection.cpp`)

## Implementation Notes
- **Unforeseen Issues:**
    - Encountered a UBSan error in CI due to uninitialized `bool` members in the `SessionInfo` struct used in coverage tests. Fixed by value-initializing the struct.
    - `test_game_connector_expansion` failed in CI because the simulated lock failure was incomplete without explicitly setting the `WaitResult` to a timeout code.
- **Plan Deviations:**
    - Updated `test_health_monitor.cpp` to verify the new `is_connected` field.
    - Updated `test_coverage_boost_v6.cpp` and `test_coverage_expansion.cpp` to address the CI failures discovered during the process.
- **Challenges:**
    - Simulating rapid state transitions in unit tests required careful coordination of the shared memory mock and the `GameConnector` state accessors.
- **Recommendations:**
    - Future state machine enhancements should continue to prioritize "Ground Truth" polling over transient events to ensure long-term synchronization reliability.
