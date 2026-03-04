The proposed code change addresses the issue of force feedback (FFB) noise and "grinding" in the garage and menus by introducing filter resets on state transitions and tightening the criteria for allowing forces when the game is not in a "real-time" driving state.

### Analysis and Reasoning:

1.  **User's Goal:** The objective is to eliminate unwanted FFB vibrations and noise in the garage and main menus while ensuring that safety features like Soft Lock remain functional when the steering reaches its physical limits.

2.  **Evaluation of the Solution:**
    *   **Core Functionality:** The patch addresses the root causes of the "grinding" noise:
        *   **Filter Residuals:** By resetting all up-samplers and smoothed state variables when transitioning from "allowed" (driving) to "muted" (garage/menu), the patch ensures that high-frequency oscillations from the driving session do not persist.
        *   **Noise Rejection:** It tightens the "significant soft lock" gate in `FFBEngine::calculate_force`. Previously, any force above 0.1 Nm would trigger the `min_force` logic, which amplifies small telemetry noise into felt vibrations. The new logic requires the steering to be beyond the normalized limit (> 1.0) **and** the force to be significant (> 0.5 Nm), effectively filtering out jitter while allowing real rack-limit resistance.
        *   **Heartbeat Improvement:** Adding steering movement to the `GameConnector` heartbeat ensures the system stays responsive for Soft Lock in menus if the user is turning the wheel, but allows it to time out (and thus mute) if the wheel is static.
    *   **Safety & Side Effects:** The changes are safe and localized. Resetting filters on transition is a standard technique to prevent state pollution. The tighter Soft Lock gate is physically grounded in the steering position, making it robust against telemetry noise.
    *   **Completeness:** The logic fix is complete and includes unit tests verifying both the noise rejection and the filter reset behavior. However, the patch is missing several mandatory process-related deliverables required by the "Fixer" mission instructions.

3.  **Merge Assessment (Blocking vs. Non-Blocking):**
    *   **Blocking:**
        *   **Missing Version Update:** The mission instructions explicitly require updating the `VERSION` file (incrementing to 0.7.115 or similar). This is missing.
        *   **Missing Changelog:** The instructions require updating the changelog. This is missing.
        *   **Missing Code Review Records:** The mission mandates performing independent code reviews and saving them as `.md` files in `docs/dev_docs/code_reviews/`. These files are missing from the patch.
    *   **Nitpicks:**
        *   The new test filtering logic in `test_ffb_common.h` ANDs the name filter and the tag filter. While functional, users might expect an OR relationship between different filter types.

### Final Rating:

The patch is technically sound and correctly implements the fix with supporting tests. However, it fails to meet the explicit project management and documentation requirements (Version, Changelog, Review Records) defined in the mission constraints, which makes it not yet "commit-ready" for a production release following the established workflow.

### Final Rating: #Mostly Correct#
