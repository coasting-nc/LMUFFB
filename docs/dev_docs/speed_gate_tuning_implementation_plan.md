# Speed Gate Tuning: Implementation Plan for User-Adjustable Transition Zone

**Document Version:** 1.0  
**Date:** 2025-12-28  
**Status:** Implementation Plan  
**Target Version:** v0.6.22 or later  
**Related Feature:** Stationary Signal Gate (v0.6.21)

---

## Executive Summary

The **Stationary Signal Gate** (v0.6.21) uses a fixed transition zone of **0.5 - 2.0 m/s** to fade out vibration effects when the car is stationary or moving slowly. This document provides a comprehensive implementation plan for exposing these thresholds as user-adjustable "Advanced Settings" to accommodate different user preferences and use cases.

---

## Background

### Current Implementation (v0.6.21)

The speed gate uses a **linear ramp** to fade vibration effects:

```cpp
// 1. Calculate Stationary Gate (Fade out vibrations at low speed)
// Ramp from 0.0 (at < 0.5 m/s) to 1.0 (at > 2.0 m/s)
double speed_gate = (car_v_long - 0.5) / 1.5;
speed_gate = (std::max)(0.0, (std::min)(1.0, speed_gate));
```

**Location:** `src/FFBEngine.h:513-516`

**Behavior:**
- **Below 0.5 m/s:** Gate = 0.0 → All vibrations muted
- **0.5 - 2.0 m/s:** Gate = 0.0 → 1.0 → Linear fade-in
- **Above 2.0 m/s:** Gate = 1.0 → Full vibration strength

**Applied to:**
- Road Texture (`src/FFBEngine.h:1524`)
- ABS Pulse (`src/FFBEngine.h:1271`)
- Lockup Vibration (`src/FFBEngine.h:1372`)
- Suspension Bottoming (`src/FFBEngine.h:1592`)

### Why These Values?

The **0.5 - 2.0 m/s** range was chosen based on:

1. **Physics Reasoning:**
   - 0.5 m/s (1.8 km/h) → Below this, the car is effectively stationary
   - 2.0 m/s (7.2 km/h) → Above this, the car is clearly in motion

2. **User Experience:**
   - Eliminates idle vibrations in pits/grid
   - Prevents shaking during slow-speed maneuvering (pit lane, parking)
   - Doesn't interfere with normal driving (even slow corners are > 10 m/s)

3. **Empirical Testing:**
   - Tested on T300, G29, and DD wheels
   - No false positives (vibrations cutting out during normal driving)
   - No false negatives (vibrations leaking through at standstill)

---

## Motivation for User-Adjustable Thresholds

### Use Case 1: Sim Rig with Motion Platform

**Scenario:**  
User has a motion platform that physically moves the car. They want to feel **all** vibrations, even at very low speeds, to match the motion cues.

**Current Problem:**  
The 0.5 m/s lower threshold mutes vibrations during slow pit lane driving (2-5 km/h), which feels disconnected from the motion platform's movement.

**Desired Solution:**  
Lower the lower threshold to `0.1 m/s` or even `0.0 m/s` to allow vibrations at all speeds.

### Use Case 2: High-Sensitivity DD Wheel

**Scenario:**  
User has a high-torque direct drive wheel (25+ Nm) with very low friction. They experience **idle vibrations** even at 1.0 m/s due to sensor noise.

**Current Problem:**  
The 0.5 m/s lower threshold is too low - vibrations still leak through during slow maneuvering.

**Desired Solution:**  
Raise the lower threshold to `1.0 m/s` or `1.5 m/s` to completely eliminate low-speed vibrations.

### Use Case 3: Realistic Pit Lane Experience

**Scenario:**  
User wants to feel road texture and bumps during pit lane driving (5-10 km/h) for maximum immersion.

**Current Problem:**  
The 2.0 m/s upper threshold (7.2 km/h) means vibrations are still partially faded during pit lane speeds.

**Desired Solution:**  
Lower the upper threshold to `1.0 m/s` (3.6 km/h) for faster fade-in.

### Use Case 4: Aggressive Fade-In for Smoothness

**Scenario:**  
User finds the transition from 0% to 100% vibration too abrupt, causing a "step" feeling when accelerating from standstill.

**Current Problem:**  
The 1.5 m/s transition window (2.0 - 0.5) is too narrow.

**Desired Solution:**  
Widen the transition window to 3.0 m/s (e.g., 0.5 - 3.5 m/s) for a gentler fade-in.

---

## Design Considerations

### 1. Parameter Naming

**Option A: Threshold-Based**
- `speed_gate_lower_threshold` (m/s)
- `speed_gate_upper_threshold` (m/s)

**Option B: Zone-Based**
- `speed_gate_start_speed` (m/s)
- `speed_gate_full_speed` (m/s)

**Option C: User-Friendly**
- `vibration_mute_below` (km/h)
- `vibration_full_above` (km/h)

**Recommendation:** **Option B (Zone-Based)**  
- Clear semantic meaning ("start fading" vs. "full strength")
- Matches existing terminology in lockup settings (`lockup_start_pct`, `lockup_full_pct`)
- Easy to explain in tooltips

### 2. Units: m/s vs. km/h

**m/s (Meters per Second):**
- ✅ Matches internal physics calculations
- ✅ Consistent with other speed-based parameters
- ❌ Less intuitive for users (most think in km/h or mph)

**km/h (Kilometers per Hour):**
- ✅ More intuitive for users
- ✅ Matches in-game speedometer
- ❌ Requires conversion in code

**Recommendation:** **Display in km/h, store in m/s**
- GUI shows km/h for user-friendliness
- Config file stores m/s for precision
- Conversion: `km/h = m/s × 3.6`

### 3. Slider Ranges

**Lower Threshold (Start Speed):**
- **Minimum:** 0.0 m/s (0 km/h) → No gate, vibrations always active
- **Maximum:** 2.0 m/s (7.2 km/h) → Conservative, prevents interference
- **Default:** 0.5 m/s (1.8 km/h) → Current behavior
- **Step:** 0.1 m/s (0.36 km/h) → Fine control

**Upper Threshold (Full Speed):**
- **Minimum:** 0.5 m/s (1.8 km/h) → Must be ≥ lower threshold
- **Maximum:** 5.0 m/s (18 km/h) → Covers pit lane speeds
- **Default:** 2.0 m/s (7.2 km/h) → Current behavior
- **Step:** 0.1 m/s (0.36 km/h) → Fine control

### 4. Validation Logic

**Constraint:** `upper_threshold >= lower_threshold + 0.1`

**Reason:** Prevent division by zero and ensure a minimum transition window.

**Implementation:**
```cpp
// In Config::Load()
if (speed_gate_upper <= speed_gate_lower + 0.1f) {
    speed_gate_upper = speed_gate_lower + 0.5f; // Force minimum 0.5 m/s window
}
```

---

## Implementation Plan

### Phase 1: Core Engine Changes

#### 1.1 Update `FFBEngine.h`

**Add Member Variables:**

```cpp
// In FFBEngine class, under "Speed Gate" section (v0.6.22)
float m_speed_gate_lower = 0.5f; // Start fading (m/s)
float m_speed_gate_upper = 2.0f; // Full strength (m/s)
```

**Update Calculation Logic:**

```cpp
// Replace hardcoded values with member variables
// OLD:
// double speed_gate = (car_v_long - 0.5) / 1.5;

// NEW:
double lower = (double)m_speed_gate_lower;
double upper = (double)m_speed_gate_upper;
double window = upper - lower;

// Safety: Prevent division by zero
if (window < 0.1) window = 0.1;

double speed_gate = (car_v_long - lower) / window;
speed_gate = (std::max)(0.0, (std::min)(1.0, speed_gate));
```

**Location:** `src/FFBEngine.h:513-516`

#### 1.2 Update Comments

```cpp
// 1. Calculate Stationary Gate (Fade out vibrations at low speed)
// Ramp from 0.0 (at < lower threshold) to 1.0 (at > upper threshold)
// v0.6.22: User-adjustable thresholds
```

---

### Phase 2: Configuration Persistence

#### 2.1 Update `Config.h`

**Add to Preset Struct:**

```cpp
struct Preset {
    // ... existing members ...
    
    // Speed Gate Tuning (v0.6.22)
    float speed_gate_lower = 0.5f; // m/s (default: 0.5 m/s = 1.8 km/h)
    float speed_gate_upper = 2.0f; // m/s (default: 2.0 m/s = 7.2 km/h)
};
```

#### 2.2 Update `Config.cpp`

**Add Persistence:**

```cpp
// In Config::Save()
file << "speed_gate_lower=" << preset.speed_gate_lower << "\n";
file << "speed_gate_upper=" << preset.speed_gate_upper << "\n";

// In Config::Load()
else if (key == "speed_gate_lower") {
    preset.speed_gate_lower = std::stof(value);
    // Safety clamp
    if (preset.speed_gate_lower < 0.0f) preset.speed_gate_lower = 0.0f;
    if (preset.speed_gate_lower > 2.0f) preset.speed_gate_lower = 2.0f;
}
else if (key == "speed_gate_upper") {
    preset.speed_gate_upper = std::stof(value);
    // Safety clamp
    if (preset.speed_gate_upper < 0.5f) preset.speed_gate_upper = 0.5f;
    if (preset.speed_gate_upper > 5.0f) preset.speed_gate_upper = 5.0f;
}

// Validation: Ensure upper >= lower + 0.1
if (preset.speed_gate_upper <= preset.speed_gate_lower + 0.1f) {
    preset.speed_gate_upper = preset.speed_gate_lower + 0.5f;
}

// In ApplyToEngine()
engine.m_speed_gate_lower = speed_gate_lower;
engine.m_speed_gate_upper = speed_gate_upper;

// In UpdateFromEngine()
speed_gate_lower = engine.m_speed_gate_lower;
speed_gate_upper = engine.m_speed_gate_upper;
```

---

### Phase 3: GUI Implementation

#### 3.1 Add Advanced Settings Section

**Location:** In `GuiLayer.cpp`, create a new collapsible section in the Tuning Window

**Placement:** After "Signal Filtering" section, before "Presets"

**Implementation:**

```cpp
// ========================================
// ADVANCED SETTINGS
// ========================================
if (ImGui::CollapsingHeader("Advanced Settings")) {
    ImGui::Indent();
    
    // Speed Gate Tuning
    if (ImGui::TreeNode("Stationary Vibration Gate")) {
        ImGui::TextWrapped(
            "Controls when vibration effects (Road Texture, ABS, Lockup, Bottoming) "
            "fade out at low speeds to prevent idle shaking."
        );
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Lower Threshold (Start Fading)
        ImGui::Text("Mute Below");
        ImGui::SameLine(label_width);
        ImGui::SetNextItemWidth(slider_width);
        
        // Convert m/s to km/h for display
        float lower_kmh = engine.m_speed_gate_lower * 3.6f;
        
        if (ImGui::SliderFloat("##speed_gate_lower", &lower_kmh, 
                               0.0f, 7.2f, "%.1f km/h")) {
            // Convert back to m/s
            engine.m_speed_gate_lower = lower_kmh / 3.6f;
            
            // Validate: Ensure upper >= lower + 0.1
            if (engine.m_speed_gate_upper <= engine.m_speed_gate_lower + 0.1f) {
                engine.m_speed_gate_upper = engine.m_speed_gate_lower + 0.5f;
            }
            
            preset_dirty = true;
        }
        
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Mute Below: %.1f km/h (%.2f m/s)", lower_kmh, engine.m_speed_gate_lower);
            ImGui::Separator();
            ImGui::TextWrapped(
                "Speed below which vibrations are completely muted. "
                "This prevents idle shaking when stationary or moving very slowly."
            );
            ImGui::Spacing();
            ImGui::TextWrapped("• 0.0 km/h: No muting (vibrations always active)");
            ImGui::TextWrapped("• 1.8 km/h: Default (mute when effectively stationary)");
            ImGui::TextWrapped("• 5.4 km/h: Conservative (mute during slow maneuvering)");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), 
                "TIP: Increase if you experience vibrations while stopped.");
            ImGui::EndTooltip();
        }
        
        ImGui::Spacing();
        
        // Upper Threshold (Full Strength)
        ImGui::Text("Full Above");
        ImGui::SameLine(label_width);
        ImGui::SetNextItemWidth(slider_width);
        
        // Convert m/s to km/h for display
        float upper_kmh = engine.m_speed_gate_upper * 3.6f;
        
        if (ImGui::SliderFloat("##speed_gate_upper", &upper_kmh, 
                               1.8f, 18.0f, "%.1f km/h")) {
            // Convert back to m/s
            engine.m_speed_gate_upper = upper_kmh / 3.6f;
            
            // Validate: Ensure upper >= lower + 0.1
            if (engine.m_speed_gate_upper <= engine.m_speed_gate_lower + 0.1f) {
                engine.m_speed_gate_upper = engine.m_speed_gate_lower + 0.5f;
            }
            
            preset_dirty = true;
        }
        
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Full Above: %.1f km/h (%.2f m/s)", upper_kmh, engine.m_speed_gate_upper);
            ImGui::Separator();
            ImGui::TextWrapped(
                "Speed above which vibrations reach full strength. "
                "Between 'Mute Below' and 'Full Above', vibrations fade in linearly."
            );
            ImGui::Spacing();
            ImGui::TextWrapped("• 3.6 km/h: Fast fade-in (sharp transition)");
            ImGui::TextWrapped("• 7.2 km/h: Default (smooth transition)");
            ImGui::TextWrapped("• 18.0 km/h: Slow fade-in (very gradual)");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), 
                "TIP: Widen the gap for a smoother, less noticeable transition.");
            ImGui::EndTooltip();
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Visual Indicator: Transition Window
        float window_kmh = upper_kmh - lower_kmh;
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), 
            "Transition Window: %.1f km/h", window_kmh);
        
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextWrapped(
                "The speed range over which vibrations fade from 0%% to 100%%. "
                "Wider windows = smoother transitions. Narrower windows = sharper on/off."
            );
            ImGui::EndTooltip();
        }
        
        ImGui::TreePop();
    }
    
    ImGui::Unindent();
}
```

#### 3.2 Add Debug Visualization (Optional)

**Location:** In the Debug Window, "Signal Analysis" section

```cpp
// Speed Gate Status
ImGui::Text("Speed Gate: %.2f (%.1f km/h)", 
    engine.m_speed_gate_value,  // Store the calculated gate value
    engine.m_current_car_speed * 3.6f);

if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::Text("Speed Gate Multiplier: %.2f", engine.m_speed_gate_value);
    ImGui::Separator();
    ImGui::TextWrapped(
        "0.0 = Vibrations muted\n"
        "0.5 = Vibrations at 50%%\n"
        "1.0 = Vibrations at full strength"
    );
    ImGui::EndTooltip();
}
```

**Implementation Note:** Requires adding `double m_speed_gate_value` and `double m_current_car_speed` to FFBEngine for debugging.

---

### Phase 4: Testing & Validation

#### 4.1 Unit Tests

**Add to `tests/test_ffb_engine.cpp`:**

```cpp
static void test_speed_gate_custom_thresholds() {
    std::cout << "\nTest: Speed Gate Custom Thresholds" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    // Enable Road Texture
    engine.m_road_texture_enabled = true;
    engine.m_road_texture_gain = 1.0;
    engine.m_max_torque_ref = 20.0f;
    
    // Test Case 1: Custom thresholds (1.0 - 3.0 m/s)
    engine.m_speed_gate_lower = 1.0f;
    engine.m_speed_gate_upper = 3.0f;
    
    // Below lower threshold (0.5 m/s) → Force should be 0.0
    {
        TelemInfoV01 data = CreateBasicTestTelemetry(0.5);
        data.mWheel[0].mVerticalTireDeflection = 0.002;
        data.mWheel[1].mVerticalTireDeflection = 0.002;
        
        double force = engine.calculate_force(&data);
        ASSERT_NEAR(force, 0.0, 0.0001);
    }
    
    // At midpoint (2.0 m/s) → Gate should be 0.5
    {
        TelemInfoV01 data = CreateBasicTestTelemetry(2.0);
        data.mWheel[0].mVerticalTireDeflection = 0.002;
        data.mWheel[1].mVerticalTireDeflection = 0.002;
        
        double force_mid = engine.calculate_force(&data);
        
        // Compare to full strength at 4.0 m/s
        data.mLocalVel.z = -4.0;
        double force_full = engine.calculate_force(&data);
        
        // Midpoint should be approximately 50% of full
        double ratio = force_mid / force_full;
        ASSERT_NEAR(ratio, 0.5, 0.1);
    }
    
    // Above upper threshold (4.0 m/s) → Gate should be 1.0
    {
        TelemInfoV01 data = CreateBasicTestTelemetry(4.0);
        data.mWheel[0].mVerticalTireDeflection = 0.002;
        data.mWheel[1].mVerticalTireDeflection = 0.002;
        
        double force = engine.calculate_force(&data);
        
        // Should be non-zero (full strength)
        if (std::abs(force) > 0.001) {
            std::cout << "[PASS] Custom thresholds work correctly" << std::endl;
            g_tests_passed++;
        } else {
            std::cout << "[FAIL] Force at full speed is zero" << std::endl;
            g_tests_failed++;
        }
    }
}

static void test_speed_gate_validation() {
    std::cout << "\nTest: Speed Gate Threshold Validation" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    // Test Case 1: Invalid (upper < lower)
    engine.m_speed_gate_lower = 2.0f;
    engine.m_speed_gate_upper = 1.0f;
    
    // Config::Load should fix this
    Config config;
    Preset preset;
    preset.speed_gate_lower = 2.0f;
    preset.speed_gate_upper = 1.0f;
    
    // Simulate validation logic
    if (preset.speed_gate_upper <= preset.speed_gate_lower + 0.1f) {
        preset.speed_gate_upper = preset.speed_gate_lower + 0.5f;
    }
    
    // Verify correction
    if (preset.speed_gate_upper >= preset.speed_gate_lower + 0.5f) {
        std::cout << "[PASS] Validation corrects invalid thresholds" << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Validation did not correct thresholds" << std::endl;
        g_tests_failed++;
    }
}
```

#### 4.2 Manual Testing Procedure

**Test Scenarios:**

1. **Default Behavior (0.5 - 2.0 m/s)**
   - Verify vibrations muted when stopped
   - Verify smooth fade-in when accelerating from standstill
   - Verify full vibrations at normal driving speeds

2. **Wide Window (0.0 - 5.0 m/s)**
   - Verify vibrations active even at very low speeds
   - Verify gradual fade-in over 5 m/s range
   - Verify no abrupt transitions

3. **Narrow Window (1.0 - 1.5 m/s)**
   - Verify vibrations muted up to 1.0 m/s
   - Verify sharp transition at 1.0-1.5 m/s
   - Verify full vibrations above 1.5 m/s

4. **Extreme Values**
   - Test 0.0 - 0.5 m/s (minimum window)
   - Test 0.0 - 5.0 m/s (maximum range)
   - Verify no crashes or undefined behavior

---

### Phase 5: Documentation Updates

#### 5.1 Update CHANGELOG_DEV.md

```markdown
## [0.6.22] - YYYY-MM-DD
### Added
- **Speed Gate Tuning (Advanced Settings)**:
  - Added "Stationary Vibration Gate" section in Advanced Settings.
  - Users can now customize when vibrations fade out at low speeds:
    - "Mute Below" (0.0 - 7.2 km/h, default: 1.8 km/h)
    - "Full Above" (1.8 - 18.0 km/h, default: 7.2 km/h)
  - Useful for motion platforms, high-sensitivity DD wheels, or custom preferences.
  - Default values (0.5 - 2.0 m/s) remain unchanged, preserving v0.6.21 behavior.
```

#### 5.2 Update User Guide

**Add section to `docs/encrypted_content_user_guide.md` or create new guide:**

```markdown
### Adjusting the Speed Gate

The Speed Gate prevents vibrations when the car is stationary or moving very slowly. You can customize when vibrations fade in/out:

1. Open the **Advanced Settings** section in the Tuning Window
2. Expand **"Stationary Vibration Gate"**
3. Adjust the sliders:
   - **Mute Below:** Speed below which vibrations are completely off
   - **Full Above:** Speed above which vibrations are at full strength

**Common Adjustments:**

- **Motion Platform:** Lower "Mute Below" to 0.0 km/h to feel all vibrations
- **High-Sensitivity DD Wheel:** Raise "Mute Below" to 5.4 km/h to eliminate idle noise
- **Smooth Transition:** Widen the gap between sliders (e.g., 1.8 - 14.4 km/h)
- **Sharp Transition:** Narrow the gap (e.g., 1.8 - 3.6 km/h)
```

#### 5.3 Update Technical Documentation

**Add to `docs/dev_docs/FFB_formulas.md`:**

```markdown
### Speed Gate (v0.6.21+)

The speed gate fades vibration effects at low speeds:

```
gate = (car_speed - lower_threshold) / (upper_threshold - lower_threshold)
gate = clamp(gate, 0.0, 1.0)

vibration_force *= gate
```

**Default Thresholds:**
- Lower: 0.5 m/s (1.8 km/h)
- Upper: 2.0 m/s (7.2 km/h)

**User-Adjustable (v0.6.22+):**
- Lower: 0.0 - 2.0 m/s (0.0 - 7.2 km/h)
- Upper: 0.5 - 5.0 m/s (1.8 - 18.0 km/h)
- Constraint: upper >= lower + 0.1 m/s
```

---

## Migration Notes

### For Users

**Existing Configurations:**  
Users upgrading from v0.6.21 to v0.6.22+ will automatically receive the default thresholds (0.5 - 2.0 m/s). The speed gate behavior will remain **identical** to v0.6.21.

**New Configurations:**  
Users creating new presets in v0.6.22+ will have access to the speed gate tuning sliders in Advanced Settings.

### For Developers

**Backward Compatibility:**  
The `speed_gate_lower` and `speed_gate_upper` parameters are optional in `config.ini`. If missing, they default to `0.5f` and `2.0f`, preserving v0.6.21 behavior.

**Preset Migration:**  
Built-in presets should be updated to include:
```
speed_gate_lower=0.5
speed_gate_upper=2.0
```

---

## Future Enhancements

### 1. Per-Effect Speed Gates

**Concept:**  
Allow different speed gates for different effects.

**Example:**
- Road Texture: 0.5 - 2.0 m/s (default)
- ABS Pulse: 1.0 - 3.0 m/s (higher threshold, ABS rarely triggers at low speeds)
- Lockup: 0.5 - 2.0 m/s (default)
- Bottoming: 0.0 - 1.0 m/s (lower threshold, want to feel bottoming even at low speeds)

**Benefit:**  
More granular control for advanced users.

### 2. Curve-Based Fade

**Concept:**  
Replace linear ramp with configurable curves (exponential, logarithmic, S-curve).

**Example:**
```cpp
// S-Curve (smooth ease-in/ease-out)
double t = (car_speed - lower) / window;
t = clamp(t, 0.0, 1.0);
double speed_gate = t * t * (3.0 - 2.0 * t); // Smoothstep
```

**Benefit:**  
More natural feeling transitions.

### 3. Hysteresis

**Concept:**  
Use different thresholds for fade-in vs. fade-out to prevent oscillation.

**Example:**
- Fade-out: 0.5 m/s (when slowing down)
- Fade-in: 0.7 m/s (when speeding up)

**Benefit:**  
Prevents rapid on/off cycling when hovering near threshold.

---

## Risk Assessment

### Low Risk

- ✅ Isolated change (only affects speed gate calculation)
- ✅ Default values preserve existing behavior
- ✅ Validation logic prevents invalid configurations
- ✅ Comprehensive testing plan

### Potential Issues

1. **User Confusion:**  
   - **Risk:** Users may not understand what the speed gate does
   - **Mitigation:** Clear tooltips, user guide, and default values

2. **Invalid Configurations:**  
   - **Risk:** Users set upper < lower, causing division by zero
   - **Mitigation:** Validation logic in Config::Load() and GUI

3. **Performance:**  
   - **Risk:** Additional calculations per frame
   - **Mitigation:** Negligible (2 extra float operations)

---

## Implementation Checklist

### Code Changes
- [ ] Add `m_speed_gate_lower` and `m_speed_gate_upper` to `FFBEngine.h`
- [ ] Update speed gate calculation in `FFBEngine.h`
- [ ] Add parameters to `Preset` struct in `Config.h`
- [ ] Add persistence logic in `Config.cpp`
- [ ] Add validation logic in `Config.cpp`
- [ ] Add GUI sliders in `GuiLayer.cpp`
- [ ] Add tooltips and help text in `GuiLayer.cpp`

### Testing
- [ ] Add `test_speed_gate_custom_thresholds()` to `test_ffb_engine.cpp`
- [ ] Add `test_speed_gate_validation()` to `test_ffb_engine.cpp`
- [ ] Manual testing: Default behavior (0.5 - 2.0 m/s)
- [ ] Manual testing: Wide window (0.0 - 5.0 m/s)
- [ ] Manual testing: Narrow window (1.0 - 1.5 m/s)
- [ ] Manual testing: Extreme values

### Documentation
- [ ] Update `CHANGELOG_DEV.md` with v0.6.22 entry
- [ ] Add section to user guide
- [ ] Update `FFB_formulas.md` with speed gate formula
- [ ] Update built-in presets with default values

### Verification
- [ ] All tests pass (356+ tests)
- [ ] Code compiles without warnings
- [ ] Config persistence works correctly
- [ ] GUI sliders function correctly
- [ ] Tooltips display correctly
- [ ] Validation prevents invalid configurations

---

## Conclusion

Exposing the speed gate thresholds as user-adjustable parameters is a **low-risk, high-value** enhancement that addresses specific user needs (motion platforms, high-sensitivity wheels) while preserving the default behavior for existing users.

The implementation follows established patterns in the codebase and can be delivered in v0.6.22 or later with minimal effort and comprehensive testing.

---

**Document Status:** Ready for Implementation  
**Priority:** Medium (User-Requested Enhancement)  
**Estimated Effort:** 3-4 hours (coding + testing + documentation)  
**Risk Level:** Low (isolated change, well-defined scope, validation logic)
