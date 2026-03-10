



Based on an analysis of your codebase, there are **three distinct reasons** why you cannot feel the Longitudinal Load effect. One is a misunderstanding of how the math works when you try to "isolate" the effect, one is a bug in the code related to encrypted car telemetry, and the third is a potential UI setting override.

Here is the breakdown of why this is happening and how to fix it:

### 1. The "Isolation" Problem (Math Logic)
You mentioned: *"I also tried to isolate it from other effects, setting their slider to zero or low values."*

If you turned down the **In-Game FFB Gain** or **Steering Shaft Gain** to `0.0` to isolate the longitudinal load, you inadvertently muted the longitudinal load itself. 

In `src/FFBEngine.cpp` (around line 394), the longitudinal load is applied as a **multiplier** to the base steering force, not as an independent additive force:
```cpp
// Formula Refactor (#301): Longitudinal Load MUST remain a multiplier to maintain
// physical aligning torque correctness (zero torque in straight line despite weight shift).
double base_steer_force = (base_input * gain_to_apply) * grip_factor_applied;
double output_force = base_steer_force * dw_factor_applied; // dw_factor_applied is your long_load
```
**Why this matters:** If `gain_to_apply` is `0`, then `base_steer_force` is `0`. Multiplying `0` by a 1000% longitudinal load factor still equals `0`. To test this effect, you **must** have a base steering force active.

### 2. The Encrypted Telemetry Bug (Code Issue)
Even with base FFB active, you likely won't feel the effect on most cars because of a bug in `src/FFBEngine.cpp` (around line 365):

```cpp
// Only apply if enabled AND we have real load data (no warnings)
if (m_long_load_effect > 0.0 && !ctx.frame_warn_load) {
    double long_load_norm = (ctx.avg_front_load / m_static_front_load) - 1.0;
    // ... calculates the effect ...
}
```
**The Bug:** Le Mans Ultimate encrypts the physics for almost all cars (Hypercars, LMP2, GT3, GTE), meaning the raw `mTireLoad` telemetry channel returns `0.0`. Your engine correctly detects this and triggers a kinematic fallback to estimate the load, but it also sets `ctx.frame_warn_load = true`. 

Because of the `&& !ctx.frame_warn_load` condition, the engine **silently disables the longitudinal load effect** whenever the fallback is used. Since the fallback data (`approximate_load` or `calculate_kinematic_load`) is actually perfectly valid for calculating weight transfer, this restriction shouldn't be there.

**The Fix:**
Remove the warning check so the effect can use the fallback load data. Change line 365 in `src/FFBEngine.cpp` to:
```cpp
// Apply if enabled (works with both raw telemetry and kinematic fallback)
if (m_long_load_effect > 0.0) {
```

### 3. The "Pure Passthrough" Override
Check your UI settings. In `src/FFBEngine.cpp` (around line 391), there is an override:
```cpp
// v0.7.63: Final factor application
double dw_factor_applied = m_torque_passthrough ? 1.0 : long_load_factor;
```
If you have the **"Pure Passthrough"** checkbox enabled in the UI, it forces the longitudinal load multiplier to `1.0`, completely bypassing the effect regardless of your slider settings. Ensure this is unchecked.

### Summary of Action Items:
1. **Fix the code:** Remove `&& !ctx.frame_warn_load` from the `if` statement in `FFBEngine.cpp`.
2. **Fix your testing method:** Do not set your base FFB gain to 0. Leave it at a normal level (e.g., 100%) so the longitudinal multiplier has a base force to scale up and down.
3. **Check your settings:** Ensure "Pure Passthrough" is disabled.