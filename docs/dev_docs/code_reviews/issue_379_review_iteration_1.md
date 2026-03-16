### Code Review - Issue #379: Clear historical data across context changes

The proposed code change addresses the historical data carry-over issues described in Issue #379 by implementing four specific fixes across the physics and FFB engines.

#### **Analysis and Reasoning:**

1.  **User's Goal:**
    The objective is to ensure that the physics engine starts with a clean state whenever the context changes (e.g., switching cars or teleporting), preventing discontinuous derivatives and Force Feedback (FFB) jolts.

2.  **Evaluation of the Solution:**
    *   **Core Functionality:** The patch correctly implements all four requested fixes:
        *   **Bug 1 (Slope Detection):** Resets circular buffers and smoothing variables in `InitializeLoadReference`.
        *   **Bug 2 (REST API):** Clears the steering range fallback in `RestApiProvider` during car changes to prevent using outdated data.
        *   **Bug 3 (Teleport Spikes):** Introduces a `m_derivatives_seeded` flag in `FFBEngine` to detect transitions from unallowed to allowed states (e.g., Garage to Track) and seeds the previous frame's data with current values, effectively zeroing out the initial delta.
        *   **Bug 4 (Normalization):** Fixes incomplete resets in `ResetNormalization()` by including `m_last_raw_torque` and `m_static_rear_load`.
    *   **Safety & Side Effects:** The logic is safe. Derivative seeding is a standard technique to prevent spikes in stateful systems. Mutexes are used correctly in reset functions. No regressions are expected.
    *   **Completeness:** The patch includes the necessary code fixes, a detailed implementation plan, and a verbatim copy of the GitHub issue. It also includes a comprehensive regression test (`tests/test_issue_379.cpp`) that simulates car changes and teleportation.
    *   **Maintainability:** The use of a boolean flag for seeding is clear and follows existing patterns in the codebase (e.g., `m_local_vel_seeded`).

3.  **Merge Assessment (Blocking vs. Non-Blocking):**
    *   **Nitpicks:**
        *   **Missing Metadata:** The patch fails to update the `VERSION` file and `CHANGELOG_DEV.md`, which were explicitly listed as mandatory deliverables in the instructions.
        *   **Test Code Quality:** The new test file contains residual debug `printf` logic and conditional checks that should have been removed or simplified once the test was verified.
        *   **Incomplete Wheel Seeding:** While seeding indices 0 and 1 for `m_prev_susp_force` covers front wheels (used for steering rack effects), it is inconsistent with other variables (like tire deflection) that seed all four wheels. However, this is likely sufficient for the current FFB implementation.

The patch is technically very sound and resolves the core logic issues, but it is not 100% commit-ready due to the missing version/changelog updates and slightly unpolished test code.

#### Final Rating: #Mostly Correct#
