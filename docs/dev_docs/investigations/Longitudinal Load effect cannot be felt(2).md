


Based on your description and an analysis of the codebase, I have identified the root cause of why you cannot feel the Longitudinal Load effect, as well as a secondary physical reason related to how the effect is designed. 

### 1. The Bug: Uncalibrated Static Load (Constant Clamping)
The longitudinal load effect relies on knowing the car's **static front weight** to calculate the weight transfer ratio (`long_load_norm`). 

In `FFBEngine.cpp`, the function `update_static_load_reference()` is responsible for learning the actual static weight of the car as you drive out of the pits. However, this function is currently **only called if Dynamic Load Normalization is enabled**:
```cpp
// v0.7.46: Longitudinal Load logic (#301)
if (m_auto_load_normalization_enabled) {
    update_static_load_reference(ctx.avg_front_load, ctx.car_speed, ctx.dt);
}
```
Because you likely have Dynamic Load Normalization disabled (it is off by default), the engine falls back to a hardcoded generic class default (e.g., 2400N for a GT3 car). If your specific car actually weighs 3500N at the front, the math thinks you are *constantly* under heavy braking. The `long_load_norm` saturates and is clamped to `1.0` permanently. 

As a result, your 1000% setting acts as a **constant 10x multiplier** on the steering force rather than a dynamic effect. Because it never changes, you don't feel any weight *transfer*.

### 2. The Physical Limitation: Straight-Line Braking
You mentioned you isolated the effect to feel it. If you tested this by driving in a straight line and slamming the brakes, **you will feel absolutely nothing**. 

Because the longitudinal load is applied as a *multiplier* to the base steering shaft torque, and the steering shaft torque is `0 Nm` when the wheels are perfectly straight, the math becomes: `0 Nm * 10.0 (multiplier) = 0 Nm`. To feel the weight transfer with a multiplier-based approach, you must be turning the wheel slightly while braking.

---

### The Fixes

Here are the modifications to fix the C++ bug and add the requested diagnostic plots, stats, and warnings to your Python Log Analyzer.

#### 1. C++ Fix: Always Learn Static Load
Modify `src/FFBEngine.cpp` to unconditionally learn the static load.

**Find this block in `FFBEngine::calculate_force` (around line 250):**
```cpp
    if (m_missing_load_frames > MISSING_LOAD_WARN_THRESHOLD) {
        // Fallback Logic
        // ...
        ctx.frame_warn_load = true;
    }

    // Peak Hold Logic
    if (m_auto_load_normalization_enabled && !seeded) {
```

**Insert the update call right before the Peak Hold Logic:**
```cpp
    if (m_missing_load_frames > MISSING_LOAD_WARN_THRESHOLD) {
        // Fallback Logic
        // ...
        ctx.frame_warn_load = true;
    }

    // ---> ADD THIS <---
    // ALWAYS learn static load reference (used by Longitudinal Load, Bottoming, and Normalization).
    // This ensures m_static_front_load is accurate for the specific car,
    // preventing the longitudinal load multiplier from being constantly clamped.
    update_static_load_reference(ctx.avg_front_load, ctx.car_speed, ctx.dt);

    // Peak Hold Logic
    if (m_auto_load_normalization_enabled && !seeded) {
```

**Then, scroll down and REMOVE the conditional call:**
```cpp
    // v0.7.63: Passthrough Logic for Direct Torque (TIC mode)
    double grip_factor_applied = m_torque_passthrough ? 1.0 : ctx.grip_factor;

    // v0.7.46: Longitudinal Load logic (#301)
    // REMOVE THESE 3 LINES:
    // if (m_auto_load_normalization_enabled) {
    //     update_static_load_reference(ctx.avg_front_load, ctx.car_speed, ctx.dt);
    // }
    
    double long_load_factor = 1.0;
```

---
