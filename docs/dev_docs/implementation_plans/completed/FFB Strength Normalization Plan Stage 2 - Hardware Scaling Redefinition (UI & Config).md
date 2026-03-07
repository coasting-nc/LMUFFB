# Implementation Plan: Stage 2 - Hardware Scaling Redefinition (UI & Config)

## Context
Following the implementation of dynamic normalization for structural forces (Stage 1), the legacy `m_max_torque_ref` setting is now mathematically obsolete and actively confusing for users. Because it previously acted as a clipping avoidance hack (often set to 100 Nm), it broke the 1:1 scaling of tactile effects. 

Stage 2 replaces `m_max_torque_ref` with a "Physical Target" model consisting of two explicit settings: `m_wheelbase_max_nm` (the physical limit of the user's hardware) and `m_target_rim_nm` (the desired peak force at the steering rim). This completely decouples the user's hardware capabilities from their subjective strength preferences, and allows tactile textures to be rendered in absolute Newton-meters.

## Reference Documents

* `docs\dev_docs\investigations\FFB Strength and Tire Load Normalization2.md`
* `docs\dev_docs\investigations\FFB Strength and Tire Load Normalization3.md`
* Previous stage 1 implementation plan: `docs\dev_docs\implementation_plans\FFB Strength Normalization Plan Stage 1 - Dynamic Normalization for Structural Forces.md`
* Previous stage 1 implementation plan (modified during development): `docs\dev_docs\implementation_plans\plan_152.md`


## Codebase Analysis Summary
*   **Current Architecture:** `m_max_torque_ref` is used to calculate `ctx.decoupling_scale` (`max_torque_ref / 20.0`). This scale is multiplied into almost every FFB effect. Finally, the total sum is divided by `max_torque_ref`. This causes textures to scale as a fixed percentage of the wheelbase, meaning a 2 Nm road texture feels like 1 Nm on a 10 Nm wheelbase, but 2.5 Nm on a 25 Nm wheelbase.
*   **Impacted Functionalities:**
    *   `Config.h` / `Config.cpp`: Preset parsing, saving, loading, and legacy migration.
    *   `GuiLayer_Common.cpp`: The FFB Tuning UI.
    *   `FFBEngine.cpp`: `ctx.decoupling_scale` will be completely removed. All artificial effects will be calculated in absolute Nm. The final summation will map structural forces using `target_rim_nm / wheelbase_max_nm` and texture forces using `1.0 / wheelbase_max_nm`.

## FFB Effect Impact Analysis

| FFB Effect | Category | Impact | Technical Change | User-Facing Change |
| :--- | :--- | :--- | :--- | :--- |
| **Base Steering / SoP** | Structural | **Modified** | Mapped via `target_rim_nm / wheelbase_max_nm`. | Users now explicitly set how heavy they want the wheel to feel (e.g., 10 Nm) regardless of their wheelbase capacity. |
| **All Textures (Road, ABS, etc.)** | Tactile | **Modified** | `decoupling_scale` removed. Mapped via `1.0 / wheelbase_max_nm`. | Textures now output absolute physical torque. A 2 Nm ABS pulse will feel exactly like 2 Nm on *any* wheelbase, preventing violent shaking on high-end Direct Drive wheels. |

## Proposed Changes

### 1. Parameter Synchronization Checklist
**Remove:** `max_torque_ref` / `m_max_torque_ref`
**Add:** `wheelbase_max_nm` / `m_wheelbase_max_nm`
**Add:** `target_rim_nm` / `m_target_rim_nm`

* **FFBEngine.h**: Replace `float m_max_torque_ref;` with `float m_wheelbase_max_nm;` and `float m_target_rim_nm;`.
* **Config.h (Preset struct)**: Replace `max_torque_ref` with `wheelbase_max_nm = 15.0f;` and `target_rim_nm = 10.0f;`.
* **Config.h (`Preset::Apply`)**: Map to engine variables with safety clamps (`std::max(1.0f, ...)`).
* **Config.h (`Preset::UpdateFromEngine`)**: Capture from engine variables.
* **Config.h (`Preset::Validate`)**: Add clamping validation for both new variables.
* **Config.h (`Preset::Equals`)**: Update equality checks.
* **Config.cpp (`WritePresetFields`)**: Write new keys, remove old key.
* **Config.cpp (`ParsePresetLine` & `Load`)**: Add parsing for new keys. Add **Migration Logic** for `max_torque_ref`:
    ```cpp
    else if (key == "max_torque_ref") {
        float old_val = std::stof(value);
        if (old_val > 40.0f) {
            // Likely the 100Nm clipping hack. Reset to safe DD defaults.
            current_preset.wheelbase_max_nm = 15.0f;
            current_preset.target_rim_nm = 10.0f;
        } else {
            // User actually tuned it to their wheelbase
            current_preset.wheelbase_max_nm = old_val;
            current_preset.target_rim_nm = old_val;
        }
    }
    ```
* **Config.cpp (`LoadPresets`)**: Update all hardcoded presets (T300, Simagic, Moza) to use the new variables. (e.g., T300: wheelbase=4.0, target=4.0. Simagic: wheelbase=15.0, target=10.0).

### 2. `src/GuiLayer_Common.cpp`
Replace the `Max Torque Ref` slider with two new sliders:
```cpp
FloatSetting("Wheelbase Max Torque", &engine.m_wheelbase_max_nm, 1.0f, 50.0f, "%.1f Nm", "The absolute maximum physical torque your wheelbase can produce (e.g., 15.0 for Simagic Alpha, 4.0 for T300).");
FloatSetting("Target Rim Torque", &engine.m_target_rim_nm, 1.0f, 50.0f, "%.1f Nm", "The maximum force you want to feel in your hands during heavy cornering.");
```
*Note: Update the `FormatDecoupled` lambda to use the new variables if it is still needed for UI display.*

### 3. `src/FFBEngine.cpp`
**A. Remove Decoupling Scale:**
Remove `ctx.decoupling_scale` entirely from `FFBCalculationContext` and delete its calculation.

**B. Update Effect Calculations:**
Remove `* ctx.decoupling_scale` from *every* effect calculation in `FFBEngine.cpp` (e.g., `calculate_sop_lateral`, `calculate_gyro_damping`, `calculate_road_texture`, etc.). All effects will now natively calculate their contribution in absolute Newton-meters.

**C. Update Final Summation and Scaling:**
```cpp
// --- 6. SUMMATION ---
double structural_sum = output_force + ctx.sop_base_force + ctx.rear_torque + ctx.yaw_force + ctx.gyro_force + ctx.soft_lock_force;
structural_sum *= ctx.gain_reduction_factor;

// Normalize structural forces to range
double norm_structural = structural_sum * m_smoothed_structural_mult;

// Textures are already in absolute Nm
double texture_sum_nm = ctx.road_noise + ctx.slide_noise + ctx.spin_rumble + ctx.bottoming_crunch + ctx.abs_pulse_force + ctx.lockup_rumble;

// --- 7. OUTPUT SCALING ---
// Map structural to the target rim torque, then divide by wheelbase max to get DirectInput %
double di_structural = norm_structural * ((double)m_target_rim_nm / (double)m_wheelbase_max_nm);

// Map absolute texture Nm directly to the wheelbase max
double di_texture = texture_sum_nm / (double)m_wheelbase_max_nm;

double norm_force = (di_structural + di_texture) * m_gain;
```

### Initialization Order Analysis
*   No cross-header circular dependencies introduced.
*   `m_wheelbase_max_nm` and `m_target_rim_nm` are primitive floats and will be initialized via `Preset::ApplyDefaultsToEngine()`.

### Version Increment Rule
*   Increment the version number in `VERSION` and `src/Version.h` by the **smallest possible increment** (e.g., `0.7.64` -> `0.7.65`).

## Test Plan (TDD-Ready)

**File:** `tests/test_ffb_hardware_scaling.cpp` (New file)
**Expected Test Count:** Baseline + 3 new tests.

**1. `test_hardware_scaling_structural`**
*   **Description:** Verifies that structural forces are correctly mapped to the wheelbase based on the target rim torque.
*   **Inputs:** `m_session_peak_torque` = 30.0 (multiplier = 1/30). `structural_sum` = 30.0 Nm. `m_target_rim_nm` = 10.0. `m_wheelbase_max_nm` = 20.0.
*   **Expected Output:** `di_structural` should be `(30 * (1/30)) * (10 / 20) = 0.5`.

**2. `test_hardware_scaling_textures`**
*   **Description:** Verifies that tactile textures output absolute Nm regardless of the target rim torque.
*   **Inputs:** `texture_sum_nm` = 5.0 Nm. `m_target_rim_nm` = 10.0. `m_wheelbase_max_nm` = 20.0.
*   **Expected Output:** `di_texture` should be `5.0 / 20.0 = 0.25`. (Changing `m_target_rim_nm` to 15.0 should still result in `0.25`).

**3. `test_config_migration_max_torque`**
*   **Description:** Verifies that legacy `max_torque_ref` values are safely migrated.
*   **Inputs:** Call `Config::ParsePresetLine` with `max_torque_ref=100.0`.
*   **Expected Output:** The resulting preset has `wheelbase_max_nm == 15.0f` and `target_rim_nm == 10.0f`.
*   **Inputs 2:** Call `Config::ParsePresetLine` with `max_torque_ref=20.0`.
*   **Expected Output 2:** The resulting preset has `wheelbase_max_nm == 20.0f` and `target_rim_nm == 20.0f`.

## Deliverables
- Update `src/FFBEngine.h` and `src/Config.h` with new parameters.
- Update `src/Config.cpp` with parsing, saving, and migration logic.
- Update `src/GuiLayer_Common.cpp` to replace the UI sliders.
- Update `src/FFBEngine.cpp` to remove `decoupling_scale` and implement the new absolute Nm summation math.
- Create `tests/test_ffb_hardware_scaling.cpp` and implement the 3 test cases.
- Update `VERSION` and `src/Version.h`.
- **Implementation Notes:** Update this plan document with "Unforeseen Issues", "Plan Deviations", "Challenges", and "Recommendations" upon completion.
