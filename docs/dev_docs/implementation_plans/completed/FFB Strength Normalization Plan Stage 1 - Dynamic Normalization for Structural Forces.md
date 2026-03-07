# Implementation Plan: Stage 1 - Dynamic Normalization for Structural Forces

## Context
The current FFB architecture uses a static `m_max_torque_ref` to normalize the raw physics torque from the game. This causes issues across different car classes (e.g., GT3 vs. Hypercar), leading users to artificially set `m_max_torque_ref` to 100 Nm to avoid clipping, which breaks the intended 1:1 hardware scaling. 

Stage 1 introduces a **Session-Learned Dynamic Normalization** system using a Leaky Integrator (Exponential Decay) and Contextual Spike Rejection. Crucially, this normalization is applied *only* to structural forces (steering, SoP) via an EMA-smoothed gain multiplier, leaving tactile textures and user UI/Config untouched for now.

## Reference Documents

* `docs\dev_docs\investigations\FFB Strength and Tire Load Normalization2.md`
* `docs\dev_docs\investigations\FFB Strength and Tire Load Normalization3.md`

## Codebase Analysis Summary
*   **Current Architecture:** In `FFBEngine::calculate_force`, all FFB effects are summed together into `total_sum`. This sum is then divided by `max_torque_safe` (derived from `m_max_torque_ref`) to produce the final `0.0 - 1.0` normalized signal.
*   **Impacted Functionalities:** 
    *   `FFBEngine::calculate_force`: Needs new logic to track the peak torque dynamically.
    *   Summation logic: The single `total_sum / max_torque_safe` division must be split. Structural forces will be multiplied by the new dynamic multiplier, while texture forces will continue to use the legacy `max_torque_safe` division.

## FFB Effect Impact Analysis

| FFB Effect | Category | Impact | Technical Change | User-Facing Change |
| :--- | :--- | :--- | :--- | :--- |
| **Base Steering** | Structural | **Modified** | Multiplied by `m_smoothed_structural_mult` | Consistent weight across all car classes. |
| **SoP Lateral** | Structural | **Modified** | Multiplied by `m_smoothed_structural_mult` | Scales correctly with the car's actual cornering forces. |
| **Rear Align** | Structural | **Modified** | Multiplied by `m_smoothed_structural_mult` | Counter-steer force remains proportional to base steering. |
| **Yaw Kick** | Structural | **Modified** | Multiplied by `m_smoothed_structural_mult` | Impulse remains proportional to base steering. |
| **Gyro Damping** | Structural | **Modified** | Multiplied by `m_smoothed_structural_mult` | Damping remains proportional to base steering. |
| **Soft Lock** | Structural | **Modified** | Multiplied by `m_smoothed_structural_mult` | Bump stop resistance scales correctly. |
| **Road Texture** | Tactile | *Unaffected* | Divided by `max_torque_safe` (Legacy) | No change. |
| **Slide Rumble** | Tactile | *Unaffected* | Divided by `max_torque_safe` (Legacy) | No change. |
| **Lockup/ABS** | Tactile | *Unaffected* | Divided by `max_torque_safe` (Legacy) | No change. |
| **Wheel Spin** | Tactile | *Unaffected* | Divided by `max_torque_safe` (Legacy) | No change. |
| **Bottoming** | Tactile | *Unaffected* | Divided by `max_torque_safe` (Legacy) | No change. |

## Proposed Changes

### 1. `src/FFBEngine.h`
Add internal state variables for the peak follower and spike rejection.
```cpp
// Add to FFBEngine class (private or public for test access)
double m_session_peak_torque = 25.0; 
double m_smoothed_structural_mult = 1.0 / 25.0;
double m_rolling_average_torque = 0.0;
double m_last_raw_torque = 0.0;
```
*Also add `m_session_peak_torque` to `FFBSnapshot` so it can be visualized in the Debug UI.*

### 2. `src/FFBEngine.cpp`
Inject the dynamic normalization logic into `calculate_force`, right after telemetry ingestion and before effect calculations.

**A. Contextual Spike Rejection & Leaky Integrator:**
```cpp
// 1. Contextual Spike Rejection (Lightweight MAD alternative)
double current_abs_torque = std::abs(raw_torque_input);
double alpha_slow = ctx.dt / (1.0 + ctx.dt); // 1-second rolling average
m_rolling_average_torque += alpha_slow * (current_abs_torque - m_rolling_average_torque);

double lat_g_abs = std::abs(data->mLocalAccel.x / 9.81);
double torque_slew = std::abs(raw_torque_input - m_last_raw_torque) / ctx.dt;
m_last_raw_torque = raw_torque_input;

// Flag as spike if torque jumps > 3x the rolling average (with a 15Nm floor to prevent low-speed false positives)
bool is_contextual_spike = (current_abs_torque > (m_rolling_average_torque * 3.0)) && (current_abs_torque > 15.0);
bool is_clean_state = (lat_g_abs < 8.0) && (torque_slew < 1000.0) && !restricted && !is_contextual_spike;

// 2. Leaky Integrator (Exponential Decay + Floor)
if (is_clean_state && m_torque_source == 0) {
    if (current_abs_torque > m_session_peak_torque) {
        m_session_peak_torque = current_abs_torque; // Fast attack
    } else {
        // Exponential decay (0.5% reduction per second)
        double decay_factor = 1.0 - (0.005 * ctx.dt); 
        m_session_peak_torque *= decay_factor; 
    }
    // Absolute safety floor and ceiling
    m_session_peak_torque = std::clamp(m_session_peak_torque, 15.0, 80.0);
}

// 3. EMA Filtering on the Gain Multiplier (Zero-latency physics)
double target_structural_mult = (m_torque_source == 1) ? 1.0 : (1.0 / m_session_peak_torque);
double alpha_gain = ctx.dt / (0.25 + ctx.dt); // 250ms smoothing
m_smoothed_structural_mult += alpha_gain * (target_structural_mult - m_smoothed_structural_mult);
```

**B. Split Summation Logic:**
Modify the `--- 6. SUMMATION ---` block:
```cpp
double structural_sum = output_force + ctx.sop_base_force + ctx.rear_torque + ctx.yaw_force + ctx.gyro_force + ctx.soft_lock_force;
structural_sum *= ctx.gain_reduction_factor;

// Apply smoothed multiplier ONLY to structural forces
double norm_structural = structural_sum * m_smoothed_structural_mult;

// Textures remain untouched for now (Legacy scaling)
double texture_sum = ctx.road_noise + ctx.slide_noise + ctx.spin_rumble + ctx.bottoming_crunch + ctx.abs_pulse_force + ctx.lockup_rumble;
double norm_texture = texture_sum / max_torque_safe; 

double total_sum = norm_structural + norm_texture;
```

### Parameter Synchronization Checklist
*N/A for Stage 1. No new user-facing settings are added to `Config.h` or the GUI. This is purely an internal physics engine upgrade.*

### Initialization Order Analysis
*   `m_session_peak_torque` must be initialized to `25.0` (a safe GT3 baseline).
*   `m_smoothed_structural_mult` must be initialized to `1.0 / 25.0` to prevent a massive jolt on the very first frame before the EMA settles.
*   These are standard primitive types and can be initialized directly in the `FFBEngine.h` class definition.

### Version Increment Rule
*   Increment the version number in `VERSION` and `src/Version.h` by the **smallest possible increment** (e.g., `0.7.63` -> `0.7.64`).

## Test Plan (TDD-Ready)

**File:** `tests/test_ffb_dynamic_normalization.cpp` (New file)
**Expected Test Count:** Baseline + 4 new tests.

**1. `test_peak_follower_fast_attack`**
*   **Description:** Verifies that `m_session_peak_torque` updates instantly when a valid high torque is encountered.
*   **Inputs:** `raw_torque_input` = 40.0 Nm, `m_session_peak_torque` initially 25.0.
*   **Expected Output:** `m_session_peak_torque` becomes exactly 40.0 on the first frame.

**2. `test_peak_follower_exponential_decay`**
*   **Description:** Verifies the leaky integrator slowly reduces the peak over time when torque is low.
*   **Data Flow Analysis:**
    *   Frame 0: Peak is 40.0 Nm. Input drops to 10.0 Nm.
    *   Frame 1 (dt=0.01): Peak should be `40.0 * (1.0 - (0.005 * 0.01))` = `39.998`.
    *   Frame 100 (1 second later): Peak should be noticeably lower but not instantly reset.
*   **Assertions:** `new_peak < 40.0` and `new_peak > 39.0` after 1 second.

**3. `test_contextual_spike_rejection`**
*   **Description:** Verifies that a sudden, massive spike does not corrupt the peak learner.
*   **Inputs:** 
    *   Feed 100 frames of 15.0 Nm to settle the `m_rolling_average_torque`.
    *   Feed 1 frame of 100.0 Nm (simulating a wall hit).
*   **Expected Output:** `m_session_peak_torque` remains at 15.0 (or its decayed value), ignoring the 100.0 Nm spike.

**4. `test_structural_vs_texture_separation`**
*   **Description:** Verifies that the dynamic multiplier only affects structural forces.
*   **Inputs:** Set `m_session_peak_torque` to 50.0 (multiplier = 0.02). Set `max_torque_safe` to 20.0 (multiplier = 0.05). Inject 10.0 Nm of base steering and 10.0 Nm of road texture.
*   **Expected Output:** 
    *   Structural contribution = `10.0 * 0.02 = 0.2`
    *   Texture contribution = `10.0 / 20.0 = 0.5`
    *   Total normalized force = `0.7`

## Deliverables
- Update `src/FFBEngine.h` with new state variables.
- Update `src/FFBEngine.cpp` with Contextual Spike Rejection, Leaky Integrator, EMA Gain Smoothing, and split summation.
- Create `tests/test_ffb_dynamic_normalization.cpp` and implement the 4 test cases.
- Update `VERSION` and `src/Version.h`.
- **Implementation Notes:** Update this plan document with "Unforeseen Issues", "Plan Deviations", "Challenges", and "Recommendations" upon completion.
