The proposed patch provides a comprehensive and robust implementation of the session and state detection system for the LMUFFB project, as requested in Issue #267.

### 1. User's Goal
The objective is to implement a robust state machine to accurately track game sessions and the player's cockpit status ("Realtime") to improve the reliability of FFB gating and telemetry logging.

### 2. Evaluation of the Solution

#### Core Functionality
- **State Machine Implementation:** The patch introduces a formal, thread-safe state machine within `GameConnector` using `std::atomic` variables. This centralizes the simulation status (Session Active, In Realtime, Session Type, Game Phase).
- **Event-Driven Transitions:** By leveraging LMU's internal event system (`SME_*` flags), the system is significantly more robust than the previous polling-only approach. It captures transitions (like entering/exiting the cockpit or session ends) instantaneously.
- **FFB Gating & Logging:** The `main.cpp` FFB loop is refactored to use these robust states. The addition of automatic log rotation on session type changes (e.g., transitioning from Practice to Qualifying while remaining on the server) is a high-value improvement for telemetry integrity.

#### Safety & Side Effects
- **Thread Safety:** The use of `std::atomic` ensures that states updated in the telemetry/FFB thread can be safely read by other potential consumers (like UI or logging threads).
- **Regressions:** The patch includes a fallback in `TryConnect` to poll the initial state, ensuring the system works correctly even if the application connects while the player is already in the cockpit.
- **Security:** No security vulnerabilities or exposed secrets were identified. The logic stays within the scope of telemetry processing.

#### Completeness
- All relevant files (`GameConnector.h/cpp`, `main.cpp`) were updated.
- A new functional test suite (`tests/test_issue_267_state_detection.cpp`) was added and integrated into the build system, providing verification for the state machine logic via mocks.
- Versioning and documentation (Changelog and Implementation Plan) are correctly updated.

### 3. Merge Assessment

#### Blocking
- None.

#### Nitpicks
- In `GameConnector::TryConnect`, the log message `"[GameConnector] Connected to LMU Shared Memory."` is duplicated on two consecutive lines. This is a trivial cosmetic issue that does not impact functionality.

### Final Rating:
### Final Rating: #Correct#
