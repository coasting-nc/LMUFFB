# Implementation Plan - Issue #371: Imported profiles don't save.

## Context
User profiles (presets) were being lost because the `Config::presets` library was only populated when the Tuning GUI window was first opened. If an auto-save occurred before this (e.g., when static load was latched during the first few seconds of driving), `Config::Save` would overwrite `config.ini` with an empty preset list, effectively deleting all user-created and imported presets.

## Design Rationale
The root cause was a "lazy loading" pattern of the presets library that didn't account for the fact that the `Save` operation is global and destructive to sections not currently in memory. By moving the loading of the presets library to the application's initialization phase (within `Config::Load`), we guarantee that the memory state always matches the disk state before any write operation occurs.

## Reference Documents
- GitHub Issue #371
- `src/core/Config.cpp`
- `src/core/main.cpp`

## Codebase Analysis Summary
- **Current architecture:** `Config::Load` loads active engine settings. `Config::LoadPresets` loads the library of presets. `Config::Save` writes both.
- **Impacted functionalities:**
    - `Config::Load`: Now triggers preset loading.
    - `Config::LoadPresets`: Now supports loading from a specific file path to stay in sync with `Config::Load`.
    - `GuiLayer_Common`: The lazy-loading check remains as a safety measure but is now redundant.
- **Design Rationale:** `Config::Load` is the central entry point for loading settings at startup. It is the logical place to ensure all configuration data (including the library) is loaded.

## FFB Effect Impact Analysis
- No direct impact on FFB physics math.
- Indirect impact: Ensures user preferences for FFB effects are correctly persisted across sessions.

## Proposed Changes

### 1. `src/core/Config.h` & `src/core/Config.cpp`
- **Modify `Config::LoadPresets` signature:**
  - Changed from `static void LoadPresets();` to `static void LoadPresets(const std::string& filename = "");`.
- **Implement filename support in `Config::LoadPresets`:**
  - Uses the provided filename or falls back to `m_config_path`.
- **Update `Config::Load`:**
  - Calls `LoadPresets(filename);` at the end of the `Load` function.
- **Design Rationale:** This ensures that whenever a config is loaded (at startup or during tests), the associated presets are also loaded into memory, preventing data loss on subsequent saves.
- **Thread Safety:** Added `g_engine_mutex` protection to `LoadPresets` as it modifies the static `presets` vector which can be accessed by the FFB thread (for auto-saves) or the GUI thread.

### 2. Version Increment
- Increment version in `VERSION` to `0.7.192`.
- Update `CHANGELOG_DEV.md` with the fix.

## Detailed Steps

1.  **Create a reproduction test case in `tests/test_issue_371_repro.cpp`.** (Done)
2.  **Verify the test failure.** (Done)
3.  **Modify `src/core/Config.h` and `src/core/Config.cpp` to implement the fix.** (Done)
4.  **Verify the fix with the reproduction test.** (Done)
5.  **Perform an independent code review.** (Done)
6.  **Address any review issues and iterate if necessary.** (Done)
7.  **Run the full test suite to ensure no regressions.** (Done)
8.  **Complete pre-commit steps.** (Done)
9.  **Submit the changes.** (Pending)

## Implementation Notes
- **Review Iteration 1:** The review correctly pointed out the lack of thread safety for the static `presets` vector and the missing versioning/changelog deliverables. I addressed these by adding `g_engine_mutex` to `LoadPresets` and updating the mandatory files.
- **Thread Safety:** `LoadPresets` now correctly uses `g_engine_mutex`. Since `Config::Load` also uses it, and `Load` calls `LoadPresets`, the mutex was switched to `recursive_mutex` in a previous version, which is perfect for this nested call.
- **Test Integrity:** The reproduction test `test_issue_371_repro.cpp` was integrated into the CMake build and verifies that presets are no longer lost after a load-save cycle.

## Deliverables
- [x] Modified `src/core/Config.h` and `src/core/Config.cpp`.
- [x] New regression test case `tests/test_issue_371_repro.cpp`.
- [x] Updated `VERSION` and `CHANGELOG_DEV.md`.
- [x] Quality Assurance Records: `docs/dev_docs/code_reviews/issue_371_review_iteration_1.md` and `issue_371_review_iteration_2.md`.
- [x] Implementation Notes updated in the final plan.
