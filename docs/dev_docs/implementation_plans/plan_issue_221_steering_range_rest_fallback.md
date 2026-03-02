# Implementation Plan - Issue #221: mPhysicalSteeringWheelRange not working

**Issue:** #221 - mPhysicalSteeringWheelRange not working
**Status:** Open

## 1. Context
The rFactor 2 / Le Mans Ultimate Shared Memory API often returns `0` or invalid values for `mPhysicalSteeringWheelRange`. This parameter is crucial for:
- Correctly scaling the Soft Lock force.
- Displaying the real-time steering angle and range in the GUI.

When this value is missing, the application currently falls back to a default value (e.g., 540 degrees), which might not match the actual car setup, leading to a poor user experience.

> **Design Rationale: Why this matters**
> Accurate steering range is fundamental for simulation fidelity. Soft Lock prevents the user from over-rotating their physical wheel beyond the virtual car's limits. If the range is wrong, Soft Lock might engage too early or too late, potentially causing confusion or even hardware strain if the user fights it.

## 2. Analysis
The game provides a REST API that contains the actual garage setup, including the steering lock.
- **LMU Port:** 6397
- **rF2 Port:** 5397
- **Endpoint:** `/rest/garage/getPlayerGarageData`
- **Target Field:** `"VM_STEER_LOCK"`
- **Value Format:** String like `"540 deg"` or `"900.0"`.

### Challenges:
1. **Network I/O:** Must not block the FFB thread (400Hz) or the GUI thread (60Hz).
2. **Stability:** Excessive REST API calls can crash the game.
3. **Dependencies:** The project aims for minimal dependencies. Adding a heavy HTTP or JSON library is undesirable.

> **Design Rationale: Fallback Selection**
> The REST API is the only reliable source of ground-truth car setup data when Shared Memory fails. Throttling is mandatory to ensure game stability, as the LMU REST API is known to be fragile.

## 3. Proposed Changes

### A. Infrastructure: `RestApiProvider`
Create `src/RestApiProvider.h` and `src/RestApiProvider.cpp`.
- Implement an asynchronous `RequestSteeringRange(int port)` method.
- Use `WinINet` on Windows for HTTP GET.
- Use a simple regex or manual string scanning to find `"VM_STEER_LOCK"` and extract the number.
- Implement a Linux mock to allow the project to build and pass tests in the current environment.

> **Design Rationale: WinINet and Manual Parsing**
> `WinINet` is a built-in Windows component, avoiding the need for `libcurl` or similar. Manual string parsing for a single field avoids the overhead and complexity of a full JSON library like `nlohmann/json`, keeping the binary light and reducing the attack surface.

### B. Configuration & State
- Update `Preset` struct in `Config.h` to include:
    - `bool rest_api_enabled = false;`
    - `int rest_api_port = 6397;`
- Update `FFBEngine` to include these members and a `std::atomic<float> m_rest_fallback_range_deg`.
- Update `Config.cpp` to parse/save these settings.

> **Design Rationale: Opt-in by Default**
> As requested, the feature will be disabled by default. This protects users from potential game crashes if they don't explicitly want to use the REST API fallback.

### C. Engine Integration
- In `FFBEngine::calculate_force`, detect car changes (already seeded).
- When a car change is detected, if `rest_api_enabled` is true AND `mPhysicalSteeringWheelRange` is invalid (<= 0), trigger the `RestApiProvider` background request.
- Ensure only one request is in flight at a time.
- Use `m_rest_fallback_range_deg` if Shared Memory range is invalid.

> **Design Rationale: Trigger on Car Change**
> Triggering only on car change (or manual reset) minimizes the number of REST calls to the absolute minimum required, satisfying the requirement to avoid crashing the game.

### D. GUI Update
- Update `GuiLayer_Common.cpp` to add a "REST API Fallback" section in "General FFB" or "Advanced Settings".
- Add checkbox for "Enable REST API Fallback".
- Add input for "REST API Port" (default 6397).

> **Design Rationale: User Control**
> Giving the user control over the port allows them to use it with both LMU and rF2, and provides a clear way to disable it if they suspect it's causing game instability.

## 4. Test Plan
1. **Unit Test:** `tests/test_issue_221_rest_fallback.cpp`
    - Mock REST API responses (valid JSON with `VM_STEER_LOCK`, malformed JSON, missing field).
    - Verify regex/parsing extracts the correct float value.
    - Verify `FFBEngine` uses the fallback value only when appropriate.
2. **Manual Verification:** (On Windows, if possible, otherwise rely on mocks)
    - Check that the checkbox correctly persists in `config.ini`.
    - Check that no network calls are made when disabled.

> **Design Rationale: Mock-based Testing**
> Since we are on Linux, we cannot test the actual `WinINet` calls or the real game. Mocks are essential to verify the logic of the provider and the engine's integration.

## 5. Implementation Notes
- **Parsing:** The `RestApiProvider` uses a robust manual scanning and regex-based extraction for `"VM_STEER_LOCK"`. It correctly handles strings like `"540 deg"`, `"900.0"`, and `"270 (540 deg)"` by taking the first numeric match.
- **Throttling:** Requests are strictly throttled to car changes or manual resets, and only if the Shared Memory range is invalid.
- **GUI:** The port field was intentionally omitted from the GUI per user request to keep the UI clean, as the port is typically fixed for LMU (6397).
- **Thread Safety:** The background thread uses a try-catch block and RAII-like atomic management to ensure the `m_isRequesting` flag is handled safely even in case of exceptions.
- **Code Review:** During Iteration 1, the reviewer noted missing versioning and changelog updates, which were subsequently addressed. The suggested RAII for the requesting flag was implemented using a lambda with proper flag management.

## 6. Final Status
- **Implemented:** `src/RestApiProvider.cpp`, `src/RestApiProvider.h`.
- **Modified:** `src/FFBEngine.cpp`, `src/FFBEngine.h`, `src/Config.cpp`, `src/Config.h`, `src/GuiLayer_Common.cpp`, `src/Tooltips.h`, `CMakeLists.txt`, `VERSION`, `CHANGELOG_DEV.md`.
- **Tested:** `tests/test_issue_221_rest_fallback.cpp`.
- **Verified:** 100% pass on Linux with 395/395 test cases.

---
**Fixer's Note:** Task completed autonomously. The solution provides a safe, non-blocking fallback for the steering range issue while maintaining project stability and minimal dependencies.
