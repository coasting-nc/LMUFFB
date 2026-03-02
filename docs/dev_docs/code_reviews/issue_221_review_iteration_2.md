The proposed patch is a high-quality, comprehensive solution that addresses Issue #221 by implementing a robust REST API fallback for steering wheel range detection. It follows all user constraints, including specific UI placement, regex requirements, and performance considerations.

### User's Goal
Implement a fallback mechanism using the game's REST API to retrieve the correct steering wheel range when Shared Memory returns invalid data (0), ensuring minimal impact on game stability and a clean UI.

### Evaluation of the Solution

#### Core Functionality
- **REST API Integration:** The patch implements a `RestApiProvider` using the built-in Windows `WinINet` library, avoiding external dependencies. It successfully queries the game's garage data and extracts the steering lock.
- **Asynchronous Execution:** The network request is performed in a dedicated background thread, ensuring that the high-frequency FFB thread is never blocked by network latency.
- **Throttling:** The fallback is triggered only upon a car change and only if the Shared Memory range is invalid, satisfying the requirement for minimal API calls.
- **Regex Compliance:** The specific regex `r"\d*\.?\d+"` is used as requested to parse the steering range from the string returned by the API.

#### Safety & Side Effects
- **Thread Safety:** The singleton `RestApiProvider` uses atomics and thread joining logic to safely manage the background request thread and shared state.
- **Stability:** By limiting calls to car change events and providing an opt-in toggle (disabled by default), the patch minimizes the risk of game crashes associated with REST API usage.
- **Linux Compatibility:** The author included `#ifdef _WIN32` guards and a mock implementation for Linux, ensuring the project continues to build and pass tests in the Ubuntu environment.

#### Completeness
- **UI Integration:** The "Steerlock from REST API" checkbox is placed exactly where requested (below steering telemetry, before In-Game FFB) and wrapped in a collapsible section.
- **Configuration:** The setting is correctly integrated into the configuration system (`Config.cpp/h`), allowing it to persist across sessions.
- **Testing:** A new test suite (`test_issue_221_rest_fallback.cpp`) verifies the JSON parsing logic and engine integration logic.
- **Documentation:** The patch includes the mandatory `VERSION` update (0.7.113), a `CHANGELOG_DEV.md` entry, and a detailed implementation plan.

### Merge Assessment (Blocking vs. Non-Blocking)
The patch is complete and follows the project's reliability standards. The use of an asynchronous provider for network I/O is a correct architectural choice for this codebase.

- **Blocking:** None.
- **Nitpicks:** The log warning for invalid range might still fire once during the brief window when the REST API request is in flight, but this is a trivial side effect that does not impact functionality.

### Final Rating: #Correct#### Final Rating: #Correct#