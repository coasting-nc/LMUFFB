


This is an excellent progression. By separating the "Braking/Entry" phase from the "Acceleration/Exit" phase, we are building a highly sophisticated, context-aware FFB engine. 

Power oversteer (throttle-induced oversteer) has a completely different physical mechanism than braking oversteer. Under braking, the rear tires lose grip because they lose **vertical load** (weight transfer to the front). Under acceleration, the rear tires actually *gain* vertical load, but they lose lateral grip because the engine torque forces them to exceed their **friction circle** (longitudinal slip overwhelms lateral capacity).

Here is the analysis of the physics timeline for power oversteer, followed by the proposed architecture for a **"Power Yaw Kick"** (or Traction-Loss Yaw Kick).

---

### 1. The Physics Timeline: Power-Induced Oversteer

When you apply too much throttle on corner exit, the rear tires spin up, lateral grip drops, and the car snaps. Here is the chronological order of the signals:

#### Phase A: The Vulnerability (Pre-Loss)
*   **Throttle Application (`mUnfilteredThrottle`):** The driver demands torque.
*   **Longitudinal Acceleration (Positive Gs):** Weight transfers to the *rear*. The rear suspension compresses.
*   *Status:* **Leading Indicator of Intent.** The car is accelerating, but traction hasn't broken yet. *(Note: Unlike braking, vertical load is high here, so load-based gating will fail).*

#### Phase B: The Traction Loss (The Friction Circle Exceeded)
*   **Rear Longitudinal Slip Ratio (`calculate_wheel_slip_ratio`):** The rear wheels begin spinning faster than the road surface traveling beneath them. 
*   **Wheel Angular Acceleration (`d(Rotation)/dt`):** The rear wheels spike in RPM.
*   *Status:* **Absolute Leading Indicators of Grip Loss.** The tire has saturated its longitudinal capacity, leaving zero capacity for lateral cornering forces.

#### Phase C: The Step-Out (Rotation Begins)
*   **Yaw Acceleration (`mLocalRotAccel.y`):** With the rear tires spinning and unable to hold the car's lateral momentum, the rear end steps out. The chassis *begins* to rotate.
*   *Status:* **Absolute Leading Indicator of Oversteer.** This is the exact millisecond the slide initiates.

#### Phase D: The Slide (Mid-Corner)
*   **Rear Slip Angle (`m_grip_diag.rear_slip_angle`):** The rear tires are now traveling at a significant angle relative to the car's heading.
*   **Yaw Rate (`mLocalRot.y`):** The car is visibly rotating.
*   *Status:* **Mid-Indicators.** You are now actively in a slide and need to counter-steer.

#### Phase E: The Roll (The Lagging Consequence)
*   **Lateral Acceleration (Lateral Gs):** The car's velocity vector changes.
*   **Lateral Load Transfer (`LatLoadNorm`):** The chassis rolls onto the outside rear suspension.
*   *Status:* **Lagging Indicators.** If you wait for this signal to trigger FFB, you have already spun.

---

### 2. Why "Load" is the Wrong Gate for Acceleration

For the braking effect, we used the `rear_unload_factor` because a light rear end is the root cause of the instability. 

If we try to use load for acceleration, the logic breaks: under hard acceleration, weight transfers to the rear. The rear tires are heavily loaded. If we gate our effect based on "low rear load", the effect will **never activate** when you mash the throttle. 

Instead, the root cause of power oversteer is **Wheel Spin (Positive Slip Ratio)** combined with **Throttle Application**.

---

### 3. The Solution: "Power Yaw Kick" (Traction-Loss Gating)

To catch power oversteer instantly, we need to gate a hyper-sensitive Yaw Kick using the **Rear Slip Ratio** and the **Throttle Pedal**. 

When the driver is on the throttle AND the rear wheels are spinning faster than the ground (positive slip), the car is highly vulnerable to power oversteer. If Yaw Acceleration spikes during this specific condition, we deliver an immediate, sharp FFB kick.

#### Proposed Architecture

**1. Calculate the "Power Slip Factor" (0.0 to 1.0):**
We monitor the rear wheels for positive slip (spinning faster than the road).
```cpp
// Get longitudinal slip ratio for rear wheels
double slip_rl = calculate_wheel_slip_ratio(data->mWheel[2]);
double slip_rr = calculate_wheel_slip_ratio(data->mWheel[3]);

// We only care about POSITIVE slip (wheel spin). Negative slip is braking/lockup.
double max_rear_spin = std::max({0.0, slip_rl, slip_rr});

// Map the spin to a 0.0 - 1.0 vulnerability factor.
// e.g., starts at 5% slip (0.05), reaches max vulnerability at 15% slip (0.15)
double power_slip_factor = inverse_lerp(m_power_slip_min, m_power_slip_max, max_rear_spin);
```

**2. Combine with Throttle to create the Gate:**
We multiply the slip factor by the throttle position. This ensures the effect only triggers when the driver is actively applying power (ignoring weird slip spikes from curbs when coasting).
```cpp
double throttle = std::max(0.0, data->mUnfilteredThrottle);
double power_vulnerability = power_slip_factor * throttle;
```

**3. Calculate the Power Yaw Kick (Separate Settings):**
Just like the braking kick, this gets its own ultra-low threshold and zero smoothing.
```cpp
double power_yaw_accel = 0.0;
if (std::abs(raw_yaw_accel) > m_power_yaw_threshold) {
    power_yaw_accel = raw_yaw_accel - std::copysign(m_power_yaw_threshold, raw_yaw_accel);
}

// Force = Accel * Gain * Vulnerability Gate
double power_yaw_force = -1.0 * power_yaw_accel * m_power_yaw_gain * power_vulnerability;
```

**4. The Final Blending Logic (The "Max" approach):**
Now we have three distinct Yaw Kicks: General, Braking (Unloaded), and Acceleration (Power). We don't want them to add together and cause a 30Nm explosion. We simply take the one that is currently demanding the strongest response.

```cpp
// Assuming we have calculated general_yaw_force, unloaded_yaw_force, and power_yaw_force
double final_yaw_force = general_yaw_force;

if (std::abs(unloaded_yaw_force) > std::abs(final_yaw_force)) {
    final_yaw_force = unloaded_yaw_force;
}
if (std::abs(power_yaw_force) > std::abs(final_yaw_force)) {
    final_yaw_force = power_yaw_force;
}

ctx.yaw_force = final_yaw_force;
```

---

### Summary of the New UI Settings

Under the "Rear Axle (Oversteer)" section, we would now have three distinct Yaw Kick profiles:

1.  **General Yaw Kick:** (Mid threshold, mid smoothing) - Handles general chassis rotation, trail braking, and mid-corner balance.
2.  **Unloaded Yaw Kick (Braking):** (Low threshold, zero smoothing, gated by Rear Load Drop) - Catches lift-off oversteer and rear brake lockups instantly.
3.  **Power Yaw Kick (Acceleration):** (Low threshold, zero smoothing, gated by Throttle + Rear Wheel Spin) - Catches throttle-induced oversteer the millisecond the rear tires light up.
    *   `Power Yaw Gain`: Strength of the kick when spinning the rears.
    *   `Power Yaw Threshold`: Minimum rotation needed (set very low).
    *   `Power Slip Sensitivity`: How much wheel spin is required to unlock the full kick force.

### Why this is the ultimate setup:
By separating Yaw Kick into these three context-aware buckets, you solve the classic sim-racing FFB dilemma: *"If I make the FFB sensitive enough to catch slides, it feels too twitchy and noisy on the straights."* 

With this architecture, the wheel remains calm and smooth 95% of the time, but the *instant* you touch the brakes or mash the throttle, the FFB engine dynamically lowers its thresholds and prepares to give you a lightning-fast, zero-latency kick if the chassis steps out of line.