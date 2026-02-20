# Implementation Plan - Issue #155: Implement FFB Strength Normalization Stage 4 - Persistent Storage of Static Load

## Context
In Stage 3, tactile haptics were anchored to the vehicle's static mechanical load, which is learned dynamically when the car travels between 2 and 15 m/s. This requires a "calibration roll" every session. Stage 4 introduces persistent storage for these learned values in `config.ini`, allowing immediate consistent tactile FFB upon leaving the pits in subsequent sessions with the same vehicle.

## Reference Documents
* `docs/dev_docs/implementation_plans/FFB Strength Normalization Plan Stage 4 - Persistent Storage of Static Load.md`
* Stage 3 Implementation: `docs/dev_docs/implementation_plans/plan_154.md`
* GitHub Issue: [https://github.com/coasting-nc/LMUFFB/issues/155](https://github.com/coasting-nc/LMUFFB/issues/155)

## Codebase Analysis Summary
### Current Architecture Overview
* `FFBEngine` manages high-frequency FFB logic.
* `Config` handles persistent settings via `config.ini`.
* `main.cpp` contains the main application loop and the high-priority FFB thread.
* Static load is currently learned every session in `FFBEngine::update_static_load_reference` and stored in `m_static_front_load`.

### Impacted Functionalities
* **Config Storage**: `src/Config.h` and `src/Config.cpp` need a new data structure to store vehicle-specific static loads.
* **INI Parsing/Saving**: `Config::Load` and `Config::Save` need to handle a new `[StaticLoads]` section.
* **FFB Initialization**: `FFBEngine::InitializeLoadReference` needs to check for stored values.
* **FFB Learning Loop**: `FFBEngine::update_static_load_reference` needs to update the config map and signal for a background save.
* **Main Loop**: `src/main.cpp` needs to process save requests to avoid I/O on the FFB thread.

## FFB Effect Impact Analysis
| FFB Effect | Category | Impact | Technical Change | User-Facing Change |
| :--- | :--- | :--- | :--- | :--- |
| **Tactile Textures** | Tactile | **Consistency** | Bypasses learning phase if car is known. | Effects feel consistent immediately upon leaving garage. |
| **Braking/Lockup** | Tactile | **Consistency** | Uses saved static load for normalization. | Consistent vibration intensity from the first braking zone. |

## Proposed Changes

### 1. `src/Config.h`
* Add `static std::map<std::string, double> m_saved_static_loads;`
* Add `static std::atomic<bool> m_needs_save;`

### 2. `src/Config.cpp`
* Initialize static members.
* **`Config::Save`**: Append `[StaticLoads]` section and iterate over the map to save `Vehicle Name=Value` pairs.
* **`Config::Load`**: Implement parsing for `[StaticLoads]` section.

### 3. `src/FFBEngine.cpp`
* **`InitializeLoadReference`**:
  - Search `Config::m_saved_static_loads` for `vehicleName`.
  - If found, set `m_static_front_load = value` and `m_static_load_latched = true`.
* **`update_static_load_reference`**:
  - When latching occurs (`speed >= 15.0`), update `Config::m_saved_static_loads[m_vehicle_name] = m_static_front_load`.
  - Set `Config::m_needs_save = true`.

### 4. `src/main.cpp`
* In the `while (g_running)` loop:
  - Check `if (Config::m_needs_save.exchange(false)) { Config::Save(g_engine); }`.

### Version Increment Rule
* Increment `VERSION` from `0.7.69` to `0.7.70`.

## Test Plan (TDD-Ready)
**File**: `tests/test_ffb_persistent_load.cpp`

1. **`test_config_static_load_parsing`**
   - **Description**: Verify `Config::Load` parses `[StaticLoads]`.
   - **Inputs**: Mock `config.ini` with `[StaticLoads]` section.
   - **Expected**: `Config::m_saved_static_loads` populated correctly.

2. **`test_engine_uses_saved_static_load`**
   - **Description**: Verify `InitializeLoadReference` uses saved data.
   - **Inputs**: Inject value into `Config::m_saved_static_loads`, call `InitializeLoadReference`.
   - **Expected**: `m_static_front_load` matches injected value, `m_static_load_latched` is true.

3. **`test_engine_saves_new_static_load`**
   - **Description**: Verify latching updates config and sets save flag.
   - **Inputs**: Latch a new load in `update_static_load_reference`.
   - **Expected**: `Config::m_saved_static_loads` updated, `Config::m_needs_save` is true.

## Deliverables
* Modified `src/Config.h`, `src/Config.cpp`, `src/FFBEngine.cpp`, `src/main.cpp`.
* New test file `tests/test_ffb_persistent_load.cpp`.
* Updated `VERSION` and `CHANGELOG_DEV.md`.
* Updated `docs/dev_docs/plans/plan_155.md` with implementation notes.

## Implementation Notes
### Unforeseen Issues
* **Thread Safety**: Initial implementation relied on `g_engine_mutex` held during `calculate_force`, but code reviews highlighted the need for more explicit and dedicated protection for the global `m_saved_static_loads` map.

### Plan Deviations
* **Dedicated Mutex**: Added `Config::m_static_loads_mutex` and thread-safe accessors (`SetSavedStaticLoad`, `GetSavedStaticLoad`) to explicitly protect the static loads map, rather than relying solely on the broad `g_engine_mutex`.
* **Parser Robustness**: Enhanced `Config::Load` to handle `[StaticLoads]` section anywhere in the file (before presets), making it less brittle to manual edits.

### Challenges
* Synchronizing state between the 400Hz FFB thread and the main thread's background save without introducing latency or race conditions was solved using an atomic flag and dedicated mutex.

### Recommendations
* Monitor the size of `[StaticLoads]` in `config.ini` over time. If users drive hundreds of different cars, the section could become large, though practically it should remain manageable.
