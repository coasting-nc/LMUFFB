# Implementation Plan - Issue 59: User Presets Ordering and Save Improvements

## Context
Issue #59:
1. Place user saved presets at the top of the drop-down list (after "Default").
2. Review if "Add 'Save' for current preset state just updating current preset" is already addressed in v0.7.15.

## Reference Documents
- GitHub Issue: https://github.com/coasting-nc/LMUFFB/issues/59

## Codebase Analysis Summary
- **Existing Architecture**:
    - `Config.h` / `Config.cpp`: Manages the `presets` vector. `LoadPresets()` populates it from built-ins and `config.ini`.
    - `GuiLayer.cpp`: Displays presets in a combo box using the `presets` vector indices.
    - Currently, `LoadPresets()` adds all built-ins first, then appends user presets from the INI file.
    - `AddUserPreset`, `DuplicatePreset`, and `ImportPreset` currently append to the end of the `presets` vector.
- **Impacted Functionalities**:
    - `Config::LoadPresets`: Change order of population.
    - `Config::AddUserPreset`: Change insertion point.
    - `Config::DuplicatePreset`: Change insertion point.
    - `Config::ImportPreset`: Change insertion point.
- **Save Feature Review**:
    - `GuiLayer.cpp` (v0.7.15) already checks if the selected preset is not built-in and calls `AddUserPreset` with the same name, which effectively updates it. This addresses the second part of the issue.

## FFB Effect Impact Analysis
None. This is a configuration and UI improvement. Physics logic is unaffected.

## Proposed Changes

1. **Create the test file `tests/test_issue_59.cpp`** with test cases verifying the correct ordering and insertion point of user presets.
2. **Build and run tests** to confirm that the new tests fail.
3. **Modify `src/Config.cpp`** to implement the new preset ordering and insertion logic:
    - Update `Config::LoadPresets()` to parse user presets earlier (after "Default").
    - Update `Config::AddUserPreset()`, `Config::DuplicatePreset()`, and `Config::ImportPreset()` to insert at the correct position.
4. **Verify the modifications to `src/Config.cpp`** using `read_file` to ensure the insertion logic matches the design.
5. **Update versioning and documentation**:
    - Increment version in `VERSION` to `0.7.16`.
    - Update `LMUFFB_VERSION` in `src/Version.h` to `"0.7.16"`.
    - Add entry to `CHANGELOG_DEV.md` describing the changes.
6. **Verify the updates to version files and changelog** using `read_file`.
7. **Run the full test suite** using the project's build and test commands (e.g., `cmake -S . -B build -DBUILD_HEADLESS=ON && cmake --build build --config Release`) to ensure all tests pass and no regressions were introduced.
8. **Complete pre-commit steps** to ensure proper testing, verification, review, and reflection are done.
9. **Submit the changes** with a descriptive commit message.

## Test Plan (TDD-Ready)

### Test Case 1: `test_user_presets_ordering`
- **Description**: Verify that user presets are placed between "Default" and other built-ins.
- **Expected Order**: [0]: "Default", [1..N]: User Presets, [N+1..M]: Other Built-ins.

### Test Case 2: `test_add_user_preset_insertion_point`
- **Description**: Verify that `AddUserPreset` inserts at the correct position (after "Default" and other user presets).

## Deliverables
- [x] Code changes in `Config.cpp`.
- [x] Version bump in `VERSION`, `src/Version.h`.
- [x] Changelog entry in `CHANGELOG_DEV.md`.
- [x] New test cases in `tests/test_issue_59.cpp`.
- [x] Implementation Notes (to be updated during development).

## Implementation Notes

### Unforeseen Issues
- **Test Fragility**: Moving user presets to the top of the list broke `test_duplicate_preset` which assumed duplicated presets were appended to the end of the vector. Updated the test to find the preset by name.

### Plan Deviations
- **Refined Insertion Point**: Specifically ensured user presets are inserted after "Default" (index 0) but before any other built-in presets.

### Challenges
- **Vector Reordering**: Careful refactoring of `LoadPresets` was required to ensure user presets are loaded and inserted at the correct position without breaking built-in loading.

### Recommendations
- **Preset Registry Class**: Consider moving preset management into a dedicated class with explicit methods for `AddUserPreset`, `GetBuiltinPresets`, etc., to make ordering more robust and less dependent on raw vector operations.
