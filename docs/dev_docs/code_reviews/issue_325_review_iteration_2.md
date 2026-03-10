The proposed code change addresses the user's request to replace the longitudinal load effect with a longitudinal G-force effect in the FFB engine. This is a significant physical model improvement that decouples steering weight from aerodynamic downforce, solving the issue where high-speed cars would have excessively heavy steering on straights due to aero-load being misinterpreted as braking weight transfer.

### Analysis and Reasoning:

1.  **User's Goal:** The objective is to switch the longitudinal FFB scaling input from tire load telemetry to velocity-derived longitudinal acceleration (G-force) to ensure consistent steering feel regardless of aerodynamic downforce.

2.  **Evaluation of the Solution:**
    *   **Core Functionality:** The patch successfully replaces the old logic in `FFBEngine.cpp`. It uses `m_accel_z_smoothed` (which is properly handled in the coordinate system where $+Z$ is deceleration) and normalizes it by gravity. The implementation of "Conditional Clamping" is a clever touch: it keeps non-linear transformations (Cubic, etc.) stable within their expected $[-1, 1]$ range while allowing the Linear mode to capture higher-dynamic events like wall impacts (up to $5G$).
    *   **Safety & Side Effects:** The change is safe. It preserves the internal variable names (`m_long_load_effect`, etc.), which ensures that user configuration files (.INI) remain compatible without requiring a migration script. It also keeps the `update_static_load_reference` call (implicitly, as it wasn't removed) which is vital for other effects like bottoming.
    *   **Completeness:** The solution is comprehensive. It includes:
        *   Core physics logic update.
        *   UI label updates for user clarity.
        *   Tooltip documentation updates to explain the new physics.
        *   A new dedicated test suite (`test_issue_325_longitudinal_g.cpp`) verifying the three core requirements: braking boost, acceleration reduction, and aero-independence.
        *   Updates to existing tests to reflect the new model.
        *   Version increment and changelog entry.

3.  **Merge Assessment:**
    *   **Blocking:** None. The logic is sound, tests are passing, and it adheres to the project's architectural standards.
    *   **Nitpicks:** None. The developer followed the "Architect First" and "TDD" constraints perfectly, as evidenced by the detailed implementation plan and high-quality unit tests.

### Final Rating:

### Final Rating: #Correct#
