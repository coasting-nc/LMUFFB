


The reason the **SoP Self-Aligning Torque** effect spikes violently on kerbs is due to its current mathematical formula in `src/ffb/FFBEngine.cpp` (inside `calculate_sop_lateral`):

```cpp
// Current Formula
double rear_slip_angle = m_grip_diag.rear_slip_angle;
ctx.calc_rear_lat_force = rear_slip_angle * ctx.avg_rear_load * REAR_TIRE_STIFFNESS_COEFFICIENT;
```

When you hit a kerb, two things happen simultaneously in the telemetry:
1. **`ctx.avg_rear_load` spikes massively** due to the vertical impact of the suspension compressing.
2. **`rear_slip_angle` spikes** because the tire is physically deflected sideways by the kerb geometry (a spike in `mLateralPatchVel`).

Because these two spiked values are multiplied together, the resulting `rear_torque` explodes, sending a harsh jolt to the Direct Drive wheel.

Here is how you can fix this **without adding any latency** and **without losing slide-catching details**.

---

### 1. The Best Code Fix: Dynamic Load Capping & Slip Soft-Clipping
To fix this without adding latency (which time-based smoothing filters would do), you should apply instantaneous mathematical bounds to the inputs. 

Modify `FFBEngine::calculate_sop_lateral` in `src/ffb/FFBEngine.cpp` (around line 450):

```cpp
    // 3. Rear Aligning Torque (v0.4.9)
    double rear_slip_angle = m_grip_diag.rear_slip_angle;
    
    // FIX A: Cap the dynamic load. 
    // Normal cornering weight transfer rarely exceeds 1.5x the static weight of the car.
    // Kerb strikes can spike to 3x-4x. Capping it preserves cornering feel but chops off kerb spikes.
    double max_effective_load = m_static_rear_load * 1.5; 
    double effective_rear_load = std::min(ctx.avg_rear_load, max_effective_load);

    // FIX B: Soft-clip the slip angle (Simulate Pneumatic Trail falloff).
    // Real tires don't produce infinite aligning torque; it drops off at high slip angles.
    // std::tanh keeps the response linear and reactive near the center, but flattens massive kerb deflections.
    double normalized_slip = rear_slip_angle / (m_optimal_slip_angle + 0.001);
    double effective_slip = m_optimal_slip_angle * std::tanh(normalized_slip);

    // Calculate with the sanitized, zero-latency variables
    ctx.calc_rear_lat_force = effective_slip * effective_rear_load * REAR_TIRE_STIFFNESS_COEFFICIENT;
    ctx.calc_rear_lat_force = (std::max)(-MAX_REAR_LATERAL_FORCE, (std::min)(MAX_REAR_LATERAL_FORCE, ctx.calc_rear_lat_force));
    
    // Torque = Force * Aligning_Lever
    ctx.rear_torque = -ctx.calc_rear_lat_force * REAR_ALIGN_TORQUE_COEFFICIENT * m_rear_align_effect;
```

**Why this works perfectly:**
* **Zero Latency:** `std::min` and `std::tanh` are instantaneous math operations. There is no time-based filter (like an EMA or Low Pass Filter) being added, so the wheel will react to a slide on the exact same millisecond it did before.
* **Preserves Detail:** Normal cornering keeps the load under `1.5x` and the slip angle within the linear portion of the `tanh` curve, meaning the standard FFB feel is 100% identical.

---

### 2. Alternative Code Fix: Suspension Velocity "Gating"
If you want to completely mute the effect *only* when a kerb is struck, you can reuse the suspension velocity logic that the app already uses for ABS/Lockup bump rejection.

```cpp
    // Calculate how fast the rear suspension is compressing (m/s)
    double susp_vel_rl = std::abs(data->mWheel[2].mVerticalTireDeflection - m_prev_vert_deflection[2]) / ctx.dt;
    double susp_vel_rr = std::abs(data->mWheel[3].mVerticalTireDeflection - m_prev_vert_deflection[3]) / ctx.dt;
    double max_rear_susp_vel = std::max(susp_vel_rl, susp_vel_rr);

    // If suspension is moving faster than 1.0 m/s (a sharp kerb strike)
    double kerb_attenuation = 1.0;
    if (max_rear_susp_vel > 1.0) {
        // Fade out the effect based on how violent the bump is
        kerb_attenuation = std::max(0.0, 1.0 - (max_rear_susp_vel - 1.0)); 
    }

    ctx.calc_rear_lat_force = rear_slip_angle * ctx.avg_rear_load * REAR_TIRE_STIFFNESS_COEFFICIENT;
    ctx.calc_rear_lat_force = std::clamp(ctx.calc_rear_lat_force, -MAX_REAR_LATERAL_FORCE, MAX_REAR_LATERAL_FORCE);
    
    ctx.rear_torque = -ctx.calc_rear_lat_force * REAR_ALIGN_TORQUE_COEFFICIENT * m_rear_align_effect * kerb_attenuation;
```

---

### 3. What users can do right now (UI Tweaks)
If you need to advise users on how to tune this out using the *current* compiled build, they have two options, though both come with slight compromises:

1. **Increase `Slip Angle Smoothing` (e.g., from `0.002s` to `0.025s`)**
   * *Why:* This applies a Low Pass Filter to the `rear_slip_angle`. It will round off the sharp, high-frequency spikes caused by kerbs.
   * *Compromise:* It adds a few milliseconds of latency to the slide detection.
2. **Rely more on `Yaw Kick` and less on `Rear Align Torque`**
   * *Why:* `Yaw Kick` is driven by the chassis gyroscope (`mLocalRotAccel.y`), which is heavily isolated from vertical suspension spikes. `Rear Align Torque` is driven directly by the tires, which take the brunt of the kerb impacts.
   * *Compromise:* Yaw Kick is an impulse force (it snaps and fades), whereas Rear Align is a sustained force. The feeling of holding a long drift will be slightly reduced.