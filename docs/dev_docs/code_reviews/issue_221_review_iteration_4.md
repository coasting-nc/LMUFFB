The proposed code change is a well-engineered and robust solution to Issue #221, addressing the unreliable steering wheel range reporting in the LMU/rF2 Shared Memory API. The implementation follows all user constraints, including specific UI formatting, call throttling for game stability, and threading safety.

### User's Goal
Implement a reliable fallback for the steering wheel range using the game's REST API, with strict performance constraints and specific UI placement.

### Evaluation of the Solution

#### Core Functionality
- **REST API Fallback:** The patch implements a `RestApiProvider` using the native Windows `WinINet` library, avoiding unnecessary external dependencies. It successfully queries the game's garage data and extracts the `VM_STEER_LOCK` value.
- **Throttling:** Asynchronous network calls are triggered exclusively upon car changes and only if the Shared Memory range is invalid (0). This minimizes network overhead and prevents game instability.
- **Regex Parsing:** The specific regex `r"\d*\.?\d+"` is used to parse the steering lock string, ensuring compatibility with various formats returned by the game (e.g., "540 deg", "900.0").
- **Engine Integration:** The retrieved fallback value is correctly applied to the FFB physics (gyro damping) and the real-time telemetry display.

#### Safety & Side Effects
- **Thread Safety:** The `RestApiProvider` uses a dedicated background thread for network I/O, ensuring that the high-frequency FFB thread (400Hz) is never blocked by network latency. Atomics and mutexes are used correctly to manage shared state.
- **Resource Management:** `WinINet` handles are properly closed, and the background thread is joined during singleton destruction.
- **Cross-Platform Compatibility:** The code includes proper `#ifdef _WIN32` guards and provides mocks for Linux, ensuring the project remains buildable and testable in the provided Ubuntu environment.

#### Completeness
- **UI Integration:** The "Steerlock from REST API" checkbox is placed exactly as requested: below the steering telemetry and above the "In-Game FFB" toggle, without being wrapped in an unnecessary collapsible section.
- **UI Formatting:** The steering telemetry display was updated to the requested format: `"Steering: <angle>° (<range>)"`.
- **Configuration:** The new settings are correctly integrated into the configuration system (`Config.cpp/h`), allowing them to persist in `config.ini`.
- **Testing:** A new unit test suite (`tests/test_issue_221_rest_fallback.cpp`) verifies the parsing logic and engine integration.
- **Process Compliance:** The patch includes the mandatory `VERSION` update (0.7.113), a `CHANGELOG_DEV.md` entry, and the required implementation plan and code review records.

### Merge Assessment (Blocking vs. Non-Blocking)
- **Blocking:** None. The solution is technically sound and meets all functional and safety requirements.
- **Nitpicks:** There are some minor stale descriptions in the "Implementation Plan" (e.g., the plan's "Proposed Changes" section mentions a port field that was later removed from the GUI per instructions, though the "Deviations" section correctly notes this removal). These are documentation-only inconsistencies that do not affect the production code.

### Final Rating: #Correct#