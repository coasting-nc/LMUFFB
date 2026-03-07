# Implementation Plan: Min/Max Threshold Mapping for Slope Detection

## Context

This plan describes the implementation of a **Min/Max Threshold System** for the Slope Detection algorithm, adopted from Marvin's AIRA. The change replaces the current single-threshold-plus-sensitivity approach with a two-threshold system that provides a dead zone, linear response region, and saturation.

### Problem Statement

The current Slope Detection uses a single threshold with an arbitrary sensitivity multiplier:

```cpp
if (m_slope_current < m_slope_negative_threshold) {
    double excess = m_slope_negative_threshold - m_slope_current;
    current_grip_factor = 1.0 - (excess * 0.1 * m_slope_sensitivity);
}
```

Issues:
1. **No dead zone** - Any slope below threshold triggers effect, including noise
2. **Arbitrary scaling** - `excess * 0.1 * sensitivity` has no intuitive meaning
3. **No saturation** - Effect can exceed 100% or go negative theoretically
4. **Hard to tune** - Users don't know what "sensitivity 0.5" means

### Proposed Solution

Replace with **InverseLerp threshold mapping**:
- `m_slope_min_threshold`: Slope where effect **starts** (dead zone edge)
- `m_slope_max_threshold`: Slope where effect reaches **100%** (saturation point)
- Effect intensity = InverseLerp(min, max, slope)

---

## Reference Documents

1. **Source Implementation:**
   - `docs/dev_docs/tech_from_other_apps/Marvin's AIRA Refactored FFB_Effects_Technical_Report.md` - Section "Threshold System (Min/Max with Linear Interpolation)"

2. **Related Plans:**
   - `docs/dev_docs/implementation_plans/plan_slope_detection.md`
   - `docs/dev_docs/implementation_plans/plan_slope_detection_fixes_v0.7.1.md`

---

## Codebase Analysis Summary

### Current Implementation

**Location:** `FFBEngine.h` lines 846-854

```cpp
double current_grip_factor = 1.0;
if (m_slope_current < (double)m_slope_negative_threshold) {
    // Slope is negative -> tire is sliding
    double excess = (double)m_slope_negative_threshold - m_slope_current;
    current_grip_factor = 1.0 - (excess * 0.1 * (double)m_slope_sensitivity);
}

// Apply Floor (Safety)
current_grip_factor = (std::max)(0.2, (std::min)(1.0, current_grip_factor));
```

### Current Settings

| Setting | FFBEngine.h | Config.h | Default | Purpose |
|---------|-------------|----------|---------|---------|
| `m_slope_negative_threshold` | Line 319 | Line 107 | -0.3 | Single trigger threshold |
| `m_slope_sensitivity` | Line 318 | Line 106 | 0.5 | Arbitrary multiplier |

### Data Flow

```
Telemetry → calculate_slope_grip() → grip_factor → Understeer Effect
                    ↓
            m_slope_current (from derivative calc)
                    ↓
            Threshold comparison
                    ↓
            Effect intensity
```

---

## FFB Effects Impact Analysis

### Affected Effect: Understeer (via Slope Detection)

| Aspect | Current Behavior | New Behavior |
|--------|------------------|--------------|
| **Dead Zone** | None - any slope < threshold triggers | Slopes above min_threshold ignored |
| **Response Curve** | `(threshold - slope) * 0.1 * sensitivity` | Linear between min and max |
| **Saturation** | None (floor at 0.2 applied later) | Saturates at max_threshold |
| **Tunability** | Abstract "sensitivity" | Intuitive "start" and "full" thresholds |

### User Experience Changes

| Scenario | Current | New |
|----------|---------|-----|
| Minor tire squeal (slope = -0.4) | Triggers small effect | Dead zone - no effect |
| Moderate slide (slope = -1.0) | Triggers based on sensitivity | Linear ramp ~41% effect |
| Heavy understeer (slope = -2.0) | May exceed intended range | Saturated at 100% |
| Extreme (slope = -5.0) | Undefined behavior | Still at 100% (saturated) |

### Default Values (Proposed)

| Setting | Value | Meaning |
|---------|-------|---------|
| `m_slope_min_threshold` | -0.3 | Effect starts when slope drops below -0.3 |
| `m_slope_max_threshold` | -2.0 | Effect reaches 100% at slope of -2.0 |

---

## Proposed Changes

### Change 1: Add InverseLerp Helper Function

**File:** `FFBEngine.h`  
**Location:** After line 790 (helper functions section)

```cpp
// Helper: Inverse linear interpolation - v0.7.11
// Returns normalized position of value between min and max
// Returns 0 if value >= min, 1 if value <= max (for negative threshold use)
// Clamped to [0, 1] range
inline double inverse_lerp(double min_val, double max_val, double value) {
    double range = max_val - min_val;
    if (std::abs(range) < 0.0001) return (value < min_val) ? 0.0 : 1.0;
    
    double t = (value - min_val) / range;
    return (std::max)(0.0, (std::min)(1.0, t));
}
```

### Change 2: Add New Settings

**File:** `FFBEngine.h`  
**Location:** After line 320 (slope detection settings section)

```cpp
// ===== SLOPE DETECTION (v0.7.0 → v0.7.11 enhancements) =====
bool m_slope_detection_enabled = false;
int m_slope_sg_window = 15;
float m_slope_sensitivity = 0.5f;            // DEPRECATED v0.7.11 - kept for migration
float m_slope_negative_threshold = -0.3f;    // DEPRECATED v0.7.11 - use min_threshold
float m_slope_smoothing_tau = 0.04f;

// NEW v0.7.11: Min/Max Threshold System
float m_slope_min_threshold = -0.3f;   // Effect starts here (dead zone edge)
float m_slope_max_threshold = -2.0f;   // Effect saturates here (100%)
```

### Change 3: Modify calculate_slope_grip()

**File:** `FFBEngine.h`  
**Location:** Lines 846-854 (inside `calculate_slope_grip()`)

```cpp
// BEFORE: Single threshold with sensitivity
double current_grip_factor = 1.0;
if (m_slope_current < (double)m_slope_negative_threshold) {
    double excess = (double)m_slope_negative_threshold - m_slope_current;
    current_grip_factor = 1.0 - (excess * 0.1 * (double)m_slope_sensitivity);
}

// AFTER: Min/Max threshold with inverse_lerp
double current_grip_factor = 1.0;
if (m_slope_current < (double)m_slope_min_threshold) {
    // Slope is below minimum threshold -> calculate effect intensity
    // inverse_lerp returns 0 at min, 1 at max
    double effect_intensity = inverse_lerp(
        (double)m_slope_min_threshold,   // 0% effect here
        (double)m_slope_max_threshold,   // 100% effect here  
        m_slope_current
    );
    current_grip_factor = 1.0 - effect_intensity;
}
```

---

## Parameter Synchronization Checklist

### New Settings: `m_slope_min_threshold`, `m_slope_max_threshold`

| Step | File | Location | Required |
|------|------|----------|----------|
| Declaration in FFBEngine.h | FFBEngine.h | After line 320 | ✓ |
| Declaration in Preset struct | Config.h | After line 108 | ✓ |
| Add to `SetSlopeDetection()` | Config.h | Line 185 | ✓ |
| Entry in `Preset::Apply()` | Config.h | After line 283 | ✓ |
| Entry in `Preset::UpdateFromEngine()` | Config.h | After line 353 | ✓ |
| Entry in `Config::Save()` | Config.cpp | Slope section | ✓ |
| Entry in `Config::Load()` | Config.cpp | Slope parsing | ✓ |
| Validation logic | Config.cpp | Load validation | ✓ |
| GUI sliders | GuiLayer.cpp | Slope section | ✓ |

### Config.h Preset Struct Addition

```cpp
// After line 108
float slope_min_threshold = -0.3f;   // v0.7.11
float slope_max_threshold = -2.0f;   // v0.7.11
```

### Config.h SetSlopeDetection() Update

```cpp
// Update signature
Preset& SetSlopeDetection(bool enabled, int window = 15, float min_thresh = -0.3f, 
                          float max_thresh = -2.0f, float tau = 0.04f) {
    slope_detection_enabled = enabled;
    slope_sg_window = window;
    slope_min_threshold = min_thresh;
    slope_max_threshold = max_thresh;
    slope_smoothing_tau = tau;
    return *this;
}
```

### Config.h Apply() Addition

```cpp
// After line 283
engine.m_slope_min_threshold = slope_min_threshold;
engine.m_slope_max_threshold = slope_max_threshold;
```

### Config.h UpdateFromEngine() Addition

```cpp
// After line 353
slope_min_threshold = engine.m_slope_min_threshold;
slope_max_threshold = engine.m_slope_max_threshold;
```

### Migration Logic

**File:** `Config.cpp` (in Load validation section)

```cpp
// Migration: v0.7.x sensitivity → v0.7.11 thresholds
// If loading old config with sensitivity but at default thresholds
if (engine.m_slope_min_threshold == -0.3f && 
    engine.m_slope_max_threshold == -2.0f &&
    engine.m_slope_sensitivity != 0.5f) {
    
    // Old formula: factor = 1 - (excess * 0.1 * sens)
    // At factor=0.2 (floor): excess * 0.1 * sens = 0.8
    // excess = 0.8 / (0.1 * sens) = 8 / sens
    // max = min - excess = -0.3 - (8/sens)
    double sens = (double)engine.m_slope_sensitivity;
    if (sens > 0.01) {
        engine.m_slope_max_threshold = engine.m_slope_min_threshold - (8.0f / sens);
        std::cout << "[Config] Migrated slope_sensitivity " << sens 
                  << " to max_threshold " << engine.m_slope_max_threshold << std::endl;
    }
}

// Validation: max should be more negative than min
if (engine.m_slope_max_threshold > engine.m_slope_min_threshold) {
    std::swap(engine.m_slope_min_threshold, engine.m_slope_max_threshold);
    std::cout << "[Config] Swapped slope thresholds (min should be > max)" << std::endl;
}
```

---

## GUI Changes

**File:** `GuiLayer.cpp`  
**Location:** Inside Slope Detection settings section (around line 1172)

```cpp
// BEFORE: Single threshold
FloatSetting("  Slope Threshold", &engine.m_slope_negative_threshold, -1.0f, 0.0f, "%.2f", ...);
FloatSetting("  Slope Sensitivity", &engine.m_slope_sensitivity, 0.1f, 2.0f, "%.2f", ...);

// AFTER: Min/Max thresholds
FloatSetting("  Effect Start", &engine.m_slope_min_threshold, -1.0f, 0.0f, "%.2f",
    "Slope value where understeer effect BEGINS.\n"
    "Values above this (closer to 0) are ignored (dead zone).\n"
    "Default: -0.3 (slight tire squeal triggers effect)");
    
FloatSetting("  Effect Full", &engine.m_slope_max_threshold, -5.0f, -0.1f, "%.2f",
    "Slope value where understeer effect reaches MAXIMUM.\n"
    "Must be more negative than 'Effect Start'.\n"
    "Default: -2.0 (heavy slide = full FFB lightening)\n\n"
    "Tuning Guide:\n"
    "- Wider range (min=-0.3, max=-3.0): Gradual, progressive feel\n"
    "- Narrow range (min=-0.3, max=-0.8): Sharper on/off response");

// Remove or hide deprecated sensitivity slider
// FloatSetting("  Slope Sensitivity", ...) // DEPRECATED
```

---

## Test Plan (TDD-Ready)

### Test 1: `test_inverse_lerp_helper`

**Purpose:** Verify inverse_lerp returns mathematically correct values

**Test Script:**
```cpp
static void test_inverse_lerp_helper() {
    std::cout << "\nTest: InverseLerp Helper Function (v0.7.11)" << std::endl;
    FFBEngine engine;
    
    // Note: For slope thresholds, min is less negative (-0.3), max is more negative (-2.0)
    // slope=-0.3 → 0%, slope=-2.0 → 100%
    
    // At min (start of range)
    double at_min = engine.inverse_lerp(-0.3, -2.0, -0.3);
    ASSERT_NEAR(at_min, 0.0, 0.001);
    
    // At max (end of range)
    double at_max = engine.inverse_lerp(-0.3, -2.0, -2.0);
    ASSERT_NEAR(at_max, 1.0, 0.001);
    
    // At midpoint (-1.15)
    double at_mid = engine.inverse_lerp(-0.3, -2.0, -1.15);
    ASSERT_NEAR(at_mid, 0.5, 0.001);
    
    // Above min (dead zone)
    double dead_zone = engine.inverse_lerp(-0.3, -2.0, 0.0);
    ASSERT_NEAR(dead_zone, 0.0, 0.001);
    
    // Below max (saturated)
    double saturated = engine.inverse_lerp(-0.3, -2.0, -5.0);
    ASSERT_NEAR(saturated, 1.0, 0.001);
}
```

### Test 2: `test_slope_minmax_dead_zone`

**Purpose:** Verify slopes above min_threshold produce no effect

**Test Script:**
```cpp
static void test_slope_minmax_dead_zone() {
    std::cout << "\nTest: Slope Min/Max Dead Zone (v0.7.11)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    engine.m_slope_min_threshold = -0.3f;
    engine.m_slope_max_threshold = -2.0f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    
    // Simulate slopes in dead zone
    for (double slope : {0.0, -0.1, -0.2, -0.29}) {
        engine.m_slope_current = slope;
        engine.m_slope_smoothed_output = 1.0;  // Reset
        
        // Run multiple frames to settle smoothing
        for (int i = 0; i < 20; i++) {
            engine.calculate_slope_grip(0.5, 0.05, 0.01);
        }
        
        // Should remain at 1.0 (full grip)
        ASSERT_GE(engine.m_slope_smoothed_output, 0.98);
    }
}
```

### Test 3: `test_slope_minmax_linear_response`

**Purpose:** Verify linear scaling between thresholds

**Multi-Frame Telemetry Script:**
| Slope | Position in Range | Expected Effect | Expected Grip |
|-------|-------------------|-----------------|---------------|
| -0.3 | 0% (min) | 0% | 1.0 |
| -0.725 | 25% | 25% | 0.75 |
| -1.15 | 50% | 50% | 0.5 |
| -1.575 | 75% | 75% | 0.25 |
| -2.0 | 100% (max) | 100% | 0.0 → 0.2 |

**Test Script:**
```cpp
static void test_slope_minmax_linear_response() {
    std::cout << "\nTest: Slope Min/Max Linear Response (v0.7.11)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    engine.m_slope_min_threshold = -0.3f;
    engine.m_slope_max_threshold = -2.0f;
    engine.m_slope_smoothing_tau = 0.001f;  // Fast smoothing for test
    
    // At 25% into range: slope = -0.3 + 0.25 * (-2.0 - (-0.3)) = -0.725
    engine.m_slope_current = -0.725;
    engine.m_slope_smoothed_output = 1.0;
    for (int i = 0; i < 50; i++) {
        engine.calculate_slope_grip(0.5, 0.05, 0.01);
    }
    ASSERT_NEAR(engine.m_slope_smoothed_output, 0.75, 0.05);
    
    // At 50% into range: slope = -1.15
    engine.m_slope_current = -1.15;
    engine.m_slope_smoothed_output = 1.0;
    for (int i = 0; i < 50; i++) {
        engine.calculate_slope_grip(0.5, 0.05, 0.01);
    }
    ASSERT_NEAR(engine.m_slope_smoothed_output, 0.5, 0.05);
    
    // At 100% (max): grip should hit floor
    engine.m_slope_current = -2.0;
    engine.m_slope_smoothed_output = 1.0;
    for (int i = 0; i < 50; i++) {
        engine.calculate_slope_grip(0.5, 0.05, 0.01);
    }
    ASSERT_NEAR(engine.m_slope_smoothed_output, 0.2, 0.05);  // Floor
}
```

### Test 4: `test_slope_minmax_saturation`

**Purpose:** Verify slopes beyond max are saturated at 100%

**Test Script:**
```cpp
static void test_slope_minmax_saturation() {
    std::cout << "\nTest: Slope Min/Max Saturation (v0.7.11)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    engine.m_slope_min_threshold = -0.3f;
    engine.m_slope_max_threshold = -2.0f;
    engine.m_slope_smoothing_tau = 0.001f;
    
    // Extreme slope (way beyond max)
    engine.m_slope_current = -10.0;
    engine.m_slope_smoothed_output = 1.0;
    for (int i = 0; i < 50; i++) {
        engine.calculate_slope_grip(0.5, 0.05, 0.01);
    }
    
    // Should saturate at floor (0.2), not go negative or beyond
    ASSERT_NEAR(engine.m_slope_smoothed_output, 0.2, 0.02);
}
```

### Test 5: `test_slope_threshold_config_persistence`

**Purpose:** Verify new settings are saved and loaded correctly

**Test Script:**
```cpp
static void test_slope_threshold_config_persistence() {
    std::cout << "\nTest: Slope Threshold Config Persistence (v0.7.11)" << std::endl;
    std::string test_file = "test_slope_minmax.ini";
    
    FFBEngine engine_save;
    engine_save.m_slope_min_threshold = -0.5f;
    engine_save.m_slope_max_threshold = -3.0f;
    Config::Save(engine_save, test_file);
    
    FFBEngine engine_load;
    InitializeEngine(engine_load);
    Config::Load(engine_load, test_file);
    
    ASSERT_NEAR(engine_load.m_slope_min_threshold, -0.5f, 0.001);
    ASSERT_NEAR(engine_load.m_slope_max_threshold, -3.0f, 0.001);
    
    std::remove(test_file.c_str());
}
```

### Test 6: `test_slope_sensitivity_migration`

**Purpose:** Verify legacy sensitivity values are migrated to new thresholds

**Test Script:**
```cpp
static void test_slope_sensitivity_migration() {
    std::cout << "\nTest: Slope Sensitivity Migration (v0.7.11)" << std::endl;
    std::string test_file = "test_slope_migration.ini";
    
    // Create legacy config
    {
        std::ofstream file(test_file);
        file << "slope_detection_enabled=1\n";
        file << "slope_sensitivity=1.0\n";           // Legacy
        file << "slope_negative_threshold=-0.3\n";   // Legacy
        // No slope_min_threshold or slope_max_threshold
    }
    
    FFBEngine engine;
    InitializeEngine(engine);
    Config::Load(engine, test_file);
    
    // With sensitivity=1.0, max_threshold should be calculated
    // Formula: max = min - (8/sens) = -0.3 - 8 = -8.3
    ASSERT_TRUE(engine.m_slope_max_threshold < engine.m_slope_min_threshold);
    ASSERT_NEAR(engine.m_slope_max_threshold, -8.3f, 0.5);
    
    std::remove(test_file.c_str());
}
```

### Test 7: `test_inverse_lerp_edge_cases`

**Purpose:** Test boundary conditions

**Test Script:**
```cpp
static void test_inverse_lerp_edge_cases() {
    std::cout << "\nTest: InverseLerp Edge Cases (v0.7.11)" << std::endl;
    FFBEngine engine;
    
    // Min == Max (degenerate)
    double same = engine.inverse_lerp(-0.3, -0.3, -0.3);
    ASSERT_TRUE(same == 0.0 || same == 1.0);
    
    // Very small range
    double tiny = engine.inverse_lerp(-0.3, -0.30001, -0.30001);
    ASSERT_NEAR(tiny, 1.0, 0.01);
    
    // Reversed order (should still work or be caught)
    double reversed = engine.inverse_lerp(-2.0, -0.3, -1.15);
    ASSERT_TRUE(reversed >= 0.0 && reversed <= 1.0);
}
```

---

## Deliverables

### Code Changes

- [x] Add `inverse_lerp()` helper function to FFBEngine.h
- [x] Add `m_slope_min_threshold` and `m_slope_max_threshold` settings
- [x] Modify `calculate_slope_grip()` to use min/max thresholds
- [x] Update Preset struct with new settings
- [x] Update `SetSlopeDetection()` fluent setter
- [x] Update `Preset::Apply()` and `UpdateFromEngine()`
- [x] Update `Config::Save()` and `Config::Load()`
- [x] Add migration logic for legacy sensitivity values
- [x] Update GUI with new threshold sliders

### Tests

- [x] test_inverse_lerp_helper
- [x] test_slope_minmax_dead_zone
- [x] test_slope_minmax_linear_response
- [x] test_slope_minmax_saturation
- [x] test_slope_threshold_config_persistence
- [x] test_slope_sensitivity_migration
- [x] test_inverse_lerp_edge_cases

### Documentation

- [x] Update CHANGELOG.md with v0.7.11 entry
- [x] Update Slope Detection user guide with new settings

---

## Implementation Notes

Implementation completed and verified with the test suite.

### Issues Encountered

1. **Division by Zero Warnings**: MSVC issued warning C4723 in `inverse_lerp` and `smoothstep`. This was addressed by restructuring the zero-range checks to be more explicit, although the user requested to postpone further suppression if it still appears.
2. **Test Expectations**: The switch from the old sensitivity formula to a linear min/max mapping changed the output values for identical slope inputs. Updated `test_ffb_slope_detection.cpp` to match the new v0.7.11 formula.
3. **Floating Point Precision**: Initial `inverse_lerp` tests failed on tiny ranges; corrected the logic to handle the "direction" of the threshold (negative slope) correctly.

### Deviations from Plan

1. **Version Number**: Per USER request, version was incremented to **v0.7.11** instead of v0.8.0 to follow the "minimal increment" rule.
2. **Helper Function Refactor**: Slightly modified the `inverse_lerp` implementation from the plan to handle the degenerate case where `max_val` < `min_val` more robustly for negative thresholds.

---

## Document History

| Version | Date | Author | Notes |
|---------|------|--------|-------|
| 1.0 | 2026-02-03 | Antigravity | Initial plan (split from combined plan) |
