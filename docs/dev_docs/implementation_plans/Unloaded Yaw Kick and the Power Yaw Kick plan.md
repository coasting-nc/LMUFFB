# Implementation Plan: Context-Aware Yaw Kicks (Unloaded & Power) - Issue #322

## Context
The current "General Yaw Kick" effect is a compromise. If made sensitive enough to catch slides early, it becomes too noisy over curbs and during normal driving. If smoothed and thresholded to reduce noise, it reacts too late to save the car from sudden oversteer. 
This plan introduces two new context-aware, hyper-sensitive yaw kicks that only trigger when the car is physically vulnerable to spinning:
1. **Unloaded Yaw Kick**: Triggered by rear load drop (braking/lift-off oversteer).
2. **Power Yaw Kick**: Triggered by throttle application and rear wheel spin (traction loss / power oversteer).

## Design Rationale
*   **Leading vs. Lagging Indicators**: Yaw acceleration and longitudinal slip ratio are leading indicators of a spin. Yaw acceleration and longitudinal slip ratio are leading indicators. By gating the FFB on leading indicators, we achieve zero-latency warnings.
*   **Transient Shaping (Jerk)**: Belt-driven wheelbases act as mechanical low-pass filters due to belt stretch and stiction. Injecting `yaw_jerk` (the derivative of yaw acceleration) provides an instantaneous "punch" to overcome this inertia. For Direct Drive users, this translates to a sharp, tactile "snap" that alerts the hands before the slide fully develops.
*   **Gamma Curve**: A slide's yaw acceleration grows exponentially. A linear FFB response means the early milliseconds of a slide are too weak to feel. A gamma curve (< 1.0) amplifies these early, small signals, making the onset of the slide instantly perceptible.
*   **Traction Control Analogy**: Exposing `m_power_slip_threshold` allows users to tune the Power Yaw Kick like a real-world Traction Control system, deciding exactly how much slip is allowed before the FFB intervenes.
*   **Context-Aware Gating**: strictly gating these effects prevents them from adding noise during normal driving or over curbs.

## Reference Documents
*   Unloaded Yaw Kick: `docs/dev_docs/investigations/Unloaded Yaw Kick (braking).md`
*   Power Yaw Kick : `docs/dev_docs/investigations/Power Yaw Kick (or Traction-Loss Yaw Kick).md`
*   Issue #322: Implement Context-Aware Yaw Kicks: "Unloaded Yaw Kick" and "Power Yaw Kick"

## Codebase Analysis Summary
*   **`FFBEngine.h`**: Needs new state variables (`m_static_rear_load`, `m_prev_raw_yaw_accel`, `m_yaw_accel_seeded`) and 10 new configuration parameters for the two new effects.
*   **`GripLoadEstimation.cpp`**: `update_static_load_reference` currently only tracks front load. It must be updated to track and latch `m_static_rear_load` simultaneously.
*   **`FFBEngine.cpp`**: 
    *   `ctx.avg_rear_load` calculation must be moved earlier in `calculate_force` so it can be passed to `update_static_load_reference`.
    *   `calculate_sop_lateral` will be refactored to calculate the three distinct yaw kicks (General, Unloaded, Power) and blend them using a maximum-absolute-value approach.
*   **`Config.h` / `Config.cpp`**: Parameter synchronization for the 10 new settings.
*   **`GuiLayer_Common.cpp`**: UI additions under the "Rear Axle (Oversteer)" section.

**Design Rationale for Impact Zone**: Modifying `calculate_sop_lateral` keeps all oversteer-related logic centralized. Moving `ctx.avg_rear_load` earlier in the pipeline ensures that static load learning and kinematic fallbacks are applied consistently before any effects consume the load data.

## FFB Effect Impact Analysis

| Effect | Technical Changes | User-Facing Changes |
| :--- | :--- | :--- |
| **General Yaw Kick** | Remains unchanged but will now act as the "baseline" rotation feel. | Can now be tuned for smooth, mid-corner balance without worrying about catching sudden snaps. |
| **Unloaded Yaw Kick** | New calculation gated by `rear_load_drop`. Injects `yaw_jerk` and applies a gamma curve. | Users will feel a sharp "snap" the millisecond the rear steps out under heavy braking or lift-off. |
| **Power Yaw Kick** | New calculation gated by `throttle` and `max_rear_spin`. Injects `yaw_jerk` and applies a gamma curve. | Users will feel an instant warning when applying too much throttle, tuned via a TC-style slip target slider. |

**Design Rationale**: By separating Yaw Kick into three context-aware buckets, we solve the classic sim-racing FFB dilemma. The wheel remains calm and smooth 95% of the time, but the *instant* the driver touches the brakes or mashes the throttle, the FFB engine dynamically lowers its thresholds and prepares to deliver a lightning-fast, zero-latency kick.

## Proposed Changes

### Step 1: Update `src/FFBEngine.h`
Add new configuration parameters and state variables:
```cpp
// Unloaded Yaw Kick (Braking/Lift-off)
float m_unloaded_yaw_gain = 0.0f;
float m_unloaded_yaw_threshold = 0.2f;
float m_unloaded_yaw_sens = 1.0f;
float m_unloaded_yaw_gamma = 0.5f;
float m_unloaded_yaw_punch = 0.05f;

// Power Yaw Kick (Acceleration)
float m_power_yaw_gain = 0.0f;
float m_power_yaw_threshold = 0.2f;
float m_power_slip_threshold = 0.10f;
float m_power_yaw_gamma = 0.5f;
float m_power_yaw_punch = 0.05f;

// State variables
double m_static_rear_load = 0.0;
double m_prev_derived_yaw_accel = 0.0;
bool m_yaw_accel_seeded = false;

// Update signature
void update_static_load_reference(double current_front_load, double current_rear_load, double speed, double dt);
```
**Design Rationale**: Adding these to `FFBEngine` provides the necessary state for cross-frame derivative calculations (jerk) and car-specific load learning. Using `float` for parameters and `double` for physics state maintains precision where it matters.

### Step 2: Update `src/GripLoadEstimation.cpp`
Implement rear load learning in `update_static_load_reference`:
*   Apply the same 2-15 m/s learning window to the rear load.
*   Latch both loads simultaneously.
*   Save/Load `m_static_rear_load` using `Config::SetSavedStaticLoad(vName + "_rear", m_static_rear_load)`.
*   Handle `m_static_rear_load` initialization in `InitializeLoadReference`.
**Design Rationale**: Expanding the static load learning to the rear axle allows for a "Rear Unload Factor" that automatically accounts for aero and weight transfer without needing manual car-by-car tuning.

### Step 3: Update `src/FFBEngine.cpp`
*   **In `calculate_force`**: Move the calculation of `ctx.avg_rear_load` (including kinematic fallbacks) to follow `ctx.avg_front_load`. Pass both to `update_static_load_reference`.
*   **In `calculate_sop_lateral`**:
    *   Calculate `yaw_jerk = (derived_yaw_accel - m_prev_derived_yaw_accel) / ctx.dt`.
    *   Calculate `general_yaw_force` (existing logic).
    *   Calculate `unloaded_yaw_force` gated by `rear_load_drop` and `m_unloaded_yaw_sens`.
    *   Calculate `power_yaw_force` gated by `throttle` and `max_rear_spin` relative to `m_power_slip_threshold`.
    *   Implement Gamma curve and Punch (Jerk) for both new kicks.
    *   Blend kicks: `ctx.yaw_force = max_abs_sign(general, unloaded, power)`.
**Design Rationale**: Moving rear load calculation earlier ensures that all effects have access to consistent, fallback-safe load data. Blending kicks using `max_abs` ensures that the most critical warning always reaches the driver without additive stacking that could exceed safety limits.

### Step 4: Update Configuration Handling (`src/Config.h` & `src/Config.cpp`)
Synchronize the 10 new parameters:
*   [ ] Declaration in `Preset` struct (`Config.h`) with default values.
*   [ ] Add fluent setters to `Preset`.
*   [ ] Entry in `Preset::Apply()` (with safety clamping).
*   [ ] Entry in `Preset::UpdateFromEngine()`.
*   [ ] Entry in `Preset::Equals()`.
*   [ ] Entry in `Config::Save()` and `WritePresetFields()`.
*   [ ] Entry in `Config::Load()` and `ParsePresetLine()` (with migration/validation logic).
*   [ ] Update built-in presets (T300, GT3 DD, LMPx, GM).
**Design Rationale**: Maintaining strict parameter synchronization across the Config layer ensures that user settings are persisted correctly and that built-in presets provide a sane starting point for the new effects.

### Step 5: Update UI and Tooltips (`src/GuiLayer_Common.cpp` & `src/Tooltips.h`)
*   Add tooltips for all 10 new settings in `src/Tooltips.h`.
*   Add UI sliders for the new settings under the "Rear Axle (Oversteer)" section in `src/GuiLayer_Common.cpp`, grouped into "Unloaded Yaw Kick (Braking)" and "Power Yaw Kick (Acceleration)".
**Design Rationale**: Providing clear tooltips and organized UI groups helps users understand the context-aware nature of the new yaw kicks and how to tune them effectively.

### Step 6: Versioning and Build Verification
*   Increment `VERSION` and `src/Version.h.in` to `0.7.164`.
*   Run `cmake --build build` to ensure the project still compiles after configuration changes.
**Design Rationale**: Version increments track feature additions, and build verification ensures that header changes didn't introduce circular dependencies.

### Step 7: Complete pre-commit steps
*   Complete pre-commit steps to ensure proper testing, verification, review, and reflection are done.

### Step 8: Final Test Execution
*   Run all relevant tests using `./build/tests/run_combined_tests` to ensure the new yaw kick logic works correctly and no regressions were introduced.

## Test Plan (TDD-Ready)

**Design Rationale**: These tests ensure that the gating logic correctly isolates the effects based on driver intent (throttle/brake) and physical state (load/slip), and that the transient shaper (jerk) amplifies the signal as expected.

1.  **`test_unloaded_yaw_kick_activation`**
    *   *Description*: Simulates heavy braking with a sudden yaw spike.
    *   *Inputs*: High Z deceleration, low rear load, zero throttle, sudden `mLocalRot.y` change.
    *   *Expected Output*: `unloaded_yaw_force` dominates the final `ctx.yaw_force`.
    *   *Assertion*: `std::abs(ctx.yaw_force) > std::abs(general_yaw_force)`.

2.  **`test_power_yaw_kick_activation`**
    *   *Description*: Simulates high throttle with rear wheel spin and a sudden yaw spike.
    *   *Inputs*: High throttle (1.0), high rear slip ratio (> `m_power_slip_threshold`), sudden `mLocalRot.y` change.
    *   *Expected Output*: `power_yaw_force` dominates the final `ctx.yaw_force`.
    *   *Assertion*: `std::abs(ctx.yaw_force) > std::abs(general_yaw_force)`.

3.  **`test_yaw_jerk_punch`**
    *   *Description*: Verifies the transient shaper amplifies the initial attack of a slide.
    *   *Inputs*: Two consecutive frames. Frame 1: zero yaw. Frame 2: small yaw acceleration.
    *   *Expected Output*: With `m_power_yaw_punch > 0`, the resulting force is significantly higher than with `punch = 0`.
    *   *Assertion*: `force_with_punch > force_without_punch * 1.5`.

4.  **`test_yaw_kick_blending`**
    *   *Description*: Verifies the `std::max` blending logic preserves the sign and selects the strongest effect.
    *   *Inputs*: Artificial scenario where both Unloaded and Power gates are partially open, but Power demands more force.
    *   *Expected Output*: Final force exactly equals the Power force.
    *   *Assertion*: `ctx.yaw_force == power_yaw_force`.

5.  **`test_static_rear_load_tracking`**
    *   *Description*: Verifies `m_static_rear_load` is learned and latched correctly.
    *   *Inputs*: Speed sweeps from 0 to 20 m/s with constant front/rear loads.
    *   *Expected Output*: `m_static_rear_load` matches the input load and `m_static_load_latched` becomes true.

## Deliverables
*   [x] Code changes in `FFBEngine.h`, `FFBEngine.cpp`, `GripLoadEstimation.cpp`, `Config.h`, `Config.cpp`, `GuiLayer_Common.cpp`, `Tooltips.h`.
*   [x] New unit tests in `tests/test_issue_322_yaw_kicks.cpp`.
*   [x] Documentation updates (this plan).
*   [x] **Implementation Notes**: Update this plan document with "Unforeseen Issues", "Plan Deviations", "Challenges", and "Recommendations" upon completion.

## Implementation Notes

### Unforeseen Issues
*   **Test Environment Sensitivity**: Learning static load in tests required careful sequencing. `calculate_force` calls `InitializeLoadReference` which can reset learned state if called out of order. Tests were updated to manually seed and latch static loads to ensure deterministic gating.
*   **Speed Gate Interaction**: The global speed gate was muting effects at low speeds in unit tests. Tests now explicitly disable the speed gate to verify the core physics logic.

### Plan Deviations
*   **Sign-Preserving Blending**: Instead of a simple `max_abs`, I implemented a sign-preserving blending where the kick with the highest magnitude is selected while maintaining its physical direction.
*   **Load Learning Migration**: Added a migration path in `InitializeLoadReference` where if a front static load exists but a rear doesn't (from previous versions), the rear is initialized to a sane default rather than zero, preventing instant triggers.

### Challenges
*   **Coordinate System Consistency**: Verified that LMU's coordinate system (+Z rearward, +X left) is correctly handled for jerk calculation and sign convention in `calculate_sop_lateral`.

### Recommendations
*   Future investigation could involve gating the "Power Yaw Kick" even more strictly using `abs(mLocalRot.y)` to ensure the kick only triggers in the direction of the spin, though the current `max_abs` blending handles this well.
