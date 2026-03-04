The proposed code change effectively addresses the console spam issue described in Issue #238. The root cause was identified as a logic mismatch in the vehicle change detection within `FFBEngine::calculate_force`, which caused the initialization and normalization logic (and its associated log messages) to trigger every frame when certain conditions were met (e.g., a slight difference between telemetry names and scoring names).

### Analysis of the Solution:

1.  **User's Goal:** Prevent the console from being spammed with vehicle identification and FFB normalization reset messages during session transitions or vehicle changes.

2.  **Core Functionality:**
    *   **Immediate State Synchronization:** The patch introduces `m_last_handled_vehicle_name` (a `std::string`) to track the vehicle identity as known by the seeding logic. This is updated immediately inside `InitializeLoadReference`. By comparing against this variable instead of the transient `m_vehicle_name` (which is updated elsewhere from telemetry frames), the trigger condition is satisfied and cleared in the same frame, preventing a loop.
    *   **Robust String Comparison:** The patch replaces a fragile "micro-optimization" (checking characters at specific indices) with `strcmp` for updating UI/Logging context strings. This ensures that the engine correctly detects changes even when strings share prefixes or specific characters.
    *   **Warning Suppression:** By fixing the trigger logic, the patch also prevents the constant resetting of `m_warned_invalid_range`, which was causing `[WARNING] Invalid PhysicalSteeringWheelRange` to be spammed.

3.  **Safety & Side Effects:**
    *   **Regressions:** The patch includes a comprehensive regression test (`tests/test_issue_238_spam.cpp`) that explicitly reproduces the spam scenario and verifies the fix by capturing and inspecting standard output.
    *   **Thread Safety:** The changes occur within the physics/FFB calculation flow. While the engine uses a global mutex (`g_engine_mutex`), these local state updates are consistent with the existing threading model of the `FFBEngine`.
    *   **Performance:** Replacing a few character index checks with `strcmp` at physics frequency (usually 400Hz-1000Hz) is computationally negligible and significantly improves reliability.

4.  **Completeness:**
    *   The patch updates the C++ source, headers, and includes necessary version bumps and changelog entries.
    *   The implementation plan is well-documented and covers the design rationale for all changes.

5.  **Merge Assessment:**
    *   The solution is complete, functional, and well-tested. No blocking issues were identified.
    *   **Nitpick:** The `test_normalization.ini` version bump (`0.7.118`) is one version behind the `VERSION` file (`0.7.119`), which is likely a minor oversight but does not affect the functionality or safety of the code.

### Final Rating: #Correct#
