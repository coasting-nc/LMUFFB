The proposed patch effectively addresses Issue #174, where Force Feedback (FFB) forces could become "stuck" when a user entered a menu or paused the game.

### Analysis and Reasoning:

1.  **User's Goal:** To ensure that all FFB forces (including the Soft Lock effect) are smoothly zeroed out when the game is not in a real-time state (e.g., in a menu or paused).

2.  **Evaluation of the Solution:**
    *   **Core Functionality:** The patch modifies the primary FFB thread in `src/main.cpp`. It introduces a check that explicitly sets the calculated force to `0.0` if `in_realtime` is false. Crucially, it still calls `calculate_force` before this override, which ensures that the FFB engine's internal state remains updated with the latest (albeit potentially frozen) telemetry.
    *   **Safety & Side Effects:** The placement of the `force = 0.0` override is strategically positioned *before* the safety slew rate limiter. This allows the application to leverage the existing slew logic to "relax" the wheel from its last active force down to zero at a controlled rate, avoiding a violent snap when pausing. It also correctly maintains the "Soft Lock" safety feature for real-time but non-driving states (like being in the garage box or AI driving), which was a requirement for preserving related functionality (Issue #184).
    *   **Completeness:** The solution includes the necessary logic change, a comprehensive implementation plan, a specific reproduction test case that verifies the fix, and the required version/changelog updates. The test case clearly distinguishes between "AI driving" (force expected) and "Menu" (force zeroed).

3.  **Merge Assessment:**
    *   **Blocking:** None. The logic is sound, thread-safe (within the existing FFB thread context), and follows the project's reliability standards.
    *   **Nitpicks:** None.

The patch is a high-quality, maintainable fix that solves the reported problem without introducing regressions in the stationary soft lock logic.

### Final Rating: #Correct#