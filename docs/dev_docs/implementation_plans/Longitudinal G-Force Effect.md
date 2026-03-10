# Implementation Plan: Longitudinal G-Force Effect

## 1. Context & Problem Statement
**Goal:** Replace the current tire-load-based Longitudinal Load effect with a kinematics-based Longitudinal G-Force effect, while keeping the UI intuitive for the user.

**Problem:** 
The current implementation calculates longitudinal weight transfer by comparing the real-time front tire load against the static front tire load. In high-downforce vehicles (like Hypercars or GT3s), aerodynamic downforce pushes the car into the track at high speeds, effectively doubling the front tire load. The current math misinterprets this aero-load as massive braking weight transfer, causing the steering wheel to become artificially heavy and stiff while accelerating down straightaways. Furthermore, raw tire load data is highly susceptible to high-frequency noise from track bumps and curb strikes.

## 2. Recap of Architectural Discussions & Design Rationale
During the analysis phase, several alternative approaches were evaluated and discarded in favor of the G-Force method:

*   **Why not use the Front/Rear Load Ratio?**
    *   *Aero Balance:* Cars do not generate downforce equally (e.g., 30% front / 70% rear). As speed increases, the rear load grows faster than the front, which a ratio would misinterpret as acceleration, making the wheel artificially light.
    *   *Pitch Sensitivity:* Braking causes the nose to dive, increasing front aero efficiency. This makes the ratio highly non-linear and speed-dependent.
    *   *Curb Strikes:* Hitting a bump with the front tires causes a massive, instantaneous spike in the ratio, sending violent shocks to the wheel.
*   **Why not gate the effect to the Brake Pedal?**
    *   *Loss of Detail:* We would lose the sensation of the front end getting "light" under hard acceleration out of corners.
    *   *Speed Dependency:* Braking at 300 km/h (with massive aero) would still feel drastically heavier than braking at 80 km/h for the exact same pedal pressure, ruining muscle memory.
    *   *Engine Braking:* Lift-off oversteer cues (weight transfer from coasting) would be lost.
*   **Why keep it as a Multiplier instead of an Additive Force?**
    *   *Directionality:* Longitudinal forces push forward/backward, not left/right. Adding a raw force would violently pull the wheel to one side on a straightaway.
    *   *Aligning Torque:* Real cars feel stiff under braking because the massive front load amplifies the tires' natural aligning torque. A multiplier perfectly replicates this by amplifying the existing road texture and slip angles, making the wheel feel "planted" without artificially steering the car.
*   **Why use Derived Acceleration instead of Raw Acceleration?**
    *   Raw accelerometers (`mLocalAccel.z`) capture every engine vibration and suspension impact. By using the derivative of the car's macroscopic velocity (`mLocalVel.z`), we obtain a mathematically pure, buttery-smooth signal (`m_accel_z_smoothed`) that only registers true chassis acceleration/deceleration.

## 3. Reference Documents
*   `docs/dev_docs/reports/FFBEngine_refactoring_analysis.md` (Previous refactoring context)
*   Log Analyzer Diagnostic Reports (March 10, 2026)

## 4. Codebase Analysis Summary
*   **`src/FFBEngine.cpp` (`calculate_force`)**: This is the core impact zone. The logic currently calculates `long_load_norm` using `ctx.avg_front_load / m_static_front_load`. This must be swapped to use `m_accel_z_smoothed / GRAVITY_MS2`. The call to `update_static_load_reference` must be preserved outside the effect block, as it is required by the Bottoming effect and Dynamic Normalization.
*   **`src/GuiLayer_Common.cpp`**: The UI currently labels this effect "Longitudinal Load". To maintain user familiarity while being technically accurate, we will rename the label to "Longitudinal G-Force" but keep it within the "Load Forces" ImGui tree node.
*   **`src/Tooltips.h`**: The tooltip for `DYNAMIC_WEIGHT` needs to be updated to explain that it uses G-forces and ignores aerodynamics.
*   **`src/Config.h` / `src/Config.cpp`**: We will **retain** the internal variable names (`m_long_load_effect`, `long_load_effect` in INI) to ensure backward compatibility with existing user presets. No migration logic is required.

## 5. FFB Effect Impact Analysis
| Effect | Technical Change | User Perspective (Feel & UI) |
| :--- | :--- | :--- |
| **Longitudinal G-Force** (formerly Long. Load) | Input signal changed from `avg_front_load` to `m_accel_z_smoothed`. | **Massive Improvement:** The wheel will no longer get heavy on straights. Braking feel will be consistent regardless of speed (ignoring aero). Acceleration will correctly make the wheel feel light. Bumps will not cause multiplier spikes. |
| **Suspension Bottoming** | None directly, but relies on `m_static_front_load`. | No change. We ensure `update_static_load_reference` is still called unconditionally. |

## 6. Proposed Changes

### 6.1. `src/FFBEngine.cpp`
*   **Target:** `FFBEngine::calculate_force` (around line 380).
*   **Action:** Replace the `long_load_norm` calculation.
*   **Logic:**
    ```cpp
    // ALWAYS learn static load reference (used by Bottoming and Normalization).
    update_static_load_reference(ctx.avg_front_load, ctx.car_speed, ctx.dt);

    double long_load_factor = 1.0;

    if (m_long_load_effect > 0.0) {
        // Use Derived Longitudinal Acceleration (Z-axis) to isolate weight transfer.
        // LMU Coordinate System: +Z is rearward (deceleration/braking). -Z is forward (acceleration).
        // Normalize: 1G braking = +1.0, 1G acceleration = -1.0
        double long_g = m_accel_z_smoothed / GRAVITY_MS2;
        
        // CLAMPING We clamp the normalized G-force between -1.0G and +1.0G. 
        // Why? Because if you crash into a wall at 200 km/h, the game might output 50.0G. 
        // If we didn't clamp it, the multiplier would become 50x, and the steering wheel 
        // would snap your wrists. We clamp it to 1.0G because that represents the maximum 
        // realistic threshold braking limit for most cars.
        double long_load_norm = std::clamp(long_g, -1.0, 1.0);

        // Apply Transformation
        switch (m_long_load_transform) {
            case LoadTransform::CUBIC: long_load_norm = apply_load_transform_cubic(long_load_norm); break;
            case LoadTransform::QUADRATIC: long_load_norm = apply_load_transform_quadratic(long_load_norm); break;
            case LoadTransform::HERMITE: long_load_norm = apply_load_transform_hermite(long_load_norm); break;
            case LoadTransform::LINEAR: default: break;
        }

        long_load_factor = 1.0 + long_load_norm * (double)m_long_load_effect;
        long_load_factor = std::clamp(long_load_factor, LONG_LOAD_MIN, LONG_LOAD_MAX);
    }
    ```
*   **Design Rationale:** Using `m_accel_z_smoothed` completely decouples the effect from aerodynamic downforce and high-frequency suspension noise.

### 6.2. `src/GuiLayer_Common.cpp`
*   **Target:** `DrawTuningWindow` -> "Load Forces" section.
*   **Action:** Rename the UI label.
*   **Logic:** Change `FloatSetting("Longitudinal Load", ...)` to `FloatSetting("Longitudinal G-Force", ...)`.

### 6.3. `src/Tooltips.h`
*   **Target:** `DYNAMIC_WEIGHT`
*   **Action:** Update text.
*   **Logic:** `"Scales steering weight based on longitudinal G-forces (acceleration/braking).\nHeavier under braking, lighter under acceleration.\nIgnores aerodynamic downforce for consistent feel."`

### 6.4. Parameter Synchronization Checklist
*   *Note: No new parameters are being added. We are reusing `m_long_load_effect` to maintain preset compatibility.*

### 6.5. Version Increment Rule
*   Increment the version number in `VERSION` and `src/Version.h` by the smallest possible increment (e.g., `0.7.161` -> `0.7.162`).

## 7. Test Plan (TDD-Ready)

**Design Rationale:** We must prove that the multiplier responds correctly to G-forces and completely ignores massive tire loads (simulating aero).

*   **Test 1: `test_longitudinal_g_braking`**
    *   *Input:* `m_accel_z_smoothed` = `9.81` (1G braking), `m_long_load_effect` = `1.0` (100%).
    *   *Expected Output:* `long_load_factor` should be `2.0`.
    *   *Assertion:* `ASSERT_NEAR(factor, 2.0, 0.01)`
*   **Test 2: `test_longitudinal_g_acceleration`**
    *   *Input:* `m_accel_z_smoothed` = `-9.81` (1G acceleration), `m_long_load_effect` = `0.5` (50%).
    *   *Expected Output:* `long_load_factor` should be `0.5`.
    *   *Assertion:* `ASSERT_NEAR(factor, 0.5, 0.01)`
*   **Test 3: `test_longitudinal_g_aero_independence`**
    *   *Input:* `m_accel_z_smoothed` = `0.0` (Constant speed), `avg_front_load` = `10000.0` (Massive aero), `m_long_load_effect` = `1.0`.
    *   *Expected Output:* `long_load_factor` should be exactly `1.0` (no weight transfer).
    *   *Assertion:* `ASSERT_EQ(factor, 1.0)`

## 8. Deliverables
- [ ] Update `src/FFBEngine.cpp` with the new G-Force logic.
- [ ] Update `src/GuiLayer_Common.cpp` UI labels.
- [ ] Update `src/Tooltips.h` descriptions.
- [ ] Write and pass the 3 new unit tests in `tests/test_ffb_common.cpp` (or appropriate test file).
- [ ] Increment version numbers.
- [ ] **Implementation Notes:** Update this plan document with any unforeseen issues or plan deviations encountered during implementation.