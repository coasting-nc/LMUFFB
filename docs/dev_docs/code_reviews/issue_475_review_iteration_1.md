The patch addresses a specific bug where the `Lateral Load` configuration setting was being incorrectly clamped during validation, preventing values above 2.0 (200%) from being persisted across application restarts, despite the GUI allowing values up to 10.0 (1000%).

### Analysis and Reasoning:

1.  **User's Goal:** Resolve the issue where the `Lateral Load` slider value is reset to 200% upon reloading the application if set higher than that.

2.  **Evaluation of the Solution:**
    *   **Core Functionality:** The patch directly targets the root cause in `src/ffb/FFBConfig.h`. By updating the `Validate()` method for `LoadForcesConfig`, it changes the upper bound of the `lat_load_effect` clamp from `2.0f` to `10.0f`. This aligns the backend validation logic with the frontend GUI constraints.
    *   **Safety & Side Effects:** The change is safe. Increasing the allowed range for this specific physics parameter does not introduce memory safety issues or security vulnerabilities. It makes the behavior consistent with other similar parameters like `long_load_effect`, which was already clamped to `10.0f`.
    *   **Completeness:** The patch is exceptionally thorough. It includes:
        *   The logic fix.
        *   a new reproduction test case (`tests/repro_issue_475.cpp`) that verifies the fix by performing a Save/Load cycle with a value > 2.0.
        *   Registration of the new test in `tests/CMakeLists.txt`.
        *   Required documentation updates (`CHANGELOG_DEV.md`, `USER_CHANGELOG.md`).
        *   A version increment in the `VERSION` file, following the project's workflow.

3.  **Merge Assessment:**
    *   **Blocking:** None.
    *   **Nitpicks:** None. The inclusion of a reproduction test confirms the fix and prevents future regressions.

### Final Rating: #Correct#
