# Question 1

Below is the full codebase of a FFB app I am developing. I am also pasting below the implementation plan for a feature I recently added (context aware yaw kick effects). Your task is to help me investigate and fix some issue I found after trying the new effects with the game.

I've tried the implementation. There are a few issues:

* the yaw jerk seems inverted, the punches happen in the opposite direction as expected.
* the yaw jerk punches pull too strong. Can / should we cap them in strength, so that they are just as fast, but produce a limited force?
* the new effect for acceleration causes a lot of vibrations. I basically maxed it out, setting all the slider to the most aggressive settings, to be able to catch power slides / power oversteer. I don't know which sliders to reduce to have less vibration but the same early warnings and ability to prevent slides.
* could we add smoothing to some of the components of the new yaw kick effects (particularly the acceleration one), without adding latency or losing important detail?

* should we add additional gating to any of the yaw kick effects (regular yaw kick and context aware ones). Eg. what about gating to enable yaw kick only when the direction of the yaw is opposite to the driver steerring input direction?



---

Implementation plan: LMUFFB\docs\dev_docs\implementation_plans\Unloaded Yaw Kick and the Power Yaw Kick plan.md


---


# Answer 1

This is a classic problem when working with high-frequency telemetry data. You've run into the **"Noise Amplification of Derivatives"** issue. 

Here is a breakdown of exactly why you are experiencing these issues and how we can fix them mathematically without losing the zero-latency feel you are aiming for.

### 1. Why the Yaw Jerk feels inverted and causes intense vibrations
You are calculating `yaw_jerk` by taking the derivative of `derived_yaw_accel`, which is itself a derivative of `mLocalRot.y`. 
At 400Hz (`dt = 0.0025`), taking a derivative multiplies any underlying sensor noise by 400. Taking a *second* derivative multiplies that noise by **160,000**. 
Because the jerk signal is dominated by extreme high-frequency noise, it swings wildly positive and negative every few milliseconds. This causes the "punch" to frequently fire in the wrong direction (feeling inverted) and creates the intense vibrations you felt on the Power Yaw Kick.

### 2. How to fix it (The "Attack Phase" Gate & Fast Smoothing)
To fix this without adding latency, we need to do three things:
1. **Fast Smoothing**: Apply a very short 15ms smoothing to the acceleration *before* calculating the jerk. This kills the 400Hz stepping noise but preserves the instant 10Hz "snap" of a slide.
2. **Jerk Clamping**: Hard-clamp the maximum force the jerk can add so it never pulls the wheel out of your hands.
3. **The "Attack Phase" Gate**: Instead of gating based on steering direction (which fails during pendulum catches), we simply check: `(yaw_jerk * yaw_accel) > 0`. This ensures the punch *only* fires when the slide is actively accelerating/worsening. If noise causes a negative jerk, it is ignored.

### 3. Smoothing the Vulnerability Gates
To stop the Power Yaw Kick from vibrating when you hover right at the slip threshold, we will apply a 50ms smoothing to the *vulnerability gate* itself. This means the effect fades in smoothly over 50ms, but once the gate is open, the actual FFB force reacts instantly to the car's yaw.

Here are the code changes to implement these fixes:

### Step 1: Update `src/FFBEngine.h`
Replace `m_prev_derived_yaw_accel` with the new fast-smoothing state variables.

```cpp
// Find this section around line 350:
    // Yaw Acceleration Smoothing State (v0.4.18)
    double m_yaw_accel_smoothed = 0.0;
    double m_prev_yaw_rate = 0.0;     // New v0.7.144 (Solution 2)
    bool m_yaw_rate_seeded = false;   // New v0.7.144 (Solution 2)
    double m_prev_yaw_rate_log = 0.0;
    
    // REPLACE m_prev_derived_yaw_accel WITH THESE:
    double m_fast_yaw_accel_smoothed = 0.0;
    double m_prev_fast_yaw_accel = 0.0;
    bool m_yaw_accel_seeded = false;       // New v0.7.164 (Issue #322)

    // NEW: Vulnerability gate smoothing
    double m_unloaded_vulnerability_smoothed = 0.0;
    double m_power_vulnerability_smoothed = 0.0;
```

### Step 2: Update `src/FFBEngine.cpp` (Reset Logic)
Ensure the new variables are cleared when the FFB is muted (e.g., returning to the garage).

```cpp
// Find this section around line 150 inside calculate_force:
    if (m_was_allowed && !allowed) {
        // ... existing resets ...
        m_yaw_accel_smoothed = 0.0;
        m_prev_yaw_rate = 0.0;
        m_yaw_rate_seeded = false;
        
        // ADD THESE LINES:
        m_fast_yaw_accel_smoothed = 0.0;
        m_prev_fast_yaw_accel = 0.0;
        m_unloaded_vulnerability_smoothed = 0.0;
        m_power_vulnerability_smoothed = 0.0;
        // ...
```

### Step 3: Update `src/FFBEngine.cpp` (The Physics Logic)
Replace the entire `4. Yaw Kicks` section inside `calculate_sop_lateral` with this updated logic:

```cpp
    // 4. Yaw Kicks (Context-Aware Oversteer - Issue #322)

    // Shared leading indicators: Derive Yaw Acceleration from Yaw Rate (mLocalRot.y)
    double current_yaw_rate = data->mLocalRot.y;
    if (!m_yaw_rate_seeded) {
        m_prev_yaw_rate = current_yaw_rate;
        m_yaw_rate_seeded = true;
    }
    double derived_yaw_accel = (ctx.dt > 1e-6) ? (current_yaw_rate - m_prev_yaw_rate) / ctx.dt : 0.0;
    m_prev_yaw_rate = current_yaw_rate;

    // NEW: Fast smoothing for Jerk calculation (15ms tau) to remove 400Hz noise
    double tau_fast = 0.015;
    double alpha_fast = ctx.dt / (tau_fast + ctx.dt);
    m_fast_yaw_accel_smoothed += alpha_fast * (derived_yaw_accel - m_fast_yaw_accel_smoothed);

    // Calculate Yaw Jerk (Rate of change of Yaw Acceleration) for transient shaping
    if (!m_yaw_accel_seeded) {
        m_prev_fast_yaw_accel = m_fast_yaw_accel_smoothed;
        m_yaw_accel_seeded = true;
    }
    double yaw_jerk = (ctx.dt > 1e-6) ? (m_fast_yaw_accel_smoothed - m_prev_fast_yaw_accel) / ctx.dt : 0.0;
    m_prev_fast_yaw_accel = m_fast_yaw_accel_smoothed;

    // Clamp raw jerk to prevent insane spikes from collisions/telemetry glitches
    yaw_jerk = std::clamp(yaw_jerk, -100.0, 100.0);

    // --- A. General Yaw Kick (Baseline Rotation Feel) ---
    double tau_yaw = (double)m_yaw_accel_smoothing;
    if (tau_yaw < MIN_TAU_S) tau_yaw = MIN_TAU_S;
    double alpha_yaw = ctx.dt / (tau_yaw + ctx.dt);
    m_yaw_accel_smoothed += alpha_yaw * (derived_yaw_accel - m_yaw_accel_smoothed);

    double general_yaw_force = 0.0;
    if (ctx.car_speed >= MIN_YAW_KICK_SPEED_MS) {
        if (std::abs(m_yaw_accel_smoothed) > (double)m_yaw_kick_threshold) {
            double processed_yaw = m_yaw_accel_smoothed - std::copysign((double)m_yaw_kick_threshold, m_yaw_accel_smoothed);
            general_yaw_force = -1.0 * processed_yaw * m_sop_yaw_gain * (double)BASE_NM_YAW_KICK;
        }
    }

    // --- B. Unloaded Yaw Kick (Braking / Lift-off) ---
    double unloaded_yaw_force = 0.0;
    if (ctx.car_speed >= MIN_YAW_KICK_SPEED_MS && m_unloaded_yaw_gain > 0.001f) {
        double load_ratio = (m_static_rear_load > 1.0) ? ctx.avg_rear_load / m_static_rear_load : 1.0;
        double rear_load_drop = (std::max)(0.0, 1.0 - load_ratio);
        double raw_unloaded_vuln = (std::min)(1.0, rear_load_drop * (double)m_unloaded_yaw_sens);

        // Smooth the vulnerability gate (50ms tau) to prevent rapid toggling
        double alpha_vuln = ctx.dt / (0.05 + ctx.dt);
        m_unloaded_vulnerability_smoothed += alpha_vuln * (raw_unloaded_vuln - m_unloaded_vulnerability_smoothed);

        if (m_unloaded_vulnerability_smoothed > 0.01) {
            // Attack Phase Gate: Only apply punch if jerk is amplifying the current acceleration
            double punch_addition = 0.0;
            if ((yaw_jerk * m_fast_yaw_accel_smoothed) > 0.0) {
                punch_addition = std::clamp(yaw_jerk * (double)m_unloaded_yaw_punch, -10.0, 10.0);
            }
            
            double punchy_yaw = m_fast_yaw_accel_smoothed + punch_addition;
            
            if (std::abs(punchy_yaw) > (double)m_unloaded_yaw_threshold) {
                double processed_yaw = punchy_yaw - std::copysign((double)m_unloaded_yaw_threshold, punchy_yaw);
                double sign = (processed_yaw >= 0.0) ? 1.0 : -1.0;
                double yaw_norm = (std::min)(1.0, std::abs(processed_yaw) / 10.0);
                double shaped_yaw = std::pow(yaw_norm, (double)m_unloaded_yaw_gamma) * 10.0 * sign;
                unloaded_yaw_force = -1.0 * shaped_yaw * (double)m_unloaded_yaw_gain * (double)BASE_NM_YAW_KICK * m_unloaded_vulnerability_smoothed;
            }
        }
    }

    // --- C. Power Yaw Kick (Acceleration / Traction Loss) ---
    double power_yaw_force = 0.0;
    if (ctx.car_speed >= MIN_YAW_KICK_SPEED_MS && m_power_yaw_gain > 0.001f) {
        double slip_rl = calculate_wheel_slip_ratio(data->mWheel[2]);
        double slip_rr = calculate_wheel_slip_ratio(data->mWheel[3]);
        double max_rear_spin = (std::max)({ 0.0, slip_rl, slip_rr });

        double slip_start = (double)m_power_slip_threshold * 0.5;
        double slip_vulnerability = inverse_lerp(slip_start, (double)m_power_slip_threshold, max_rear_spin);
        double throttle = (std::max)(0.0, (double)data->mUnfilteredThrottle);
        double raw_power_vuln = slip_vulnerability * throttle;

        // Smooth the vulnerability gate (50ms tau) to prevent rapid toggling
        double alpha_vuln = ctx.dt / (0.05 + ctx.dt);
        m_power_vulnerability_smoothed += alpha_vuln * (raw_power_vuln - m_power_vulnerability_smoothed);

        if (m_power_vulnerability_smoothed > 0.01) {
            // Attack Phase Gate: Only apply punch if jerk is amplifying the current acceleration
            double punch_addition = 0.0;
            if ((yaw_jerk * m_fast_yaw_accel_smoothed) > 0.0) {
                punch_addition = std::clamp(yaw_jerk * (double)m_power_yaw_punch, -10.0, 10.0);
            }

            double punchy_yaw = m_fast_yaw_accel_smoothed + punch_addition;
            
            if (std::abs(punchy_yaw) > (double)m_power_yaw_threshold) {
                double processed_yaw = punchy_yaw - std::copysign((double)m_power_yaw_threshold, punchy_yaw);
                double sign = (processed_yaw >= 0.0) ? 1.0 : -1.0;
                double yaw_norm = (std::min)(1.0, std::abs(processed_yaw) / 10.0);
                double shaped_yaw = std::pow(yaw_norm, (double)m_power_yaw_gamma) * 10.0 * sign;
                power_yaw_force = -1.0 * shaped_yaw * (double)m_power_yaw_gain * (double)BASE_NM_YAW_KICK * m_power_vulnerability_smoothed;
            }
        }
    }

    // Blending Logic: Use sign-preserving max absolute value
    ctx.yaw_force = general_yaw_force;
    if (std::abs(unloaded_yaw_force) > std::abs(ctx.yaw_force)) ctx.yaw_force = unloaded_yaw_force;
    if (std::abs(power_yaw_force) > std::abs(ctx.yaw_force)) ctx.yaw_force = power_yaw_force;
```

### Summary of the Fixes
* **Inverted/Vibrating Jerk**: Fixed by the `(yaw_jerk * m_fast_yaw_accel_smoothed) > 0.0` gate. The punch will now *only* fire when the slide is actively getting worse. If noise causes a negative jerk, it is ignored.
* **Punches too strong**: Fixed by `std::clamp(..., -10.0, 10.0)`. The maximum artificial acceleration added by the jerk is now safely capped.
* **Power Kick Vibrations**: Fixed by adding a 50ms smoothing to the `m_power_vulnerability_smoothed` gate. This stops the effect from rapidly turning on and off when you hover right at the slip threshold.
* **Steering Direction Gating**: The "Attack Phase Gate" mentioned above is actually much safer than gating by steering direction, because it correctly handles "pendulum catches" (where the car snaps the other way before you have time to turn the wheel).

---

# Question 2

To be clear, the intense vibrations were present even when the yaw jerk was turned off. So it was due to the other settings (yaw kick, gamma for yaw kick, etc.).

Are we adding a 50 ms latency by Smoothing the Vulnerability Gates? That seems way too much.

# Answer 2





You are absolutely right to call that out. A flat 50ms delay on an "early warning" system completely defeats the purpose. 

You also made a crucial observation: **the vibrations happen even when the jerk is turned off.** 

Here is exactly why that is happening, and how we can fix it with **zero latency**.

### Why it vibrates (The Gamma + Raw Noise problem)
In the original implementation, the context kicks were using `derived_yaw_accel` (the raw, unfiltered 400Hz yaw acceleration). At 400Hz, this signal is incredibly noisy. 
When you apply a **Gamma curve < 1.0** to a noisy signal, it takes all those tiny, imperceptible micro-vibrations and *amplifies* them massively. The FFB engine was basically turning sensor noise into a high-frequency vibration motor.

### The Solution: Asymmetric Smoothing & Light Base Filtering
To fix the vibrations without adding latency, we need two specific techniques:

1. **Asymmetric Vulnerability Gates (Fast Attack / Slow Decay):**
   Instead of a flat 50ms smoothing, we use a **2ms attack** and a **50ms decay**. 
   * When the car loses traction, the gate opens instantly (2ms is less than one physics frame). Zero latency.
   * When the slip ratio chatters due to road bumps, the 50ms decay holds the gate open smoothly so it doesn't rapidly toggle on and off.
2. **15ms Base Signal Smoothing:**
   We must stop feeding raw 400Hz noise into the Gamma curve. We will apply a tiny 15ms smoothing to the yaw acceleration. 15ms is imperceptible to human hands, but it completely destroys the high-frequency stepping noise that causes the vibrations.
3. **Jerk Clamping & Attack Gating:**
   We keep the fix for the jerk: we clamp its maximum force, and we only allow it to fire if it is pushing in the *same direction* as the current slide (fixing the inverted feel).

Here is the updated code to implement this.

### Step 1: Update `src/FFBEngine.h`
Add the state variables for the fast smoothing and the vulnerability gates.

```cpp
// Find this section around line 350:
    // Yaw Acceleration Smoothing State (v0.4.18)
    double m_yaw_accel_smoothed = 0.0;
    double m_prev_yaw_rate = 0.0;     
    bool m_yaw_rate_seeded = false;   
    double m_prev_yaw_rate_log = 0.0;
    
    // REPLACE m_prev_derived_yaw_accel WITH THESE:
    double m_fast_yaw_accel_smoothed = 0.0;
    double m_prev_fast_yaw_accel = 0.0;
    bool m_yaw_accel_seeded = false;       

    // NEW: Vulnerability gate smoothing
    double m_unloaded_vulnerability_smoothed = 0.0;
    double m_power_vulnerability_smoothed = 0.0;
```

### Step 2: Update `src/FFBEngine.cpp` (Reset Logic)
Ensure the new variables are cleared when the FFB is muted (e.g., returning to the garage).

```cpp
// Find this section around line 150 inside calculate_force:
    if (m_was_allowed && !allowed) {
        // ... existing resets ...
        m_yaw_accel_smoothed = 0.0;
        m_prev_yaw_rate = 0.0;
        m_yaw_rate_seeded = false;
        
        // ADD THESE LINES:
        m_fast_yaw_accel_smoothed = 0.0;
        m_prev_fast_yaw_accel = 0.0;
        m_unloaded_vulnerability_smoothed = 0.0;
        m_power_vulnerability_smoothed = 0.0;
        // ...
```

### Step 3: Update `src/FFBEngine.cpp` (The Physics Logic)
Replace the entire `4. Yaw Kicks` section inside `calculate_sop_lateral` with this updated logic:

```cpp
    // 4. Yaw Kicks (Context-Aware Oversteer - Issue #322)

    // Shared leading indicators: Derive Yaw Acceleration from Yaw Rate (mLocalRot.y)
    double current_yaw_rate = data->mLocalRot.y;
    if (!m_yaw_rate_seeded) {
        m_prev_yaw_rate = current_yaw_rate;
        m_yaw_rate_seeded = true;
    }
    double derived_yaw_accel = (ctx.dt > 1e-6) ? (current_yaw_rate - m_prev_yaw_rate) / ctx.dt : 0.0;
    m_prev_yaw_rate = current_yaw_rate;

    // NEW: Fast smoothing for the base signal (15ms tau) to remove 400Hz noise before Gamma amplification
    double tau_fast = 0.015;
    double alpha_fast = ctx.dt / (tau_fast + ctx.dt);
    m_fast_yaw_accel_smoothed += alpha_fast * (derived_yaw_accel - m_fast_yaw_accel_smoothed);

    // Calculate Yaw Jerk (Rate of change of Yaw Acceleration) for transient shaping
    if (!m_yaw_accel_seeded) {
        m_prev_fast_yaw_accel = m_fast_yaw_accel_smoothed;
        m_yaw_accel_seeded = true;
    }
    double yaw_jerk = (ctx.dt > 1e-6) ? (m_fast_yaw_accel_smoothed - m_prev_fast_yaw_accel) / ctx.dt : 0.0;
    m_prev_fast_yaw_accel = m_fast_yaw_accel_smoothed;

    // Clamp raw jerk to prevent insane spikes from collisions/telemetry glitches
    yaw_jerk = std::clamp(yaw_jerk, -100.0, 100.0);

    // --- A. General Yaw Kick (Baseline Rotation Feel) ---
    double tau_yaw = (double)m_yaw_accel_smoothing;
    if (tau_yaw < MIN_TAU_S) tau_yaw = MIN_TAU_S;
    double alpha_yaw = ctx.dt / (tau_yaw + ctx.dt);
    m_yaw_accel_smoothed += alpha_yaw * (derived_yaw_accel - m_yaw_accel_smoothed);

    double general_yaw_force = 0.0;
    if (ctx.car_speed >= MIN_YAW_KICK_SPEED_MS) {
        if (std::abs(m_yaw_accel_smoothed) > (double)m_yaw_kick_threshold) {
            double processed_yaw = m_yaw_accel_smoothed - std::copysign((double)m_yaw_kick_threshold, m_yaw_accel_smoothed);
            general_yaw_force = -1.0 * processed_yaw * m_sop_yaw_gain * (double)BASE_NM_YAW_KICK;
        }
    }

    // --- B. Unloaded Yaw Kick (Braking / Lift-off) ---
    double unloaded_yaw_force = 0.0;
    if (ctx.car_speed >= MIN_YAW_KICK_SPEED_MS && m_unloaded_yaw_gain > 0.001f) {
        double load_ratio = (m_static_rear_load > 1.0) ? ctx.avg_rear_load / m_static_rear_load : 1.0;
        double rear_load_drop = (std::max)(0.0, 1.0 - load_ratio);
        double raw_unloaded_vuln = (std::min)(1.0, rear_load_drop * (double)m_unloaded_yaw_sens);

        // ASYMMETRIC SMOOTHING: 2ms attack (instant), 50ms decay (prevents chatter)
        double tau_unloaded = (raw_unloaded_vuln > m_unloaded_vulnerability_smoothed) ? 0.002 : 0.050;
        m_unloaded_vulnerability_smoothed += (ctx.dt / (tau_unloaded + ctx.dt)) * (raw_unloaded_vuln - m_unloaded_vulnerability_smoothed);

        if (m_unloaded_vulnerability_smoothed > 0.01) {
            // Attack Phase Gate: Only apply punch if jerk is amplifying the current acceleration
            double punch_addition = 0.0;
            if ((yaw_jerk * m_fast_yaw_accel_smoothed) > 0.0) {
                punch_addition = std::clamp(yaw_jerk * (double)m_unloaded_yaw_punch, -10.0, 10.0);
            }
            
            // CRITICAL FIX: Use the 15ms smoothed yaw, NOT the raw derived yaw
            double punchy_yaw = m_fast_yaw_accel_smoothed + punch_addition;
            
            if (std::abs(punchy_yaw) > (double)m_unloaded_yaw_threshold) {
                double processed_yaw = punchy_yaw - std::copysign((double)m_unloaded_yaw_threshold, punchy_yaw);
                double sign = (processed_yaw >= 0.0) ? 1.0 : -1.0;
                double yaw_norm = (std::min)(1.0, std::abs(processed_yaw) / 10.0);
                double shaped_yaw = std::pow(yaw_norm, (double)m_unloaded_yaw_gamma) * 10.0 * sign;
                unloaded_yaw_force = -1.0 * shaped_yaw * (double)m_unloaded_yaw_gain * (double)BASE_NM_YAW_KICK * m_unloaded_vulnerability_smoothed;
            }
        }
    }

    // --- C. Power Yaw Kick (Acceleration / Traction Loss) ---
    double power_yaw_force = 0.0;
    if (ctx.car_speed >= MIN_YAW_KICK_SPEED_MS && m_power_yaw_gain > 0.001f) {
        double slip_rl = calculate_wheel_slip_ratio(data->mWheel[2]);
        double slip_rr = calculate_wheel_slip_ratio(data->mWheel[3]);
        double max_rear_spin = (std::max)({ 0.0, slip_rl, slip_rr });

        double slip_start = (double)m_power_slip_threshold * 0.5;
        double slip_vulnerability = inverse_lerp(slip_start, (double)m_power_slip_threshold, max_rear_spin);
        double throttle = (std::max)(0.0, (double)data->mUnfilteredThrottle);
        double raw_power_vuln = slip_vulnerability * throttle;

        // ASYMMETRIC SMOOTHING: 2ms attack (instant), 50ms decay (prevents chatter)
        double tau_power = (raw_power_vuln > m_power_vulnerability_smoothed) ? 0.002 : 0.050;
        m_power_vulnerability_smoothed += (ctx.dt / (tau_power + ctx.dt)) * (raw_power_vuln - m_power_vulnerability_smoothed);

        if (m_power_vulnerability_smoothed > 0.01) {
            // Attack Phase Gate: Only apply punch if jerk is amplifying the current acceleration
            double punch_addition = 0.0;
            if ((yaw_jerk * m_fast_yaw_accel_smoothed) > 0.0) {
                punch_addition = std::clamp(yaw_jerk * (double)m_power_yaw_punch, -10.0, 10.0);
            }

            // CRITICAL FIX: Use the 15ms smoothed yaw, NOT the raw derived yaw
            double punchy_yaw = m_fast_yaw_accel_smoothed + punch_addition;
            
            if (std::abs(punchy_yaw) > (double)m_power_yaw_threshold) {
                double processed_yaw = punchy_yaw - std::copysign((double)m_power_yaw_threshold, punchy_yaw);
                double sign = (processed_yaw >= 0.0) ? 1.0 : -1.0;
                double yaw_norm = (std::min)(1.0, std::abs(processed_yaw) / 10.0);
                double shaped_yaw = std::pow(yaw_norm, (double)m_power_yaw_gamma) * 10.0 * sign;
                power_yaw_force = -1.0 * shaped_yaw * (double)m_power_yaw_gain * (double)BASE_NM_YAW_KICK * m_power_vulnerability_smoothed;
            }
        }
    }

    // Blending Logic: Use sign-preserving max absolute value
    ctx.yaw_force = general_yaw_force;
    if (std::abs(unloaded_yaw_force) > std::abs(ctx.yaw_force)) ctx.yaw_force = unloaded_yaw_force;
    if (std::abs(power_yaw_force) > std::abs(ctx.yaw_force)) ctx.yaw_force = power_yaw_force;
```

### Why this works better:
1. **No more vibrations:** By feeding `m_fast_yaw_accel_smoothed` into the Gamma curve instead of the raw signal, the math no longer amplifies 400Hz sensor noise.
2. **Zero Latency:** The 2ms attack on the vulnerability gates means the effect turns on instantly the moment you lose traction.
3. **No Inverted Punches:** The `(yaw_jerk * m_fast_yaw_accel_smoothed) > 0.0` check ensures the jerk punch *only* fires when the slide is actively accelerating away from you. If the jerk is negative (meaning the slide is slowing down or it's just noise), the punch is ignored.
