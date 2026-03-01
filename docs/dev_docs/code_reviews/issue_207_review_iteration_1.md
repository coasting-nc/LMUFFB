The proposed code change effectively addresses the core requirement of disabling "Session-Learned Dynamic Normalization" by default and providing a UI toggle to re-enable it. The implementation is logical, well-integrated into the existing configuration system, and supported by thorough unit tests.

### Analysis and Reasoning:

1.  **User's Goal:** The objective is to disable the session-learned dynamic normalization feature by default to prevent inconsistent FFB strength during a session and to provide a manual control for users who still want it.

2.  **Evaluation of the Solution:**
    *   **Core Functionality:** The patch successfully introduces a `m_dynamic_normalization_enabled` flag (defaulting to `false`). In `FFBEngine::calculate_force`, this flag correctly gates the update of the session peak torque and determines the normalization strategy. If disabled, the engine falls back to using `m_target_rim_nm` for normalization, which provides the desired manual scaling behavior.
    *   **Safety & Side Effects:** The changes are safe and localized. By explicitly checking `m_torque_source`, the patch ensures that "In-Game FFB" (source 1) remains unaffected, preserving its specific normalization logic. The use of `EPSILON_DIV` prevents potential division-by-zero errors.
    *   **Completeness:** The patch updates the engine logic, configuration parsing/saving for both global settings and presets, adds a UI toggle in the tuning window, and includes appropriate tooltips. It also updates existing tests that relied on the previous default behavior and adds a new comprehensive test suite for the toggle itself.
    *   **Maintainability:** The code follows the established patterns of the codebase. The addition of a descriptive tooltip and clear logic branches in `calculate_force` improves clarity compared to the previous ternary operator.

3.  **Merge Assessment (Blocking vs. Non-Blocking):**
    *   **Nitpick (Maintainability):** The member `m_dynamic_normalization_enabled` is added to `FFBEngine.h` without an inline initializer (e.g., `bool m_dynamic_normalization_enabled = false;`). While the `Preset` and `Config` classes handle initialization during load/apply, initializing at the declaration is safer to prevent undefined behavior in case of unconventional instantiation.
    *   **Nitpick (Redundancy):** The implementation plan mentioned an existing variable `m_auto_load_normalization_enabled`. The patch introduces `m_dynamic_normalization_enabled` instead. While the new name is clearer for the UI, having both in the codebase (as evidenced by `test_ffb_common.h`) may lead to confusion if the older one is unused or serves a redundant purpose.
    *   **Nitpick (Process/Rollout):** The patch is missing the mandatory `VERSION` increment and `CHANGELOG.md` updates explicitly requested in the mission instructions. It also lacks the required quality assurance records (`review_iteration_X.md`). While these do not impact the functional quality of the code, they are required for a complete, commit-ready "Fixer" submission.

Overall, the logic is sound and the solution is functional, but the "packaging" (versioning, documentation, and process records) is incomplete.

### Final Rating: #Mostly Correct#