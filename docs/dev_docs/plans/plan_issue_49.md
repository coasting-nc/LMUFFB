# Implementation Plan - Issue 49: Add Preset Import/Export Feature (Updated)

## Context
Issue #49: Add preset import/export feature.
Goal: Allow users to easily share presets by exporting them to a file and importing them from a file, instead of manual INI editing.

## Reference Documents
- GitHub Issue: https://github.com/coasting-nc/LMUFFB/issues/49
- Plan Review: `docs/dev_docs/reviews/plan_review_issue_49.md`

## Codebase Analysis Summary
- **Existing Architecture**:
    - `Config.h` / `Config.cpp`: Handles preset management, saving/loading from `config.ini`.
    - `GuiLayer.cpp`: Handles the user interface, including the preset selection dropdown and "Save New" button.
    - `Preset` struct in `Config.h`: Contains all FFB parameters and helper methods to apply/update from engine.
    - `Config::presets`: A `std::vector<Preset>` holding all presets (built-in and user-created).
- **Impacted Functionalities**:
    - `Config`: Needs new methods for single preset file I/O.
    - `GuiLayer`: Needs new buttons and Win32 file dialog integration.
- **Data Flow**:
    - Export: `selected_preset` index -> `Config::ExportPreset` -> Disk file (.ini).
    - Import: Disk file (.ini) -> `Config::ImportPreset` -> `Config::presets` vector -> `Config::Save` (persists to `config.ini`).

## FFB Effect Impact Analysis
Not applicable. This is a configuration/UI feature. No physics logic or FFB effects are modified.

## Proposed Changes

### 1. `src/Config.h`
- Add static methods:
    ```cpp
    static void ExportPreset(int index, const std::string& filename);
    static bool ImportPreset(const std::string& filename);
    ```
- Add a private helper method for parsing a preset section (to be used by both `LoadPresets` and `ImportPreset`):
    ```cpp
    private:
    static void ParsePresetSection(std::ifstream& file, Preset& p, std::string& version, bool& preset_pending);
    ```

### 2. `src/Config.cpp`
- **Refactor `LoadPresets`**:
    - Move the logic that parses a `[Preset:...]` section into `ParsePresetSection`.
- **`ExportPreset(int index, const std::string& filename)`**:
    - Validate index.
    - Open `filename` for writing.
    - Write a single `[Preset:Name]` section containing all members of the `Preset` struct (same format as in `config.ini`).
- **`ImportPreset(const std::string& filename)`**:
    - Open `filename` for reading.
    - Use `ParsePresetSection` to read the preset.
    - Check if a preset with the same name already exists in `presets`.
    - If it exists, append a suffix like " (imported)" to the name.
    - Add to `presets` vector.
    - Call `Config::Save()` to persist the new preset into the main `config.ini`.

### 3. `src/GuiLayer.cpp`
- Inside `DrawTuningWindow`, in the "Presets and Configuration" section:
    - Add an **"Export Selected"** button.
        - Enabled if `selected_preset >= 0`.
    - Add an **"Import Preset"** button.
- **Win32 File Dialogs**:
    - Implement `OpenPresetFileDialog` and `SavePresetFileDialog` using `GetOpenFileName` and `GetSaveFileName`.
    - Handle the returned file paths and call `Config::ImportPreset` / `Config::ExportPreset`.

### 4. Versioning & Changelog
- **`VERSION`**: Increment by smallest possible step (e.g., `0.7.11` -> `0.7.12`).
- **`src/Version.h`**: Match `VERSION` file.
- **`CHANGELOG_DEV.md`**: Add entry: "Added Preset Import/Export feature for easier sharing."

### 5. Documentation
- **`README.md`**: Mention the new Import/Export buttons in the usage section.
- **`docs/ffb_customization.md`**: Add a section on how to share presets using the new feature.

## Parameter Synchronization Checklist
Not applicable. No new FFB parameters are added.

## Initialization Order Analysis
No cross-header changes that would affect initialization order or introduce circular dependencies.

## Test Plan (TDD-Ready)

### Test Case 1: `test_export_preset`
- **Description**: Verify a preset can be exported to a standalone file.
- **Inputs**: A `Preset` object with specific values, a target filename "test_export.ini".
- **Expected Outputs**: A file "test_export.ini" containing the preset parameters.
- **Assertions**:
    - File exists.
    - File contains `[Preset:...]`.
    - Key-value pairs match the preset object.

### Test Case 2: `test_import_preset`
- **Description**: Verify a preset can be imported from a standalone file.
- **Inputs**: A valid preset file on disk "test_import.ini".
- **Expected Outputs**: `Config::presets` contains the new preset.
- **Assertions**:
    - `ImportPreset` returns `true`.
    - The imported preset in `Config::presets` has correct values.
    - `config.ini` is updated with the new preset.

## Deliverables
- [x] Code changes in `Config.h`, `Config.cpp`, `GuiLayer.cpp`.
- [x] Version bump in `VERSION`, `src/Version.h`.
- [x] Changelog entry in `CHANGELOG_DEV.md`.
- [x] New test cases in `tests/test_ffb_import_export.cpp`.
- [x] Documentation updates in `README.md` and `docs/ffb_customization.md`.
- [x] Implementation Notes (to be updated during development).

## Implementation Notes

### Unforeseen Issues
- **Win32 File Dialogs on Linux**: The Linux sandbox environment does not support `windows.h` or `commdlg.h`, which are required for `GetOpenFileNameA`. This made full compilation impossible in the sandbox.
- **Shared Memory Mocking**: Building and running unit tests required extensive mocking of Windows-specific types and functions used in the vendor-provided shared memory headers.

### Plan Deviations
- **Added `WritePresetFields` Helper**: Decided to extract the writing logic into a separate method to ensure consistency between `Config::Save` and `Config::ExportPreset`.
- **Version Increment**: Incremented version to `0.7.13` instead of `0.7.12` because `0.7.12` was already used in the base branch.

### Challenges
- **Mocking `windows.h`**: Creating a clean `windows.h` stub in the test directory was necessary to allow the physics engine and configuration logic to be tested in the Linux-based development environment.
- **Handling Name Collisions**: Ensuring that imported presets do not overwrite existing ones without user consent required implementing a robust counter-based renaming logic.

### Recommendations
- **Modernize File Dialogs**: In the future, consider using the Common Item Dialog (Shell) for a more modern Windows look, although it requires COM initialization.
- **Cross-Platform Tests**: Continue isolating physics logic from platform-specific APIs to maintain high test coverage in non-Windows environments.
