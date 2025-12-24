# Technical Report: Lockup Vibration Fixes & Enhancements

**Date:** December 24, 2025
**Subject:** Resolution of missing rear lockup effects, manual slip calculation bugs, and implementation of advanced tuning controls.

---

## 1. Problem Statement

User testing identified three critical deficiencies in the "Braking Lockup" FFB effect:
1.  **Rear Lockups Ignored:** The application only monitored front wheels for slip. Locking the rear brakes (common in LMP2/Hypercars) produced no vibration, leading to unrecoverable spins without tactile warning.
2.  **Late Warning:** The hardcoded trigger threshold of 10% slip (`-0.1`) was too high. By the time the effect triggered, the tires were already deep into the lockup phase, offering no opportunity for threshold braking modulation.
3.  **Broken Manual Calculation:** The "Manual Slip Calc" fallback option produced no output due to a coordinate system sign error.

## 2. Root Cause Analysis

*   **Rear Blindness:** The `calculate_force` loop explicitly checked only `mWheel[0]` and `mWheel[1]`. Telemetry analysis confirms LMU *does* provide valid longitudinal patch velocity for rear wheels, so this was a logic omission, not a data gap.
*   **Manual Slip Bug:** The formula used `data->mLocalVel.z` directly. In LMU, forward velocity is **negative**. Passing a negative denominator to the slip ratio formula resulted in positive (traction) ratios during braking, failing the `< -0.1` check.
*   **Thresholds:** The linear ramp from 10% to 50% slip did not align with the physics of slick tires, which typically peak around 12-15% slip.

---

## 3. Implementation Plan

### A. Physics Engine (`FFBEngine.h`)

We will implement a **4-Wheel Monitor** with **Axle Differentiation**.
*   **Differentiation:** If the rear axle has a higher slip ratio than the front, the vibration frequency drops to **30%** (Heavy Judder) and amplitude is boosted. This allows the driver to distinguish between Understeer (Front Lock - High Pitch) and Instability (Rear Lock - Low Pitch).
*   **Dynamic Ramp:** Instead of a fixed threshold, we introduce a configurable window:
    *   **Start %:** Vibration begins (Early Warning).
    *   **Full %:** Vibration hits max amplitude (Peak Limit).
    *   **Curve:** Quadratic ($x^2$) ramp to provide a sharp, distinct "wall" of vibration as the limit is reached.

#### Code Changes

**1. Fix Manual Slip Calculation**
```cpp
// Inside calculate_force lambda
auto get_slip_ratio = [&](const TelemWheelV01& w) {
    if (m_use_manual_slip) {
        // FIX: Use std::abs() because mLocalVel.z is negative (forward)
        return calculate_manual_slip_ratio(w, std::abs(data->mLocalVel.z));
    }
    // ... existing fallback ...
};
```

**2. New Member Variables**
```cpp
// Defaults
float m_lockup_start_pct = 5.0f;      // Warning starts at 5% slip
float m_lockup_full_pct = 15.0f;      // Max vibration at 15% slip
float m_lockup_rear_boost = 1.5f;     // Rear lockups are 1.5x stronger
```

**3. Updated Lockup Logic**
```cpp
// --- 2b. Progressive Lockup (4-Wheel Monitor) ---
if (m_lockup_enabled && data->mUnfilteredBrake > 0.05) {
    // 1. Calculate Slip for ALL wheels
    double slip_fl = get_slip_ratio(data->mWheel[0]);
    double slip_fr = get_slip_ratio(data->mWheel[1]);
    double slip_rl = get_slip_ratio(data->mWheel[2]);
    double slip_rr = get_slip_ratio(data->mWheel[3]);

    // 2. Find worst slip per axle (Slip is negative, so use min)
    double max_slip_front = (std::min)(slip_fl, slip_fr);
    double max_slip_rear  = (std::min)(slip_rl, slip_rr);

    // 3. Determine Source & Differentiation
    double effective_slip = 0.0;
    double freq_multiplier = 1.0;
    double amp_multiplier = 1.0;

    if (max_slip_rear < max_slip_front) {
        // REAR LOCKUP DETECTED
        effective_slip = max_slip_rear;
        freq_multiplier = 0.3; // Low Pitch (Judder/Thud)
        amp_multiplier = m_lockup_rear_boost; // Configurable Boost
    } else {
        // FRONT LOCKUP DETECTED
        effective_slip = max_slip_front;
        freq_multiplier = 1.0; // Standard Pitch (Screech)
        amp_multiplier = 1.0;
    }

    // 4. Dynamic Thresholds
    double start_ratio = m_lockup_start_pct / 100.0;
    double full_ratio = m_lockup_full_pct / 100.0;
    
    // Check if we crossed the start threshold
    if (effective_slip < -start_ratio) {
        // Normalize slip into 0.0 - 1.0 range based on window
        double slip_abs = std::abs(effective_slip);
        double window = full_ratio - start_ratio;
        if (window < 0.01) window = 0.01; // Safety div/0

        double normalized = (slip_abs - start_ratio) / window;
        double severity = (std::min)(1.0, normalized);
        
        // Apply Quadratic Curve (Sharp feel at limit)
        severity = severity * severity;

        // Frequency Calculation
        double car_speed_ms = std::abs(data->mLocalVel.z); 
        double base_freq = 10.0 + (car_speed_ms * 1.5); 
        double final_freq = base_freq * freq_multiplier;

        // Phase Integration
        m_lockup_phase += final_freq * dt * TWO_PI;
        m_lockup_phase = std::fmod(m_lockup_phase, TWO_PI);

        // Final Amplitude
        double amp = severity * m_lockup_gain * 4.0 * decoupling_scale * amp_multiplier;
        
        total_force += std::sin(m_lockup_phase) * amp;
    }
}
```

---

### B. Configuration (`src/Config.h` / `.cpp`)

We need to persist the three new tuning parameters.

**1. Update `Preset` Struct**
```cpp
struct Preset {
    // ... existing ...
    float lockup_start_pct = 5.0f;
    float lockup_full_pct = 15.0f;
    float lockup_rear_boost = 1.5f;
    
    // Add setters and update Apply/UpdateFromEngine methods
    Preset& SetLockupThresholds(float start, float full, float rear_boost) {
        lockup_start_pct = start;
        lockup_full_pct = full;
        lockup_rear_boost = rear_boost;
        return *this;
    }
};
```

**2. Update Persistence**
Add `lockup_start_pct`, `lockup_full_pct`, and `lockup_rear_boost` to the `config.ini` read/write logic.

---

### C. GUI Layer (`src/GuiLayer.cpp`)

Expose the new controls in the "Tactile Textures" -> "Lockup Vibration" section.

```cpp
BoolSetting("Lockup Vibration", &engine.m_lockup_enabled);
if (engine.m_lockup_enabled) {
    // Existing Gain
    FloatSetting("  Lockup Strength", &engine.m_lockup_gain, 0.0f, 2.0f, ...);
    
    // NEW: Thresholds
    FloatSetting("  Start Slip %", &engine.m_lockup_start_pct, 1.0f, 10.0f, "%.1f%%", 
        "Slip % where vibration begins (Early Warning).\nTypical: 4-6%.");
        
    FloatSetting("  Full Slip %", &engine.m_lockup_full_pct, 5.0f, 25.0f, "%.1f%%", 
        "Slip % where vibration hits 100% amplitude (Peak Grip).\nTypical: 12-15%.");
        
    // NEW: Rear Boost
    FloatSetting("  Rear Boost", &engine.m_lockup_rear_boost, 1.0f, 3.0f, "%.1fx", 
        "Amplitude multiplier for rear wheel lockups.\nMakes rear instability feel more dangerous/distinct.");
}
```

---

## 4. Verification Strategy

1.  **Manual Slip Test:** Enable "Manual Slip Calc". Drive. Brake hard. Verify vibration occurs (proving sign fix).
2.  **Rear Lockup Test:** Drive LMP2. Bias brakes to rear (or engine brake heavily). Lock rear wheels. Verify vibration is **Lower Pitch** (thudding) compared to front lockup.
3.  **Threshold Test:** Set "Start %" to 1.0%. Lightly brake. Verify subtle vibration starts immediately. Set "Start %" to 10%. Lightly brake. Verify silence until heavy braking.

---



### 5. Future Enhancement: Advanced Response Curves (Non-Linearity)

While the current implementation introduces a hardcoded **Quadratic ($x^2$)** ramp to improve the "sharpness" of the limit, future versions (v0.6.0+) should expose this as a fully configurable **Gamma** setting.

*   **The Concept:** Instead of a linear progression ($0\% \to 100\%$ vibration as slip increases), a non-linear curve allows the user to define the "feel" of the approach.
*   **Proposed Control:** **"Vibration Gamma"** slider (0.5 to 3.0).
    *   **Gamma 1.0 (Linear):** Vibration builds steadily. Good for learning threshold braking.
    *   **Gamma 2.0 (Quadratic - Current Default):** Vibration remains subtle in the "Warning Zone" (5-10% slip) and ramps up aggressively near the "Limit" (12-15%). This creates a distinct tactile "Wall" at the limit.
    *   **Gamma 3.0 (Cubic):** The wheel is almost silent until the very last moment, then spikes to max. Preferred by aliens/pros who find early vibrations distracting.
*   **Implementation Logic:**
    $$ \text{Amplitude} = \text{BaseAmp} \times (\text{NormalizedSeverity})^{\gamma} $$

### 6. Future Enhancement: Predictive Lockup via Angular Deceleration

To achieve a true "ABS-like" prediction that triggers *before* significant slip occurs, we can implement **Wheel Angular Deceleration** monitoring.

*   **The Physics:** Before a tire reaches its peak slip ratio, the wheel's rotational speed ($\omega$) drops rapidly. If the wheel decelerates significantly faster than the car chassis, a lockup is imminent.
*   **The Metric:** $\alpha_{wheel} = \frac{d\omega}{dt}$ (Angular Acceleration).
*   **Trigger Logic:**
    *   Calculate $\alpha_{wheel}$ for each wheel (requires differentiating `mRotation` over time).
    *   Compare against a threshold (e.g., $-100 \text{ rad/s}^2$).
    *   If $\alpha_{wheel} < \text{Threshold}$, trigger the vibration *even if* Slip Ratio is still low.
*   **Challenges:**
    *   **Noise:** Derivatives amplify signal noise. Bumps and kerbs cause massive spikes in angular acceleration.
    *   **Smoothing:** Requires a robust Low Pass Filter (LPF) on the derivative to prevent false positives, which adds slight latency.
*   **Benefit:** This provides the earliest possible warning, potentially 50-100ms faster than Slip Ratio, allowing the driver to modulate brake pressure at the very onset of instability.

### 7. Verification & Test Plan (New Changes)

To ensure the stability and correctness of the v0.5.11 changes, the following tests must be implemented in `tests/test_ffb_engine.cpp`.

#### A. `test_manual_slip_sign_fix`
**Goal:** Verify that the "Manual Slip Calc" option now works correctly with LMU's negative forward velocity.
*   **Setup:**
    *   Enable `m_use_manual_slip`.
    *   Set `mLocalVel.z = -20.0` (Forward).
    *   Set Wheel Velocity to `0.0` (Locked).
*   **Check:**
    *   Previous Bug: Calculated Slip $= (0 - (-20)) / 20 = +1.0$ (Traction). Effect Silent.
    *   Expected Fix: Calculated Slip $= (0 - 20) / 20 = -1.0$ (Lockup). Effect Active.
*   **Assert:** `m_lockup_phase` > 0.0.

#### B. `test_rear_lockup_differentiation`
**Goal:** Verify that rear lockups trigger the effect AND produce a lower frequency.
*   **Setup:**
    *   **Pass 1 (Front):** Front Slip `-0.2`, Rear Slip `0.0`. Record `phase_delta_front`.
    *   **Pass 2 (Rear):** Front Slip `0.0`, Rear Slip `-0.2`. Record `phase_delta_rear`.
*   **Check:**
    *   `phase_delta_rear > 0` (Rear lockup is detected).
    *   `phase_delta_rear` $\approx$ `0.3 * phase_delta_front` (Frequency is lower).

#### C. `test_lockup_threshold_config`
**Goal:** Verify that the new `Start %` and `Full %` sliders correctly alter the trigger point and intensity.
*   **Setup:**
    *   Set `m_lockup_start_pct = 5.0`.
    *   Set `m_lockup_full_pct = 15.0`.
*   **Scenario 1:** Input Slip `-0.04` (4%).
    *   **Assert:** Force == 0.0 (Below Start Threshold).
*   **Scenario 2:** Input Slip `-0.10` (10%).
    *   **Assert:** Force > 0.0 AND Force < Max (In the Ramp).
*   **Scenario 3:** Input Slip `-0.20` (20%).
    *   **Assert:** Force == Max (Saturated).