
This is a classic example of a configuration system that has grown organically over time. As you add more features to the physics engine, the boilerplate required to save, load, apply, and validate those features multiplies. 

Here is a comprehensive review of the current architecture, the bugs/flaws it currently has, and a concrete plan on how to refactor it into a modern, robust, and highly maintainable system.

---

### 1. Should you move to JSON, YAML, or TOML?
**Absolutely, YES.** 

Currently, you are manually serializing and deserializing INI files line-by-line. 
Moving to a library like **JSON (via `nlohmann/json`)** or **TOML (via `toml++`)** will instantly delete ~600 lines of boilerplate code from `Config.cpp`. 

**Benefits of moving to JSON/TOML:**
1. **No more hardcoded C++ presets:** You can move the massive `T300`, `Simagic Alpha`, and `Moza` presets out of `Config.cpp` and into actual `.json` files shipped in a `presets/` folder. If you want to tweak a default preset, you just edit the text file—no C++ recompilation required.
2. **Automatic Parsing:** Libraries handle whitespace, missing keys, type conversions, and malformed files safely.
3. **Community Sharing:** Users can easily share `.json` files on Discord/GitHub.

---

### 2. Bugs & Flaws in the Current Code

Before refactoring, it's important to understand why the current code is fragile:

*   **The "10-Place Update" Anti-Pattern:** Right now, adding a single new FFB parameter (e.g., `tire_wear_feel`) requires you to update the code in **10 different places**: `Preset` struct, `Apply()`, `UpdateFromEngine()`, `Equals()`, `Validate()`, `Parse...Line()`, `Sync...Line()`, `WritePresetFields()`, `Config::Save()`, and `FFBEngine`. This is a massive maintenance burden and guarantees that you will eventually forget one, leading to a bug.
*   **Double Parsing of `config.ini`:** In `main.cpp`, you call `Config::Load()`. Inside `Load()`, you call `LoadPresets()`. 
    * `LoadPresets()` opens `config.ini`, reads it line-by-line, looks for `[Presets]`, and parses them.
    * Then `Load()` opens `config.ini` *again*, reads it line-by-line, and skips the `[Presets]` section. This is inefficient and risks race conditions or file-lock issues.
*   **Fragile Exception Handling:** In `ParsePresetLine`, you wrap the parsing in a generic `catch (...)`. If `std::stof` fails because a user typed `gain=1.0f` (with an 'f' in the INI), it silently fails, logs a generic error, and skips the line.
*   **Tangled Migration Logic:** You are passing `legacy_torque_hack` and `legacy_torque_val` by reference through multiple layers of parsing functions. Migration logic should be completely separated from parsing logic.
*   **Precision Bugs in `Equals()`:** You use a fixed `epsilon = 0.0001f` for all comparisons. This is fine for `gain` (0.0 to 2.0), but for `wheelbase_max_nm` (which can be 50.0), floating-point inaccuracies might cause false positives, making the UI think the preset is "dirty" (showing the `*` asterisk) when it isn't.

---

### 3. The Re-Design Strategy

Here is how you should architect the new system. 

#### Step A: The Data Model (Single Source of Truth)
Keep the `Preset` struct, but use the TOML library to automatically generate the serialization code.

#### Step B: The Migration Pipeline
Instead of migrating data *while* parsing strings, you parse the file into a raw TOML object, migrate the TOML object, and *then* convert it to your C++ struct.
 

#### Step C: Loading Presets (External Files)
Remove the hundreds of lines of hardcoded presets. Ship your app with a folder: `assets/presets/`.

 

#### Step D: Saving and Exporting
 
#### Step E: Fixing `Equals()`
 

### 4. Summary of the Refactored Architecture

**Why this is better:**
* **Zero Parsing Code:** You delete `ParseSystemLine`, `ParsePhysicsLine`, `SyncSystemLine`, etc.
* **Zero Serialization Code:** You delete `WritePresetFields` and the massive `file << "key=" << val` block in `Config::Save`.
* **Safe Migrations.**
* **Maintainability:** When you add a new FFB feature, you add it to the `Preset` struct, add it to the `NLOHMANN_DEFINE_TYPE` macro, and you are done. The UI, saving, loading, and exporting will all support it automatically.

---

**TOML is an excellent choice**, arguably even better than JSON for this specific use case. It was designed specifically for configuration files, it supports comments (which JSON does not), and its syntax is essentially a modernized, standardized version of INI. 

Here are the answers to your questions, followed by a step-by-step roadmap to refactor the system safely.

### 1. Does the advice about JSON still hold for TOML?
**Yes, 100%.** 
Using a robust C++ TOML library (I highly recommend **`toml++`** by marzer, which is header-only and the industry standard for C++17/20) gives you all the same benefits:
*   **No manual string parsing:** It handles whitespace, types (floats vs ints vs booleans), and missing keys automatically.
*   **Tree-based Migration:** You can load the TOML file into a `toml::table` object in memory, check the `app_version`, modify the nodes (e.g., rename a key or scale a value), and *then* map it to your C++ struct.
*   **Easy Serialization:** You just build a `toml::table` in C++ and call `file << table;`.

### 2. Which extension should you use?
You should use **`.toml`**. 
While a TOML parser can technically read most `.ini` files, using `.toml` ensures that modern text editors (VS Code, Notepad++, GitHub) will provide the correct syntax highlighting and validation for your end users. 

For user presets, you can use a folder structure like:
*   `userdata/presets/my_custom_wheel.toml`
*   `assets/presets/t300_default.toml`

### 3. Is moving to TOML all there is to do?
No. If you just swap the parser but keep the current architecture (100+ loose variables in a struct, 500 lines of hardcoded C++ presets, manual `Equals()` comparisons), you will still have a massive maintenance burden. The refactoring needs to address the *data structures* as well as the *file format*.

---

### 4. The Step-by-Step Refactoring Roadmap

To minimize risk and complexity, do not do this all at once. Break it down into these **4 focused patches**.

#### Phase 1: Grouping the Data Structures (No File I/O changes yet)
**Goal:** Fix the "10-place update" anti-pattern and the fragile `Equals()` method.
Currently, `Preset` and `FFBEngine` have over 100 loose variables. Group them into logical sub-structs.

*   **Action:** Create structs like `FrontAxleConfig`, `RearAxleConfig`, `VibrationConfig`, etc.
*   **Code Example:**
    ```cpp
    struct FrontAxleConfig {
        float steering_shaft_gain = 1.0f;
        float understeer_effect = 1.0f;
        float understeer_gamma = 1.0f;
        // ...
        bool operator==(const FrontAxleConfig& other) const = default; // C++20 auto-comparison!
    };

    struct Preset {
        std::string name;
        FrontAxleConfig front_axle;
        VibrationConfig vibration;
        // ...
        
        bool Equals(const Preset& p) const {
            // No more 100-line epsilon comparisons!
            return front_axle == p.front_axle && vibration == p.vibration; 
        }
    };
    ```
*   **Why this is safe:** You aren't touching the file reading/writing yet. You are just organizing memory. This instantly makes adding new features trivial.

#### Phase 2: Integrate `toml++` and Rewrite Main Config I/O
**Goal:** Replace the manual `std::getline` and `std::stof` logic for the main `config.ini` file.
*   **Action:** Add `toml++` to your project. Rename `config.ini` to `config.toml`. Rewrite `Config::Load` and `Config::Save`.
*   **Code Example (Loading):**
    ```cpp
    #include <toml++/toml.h>

    void Config::Load(FFBEngine& engine, const std::string& filename) {
        try {
            toml::table tbl = toml::parse_file(filename);
            
            // Safely read values with fallbacks if missing
            engine.m_general.gain = tbl["General"]["gain"].value_or(1.0f);
            engine.m_invert_force = tbl["General"]["invert_force"].value_or(false);
            
            // Read grouped structs easily
            engine.front_axle.understeer_effect = tbl["FrontAxle"]["understeer"].value_or(1.0f);
            
        } catch (const toml::parse_error& err) {
            Logger::Get().LogFile("TOML Parse Error: %s", err.description().c_str());
        }
    }
    ```
*   **Why this is safe:** You are only replacing the I/O for the main settings. The preset logic remains untouched for now.

Since we are adding toml++, we must make sure that it gets properly installed or included in the project also in the *.yml files like .github\workflows\windows-build-and-test.yml (GitHub Actions) or in the makefile. See how we do this for other external libraries, like LZ4 and ImGui (note that we integrate differently LZ4 and ImGui, see the most appropriate approach for toml++).
We also must update the license information if we are bundling toml++ with our compiled and distributed app. Make sure that our LICENSE file include all necessary information for LZ4, ImGui (if necessary to put such info in the LICENSE file), and toml++.

#### Phase 3: Refactor Preset Loading & Migration Logic
**Goal:** Move the migration logic out of the parsing loop and into a dedicated step.
*   **Action:** Rewrite `ImportPreset`, `ExportPreset`, and `ParsePresetLine` to use TOML. Create a dedicated `MigratePreset(toml::table& tbl)` function.
*   **Code Example (Migration):**
    ```cpp
    void MigratePreset(toml::table& tbl) {
        std::string version = tbl["System"]["app_version"].value_or("0.0.0");

        // Example: Migrate legacy 100Nm hack
        if (IsVersionLessEqual(version, "0.7.66")) {
            if (auto max_ref = tbl["General"]["max_torque_ref"].value<float>()) {
                if (*max_ref > 40.0f) {
                    tbl["General"]["wheelbase_max_nm"] = 15.0f;
                    tbl["General"]["target_rim_nm"] = 10.0f;
                    // Scale gain
                    float old_gain = tbl["General"]["gain"].value_or(1.0f);
                    tbl["General"]["gain"] = old_gain * (15.0f / *max_ref);
                }
            }
        }
        tbl["System"]["app_version"] = LMUFFB_VERSION; // Mark as up-to-date
    }
    ```
*   **Why this is safe:** You separate the *reading of the file* from the *upgrading of the data*. This eliminates the messy `legacy_torque_hack` boolean flags being passed by reference through 5 different functions.

#### Phase 4: Externalize Hardcoded Presets
**Goal:** Delete the 500+ lines of hardcoded C++ presets (`T300`, `Simagic Alpha`, etc.) from `Config.cpp`.
*   **Action:** Create a folder in your repository called `assets/presets/`. Create files like `T300.toml`, `Moza_R21.toml`. 
*   **Action:** Change `Config::LoadPresets()` to simply iterate over the `assets/presets/` directory and the `userdata/presets/` directory, parsing every `.toml` file it finds.
*   **Why this is safe:** By this point, your TOML parsing and migration logic is rock solid. Moving the presets to text files means you (and your users) can tweak default profiles without recompiling the C++ code. It drastically reduces the size and complexity of `Config.cpp`.

### Summary of the Plan
1. **Patch 1:** Group variables into sub-structs. Fix `Equals()` and `UpdateFromEngine()`.
2. **Patch 2:** Add `toml++`. Rewrite `Config::Load` and `Config::Save` for the main config.
3. **Patch 3:** Rewrite Preset Import/Export/Migration using TOML tables.
4. **Patch 4:** Move hardcoded C++ presets into external `.toml` files.

This approach ensures that if something breaks, you know exactly which layer caused it, and your test suite will catch it immediately.