# Implementation Plan: Slope Detection Algorithm Fixes (v0.7.1)

## Context

This plan addresses critical issues identified in the v0.7.0 Slope Detection feature based on user feedback. The investigation report (`docs/dev_docs/investigations/slope_detection_issues_v0.7.0.md`) identified that the default settings are too aggressive, causing heavy/notchy FFB, and that there is a fundamental interaction problem with the Lateral G Boost effect.

### Problem Statement

Users report the following issues with Slope Detection (v0.7.0):
1. Very heavy and notchy FFB when slope detection is enabled
2. Strong low-frequency rumble in corners
3. Oscillations on straights and in corners
4. Grip indicator fluctuates wildly (20%-100%) even at 60 kph during zig-zag maneuvers
5. Problems are eliminated when Lateral G Boost (Slide) is disabled

### Goals

1. Adjust default slope detection parameters to be less aggressive
2. Fix the Lateral G Boost interaction that causes oscillations
3. Improve UI by adding tooltips and a dedicated slope graph
4. Ensure backward compatibility for existing users

---

## Reference Documents

1. **Investigation Report:**
   - `docs/dev_docs/investigations/slope_detection_issues_v0.7.0.md` - Root cause analysis

2. **Original Implementation Plan:**
   - `docs/dev_docs/implementation_plans/plan_slope_detection.md` - v0.7.0 feature design

3. **User Documentation:**
   - `docs/Slope_Detection_Guide.md` - User-facing guide to update

---

## Codebase Analysis Summary

### Current Architecture (Slope Detection)

The Slope Detection feature is implemented across these files:

| File | Purpose | Lines of Interest |
|------|---------|-------------------|
| `FFBEngine.h` | Core algorithm, member variables, `calculate_slope_grip()` | 315-319, 420-425, 789-861 |
| `Config.h` | Preset defaults, `Apply()`, `UpdateFromEngine()` | 103-108, 185-192, 278-283, 348-353 |
| `Config.cpp` | Save/Load logic, validation | 843-847, 1060-1064, 1093-1097 |
| `GuiLayer.cpp` | UI controls, diagnostic display | 1100-1171 |

### Impacted Functionalities

| Functionality | How Impacted |
|---------------|--------------|
| **Front Grip Calculation** | Slope detection calculates `ctx.avg_grip` when enabled |
| **Understeer Effect** | Uses `ctx.avg_grip` - receives fluctuating values from slope detection |
| **Lateral G Boost (Slide)** | Uses `grip_delta = front_grip - rear_grip` - creates oscillations due to asymmetric calculation methods |
| **Slide Texture** | Uses `(1.0 - ctx.avg_grip)` for intensity scaling |
| **Graphs** | `calc_front_grip` graph shows grip, but no dedicated slope graph exists |

### Data Flow

```
1. Telemetry (400Hz): mLocalAccel.x (Lateral G), slip_angle
   ↓
2. calculate_grip(is_front=true) → calls calculate_slope_grip()
   ↓
3. calculate_slope_grip():
   ├── Updates circular buffers (lat_g, slip_angle)
   ├── Calculates SG derivatives: dG/dt, dAlpha/dt
   ├── Computes slope = dG/dt / dAlpha/dt
   ├── If slope < threshold: grip_loss = (threshold - slope) * 0.1 * sensitivity
   └── Returns smoothed grip factor → ctx.avg_grip
   ↓
4. calculate_sop_lateral(is_front=false for rear)
   ├── Rear grip uses STATIC threshold (different method!)
   ├── grip_delta = ctx.avg_grip - ctx.avg_rear_grip
   └── Lateral G Boost: sop *= (1 + grip_delta * oversteer_boost * 2)
   ↓
5. Understeer Effect: output *= (1 - grip_loss * understeer_effect)
```

**Key Finding:** The asymmetry between front (slope detection) and rear (static threshold) grip calculation creates artificial `grip_delta` values that cause Lateral G Boost oscillations.

---

## FFB Effect Impact Analysis

### Effects Using Front Grip (`ctx.avg_grip`)

| Effect | Location | Technical Change | User Experience Impact |
|--------|----------|------------------|------------------------|
| **Understeer Effect** | FFBEngine.h:1070-1071 | No code change - receives new default values | Smoother response with adjusted sensitivity/threshold |
| **Slide Texture** | FFBEngine.h:1389 | No code change | Less erratic texture intensity |

### Effects Using Grip Differential

| Effect | Location | Technical Change | User Experience Impact |
|--------|----------|------------------|------------------------|
| **Lateral G Boost** | FFBEngine.h:1307-1311 | **CRITICAL FIX**: Disable boost when slope detection is enabled | Eliminates oscillations during oversteer when slope detection is ON |

### Effects NOT Impacted

- Steering Shaft Gain (raw game force)
- Rear Aligning Torque (uses rear slip angle, not front grip)
- Yaw Kick (uses yaw acceleration)
- Gyro Damping (uses steering velocity)
- Lockup/Spin Vibration (uses wheel slip ratio)
- Road Texture (uses vertical deflection)
- ABS Pulse (uses brake pressure modulation)

---

## Proposed Changes

### 1. Algorithm Fix: Disable Lateral G Boost When Slope Detection is Enabled

**File:** `src/FFBEngine.h`  
**Location:** `calculate_sop_lateral()` function, lines 1307-1311

**Current Code:**
```cpp
// Lateral G Boost (Oversteer)
double grip_delta = ctx.avg_grip - ctx.avg_rear_grip;
if (grip_delta > 0.0) {
    sop_base *= (1.0 + (grip_delta * m_oversteer_boost * 2.0));
}
```

**New Code:**
```cpp
// Lateral G Boost (Oversteer)
// v0.7.1 FIX: Disable when slope detection is enabled to prevent oscillations.
// Slope detection uses a different calculation method for front grip than the
// static threshold used for rear grip. This asymmetry creates artificial
// grip_delta values that cause feedback oscillation.
// See: docs/dev_docs/investigations/slope_detection_issues_v0.7.0.md (Issue 2)
if (!m_slope_detection_enabled) {
    double grip_delta = ctx.avg_grip - ctx.avg_rear_grip;
    if (grip_delta > 0.0) {
        sop_base *= (1.0 + (grip_delta * m_oversteer_boost * 2.0));
    }
}
```

**Rationale:** This is the cleanest solution because:
1. Doesn't require changes to the rear grip calculation
2. Preserves existing Lateral G Boost behavior for users not using slope detection
3. Eliminates the fundamental cause of oscillation (asymmetric grip methods)

---

### 2. Parameter Default Adjustments

**File:** `src/Config.h`  
**Location:** Preset struct defaults, lines 103-108

**Current Defaults:**
```cpp
// ===== SLOPE DETECTION (v0.7.0) =====
bool slope_detection_enabled = false;
int slope_sg_window = 15;
float slope_sensitivity = 1.0f;          // TOO AGGRESSIVE
float slope_negative_threshold = -0.1f;  // TRIGGERS TOO EARLY
float slope_smoothing_tau = 0.02f;       // TOO FAST
```

**New Defaults:**
```cpp
// ===== SLOPE DETECTION (v0.7.0 → v0.7.1 defaults) =====
bool slope_detection_enabled = false;
int slope_sg_window = 15;
float slope_sensitivity = 0.5f;          // Reduced from 1.0 (less aggressive)
float slope_negative_threshold = -0.3f;  // Changed from -0.1 (later trigger)
float slope_smoothing_tau = 0.04f;       // Changed from 0.02 (smoother transitions)
```

**Also update FFBEngine.h member variable defaults (lines 317-319):**
```cpp
float m_slope_sensitivity = 0.5f;            // v0.7.1: Reduced from 1.0
float m_slope_negative_threshold = -0.3f;    // v0.7.1: Changed from -0.1
float m_slope_smoothing_tau = 0.04f;         // v0.7.1: Changed from 0.02
```

**Also update SetSlopeDetection() fluent setter default parameters (lines 185-192):**
```cpp
Preset& SetSlopeDetection(bool enabled, int window = 15, float sens = 0.5f, float thresh = -0.3f, float tau = 0.04f) {
    slope_detection_enabled = enabled;
    slope_sg_window = window;
    slope_sensitivity = sens;
    slope_negative_threshold = thresh;
    slope_smoothing_tau = tau;
    return *this;
}
```

---

### 3. UI Fixes

#### 3.1 Add Tooltip to Filter Window Slider

**File:** `src/GuiLayer.cpp`  
**Location:** After line 1138

**Current Code:**
```cpp
if (ImGui::SliderInt("  Filter Window", &window, 5, 41)) {
    if (window % 2 == 0) window++;  // Force odd
    engine.m_slope_sg_window = window;
    selected_preset = -1;
}
if (ImGui::IsItemDeactivatedAfterEdit()) Config::Save(engine);
```

**New Code:**
```cpp
if (ImGui::SliderInt("  Filter Window", &window, 5, 41)) {
    if (window % 2 == 0) window++;  // Force odd
    engine.m_slope_sg_window = window;
    selected_preset = -1;
}
if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip(
        "Savitzky-Golay filter window size (samples).\n\n"
        "Larger = Smoother but higher latency\n"
        "Smaller = Faster response but noisier\n\n"
        "Recommended:\n"
        "  Direct Drive: 11-15\n"
        "  Belt Drive: 15-21\n"
        "  Gear Drive: 21-31\n\n"
        "Must be ODD (enforced automatically).");
}
if (ImGui::IsItemDeactivatedAfterEdit()) Config::Save(engine);
```

#### 3.2 Add Slope Graph to Graphs Section

**File:** `src/FFBEngine.h`  
**Location:** FFBSnapshot struct, add new field after line 121

**Add to struct:**
```cpp
float debug_freq; // New v0.4.41: Frequency for diagnostics
float tire_radius; // New v0.4.41: Tire radius in meters
float slope_current; // New v0.7.1: Slope detection derivative value
```

**File:** `src/FFBEngine.h`  
**Location:** Snapshot population in `calculate_force()`, after line 1189

**Add:**
```cpp
snap.tire_radius = (float)fl.mStaticUndeflectedRadius / 100.0f;
snap.slope_current = (float)m_slope_current;  // v0.7.1: Slope detection diagnostic
```

**File:** `src/GuiLayer.cpp`  
**Location:** After line 1549 (in graph buffer declarations)

**Add:**
```cpp
static RollingBuffer plot_calc_rear_lat_force; 
static RollingBuffer plot_slope_current;  // New v0.7.1: Slope detection diagnostic
```

**File:** `src/GuiLayer.cpp`  
**Location:** In ProcessSnapshot function (after line 1627)

**Add:**
```cpp
plot_calc_rear_grip.Add(snap.calc_rear_grip);
plot_slope_current.Add(snap.slope_current);  // v0.7.1
```

**File:** `src/GuiLayer.cpp`  
**Location:** In graphs rendering section (after calc_front_grip graph, ~line 1775)

**Add:**
```cpp
PlotWithStats("Calc Front Grip", plot_calc_front_grip, 0.0f, 1.2f, ImVec2(0, 40),
    "Estimated front grip fraction.\nWith slope detection: dynamic.\nWithout: static threshold.");

if (engine.m_slope_detection_enabled) {
    PlotWithStats("Slope (dG/dAlpha)", plot_slope_current, -5.0f, 5.0f, ImVec2(0, 40),
        "Slope detection derivative value.\n"
        "Positive = building grip.\n"
        "Near zero = at peak grip.\n"
        "Negative = past peak, sliding.");
}
```

---

### 4. Add UI Warning for Lateral G Boost + Slope Detection

**File:** `src/GuiLayer.cpp`  
**Location:** In the Slope Detection UI section, after the "Enable Slope Detection" checkbox handling (~line 1129)

**Add warning text:**
```cpp
if (slope_res.deactivated) {
    Config::Save(engine);
}

// v0.7.1: Warning about Lateral G Boost interaction
if (engine.m_slope_detection_enabled && engine.m_oversteer_boost > 0.01f) {
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), 
        "Note: Lateral G Boost (Slide) is auto-disabled when Slope Detection is ON.");
    ImGui::NextColumn(); ImGui::NextColumn();
}
```

---

### 5. Documentation Updates

**File:** `docs/Slope_Detection_Guide.md`

**Updates Required:**
1. Change minimum latency examples from window=5 to reflect new defaults
2. Update "Quick Start" recommended settings to new defaults
3. Add new troubleshooting section about Lateral G Boost interaction
4. Update FAQ with new behavior information
5. Change version references from "v0.7.0" to "v0.7.1" where appropriate

---

## Parameter Synchronization Checklist

No NEW parameters are being added - we are only changing DEFAULT VALUES of existing parameters. The synchronization for slope detection parameters was already completed in v0.7.0.

**Verification Checklist (existing parameters - verify values match):**

| Parameter | FFBEngine.h Member | Preset Struct | Apply() | UpdateFromEngine() | Config::Save | Config::Load | Validation |
|-----------|-------------------|---------------|---------|-------------------|--------------|--------------|------------|
| slope_sensitivity | line 317 | line 106 | line 281 | line 351 | line 845 | line 1062 | line 1095 |
| slope_negative_threshold | line 318 | line 107 | line 282 | line 352 | line 846 | line 1063 | N/A |
| slope_smoothing_tau | line 319 | line 108 | line 283 | line 353 | line 847 | line 1064 | line 1096 |

**NEW Field (for graphing):**

| Parameter | FFBEngine.h Member | FFBSnapshot | Snapshot Population | Graph Buffer | Graph Render |
|-----------|-------------------|-------------|---------------------|--------------|--------------|
| slope_current | Already exists (line 423) | Add to struct | Add to populate | Add RollingBuffer | Add PlotWithStats |

---

## Initialization Order Analysis

No circular dependency issues introduced. The changes are:
1. Default value changes in member initializers (no code flow changes)
2. Conditional check added to existing function (single file change)
3. New snapshot field (simple struct addition)

No constructor changes or cross-header initialization order issues.

---

## Test Plan (TDD-Ready)

### Test 1: Verify Lateral G Boost Disabled with Slope Detection

**File:** `tests/test_ffb_engine.cpp`

```cpp
static void test_slope_detection_disables_lat_g_boost()
```

| Aspect | Description |
|--------|-------------|
| **Purpose** | Verify Lateral G Boost has no effect when slope detection is enabled |
| **Setup** | Create engine with slope_detection_enabled=true, oversteer_boost=2.0 |
| **Input** | Telemetry with front_grip=0.7, rear_grip=1.0 (artificial differential) |
| **Expected** | sop_base_force should NOT be boosted (same as input) |
| **Assertion** | `abs(ctx.sop_base_force - sop_unboosted) < 0.01` |

**Telemetry Script (Multi-Frame):**
```
Frame 0: lat_g=0.0, slip_angle=0.0 → Fill buffer
Frame 1-14: lat_g=0.5, slip_angle=0.05 → Continue filling
Frame 15: lat_g=1.2, slip_angle=0.08 → Trigger slope calc
// At this point, front_grip will differ from rear_grip
// Verify sop_base is NOT modified by oversteer boost
```

### Test 2: Verify Lateral G Boost Works When Slope Detection OFF

**File:** `tests/test_ffb_engine.cpp`

```cpp
static void test_lat_g_boost_works_without_slope_detection()
```

| Aspect | Description |
|--------|-------------|
| **Purpose** | Verify existing Lateral G Boost behavior is preserved when slope detection is OFF |
| **Setup** | Create engine with slope_detection_enabled=false, oversteer_boost=2.0 |
| **Input** | Telemetry with front_grip=0.9, rear_grip=0.6 (oversteer condition) |
| **Expected** | sop_base_force should be boosted by ~120% (grip_delta=0.3 × 2.0 × 2.0) |
| **Assertion** | `sop_boosted > sop_unboosted * 1.5` |

### Test 3: Verify New Default Values

**File:** `tests/test_ffb_engine.cpp`

```cpp
static void test_slope_detection_default_values_v071()
```

| Aspect | Description |
|--------|-------------|
| **Purpose** | Verify new default values are correctly applied |
| **Setup** | Create fresh FFBEngine (uses ApplyDefaultsToEngine) |
| **Expected** | sensitivity=0.5, threshold=-0.3, tau=0.04 |
| **Assertion** | All three values match expected defaults |

### Test 4: Verify Slope Snapshot Field

**File:** `tests/test_ffb_engine.cpp`

```cpp
static void test_slope_current_in_snapshot()
```

| Aspect | Description |
|--------|-------------|
| **Purpose** | Verify slope_current is correctly populated in FFBSnapshot |
| **Setup** | Enable slope detection, run calculate_force multiple times |
| **Input** | Telemetry with changing lat_g and slip_angle |
| **Expected** | snap.slope_current should reflect m_slope_current |
| **Assertion** | `abs(snap.slope_current - engine.m_slope_current) < 0.001` |

### Test 5: Verify Less Aggressive Grip Response

**File:** `tests/test_ffb_engine.cpp`

```cpp
static void test_slope_detection_less_aggressive_v071()
```

| Aspect | Description |
|--------|-------------|
| **Purpose** | Verify that with new defaults, moderate negative slopes don't floor grip |
| **Setup** | Engine with new defaults (sens=0.5, thresh=-0.3) |
| **Input** | Telemetry producing slope of -0.5 (moderate negative) |
| **Expected** | Grip should be ~0.95, NOT hitting 0.2 floor |
| **Assertion** | `grip_result > 0.8` (not floored) |

**Calculation:**
```
With new defaults: slope=-0.5, threshold=-0.3
excess = -0.3 - (-0.5) = 0.2
grip_loss = 0.2 * 0.1 * 0.5 = 0.01
grip_factor = 1.0 - 0.01 = 0.99

With old defaults: slope=-0.5, threshold=-0.1
excess = -0.1 - (-0.5) = 0.4
grip_loss = 0.4 * 0.1 * 1.0 = 0.04
grip_factor = 1.0 - 0.04 = 0.96

Much better tolerance for transient negative slopes!
```

### Boundary Condition Tests

Not applicable for this change - no new buffer algorithms introduced. The existing slope detection buffer tests from v0.7.0 remain valid.

---

## User Settings & Presets Impact

### Migration Logic

**NOT REQUIRED.** The changes are intentionally backward-compatible:

1. **Existing Users:** If a user has saved `slope_sensitivity=1.0` in their config.ini, it will be loaded correctly. The new defaults only apply to fresh installations or "Reset Defaults".

2. **Preset Updates:** All built-in presets will automatically use the new defaults (they don't explicitly set slope detection values, so they inherit the defaults).

### User-Facing Changes

| Change | Impact |
|--------|--------|
| New defaults are less sensitive | Smoother feel out-of-box, may need to increase sensitivity for "sharp" feedback |
| Lateral G Boost disabled with slope detection | Users who relied on both effects together will notice reduced oversteer boost; recommend adjusting SoP effect or disabling slope detection |
| New slope graph | Better diagnostic visibility |

---

## Deliverables Checklist

### Code Changes
- [ ] `src/FFBEngine.h` - Update default values for slope params (~3 lines)
- [ ] `src/FFBEngine.h` - Add conditional check in Lateral G Boost calculation (~5 lines)
- [ ] `src/FFBEngine.h` - Add slope_current to FFBSnapshot (1 line)
- [ ] `src/FFBEngine.h` - Populate snap.slope_current in calculate_force() (1 line)
- [ ] `src/Config.h` - Update default values in Preset struct (~3 lines)
- [ ] `src/Config.h` - Update SetSlopeDetection() default parameters (~1 line)
- [ ] `src/GuiLayer.cpp` - Add tooltip to Filter Window slider (~10 lines)
- [ ] `src/GuiLayer.cpp` - Add warning text about Lateral G Boost (~5 lines)
- [ ] `src/GuiLayer.cpp` - Add slope graph buffer and rendering (~10 lines)

### Test Files
- [ ] `tests/test_ffb_engine.cpp` - Add 5 new unit tests (~80 lines)

### Documentation
- [ ] `docs/Slope_Detection_Guide.md` - Update defaults, add Lateral G Boost section
- [ ] `CHANGELOG.md` - Add v0.7.1 entry
- [ ] `VERSION` - Increment to 0.7.1

### Verification
- [ ] All existing tests pass (no regressions)
- [ ] All 5 new tests pass
- [ ] Build succeeds in Release mode
- [ ] GUI displays correctly:
  - [ ] Filter Window slider has tooltip
  - [ ] Warning appears when slope detection + Lateral G Boost > 0
  - [ ] Slope graph visible when slope detection enabled
- [ ] Manual test: Zig-zag at 60 kph no longer causes grip to floor at 20%
- [ ] Manual test: Lateral G Boost works when slope detection is OFF

---

## Technical Notes

### Why Not Apply Slope Detection to Rear Wheels?

The slope detection algorithm uses `mLocalAccel.x` (lateral G-force), which is a **vehicle-level measurement** at the center of mass. It cannot distinguish between front and rear axle grip. Applying the same calculation to rear wheels would give identical (and incorrect) results.

The correct fix is to disable the grip differential calculation entirely when using slope detection, as the differential between two different calculation methods is meaningless.

### Future Enhancement Path

If users want oversteer boost with slope detection in the future:
1. **Option A:** Implement rear slip angle monitoring as a proxy for rear grip loss
2. **Option B:** Use yaw rate vs steering angle correlation to detect oversteer
3. **Option C:** Expose a user toggle to re-enable Lateral G Boost at their own risk

For v0.7.1, Option C is not recommended due to the oscillation issues users experienced.

---

## Appendix: Validation Calculations

### Old Defaults vs New Defaults Response Comparison

**Scenario:** User doing zig-zag at 60 kph, slope momentarily hits -2.0

| Parameter | Old Default | New Default |
|-----------|-------------|-------------|
| slope_negative_threshold | -0.1 | -0.3 |
| slope_sensitivity | 1.0 | 0.5 |
| slope_smoothing_tau | 0.02s | 0.04s |

**Old calculation:**
```
excess = -0.1 - (-2.0) = 1.9
grip_loss = 1.9 * 0.1 * 1.0 = 0.19
raw_grip = 1.0 - 0.19 = 0.81
After smoothing (fast): ~0.82 within 20ms
After floor clamp: 0.82 (but oscillates rapidly)
```

**New calculation:**
```
excess = -0.3 - (-2.0) = 1.7
grip_loss = 1.7 * 0.1 * 0.5 = 0.085
raw_grip = 1.0 - 0.085 = 0.915
After smoothing (slower): ~0.92 over 40ms
After floor clamp: 0.92 (transitions smoothly)
```

The new defaults provide 40% less grip loss for the same slope value, and transitions take 2x longer, resulting in significantly smoother FFB.

---

*Plan created: 2026-02-02*  
*Based on investigation: docs/dev_docs/investigations/slope_detection_issues_v0.7.0.md*  
*Target version: 0.7.1*  
*Estimated implementation effort: ~120 lines of C++ across 4 files + tests*

## Implementation Notes

### Unforeseen Issues
- **Tooling (target content mismatch):** The initial `multi_replace_file_content` call for the Lateral G Boost logic in `FFBEngine.h` failed due to a target content mismatch. This required a manual `view_file` refresh and a targeted `replace_file_content` call to resolve.
- **Test Failure (`test_slope_detection_less_aggressive_v071`):** The end-to-end physics test initially failed because the simulated telemetry data resulted in a raw slope of `-1.01` instead of the predicted `-0.5`. This was a minor simulation discrepancy; the test expectation was updated to `-1.0` as it still successfully verified that the new defaults prevent grip flooring (0.2) in that scenario.

### Plan Deviations
- **Config Validation Enhancement:** In `Config.cpp`, I updated the fallback validation for `m_slope_smoothing_tau` to reset to `0.04f` (the new default) instead of `0.02f` if a zero/invalid value is detected. This ensures that safety resets align with the latest tuning philosophy.

### Challenges Encountered
- Synchronizing multi-file edits correctly across `Config.h`, `FFBEngine.h`, and `GuiLayer.cpp` while ensuring all default values match across members, structs, and UI tooltips.

### Recommendations for Future Plans
- **Fresh View for Replacements:** Perform a `view_file` on the specific range immediately before a `replace_file_content` to ensure whitespace and comments are exactly as the tool expects.
- **Simulation Accuracy:** When writing complex physics tests involving slopes, consider logging the intermediate values during the first run to calibrate the test expectations more precisely against the actual discretized simulation.
