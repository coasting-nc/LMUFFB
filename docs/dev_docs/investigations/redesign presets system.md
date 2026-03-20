


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

Here is how you should architect the new system. I highly recommend using **`nlohmann/json`** (a single-header C++ library) because it supports automatic struct mapping.

#### Step A: The Data Model (Single Source of Truth)
Keep the `Preset` struct, but use the JSON library's macro to automatically generate the serialization code.

```cpp
// Config.h
#include <nlohmann/json.hpp>

struct Preset {
    std::string name = "Unnamed";
    std::string app_version = LMUFFB_VERSION;
    bool is_builtin = false;

    // Physics parameters
    float gain = 1.0f;
    float understeer = 1.0f;
    float understeer_gamma = 1.0f;
    // ... all other parameters ...

    // 1. Validation (Clamp values to safe ranges)
    void Validate(); 
    
    // 2. Apply to Engine
    void Apply(FFBEngine& engine) const;
    
    // 3. Capture from Engine
    void UpdateFromEngine(const FFBEngine& engine);
};

// THIS MACRO IS MAGIC. It automatically generates to_json() and from_json() 
// for the Preset struct. If a key is missing in the JSON, it keeps the C++ default!
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Preset, 
    name, app_version, gain, understeer, understeer_gamma /*, list all vars here */
)
```

#### Step B: The Migration Pipeline
Instead of migrating data *while* parsing strings, you parse the file into a raw JSON object, migrate the JSON object, and *then* convert it to your C++ struct.

```cpp
// Config.cpp
void MigratePresetJson(nlohmann::json& j) {
    std::string version = j.value("app_version", "0.0.0");

    // Migration: Legacy 100Nm hack (Issue #211)
    if (IsVersionLessEqual(version, "0.7.66")) {
        if (j.contains("max_torque_ref")) {
            float old_val = j["max_torque_ref"];
            if (old_val > 40.0f) {
                j["wheelbase_max_nm"] = 15.0f;
                j["target_rim_nm"] = 10.0f;
                j["gain"] = j.value("gain", 1.0f) * (15.0f / old_val);
            }
        }
    }

    // Migration: Reset SoP Smoothing (Issue #37)
    if (IsVersionLessEqual(version, "0.7.146")) {
        j["sop_smoothing"] = 0.0f;
    }

    // Update version to current
    j["app_version"] = LMUFFB_VERSION;
}
```

#### Step C: Loading Presets (External Files)
Remove the hundreds of lines of hardcoded presets. Ship your app with a folder: `assets/presets/`.

```cpp
void Config::LoadPresets() {
    presets.clear();

    // 1. Load Built-in Presets from disk
    for (const auto& entry : std::filesystem::directory_iterator("assets/presets/")) {
        if (entry.path().extension() == ".json") {
            std::ifstream file(entry.path());
            nlohmann::json j;
            file >> j;

            MigratePresetJson(j); // Upgrade old formats
            
            Preset p = j.get<Preset>(); // MAGIC: JSON converts directly to C++ struct
            p.is_builtin = true;
            p.Validate();
            presets.push_back(p);
        }
    }

    // 2. Load User Presets from userdata/presets/
    for (const auto& entry : std::filesystem::directory_iterator("userdata/presets/")) {
        // Same logic, but p.is_builtin = false;
    }
}
```

#### Step D: Saving and Exporting
Exporting a preset becomes literally three lines of code.

```cpp
void Config::ExportPreset(int index, const std::string& filename) {
    if (index < 0 || index >= presets.size()) return;

    nlohmann::json j = presets[index]; // MAGIC: C++ struct converts to JSON
    
    std::ofstream file(filename);
    file << j.dump(4); // Write with 4-space indentation
    
    Logger::Get().LogFile("[Config] Exported preset to %s", filename.c_str());
}
```

#### Step E: Fixing `Equals()`
Instead of manually comparing 100 floats, you can either:
1. Compare the generated JSON objects: `return nlohmann::json(*this) == nlohmann::json(other);` (Note: floating point serialization might cause minor diffs).
2. Use `memcmp` if you organize your struct carefully (no strings inside the physics block).
3. Keep the manual comparison, but group your parameters into sub-structs (e.g., `struct BrakingParams`, `struct TextureParams`) and overload `operator==` for them.

### 4. Summary of the Refactored Architecture

1. **`config.json` (Global Settings):** Contains window positions, selected device GUID, active preset name, and global toggles.
2. **`presets/` directory:** Contains one `.json` file per preset.
3. **`Preset` struct:** Acts as the pure data container.
4. **`FFBEngine`:** Only cares about physics. It no longer needs to know how to parse strings.

**Why this is better:**
* **Zero Parsing Code:** You delete `ParseSystemLine`, `ParsePhysicsLine`, `SyncSystemLine`, etc.
* **Zero Serialization Code:** You delete `WritePresetFields` and the massive `file << "key=" << val` block in `Config::Save`.
* **Safe Migrations:** You manipulate a dynamic JSON tree before it touches your strict C++ types.
* **Maintainability:** When you add a new FFB feature, you add it to the `Preset` struct, add it to the `NLOHMANN_DEFINE_TYPE` macro, and you are done. The UI, saving, loading, and exporting will all support it automatically.