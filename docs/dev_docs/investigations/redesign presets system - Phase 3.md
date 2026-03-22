# Implementation Plan: Phase 3 - Externalizing User Presets

**Task ID:** `phase3_external_presets`
**Context:** Transition from storing all presets inside the main `config.toml` file to a "One File Per User Preset" architecture. Built-in presets will be embedded as TOML strings in the binary to guarantee they are read-only, while user presets will be saved as individual `.toml` files in a `user_presets/` directory.
**Design Rationale:** 
*   **UX & Sharing:** Storing user presets as individual files in a top-level `user_presets/` folder makes it trivial for users to share, backup, and organize their profiles.
*   **Integrity:** Embedding built-in presets as raw TOML strings in the C++ binary ensures they cannot be accidentally modified or deleted by the user, preventing support tickets about "broken default FFB".
*   **UI Clarity:** Prefixing built-in presets in the UI prevents name collisions and makes it immediately obvious to the user which profiles are safe to overwrite and which are factory defaults.

**See also:** 
* Additional context with initial overview of all the Phases `docs\dev_docs\investigations\redesign presets system.md`
* Implementation notes for Phase 2 `docs/dev_docs/reports/phase2_toml_integration_implementation_notes.md`

## 1. Codebase Analysis Summary
*   **`src/core/Config.h` & `src/core/Config.cpp`:**
    *   `Config::LoadPresets()`: Currently parses the `[Presets]` table from `config.toml` and hardcodes C++ structs for built-ins. Needs to be rewritten to parse embedded TOML strings for built-ins, and iterate the `user_presets/` directory for user files.
    *   `Config::Save()`: Currently writes user presets into the `[Presets]` table of `config.toml`. Needs to be updated to *only* save the main config, and a separate mechanism will save individual user presets to `user_presets/<name>.toml`.
    *   `Config::AddUserPreset()`, `Config::DeletePreset()`: Need to be updated to create/delete actual `.toml` files on disk.
*   **`src/gui/GuiLayer_Common.cpp`:**
    *   The preset dropdown UI needs to format the display names to distinguish built-ins (e.g., `[Default] T300`).

## 2. Proposed Changes

### 2.1. Create `BuiltinPresets.h` (Embedded TOML)
*   **Action:** Create a new file containing the built-in presets as raw C++ string literals (`R"(...)"`).
*   **Design Rationale:** Keeps `Config.cpp` clean while leveraging the `toml++` parser to construct the `Preset` objects, ensuring the exact same parsing logic is used for both built-in and user presets.
*   **Example:**
    ```cpp
    constexpr const char* PRESET_T300_TOML = R"(
    [General]
    gain = 1.0
    wheelbase_max_nm = 4.0
    target_rim_nm = 4.0
    # ... rest of T300 settings ...
    )";
    ```

### 2.2. Update `Config::LoadPresets()`
*   **Action:** 
    1. Parse the embedded strings from `BuiltinPresets.h` using `toml::parse(string_view)`. Set `is_builtin = true` for these.
    2. Ensure the `user_presets/` directory exists using `std::filesystem::create_directories("user_presets")`.
    3. Iterate through `user_presets/`. For every `.toml` file, parse it, set `is_builtin = false`, and add it to the `presets` vector.

### 2.3. Implement One-Time Migration from `config.toml`
*   **Action:** When `Config::Load()` runs, check if the loaded `config.toml` contains a `[Presets]` table.
*   **Logic:**
    1. If `tbl.contains("Presets")`, iterate through the table.
    2. For each user preset found, serialize it and save it as a new file: `user_presets/<sanitized_name>.toml`.
    3. Remove the `[Presets]` node from the `config.toml` table in memory.
    4. Call `Config::Save()` to overwrite `config.toml` without the presets table, ensuring this migration only happens once.

### 2.4. Update Preset Save/Delete/Duplicate Logic
*   **Action:** Update `Config::AddUserPreset` and `Config::Save` (when saving a specific preset).
    *   Create a helper function `SanitizeFilename(std::string name)` that replaces spaces with underscores and removes illegal Windows filename characters (`\ / : * ? " < > |`).
    *   Save the preset to `user_presets/<sanitized_name>.toml`.
*   **Action:** Update `Config::DeletePreset` to call `std::filesystem::remove()` on the corresponding file.

### 2.5. Update the UI (`GuiLayer_Common.cpp`)
*   **Action:** In the `DrawTuningWindow` preset combo box, format the display string.
    *   If `p.is_builtin`, display `"[Default] " + p.name`.
    *   If `!p.is_builtin`, display `p.name`.
*   **Action:** Ensure the "Delete" and "Save Current Config" buttons remain disabled if a built-in preset is currently selected.

## 3. Test Plan (TDD-Ready)

The agent must write these tests **BEFORE** implementing the changes.

### Test 1: `test_phase3_embedded_builtins`
*   **Goal:** Prove that built-in presets are successfully parsed from raw strings and flagged correctly.
*   **Action:** Call `Config::LoadPresets()`.
*   **Assertions:** 
    *   Verify that the "T300" preset exists in the vector.
    *   Assert `p.is_builtin == true`.
    *   Assert `p.cfg.general.wheelbase_max_nm == 4.0f`.

### Test 2: `test_phase3_user_preset_file_io`
*   **Goal:** Prove that user presets are saved to and loaded from the `user_presets/` directory, and that filenames are sanitized.
*   **Setup:** Ensure `user_presets/` is empty.
*   **Action:** Call `Config::AddUserPreset("My Crazy Wheel: V2!", engine)`.
*   **Assertions:**
    *   Assert that a file named `user_presets/My_Crazy_Wheel__V2_.toml` (or similar sanitized name) exists on disk.
    *   Clear the `Config::presets` vector in memory.
    *   Call `Config::LoadPresets()`.
    *   Assert that "My Crazy Wheel: V2!" is loaded back into memory and `is_builtin == false`.

### Test 3: `test_phase3_migration_from_config_toml`
*   **Goal:** Prove that user presets trapped inside the Phase 2 `config.toml` are successfully extracted to individual files and removed from the main config.
*   **Setup:** Create a mock `config.toml` that contains a `[Presets."Trapped Preset"]` table. Ensure `user_presets/` is empty.
*   **Action:** Call `Config::Load(engine, "config.toml")`.
*   **Assertions:**
    *   Assert that `user_presets/Trapped_Preset.toml` was created on disk.
    *   Load `config.toml` directly from disk into a raw `toml::table`.
    *   Assert that `tbl.contains("Presets") == false` (proving it was cleaned up).

## 4. Deliverables
- [ ] `src/core/BuiltinPresets.h` created with raw TOML strings.
- [ ] `src/core/Config.cpp` updated (Directory creation, Migration, File I/O).
- [ ] `src/gui/GuiLayer_Common.cpp` updated (UI prefixes).
- [ ] `tests/test_toml_presets.cpp` created with the 3 new tests.
- [ ] `VERSION` incremented.
- [ ] `CHANGELOG_DEV.md` entry added.
- [ ] Current implementation plan updated with implementation notes.