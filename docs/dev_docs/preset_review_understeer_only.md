# "Test: Understeer Only" Preset Review

**Date:** 2026-01-02  
**Version:** 0.6.31  
**Purpose:** Verify that all settings in the "Test: Understeer Only" preset are properly configured

---

## Current Preset Configuration

```cpp
presets.push_back(Preset("Test: Understeer Only", true)
    .SetUndersteer(0.61f)
    .SetSoP(0.0f)
    .SetSoPScale(1.0f)
    .SetSmoothing(0.85f)
    .SetSlipSmoothing(0.015f)
    .SetSlide(false, 0.0f)
    .SetRearAlign(0.0f)
);
```

---

## Analysis: Missing Settings

The preset is **incomplete**. It only explicitly sets 7 parameters, while the Preset struct has **~50 configurable parameters**. All unset parameters will inherit from the Preset struct's default values (T300 defaults).

### ✅ Explicitly Set (Correct for Purpose)
| Setting | Value | Purpose |
|---------|-------|---------|
| `understeer` | 0.61 | **PRIMARY EFFECT** - Enables understeer feel |
| `sop` | 0.0 | Disables SoP (correct - isolates understeer) |
| `sop_scale` | 1.0 | Neutral (doesn't matter since SoP is 0) |
| `sop_smoothing` | 0.85 | Legacy smoothing (doesn't matter since SoP is 0) |
| `slip_smoothing` | 0.015 | Slip angle smoothing for grip calculation |
| `slide_enabled` | false | Disables slide texture (correct - isolates understeer) |
| `rear_align_effect` | 0.0 | Disables rear align torque (correct - isolates understeer) |

### ⚠️ **CRITICAL MISSING SETTINGS**

These settings directly affect the understeer effect but are **not explicitly set**, so they inherit T300 defaults:

| Setting | Inherited Value | Should Be | Impact |
|---------|----------------|-----------|--------|
| **`optimal_slip_angle`** | **0.10** (T300 default) | **0.10** ✅ | **CORRECT** - This is the threshold for grip loss |
| **`optimal_slip_ratio`** | **0.12** (T300 default) | **0.12** ✅ | **CORRECT** - Longitudinal slip threshold |
| `base_force_mode` | 0 (Native) | **0** ✅ | **CORRECT** - Needs native physics for understeer to work |
| `steering_shaft_gain` | 1.0 | **1.0** ✅ | **CORRECT** - Standard gain |

### ❌ **PROBLEMATIC INHERITED SETTINGS**

These settings are **enabled by default** but should probably be **disabled** for a clean "Understeer Only" test:

| Setting | Inherited Value | Should Be | Reason |
|---------|----------------|-----------|--------|
| **`lockup_enabled`** | **true** ❌ | **false** | Lockup vibration contaminates understeer feel during braking |
| **`abs_pulse_enabled`** | **true** ❌ | **false** | ABS pulse contaminates understeer feel |
| **`spin_enabled`** | **false** ✅ | false | Already correct |
| **`road_enabled`** | **true** ❌ | **false** | Road texture adds noise to the test |
| **`oversteer_boost`** | **2.0** ⚠️ | **0.0** | Rear grip boost affects balance (minor) |
| **`sop_yaw_gain`** | **0.0504** ⚠️ | **0.0** | Yaw kick adds counter-steering cues |
| **`gyro_gain`** | **0.0336** ⚠️ | **0.0** | Gyro damping affects steering feel |
| **`scrub_drag_gain`** | **0.0** ✅ | 0.0 | Already correct |

### 🆕 **NEW SETTINGS SINCE PRESET CREATION**

These settings were added in recent versions and are **not configured**:

| Setting | Version Added | Inherited Value | Recommended | Notes |
|---------|---------------|----------------|-------------|-------|
| `speed_gate_lower` | v0.6.23 | 1.0 m/s | **-10.0** | Disable speed gate for testing |
| `speed_gate_upper` | v0.6.23 | 5.0 m/s | **-5.0** | Disable speed gate for testing |
| `texture_load_cap` | v0.6.25 | 1.5 | 1.5 ✅ | Doesn't matter (textures disabled) |
| `road_fallback_scale` | v0.6.25 | 0.05 | 0.05 ✅ | Doesn't matter (road disabled) |
| `understeer_affects_sop` | v0.6.25 | false | false ✅ | Correct (SoP is 0 anyway) |
| `yaw_kick_threshold` | v0.6.10 | 0.2 | 0.2 ✅ | Doesn't matter (yaw gain is low) |
| `static_notch_width` | v0.6.10 | 2.0 | 2.0 ✅ | Doesn't matter (notch disabled) |
| `lockup_gamma` | v0.6.0 | 0.5 | 0.5 ✅ | Doesn't matter (lockup should be disabled) |
| `lockup_prediction_sens` | v0.6.0 | 20.0 | 20.0 ✅ | Doesn't matter (lockup should be disabled) |
| `lockup_bump_reject` | v0.6.0 | 0.1 | 0.1 ✅ | Doesn't matter (lockup should be disabled) |

---

## Recommended Updated Preset

```cpp
// 5. Test: Understeer Only
presets.push_back(Preset("Test: Understeer Only", true)
    // PRIMARY EFFECT
    .SetUndersteer(0.61f)
    
    // DISABLE ALL OTHER EFFECTS
    .SetSoP(0.0f)
    .SetSoPScale(1.0f)
    .SetOversteer(0.0f)          // NEW: Disable oversteer boost
    .SetRearAlign(0.0f)
    .SetSoPYaw(0.0f)             // NEW: Disable yaw kick
    .SetGyro(0.0f)               // NEW: Disable gyro damping
    
    // DISABLE ALL TEXTURES
    .SetSlide(false, 0.0f)
    .SetRoad(false, 0.0f)        // NEW: Explicitly disable road texture
    .SetSpin(false, 0.0f)        // NEW: Explicitly disable spin
    .SetLockup(false, 0.0f)      // NEW: Disable lockup vibration
    .SetAdvancedBraking(0.5f, 20.0f, 0.1f, false, 0.0f)  // NEW: Disable ABS pulse
    .SetScrub(0.0f)
    
    // SMOOTHING (Keep existing values)
    .SetSmoothing(0.85f)         // SoP smoothing (doesn't matter since SoP=0)
    .SetSlipSmoothing(0.015f)    // Slip angle smoothing (important for grip calc)
    
    // PHYSICS PARAMETERS (Explicit for clarity)
    .SetOptimalSlip(0.10f, 0.12f)  // NEW: Explicit optimal slip thresholds
    .SetBaseMode(0)                 // NEW: Native physics mode (required)
    .SetSpeedGate(-10.0f, -5.0f)   // NEW: Disable speed gate (negative = disabled)
);
```

---

## Summary

### Issues Found:
1. ❌ **Lockup and ABS effects are enabled** - These add vibrations during braking that contaminate the understeer test
2. ❌ **Road texture is enabled** - Adds noise to the steering feel
3. ⚠️ **Oversteer boost, yaw kick, and gyro damping are active** - Minor contamination
4. ⚠️ **Speed gate is active** - May affect low-speed testing

### Severity:
- **HIGH**: Lockup/ABS/Road texture contamination
- **MEDIUM**: Oversteer/Yaw/Gyro effects
- **LOW**: Speed gate (only affects <5 m/s)

### Recommendation:
**Update the preset** to explicitly disable all non-understeer effects for a clean isolation test.

---

## Implementation

The updated preset adds 9 new setter calls to ensure complete isolation:
1. `.SetOversteer(0.0f)` - Disable rear grip boost
2. `.SetSoPYaw(0.0f)` - Disable yaw kick
3. `.SetGyro(0.0f)` - Disable gyro damping
4. `.SetRoad(false, 0.0f)` - Disable road texture
5. `.SetSpin(false, 0.0f)` - Disable spin texture
6. `.SetLockup(false, 0.0f)` - Disable lockup vibration
7. `.SetAdvancedBraking(...)` - Disable ABS pulse
8. `.SetOptimalSlip(0.10f, 0.12f)` - Explicit slip thresholds
9. `.SetSpeedGate(-10.0f, -5.0f)` - Disable speed gate

This ensures the preset **only** tests understeer effect without any interference from other systems.

---

## ✅ Implementation Status

**Status:** **COMPLETED** (v0.6.31 - 2026-01-02)

### Changes Made

All recommended updates have been implemented in `src/Config.cpp`:

```cpp
// 5. Test: Understeer Only (Updated v0.6.31 for proper effect isolation)
presets.push_back(Preset("Test: Understeer Only", true)
    // PRIMARY EFFECT
    .SetUndersteer(0.61f)
    
    // DISABLE ALL OTHER EFFECTS
    .SetSoP(0.0f)
    .SetSoPScale(1.0f)
    .SetOversteer(0.0f)          // Disable oversteer boost
    .SetRearAlign(0.0f)
    .SetSoPYaw(0.0f)             // Disable yaw kick
    .SetGyro(0.0f)               // Disable gyro damping
    
    // DISABLE ALL TEXTURES
    .SetSlide(false, 0.0f)
    .SetRoad(false, 0.0f)        // Disable road texture
    .SetSpin(false, 0.0f)        // Disable spin
    .SetLockup(false, 0.0f)      // Disable lockup vibration
    .SetAdvancedBraking(0.5f, 20.0f, 0.1f, false, 0.0f)  // Disable ABS pulse
    .SetScrub(0.0f)
    
    // SMOOTHING
    .SetSmoothing(0.85f)         // SoP smoothing (doesn't affect test since SoP=0)
    .SetSlipSmoothing(0.015f)    // Slip angle smoothing (important for grip calculation)
    
    // PHYSICS PARAMETERS (Explicit for clarity and future-proofing)
    .SetOptimalSlip(0.10f, 0.12f)  // Explicit optimal slip thresholds
    .SetBaseMode(0)                 // Native physics mode (required for understeer)
    .SetSpeedGate(0.0f, 0.0f)      // Disable speed gate (0 = no gating)
);
```

### Verification

Added comprehensive regression test `test_preset_understeer_only_isolation()` in `tests/test_ffb_engine.cpp`:

- **17 assertions** covering all critical parameters
- Verifies primary effect is enabled
- Verifies all other effects are disabled (6 checks)
- Verifies all textures are disabled (5 checks)
- Verifies critical physics parameters are correct (5 checks)

### Documentation

- **Test Documentation**: `docs/dev_docs/test_preset_understeer_only_isolation.md`
- **Changelog Entry**: Added to v0.6.31 section in `CHANGELOG_DEV.md`
- **This Review**: Updated with implementation status

### Impact

The preset now provides a **clean, isolated test environment** for the understeer effect:
- ✅ No lockup vibrations during braking
- ✅ No ABS pulses interfering with testing
- ✅ No road texture noise
- ✅ No oversteer/yaw/gyro effects contaminating the feel
- ✅ Speed gate disabled for consistent low-speed testing
- ✅ All physics parameters explicitly set for future-proofing
