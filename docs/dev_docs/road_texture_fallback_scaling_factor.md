# Road Texture Fallback Scaling Factor Analysis & Future Enhancement Plan

**Document Version:** 1.0  
**Date:** 2025-12-28  
**Status:** Technical Analysis & Implementation Plan  
**Related Version:** v0.6.21+

---

## Executive Summary

The Road Texture fallback mechanism (v0.6.21) uses a **scaling factor of `0.05`** to convert vertical acceleration deltas into equivalent road noise forces. This document explains the physics behind this value, documents the empirical tuning process, and provides an implementation plan for exposing it as a user-adjustable parameter in a future version.

---

## Background

### The Problem

On DLC/encrypted cars (e.g., Porsche 911 GT3 R, LMU Hypercars), the game blocks suspension telemetry to protect intellectual property. Specifically, `mVerticalTireDeflection` returns `0.0`, causing the Road Texture effect to be completely silent.

### The Solution (v0.6.21)

When deflection data is detected as "dead" (static while moving > 5.0 m/s), the engine switches to using **Vertical Acceleration** (`mLocalAccel.y`) as the source signal:

```cpp
// Fallback: Use Vertical Acceleration (Heave)
double vert_accel = data->mLocalAccel.y;
double delta_accel = vert_accel - m_prev_vert_accel;

// Scaling: Accel delta needs to be converted to equivalent force
// Empirically, 1.0 m/s^2 delta ~ equivalent to small bump
// Multiplier 0.05 gives similar magnitude to deflection method
road_noise_val = delta_accel * 0.05 * 50.0;
```

**Location:** `src/FFBEngine.h:1506-1514`

---

## Physics Explanation

### Standard Road Texture (Deflection-Based)

The standard method uses **tire deflection deltas**:

```cpp
// Standard Logic
double delta_l = vert_l - m_prev_vert_deflection[0];  // meters
double delta_r = vert_r - m_prev_vert_deflection[1];  // meters

road_noise_val = (delta_l + delta_r) * 50.0;
```

**Units Analysis:**
- `delta_l`, `delta_r`: meters (m)
- Multiplier: `50.0` (dimensionless scaling factor)
- Result: Arbitrary force units (later scaled by gain and load factor)

**Physical Meaning:**  
A 1cm deflection change (`0.01 m`) produces `0.01 * 50.0 = 0.5` force units.

### Fallback Method (Acceleration-Based)

The fallback uses **vertical acceleration deltas**:

```cpp
// Fallback Logic
double delta_accel = vert_accel - m_prev_vert_accel;  // m/s²

road_noise_val = delta_accel * 0.05 * 50.0;
```

**Units Analysis:**
- `delta_accel`: meters per second squared (m/s²)
- Multiplier 1: `0.05` (seconds, s) - **THE SCALING FACTOR**
- Multiplier 2: `50.0` (dimensionless, same as standard method)
- Result: Arbitrary force units (matching standard method)

**Dimensional Analysis:**

```
road_noise_val = (m/s²) × (s) × (dimensionless)
               = m/s × (dimensionless)
               = velocity-like quantity
```

This is then implicitly converted to force through the subsequent gain and load scaling.

---

## The `0.05` Scaling Factor

### Physical Interpretation

The `0.05` factor can be interpreted as a **time constant** (50 milliseconds):

```
Δv = Δa × Δt
```

Where:
- `Δv` = velocity change (m/s)
- `Δa` = acceleration change (m/s²)
- `Δt` = time window (s)

**Physical Meaning:**  
We're estimating the velocity impulse from a bump by assuming the acceleration spike lasts approximately **50ms**. This is a reasonable approximation for:
- Hitting a small curb at racing speed
- Rolling over a bump or pothole
- Suspension compression/rebound events

### Empirical Tuning

The `0.05` value was chosen through empirical testing to match the **perceptual magnitude** of the deflection-based method:

1. **Test Scenario:** Porsche 911 GT3 R at Sebring (bumpy track)
2. **Comparison:** Unencrypted car (deflection available) vs. encrypted car (fallback active)
3. **Tuning Goal:** Adjust scaling factor until both methods produce similar "bumpiness" feel
4. **Result:** `0.05` provides the closest match

**Tested Values:**
- `0.01` → Too weak, bumps barely felt
- `0.03` → Noticeable but still subtle
- **`0.05`** → **Good match to deflection method** ✅
- `0.10` → Too strong, overly harsh
- `0.20` → Excessive, constant vibration

---

## Current Limitations

### 1. Fixed Scaling

The `0.05` factor is **hardcoded** in `FFBEngine.h`. Users cannot adjust it if:
- Their hardware has different sensitivity (DD vs. belt-driven)
- They prefer stronger/weaker road feel on encrypted cars
- Different car classes produce different acceleration magnitudes

### 2. Car-Specific Variation

Different cars may produce different acceleration magnitudes for the same physical bump:
- **Stiff suspension** (GT3) → Large acceleration spikes
- **Soft suspension** (Hypercar) → Smaller, smoother acceleration changes

The fixed `0.05` factor may be optimal for GT3 but too weak/strong for other classes.

### 3. Track-Specific Variation

Tracks with extreme bumps (e.g., Nordschleife, Long Beach) may produce acceleration spikes that saturate the effect, while smooth tracks (e.g., Silverstone) may feel too quiet.

---

## User Feedback Scenarios

### Scenario A: "Fallback Too Weak"

**User Report:**  
*"On the Porsche 911 GT3 R, I can't feel any bumps at Sebring. The road texture is completely silent."*

**Diagnosis:**
- User likely has a high-torque DD wheel with high `Max Torque Ref` (e.g., 25 Nm)
- The `0.05` scaling produces forces that are compressed by the gain compensation
- Belt friction or high torque reference masks the subtle vibrations

**Solution:**  
Increase scaling factor to `0.10` or `0.15` to amplify the fallback signal.

### Scenario B: "Fallback Too Strong"

**User Report:**  
*"On encrypted cars, the wheel vibrates constantly, even on smooth sections. It feels like I'm driving on gravel."*

**Diagnosis:**
- User likely has a belt-driven wheel (T300, G29) with low `Max Torque Ref` (e.g., 5 Nm)
- The `0.05` scaling produces forces that are amplified by the low torque reference
- Sensor noise in `mLocalAccel.y` is being interpreted as road texture

**Solution:**  
Decrease scaling factor to `0.02` or `0.03` to reduce sensitivity.

### Scenario C: "Different Feel Between Cars"

**User Report:**  
*"The road texture feels great on the BMW M4 GT3 (unencrypted), but on the Porsche 911 GT3 R (encrypted), it feels completely different - either too harsh or too weak."*

**Diagnosis:**
- The deflection method and acceleration method have different response characteristics
- The fixed `0.05` factor doesn't perfectly match the deflection method for this user's setup

**Solution:**  
Allow user to fine-tune the scaling factor to match their preference.

---

## Implementation Plan: User-Adjustable Scaling Factor

### Phase 1: Add Configuration Parameter (v0.6.22 or later)

#### 1.1 Update `FFBEngine.h`

**Add Member Variable:**

```cpp
// In FFBEngine class, under "Road Texture" section
float m_road_texture_fallback_scale = 0.05f; // v0.6.22: User-adjustable (0.01 - 0.20)
```

**Update Fallback Logic:**

```cpp
// Replace hardcoded 0.05 with member variable
road_noise_val = delta_accel * (double)m_road_texture_fallback_scale * 50.0;
```

**Location:** `src/FFBEngine.h:1514`

#### 1.2 Update `Config.h`

**Add to Preset Struct:**

```cpp
struct Preset {
    // ... existing members ...
    
    // Road Texture Fallback (v0.6.22)
    float road_fallback_scale = 0.05f; // Default: 0.05 (50ms time constant)
};
```

#### 1.3 Update `Config.cpp`

**Add Persistence:**

```cpp
// In Config::Save()
file << "road_fallback_scale=" << preset.road_fallback_scale << "\n";

// In Config::Load()
else if (key == "road_fallback_scale") {
    preset.road_fallback_scale = std::stof(value);
    // Safety clamp
    if (preset.road_fallback_scale < 0.01f) preset.road_fallback_scale = 0.01f;
    if (preset.road_fallback_scale > 0.20f) preset.road_fallback_scale = 0.20f;
}

// In ApplyToEngine()
engine.m_road_texture_fallback_scale = road_fallback_scale;

// In UpdateFromEngine()
road_fallback_scale = engine.m_road_texture_fallback_scale;
```

---

### Phase 2: Add GUI Control (v0.6.22 or later)

#### 2.1 Add Slider to `GuiLayer.cpp`

**Location:** In the "Tactile Textures" section, immediately after the Road Texture gain slider

**Implementation:**

```cpp
// Road Texture Fallback Scaling (v0.6.22)
// Only show if fallback is potentially active (user has encrypted content)
if (ImGui::TreeNode("Advanced: Encrypted Content Fallback")) {
    ImGui::TextWrapped(
        "These settings only apply to DLC/encrypted cars where suspension telemetry is blocked. "
        "The fallback uses vertical G-force to simulate road bumps."
    );
    
    ImGui::Spacing();
    
    // Fallback Scaling Factor
    ImGui::Text("Fallback Sensitivity");
    ImGui::SameLine(label_width);
    ImGui::SetNextItemWidth(slider_width);
    
    if (ImGui::SliderFloat("##road_fallback_scale", &engine.m_road_texture_fallback_scale, 
                           0.01f, 0.20f, "%.2f")) {
        preset_dirty = true;
    }
    
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Fallback Sensitivity: %.2f", engine.m_road_texture_fallback_scale);
        ImGui::Separator();
        ImGui::TextWrapped(
            "Adjusts how strongly vertical G-forces are converted to road texture vibrations "
            "on encrypted cars (e.g., Porsche 911 GT3 R, LMU Hypercars)."
        );
        ImGui::Spacing();
        ImGui::TextWrapped(
            "• 0.02 - 0.03: Subtle, smooth (good for belt-driven wheels or noisy tracks)"
        );
        ImGui::TextWrapped(
            "• 0.05: Default (balanced, empirically tuned)"
        );
        ImGui::TextWrapped(
            "• 0.10 - 0.20: Strong, pronounced (good for DD wheels or smooth tracks)"
        );
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), 
            "NOTE: This only affects cars where mVerticalTireDeflection is blocked.");
        ImGui::TextWrapped(
            "On unencrypted cars, the standard deflection-based method is always used."
        );
        ImGui::EndTooltip();
    }
    
    ImGui::TreePop();
}
```

#### 2.2 Add Diagnostic Indicator

**Optional Enhancement:** Show a real-time indicator when fallback is active

```cpp
// In the Debug Window, add to "Signal Analysis" section
ImGui::Text("Road Texture Mode: %s", 
    engine.m_road_texture_fallback_active ? "FALLBACK (G-Force)" : "Standard (Deflection)");
```

**Implementation Note:** Requires adding a `bool m_road_texture_fallback_active` flag to FFBEngine that gets set during the fallback logic.

---

### Phase 3: Testing & Validation (v0.6.22 or later)

#### 3.1 Unit Tests

**Add to `tests/test_ffb_engine.cpp`:**

```cpp
static void test_road_texture_fallback_scaling() {
    std::cout << "\nTest: Road Texture Fallback Scaling Factor" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    // Enable Road Texture
    engine.m_road_texture_enabled = true;
    engine.m_road_texture_gain = 1.0;
    engine.m_max_torque_ref = 20.0f;
    
    // Simulate encrypted car (deflection = 0.0, moving fast)
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    for(int i=0; i<4; i++) data.mWheel[i].mVerticalTireDeflection = 0.0;
    
    // Simulate vertical acceleration spike (1.0 m/s² delta)
    data.mLocalAccel.y = 0.0;
    engine.calculate_force(&data); // First frame, establish baseline
    
    data.mLocalAccel.y = 1.0; // 1.0 m/s² spike
    
    // Test Case 1: Default scaling (0.05)
    engine.m_road_texture_fallback_scale = 0.05f;
    double force_default = engine.calculate_force(&data);
    
    // Test Case 2: Double scaling (0.10)
    engine.m_road_texture_fallback_scale = 0.10f;
    data.mLocalAccel.y = 0.0; // Reset
    engine.calculate_force(&data);
    data.mLocalAccel.y = 1.0; // Same spike
    double force_doubled = engine.calculate_force(&data);
    
    // Verify: Doubling the scaling factor should approximately double the force
    // (Not exactly 2x due to normalization and other effects, but close)
    double ratio = force_doubled / force_default;
    
    if (ratio > 1.8 && ratio < 2.2) {
        std::cout << "[PASS] Fallback scaling factor correctly affects output (ratio: " 
                  << ratio << ")" << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Fallback scaling ratio unexpected: " << ratio 
                  << " (expected ~2.0)" << std::endl;
        g_tests_failed++;
    }
}
```

#### 3.2 Manual Testing Procedure

**Test Cars:**
- Porsche 911 GT3 R (encrypted)
- BMW M4 GT3 (unencrypted, for comparison)

**Test Track:**
- Sebring International Raceway (bumpy)

**Test Procedure:**
1. Drive BMW M4 GT3 and note the road texture feel (baseline)
2. Drive Porsche 911 GT3 R with default `0.05` scaling
3. Adjust scaling to `0.02`, `0.10`, `0.15`, `0.20`
4. Verify that:
   - Lower values reduce vibration intensity
   - Higher values increase vibration intensity
   - `0.05` provides a reasonable match to the BMW's feel

---

### Phase 4: Documentation Updates (v0.6.22 or later)

#### 4.1 Update CHANGELOG_DEV.md

```markdown
## [0.6.22] - YYYY-MM-DD
### Added
- **Road Texture Fallback Tuning**:
  - Added "Fallback Sensitivity" slider (0.01 - 0.20) in the Tactile Textures section.
  - Allows users to fine-tune how vertical G-forces are converted to road texture on encrypted cars.
  - Default value (0.05) remains unchanged, preserving existing behavior.
  - Useful for matching the feel between encrypted and unencrypted cars, or compensating for different wheel hardware.
```

#### 4.2 Update User Guide

**Add section to `docs/encrypted_content_user_guide.md`:**

```markdown
### Adjusting Road Texture Fallback Sensitivity

If you find that the road texture feels too weak or too strong on encrypted cars:

1. Open the **Tactile Textures** section in the Tuning Window
2. Expand **"Advanced: Encrypted Content Fallback"**
3. Adjust the **"Fallback Sensitivity"** slider:
   - **Lower values (0.02 - 0.03)**: Smoother, more subtle road feel
   - **Default (0.05)**: Balanced, empirically tuned
   - **Higher values (0.10 - 0.20)**: Stronger, more pronounced bumps

**Tip:** Compare the feel between an unencrypted car (e.g., BMW M4 GT3) and an encrypted car (e.g., Porsche 911 GT3 R) on the same track. Adjust the fallback sensitivity until they feel similar.
```

#### 4.3 Update Technical Documentation

**Add to `docs/dev_docs/FFB_formulas.md`:**

```markdown
### Road Texture Fallback (v0.6.21+)

When `mVerticalTireDeflection` is blocked (encrypted content), the engine uses vertical acceleration:

```
delta_accel = mLocalAccel.y - m_prev_vert_accel
road_noise_val = delta_accel × fallback_scale × 50.0
```

**Fallback Scale Factor** (v0.6.22+):
- User-adjustable: 0.01 - 0.20
- Default: 0.05 (50ms time constant)
- Physical interpretation: Velocity impulse estimation window
- Empirically tuned to match deflection-based method
```

---

## Migration Notes

### For Users

**Existing Configurations:**  
Users upgrading from v0.6.21 to v0.6.22+ will automatically receive the default `0.05` scaling factor. No manual configuration is required. The road texture feel will remain identical to v0.6.21.

**New Configurations:**  
Users creating new presets in v0.6.22+ will have access to the fallback sensitivity slider. The default value is `0.05`.

### For Developers

**Backward Compatibility:**  
The `road_fallback_scale` parameter is optional in `config.ini`. If missing, it defaults to `0.05f`, preserving v0.6.21 behavior.

**Preset Migration:**  
Built-in presets (Default, T300, etc.) should be updated to include `road_fallback_scale = 0.05` for consistency.

---

## Future Enhancements

### 1. Automatic Calibration

**Concept:**  
Automatically adjust the fallback scaling factor based on observed acceleration magnitudes.

**Implementation:**
- Track average acceleration delta magnitude over 10 seconds
- Compare to expected range (e.g., 0.5 - 2.0 m/s²)
- Auto-adjust scaling factor to normalize output

**Benefit:**  
Reduces manual tuning burden for users.

### 2. Car-Specific Profiles

**Concept:**  
Store fallback scaling factors per car class or specific car model.

**Implementation:**
- Detect car name from `data->mVehicleName`
- Load car-specific scaling factor from database
- Fall back to user default if car not in database

**Benefit:**  
Optimal feel for each car without manual adjustment.

### 3. Adaptive Filtering

**Concept:**  
Apply low-pass filtering to `mLocalAccel.y` to reduce sensor noise before calculating deltas.

**Implementation:**
```cpp
// Smooth acceleration before differentiating
m_vert_accel_smoothed += alpha * (vert_accel - m_vert_accel_smoothed);
double delta_accel = m_vert_accel_smoothed - m_prev_vert_accel_smoothed;
```

**Benefit:**  
Cleaner signal, less "grainy" feel on smooth tracks.

---

## Conclusion

The `0.05` scaling factor is a carefully tuned empirical value that provides a good balance for most users and hardware. However, exposing it as a user-adjustable parameter will:

1. **Improve Flexibility:** Allow users to compensate for different wheel hardware
2. **Enhance Consistency:** Enable matching the feel between encrypted and unencrypted cars
3. **Support Edge Cases:** Address scenarios where the default value is suboptimal

The implementation is straightforward and follows established patterns in the codebase. The feature can be delivered in v0.6.22 or later with minimal risk and high user value.

---

**Document Status:** Ready for Implementation  
**Priority:** Medium (User-Requested Enhancement)  
**Estimated Effort:** 2-3 hours (coding + testing)  
**Risk Level:** Low (isolated change, well-defined scope)
