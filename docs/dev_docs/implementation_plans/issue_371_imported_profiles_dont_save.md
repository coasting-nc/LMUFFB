# Implementation Plan - Issue #371: Imported profiles don't save

**Issue:** #371 "Imported profiles don't save."
**Status:** Complete

## Context
User-defined and imported presets are being lost upon application restart or session change. This happens because the `presets` vector in `Config.cpp` is only populated when the user interacts with the Preset UI in the GUI. If an auto-save occurs before this population, the `config.ini` file is overwritten with an empty `[Presets]` section.

## Design Rationale
The core problem is a "lazy loading" pattern that is incompatible with an "active auto-save" system. To ensure reliability and prevent data loss, the configuration state must be fully synchronized upon application startup. By moving the preset loading into the main `Config::Load` sequence, we guarantee that the internal state always reflects the persisted data before any write operations can occur. This is a critical safety and reliability fix for user data persistence.

## Reference Documents
- GitHub Issue: #371 (verbatim copy at `docs/dev_docs/github_issues/371.md`)

## Codebase Analysis Summary
- **src/core/Config.cpp**:
    - `LoadPresets()`: Populates the `presets` vector from `config.ini`. Currently called only from GUI.
    - `Load()`: Loads main settings. Does NOT call `LoadPresets()`.
    - `Save()`: Writes everything, including the `presets` vector, to `config.ini`.
- **src/gui/GuiLayer_Common.cpp**:
    - Contains the lazy call: `if (Config::presets.empty()) Config::LoadPresets();`.

### Impacted Functionalities
- **Startup sequence**: Will now load all presets immediately.
- **Auto-save mechanism**: Will no longer overwrite presets with an empty list.
- **Memory footprint**: Slightly higher at startup as presets are loaded into memory immediately (negligible for typical number of presets).

### Design Rationale for Impact Zone
The `Config` class is the central authority for persistence. The issue is a synchronization gap between memory and disk. Fixing it at the source (`Config::Load`) is the most robust approach and follows the principle of "Single Source of Truth".

## FFB Effect Impact Analysis
This task fixes a configuration persistence bug and does not directly alter FFB physics logic. However, by ensuring presets are correctly loaded and saved, it improves the reliability of user-configured FFB settings across sessions. No user-facing FFB "feel" changes are expected, only the preservation of their settings.

## Proposed Changes

### 1. src/core/Config.cpp
- Modify `Config::Load` to call `LoadPresets()` at the beginning of the function.
- Update `Config::LoadPresets()` to ensure it doesn't duplicate built-in presets if called multiple times (though `presets.clear()` is already present at the start of `LoadPresets()`).

### 2. src/gui/GuiLayer_Common.cpp
- Remove the lazy loading check `if (Config::presets.empty()) Config::LoadPresets();` since it will now be guaranteed by `Config::Load`.

### Design Rationale
- **Algorithm Choice**: Eager loading vs. Lazy loading. Eager loading is preferred for configuration data that must be preserved during auto-saves.
- **Architectural Pattern**: Ensuring data integrity by completing the "Read" phase before any "Write" can happen.

### Version Increment Rule
Version number in `VERSION` will be incremented by the smallest possible increment: `0.7.203` -> `0.7.204`.

## Test Plan (TDD-Ready)

### Design Rationale
The test must prove that a sequence of `Load` -> `Save` (without UI intervention) preserves presets. This directly targets the reported failure mode.

### 1. Reproduction Test: `test_issue_371_repro`
- **Description**: Verifies that user presets are retained after a load-save cycle without GUI intervention.
- **Inputs**: A mock `config.ini` containing a user preset.
- **Execution**:
    1. Create a temporary `config.ini` with a user preset.
    2. Call `Config::Load`.
    3. Call `Config::Save`.
    4. Call `Config::Load` again (into a fresh engine/state).
- **Assertion**: The user preset must still exist in the `presets` vector.
- **Expected Outcome**: Fails before fix (presets empty after step 2), passes after fix.

## Deliverables
- [x] Modified `src/core/Config.cpp`
- [x] Modified `src/gui/GuiLayer_Common.cpp`
- [x] New test `tests/test_issue_371_repro.cpp`
- [x] Implementation Notes updated in this plan.

## Implementation Notes
### Encountered Issues
- None. The fix was straightforward once the root cause was identified.

### Deviations from the Plan
- Fixed version comment in `Config.cpp` to match the target version `0.7.204`.
- Removed commented-out code in `GuiLayer_Common.cpp` instead of just commenting it.

### Suggestions for the future
- Consider implementing a more robust observer pattern for configuration changes to reduce dependence on manual `Save()` calls and `m_needs_save` flags.
