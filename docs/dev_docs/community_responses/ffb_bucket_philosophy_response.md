# Understanding LMUFFB's FFB Signal Processing
## Response to Community Feedback on Torque Values and UI Philosophy

**Date**: 2026-01-25  
**Original Post**: Understanding the Percentages and Nm Values  
**Status**: For Community Discussion  
**Code Verification**: ✅ Complete (verified against `FFBEngine.h` and `GuiLayer.cpp`)

---

## Summary of Questions

The user raised excellent points about the current UI being confusing, particularly around:
1. What do the **Nm values in parentheses** mean (e.g., "SoP Lateral G 147.1% (~7.2 Nm)")?
2. What is the **percentage reference**?
3. Why does **increasing Max Torque Ref** increase the Nm numbers but make the wheel lighter?
4. Why do torque numbers **exceed the wheel base's physical max** without clipping?
5. A proposed **"FFB Bucket" philosophy** for the UI.

---

## Answers to Each Question

### 1. What Do the Nm Values in Parentheses Mean?

The `~7.2 Nm` value is an **estimation of the physical torque contribution** of that specific effect at its current gain setting. 

However, this is **not the actual torque being sent to your wheel** — it's a *normalized reference value* based on a mathematical formula that helps compare effects relative to each other.

**The display formula** (from `GuiLayer.cpp` lines 831-840):

```cpp
auto FormatDecoupled = [&](float val, float base_nm) {
    float scale = (engine.m_max_torque_ref / 20.0f); 
    if (scale < 0.1f) scale = 0.1f;
    float estimated_nm = val * base_nm * scale;
    // Display: "147.1% (~7.2 Nm)"
};
```

**What this means:**
- `val` = Your slider value (e.g., 1.47 for 147%)
- `base_nm` = A physics constant for that effect (defined in `FFBEngine.h` lines 447-456):
  - `BASE_NM_SOP_LATERAL = 1.0 Nm`
  - `BASE_NM_REAR_ALIGN = 3.0 Nm`
  - `BASE_NM_YAW_KICK = 5.0 Nm`
  - `BASE_NM_GYRO_DAMPING = 1.0 Nm`
  - `BASE_NM_SLIDE_TEXTURE = 1.5 Nm`
  - `BASE_NM_ROAD_TEXTURE = 2.5 Nm`
  - `BASE_NM_LOCKUP_VIBRATION = 4.0 Nm`
  - `BASE_NM_SPIN_VIBRATION = 2.5 Nm`
  - `BASE_NM_SCRUB_DRAG = 5.0 Nm`
- `scale = Max Torque Ref / 20.0` = The decoupling scale

**Example calculation:**
- SoP Lateral G at 147% (1.47), with Base Nm = 1.0, and Max Torque Ref = 98 Nm:
  - `scale = 98 / 20 = 4.9`
  - `estimated_nm = 1.47 × 1.0 × 4.9 = 7.2 Nm` 

**Important**: This is purely for **display purposes** to help you gauge relative effect strength. This value represents the effect's contribution *before* normalization. The actual output is then **divided by Max Torque Ref** (see below). 

---

### 2. What Is the Percentage Reference?

The percentage represents the **gain multiplier** for each effect:

| Percentage | Meaning |
|------------|---------|
| 0% (0.0) | Effect disabled |
| 100% (1.0) | Effect at "default" strength (1:1 with physics) |
| 200% (2.0) | Effect doubled |

**The reference is always 100% = 1.0 multiplier.**

When you set "SoP Lateral G" to 147%, you're telling the system to multiply the raw Lateral G physics by 1.47× before mixing it into the final signal.

---

### 3. Why Does Increasing Max Torque Ref Increase the Nm Numbers But Make the Wheel Lighter?

This is the most confusing part of the current UI, and your observation is **completely correct**. Here's the verified code flow:

**Max Torque Ref is used twice with opposite effects:**

**A. Effect Calculation (Makes effects stronger)** — From `FFBEngine.h` line 929:
```cpp
// Decoupling Scale
ctx.decoupling_scale = max_torque_safe / 20.0;
```
This `decoupling_scale` is then multiplied into each effect, e.g., line 1230:
```cpp
ctx.yaw_force = -1.0 * m_yaw_accel_smoothed * m_sop_yaw_gain * (double)BASE_NM_YAW_KICK * ctx.decoupling_scale;
```

**B. Final Normalization (Makes output weaker)** — From `FFBEngine.h` line 999:
```cpp
double norm_force = total_sum / max_torque_safe;
```

**The net effect is that these cancel out for effects, but the UI display only shows step A.**

| Max Torque Ref | UI Display (Step A) | Final Output (Step B) |
|----------------|---------------------|----------------------|
| 40 Nm | ~3.0 Nm | ÷40 = lighter |
| 80 Nm | ~6.0 Nm | ÷80 = even lighter |

**The confusion arises because:**
- The UI scales the display by `MaxTorqueRef / 20` (making numbers bigger)
- But the actual output divides by `MaxTorqueRef` (making forces smaller)
- These two operations are meant to "decouple" effect tuning from overall strength, but the display formula doesn't reflect the normalization step.

---

### 4. Why Can Torque Numbers Exceed My Wheel's Max Without Clipping?

**Verified from code** — The numbers shown are **pre-normalization values**.

**Complete signal flow (from `FFBEngine.h` `calculate_force()`):**

1. **Individual effects are calculated** with `decoupling_scale` (lines 1177-1468)
2. **Effects are summed** into `total_sum` (line 996)
3. **Normalization**: `norm_force = total_sum / max_torque_ref` (line 999)
4. **Master Gain applied**: `norm_force *= m_gain` (line 1000)
5. **Final clamp**: `return max(-1.0, min(1.0, norm_force))` (line 1091)

**Why you don't always clip:**
1. **Effects are not all active simultaneously at maximum**
2. **Many effects are subtractive** (Understeer lightens the wheel)
3. **The normalization step provides headroom** — All effects are divided by Max Torque Ref
4. **Different time domains** — Yaw Kick is milliseconds, Lateral G is sustained

---

### 5. The "FFB Bucket" Philosophy — Is It Valid?

**Your analogy is absolutely valid and represents a more intuitive mental model!**

Your proposed approach:
> *"The FFB bucket size is the max physical torque of my wheel base (8 Nm for RS50). Each effect should show what percentage of that bucket it can fill."*

This is **closer to how irFFB and similar tools work** and is definitely more intuitive for end users.

**Current vs. Proposed mental model:**

| Current System | Bucket System |
|----------------|---------------|
| Max Torque Ref = Expected peak physics torque from the car | Max Torque = Your wheel's physical limit |
| Sliders = Gain multipliers on physics | Sliders = % of headroom allocated |
| Display shows pre-normalization Nm | Display would show post-normalization Nm |

---

## Analysis of Suggested UI Improvements

| Suggestion | Feasibility | Notes |
|------------|-------------|-------|
| **Input for wheel base max torque** | ✅ High | Easy to add; could replace or complement Max Torque Ref |
| **Master Gain as 0-100% (no attenuation)** | ⚠️ Medium | Semantically, 100% would mean "full pass-through". Current max is 200%. |
| **Min Force as 0-30%** | ✅ High | Already effectively works this way; just display formatting change |
| **Sliders show % of wheel max + Nm calculated** | ✅ High | Formula: `(effect_contribution / wheel_max_nm) × 100%` |
| **Sum of all effects with clipping indicator** | ✅ High | Very useful! Could show: `Total: 12.4 Nm / 8.0 Nm = 155% (High Clipping Risk)` |
| **Color coding (green/orange/red)** | ✅ High | Easy implementation; see existing clipping graph for precedent |
| **Effect priority system** | ⚠️ Medium-High | This is a **Dynamic Range Compressor** or **Side-chain/Ducking** system. Complex but valuable. |

---

## About Effect Priority (Advanced Topic)

Your example scenario:
> *"LMP2 in a high-speed corner touching curbs while the rear steps out. Instead of losing the understeer feel due to clipping, reduce the curb effect to free up space."*

This is actually a feature mentioned in our documentation under **"Signal Masking Mitigation"**:

From `ffb_effects.md` line 54:
> **Priority System**: Future versions should implement "Side-chaining" or "Ducking". For example, if a severe Lockup event occurs, reduce Road Texture gain to ensure the Lockup signal is clear.

**How it would work:**
1. Define priority levels for each effect (e.g., Grip Information > Slide Onset > Textures)
2. When the sum of effects approaches the wheel's max, compress lower-priority effects first
3. This maintains the "information content" of the most important feedback

This is absolutely realistic and is something we've been considering. Your feedback reinforces that this should be prioritized!

---

## Summary of Current System vs. Proposed "Bucket" System

| Aspect | Current System | Proposed "Bucket" System |
|--------|----------------|--------------------------|
| **Reference point** | Max Torque Ref (game physics headroom) | Wheel Base Max Torque (physical limit) |
| **Percentage meaning** | Gain multiplier (100% = 1.0×) | % of physical headroom used |
| **Displayed Nm** | Pre-normalization physics contribution | Post-normalization expected output |
| **Clipping indicator** | Graphs panel only | Real-time sum with color coding |
| **Intuitive?** | Requires understanding FFB physics | Maps directly to hardware |

---

## Conclusion

**You're not wrong.** The current UI prioritizes *physics accuracy* (how much each effect contributes to the simulated steering feel) over *hardware awareness* (how much of your wheel's torque budget you're using).

For users who just want to "make it feel good without clipping," the bucket philosophy is more intuitive. For users tuning specific physics effects, the current system provides more granularity.

**Possible solution**: A **"Basic Mode"** toggle that:
1. Asks for wheel max torque
2. Displays all effects as % of that budget
3. Shows a real-time "total load" meter with clipping warning
4. Hides advanced physics parameters

This aligns with the planned "Basic Mode" mentioned in the README.

---

## Action Items for Development Team

1. [ ] Add "Wheel Base Max Torque" input (optional, for display calculation)
2. [ ] Add real-time "Total Estimated Load" readout with clipping indicator
3. [ ] Consider Basic Mode implementation prioritizing the "bucket" mental model
4. [ ] Evaluate priority/ducking system for effect compression
5. [ ] Improve tooltips to clarify the difference between "physics contribution" and "output torque"
6. [ ] Update UI display formula to show post-normalization values

---

## Documentation Discrepancies Found During Investigation

During the code verification for this response, the following documentation inconsistencies were identified:

### 1. `docs/ffb_effects.md` — Outdated Effect Descriptions

**Location**: Lines 14-19
**Issue**: The Oversteer/Rear Grip section references "v0.2.2+" but describes a simpler implementation than what currently exists.
**Current Code**: 
- `FFBEngine.h` now has separate `calculate_sop_lateral()`, `calculate_gyro_damping()`, and distinct handling for Rear Align Torque (lines 1177-1251)
- The Lateral G Boost (Slide) is calculated separately from the base SoP effect
- Yaw Kick now has a configurable activation threshold (`m_yaw_kick_threshold`)

**Recommended Update**: Rewrite section 2 to accurately describe the current multi-effect oversteer system.

---

### 2. `docs/FFB Formulas.md` — Missing decoupling_scale in Context Struct

**Location**: Line 34
**Issue**: Documentation says `K_decouple = m_max_torque_ref / 20.0` but doesn't mention the minimum clamp.
**Current Code** (`FFBEngine.h` lines 929-930):
```cpp
ctx.decoupling_scale = max_torque_safe / 20.0;
if (ctx.decoupling_scale < 0.1) ctx.decoupling_scale = 0.1;
```
**Recommended Update**: Add the minimum clamp of 0.1 to the documentation.

---

### 3. `docs/FFB Formulas.md` — Missing BASE_NM Constants

**Location**: Lines 226-239 (Legend section)
**Issue**: The table lists some but not all BASE_NM constants.
**Current Code** (`FFBEngine.h` lines 447-456) includes:
- `BASE_NM_SOP_LATERAL = 1.0` (MISSING from docs)
- `BASE_NM_REAR_ALIGN = 3.0` (MISSING from docs)
- `BASE_NM_YAW_KICK = 5.0` (MISSING from docs)
- `BASE_NM_GYRO_DAMPING = 1.0` (MISSING from docs)
- `BASE_NM_BOTTOMING = 1.0` (MISSING from docs)

**Recommended Update**: Add complete list of all BASE_NM constants to the Legend.

---

### 4. `docs/ffb_effects.md` — Priority System Listed as "Future"

**Location**: Lines 54-55
**Issue**: States "Future versions should implement" — should clarify this is still not implemented.
**Recommended Update**: Either implement the feature or update to clarify timeline/status.

---

### 5. `docs/FFB Formulas.md` — Gyro Damping Formula Incomplete

**Location**: Lines 187-192
**Issue**: The formula shows `* 1.0Nm * K_decouple` but the code (`FFBEngine.h` line 1250) doesn't explicitly use `BASE_NM_GYRO_DAMPING`:
```cpp
ctx.gyro_force = -1.0 * m_steering_velocity_smoothed * m_gyro_gain * (ctx.car_speed / GYRO_SPEED_SCALE) * ctx.decoupling_scale;
```
The BASE_NM_GYRO_DAMPING constant exists (1.0 Nm) but isn't used in the actual calculation — the `1.0` is implicit.
**Recommended Update**: Either add `BASE_NM_GYRO_DAMPING` to the code for consistency, or update the formula documentation to show the actual implementation.

---

### 6. `README.md` — Master Gain Tooltip Discrepancy

**Location**: `GuiLayer.cpp` line 936 vs README troubleshooting
**Issue**: The tooltip says "100% = No attenuation" but the code allows up to 200% (0.0 to 2.0 range). The README troubleshooting advice says "Increase Master Gain" but doesn't clarify the 0-200% range.
**Recommended Update**: Clarify that Master Gain allows boosting above 100% in both locations.

---

*Thank you for this thoughtful feedback! Understanding how users conceptualize FFB helps us build a better tool for everyone.*
