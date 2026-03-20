# Implementation Plan - Issue #429: Fix: can only save one config. And any imported files or new profiles are gone, leaving only the first config saved

**Issue:** #429 "Fix: can only save one config. And any imported files or new profiles are gone, leaving only the first config saved"
**Status:** In Progress

## Context
The application fails to correctly load or import multiple presets from `config.ini` or external `.ini` files. Users report that only one custom preset is preserved, and others disappear. This is due to bugs in the parsing logic where presets are not always "pushed" to the internal list when a new section starts or at the end of the file. Furthermore, the logic is duplicated across `LoadPresets` and `ImportPreset`, making it prone to errors.

## Design Rationale
The current parsing logic is a manual state machine that is fragile and duplicated. A robust solution requires:
1.  **Correct State Transitions:** Ensuring that every time a new `[Preset:Name]` header is found, the *previous* preset being parsed is saved to the list.
2.  **EOF Handling:** Ensuring the final preset in a file is saved when the end of the file is reached.
3.  **Deduplication:** Consolidating the parsing logic to reduce the chance of future bugs.
4.  **Reliability:** Adding tests that specifically verify multi-preset loading and importing to prevent regressions.

## Reference Documents
- GitHub Issue: #429
- Previous related fix: Issue #371 (v0.7.204)

## Codebase Analysis Summary
- **src/core/Config.h**:
    - Defines `Preset` struct and `Config` class.
    - `Config::LoadPresets`, `Config::ImportPreset`, `Config::Load` are the key methods.
- **src/core/Config.cpp**:
    - `Config::LoadPresets()`: Reads `config.ini` and populates `presets`.
    - `Config::ImportPreset()`: Reads an external file and appends to `presets`.
    - `Config::ParsePresetLine()`: Helper for parsing key-value pairs within a preset section.

### Impacted Functionalities
- **Preset Loading:** How the app reads its own `config.ini` on startup.
- **Preset Importing:** How the app adds external `.ini` files.
- **Data Persistence:** If presets are dropped during load, they are permanently lost if a `Save()` occurs.

### Design Rationale for Impact Zone
The `Config` class's parsing methods are the direct cause of the data loss. By fixing the state machine logic in `LoadPresets` and `ImportPreset`, we ensure that all data present on disk is correctly reflected in memory.

## FFB Effect Impact Analysis
This is a configuration and data persistence fix. It does not change FFB physics but ensures that user-tuned FFB settings are not lost, which is critical for user experience and reliability.

## Proposed Changes

### 1. src/core/Config.cpp - `Config::ImportPreset`
- Update the loop to correctly push the `current_preset` when a new `[` is encountered IF `preset_pending` is true.
- Ensure the final `current_preset` is pushed after the loop finishes.
- (Optional but recommended) Refactor common "push" logic into a helper to avoid code duplication.

### 2. src/core/Config.cpp - `Config::LoadPresets`
- Similar to `ImportPreset`, verify that it correctly handles multiple `[Preset:...]` sections.
- The diagnosis in the issue suggests `LoadPresets` might be better than `ImportPreset` but still has subtle issues if non-preset sections follow presets (though they currently don't).

### 3. src/core/Config.cpp - Refactoring (Unified Parsing)
- I will attempt to unify the "finish preset" logic into a single method to ensure consistency between `LoadPresets` and `ImportPreset`.

### Parameter Synchronization Checklist
N/A (No new FFB settings added).

### Version Increment Rule
Version in `VERSION` will be incremented: `0.7.208` -> `0.7.209`.

## Test Plan (TDD-Ready)

### Design Rationale
We need to prove that multiple presets can be loaded and imported without loss.

### 1. Test: `test_issue_429_multi_preset_load`
- **Description**: Verifies that `LoadPresets` correctly reads multiple custom presets from a mock `config.ini`.
- **Steps**:
    1. Create `config.ini` with `[Preset:A]` and `[Preset:B]`.
    2. Call `Config::LoadPresets()`.
    3. Assert `presets` size is 1 (Default) + 2 (Built-ins) + 2 (Customs) = ... (Wait, I need to check how many built-ins there are).
    4. Assert both "A" and "B" exist in the vector.

### 2. Test: `test_issue_429_multi_preset_import`
- **Description**: Verifies that `ImportPreset` correctly reads multiple presets from a single external file.
- **Steps**:
    1. Create `import.ini` with `[Preset:X]` and `[Preset:Y]`.
    2. Call `Config::ImportPreset("import.ini", engine)`.
    3. Assert both "X" and "Y" were added to the `presets` vector.

## Deliverables
- [x] Modified `src/core/Config.cpp`
- [x] New test file `tests/test_issue_429_repro.cpp`
- [x] Updated `VERSION` and `CHANGELOG_DEV.md`
- [x] Implementation Notes in this plan.

## Implementation Notes
### Encountered Issues
- **Reproduction Nuance**: During testing, it was discovered that `test_issue_429_repro_load` (loading from `config.ini`) actually passed even before the fix. This is because `Config::LoadPresets` already contained a check to push pending presets when a new `[` was encountered. However, `Config::ImportPreset` was missing this logic entirely, causing the reported failure when importing multiple presets from an external file.
- **Refactoring Complexity**: Refactoring the logic into `FinalizePreset` required careful handling of name collisions, which are only desired during imports (to avoid overwriting existing presets) but not during normal loading (where we expect to load exactly what's on disk).

### Plan Deviations
- Renamed the test file to `tests/test_issue_429_repro.cpp` for consistency with existing reproduction tests.
- Refactored the "push" logic into a static helper `FinalizePreset` in `Config.cpp` to ensure consistent application of migrations, validation, and collision handling.

### Challenges
- Ensuring the `FinalizePreset` helper correctly handled the `needs_save` flag and `handle_collisions` toggle across different calling contexts (`LoadPresets` vs `ImportPreset`).

### Recommendations
- **Robust INI Parsing**: As suggested in the issue, the application would benefit from a dedicated INI parsing library or a more structured `IniDocument` representation to avoid manual state machine errors in the future.
- **Deduplication**: There is still some duplication in how `config.ini` is parsed (it's parsed once for main settings and once for user presets). Consolidating this into a single pass would improve performance and reliability.
