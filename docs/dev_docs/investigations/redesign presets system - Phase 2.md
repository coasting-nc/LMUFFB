# Implementation Plan: Phase 2 - TOML Integration

**Task ID:** `phase2_toml_integration`
**Context:** Replace the fragile, manual string-parsing logic (`std::getline`, `std::stof`) for the main configuration file with the robust, type-safe `toml++` library. Rename `config.ini` to `config.toml`.
**Reference Documents:** 
* `docs\dev_docs\investigations\redesign presets system.md`
* `docs\dev_docs\investigations\redesign presets system - Phase 1.md`

## 1. Design Rationale
*   **Why TOML?** TOML is the industry standard for configuration files. It is strongly typed (distinguishes between floats, ints, booleans, and strings), supports comments, and handles whitespace gracefully. This eliminates silent parsing failures (e.g., `std::stof` failing on `gain=1.0f`).
*   **Why `toml++`?** It is a C++17/20 header-only library that is highly performant, widely used, and easy to integrate without complex CMake/build dependencies.
*   **The "Untouched Presets" Constraint:** Because TOML syntax differs slightly from INI (e.g., strings require quotes `"value"`, booleans are `true`/`false` instead of `1`/`0`), keeping the old `std::getline` parser for the bottom half of a `.toml` file will cause crashes. Therefore, to safely transition the file to `config.toml`, we *must* use `toml++` to read/write the presets within that file. However, we will **not** change the `Preset` struct architecture or externalize them to separate files yet (that is Phase 3/4). We are simply swapping the *parser* used in `LoadPresets()`.

## 2. Codebase Analysis Summary
*   **`src/core/Config.h` & `src/core/Config.cpp`:**
    *   `m_config_path`: Needs to change default from `"config.ini"` to `"config.toml"`.
    *   `Config::Load()`: Currently reads line-by-line. Will be rewritten to parse the file into a `toml::table` and map values to `engine.m_config`.
    *   `Config::Save()`: Currently writes strings. Will be rewritten to build a `toml::table` and stream it to the file.
    *   `Config::LoadPresets()`: Will be updated to iterate over the `[Presets]` TOML table instead of parsing strings.
*   **Build System (`Makefile`, `.github/workflows/windows-build-and-test.yml`):**
    *   Needs to include the `toml.hpp` header.
*   **`LICENSE`:**
    *   Needs to be updated to reflect the bundled dependencies (LZ4, ImGui, toml++).

## 3. FFB Effect Impact Analysis
*   **Affected FFB Effects:** None.
*   **Technical/Developer Perspective:** This is a pure serialization/deserialization refactor. The physics math remains completely untouched.
*   **User Perspective:** The configuration file will be renamed to `config.toml`. The syntax will become stricter but more readable (e.g., `always_on_top = true` instead of `always_on_top = 1`). Existing `config.ini` files will be ignored, effectively resetting users to defaults unless they manually rename and fix the syntax of their old file. *(Note: A migration script from .ini to .toml could be written, but is usually out of scope for this phase unless explicitly requested).*

## 4. Proposed Changes

### 4.1. Dependency Integration & Licensing
*   **Action:** Download the single-header version of `toml++` (`toml.hpp`) and place it in `src/ext/toml++/toml.hpp`.
    *   *Rationale:* Checking the header directly into the repository guarantees build stability across all platforms (Windows/Linux/CI) without relying on external package managers or `curl` commands failing during CI runs.
*   **Action:** Update `Makefile`.
    *   Add `-Isrc/ext/toml++` to the `CXXFLAGS`.
*   **Action:** Update `LICENSE` file in the root directory.
    *   Append the MIT License text for `ImGui`.
    *   Append the BSD 2-Clause License text for `LZ4`.
    *   Append the MIT License text for `toml++`.

### 4.2. Update `Config.h`
*   **Action:** Change `static std::string m_config_path = "config.ini";` to `"config.toml"`.

### 4.3. Rewrite `Config::Save()`
*   **Action:** Replace the manual file streaming with `toml::table` construction.
*   **Logic:**
    ```cpp
    #include <toml++/toml.h>

    void Config::Save(const FFBEngine& engine, const std::string& filename) {
        std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
        std::string final_path = filename.empty() ? m_config_path : filename;

        toml::table tbl;

        // 1. System Settings
        tbl.insert("System", toml::table{
            {"app_version", LMUFFB_VERSION},
            {"always_on_top", m_always_on_top},
            {"last_device_guid", m_last_device_guid},
            // ... win_pos_x, etc.
        });

        // 2. Physics Settings (Using the new Phase 1 structs!)
        tbl.insert("General", toml::table{
            {"gain", engine.m_config.general.gain},
            {"invert_force", engine.m_config.general.invert_force},
            // ...
        });

        tbl.insert("FrontAxle", toml::table{
            {"understeer_effect", engine.m_config.front_axle.understeer_effect},
            // ...
        });
        // ... repeat for RearAxle, Braking, Vibration, etc.

        // 3. Presets Bridge (Keep them in the file for now)
        toml::table presets_tbl;
        for (const auto& p : presets) {
            if (!p.is_builtin) {
                toml::table preset_node;
                preset_node.insert("gain", p.cfg.general.gain);
                // ... map preset fields ...
                presets_tbl.insert(p.name, preset_node);
            }
        }
        tbl.insert("Presets", presets_tbl);

        // Write to file
        std::ofstream file(final_path);
        if (file.is_open()) {
            file << tbl;
        }
    }
    ```

### 4.4. Rewrite `Config::Load()`
*   **Action:** Replace `std::getline` loop with `toml::parse_file`.
*   **Logic:**
    ```cpp
    void Config::Load(FFBEngine& engine, const std::string& filename) {
        std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
        LoadPresets(); // Load presets first, as before

        std::string final_path = filename.empty() ? m_config_path : filename;
        
        try {
            toml::table tbl = toml::parse_file(final_path);

            // System
            if (auto sys = tbl["System"].as_table()) {
                m_always_on_top = sys->get_as<bool>("always_on_top")->value_or(true);
                // ...
            }

            // Physics (Using Phase 1 structs)
            if (auto gen = tbl["General"].as_table()) {
                engine.m_config.general.gain = gen->get_as<double>("gain")->value_or(1.0f);
                // ...
            }
            
            // ... repeat for other categories ...

        } catch (const toml::parse_error& err) {
            Logger::Get().LogFile("[Config] TOML Parse Error: %s", err.description().c_str());
            return;
        }

        engine.m_config.Validate(); // Ensure safe ranges
    }
    ```

### 4.5. Rewrite `Config::LoadPresets()`
*   **Action:** Use `toml++` to iterate over the `[Presets]` table instead of parsing strings.
*   **Logic:**
    ```cpp
    void Config::LoadPresets() {
        presets.clear();
        // ... push back hardcoded built-in presets ...

        try {
            toml::table tbl = toml::parse_file(m_config_path);
            if (auto presets_tbl = tbl["Presets"].as_table()) {
                for (auto& [key, node] : *presets_tbl) {
                    if (auto preset_data = node.as_table()) {
                        Preset p(std::string(key.str()), false);
                        
                        // Map TOML data to p.cfg
                        p.cfg.general.gain = preset_data->get_as<double>("gain")->value_or(1.0f);
                        // ... map rest ...

                        p.cfg.Validate();
                        presets.push_back(p);
                    }
                }
            }
        } catch (...) {
            // Ignore parse errors here, Load() will catch and log them
        }
    }
    ```

### 4.6. Version Increment Rule and Changelog
*   Increment the version number in `VERSION` by the smallest possible increment (e.g., `0.7.x` -> `0.7.x+1`).
*   You must also add an entry to `CHANGELOG_DEV.md` file.

## 5. Test Plan (TDD-Ready)

Write these tests in `tests/test_toml_config.cpp` **BEFORE** implementing the changes.

### Test 1: `test_toml_roundtrip_serialization`
*   **Design Rationale:** Proves that the engine state can be written to a TOML file and read back perfectly, ensuring no data loss or type mismatch occurs during the transition.
*   **Inputs:** An `FFBEngine` initialized with non-default, highly specific values across all config categories.
*   **Outputs:** A `test_roundtrip.toml` file.
*   **Assertions:** 
    *   Load the file into a *new* `FFBEngine` instance.
    *   Assert that `engine1.m_config.Equals(engine2.m_config)` is true.

### Test 2: `test_toml_missing_keys_fallback`
*   **Design Rationale:** Proves that if a user deletes a line from their `config.toml` (or if a new update adds a new feature), the parser safely falls back to the default value without crashing.
*   **Inputs:** A manually constructed `test_missing.toml` file containing only `[General] gain = 0.5`.
*   **Outputs:** An `FFBEngine` instance loaded from this file.
*   **Assertions:**
    *   Assert `engine.m_config.general.gain == 0.5f`.
    *   Assert `engine.m_config.front_axle.understeer_effect == 1.0f` (Default value).

### Test 3: `test_toml_type_safety`
*   **Design Rationale:** Proves that `toml++` prevents the crashes that used to happen with `std::stof` when users typed invalid data.
*   **Inputs:** A `test_bad_types.toml` file containing `gain = "high"` (string instead of float) and `invert_force = 5` (int instead of bool).
*   **Outputs:** An `FFBEngine` instance loaded from this file.
*   **Assertions:**
    *   Assert that the parser does not throw an unhandled exception.
    *   Assert that the invalid fields fall back to their safe default values.

### Test 4: `test_toml_preset_bridge`
*   **Design Rationale:** Proves that user presets are successfully saved into the `[Presets]` table and loaded back correctly, satisfying the "untouched presets" constraint.
*   **Inputs:** Add a custom `Preset` to `Config::presets`. Call `Config::Save()`.
*   **Outputs:** A `test_presets.toml` file.
*   **Assertions:**
    *   Clear `Config::presets`.
    *   Call `Config::LoadPresets()`.
    *   Assert that the custom preset exists in the vector and its values match the original.

## 6. Deliverables
- [ ] `src/ext/toml++/toml.hpp` added to repository.
- [ ] `Makefile` updated with `-Isrc/ext/toml++`.
- [ ] `LICENSE` updated with LZ4, ImGui, and toml++ licenses.
-[ ] `tests/test_toml_config.cpp` created with 4 new tests.
- [ ] `src/core/Config.h` updated (`m_config_path`).
- [ ] `src/core/Config.cpp` rewritten (`Load`, `Save`, `LoadPresets`).
- [ ] `VERSION` incremented.
- [ ] `CHANGELOG_DEV.md` updated.
- [ ] **Implementation Notes:** Update this plan document with any unforeseen issues (e.g., TOML float precision issues requiring `static_cast<float>`) encountered during development.