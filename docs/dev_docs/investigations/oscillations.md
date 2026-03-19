

## The Issue: Stationary Oscillations in the Pits
**The User's Clue:** *"Stationary Oscillations in the Pit are big... Hands-Off Protection in Moza software (...) with high enough inertia removes oscillations."*

**The Cause:**
When the car is stationary, the game's physics engine still outputs a centering spring force (simulating tire scrub/friction against the pavement). Because the physical steering wheel has mass, it gets pulled to the center, overshoots, and creates a pendulum effect (oscillation).

To stop a pendulum, you need **Damping** (friction/viscosity). 


## User Requirements

Many users don't like damping because it masks details of FFB and reduce responsiveness.
If possible, I want a damping that is not into effect while driving, but only in stationary situations. So we can avoid oscillations when stationary, without reducing FFB details and responsiveness while driving.

What about adding inertia instead of damping?
Note that the same argument from above applies to inertia: it can reduce FFB detail and responsiveness, so in this case I want to apply it only when stationary to reduce oscillations, and never when driving.

## The Fix
Here is the breakdown of **Inertia vs. Damping**, and exactly how we can implement a **"Stationary Only"** option that completely turns itself off the moment you start driving.

### Inertia vs. Damping: Which is better for oscillations?
From a physics standpoint, **Damping is the correct tool to stop oscillations.**
*   **Inertia** simulates mass. If you add inertia to a swinging pendulum (your steering wheel), you don't stop it from swinging; you just make it swing slower and hit the end-stops with *more force*. 
*   **Damping** simulates friction (viscosity). It actively removes energy from the system. When the wheel tries to swing past the center, damping resists the movement and absorbs the energy, stopping the oscillation dead in its tracks.

*(Note: Wheelbase software like Moza often bundles heavy damping and friction under the name "Hands-Off Protection" or "Inertia", which is why users associate those terms with fixing oscillations).*

### The Solution: Speed-Gated Stationary Damping
We can create a new setting called **Stationary Damping**. 

Because `lmuFFB` already has a `speed_gate` system (used to fade out road textures when you park), we can use the **exact inverse** of that gate. 
* At `0 km/h`, the damping is at **100%** (stopping the pit-box oscillations).
* As you accelerate, it smoothly fades out.
* By the time you reach `~15 km/h` (your upper speed gate), the damping is at **0%**. It completely removes itself from the physics loop, leaving your driving FFB pure and responsive.

Here is how to implement this cleanly into the codebase:

#### 1. Add the variable to `src/ffb/FFBEngine.h`
Find the Gyro settings (around line 100) and add the new variable:
```cpp
    float m_gyro_gain;
    float m_gyro_smoothing;
    float m_stationary_damping = 0.0f; // <--- ADD THIS
```

#### 2. Update the Math in `src/ffb/FFBEngine.cpp`
Find the `calculate_gyro_damping` function (around line 650). We will rename the internal logic slightly to handle both the driving gyro and the stationary damping:

```cpp
// Helper: Calculate Gyroscopic & Stationary Damping
void FFBEngine::calculate_gyro_damping(const TelemInfoV01* data, FFBCalculationContext& ctx) {
    // 1. Calculate Steering Velocity (rad/s)
    float range = data->mPhysicalSteeringWheelRange;

    if (m_rest_api_enabled && range <= 0.0f) {
        float fallback_deg = RestApiProvider::Get().GetFallbackRangeDeg();
        if (fallback_deg > 0.0f) {
            range = fallback_deg * ((float)PI / 180.0f);
        }
    }

    if (range <= 0.0f) range = (float)DEFAULT_STEERING_RANGE_RAD;
    double steer_angle = data->mUnfilteredSteering * (range / DUAL_DIVISOR);
    double steer_vel = (steer_angle - m_prev_steering_angle) / ctx.dt;
    m_prev_steering_angle = steer_angle;
    
    // 2. Alpha Smoothing
    double tau_gyro = (double)m_gyro_smoothing;
    if (tau_gyro < MIN_TAU_S) tau_gyro = MIN_TAU_S;
    double alpha_gyro = ctx.dt / (tau_gyro + ctx.dt);
    m_steering_velocity_smoothed += alpha_gyro * (steer_vel - m_steering_velocity_smoothed);
    
    // 3. Calculate Forces
    // Gyro effect increases with wheel RPM (car speed)
    double driving_gyro = m_gyro_gain * (ctx.car_speed / GYRO_SPEED_SCALE);
    
    // Stationary damping uses the INVERSE of the speed gate.
    // ctx.speed_gate is 0.0 when parked, and 1.0 when driving.
    double stationary_blend = 1.0 - ctx.speed_gate; 
    double stationary_friction = m_stationary_damping * stationary_blend;

    // Combine them and apply opposing force to velocity
    ctx.gyro_force = -1.0 * m_steering_velocity_smoothed * (driving_gyro + stationary_friction);
}
```

#### 3. Add the UI Slider in `src/gui/GuiLayer_Win32.cpp`
Find where `Gyro Damping` is drawn (around line 450) and add the new slider right below it:
```cpp
        FloatSetting("Gyro Damping", &engine.m_gyro_gain, 0.0f, 1.0f, FormatDecoupled(engine.m_gyro_gain, FFBEngine::BASE_NM_GYRO_DAMPING), Tooltips::GYRO_DAMPING);

        // ADD THIS:
        FloatSetting("Stationary Damping", &engine.m_stationary_damping, 0.0f, 1.0f, FormatDecoupled(engine.m_stationary_damping, FFBEngine::BASE_NM_GYRO_DAMPING), 
            "Applies friction ONLY when the car is stopped to prevent oscillations in the pits.\nFades out completely to 0% as you start driving.");

        FloatSetting("  Gyro Smooth", &engine.m_gyro_smoothing, 0.000f, 0.050f, "%.3f s",
```

#### 4. Save it to the Config (`src/core/Config.h` & `Config.cpp`)
To ensure the user's setting saves, add it to the `Preset` struct in `Config.h`:
```cpp
    float gyro_gain = 0.0f;
    float stationary_damping = 0.0f; // <--- ADD THIS
```
In `Config.h` under `Apply(FFBEngine& engine)`:
```cpp
    engine.m_gyro_gain = (std::max)(0.0f, gyro_gain);
    engine.m_stationary_damping = (std::max)(0.0f, stationary_damping); // <--- ADD THIS
```
In `Config.h` under `UpdateFromEngine`:
```cpp
    gyro_gain = engine.m_gyro_gain;
    stationary_damping = engine.m_stationary_damping; // <--- ADD THIS
```
In `Config.cpp` under `SyncPhysicsLine`:
```cpp
    if (key == "gyro_gain") { engine.m_gyro_gain = (std::min)(1.0f, std::stof(value)); return true; }
    if (key == "stationary_damping") { engine.m_stationary_damping = (std::min)(1.0f, std::stof(value)); return true; } // <--- ADD THIS
```
In `Config.cpp` under `WritePresetFields` and `Save`:
```cpp
    file << "gyro_gain=" << p.gyro_gain << "\n";
    file << "stationary_damping=" << p.stationary_damping << "\n"; // <--- ADD THIS
```

### Why this is the perfect solution:
1. **It fixes the user's problem:** Setting `Stationary Damping` to `0.5` or `1.0` will act like thick molasses on the steering wheel when parked in the garage, completely killing the pendulum effect.
2. **It respects FFB purity:** Because it multiplies by `(1.0 - ctx.speed_gate)`, the math guarantees that once the car reaches the upper speed gate (default 18 km/h), this variable becomes `0.0`. It physically cannot mask details while driving.
3. **It mimics reality:** In a real race car, turning the wheel while parked on sticky slick tires requires immense effort (static friction). Once the car rolls, the steering lightens up (rolling friction). This perfectly simulates that transition. 

*(Don't forget to also apply the `SAFETY_SLEW_RESTRICTED = 2.0f` fix from the previous message to fix the "clack" when they hit the ESC key!)*