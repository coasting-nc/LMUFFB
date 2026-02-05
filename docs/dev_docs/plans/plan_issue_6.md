# Implementation Plan - Preset Handling Improvements (Issue #6)

## Context
Improve the preset handling in lmuFFB to make it more intuitive and bug-free for users. This includes persisting the last used preset, indicating unsaved changes (dirty state), fixing the "Save Current Config" behavior, and adding "Delete" and "Copy" functionality for presets.

## Reference Documents
- GitHub Issue: https://github.com/coasting-nc/LMUFFB/issues/6

## Codebase Analysis Summary
- **Config Management:** `Config` class in `src/Config.h/cpp` handles saving/loading and preset management.
- **UI Layer:** `GuiLayer.cpp` handles the ImGui interface for selecting and saving presets.
- **Current Architecture:** Presets are a mix of built-in (hardcoded) and user-defined (stored in `config.ini`). Built-in presets cannot be modified or deleted.

## FFB Effect Impact Analysis
- **Affected Effects:** None.
- **User Perspective:** The UI will accurately reflect the current preset state and provide better tools for managing custom configurations.

## Proposed Changes

### 1. Persistence of Last Used Preset
- **Config.h:** Add `static std::string m_last_preset_name;` to `Config` class.
- **Config.cpp:**
    - Initialize `m_last_preset_name = "Default"`.
    - Update `Config::Save` to include `last_preset_name=...` in `config.ini`.
    - Update `Config::Load` to read `last_preset_name`.
    - Update `Config::ApplyPreset` to set `m_last_preset_name = presets[index].name`.
    - Update `Config::AddUserPreset` to set `m_last_preset_name = name`.

### 2. Dirty State Indication
- **Config.h:** Add `static bool IsEngineDirtyRelativeToPreset(int index, const FFBEngine& engine);`.
- **Config.cpp:** Implement `IsEngineDirtyRelativeToPreset` by comparing all relevant fields between the `Preset` struct at `index` and the `FFBEngine` state.
- **GuiLayer.cpp:**
    - Update the `preview_value` logic to append `*` if `IsEngineDirtyRelativeToPreset` returns true.
    - If `selected_preset` is -1 (Custom), it stays "Custom".

### 3. Improve "Save Current Config"
- **GuiLayer.cpp:**
    - Change "Save Current Config" button behavior:
        - If `selected_preset` corresponds to a user preset (non-builtin), call `Config::AddUserPreset(presets[selected_preset].name, engine)`.
        - This will update the existing user preset with current engine settings.
    - If `selected_preset` is a builtin preset or "Custom", it should probably stay as is (saving to the top-level of `config.ini`) or prompt the user to save as a new preset. *Decision: If it's a builtin, "Save Current Config" only updates the top-level config.ini (current behavior). If it's a user preset, it updates that preset.*

### 4. Delete and Copy Functionality
- **Config.h:**
    - `static void DeletePreset(int index);`
    - `static void DuplicatePreset(int index, const FFBEngine& engine);`
- **Config.cpp:**
    - Implement `DeletePreset`: remove from `presets` vector (if not builtin) and `Save(engine)`.
    - Implement `DuplicatePreset`: create a copy of `presets[index]`, give it a unique name (e.g., "Name (Copy)"), add to `presets` and `Save(engine)`.
- **GuiLayer.cpp:**
    - Add "Delete" button: Only visible/enabled when a user preset is selected.
    - Add "Copy" button: Visible for any selected preset.

### 5. Initialization and UI Fixes
- **GuiLayer.cpp:**
    - Initialize `selected_preset` in `DrawTuningWindow` by searching `Config::presets` for `Config::m_last_preset_name` if `Config::presets` is not empty and `selected_preset` is still at its initial value.

### 6. Version Increment
- Increment version in `VERSION` and `src/Version.h` by the smallest increment (e.g., v0.7.12 -> v0.7.13).

## Test Plan (TDD-Ready)

### New Tests in `tests/test_preset_improvements.cpp`

1. **`test_last_preset_persistence`**
    - Apply a preset.
    - Save config.
    - Load config into new engine/config state.
    - Assert `Config::m_last_preset_name` matches.

2. **`test_engine_dirty_detection`**
    - Apply a preset.
    - Assert `IsEngineDirtyRelativeToPreset` is false.
    - Change a setting in engine (e.g., `engine.m_gain += 0.1f`).
    - Assert `IsEngineDirtyRelativeToPreset` is true.

3. **`test_delete_user_preset`**
    - Add a user preset.
    - Delete it.
    - Assert it's gone from `Config::presets`.
    - Assert builtin presets cannot be deleted.

4. **`test_duplicate_preset`**
    - Duplicate a builtin preset.
    - Assert a new user preset exists with "(Copy)" or similar in the name.
    - Assert settings match the original.

## Deliverables
- [ ] Modified `src/Config.h` and `src/Config.cpp`
- [ ] Modified `src/GuiLayer.cpp`
- [ ] Modified `VERSION` and `src/Version.h`
- [ ] New test file `tests/test_preset_improvements.cpp`
- [ ] Documentation update (if needed, though this is mostly intuitive UI)
- [ ] Implementation Notes updated with any deviations.

## Implementation Notes

### Unforeseen Issues
- **Win32 Dependencies in Tests**: Running `Config` tests on Linux required a robust `windows.h` mock to satisfy Win32-specific types and function signatures used in the shared memory interface headers.

### Plan Deviations
- **Added `linux_mock/windows.h`**: Included a comprehensive Win32 stub to enable testing of core configuration logic in the Linux sandbox.
- **Improved "Save Current Config"**: Specifically targeted user presets for updates while maintaining the global save behavior for built-in/Custom states.

### Challenges
- **Floating Point Comparisons**: Implementing `IsEngineDirtyRelativeToPreset` required careful epsilon-based comparisons for all 40+ FFB parameters to avoid false "dirty" flags due to precision jitter.

### Recommendations
- **Modern File Dialogs**: Future updates could move to the Windows IFileDialog interface for a more modern look, though it requires COM initialization.
- **UI Logic Testing**: Continue expanding the `GuiInteractionTests` to verify complex UI-to-Engine state transitions without requiring a physical DirectX device.
