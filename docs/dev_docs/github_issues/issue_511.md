# Gate Gyroscopic Damping #511

Open
coasting-nc opened this issue on Mar 27, 2026

## Description

Gate Gyroscopic Damping

Yes, we can absolutely "gate" or shape the Gyroscopic Damping to mitigate its downsides. In fact, that is exactly how high-end direct drive wheelbases (like Simucube) handle their internal damping filters.

Here is a breakdown of how we can implement these mitigations, why certain gates work better than others, and how to preserve fine details.

---

### 1. Can we "gate" it to only work on a straight line?

Yes, but gating by the gas pedal is dangerous.
If you only apply damping at 100% throttle, the moment you lift off the gas or hit the brakes on a straight line, the damping will instantly turn off. Braking actually shifts weight to the front, making the steering geometry highly susceptible to violent wobbles. If the damping turns off under braking, the wheel could snap out of the driver's hands.

**The Better Gate: Lateral G-Force (Cornering Detection)**
Instead of looking at the pedals, we should look at the chassis. A car is on a "straight line" when the Lateral G-Force is near zero. We can create a smooth fade-out: when driving straight (0.0 G), damping is 100%. As you turn into a corner and Lateral G builds up (e.g., past 0.5 G), the damping smoothly fades to 0%.

**How to implement this in FFBEngine::calculate_gyro_damping:**

```cpp
// 1. Calculate how much we are cornering (Lateral G)
double lat_g = std::abs(data->mLocalAccel.x / 9.81);

// 2. Create a smooth gate: 1.0 on straights (< 0.2G), fading to 0.0 in corners (> 0.8G)
double straight_line_gate = 1.0 - smoothstep(0.2, 0.8, lat_g);

// 3. Apply the gate to the gyro calculation
double driving_gyro = m_advanced.gyro_gain * (ctx.car_speed / GYRO_SPEED_SCALE) * straight_line_gate;
```

*   **Result:** On a straight, you get full wobble protection. The moment you turn into a fast chicane or a sweeping corner, the damping gets out of your way, completely eliminating the "sluggishness" and fighting.

---

### 2. Are there other ways to mitigate sluggishness?

Yes. Even with the straight-line gate, you might need to make a sudden evasive maneuver on a straight. To prevent the wheel from fighting you, we can use **Force Capping**.

Right now, if you turn the wheel extremely fast, the damping force scales to infinity. We can put a hard limit on it so it catches the start of a wobble, but never exceeds a comfortable resistance level (e.g., 2.0 Nm).

**How to implement:**

```cpp
// Calculate raw gyro force
ctx.gyro_force = -1.0 * m_steering_velocity_smoothed * driving_gyro;

// Cap the maximum resistance to 2.0 Nm so it never overpowers the driver
double max_gyro_nm = 2.0;
ctx.gyro_force = std::clamp(ctx.gyro_force, -max_gyro_nm, max_gyro_nm);
```

---

### 3. What about mitigating the masking of fine details?

To understand how to fix this, we first need to understand why damping masks details.

In your code, `ctx.gyro_force` is just added to the total FFB output. It doesn't actually erase the high-frequency road textures in the software math. The masking happens physically at the wheelbase and in the user's hands.
If the motor is constantly fighting velocity (damping), it becomes "stiff." When a tiny 50Hz road bump signal tells the motor to twitch, that twitch is immediately resisted by the damping logic, making it feel muted.

**How to feel fine details while damping is active:**

1.  **The Lateral G Gate (from Point 1)** already fixes 80% of this. Because damping will turn off in corners, you will feel all the subtle understeer and tire scrubbing details perfectly when you actually need them (mid-corner).
2.  **Frequency Separation (Advanced):** If you want to feel road textures on the straight while damping is active, you can apply a "Deadzone" to the steering velocity.
    Road vibrations cause tiny, micro-movements in the wheel. If we tell the Gyro Damping to ignore very slow or very tiny velocity changes, it won't fight the road textures.

**How to implement a Velocity Deadzone:**

```cpp
double steer_vel = m_steering_velocity_smoothed;

// Create a small deadzone (e.g., ignore velocities under 0.5 rad/s)
// This allows micro-vibrations (road textures) to pass through without triggering damping
double vel_deadzone = 0.5;
if (std::abs(steer_vel) < vel_deadzone) {
    steer_vel = 0.0;
} else {
    // Smoothly subtract the deadzone so there is no harsh "step" when it activates
    steer_vel -= std::copysign(vel_deadzone, steer_vel);
}

ctx.gyro_force = -1.0 * steer_vel * driving_gyro;
```

---

### Summary of the Ultimate Gyro Damping Fix:

If you combine these three concepts into `calculate_gyro_damping`, you get a "Smart Damper":

1.  **Lateral G Gate:** Turns off in corners so it doesn't fight intentional steering.
2.  **Velocity Deadzone:** Ignores micro-movements so road textures shine through on straights.
3.  **Force Cap:** Limits the maximum resistance so evasive maneuvers are never sluggish.

This transforms Gyroscopic Damping from a "dumb friction slider" into a highly sophisticated hands-off protector that stays completely invisible until the exact moment a wobble starts.
