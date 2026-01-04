# Preset Comparison: GT3 vs LMPx/HY (DD 15 Nm Simagic Alpha)

**Date**: 2026-01-04  
**Comparison**: GT3 DD 15 Nm (Simagic Alpha) vs LMPx/HY DD 15 Nm (Simagic Alpha)

## Executive Summary

Both presets are optimized for the Simagic Alpha (15 Nm) direct drive wheel but target different racing categories:
- **GT3**: Optimized for GT3 racing with sharper, more responsive feedback
- **LMPx/HY**: Optimized for LMP prototypes and hypercars with smoother, more refined feedback for high-speed stability

The presets share identical base settings but differ in **7 key smoothing and physics parameters** that fundamentally change the FFB character.

---

## Differences Overview

| Parameter | GT3 Value | LMPx/HY Value | Delta | Impact |
|-----------|-----------|---------------|-------|--------|
| `yaw_smoothing` | 0.001 | 0.003 | +200% | More smoothing for high-speed stability |
| `gyro_smoothing` | 0.0 | 0.003 | +∞ | Added gyroscopic damping |
| `sop_smoothing` | 0.99 | 0.97 | -2% | Less aggressive SoP smoothing |
| `sop_scale` | 1.98 | 1.59 | -19.7% | Reduced SoP magnitude |
| `slip_smoothing` | 0.002 | 0.003 | +50% | More slip angle smoothing |
| `chassis_smoothing` | 0.012 | 0.019 | +58.3% | More chassis inertia smoothing |
| `optimal_slip_angle` | 0.1 | 0.12 | +20% | Higher optimal slip threshold |

**Total parameters**: 60  
**Identical parameters**: 53 (88.3%)  
**Different parameters**: 7 (11.7%)

---

## Detailed Parameter Analysis

### 1. Yaw Acceleration Smoothing (`yaw_smoothing`)
- **GT3**: `0.001` - Minimal smoothing for sharp yaw kick response
- **LMPx/HY**: `0.003` - 3x more smoothing for stable high-speed behavior
- **Rationale**: LMP cars operate at higher speeds and need more filtered yaw inputs to avoid twitchy behavior

### 2. Gyroscopic Damping Smoothing (`gyro_smoothing`)
- **GT3**: `0.0` - No gyroscopic damping smoothing
- **LMPx/HY**: `0.003` - Added smoothing for gyro effect
- **Rationale**: Prototypes have more complex aerodynamics and benefit from damped gyroscopic effects

### 3. Seat of Pants Smoothing (`sop_smoothing`)
- **GT3**: `0.99` - Very aggressive smoothing (closer to 1.0 = more smoothing)
- **LMPx/HY**: `0.97` - Less aggressive smoothing
- **Rationale**: LMP cars need more immediate SoP feedback due to higher downforce changes

### 4. Seat of Pants Scale (`sop_scale`)
- **GT3**: `1.98` - High SoP magnitude
- **LMPx/HY**: `1.59` - Reduced by ~20%
- **Rationale**: LMP cars have more predictable rear-end behavior; less exaggerated SoP needed

### 5. Slip Angle Smoothing (`slip_smoothing`)
- **GT3**: `0.002` - Moderate smoothing
- **LMPx/HY**: `0.003` - 50% more smoothing
- **Rationale**: Prototypes operate in higher slip angle ranges; more filtering prevents noise

### 6. Chassis Inertia Smoothing (`chassis_smoothing`)
- **GT3**: `0.012` - Moderate chassis smoothing
- **LMPx/HY**: `0.019` - 58% more smoothing
- **Rationale**: LMP cars have more complex weight transfer; smoothing prevents jittery feedback

### 7. Optimal Slip Angle (`optimal_slip_angle`)
- **GT3**: `0.1` (rad) ≈ 5.73°
- **LMPx/HY**: `0.12` (rad) ≈ 6.88°
- **Rationale**: Prototypes with high downforce operate at higher optimal slip angles

---

## Identical Parameters (53 total)

### General FFB
- `gain`: 1.0
- `max_torque_ref`: 100.0
- `min_force`: 0.0

### Front Axle (Understeer)
- `steering_shaft_gain`: 1.0
- `steering_shaft_smoothing`: 0.0
- `understeer`: 1.0
- `base_force_mode`: 0 (Native Physics)
- `flatspot_suppression`: false
- `notch_q`: 2.0
- `flatspot_strength`: 1.0
- `static_notch_enabled`: false
- `static_notch_freq`: 11.0
- `static_notch_width`: 2.0

### Rear Axle (Oversteer)
- `oversteer_boost`: 2.52101
- `sop`: 1.666
- `rear_align_effect`: 0.666
- `sop_yaw_gain`: 0.333
- `yaw_kick_threshold`: 0.0
- `gyro_gain`: 0.0
- `understeer_affects_sop`: false

### Physics
- `optimal_slip_ratio`: 0.12

### Braking & Lockup
- `lockup_enabled`: true
- `lockup_gain`: 0.37479
- `brake_load_cap`: 2.0
- `lockup_freq_scale`: 1.0
- `lockup_gamma`: 1.0
- `lockup_start_pct`: 1.0
- `lockup_full_pct`: 7.5
- `lockup_prediction_sens`: 10.0
- `lockup_bump_reject`: 0.1
- `lockup_rear_boost`: 1.0
- `abs_pulse_enabled`: false
- `abs_gain`: 2.1
- `abs_freq`: 25.5

### Tactile Textures
- `texture_load_cap`: 1.5
- `slide_enabled`: false
- `slide_gain`: 0.226562
- `slide_freq`: 1.47
- `road_enabled`: true
- `road_gain`: 0.0
- `road_fallback_scale`: 0.05
- `spin_enabled`: true
- `spin_gain`: 0.462185
- `spin_freq_scale`: 1.8
- `scrub_drag_gain`: 0.333
- `bottoming_method`: 1

### Advanced Settings
- `speed_gate_lower`: 1.0
- `speed_gate_upper`: 5.0

---

## FFB Character Comparison

### GT3 Preset Character
**Philosophy**: Sharp, responsive, communicative

- ✅ **Immediate feedback** - Minimal smoothing allows quick response to grip changes
- ✅ **Strong SoP effect** - High scale (1.98) emphasizes rear-end movement
- ✅ **Sharp yaw kicks** - Minimal yaw smoothing for instant rotation feedback
- ✅ **Lower slip threshold** - 0.1 rad optimal slip angle for GT3 tire characteristics
- ⚠️ **Potential for high-speed twitchiness** - Less filtering may feel busy at 200+ km/h

**Best for**: GT3, GT4, touring cars, lower-downforce racing

### LMPx/HY Preset Character
**Philosophy**: Smooth, refined, high-speed stable

- ✅ **Stable at high speeds** - Increased smoothing prevents oscillation
- ✅ **Refined SoP** - Lower scale (1.59) for more subtle rear feedback
- ✅ **Damped gyroscopic effects** - Added gyro smoothing for complex aero
- ✅ **Higher slip threshold** - 0.12 rad for high-downforce prototype tires
- ✅ **Filtered inputs** - More chassis/slip smoothing reduces noise
- ⚠️ **Slightly less immediate** - Smoothing may delay some feedback

**Best for**: LMP1, LMP2, LMP3, DPi, hypercars, high-downforce prototypes

---

## Recommended Use Cases

### Choose GT3 Preset For:
- GT3 / GT4 racing
- Touring cars (TCR, WTCC)
- Lower-downforce categories
- Drivers who prefer sharp, immediate feedback
- Tracks with frequent direction changes
- Sprint races requiring maximum communication

### Choose LMPx/HY Preset For:
- LMP1, LMP2, LMP3 prototypes
- DPi (IMSA)
- Hypercars (WEC)
- High-speed circuits (Le Mans, Spa, Monza)
- Endurance racing requiring stable, predictable FFB
- Drivers who prefer refined, smooth feedback
- High-speed stability over immediate response

---

## Migration Notes

If switching between presets:

### GT3 → LMPx/HY
- Expect: Smoother, less twitchy feedback at high speeds
- Adapt to: Slightly delayed yaw response, more filtered slip angle
- Benefit: Better high-speed stability, less fatigue in long stints

### LMPx/HY → GT3
- Expect: Sharper, more immediate feedback
- Adapt to: More active wheel at high speeds, stronger SoP effect
- Benefit: Better communication of grip changes, more responsive feel

---

## Technical Implementation Notes

### Smoothing Parameter Ranges
- `yaw_smoothing`: 0.0 - 1.0 (higher = more smoothing)
- `gyro_smoothing`: 0.0 - 1.0 (higher = more smoothing)
- `sop_smoothing`: 0.0 - 1.0 (higher = more smoothing, 0.99 is very aggressive)
- `slip_smoothing`: 0.0 - 1.0 (higher = more smoothing)
- `chassis_smoothing`: 0.0 - 1.0 (higher = more smoothing)

### Physics Parameter Ranges
- `optimal_slip_angle`: Radians, typically 0.06 - 0.15 (3.4° - 8.6°)
- `sop_scale`: Multiplier, typically 0.5 - 2.5

### Smoothing Impact on Latency
All smoothing parameters introduce minimal latency (< 16ms at 60Hz update rate) but significantly improve signal quality by filtering high-frequency noise.

---

## Conclusion

The GT3 and LMPx/HY presets represent two distinct FFB philosophies for the same hardware:

- **GT3**: Maximum communication and responsiveness for lower-downforce racing
- **LMPx/HY**: Refined stability and smoothness for high-speed prototype racing

Both presets maintain the same core FFB effects (understeer, SoP, lockup, etc.) but tune the filtering and physics parameters to match the driving characteristics of their respective vehicle categories.

The 11.7% difference in parameters (7 out of 60) demonstrates that subtle tuning of smoothing and physics thresholds can dramatically change FFB character without requiring wholesale changes to effect gains or modes.
