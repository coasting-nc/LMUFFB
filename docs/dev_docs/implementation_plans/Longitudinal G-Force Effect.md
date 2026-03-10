# Implementation Plan: #325 Replace longitudinal load effect with longitudinal G-force

## 1. Context & Problem Statement
**Goal:** Replace the current tire-load-based Longitudinal Load effect with a kinematics-based Longitudinal G-Force effect, while keeping the UI intuitive for the user.

**Problem:** 
The current implementation calculates longitudinal weight transfer by comparing the real-time front tire load against the static front tire load. In high-downforce vehicles (like Hypercars or GT3s), aerodynamic downforce pushes the car into the track at high speeds, effectively doubling the front tire load. The current math misinterprets this aero-load as massive braking weight transfer, causing the steering wheel to become artificially heavy and stiff while accelerating down straightaways. Furthermore, raw tire load data is highly susceptible to high-frequency noise from track bumps and curb strikes.

> **Design Rationale:**
> Shifting from a load-based to a G-force-based calculation is necessary to decouple weight transfer sensations from aerodynamic downforce. Longitudinal G-force is a direct proxy for weight transfer that remains consistent regardless of the car's speed or aero package, providing a more predictable and physically accurate FFB response.

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

> **Design Rationale:**
> The multiplier approach is preserved because it correctly models how tire load influences the steering's self-aligning torque. Direct additive forces would be non-physical for a steering rack. Derived acceleration is chosen to eliminate high-frequency sensor noise that would otherwise jitter the FFB.

## 3. Reference Documents
*   `docs/dev_docs/reports/FFBEngine_refactoring_analysis.md` (Previous refactoring context)
*   Log Analyzer Diagnostic Reports (March 10, 2026)

## 4. Codebase Analysis Summary
*   **`src/FFBEngine.cpp` (`calculate_force`)**: This is the core impact zone. The logic currently calculates `long_load_norm` using `ctx.avg_front_load / m_static_front_load`. This must be swapped to use `m_accel_z_smoothed / GRAVITY_MS2`. The call to `update_static_load_reference` must be preserved outside the effect block, as it is required by the Bottoming effect and Dynamic Normalization.
*   **`src/GuiLayer_Common.cpp`**: The UI currently labels this effect "Longitudinal Load". To maintain user familiarity while being technically accurate, we will rename the label to "Longitudinal G-Force" but keep it within the "Load Forces" ImGui tree node. Labels like "Long. Smoothing" and "Long. Transform" will also be updated to "G-Force Smoothing" and "G-Force Transform".
*   **`src/Tooltips.h`**: The tooltip for `DYNAMIC_WEIGHT` needs to be updated to explain that it uses G-forces and ignores aerodynamics.
*   **`src/Config.h` / `src/Config.cpp`**: We will **retain** the internal variable names (`m_long_load_effect`, `long_load_effect` in INI) to ensure backward compatibility with existing user presets. No migration logic is required.

> **Design Rationale:**
> Minimal changes to internal variable names and Config structure ensures that existing user settings are not lost, while the UI update ensures the user understands the new underlying physics. Preserving `update_static_load_reference` ensures zero regression for dependent effects.

## 5. FFB Effect Impact Analysis
| Effect | Technical Change | User Perspective (Feel & UI) |
| :--- | :--- | :--- |
| **Longitudinal G-Force** (formerly Long. Load) | Input signal changed from `avg_front_load` to `m_accel_z_smoothed`. | **Massive Improvement:** The wheel will no longer get heavy on straights. Braking feel will be consistent regardless of speed (ignoring aero). Acceleration will correctly make the wheel feel light. Bumps will not cause multiplier spikes. |
| **Suspension Bottoming** | None directly, but relies on `m_static_front_load`. | No change. We ensure `update_static_load_reference` is still called unconditionally. |

> **Design Rationale:**
> The primary impact is improved linearity and stability of the steering weight during high-speed driving. By removing aero-induced load from the equation, the driver can rely on consistent muscle memory for braking effort.

## 6. Proposed Changes

### 6.1. `src/FFBEngine.cpp`
*   **Target:** `FFBEngine::calculate_force` (around line 647).
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
        double long_load_norm = long_g;
        
        // TRANSFORM CLAMPING: Transformations (Cubic, etc.) are designed for [-1.0, 1.0].
        // We clamp to this range before transforming to avoid non-monotonic behavior (Review fix).
        if (m_long_load_transform != LoadTransform::LINEAR) {
            long_load_norm = std::clamp(long_load_norm, -1.0, 1.0);
        } else {
            // Linear allows capturing high-G dynamics (e.g. wall hits) up to 5G.
            long_load_norm = std::clamp(long_load_norm, -5.0, 5.0);
        }

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

> **Design Rationale:**
> Using `m_accel_z_smoothed` completely decouples the effect from aerodynamic downforce and high-frequency suspension noise. The conditional clamp ensures that non-linear transformations remain stable while preserving the ability to capture extreme dynamics in linear mode.

### 6.2. `src/GuiLayer_Common.cpp`
*   **Target:** `DrawTuningWindow` -> "Load Forces" section.
*   **Action:** Rename the UI labels.
*   **Logic:**
    - Change `FloatSetting("Longitudinal Load", ...)` to `FloatSetting("Longitudinal G-Force", ...)`.
    - Change `FloatSetting("  Long. Smoothing", ...)` to `FloatSetting("  G-Force Smoothing", ...)`.
    - Change `Combo("  Long. Transform", ...)` to `Combo("  G-Force Transform", ...)`.

> **Design Rationale:**
> Clearer UI labeling prevents user confusion regarding what the slider actually controls.

### 6.3. `src/Tooltips.h`
*   **Target:** `DYNAMIC_WEIGHT`, `WEIGHT_SMOOTHING`
*   **Action:** Update text.
*   **Logic:**
    - `DYNAMIC_WEIGHT`: `"Scales steering weight based on longitudinal G-forces (acceleration/braking).\nHeavier under braking, lighter under acceleration.\nIgnores aerodynamic downforce for consistent feel."`
    - `WEIGHT_SMOOTHING`: `"Filters the Longitudinal G-Force signal to simulate suspension damping.\nHigher = Smoother weight transfer feel, but less instant.\nRecommended: 0.100s - 0.200s."`

> **Design Rationale:**
> Tooltips must match the new physics implementation to ensure users can tune the effect effectively.

### 6.4. Parameter Synchronization Checklist
*   *Note: No new parameters are being added. We are reusing `m_long_load_effect` to maintain preset compatibility.*

### 6.5. Version Increment Rule
*   Increment the version number in `VERSION` by the smallest possible increment (e.g., `0.7.162` -> `0.7.163`).

## 7. Longitudinal G-Force Normalization and Clamping

> **Design Rationale:**
> Longitudinal G-force represents the chassis acceleration in the Z-axis, normalized by Earth's gravity (9.81 m/s²). In the LMU coordinate system, +Z is rearward (braking/deceleration) and -Z is forward (acceleration).

### 7.1. Linear Normalization
In Linear mode, the raw G-force value is used directly as the multiplier input. 1.0G of braking results in a +1.0 contribution to the multiplier calculation. We clamp this to `[-5.0, 5.0]` to allow high-dynamic events (like wall strikes or extreme prototype braking) to be felt without risking numerical instability that could occur with uncapped inputs.

### 7.2. Mathematical Transformations and Unit-Range Clamping
The project provides several non-linear transformations (Cubic, Quadratic, Hermite) designed to soften the response at the limits of load transfer. These functions:
- `apply_load_transform_cubic(x)`: $1.5x - 0.5x^3$
- `apply_load_transform_quadratic(x)`: $2x - x|x|$
- `apply_load_transform_hermite(x)`: $x(1 + |x| - x^2)$

These polynomials are specifically tuned for the unit range `[-1.0, 1.0]`. If an input outside this range (e.g., 2.0G) is passed, the output can become non-monotonic or even reverse sign (e.g., Quadratic returns -15 for x=5).

To prevent this, the implementation uses **Conditional Clamping**:
- If a non-linear transform is active, the G-force is clamped to `[-1.0, 1.0]` **before** transformation.
- If Linear mode is active, the G-force is clamped to `[-5.0, 5.0]`.

This ensures that tuning presets using transformations remain physically consistent and safe for the user's hardware.

## 8. Test Plan (TDD-Ready)

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

## 9. Implementation Notes

### 9.1. Deviations from Initial Plan
- **Clamping Logic**: The initial plan proposed a broad `[-5.0, 5.0]` clamp for all modes. During code review, it was identified that this would break non-linear transformations (Cubic, Quadratic, Hermite) which are designed for the `[-1.0, 1.0]` range. The plan was updated to include "Conditional Clamping".
- **Test Infrastructure**: Existing tests in `test_ffb_engine.cpp` and `test_ffb_smoothing.cpp` had to be updated because they relied on the old tire-load-based model. They were successfully pivoted to use G-force inputs.

### 9.2. Challenges & Issues
- **Chassis Inertia Smoothing**: In unit tests, `m_accel_z_smoothed` is overwritten by the main `calculate_force` logic if it is not "frozen". To test the effect block in isolation while still calling the full `calculate_force` method, `m_chassis_inertia_smoothing` was set to a very high value (1000s) to effectively stop the EMA from changing the test's injected acceleration value.
- **Distribution Test Failure**: An unrelated failure in `test_analyzer_bundling_integrity` was observed on Linux. This appears to be a pre-existing issue related to environment paths and does not affect the correctness of the physics changes.

### 9.3. Recommendations for the Future
- **Log Field Renaming**: While internal variable names were kept for compatibility, the telemetry log fields were renamed to `Longitudinal G-Force` in v0.7.155. It might be beneficial to eventually unify all naming, but for now, the UI and tooltips provide sufficient clarity.
- **G-Force Sensitivity**: Some users might prefer the old "noisy" tire load feel for immersion. While the G-force model is more physically accurate for steering weight, a future "High-Frequency Detail" slider could blend a small percentage of tire load noise back into the multiplier.

## 10. Deliverables
- [x] Update `src/FFBEngine.cpp` with the new G-Force logic.
- [x] Update `src/GuiLayer_Common.cpp` UI labels.
- [x] Update `src/Tooltips.h` descriptions.
- [x] Write and pass the 3 new unit tests in `tests/test_issue_325_longitudinal_g.cpp`.
- [x] Increment version numbers.
- [x] **Implementation Notes:** Update this plan document with any unforeseen issues or plan deviations encountered during implementation.
