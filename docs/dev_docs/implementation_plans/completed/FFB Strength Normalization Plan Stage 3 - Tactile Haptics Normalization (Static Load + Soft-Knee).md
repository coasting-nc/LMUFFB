# Implementation Plan: Stage 3 - Tactile Haptics Normalization (Static Load + Soft-Knee)

## Context
Currently, tactile effects (like road texture and lockup vibration) are scaled using a dynamic load factor (`ctx.avg_load / m_auto_peak_load`). Because `m_auto_peak_load` acts as a peak follower, high-speed aerodynamic downforce pushes the denominator to extreme values. When the car slows down, the dynamic load drops but the peak denominator remains high, causing low-speed tactile effects to feel artificially weak or "dead".

Stage 3 resolves this by anchoring tactile effects to the vehicle's **Static Mechanical Load** (learned exclusively between 2-15 m/s). To prevent high-speed aero loads from causing violent vibrations when scaled against this static baseline, we will implement the **Giannoulis Soft-Knee Compression** algorithm, which smoothly tapers the tactile multiplier at high loads.

## Reference Documents

* `docs\dev_docs\investigations\FFB Strength and Tire Load Normalization2.md`
* `docs\dev_docs\investigations\FFB Strength and Tire Load Normalization3.md`
* Previous stage 1 implementation plan: `docs\dev_docs\implementation_plans\FFB Strength Normalization Plan Stage 1 - Dynamic Normalization for Structural Forces.md`
* Previous stage 1 implementation plan (modified during development): `docs\dev_docs\implementation_plans\plan_152.md`
* Previous stage 2 implementation plan: `docs\dev_docs\implementation_plans\FFB Strength Normalization Plan Stage 2 - Hardware Scaling Redefinition (UI & Config).md`
* Previous stage 2 implementation plan (modified during development): `docs\dev_docs\implementation_plans\plan_153.md`

## Codebase Analysis Summary
*   **Current Architecture:** `FFBEngine::update_static_load_reference` calculates `m_static_front_load`, but it is currently only used for the `dynamic_weight_factor`. Tactile scaling (`ctx.texture_load_factor` and `ctx.brake_load_factor`) relies on `m_auto_peak_load`.
*   **Impacted Functionalities:**
    *   `FFBEngine::update_static_load_reference`: Needs to latch (freeze) the learned value once the vehicle exceeds 15 m/s to prevent aerodynamic pollution.
    *   `FFBEngine::calculate_force`: The calculation of `raw_load_factor` must be replaced with the Soft-Knee compression logic based on `m_static_front_load`.
    *   `FFBEngine::calculate_suspension_bottoming`: Currently uses `m_auto_peak_load * 1.6` as a safety trigger. This should be updated to use `m_static_front_load * 2.5` to remain consistent with the new static baseline.

## FFB Effect Impact Analysis

| FFB Effect | Category | Impact | Technical Change | User-Facing Change |
| :--- | :--- | :--- | :--- | :--- |
| **Road Texture** | Tactile | **Modified** | Scaled by Soft-Knee compressed static load ratio. | Consistent detail at low speeds; prevents violent shaking at top speed in high-downforce cars. |
| **Slide Rumble** | Tactile | **Modified** | Scaled by Soft-Knee compressed static load ratio. | More consistent friction feel regardless of aerodynamic load. |
| **Lockup Vibration** | Tactile | **Modified** | Scaled by Soft-Knee compressed static load ratio. | Braking haptics remain strong and communicative even after long straights. |
| **Bottoming** | Tactile | **Modified** | Safety trigger threshold changed to `m_static_front_load * 2.5`. | More accurate detection of severe suspension compressions. |
| **Base Steering / SoP** | Structural | *Unaffected* | None. | No change. |

## Proposed Changes

### 1. `src/FFBEngine.h`
Add state variables to track the latching status and smooth the tactile multiplier.
```cpp
// Add to FFBEngine class (private or public for test access)
bool m_static_load_latched = false;
double m_smoothed_tactile_mult = 1.0;
```

### 2. `src/FFBEngine.cpp`
**A. Update `update_static_load_reference`:**
Modify the function to latch the value once the speed exceeds 15 m/s.
```cpp
void FFBEngine::update_static_load_reference(double current_load, double speed, double dt) {
    if (m_static_load_latched) return; // Do not update if latched

    if (speed > 2.0 && speed < 15.0) {
        if (m_static_front_load < 100.0) {
             m_static_front_load = current_load;
        } else {
             m_static_front_load += (dt / 5.0) * (current_load - m_static_front_load);
        }
    } else if (speed >= 15.0 && m_static_front_load > 1000.0) {
        // Latch the value once we exceed 15 m/s (aero begins to take over)
        m_static_load_latched = true;
    }
    
    if (m_static_front_load < 1000.0) m_static_front_load = 4500.0; // Safe fallback
}
```

**B. Implement Giannoulis Soft-Knee in `calculate_force`:**
Replace the `raw_load_factor` calculation (around line 250) with the new Soft-Knee logic.
```cpp
// 1. Calculate raw load multiplier based on static weight
double x = ctx.avg_load / m_static_front_load; 

// 2. Giannoulis Soft-Knee Parameters
double T = 1.5;  // Threshold (Start compressing at 1.5x static weight)
double W = 0.5;  // Knee Width (Transition from 1.25x to 1.75x)
double R = 4.0;  // Compression Ratio (4:1 above the knee)

double lower_bound = T - (W / 2.0);
double upper_bound = T + (W / 2.0);
double compressed_load_factor = x;

// 3. Apply Compression
if (x > upper_bound) {
    // Linear compressed region
    compressed_load_factor = T + ((x - T) / R);
} else if (x > lower_bound) {
    // Quadratic soft-knee transition
    double diff = x - lower_bound;
    compressed_load_factor = x + (((1.0 / R) - 1.0) * (diff * diff)) / (2.0 * W);
}

// 4. EMA Smoothing on the tactile multiplier (100ms time constant)
double alpha_tactile = ctx.dt / (0.1 + ctx.dt);
m_smoothed_tactile_mult += alpha_tactile * (compressed_load_factor - m_smoothed_tactile_mult);

// 5. Apply to context with user caps
double texture_safe_max = (std::min)(10.0, (double)m_texture_load_cap);
ctx.texture_load_factor = (std::min)(texture_safe_max, m_smoothed_tactile_mult);

double brake_safe_max = (std::min)(10.0, (double)m_brake_load_cap);
ctx.brake_load_factor = (std::min)(brake_safe_max, m_smoothed_tactile_mult);
```

**C. Update `calculate_suspension_bottoming`:**
Change the safety trigger threshold to use the static load.
```cpp
// Replace: double bottoming_threshold = m_auto_peak_load * 1.6;
double bottoming_threshold = m_static_front_load * 2.5;
```

### Parameter Synchronization Checklist
*N/A for Stage 3. No new user-facing settings are added to `Config.h` or the GUI.*

### Initialization Order Analysis
*   `m_static_load_latched` must be initialized to `false`.
*   `m_smoothed_tactile_mult` must be initialized to `1.0`.
*   These are standard primitive types and can be initialized directly in the `FFBEngine.h` class definition.
*   `m_static_load_latched` should be reset to `false` inside `InitializeLoadReference` when a new car is loaded.

### Version Increment Rule
*   Increment the version number in `VERSION` and `src/Version.h` by the **smallest possible increment** (e.g., `0.7.65` -> `0.7.66`).

## Test Plan (TDD-Ready)

**File:** `tests/test_ffb_tactile_normalization.cpp` (New file)
**Expected Test Count:** Baseline + 4 new tests.

**1. `test_static_load_latching`**
*   **Description:** Verifies that the static load learner stops updating once speed exceeds 15 m/s.
*   **Inputs:** 
    *   Call `update_static_load_reference` with load=4000, speed=10.0 (should update).
    *   Call `update_static_load_reference` with load=8000, speed=20.0 (should latch).
    *   Call `update_static_load_reference` with load=2000, speed=10.0 (should ignore because latched).
*   **Expected Output:** `m_static_load_latched` is true, and `m_static_front_load` remains near 4000.

**2. `test_soft_knee_linear_region`**
*   **Description:** Verifies that loads below the knee threshold are uncompressed (1:1).
*   **Inputs:** `ctx.avg_load` = 4000, `m_static_front_load` = 4000. (Ratio = 1.0).
*   **Expected Output:** `compressed_load_factor` (and eventually `m_smoothed_tactile_mult` after settling) equals 1.0.

**3. `test_soft_knee_compression_region`**
*   **Description:** Verifies that loads above the knee are strictly compressed at the 4:1 ratio.
*   **Inputs:** `ctx.avg_load` = 10000, `m_static_front_load` = 4000. (Ratio = 2.5).
*   **Expected Output:** `x` = 2.5. Since 2.5 > 1.75, output should be `1.5 + (2.5 - 1.5) / 4.0` = `1.75`.

**4. `test_soft_knee_transition_region`**
*   **Description:** Verifies the quadratic curve inside the knee width.
*   **Inputs:** `ctx.avg_load` = 6000, `m_static_front_load` = 4000. (Ratio = 1.5).
*   **Expected Output:** `x` = 1.5. Output should be `1.5 + ((0.25 - 1.0) * (0.25 * 0.25)) / 1.0` = `1.5 - 0.046875` = `1.453125`.

## Deliverables
- Update `src/FFBEngine.h` with new state variables.
- Update `src/FFBEngine.cpp` with the latching logic, Giannoulis Soft-Knee algorithm, and bottoming threshold update.
- Create `tests/test_ffb_tactile_normalization.cpp` and implement the 4 test cases.
- Update `VERSION` and `src/Version.h`.
- **Implementation Notes:** Update this plan document with "Unforeseen Issues", "Plan Deviations", "Challenges", and "Recommendations" upon completion.
