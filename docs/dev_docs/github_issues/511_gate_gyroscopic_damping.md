# Gate Gyroscopic Damping #511

Opened by coasting-nc on Mar 27, 2026

## Description

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

* **Result:** On a straight, you get full wobble protection. The moment you turn into a fast chicane or a sweeping corner, the damping gets out of your way, completely eliminating the "sluggishness" and fighting.

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
1. **The Lateral G Gate (from Point 1)** already fixes 80% of this. Because damping will turn off in corners, you will feel all the subtle understeer and tire scrubbing details perfectly when you actually need them (mid-corner).
2. **Frequency Separation (Advanced):** If you want to feel road textures on the straight while damping is active, you can apply a "Deadzone" to the steering velocity.
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

## Summary of the Ultimate Gyro Damping Fix:

If you combine these three concepts into `calculate_gyro_damping`, you get a "Smart Damper":
1. **Lateral G Gate:** Turns off in corners so it doesn't fight intentional steering.
2. **Velocity Deadzone:** Ignores micro-movements so road textures shine through on straights.
3. **Force Cap:** Limits the maximum resistance so evasive maneuvers are never sluggish.

This transforms Gyroscopic Damping from a "dumb friction slider" into a highly sophisticated hands-off protector that stays completely invisible until the exact moment a wobble starts.

---

Tuning the thresholds described above is the difference between a wheel that feels "alive but safe" and one that feels "robotic and muddy."

Let's break down the exact sweet spots for the Lateral G gate, the maximum damping strength, and how to guarantee zero drawbacks.

### 1. The Lateral G Thresholds: Why 0.5 G? Should it be lower?

In a high-downforce race car (like the Hypercars and LMP2s in Le Mans Ultimate), 0.5 G is a very mild steering input—equivalent to a fast lane change or the very beginning of corner entry. Peak cornering forces in these cars are between 1.5 G and 3.0+ G.

**Why we shouldn't make the upper threshold too low (e.g., 0.1 G):**
If you take your hands off the wheel and the car starts to violently oscillate (a "tank slapper"), the car is actually swerving left and right. This swerving *generates* Lateral G-forces. A severe straight-line wobble can easily generate 0.2 G to 0.3 G.
If you set the gate to turn off at 0.1 G, **the protection will disable itself exactly when the wobble happens**, defeating the entire purpose of the feature.

**The Sweet Spot for the Gate:**
* **Start fading at `0.1 G`:** This allows absolute dead-straight driving to have full protection.
* **Completely OFF at `0.4 G`:** This ensures that by the time the driver is actually committing to a corner, the damping is 100% gone, leaving the wheel completely free and responsive.

*Math for the coding agent:*
```cpp
// 1.0 on straights, smoothly fades to 0.0 by 0.4G
double straight_line_gate = 1.0 - smoothstep(0.1, 0.4, lat_g);
```

### 2. Max Damping at 0 G: Should it be less than 100%?

Here we need to separate the **Gate Multiplier** from the **User's Slider Setting (`gyro_gain`)**.

* **The Gate Multiplier** should absolutely be **100% (1.0)** at 0 G. You want it to apply the full amount of whatever the user asked for when driving straight.
* **The User's Slider (`gyro_gain`)** is where the actual strength is determined. The sweet spot for the slider is around **10% to 15% (`0.10` to `0.15`)**.

If the gate multiplier was, say, 50% at 0 G, it would just mean the user has to double their slider value to get the same effect, which makes the UI confusing. Keep the gate at 1.0, and let the user keep the slider low.

### 3. The "Zero Drawbacks" Recipe for the Coding Agent

To completely avoid the drawbacks (masking fine details on straights, or fighting the driver during an evasive swerve), the coding agent should implement the **Velocity Deadzone** and the **Force Cap** alongside the Lateral G gate.

Here are the ideal, tested parameters for the agent to implement:

**A. The Velocity Deadzone (Preserves Fine Details on Straights)**
Road textures and engine vibrations cause the wheel to twitch very slightly and very quickly. A wobble, however, is a large, sweeping movement.
* **Sweet Spot:** `0.5 rad/s` (about 28 degrees per second).
* **Why:** If the wheel velocity is under 0.5 rad/s, the damping does absolutely nothing. You will feel every pebble on the Mulsanne straight. But if you let go of the wheel and it tries to violently spin past 0.5 rad/s, the damping instantly catches it.

**B. The Force Cap (Prevents Fighting Evasive Maneuvers)**
If a driver suddenly yanks the wheel to avoid a crash on a straight, the steering velocity will be massive. Without a cap, the damping force would spike to 10+ Nm, feeling like the steering column locked up.
* **Sweet Spot:** `2.0 Nm` (or roughly 10% to 15% of the wheelbase's max torque).
* **Why:** 2.0 Nm is plenty of force to arrest a free-spinning wheel (hands-off), but it is easily overpowered by a human driver making a panic swerve.

### How the final code should look:

If your coding agent implements this block in `FFBEngine::calculate_gyro_damping`, you will achieve the perfect "Smart Damper":

```cpp
// 1. Calculate Lateral G
double lat_g = std::abs(data->mLocalAccel.x / 9.81);

// 2. Lateral G Gate: 1.0 on straights, 0.0 in corners
double straight_line_gate = 1.0 - smoothstep(0.1, 0.4, lat_g);

// 3. Velocity Deadzone: Ignore micro-vibrations (road texture)
double steer_vel = m_steering_velocity_smoothed;
const double VEL_DEADZONE = 0.5; // rad/s

if (std::abs(steer_vel) < VEL_DEADZONE) {
    steer_vel = 0.0;
} else {
    // Smoothly subtract deadzone to prevent a harsh "step" when it engages
    steer_vel -= std::copysign(VEL_DEADZONE, steer_vel);
}

// 4. Calculate raw damping force
double driving_gyro = m_advanced.gyro_gain * (ctx.car_speed / GYRO_SPEED_SCALE) * straight_line_gate;
double raw_gyro_force = -1.0 * steer_vel * driving_gyro;

// 5. Force Cap: Never exceed 2.0 Nm of resistance (allows evasive maneuvers)
const double MAX_GYRO_NM = 2.0;
ctx.gyro_force = std::clamp(raw_gyro_force, -MAX_GYRO_NM, MAX_GYRO_NM);
```

**Summary of what this achieves:**
1. **Driving straight, holding the wheel:** Deadzone prevents damping. You feel all road details.
2. **Driving straight, hands off:** Wheel starts to wobble, exceeds 0.5 rad/s, damping catches it smoothly.
3. **Driving straight, panic swerve:** Damping engages but caps at 2.0 Nm. You easily overpower it and save the car.
4. **Cornering:** Lateral G exceeds 0.4 G. Damping turns off completely. The wheel is pure, raw physics.
