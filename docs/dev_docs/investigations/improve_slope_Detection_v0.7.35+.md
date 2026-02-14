Based on the analysis of the code and the provided diagnostic reports, the Slope Detection feature is failing primarily due to **mathematical instability (singularities)** and **logic gaps during steady-state cornering**.

Here is the breakdown of the issues and the specific code fixes required.

### Analysis of the Failure

1.  **Singularities & Explosions (The "McLaren Report" Issue)**
    *   **Symptom:** The McLaren report shows `Slope Std Dev: 8.10` and `Singularities: 8268 events`.
    *   **Cause:** The current formula `m_slope_current = dG_dt / protected_denom` is numerically unstable. Even with the protection `std::max(0.005, abs_dAlpha)`, when `dAlpha` is small (e.g., 0.005) and `dG` is moderate (e.g., 0.5 due to a bump), the result is `100.0`. The code then clamps this to `20.0`, creating a square-wave "banging" effect between -20 and 20.
    *   **Evidence:** The "Binary Residence: 98.3%" in the report means the value is almost always stuck at the clamp limits.

2.  **The Steady-State Flaw (Low Active Time)**
    *   **Symptom:** `Active Time: 13.8%`.
    *   **Cause:** The code only calculates slope when `dAlpha/dt > threshold`. During a long corner (like at Paul Ricard), the driver holds the steering wheel steady. `dAlpha/dt` drops to near zero.
    *   **Result:** The code enters the `else` block: `m_slope_current` decays to 0.0. Since 0.0 slope implies "Linear/Grip", the FFB feels heavy (full grip) exactly when it should feel light (understeer), because the *rate of change* stopped, even if the car is sliding.

3.  **Noise Amplification**
    *   **Symptom:** `Zero-Crossing Rate: 8.69 Hz`.
    *   **Cause:** Taking the derivative ($d/dt$) of raw telemetry naturally amplifies high-frequency noise. The Savitzky-Golay filter helps, but the inputs (`lateral_g` and `slip_angle`) are not pre-smoothed enough before entering the derivative buffer.

---

### The Solution

We need to replace the direct division with a **Robust Least Squares** approach (Projected Slope) to eliminate singularities, and implement a **Hold-and-Decay** logic to handle steady-state cornering.

#### Step 1: Modify `FFBEngine.h`

You need to update the `FFBEngine` class to add state variables for the "Hold" logic and input smoothing.

**In `src/FFBEngine.h`, inside the `FFBEngine` class `private` section:**

Add these new member variables:
```cpp
    // ... existing slope members ...
    double m_slope_current = 0.0;
    double m_slope_grip_factor = 1.0;
    double m_slope_smoothed_output = 1.0;
    
    // NEW: Input Smoothing State
    double m_slope_lat_g_smoothed = 0.0;
    double m_slope_slip_smoothed = 0.0;

    // NEW: Steady State Logic
    double m_slope_hold_timer = 0.0;
    static constexpr double SLOPE_HOLD_TIME = 0.25; // Hold value for 250ms before decaying
```

#### Step 2: Rewrite `calculate_slope_grip`

Replace the entire `calculate_slope_grip` function in `src/FFBEngine.h` with this robust implementation.

```cpp
    // Helper: Calculate Grip Factor from Slope - v0.7.22 FIX
    // Replaces direct division with Robust Projected Slope and adds Steady-State Hold
    double calculate_slope_grip(double lateral_g, double slip_angle, double dt) {
        
        // 1. Input Pre-Smoothing (Low Pass Filter)
        // Reduces high-frequency telemetry noise before derivative calculation.
        // Tau = 0.01s (100Hz cutoff) is fast enough to catch slides but kills jitter.
        const double input_tau = 0.01; 
        double alpha_in = dt / (input_tau + dt);
        m_slope_lat_g_smoothed += alpha_in * (lateral_g - m_slope_lat_g_smoothed);
        m_slope_slip_smoothed += alpha_in * (std::abs(slip_angle) - m_slope_slip_smoothed);

        // 2. Update Buffers with SMOOTHED data
        m_slope_lat_g_buffer[m_slope_buffer_index] = m_slope_lat_g_smoothed;
        m_slope_slip_buffer[m_slope_buffer_index] = m_slope_slip_smoothed;
        m_slope_buffer_index = (m_slope_buffer_index + 1) % SLOPE_BUFFER_MAX;
        if (m_slope_buffer_count < SLOPE_BUFFER_MAX) m_slope_buffer_count++;

        // 3. Calculate Derivatives (Savitzky-Golay)
        double dG_dt = calculate_sg_derivative(m_slope_lat_g_buffer, m_slope_buffer_count, m_slope_sg_window, dt);
        double dAlpha_dt = calculate_sg_derivative(m_slope_slip_buffer, m_slope_buffer_count, m_slope_sg_window, dt);

        // Store for logging
        m_slope_dG_dt = dG_dt;
        m_slope_dAlpha_dt = dAlpha_dt;

        // 4. Robust Slope Estimation (Projected Slope)
        // Instead of slope = y/x, we use slope = (x*y) / (x*x + epsilon)
        // This mathematically projects the vector onto the axis, eliminating division by zero.
        // It naturally dampens the output when dAlpha (x) is small.
        
        double numerator = dG_dt * dAlpha_dt;
        double denominator = (dAlpha_dt * dAlpha_dt) + 0.0001; // Epsilon prevents /0
        double raw_slope = numerator / denominator;

        // 5. Steady-State Logic (Hold & Decay)
        // If dAlpha is significant, we trust the new calculation.
        // If dAlpha is near zero (steady cornering), we HOLD the last known slope.
        
        if (std::abs(dAlpha_dt) > (double)m_slope_alpha_threshold) {
            // Active transient - update slope immediately
            m_slope_current = std::clamp(raw_slope, -20.0, 20.0);
            m_slope_hold_timer = SLOPE_HOLD_TIME; // Reset hold timer
        } else {
            // Steady state - Hold, then Decay
            if (m_slope_hold_timer > 0.0) {
                m_slope_hold_timer -= dt;
                // Keep m_slope_current as is (Hold)
            } else {
                // Timer expired, slowly decay to 0 (Linear Region assumption)
                // Decay rate is slower here to prevent "heavy wheel" mid-corner
                m_slope_current += (double)m_slope_decay_rate * dt * (0.0 - m_slope_current);
            }
        }

        // 6. Calculate Grip Factor
        double current_grip_factor = 1.0;
        double confidence = calculate_slope_confidence(dAlpha_dt);

        // Map slope to loss (InverseLerp)
        // m_slope_min_threshold (e.g. -0.3): Start of loss
        // m_slope_max_threshold (e.g. -2.0): Max loss
        double loss_percent = inverse_lerp((double)m_slope_min_threshold, (double)m_slope_max_threshold, m_slope_current);
        
        // Apply confidence
        // Note: We allow confidence to be lower during hold, but we don't zero it out completely
        // to ensure we feel the understeer during the steady state.
        if (m_slope_hold_timer > 0.0) confidence = 1.0; // Trust the held value

        current_grip_factor = 1.0 - (loss_percent * 0.8 * confidence);

        // Apply Floor (Safety)
        current_grip_factor = (std::max)(0.2, (std::min)(1.0, current_grip_factor));

        // 7. Output Smoothing
        double alpha = dt / ((double)m_slope_smoothing_tau + dt);
        alpha = (std::max)(0.001, (std::min)(1.0, alpha));
        m_slope_smoothed_output += alpha * (current_grip_factor - m_slope_smoothed_output);

        return m_slope_smoothed_output;
    }
```

### Summary of Changes

1.  **Input Pre-Smoothing:** Added `m_slope_lat_g_smoothed` and `m_slope_slip_smoothed`. This cleans up the 8.69Hz noise before it gets differentiated, making `dG_dt` and `dAlpha_dt` much cleaner.
2.  **Robust Division:** Replaced `dG / dAlpha` with `(dG * dAlpha) / (dAlpha^2 + epsilon)`. This mathematically prevents the "Singularities" (8268 events) reported in the McLaren log. It naturally tends toward zero when `dAlpha` is small, rather than exploding to infinity.
3.  **Hold Timer:** Added `m_slope_hold_timer`. When the driver holds the wheel steady in a corner (`dAlpha` drops below threshold), the code now **remembers** that the car was understeering 100ms ago and maintains the lightness in the wheel, rather than immediately restoring full grip. This fixes the "Low Active Time" issue.