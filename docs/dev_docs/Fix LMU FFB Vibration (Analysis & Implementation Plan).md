
# Technical Specification: Dynamic Notch Filter & Frequency Estimator

**Target Version:** v0.4.41 (Proposed)
**Context:** Addressing user reports of constant, speed-dependent vibrations (e.g., flat spots) in Le Mans Ultimate.
**Objective:** Implement a surgical filter to remove specific frequencies linked to wheel rotation without adding global latency, and a diagnostic tool to visualize signal frequency.

---

## 1. Mathematical Theory

### A. Wheel Rotation Frequency
To target the vibration caused by a flat spot or tire polygon issue, we must calculate the fundamental frequency of the wheel's rotation.

$$ F_{wheel} = \frac{V_{car}}{C_{tire}} = \frac{V_{car}}{2 \pi r} $$

*   $V_{car}$: Longitudinal Velocity (`mLocalVel.z`) in $m/s$.
*   $r$: Tire Radius (`mStaticUndeflectedRadius`) in meters.
*   $F_{wheel}$: Frequency in Hz.

### B. Biquad Notch Filter
A standard IIR Biquad filter is used to reject a narrow band of frequencies around a center frequency ($F_c$) with a configurable width ($Q$).

**Coefficients Calculation:**
Given sampling rate $F_s$ (400Hz), Center Frequency $F_c$, and Quality Factor $Q$:

1.  $\omega = 2\pi \frac{F_c}{F_s}$
2.  $\alpha = \frac{\sin(\omega)}{2Q}$
3.  Coefficients:
    *   $b_0 = 1$
    *   $b_1 = -2\cos(\omega)$
    *   $b_2 = 1$
    *   $a_0 = 1 + \alpha$
    *   $a_1 = -2\cos(\omega)$
    *   $a_2 = 1 - \alpha$

**Normalization:**
Divide all coefficients by $a_0$.

**Difference Equation (Runtime):**
$$ y[n] = b_0 x[n] + b_1 x[n-1] + b_2 x[n-2] - a_1 y[n-1] - a_2 y[n-2] $$

### C. Frequency Estimator (Zero-Crossing)
To diagnose the vibration, we estimate the frequency of the AC component of the torque signal.

1.  **High-Pass Filter:** Isolate vibration from steering weight.
    $$ x_{AC} = x_{raw} - \text{LowPass}(x_{raw}) $$
2.  **Zero Crossing:** Detect when $x_{AC}$ changes sign.
3.  **Period Calculation:** $T = t_{current} - t_{last\_crossing}$.
4.  **Frequency:** $F = \frac{1}{2T}$ (Half-cycle) or average over full cycles.

---

## 2. Implementation Specification

### Component A: Core Engine (`FFBEngine.h`)

#### 1. New Struct: `BiquadNotch`
Define this helper struct to encapsulate the filter logic.

```cpp
struct BiquadNotch {
    // Coefficients
    double b0, b1, b2, a1, a2;
    // State history (Inputs x, Outputs y)
    double x1 = 0.0, x2 = 0.0;
    double y1 = 0.0, y2 = 0.0;

    // Update coefficients based on dynamic frequency
    void Update(double center_freq, double sample_rate, double Q) {
        // Safety: Clamp frequency to Nyquist (sample_rate / 2) and min 1Hz
        center_freq = (std::max)(1.0, (std::min)(center_freq, sample_rate * 0.49));
        
        const double PI = 3.14159265358979323846;
        double omega = 2.0 * PI * center_freq / sample_rate;
        double sn = std::sin(omega);
        double cs = std::cos(omega);
        double alpha = sn / (2.0 * Q);

        double a0 = 1.0 + alpha;
        
        // Calculate and Normalize
        b0 = 1.0 / a0;
        b1 = (-2.0 * cs) / a0;
        b2 = 1.0 / a0;
        a1 = (-2.0 * cs) / a0;
        a2 = (1.0 - alpha) / a0;
    }

    // Apply filter to single sample
    double Process(double in) {
        double out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        
        // Shift history
        x2 = x1; x1 = in;
        y2 = y1; y1 = out;
        
        return out;
    }
    
    void Reset() {
        x1 = x2 = y1 = y2 = 0.0;
    }
};
```

#### 2. Class Members (`FFBEngine`)
Add these variables to the `FFBEngine` class.

```cpp
public:
    // Settings
    bool m_flatspot_suppression = false;
    float m_notch_q = 2.0f; // Default Q-Factor

    // Diagnostics
    double m_debug_freq = 0.0; // Estimated frequency for GUI

private:
    // Filter Instance
    BiquadNotch m_notch_filter;

    // Frequency Estimator State
    double m_freq_est_timer = 0.0;
    double m_last_crossing_time = 0.0;
    double m_torque_ac_smoothed = 0.0; // For High-Pass
    double m_prev_ac_torque = 0.0;
```

#### 3. Logic Integration (`calculate_force`)
Insert this logic **immediately after** reading `game_force` and before any other processing.

```cpp
// 1. Frequency Estimator Logic
// ---------------------------
// Isolate AC component (Vibration) using simple High Pass (remove DC offset)
// Alpha for HPF: fast smoothing to get the "average" center
double alpha_hpf = dt / (0.1 + dt); 
m_torque_ac_smoothed += alpha_hpf * (game_force - m_torque_ac_smoothed);
double ac_torque = game_force - m_torque_ac_smoothed;

// Detect Zero Crossing (Sign change)
// Add hysteresis (0.05 Nm) to avoid noise triggering
if ((m_prev_ac_torque < -0.05 && ac_torque > 0.05) || 
    (m_prev_ac_torque > 0.05 && ac_torque < -0.05)) {
    
    double now = data->mElapsedTime;
    double period = now - m_last_crossing_time;
    
    // Sanity check period (e.g., 1Hz to 200Hz)
    if (period > 0.005 && period < 1.0) {
        // Half-cycle * 2 = Full Cycle Period
        // Or if we detect both crossings, period is half. 
        // Let's assume we detect every crossing (2 per cycle).
        double inst_freq = 1.0 / (period * 2.0);
        
        // Smooth the readout for GUI
        m_debug_freq = m_debug_freq * 0.9 + inst_freq * 0.1;
    }
    m_last_crossing_time = now;
}
m_prev_ac_torque = ac_torque;


// 2. Dynamic Notch Filter Logic
// ---------------------------
if (m_flatspot_suppression) {
    // Calculate Wheel Frequency
    double car_speed = std::abs(data->mLocalVel.z);
    
    // Get radius (convert cm to m)
    // Use Front Left as reference
    double radius = (double)fl.mStaticUndeflectedRadius / 100.0;
    if (radius < 0.1) radius = 0.33; // Safety fallback
    
    double circumference = 2.0 * 3.14159265 * radius;
    
    // Avoid divide by zero
    double wheel_freq = (circumference > 0.0) ? (car_speed / circumference) : 0.0;

    // Only filter if moving fast enough (> 1Hz)
    if (wheel_freq > 1.0) {
        // Update filter coefficients
        m_notch_filter.Update(wheel_freq, 1.0/dt, (double)m_notch_q);
        
        // Apply filter
        game_force = m_notch_filter.Process(game_force);
    } else {
        // Reset filter state when stopped to prevent "ringing" on start
        m_notch_filter.Reset();
    }
}
```

#### 4. Snapshot Update
Update `FFBSnapshot` in `FFBEngine.h` to carry the debug value.

```cpp
struct FFBSnapshot {
    // ... existing ...
    float debug_freq; // Add this
};

// In calculate_force snapshot block:
snap.debug_freq = (float)m_debug_freq;
```

---

### Component B: Configuration (`src/Config.cpp`)

Ensure the new settings persist.

1.  **Update `Preset` struct in `Config.h`**:
    *   Add `bool flatspot_suppression` and `float notch_q`.
    *   Add `SetFlatspot(bool, float)` method.
    *   Update `Apply()` and `UpdateFromEngine()`.

2.  **Update `Config::Save`**:
    *   `file << "flatspot_suppression=" << engine.m_flatspot_suppression << "\n";`
    *   `file << "notch_q=" << engine.m_notch_q << "\n";`

3.  **Update `Config::Load`**:
    *   Parse the keys.

4.  **Update `LoadPresets`**:
    *   Initialize defaults (False, 2.0) for existing presets.

---

### Component C: GUI (`src/GuiLayer.cpp`)

#### 1. Tuning Window (`DrawTuningWindow`)
Add a new section for Signal Filtering.

```cpp
if (ImGui::TreeNode("Signal Filtering")) {
    // Existing Smoothing
    FloatSetting("SoP Smoothing", &engine.m_sop_smoothing_factor, 0.0f, 1.0f, "%.2f");
    
    ImGui::Separator();
    
    // Notch Filter Controls
    BoolSetting("Dynamic Flatspot Suppression", &engine.m_flatspot_suppression);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Removes vibrations linked to wheel speed (e.g. flat spots)\nusing a zero-latency tracking filter.");
    }

    if (engine.m_flatspot_suppression) {
        ImGui::Indent();
        FloatSetting("Notch Width (Q)", &engine.m_notch_q, 0.5f, 10.0f, "Q: %.1f");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Controls filter precision.\n2.0 = Balanced.\n>2.0 = Narrower (Surgical).\n<2.0 = Wider (Softer).");
        }
        ImGui::Unindent();
    }
    ImGui::TreePop();
}
```

#### 2. Debug Window (`DrawDebugWindow`)
Add the frequency readout to the "Raw Telemetry" section.

```cpp
// Inside Header C
ImGui::TextColored(ImVec4(1,1,0,1), "Signal Analysis");
// Get latest snapshot
float freq = snapshots.back().debug_freq;
ImGui::Text("Est. Vibration Freq: %.1f Hz", freq);

// Calculate theoretical freq for comparison
float speed = snapshots.back().raw_car_speed;
// Assuming 0.33m radius for display approximation or pass radius in snapshot
float theoretical = speed / (2.0f * 3.14159f * 0.33f); 
ImGui::Text("Theoretical Wheel Freq: %.1f Hz", theoretical);
```

---

## 3. Verification Strategy

### Unit Tests (`tests/test_ffb_engine.cpp`)

1.  **`test_notch_filter_attenuation`**:
    *   Instantiate `BiquadNotch`.
    *   Update with $F_c = 10Hz, F_s = 400Hz, Q = 2.0$.
    *   Feed input: $\sin(2\pi \cdot 10 \cdot t)$ (Target frequency).
    *   **Assert:** Output amplitude should be near 0 (< 0.1).
    *   Feed input: $\sin(2\pi \cdot 2 \cdot t)$ (Low frequency steering).
    *   **Assert:** Output amplitude should be near 1.0 (Pass-through).

2.  **`test_frequency_estimator`**:
    *   Feed `calculate_force` a pure sine wave at 15Hz via `mSteeringShaftTorque`.
    *   Run for 1 second (400 frames).
    *   **Assert:** `m_debug_freq` converges to $15.0 \pm 1.0$ Hz.

### Manual Verification
1.  Drive car at constant speed (e.g., 100 kph).
2.  Observe "Est. Vibration Freq" in Debug Window.
3.  Enable "Dynamic Flatspot Suppression".
4.  Verify vibration disappears from wheel but "Steering Torque" graph still shows road details.
