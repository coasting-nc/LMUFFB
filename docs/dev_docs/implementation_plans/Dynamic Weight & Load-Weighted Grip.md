# Implementation Plan - Dynamic Weight & Load-Weighted Grip

## 1. Context
This plan implements two advanced FFB features that leverage the availability of real tire load (`mTireLoad`) and grip (`mGripFract`) data.

**Goals:**
1.  **Dynamic Weight (Longitudinal Load Transfer):** Scale the master FFB gain based on the ratio of current front tire load to static load. This makes the steering heavier under braking (load transfer to front) and lighter under acceleration.
2.  **Load-Weighted Grip:** Improve the grip estimation by weighting the grip fraction of each wheel by its vertical load. This ensures that the FFB feel is dominated by the loaded (working) tire during cornering, rather than an average that includes the unloaded (lifting) tire.

**Reference Documents:**
*   User Request: "Dynamic Weight scaling and Load-Weighted Grip"
*   Diagnostic Reports: `docs/dev_docs/investigations/slope_detection_feasibility.md` (Context on estimation vs real data)

## 2. Codebase Analysis

### 2.1 Architecture Overview
*   **`src/FFBEngine.h`:**
    *   `calculate_force`: Main physics loop. Currently calculates `ctx.avg_load` and handles fallbacks if data is missing.
    *   `calculate_grip`: Helper function currently using a simple average `(w1.mGripFract + w2.mGripFract) / 2.0`.
*   **`src/Config.h`:** Defines the `Preset` struct and default values.
*   **`src/Config.cpp`:** Handles persistence (Save/Load) of settings.
*   **`src/GuiLayer_Common.cpp`:** Handles the ImGui interface and sliders.

### 2.2 Impacted Functionalities
*   **Grip Calculation:** `calculate_grip` will be mathematically altered to favor loaded tires.
*   **Force Output:** The final `output_force` will be modulated by a new `dynamic_weight_factor`.
*   **State Management:** New state variable `m_static_front_load` required to track baseline weight.
*   **User Interface:** New slider required for "Dynamic Weight".

## 3. FFB Effect Impact Analysis

| Effect | Technical Impact | User Perspective |
| :--- | :--- | :--- |
| **Dynamic Weight** | **New Modifier.** Scales `output_force` based on `ctx.avg_load / m_static_front_load`. <br> **Constraint:** Only active if `!ctx.frame_warn_load` (Real data available). | **Braking Feel:** Steering becomes heavier under hard braking (confidence). <br> **Acceleration:** Steering becomes lighter on exit (understeer cue). <br> **New Setting:** "Dynamic Weight Gain" (0-200%). |
| **Understeer (Grip)** | **Refinement.** `ctx.avg_grip` becomes a weighted average. <br> **Logic:** `(G1*L1 + G2*L2) / (L1+L2)`. | **Cornering Precision:** FFB will feel more connected to the outside tire. Lifting the inside wheel won't artificially restore "grip feel" to the FFB signal. |

## 4. Proposed Changes

### 4.1 File: `src/Config.h`

**A. Update `Preset` Struct**
Add `dynamic_weight_gain` parameter.
```cpp
struct Preset {
    // ... existing members ...
    float dynamic_weight_gain = 0.0f; // Default 0.0 (Off)
    
    // ... setters ...
    Preset& SetDynamicWeight(float v) { dynamic_weight_gain = v; return *this; }
    
    // ... Apply/Validate/UpdateFromEngine/Equals updates ...
};
```

**Parameter Synchronization Checklist:**
*   [ ] Declaration in `FFBEngine.h` (`float m_dynamic_weight_gain`).
*   [ ] Declaration in `Preset` struct (`Config.h`).
*   [ ] Entry in `Preset::Apply()`.
*   [ ] Entry in `Preset::UpdateFromEngine()`.
*   [ ] Entry in `Preset::Validate()` (Clamp 0.0 to 2.0).
*   [ ] Entry in `Preset::Equals()`.
*   [ ] Entry in `Config::Save()` (`src/Config.cpp`).
*   [ ] Entry in `Config::Load()` (`src/Config.cpp`).

### 4.2 File: `src/FFBEngine.h`

**A. Add State Members**
```cpp
private:
    // ... existing members ...
    double m_static_front_load = 0.0; // Learned baseline load
    
public:
    float m_dynamic_weight_gain; // Configurable setting
```

**B. Implement `update_static_load_reference`**
Helper to learn the static weight when aero/transfer is minimal.
```cpp
void update_static_load_reference(double current_load, double speed, double dt) {
    // Update only at low speeds (e.g., 2-15 m/s) where aero is negligible
    if (speed > 2.0 && speed < 15.0) {
        // Slow LPF (5.0s time constant)
        m_static_front_load += (dt / 5.0) * (current_load - m_static_front_load);
    }
    // Safety floor
    if (m_static_front_load < 1000.0) m_static_front_load = 4000.0;
}
```

**C. Update `calculate_grip`**
Replace simple average with load-weighted average.
```cpp
// Inside calculate_grip(...)
double total_load = w1.mTireLoad + w2.mTireLoad;
if (total_load > 1.0) {
    result.original = (w1.mGripFract * w1.mTireLoad + w2.mGripFract * w2.mTireLoad) / total_load;
} else {
    // Fallback for zero load (e.g. jump/missing data)
    result.original = (w1.mGripFract + w2.mGripFract) / 2.0;
}
```

**D. Update `calculate_force`**
Implement the Dynamic Weight logic.
```cpp
// 1. Update static reference
update_static_load_reference(ctx.avg_load, ctx.car_speed, ctx.dt);

// 2. Calculate Dynamic Weight Factor
double dynamic_weight_factor = 1.0;

// Only apply if enabled AND we have real load data (no warnings)
if (m_dynamic_weight_gain > 0.0 && !ctx.frame_warn_load) {
    double load_ratio = ctx.avg_load / m_static_front_load;
    // Blend: 1.0 + (Ratio - 1.0) * Gain
    dynamic_weight_factor = 1.0 + (load_ratio - 1.0) * (double)m_dynamic_weight_gain;
    dynamic_weight_factor = std::clamp(dynamic_weight_factor, 0.5, 2.0);
}

// 3. Apply to Output Force
// double output_force = (base_input * m_steering_shaft_gain) * ctx.grip_factor; 
// BECOMES:
double output_force = (base_input * m_steering_shaft_gain) * dynamic_weight_factor * ctx.grip_factor;
```

### 4.3 File: `src/GuiLayer_Common.cpp`

**A. Add Slider to "Front Axle (Understeer)" Section**
Add the control for `m_dynamic_weight_gain`.
```cpp
// Inside DrawTuningWindow -> Front Axle section
FloatSetting("Dynamic Weight", &engine.m_dynamic_weight_gain, 0.0f, 2.0f, FormatPct(engine.m_dynamic_weight_gain),
    "Scales steering weight based on longitudinal load transfer.\n"
    "Heavier under braking, lighter under acceleration.\n"
    "Requires valid tire load data (Hypercars).");
```

### 4.4 File: `VERSION` & `src/Version.h`
*   Increment version (e.g., `0.7.45` -> `0.7.46`).

## 5. Test Plan (TDD)

**New Test File:** `tests/test_ffb_dynamic_weight.cpp`

### Test 1: `test_load_weighted_grip`
*   **Goal:** Verify that the tire with higher load dominates the grip calculation.
*   **Setup:**
    *   Wheel 1 (Outside): Load = 10000N, Grip = 0.8 (Sliding).
    *   Wheel 2 (Inside): Load = 500N, Grip = 1.0 (Not sliding).
*   **Action:** Call `calculate_grip`.
*   **Assertion:**
    *   Simple Average would be `0.9`.
    *   Weighted Average should be approx `(8000 + 500) / 10500 = 0.81`.
    *   Assert `result.original` is near `0.81`.

### Test 2: `test_dynamic_weight_scaling`
*   **Goal:** Verify master gain increases under braking load.
*   **Setup:**
    *   Configure `dynamic_weight_gain = 1.0`.
    *   Seed `m_static_front_load = 4000.0`.
    *   Input `mTireLoad` (avg) = 8000.0 (Heavy Braking).
    *   Input `SteeringTorque` = 5.0 Nm.
*   **Action:** Call `calculate_force`.
*   **Assertion:**
    *   Load Ratio = 2.0.
    *   Factor = 1.0 + (1.0 * 1.0) = 2.0.
    *   Output should be approx `5.0 * 2.0 = 10.0 Nm`.

### Test 3: `test_dynamic_weight_safety_gate`
*   **Goal:** Verify Dynamic Weight is disabled if load data is missing (fallback mode).
*   **Setup:**
    *   Configure `dynamic_weight_gain = 1.0`.
    *   Input `mTireLoad` = 0.0 (Trigger fallback/warning).
*   **Action:** Call `calculate_force`.
*   **Assertion:**
    *   `ctx.frame_warn_load` should be true.
    *   `dynamic_weight_factor` should remain 1.0.

## 6. Deliverables

*   [ ] **Code:** Updated `src/Config.h` & `src/Config.cpp` (New setting).
*   [ ] **Code:** Updated `src/FFBEngine.h` (Logic implementation).
*   [ ] **Code:** Updated `src/GuiLayer_Common.cpp` (New slider).
*   [ ] **Tests:** New `tests/test_ffb_dynamic_weight.cpp`.
*   [ ] **Implementation Notes:** Update plan with any deviations.


## Implementation Notes
- **Unforeseen Issues:** The default `invert_force = true` in the `Preset` struct caused the initial unit tests to fail due to negative force output. This was resolved by explicitly setting `invert_force = false` in the test cases.
- **Plan Deviations:** None. The implementation followed the plan exactly.
- **Challenges Encountered:** Managing the learning of `m_static_front_load` required a few frames of simulation in tests to reach a stable state.
- **Recommendations for Future Plans:** Explicitly state the expected `invert_force` state for physics-based unit tests to avoid confusion.
