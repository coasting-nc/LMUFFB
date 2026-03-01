The patch provides a comprehensive and high-quality solution to Issue #207, disabling session-learned dynamic normalization by default and introducing granular UI controls for users.

### Analysis and Reasoning:

1.  **User's Goal:** The objective is to disable "Session-Learned Dynamic Normalization" by default for structural forces (to ensure FFB consistency) and provide a way to toggle it back on, while also handling tactile haptics similarly.

2.  **Evaluation of the Solution:**
    *   **Core Functionality:** The patch correctly implements two independent toggles: `m_dynamic_normalization_enabled` for structural forces and `m_auto_load_normalization_enabled` for tactile haptics. Both default to `false`. The logic in `FFBEngine::calculate_force` is updated to fallback to manual targets (`m_target_rim_nm` or class-based seeds) when these toggles are off.
    *   **Safety & Side Effects:** The solution includes a robust `ResetNormalization()` method that restores baseline state immediately when toggles are disabled or cars are changed. This prevents "learned" values from one car/session from affecting another. The implementation uses proper mutex locking and safety constants (`EPSILON_DIV`).
    *   **Completeness:** The patch updates the engine logic, configuration persistence (INI and Presets), GUI (checkboxes and tooltips), and documentation (Changelogs and Version). It also addresses a performance nitpick from previous iterations by ensuring string parsing for vehicle classes occurs during car changes or resets, rather than in the high-frequency FFB loop.
    *   **Testing:** The developer followed the specific instruction to provide "disabled" versions of modified tests. This ensures that both the legacy behavior (when enabled) and the new default behavior (when disabled) are verified and regression-tested. A new dedicated test suite for the toggle logic and persistence was also added.
    *   **Maintainability:** The code follows established patterns in the codebase, and the addition of `ResetNormalization` consolidates state management for normalization features.

3.  **Merge Assessment (Blocking vs. Non-Blocking):**
    *   The patch meets all functional and procedural requirements.
    *   **Nitpick:** The addition of `test_normalization.ini` to the repository root is slightly unconventional for test data (usually generated or stored in a data folder), but it is used appropriately in the new unit tests for persistence verification and does not block the release.

The patch is a complete, well-documented, and thoroughly tested solution.

### Final Rating: #Correct#