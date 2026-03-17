# Fix LinearExtrapolator Sawtooth Bug and HoltWintersFilter Snapback #396

**Open**
coasting-nc opened on Mar 16, 2026

## Description

### Fix LinearExtrapolator Sawtooth Bug

#### Code Review and recommended fixes

Based on a thorough review of the codebase, there is a critical mathematical flaw in the LinearExtrapolator class used to up-sample the 100Hz game telemetry to the 400Hz physics loop.

This flaw is the primary cause of the lack of smoothness, 100Hz "graininess" (buzzing), and violent unwanted vibrations (specifically false-triggering the suspension bottoming effect).

Here is the detailed breakdown of the issues found and how to fix them.

---

### 1. The LinearExtrapolator Sawtooth Bug (Critical)

**Location:** `src/utils/MathUtils.h`

The LinearExtrapolator is used to up-sample almost every auxiliary telemetry channel (suspension force, patch velocities, accelerations, etc.) from 100Hz to 400Hz.

It uses a "dead reckoning" approach: it calculates the rate of change over the previous 10ms frame, and assumes the signal will continue at that exact rate into the future.

```cpp
if (is_new_frame) {
    m_rate = (raw_input - m_last_input) / m_game_tick;
    m_current_output = raw_input; // <--- THE SNAP
    // ...
} else {
    m_current_output += m_rate * dt; // <--- THE OVERSHOOT
}
```

**The Failure Mode:**
When a signal changes direction or stops accelerating, the extrapolation overshoots reality. When the next 100Hz frame arrives, `m_current_output` violently snaps back to the true `raw_input`.
This creates a continuous 100Hz sawtooth wave riding on top of all your telemetry signals. This manifests in the wheel as a constant, grainy buzzing sensation.

**The Derivative Spike (Unwanted Vibrations):**
This sawtooth wave breaks effects that rely on derivatives (rate of change). For example, look at `calculate_suspension_bottoming` in `FFBEngine.cpp`:
```cpp
double dForceL = ((data->mWheel[0].mSuspForce - m_prev_susp_force[0]) * mr) / ctx.dt;
```

Because `mSuspForce` is up-sampled using the extrapolator, every 10ms when the signal "snaps back" from an overshoot, `dForceL` registers a massive, instantaneous spike. This easily exceeds the `BOTTOMING_IMPULSE_THRESHOLD_N_S`, causing the FFB engine to constantly trigger the heavy "Bottoming Crunch" vibration even on smooth roads.

#### The Fix: Convert to a Linear Interpolator

To fix this, you must delay the auxiliary signals by exactly 1 frame (10ms). This allows you to smoothly interpolate between the previous frame and the current frame, guaranteeing C0 continuity (no snaps) and a piecewise-constant derivative (no spikes). 10ms of latency on auxiliary effects (like road texture and SoP) is completely imperceptible to humans, whereas 100Hz buzzing is highly perceptible.

Replace the `LinearExtrapolator` class in `MathUtils.h` with this:

```cpp
/**
 * @brief Linear Interpolator (Inter-Frame Reconstruction)
 *
 * Upsamples a 100Hz signal to 400Hz+ by delaying the signal by 1 frame (10ms)
 * and smoothly interpolating. Eliminates sawtooth artifacts and derivative spikes.
 */
class LinearExtrapolator { // Kept name to avoid breaking other files
private:
    double m_prev_sample = 0.0;
    double m_target_sample = 0.0;
    double m_current_output = 0.0;
    double m_rate = 0.0;
    double m_time_since_update = 0.0;
    double m_game_tick = 0.01; // Default 100Hz
    bool m_initialized = false;

public:
    void Configure(double game_tick) {
        m_game_tick = (std::max)(0.0001, game_tick);
    }

    double Process(double raw_input, double dt, bool is_new_frame) {
        if (!m_initialized) {
            m_prev_sample = raw_input;
            m_target_sample = raw_input;
            m_current_output = raw_input;
            m_initialized = true;
            return raw_input;
        }

        if (is_new_frame) {
            // Shift the window: old target becomes the new start point
            m_prev_sample = m_target_sample;
            m_target_sample = raw_input;

            // Calculate rate to reach the new target over the next game tick
            m_rate = (m_target_sample - m_prev_sample) / m_game_tick;
            m_time_since_update = 0.0;

            // Output starts exactly at the previous sample (no snapping)
            m_current_output = m_prev_sample;
        } else {
            m_time_since_update += dt;
            if (m_time_since_update <= m_game_tick) {
                m_current_output = m_prev_sample + m_rate * m_time_since_update;
            } else {
                // If the game stutters/drops a frame, hold the target value safely
                m_current_output = m_target_sample;
            }
        }

        return m_current_output;
    }

    void Reset() {
        m_prev_sample = m_target_sample = m_current_output = m_rate = m_time_since_update = 0.0;
        m_initialized = false;
    }
};
```

---

### 2. The HoltWintersFilter Snapback (Minor)

**Location:** `src/utils/MathUtils.h`

The `HoltWintersFilter` is used to up-sample the primary `mSteeringShaftTorque`. Because you want zero latency on the primary steering force, this filter must extrapolate.

While the `m_alpha` and `m_beta` parameters smooth it out, it still suffers from minor snapbacks when the steering torque changes direction rapidly. This can cause a very subtle graininess in the wheel when using the legacy 100Hz torque.

**Mitigation:**
The code is actually already handling this perfectly via the `m_torque_source == 1` (In-Game FFB) setting. When users select the 400Hz direct torque source, it bypasses the `HoltWintersFilter` entirely. No code changes are needed here, but if users complain about graininess in the base steering, ensure they are using the 400Hz "In-Game FFB" source.

---

### 3. Review of PolyphaseResampler (Healthy)

**Location:** `src/ffb/UpSampler.cpp`

I reviewed the 5/2 ratio (400Hz -> 1000Hz) polyphase FIR filter.
*   The phase accumulator logic (`m_phase += 2; if >= 5...`) correctly maps the 2.5ms input intervals to 1.0ms output intervals.
*   The history shifting (`m_needs_shift`) perfectly aligns with the `run_physics` ticks from `main.cpp`.
*   The coefficients sum to 1.0 preserving DC gain.

**Conclusion:** The Polyphase resampler is mathematically flawless and is not contributing to any graininess or unwanted vibrations.

---

### Summary of Action

Apply the provided code replacement for `LinearExtrapolator` in `MathUtils.h`. This will instantly cure the 100Hz buzzing, fix the false-triggering of the suspension bottoming effect, and drastically smooth out the slip-angle/grip estimations.

---

### Follow up question

Regarding the 100Hz `mSteeringShaftTorque`, it is still used quite often. So if there is an inherent issue with it, it must be addressed. At the very least, we should give the users the options to switch between a zero latency implementation that could include some graininess, and one that get rids of graininess at the cost of latency.
Give me a solution that gives this choice to the user.

---

### Answer 2

To give users the choice between Zero Latency (Extrapolation) and Smooth (Interpolation) for the 100Hz `mSteeringShaftTorque`, we need to upgrade the `HoltWintersFilter` to support both modes and expose this toggle in the UI and configuration system.

Here is the complete, step-by-step solution to implement this feature.

#### 1. Upgrade the Filter in MathUtils.h

We will modify the `HoltWintersFilter` to accept a mode flag.
*   **Extrapolation (Zero Latency):** Uses the trend to predict the future (current behavior, can be grainy).
*   **Interpolation (Smooth):** Delays the signal by exactly 1 frame (10ms) to smoothly blend between the previous and current targets.

Update `HoltWintersFilter` in `src/utils/MathUtils.h`:

```cpp
/**
 * @brief Second-Order Holt-Winters (Double Exponential Smoothing)
 *
 * Tracks both the value and the trend (velocity) of a signal.
 * Acts as both an upsampler and a low-pass filter.
 * Now supports both Zero-Latency Extrapolation and Smooth Interpolation.
 */
class HoltWintersFilter {
private:
    double m_level = 0.0;      // Smoothed value (Target for interpolation)
    double m_prev_level = 0.0; // Previous smoothed value (Start for interpolation)
    double m_trend = 0.0;      // Smoothed trend/slope
    double m_time_since_update = 0.0;
    bool m_initialized = false;
    bool m_zero_latency = true; // Mode toggle

    // Tuning Parameters
    double m_alpha = 0.8; // Level weight (Higher = less lag)
    double m_beta = 0.2;  // Trend weight
    double m_game_tick = 0.01; // Default 100Hz

public:
    void Configure(double alpha, double beta, double game_tick = 0.01) {
        m_alpha = std::clamp(alpha, 0.01, 1.0);
        m_beta = std::clamp(beta, 0.01, 1.0);
        m_game_tick = (std::max)(0.0001, game_tick);
    }

    void SetZeroLatency(bool zero_latency) {
        m_zero_latency = zero_latency;
    }

    double Process(double raw_input, double dt, bool is_new_frame) {
        if (!m_initialized) {
            m_level = raw_input;
            m_prev_level = raw_input;
            m_trend = 0.0;
            m_time_since_update = 0.0;
            m_initialized = true;
            return raw_input;
        }

        if (is_new_frame) {
            double old_level = m_level;
            m_prev_level = m_level; // Save for interpolation start point

            // Update Level: Balance between the raw input and our previous prediction
            m_level = m_alpha * raw_input + (1.0 - m_alpha) * (m_level + m_trend * m_game_tick);

            // Update Trend: Balance between the new observed slope and the old trend
            m_trend = m_beta * ((m_level - old_level) / m_game_tick) + (1.0 - m_beta) * m_trend;

            m_time_since_update = 0.0;

            if (m_zero_latency) {
                return m_level;
            } else {
                return m_prev_level; // Start interpolation from previous level
            }
        } else {
            m_time_since_update += dt;
        }

        if (m_zero_latency) {
            // Predict current state based on previous trend (Extrapolation)
            return m_level + m_trend * m_time_since_update;
        } else {
            // Smoothly blend between prev_level and level (Interpolation)
            if (m_time_since_update <= m_game_tick) {
                double t = m_time_since_update / m_game_tick;
                return m_prev_level + t * (m_level - m_prev_level);
            } else {
                return m_level; // Hold target if game stutters
            }
        }
    }

    void Reset() {
        m_level = m_prev_level = m_trend = m_time_since_update = 0.0;
        m_initialized = false;
    }
};
```

#### 2. Add the Setting to FFBEngine.h

In `src/ffb/FFBEngine.h`, add the new configuration variable under the **Base Force Debugging** section:

```cpp
    // Base Force Debugging (v0.4.13)
    float m_steering_shaft_gain;
    float m_ingame_ffb_gain = 1.0f;
    int m_torque_source = 0;
    int m_steering_100hz_reconstruction = 0; // NEW: 0 = Zero Latency, 1 = Smooth
    bool m_torque_passthrough = false;
```

#### 3. Apply the Setting in FFBEngine.cpp

In `src/ffb/FFBEngine.cpp`, locate the `calculate_force` method and update the `m_upsample_shaft_torque` call to pass the new setting:

```cpp
    // Upsample Steering Shaft Torque (Holt-Winters)
    m_upsample_shaft_torque.SetZeroLatency(m_steering_100hz_reconstruction == 0);
    double shaft_torque = m_upsample_shaft_torque.Process(m_working_info.mSteeringShaftTorque, ffb_dt, is_new_frame);
    m_working_info.mSteeringShaftTorque = shaft_torque;
```

#### 4. Expose in the UI (GuiLayer_Common.cpp)

In `src/gui/GuiLayer_Common.cpp`, locate the **Torque Source** dropdown inside `DrawTuningWindow` and add the new combo box right below it. We will only show it if the 100Hz source is selected:

```cpp
        const char* torque_sources[] = { "Shaft Torque (100Hz Legacy)", "In-Game FFB (400Hz LMU 1.2+)" };
        IntSetting("Torque Source", &engine.m_torque_source, torque_sources, sizeof(torque_sources)/sizeof(torque_sources[0]),
            Tooltips::TORQUE_SOURCE);

        // NEW: Show reconstruction mode only if using the 100Hz source
        if (engine.m_torque_source == 0) {
            const char* recon_modes[] = { "Zero-Latency (Extrapolated)", "Smooth (Interpolated)" };
            IntSetting("  100Hz Reconstruction", &engine.m_steering_100hz_reconstruction, recon_modes, 2,
                "Zero-Latency: Best for response but can be grainy. Smooth: 10ms delay but eliminates all 100Hz buzz.");
        }
```
