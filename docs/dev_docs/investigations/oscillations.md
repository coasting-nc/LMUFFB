

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

We must ensure that **Stationary Damping** is 100% decoupled from Gyro Damping, and that it physically cannot interfere with the FFB while driving.

Here is the exact implementation to make them completely independent. We will separate the math into two distinct forces: `ctx.gyro_force` (which turns off in the menus) and `ctx.stationary_damping_force` (which stays active in the menus/pits but fades to exactly zero when driving).

### 1. Update the Context & Engine Header (`src/ffb/FFBEngine.h`)
First, we add the new variable and a dedicated force output to the context so they don't share the same pipeline.

Find `struct FFBCalculationContext` (around line 40) and add the new force:
```cpp
    double scrub_drag_force = 0.0;
    double gyro_force = 0.0;
    double stationary_damping_force = 0.0; // <--- ADD THIS
    double avg_rear_grip = 0.0;
```

Find the `FFBEngine` class settings (around line 100) and add the new setting:
```cpp
    float m_sop_yaw_gain;
    float m_gyro_gain;
    float m_gyro_smoothing;
    float m_stationary_damping = 0.0f; // <--- ADD THIS (Independent of gyro_gain)
    float m_yaw_accel_smoothing;
```

### 2. Update the Math (`src/ffb/FFBEngine.cpp`)
Find the `calculate_gyro_damping` function (around line 650). We will split the calculation so that `m_gyro_gain` and `m_stationary_damping` are completely independent.

```cpp
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
    
    // 2. Alpha Smoothing (Shared velocity tracker, works perfectly even if smoothing is 0)
    double tau_gyro = (double)m_gyro_smoothing;
    if (tau_gyro < MIN_TAU_S) tau_gyro = MIN_TAU_S;
    double alpha_gyro = ctx.dt / (tau_gyro + ctx.dt);
    m_steering_velocity_smoothed += alpha_gyro * (steer_vel - m_steering_velocity_smoothed);
    
    // 3. DRIVING GYRO (Scales UP with speed. If m_gyro_gain is 0, this is 0.0)
    double driving_gyro = m_gyro_gain * (ctx.car_speed / GYRO_SPEED_SCALE);
    ctx.gyro_force = -1.0 * m_steering_velocity_smoothed * driving_gyro;

    // 4. STATIONARY DAMPING (Scales DOWN with speed. If m_stationary_damping is 0, this is 0.0)
    // ctx.speed_gate is 0.0 at 0km/h, and 1.0 at the upper threshold (e.g., 15km/h)
    double stationary_blend = 1.0 - ctx.speed_gate; 
    ctx.stationary_damping_force = -1.0 * m_steering_velocity_smoothed * m_stationary_damping * stationary_blend;
}
```

### 3. Apply the Force & Protect it in Menus (`src/ffb/FFBEngine.cpp`)
Now we need to add `ctx.stationary_damping_force` to the final output. 

Find the `SUMMATION` section in `calculate_force` (around line 465) and add it to the `structural_sum`:
```cpp
    // --- 6. SUMMATION ---
    double structural_sum = output_force + ctx.sop_base_force + ctx.lat_load_force + ctx.rear_torque + ctx.yaw_force + ctx.gyro_force + ctx.stationary_damping_force + ctx.scrub_drag_force;
```

**CRITICAL STEP:** Just above that, look at the `!allowed` block (around line 445). This block mutes forces when the user is in the ESC menu or Garage. We must **preserve** the stationary damping here so the wheel doesn't oscillate while parked in the menus!

```cpp
    // v0.7.78 FIX: Support stationary/garage soft lock (Issue #184)
    // If not allowed (e.g. in garage or AI driving), mute all forces EXCEPT Soft Lock and Stationary Damping.
    if (!allowed) {
        output_force = 0.0;
        ctx.sop_base_force = 0.0;
        ctx.rear_torque = 0.0;
        ctx.yaw_force = 0.0;
        ctx.gyro_force = 0.0;
        ctx.scrub_drag_force = 0.0;
        ctx.road_noise = 0.0;
        ctx.slide_noise = 0.0;
        ctx.spin_rumble = 0.0;
        ctx.bottoming_crunch = 0.0;
        ctx.abs_pulse_force = 0.0;
        ctx.lockup_rumble = 0.0;
        // NOTE: ctx.soft_lock_force and ctx.stationary_damping_force are PRESERVED.

        base_input = 0.0;
    }
```

*(Regarding telemetry logs, we should keep the gyro damping and stationary damping separate. Therefore we would need to add stationary damping to the telemetry. We could skip adding it in this patch, since we are just fixing the oscillations, and having stationary damping in the telemetry logs might not be a priority. In this way we simplify the work for this patch. We will consider adding stationary damping to the telemetry in a future patch, if necessary for debugging)*.
    

### 4. Add the UI Slider (`src/gui/GuiLayer_Win32.cpp`)
Find where `Gyro Damping` is drawn (around line 450) and add the new slider right below it:
```cpp
        FloatSetting("Gyro Damping", &engine.m_gyro_gain, 0.0f, 1.0f, FormatDecoupled(engine.m_gyro_gain, FFBEngine::BASE_NM_GYRO_DAMPING), Tooltips::GYRO_DAMPING);

        // ADD THIS NEW SLIDER
        FloatSetting("Stationary Damping", &engine.m_stationary_damping, 0.0f, 1.0f, FormatDecoupled(engine.m_stationary_damping, FFBEngine::BASE_NM_GYRO_DAMPING), 
            "Applies friction ONLY when the car is stopped to prevent oscillations in the pits.\nFades out completely to 0% as you start driving.\nWorks independently of Gyro Damping.");

        FloatSetting("  Gyro Smooth", &engine.m_gyro_smoothing, 0.000f, 0.050f, "%.3f s",
```

### 5. Save it to the Config (`src/core/Config.h` & `Config.cpp`)
To ensure the user's setting saves to their `.ini` file:

**In `src/core/Config.h` (inside `struct Preset`):**
```cpp
    float gyro_gain = 0.0f;
    float stationary_damping = 0.0f; // <--- ADD THIS
```
*(Also in `Config.h`, update `Apply()`, `UpdateFromEngine()`, and `Equals()` to include `stationary_damping` just like `gyro_gain`)*.

**In `src/core/Config.cpp`:**
In `SyncPhysicsLine`:
```cpp
    if (key == "gyro_gain") { engine.m_gyro_gain = (std::min)(1.0f, std::stof(value)); return true; }
    if (key == "stationary_damping") { engine.m_stationary_damping = (std::min)(1.0f, std::stof(value)); return true; } // <--- ADD THIS
```
In `WritePresetFields` and `Save`:
```cpp
    file << "gyro_gain=" << p.gyro_gain << "\n";
    file << "stationary_damping=" << p.stationary_damping << "\n"; // <--- ADD THIS
```

### Why this guarantees zero interference while driving:
Because `ctx.stationary_damping_force` is multiplied by `(1.0 - ctx.speed_gate)`, the math dictates that the moment the car reaches the upper speed gate (default 18 km/h), `stationary_blend` becomes exactly `0.0`. 

Any number multiplied by `0.0` is `0.0`. It physically removes itself from the FFB summation while driving, leaving the wheel 100% raw and responsive, while completely curing the pit-box oscillations.

### Why this is the perfect solution:
1. **It fixes the user's problem:** Setting `Stationary Damping` to `0.5` or `1.0` will act like thick molasses on the steering wheel when parked in the garage, completely killing the pendulum effect.
2. **It respects FFB purity:** Because it multiplies by `(1.0 - ctx.speed_gate)`, the math guarantees that once the car reaches the upper speed gate (default 18 km/h), this variable becomes `0.0`. It physically cannot mask details while driving.
3. **It mimics reality:** In a real race car, turning the wheel while parked on sticky slick tires requires immense effort (static friction). Once the car rolls, the steering lightens up (rolling friction). This perfectly simulates that transition. 


