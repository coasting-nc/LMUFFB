# Code Review - Issue #371: Imported profiles don't save - Iteration 3

## Evaluation of the Solution
*   **Core Functionality:** The patch correctly identifies the root cause: "lazy-loading" of presets. Presets were only loaded into memory when the GUI expanded the relevant section. Because the application uses an auto-save mechanism (e.g., on exit), the `config.ini` file was being overwritten with an empty preset list because the internal `presets` vector had not yet been populated. By moving `LoadPresets()` into the main `Config::Load()` sequence, the internal state is synchronized with the disk immediately upon startup.
*   **Safety & Side Effects:** The solution is safe and maintains thread integrity. `LoadPresets()` utilizes the existing recursive `g_engine_mutex`, making it safe to call within `Config::Load()`. The performance impact is negligible as it merely shifts a small I/O operation from UI interaction to startup.
*   **Completeness:** The patch is exceptionally complete. It includes:
    *   The logic fix in `Config.cpp` and cleanup in `GuiLayer_Common.cpp`.
    *   A robust regression test (`tests/test_issue_371_repro.cpp`) that simulates the failure lifecycle.
    *   Necessary project metadata updates (`VERSION`, `CHANGELOG_DEV.md`).
    *   Full documentation following the "Fixer" workflow (Implementation Plan, GitHub issue copy, and multiple iteration code reviews).

## Merge Assessment (Blocking vs. Non-Blocking)
*   **Blocking:** None.
*   **Nitpicks:** None. The developer addressed previous review concerns (iteration 1) regarding versioning and commented-out code.

The solution is professional, follows the mandated reliability standards, and includes verified regression testing.

## Final Rating: #Correct#
