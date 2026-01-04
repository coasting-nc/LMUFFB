# Preset Comparison Report: DD 15 Nm vs Default vs T300

**Date**: 2026-01-03  
**Author**: Antigravity AI  
**Purpose**: Detailed analysis of the new "DD 15 Nm" preset compared to "Default" and "T300" presets

---

## Executive Summary

The **DD 15 Nm** preset represents a configuration optimized for direct drive wheels with 15 Nm torque capacity. Compared to the Default and T300 presets, it features:

- **Reduced understeer effect** (0.75 vs 1.0/0.5)
- **Significantly increased SoP (Seat of Pants) effect** (1.666 vs 1.5/0.425)
- **Much lower lockup gain** (0.37479 vs 2.0/2.0)
- **Disabled slide texture and ABS pulse** (for cleaner feedback)
- **Disabled road texture gain** (enabled but set to 0)
- **Different smoothing characteristics** optimized for DD responsiveness

---

## Detailed Parameter Comparison

### 1. General FFB Settings

| Parameter | Default | T300 | DD 15 Nm | Analysis |
|-----------|---------|------|----------|----------|
| `gain` | 1.0 | 1.0 | 1.0 | **Identical** - All use unity gain |
| `max_torque_ref` | 100.0 | 100.1 | 100.0 | **Effectively identical** - T300 has negligible 0.1% difference |
| `min_force` | 0.0 | 0.01 | 0.0 | **DD matches Default** - T300 has minimal deadzone (1%) |
| `invert_force` | true | true | *(default: true)* | **All inverted** - Standard configuration |

**Key Insight**: DD 15 Nm removes the minimal deadzone present in T300, suggesting DD wheels have better low-force fidelity.

---

### 2. Front Axle (Understeer) Settings

| Parameter | Default | T300 | DD 15 Nm | Analysis |
|-----------|---------|------|----------|----------|
| `understeer` | 1.0 | 0.5 | 0.75 | **DD is middle ground** - Less than Default, more than T300 |
| `steering_shaft_gain` | 1.0 | 1.0 | 1.0 | **Identical** |
| `steering_shaft_smoothing` | 0.0 | 0.0 | 0.0 | **Identical** - No smoothing |
| `base_force_mode` | 0 | 0 | 0 | **Identical** - All use Native physics |
| `flatspot_suppression` | false | false | false | **Identical** - All disabled |
| `notch_q` | 2.0 | 2.0 | 2.0 | **Identical** |
| `flatspot_strength` | 1.0 | 1.0 | 1.0 | **Identical** |
| `static_notch_enabled` | false | false | false | **Identical** |
| `static_notch_freq` | 11.0 | 11.0 | 11.0 | **Identical** |
| `static_notch_width` | 2.0 | 2.0 | 2.0 | **Identical** |

**Key Insight**: The understeer value progression (T300: 0.5 → DD: 0.75 → Default: 1.0) suggests:
- **T300**: Reduced understeer effect (50% of proportional)
- **DD 15 Nm**: Moderate understeer effect (75% of proportional)
- **Default**: Full proportional understeer effect

This makes sense as DD wheels can handle more nuanced force feedback without overwhelming the user.

---

### 3. Rear Axle (Oversteer) Settings

| Parameter | Default | T300 | DD 15 Nm | Analysis |
|-----------|---------|------|----------|----------|
| `oversteer_boost` | 2.0 | 2.40336 | 2.52101 | **DD has highest boost** (+26% vs Default, +5% vs T300) |
| `sop` | 1.5 | 0.425003 | 1.666 | **DD is 292% higher than T300!** |
| `rear_align_effect` | 1.0084 | 0.966383 | 0.666 | **DD is 34% lower** - Reduced rear alignment torque |
| `sop_yaw_gain` | 0.0504202 | 0.386555 | 0.333 | **DD is 14% lower than T300** but 560% higher than Default |
| `yaw_kick_threshold` | 0.2 | 1.68 | 0.0 | **DD has NO threshold** - Always active |
| `yaw_smoothing` | 0.015 | 0.005 | 0.001 | **DD has minimal smoothing** - Most responsive |
| `gyro_gain` | 0.0336134 | 0.0336134 | 0.0 | **DD disables gyro damping** |
| `gyro_smoothing` | 0.0 | 0.0 | 0.0 | **Identical** |
| `sop_smoothing` | 1.0 | 1.0 | 0.99 | **DD slightly less smoothed** (1% reduction) |
| `sop_scale` | 1.0 | 1.0 | 1.98 | **DD nearly doubles SoP scale!** |
| `understeer_affects_sop` | false | false | false | **Identical** |

**Key Insights**:
1. **SoP Effect is MASSIVELY increased** in DD 15 Nm (1.666 vs 0.425 in T300)
2. **SoP Scale is nearly doubled** (1.98 vs 1.0), creating a **3.9x combined multiplier** vs T300
3. **Yaw kick is always active** (threshold = 0) and has **minimal smoothing** (0.001)
4. **Gyro damping is completely disabled** - DD wheels don't need artificial damping
5. **Rear alignment effect is reduced** - Less corrective torque from rear slip

**Philosophy**: DD 15 Nm emphasizes **raw, responsive rear-end feedback** over smoothed/damped sensations.

---

### 4. Physics (Grip & Slip Angle) Settings

| Parameter | Default | T300 | DD 15 Nm | Analysis |
|-----------|---------|------|----------|----------|
| `slip_smoothing` | 0.002 | 0.0 | 0.002 | **DD matches Default** - T300 has no smoothing |
| `chassis_smoothing` | 0.0 | 0.0 | 0.012 | **DD adds chassis smoothing** - Unique to DD |
| `optimal_slip_angle` | 0.1 | 0.1 | 0.12 | **DD has 20% higher threshold** |
| `optimal_slip_ratio` | 0.12 | 0.12 | 0.12 | **Identical** |

**Key Insight**: DD 15 Nm uses **chassis inertia smoothing** (0.012) to filter out high-frequency chassis vibrations, while keeping slip angle smoothing minimal. The higher optimal slip angle (0.12 vs 0.1) means the grip calculation allows more slip before reducing grip, which could provide a wider "sweet spot" for cornering.

---

### 5. Braking & Lockup Settings

| Parameter | Default | T300 | DD 15 Nm | Analysis |
|-----------|---------|------|----------|----------|
| `lockup_enabled` | true | true | true | **Identical** |
| `lockup_gain` | 2.0 | 2.0 | 0.37479 | **DD is 81% LOWER!** - Massive reduction |
| `brake_load_cap` | 3.0 | 10.0 | 2.0 | **DD is 33% lower than Default** |
| `lockup_freq_scale` | 1.0 | 1.02 | 1.0 | **DD matches Default** |
| `lockup_gamma` | 0.5 | 0.1 | 1.0 | **DD is 2x Default, 10x T300** - Steeper curve |
| `lockup_start_pct` | 1.0 | 1.0 | 1.0 | **Identical** |
| `lockup_full_pct` | 5.0 | 5.0 | 7.5 | **DD is 50% higher** - Wider activation range |
| `lockup_prediction_sens` | 20.0 | 10.0 | 10.0 | **DD matches T300** - Lower sensitivity |
| `lockup_bump_reject` | 0.1 | 0.1 | 0.1 | **Identical** |
| `lockup_rear_boost` | 3.0 | 10.0 | 1.0 | **DD is 67% lower than Default** |
| `abs_pulse_enabled` | true | true | false | **DD DISABLES ABS pulse** |
| `abs_gain` | 2.0 | 2.0 | 2.1 | **DD slightly higher** (but disabled) |
| `abs_freq` | 20.0 | 20.0 | 25.5 | **DD is 27.5% higher** (but disabled) |

**Key Insights**:
1. **Lockup gain is dramatically reduced** (0.37479 vs 2.0) - **81% reduction**
2. **Lockup gamma is doubled** (1.0 vs 0.5) - Creates a more aggressive/sudden onset
3. **Lockup activation range is wider** (7.5% vs 5.0%) - More gradual build-up
4. **Rear lockup boost is minimal** (1.0 vs 3.0/10.0) - Less emphasis on rear braking
5. **ABS pulse is completely disabled** - DD users prefer continuous feedback over pulses

**Philosophy**: DD 15 Nm uses **subtle, wide-range lockup** with **no ABS pulses**, relying on the wheel's fidelity to convey braking information through continuous forces rather than artificial vibrations.

---

### 6. Tactile Textures Settings

| Parameter | Default | T300 | DD 15 Nm | Analysis |
|-----------|---------|------|----------|----------|
| `texture_load_cap` | 1.5 | 1.96 | 1.5 | **DD matches Default** |
| `slide_enabled` | true | true | false | **DD DISABLES slide texture** |
| `slide_gain` | 0.39 | 0.235294 | 0.226562 | **DD is 42% lower** (but disabled) |
| `slide_freq` | 1.0 | 1.0 | 1.47 | **DD is 47% higher** (but disabled) |
| `road_enabled` | true | true | true | **Identical** |
| `road_gain` | 0.5 | 2.0 | 0.0 | **DD DISABLES road texture gain!** |
| `road_fallback_scale` | 0.05 | 0.05 | 0.05 | **Identical** |
| `spin_enabled` | false | true | true | **DD matches T300** |
| `spin_gain` | 0.5 | 0.5 | 0.462185 | **DD is 8% lower** |
| `spin_freq_scale` | 1.0 | 1.0 | 1.8 | **DD is 80% higher frequency** |
| `scrub_drag_gain` | 0.0 | 0.0462185 | 0.333 | **DD is 620% higher than T300!** |
| `bottoming_method` | 0 | 0 | 1 | **DD uses different method** |

**Key Insights**:
1. **Slide texture is completely disabled** - DD doesn't need artificial scrubbing vibrations
2. **Road texture is enabled but gain is 0** - Effectively disabled
3. **Spin is enabled with 80% higher frequency** - Faster, more responsive wheel spin feedback
4. **Scrub drag gain is MASSIVELY increased** (0.333 vs 0.046) - **7.2x multiplier**
5. **Bottoming method is different** (1 vs 0) - Alternative suspension bottoming calculation

**Philosophy**: DD 15 Nm **disables artificial textures** (slide, road) and instead emphasizes **physics-based effects** (scrub drag, spin). The high scrub drag gain suggests DD wheels can convey tire scrubbing through force resistance rather than vibrations.

---

### 7. Advanced Settings

| Parameter | Default | T300 | DD 15 Nm | Analysis |
|-----------|---------|------|----------|----------|
| `speed_gate_lower` | 1.0 | 0.0 | 1.0 | **DD matches Default** - T300 has no lower gate |
| `speed_gate_upper` | 5.0 | 0.277778 | 5.0 | **DD matches Default** - T300 has much lower upper gate |

**Key Insight**: T300 uses a very narrow speed gate (0 to 0.277778 m/s = 1 km/h), while DD and Default use a wider gate (1 to 5 m/s = 3.6 to 18 km/h). This suggests DD wheels handle low-speed forces better and don't need aggressive gating.

---

## Summary of Key Differences

### DD 15 Nm vs Default

| Category | Key Differences |
|----------|-----------------|
| **Understeer** | 25% reduction (0.75 vs 1.0) |
| **SoP** | 11% increase (1.666 vs 1.5), but **98% scale increase** (1.98 vs 1.0) |
| **Oversteer** | 26% boost increase (2.52 vs 2.0) |
| **Lockup** | 81% gain reduction (0.37 vs 2.0), wider activation range |
| **Textures** | Slide and road disabled, scrub drag massively increased |
| **Smoothing** | Adds chassis smoothing (0.012), minimal yaw smoothing (0.001) |
| **Gyro** | Completely disabled (0.0 vs 0.034) |

### DD 15 Nm vs T300

| Category | Key Differences |
|----------|-----------------|
| **Understeer** | 50% increase (0.75 vs 0.5) |
| **SoP** | **292% increase** (1.666 vs 0.425), **98% scale increase** (1.98 vs 1.0) |
| **Yaw Kick** | Always active (threshold 0 vs 1.68), 80% less smoothing |
| **Lockup** | 81% gain reduction (0.37 vs 2.0), 90% rear boost reduction |
| **ABS** | Disabled (vs enabled) |
| **Textures** | Slide disabled, road gain 0 (vs 2.0), scrub 620% higher |
| **Spin** | 80% higher frequency (1.8 vs 1.0) |
| **Speed Gate** | Much wider range (1-5 vs 0-0.28 m/s) |

---

## Design Philosophy Analysis

### Default Preset
- **Balanced, proportional feedback** across all effects
- **Moderate smoothing** for general use
- **All textures enabled** for rich sensory feedback
- **Target**: General-purpose, works for most wheels

### T300 Preset
- **Optimized for belt-driven wheel** with moderate torque (~3.9 Nm)
- **Reduced understeer** (0.5) to avoid overwhelming the weaker motor
- **Very low SoP** (0.425) to prevent oscillations in belt-driven system
- **High road texture gain** (2.0) to compensate for lower resolution
- **Narrow speed gate** (0-1 km/h) to eliminate low-speed noise
- **Target**: Thrustmaster T300 and similar belt-driven wheels

### DD 15 Nm Preset
- **Optimized for direct drive wheel** with high torque (15 Nm)
- **High SoP with nearly 2x scale** for immersive rear-end feedback
- **Disables artificial textures** (slide, road vibrations, ABS pulses)
- **Emphasizes physics-based forces** (scrub drag 7.2x higher)
- **Minimal smoothing** for maximum responsiveness
- **No gyro damping** - DD wheels don't need artificial resistance
- **Subtle lockup** (81% lower gain) - DD fidelity conveys braking without exaggeration
- **Wide speed gate** - DD can handle low-speed forces cleanly
- **Target**: High-end direct drive wheels (15 Nm class)

---

## Recommendations

### When to Use DD 15 Nm
- You have a direct drive wheel with **~15 Nm torque capacity**
- You prefer **raw, unfiltered physics feedback** over artificial textures
- You want **strong rear-end/oversteer sensations** (SoP, yaw kick)
- You value **responsiveness** over smoothness
- You prefer **subtle braking feedback** without ABS pulses

### When to Use T300
- You have a **belt-driven wheel** (T300, TX, etc.)
- You prefer **reduced understeer** for less fatiguing driving
- You want **artificial textures** (road rumble, slide vibrations)
- You need **aggressive speed gating** to eliminate low-speed noise
- You prefer **strong lockup feedback** with ABS pulses

### When to Use Default
- You're **unsure which preset to start with**
- You want **balanced, proportional feedback**
- You have a wheel that's **neither belt-driven nor high-end DD**
- You prefer **moderate settings** across all effects

---

## Technical Notes

### Calculated Combined Effects

**SoP Total Multiplier** (sop × sop_scale):
- Default: 1.5 × 1.0 = **1.5**
- T300: 0.425 × 1.0 = **0.425**
- DD 15 Nm: 1.666 × 1.98 = **3.30** ← **7.8x higher than T300!**

**Lockup Effective Strength** (lockup_gain × rear_boost):
- Default: 2.0 × 3.0 = **6.0**
- T300: 2.0 × 10.0 = **20.0**
- DD 15 Nm: 0.37479 × 1.0 = **0.37** ← **98% reduction vs Default!**

### Missing/Implicit Parameters

The DD 15 Nm preset doesn't explicitly set `invert_force`, so it inherits the **default value of `true`** from the Preset struct definition.

---

## Conclusion

The **DD 15 Nm** preset represents a **fundamentally different philosophy** compared to T300:

1. **Emphasizes physics over textures** - Disables artificial vibrations
2. **Maximizes rear-end feedback** - 7.8x SoP multiplier vs T300
3. **Minimizes smoothing** - Trusts DD fidelity for raw feedback
4. **Subtle braking** - 98% lockup reduction, no ABS pulses
5. **Responsive yaw** - Always active, minimal filtering

This preset is clearly designed for **experienced sim racers** with **high-end direct drive wheels** who want **maximum immersion** through **physics-accurate forces** rather than **artificial tactile enhancements**.

The T300 preset, by contrast, is optimized for **belt-driven hardware limitations** and uses **artificial textures** and **aggressive filtering** to provide a **comfortable, accessible experience** on weaker hardware.

The Default preset sits in the middle as a **safe starting point** for users to explore and customize based on their hardware and preferences.


# Preset Comparison: DD 15 Nm (Simagic Alpha) screesnshot vs ini file

**Date:** 2026-01-03  
**Preset Location:** `src\Config.cpp` lines 94-153  
**Screenshot:** User-provided GUI screenshot

## Comparison Results

### ✅ MATCHING Settings

| Setting | Screenshot Value | Preset Value | Location |
|---------|------------------|--------------|----------|
| **Lateral G (SoP)** | 166.6% (~8.3 Nm) | `p.sop = 1.666f` | Line 111 |
| **SoP Self-Aligning Torque** | 66.6% (~10.0 Nm) | `p.rear_align_effect = 0.666f` | Line 112 |
| **Yaw Kick Gain** | 33.3% (~8.3 Nm) | `p.sop_yaw_gain = 0.333f` | Line 113 |
| **Kick Response** | Latency: 1 ms | `p.yaw_smoothing = 0.001f` | Line 115 |
| **Gyro Damping** | 0.0% (~0.0 Nm) | `p.gyro_gain = 0.0f` | Line 116 |
| **SoP Smoothing** | Latency: 1 ms | `p.sop_smoothing = 0.99f` | Line 118 |
| **SoP Scale** | 1.98 | `p.sop_scale = 1.98f` | Line 119 |
| **Slip Angle Smoothing** | Latency: 2 ms | `p.slip_smoothing = 0.002f` | Line 121 |
| **Chassis Inertia** | 0.012 s | `p.chassis_smoothing = 0.012f` | Line 122 |
| **Optimal Slip Ratio** | 0.12 | `p.optimal_slip_ratio = 0.12f` | Line 124 |
| **Brake Load Cap** | 2.00x | `p.brake_load_cap = 2.0f` | Line 127 |
| **Lockup Strength** | 37.5% (~7.5 Nm) | `p.lockup_gain = 0.37479f` | Line 126 |
| **Vibration Pitch** | 1.00x | `p.lockup_gamma = 1.0f` | Line 129 |
| **Start Slip %** | 1.0% | `p.lockup_start_pct = 1.0f` | Line 130 |
| **Full Slip %** | 7.5% | `p.lockup_full_pct = 7.5f` | Line 131 |
| **Sensitivity** | 10 | `p.lockup_prediction_sens = 10.0f` | Line 132 |
| **Rear Boost** | 1.00x | `p.lockup_rear_boost = 1.0f` | Line 134 |
| **Texture Load Cap** | 1.50x | `p.texture_load_cap = 1.5f` | Line 138 |
| **Spin Strength** | 46.2% (~5.8 Nm) | `p.spin_gain = 0.462185f` | Line 146 |
| **Scrub Drag** | 33.3% (~8.3 Nm) | `p.scrub_drag_gain = 0.333f` | Line 148 |
| **Bottoming Logic** | Method B: Susp. Spike | `p.bottoming_method = 1` | Line 149 |

### ⚠️ DISCREPANCIES FOUND

| Setting | Screenshot Value | Preset Value | Location | Issue |
|---------|------------------|--------------|----------|-------|
| **Lateral G Boost (Slide)** | 200.0% | `252.1%` (`2.52101f`) | Line 110 | Code value is ~52% higher than screenshot. |
| **Yaw Kick Threshold** | ~1.81 (Active) | `0.0f` (Disabled) | Line 114 | Code disables this feature (0.0), screenshot shows it active (~1.81). |
| **Optimal Slip Angle** | 0.08 rad | `0.12 rad` (`0.12f`) | Line 123 | Code allows for more slip (0.12) than screenshot (0.08). |

## Analysis

### 1. Lateral G Boost (Oversteer Boost) Mismatch
- **Screenshot**: 200.0%
- **Code**: 252.1% (`2.52101f`)
- **Impact**: The code version provides significantly stronger oversteer cues than what is shown in the screenshot. If the screenshot represents the "tuned" feel, the current code is too aggressive.

### 2. Yaw Kick Threshold Mismatch
- **Screenshot**: ~1.81 (Slider is clearly advanced)
- **Code**: 0.0 (`0.0f`)
- **Impact**: Be default, the code **disables** the activation threshold for the Yaw Kick effect, meaning it will activate immediately. The screenshot shows a tuning where the kick only activates after a certain threshold (likely Yaw Acceleration).

### 3. Optimal Slip Angle Mismatch
- **Screenshot**: 0.08 rad
- **Code**: 0.12 rad (`0.12f`)
- **Impact**: The code uses a wider optimal slip window (0.12 rad) compared to the screenshot (0.08 rad). A higher value here means peak forces occur at a larger slip angle, potentially making the car feel "looser" or requiring more steering input to reach peak FFB force.

## Recommendations

1. **Update `Config.cpp` to match the Screenshot:**
   - Reduce `p.oversteer_boost` from `2.52101f` to `2.0f`.
   - Increase `p.yaw_kick_threshold` from `0.0f` to `1.81f`.
   - Decrease `p.optimal_slip_angle` from `0.12f` to `0.08f`.

2. **Verify Intent:**
   - Confirm if the screenshot settings were temporary experimental values or the intended final "Golden Tune". If they are the Golden Tune, the code update is mandatory.
