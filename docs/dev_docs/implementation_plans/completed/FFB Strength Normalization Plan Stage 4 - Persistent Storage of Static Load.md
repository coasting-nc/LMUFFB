# Implementation Plan: Stage 4 - Persistent Storage of Static Load

## Context
In Stage 3, we anchored tactile haptics to the vehicle's static mechanical load, which is learned dynamically when the car travels between 2 and 15 m/s. However, requiring the user to perform this "calibration roll" every time they leave the garage is detrimental to the user experience. 

Stage 4 introduces persistent storage for these learned values. Once a car's static load is successfully latched, it will be saved to `config.ini`. On subsequent sessions with the same vehicle, the application will instantly load the saved static weight and bypass the learning phase, ensuring consistent tactile FFB immediately out of the pits.

## Reference Documents

* `docs\dev_docs\investigations\FFB Strength and Tire Load Normalization2.md`
* `docs\dev_docs\investigations\FFB Strength and Tire Load Normalization3.md`
* Previous stage 1 implementation plan: `docs\dev_docs\implementation_plans\FFB Strength Normalization Plan Stage 1 - Dynamic Normalization for Structural Forces.md`
* Previous stage 1 implementation plan (modified during development): `docs\dev_docs\implementation_plans\plan_152.md`
* Previous stage 2 implementation plan: `docs\dev_docs\implementation_plans\FFB Strength Normalization Plan Stage 2 - Hardware Scaling Redefinition (UI & Config).md`
* Previous stage 2 implementation plan (modified during development): `docs\dev_docs\implementation_plans\plan_153.md`
* Previous stage 3 implementation plan: `docs\dev_docs\implementation_plans\FFB Strength Normalization Plan Stage 3 - Tactile Haptics Normalization (Static Load + Soft-Knee).md`
* Previous stage 3 implementation plan (modified during development): `docs\dev_docs\implementation_plans\plan_154.md`

## Codebase Analysis Summary
*   **Current Architecture:** `FFBEngine::InitializeLoadReference` seeds the `m_static_front_load` with a generic class default (e.g., 4500N for GT3) and relies on `update_static_load_reference` to refine it. `Config::Save` and `Config::Load` handle global settings and presets but do not store vehicle-specific data.
*   **Impacted Functionalities:**
    *   `Config.h` / `Config.cpp`: Needs a new data structure (`std::map`) to hold vehicle names and their static loads, plus parsing/saving logic for a new `` section in the INI file.
    *   `FFBEngine::InitializeLoadReference`: Must check the `Config` map for the current vehicle before falling back to class defaults.
    *   `FFBEngine::update_static_load_reference`: Must write to the `Config` map when latching occurs and signal that a file save is needed.
    *   `main.cpp`: The main thread loop needs to monitor the save request flag and execute `Config::Save()` to avoid performing File I/O on the high-frequency FFB thread.

## FFB Effect Impact Analysis

| FFB Effect | Category | Impact | Technical Change | User-Facing Change |
| :--- | :--- | :--- | :--- | :--- |
| **All Tactile Textures** | Tactile | **Improved Consistency** | Bypasses the 2-15 m/s learning phase if the car is known. | Tactile effects (Road, ABS, Slide) feel perfectly consistent immediately upon leaving the garage, without requiring a smooth "warm-up" roll. |

## Proposed Changes

### 1. `src/Config.h`
Add the storage map and an atomic flag to request background saves.
```cpp
#include <map>
#include <atomic>

class Config {
public:
    // ... existing code ...
    
    // Persistent storage for vehicle static loads
    static std::map<std::string, double> m_saved_static_loads;
    
    // Flag to request a save from the main thread (avoids File I/O on FFB thread)
    static std::atomic<bool> m_needs_save;
};
```

### 2. `src/Config.cpp`
**A. Initialization:**
```cpp
std::map<std::string, double> Config::m_saved_static_loads;
std::atomic<bool> Config::m_needs_save{false};
```

**B. Update `Config::Save`:**
Write the map to the file just before the `` section.
```cpp
// Inside Config::Save, right before file << "\n\n";
file << "\n\n";
for (const auto& pair : m_saved_static_loads) {
    file << pair.first << "=" << pair.second << "\n";
}
```

**C. Update `Config::Load`:**
Modify the parsing loop to handle the new `` section.
```cpp
bool in_static_loads = false;
while (std::getline(file, line)) {
    line.erase(0, line.find_first_not_of(" \t\r\n"));
    if (line.empty() || line == ';') continue;
    
    if (line == '") {
            in_static_loads = true;
            continue;
        } else if (line == "") {
            break; // Top-level settings and static loads end here
        }
    }

    std::istringstream is_line(line);
    std::string key;
    if (std::getline(is_line, key, '=')) {
        std::string value;
        if (std::getline(is_line, value)) {
            if (in_static_loads) {
                try {
                    m_saved_static_loads = std::stod(value);
                } catch(...) {}
            } else {
                // ... existing parsing logic ...
            }
        }
    }
}
```

### 3. `src/FFBEngine.cpp`
**A. Update `InitializeLoadReference`:**
Check for saved data before falling back to defaults.
```cpp
void FFBEngine::InitializeLoadReference(const char* className, const char* vehicleName) {
    std::string vName = vehicleName ? vehicleName : "Unknown";
    
    // Check if we already have a saved static load for this specific car
    auto it = Config::m_saved_static_loads.find(vName);
    if (it != Config::m_saved_static_loads.end()) {
        m_static_front_load = it->second;
        m_static_load_latched = true; // Skip the 2-15 m/s learning phase
        std::cout << " Loaded persistent static load for " << vName << ": " << m_static_front_load << "N\n";
    } else {
        // Fallback to class defaults and require learning
        ParsedVehicleClass vclass = ParseVehicleClass(className, vehicleName);
        m_auto_peak_load = GetDefaultLoadForClass(vclass);
        m_static_front_load = m_auto_peak_load * 0.5;
        m_static_load_latched = false; 
        std::cout << " No saved load for " << vName << ". Learning required.\n";
    }
}
```

**B. Update `update_static_load_reference`:**
Save the value to the map and trigger a file save when latching occurs.
```cpp
// Inside update_static_load_reference, when latching condition is met:
} else if (speed >= 15.0 && m_static_front_load > 1000.0 && !m_static_load_latched) {
    m_static_load_latched = true;
    
    // Save to config map
    std::string vName = m_vehicle_name;
    if (vName != "Unknown" && vName != "") {
        Config::m_saved_static_loads = m_static_front_load;
        Config::m_needs_save = true; // Flag main thread to write to disk
        std::cout << " Latched and saved static load for " << vName << ": " << m_static_front_load << "N\n";
    }
}
```

### 4. `src/main.cpp`
Process the save request in the low-frequency main loop to protect the 400Hz FFB thread from File I/O latency.
```cpp
// Inside the main while (g_running) loop:
while (g_running) {
    GuiLayer::Render(g_engine);
    
    // Process background save requests from the FFB thread
    if (Config::m_needs_save.exchange(false)) {
        Config::Save(g_engine);
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
}
```

### Initialization Order Analysis
*   `Config::m_saved_static_loads` is a static `std::map` initialized empty.
*   It is populated during `Config::Load()` at application startup.
*   It is read by `FFBEngine::InitializeLoadReference` when the game connects and telemetry begins.
*   Thread Safety: `FFBEngine` reads/writes the map while holding `g_engine_mutex`. `Config::Save` also locks `g_engine_mutex`. Therefore, concurrent access between the FFB thread and the Main thread is safely synchronized.

### Version Increment Rule
*   Increment the version number in `VERSION` and `src/Version.h` by the **smallest possible increment** (e.g., `0.7.66` -> `0.7.67`).

## Test Plan (TDD-Ready)

**File:** `tests/test_ffb_persistent_load.cpp` (New file)
**Expected Test Count:** Baseline + 3 new tests.

**1. `test_config_static_load_parsing`**
*   **Description:** Verifies that `Config::Load` correctly parses the `` section without breaking ``.
*   **Inputs:** Create a mock `config.ini` containing `\nFerrari 488 GTE=4200.5\n\n...`. Call `Config::Load`.
*   **Expected Output:** `Config::m_saved_static_loads` equals `4200.5`.

**2. `test_engine_uses_saved_static_load`**
*   **Description:** Verifies that `InitializeLoadReference` applies the saved load and sets the latched flag.
*   **Inputs:** Inject `"Porsche 911 RSR" = 4100.0` into `Config::m_saved_static_loads`. Call `InitializeLoadReference("GTE", "Porsche 911 RSR")`.
*   **Expected Output:** `m_static_front_load` equals `4100.0` and `m_static_load_latched` is `true`.

**3. `test_engine_saves_new_static_load`**
*   **Description:** Verifies that completing the learning phase updates the map and triggers the save flag.
*   **Inputs:** `m_vehicle_name` = "Oreca 07". `m_static_load_latched` = `false`. Call `update_static_load_reference` with `load=3800.0`, `speed=20.0`.
*   **Expected Output:** `m_static_load_latched` becomes `true`. `Config::m_saved_static_loads` equals `3800.0`. `Config::m_needs_save` is `true`.

## Deliverables
- Update `src/Config.h` and `src/Config.cpp` with the map, atomic flag, and INI parsing/saving logic.
- Update `src/FFBEngine.cpp` to read from and write to the persistent map.
- Update `src/main.cpp` to handle the asynchronous save request.
- Create `tests/test_ffb_persistent_load.cpp` and implement the 3 test cases.
- Update `VERSION` and `src/Version.h`.
- **Implementation Notes:** Update this plan document with "Unforeseen Issues", "Plan Deviations", "Challenges", and "Recommendations" upon completion.
