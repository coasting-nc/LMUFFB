# Investigation Report: Slope Detection Issues (v0.7.0)

## Context

**Feature:** Slope Detection Algorithm for Dynamic Grip Estimation  
**Version:** 0.7.0  
**Date:** 2026-02-02  
**Status:** Investigation Complete  
**Implementation Plan:** `docs/dev_docs/implementation_plans/plan_slope_detection.md`  
**User Guide:** `docs/Slope_Detection_Guide.md`

---

## User Feedback Summary

Users have reported the following issues with the Slope Detection feature:

1. **Heavy and notchy FFB** - Users experience very heavy and "notchy" steering feel when Slope Detection is enabled
2. **Strong low-frequency rumble in corners** - Feels like a strong vibration effect in corners
3. **Oscillations on straights and in corners** - FFB becomes unstable even when driving straight
4. **Correlation with Understeer Effect** - Problems diminish or disappear when Understeer Effect is set to 0%
5. **Correlation with Lateral G Boost (Slide)** - Disabling this effect makes Slope Detection work properly
6. **Incorrect grip indicator fluctuation** - Grip indicator drops to 20% even at 60 kph when doing zig-zag lines, with strong FFB hits

---

## Investigation Findings

### Issue 1: Grip Fluctuation at Low Speeds (60 kph / Zig-Zag Lines)

#### Observed Behavior
User reports that at 60 kph, the grip indicator under "Advanced Slope Settings" fluctuates between 20% and 100% when driving zig-zag lines, causing strong FFB hits.

#### Root Cause Analysis

1. **Algorithm Behavior Is Correct (But Sensitive):** The slope detection algorithm measures `dG/dAlpha` (change in lateral G-force per change in slip angle). When doing zig-zag maneuvers:
   - Rapid steering changes create rapid changes in slip angle (`dAlpha/dt` is high)
   - At low speeds (60 kph), lateral G-force changes more quickly relative to slip angle than at higher speeds
   - Quick direction changes can cause momentary negative slopes as G-force transitions directions

2. **The 20% Floor is Being Hit Correctly:** The algorithm has a safety floor of 0.2 (20%) to prevent complete FFB loss. The fact that it hits 20% suggests the slope is going very negative during these transitions.

3. **The Real Problem: Sensitivity Too High for Transient Maneuvers**
   - Default `slope_sensitivity = 1.0` may be too aggressive for quick direction changes
   - Default `slope_negative_threshold = -0.1` may trigger too early during transient steering

#### Code Reference
```cpp
// FFBEngine.h lines 846-850
if (m_slope_current < (double)m_slope_negative_threshold) {
    double excess = (double)m_slope_negative_threshold - m_slope_current;
    current_grip_factor = 1.0 - (excess * 0.1 * (double)m_slope_sensitivity);
}
```

**Finding:** The `0.1` multiplier on line 849 means with `slope_sensitivity = 1.0` and `threshold = -0.1`:
- A slope of -0.5 gives: `excess = 0.4`, `grip_loss = 0.4 * 0.1 * 1.0 = 0.04`, `grip = 0.96`
- A slope of -8.0 gives: `excess = 7.9`, `grip_loss = 7.9 * 0.1 * 1.0 = 0.79`, `grip = 0.21` (hits floor)

**The 0.1 multiplier is arbitrarily chosen and may not be appropriate for real-world telemetry values.**

---

### Issue 2: Interaction with Lateral G Boost (Slide)

#### Observed Behavior
User found that disabling "Lateral G Boost Slide" allowed Slope Detection and Understeer Effect to work properly without oscillations.

#### Root Cause Analysis

The Lateral G Boost uses a **grip differential** between front and rear:

```cpp
// FFBEngine.h lines 1307-1311
double grip_delta = ctx.avg_grip - ctx.avg_rear_grip;
if (grip_delta > 0.0) {
    sop_base *= (1.0 + (grip_delta * m_oversteer_boost * 2.0));
}
```

**The Problem: Asymmetric Grip Calculation Methods**

1. **Front Grip (with Slope Detection ON):**
   - Uses dynamic slope detection algorithm
   - Responds quickly to changes in lateral G vs slip angle derivative
   - Can fluctuate rapidly during transient steering

2. **Rear Grip (always static threshold):**
   - Uses fixed static threshold method (`is_front = false` on line 1300)
   - More stable, slower to respond
   - Does NOT use slope detection

```cpp
// FFBEngine.h lines 1297-1300
GripResult rear_grip_res = calculate_grip(..., data, false);  // is_front = false
```

**Consequence:**
When front grip fluctuates rapidly due to slope detection but rear grip remains stable:
- `grip_delta = ctx.avg_grip - ctx.avg_rear_grip` creates artificial oscillations
- The Lateral G Boost amplifies these oscillations: `sop_base *= (1.0 + (grip_delta * boost * 2.0))`
- With `m_oversteer_boost = 2.0` (200%), a grip delta of 0.3 gives: `1 + (0.3 * 2.0 * 2.0) = 2.2x` boost

**This is a fundamental design flaw:** Using two different grip estimation methods (dynamic for front, static for rear) creates artificial grip differentials that have no physical meaning.

---

### Issue 3: Heavy/Notchy FFB and Low-Frequency Rumble

#### Root Cause Analysis

The "notchy" and "heavy" feel is caused by the combination of:

1. **Rapid Grip Fluctuation:**
   - Slope detection can change grip very quickly (faster than smoothing can filter)
   - The output smoothing (`m_slope_smoothing_tau = 0.02s`) may not be sufficient

2. **Understeer Effect Amplification:**
   ```cpp
   // FFBEngine.h lines 1070-1074
   double grip_loss = (1.0 - ctx.avg_grip) * m_understeer_effect;
   ctx.grip_factor = (std::max)(0.0, 1.0 - grip_loss);
   double output_force = (base_input * (double)m_steering_shaft_gain) * ctx.grip_factor;
   ```
   With `m_understeer_effect = 100%` and rapid grip changes, the output force fluctuates dramatically.

3. **The "Low-Frequency Rumble" Is Actually FFB Oscillation:**
   - When grip jumps between high and low values rapidly
   - The steering shaft force (weighted by grip_factor) oscillates
   - This creates a perceived "rumble" that is actually unstable feedback

---

### Issue 4: UI/UX Problems

#### 4.1 Filter Window Slider Layout Issue
**Report:** "The UI display seems incorrect for 'Filter Window' slider (text on the right, slider on the left)."

**Code Location:** `GuiLayer.cpp` lines 1132-1145

```cpp
int window = engine.m_slope_sg_window;
if (ImGui::SliderInt("  Filter Window", &window, 5, 41)) {
    ...
}
ImGui::SameLine();
float latency_ms = (float)(engine.m_slope_sg_window / 2) * 2.5f;
ImGui::TextColored(color, "~%.0f ms latency", latency_ms);
ImGui::NextColumn(); ImGui::NextColumn();
```

**Finding:** The slider uses `ImGui::SliderInt` which doesn't follow the `GuiWidgets::Float` pattern that handles column alignment. The `ImGui::SameLine()` places the latency text next to the slider but the `NextColumn()` calls at the end are causing layout issues.

#### 4.2 Missing Tooltip on Filter Window Slider
**Report:** "Missing tooltip on 'Filter Window' slider"

**Finding Confirmed:** The `ImGui::SliderInt` call does not have an associated tooltip. Other sliders use `GuiWidgets::Float` or `FloatSetting` which include tooltip functionality.

#### 4.3 Missing Tooltip on Enable Checkbox
**Report:** "Missing tooltip on checkbox to activate Slope Detection"

**Finding:** The checkbox DOES have a tooltip (lines 1107-1114), so this is **NOT a valid issue**. The tooltip explains the feature comprehensively.

#### 4.4 Latency Display Location
**Report:** "Add display of latency amount depending on the filter window size"

**Finding:** This IS implemented (line 1142-1144), but could be better integrated into a proper tooltip.

---

### Issue 5: Graph for Slope Detection Grip

**Report:** "Verify that the graphs include a graph that shows the grip estimated with the slope detection"

**Finding:** The existing `calc_front_grip` graph DOES show the grip factor when slope detection is enabled, because:

```cpp
// FFBEngine.h line 1158
snap.calc_front_grip = (float)ctx.avg_grip;
```

And `ctx.avg_grip` comes from the slope detection calculation when enabled. However, there is **no dedicated graph for the slope value itself** (`m_slope_current`), which would be crucial for debugging.

---

### Issue 6: Live Slope Display Should Be a Graph

**Report:** "Convert the 'Live Slope' next to the slider into a proper graph in the graphs section"

**Finding:** Currently displayed as inline text at line 1167:
```cpp
ImGui::Text("  Live Slope: %.3f | Grip: %.0f%%", 
    engine.m_slope_current, 
    engine.m_slope_smoothed_output * 100.0f);
```

This should indeed be converted to a rolling graph for better visualization of the derivative behavior over time.

---

## Proposed Solutions

### Priority 1: Critical Algorithm Fixes

#### 1.1 Reduce Default Sensitivity
**Problem:** `slope_sensitivity = 1.0` with `0.1` multiplier is too aggressive.

**Proposed Change:** 
- Reduce default `slope_sensitivity` from `1.0` to `0.3`
- OR adjust the multiplier from `0.1` to `0.03` in the algorithm

**Location:** `Config.h` preset defaults and `FFBEngine.h` line 849

#### 1.2 Increase Default Slope Threshold (Make More Negative)
**Problem:** `slope_negative_threshold = -0.1` triggers too early on transient steering.

**Proposed Change:**
- Change default from `-0.1` to `-0.5`
- This means grip loss only triggers when the slope is significantly negative

**Location:** `Config.h` preset defaults and `FFBEngine.h` line 321

#### 1.3 Increase Default Output Smoothing
**Problem:** `slope_smoothing_tau = 0.02s` (20ms) is too fast for smooth FFB.

**Proposed Change:**
- Change default from `0.02` to `0.05` (50ms)
- This creates a gentler transition between grip states

**Location:** `Config.h` preset defaults

#### 1.4 Disable Lateral G Boost Interaction (Design Choice)
**Problem:** Using different grip calculation methods for front/rear creates artificial differentials.

**Proposed Options:**
1. **Option A (Recommended):** When slope detection is enabled, set `grip_delta = 0` in the Lateral G Boost calculation to disable the interaction entirely
2. **Option B:** Apply slope detection to rear wheels as well (requires additional work to handle the fact that lateral G is vehicle-level, not per-axle)
3. **Option C:** Document this clearly and recommend users set Lateral G Boost to 0% when using Slope Detection

**Location:** `FFBEngine.h` lines 1307-1311

### Priority 2: UI/UX Fixes

#### 2.1 Fix Filter Window Slider Layout
**Problem:** The slider doesn't follow the two-column layout pattern.

**Proposed Fix:** Replace `ImGui::SliderInt` with a proper `IntSetting` helper or modify the column handling.

**Location:** `GuiLayer.cpp` lines 1132-1145

#### 2.2 Add Tooltip to Filter Window Slider
**Proposed Fix:** Add tooltip explaining:
- That larger windows = smoother but higher latency
- That smaller windows = noisier but lower latency
- Recommended values for different wheel types

**Location:** `GuiLayer.cpp` after line 1138

#### 2.3 Add Slope Graph to Graphs Section
**Proposed Fix:**
1. Add `static RollingBuffer plot_slope_current;` to graph buffers
2. Update `ProcessSnapshot` to add `engine.m_slope_current` to buffer
3. Add `PlotWithStats("Slope (dG/dAlpha)", plot_slope_current, ...)` to graphs section

**Location:** `GuiLayer.cpp` in the graphs section (~line 1540+)

---

## Recommended Default Settings Changes

### Current Defaults (v0.7.0)
| Setting | Current Default | Issue |
|---------|-----------------|-------|
| `slope_detection_enabled` | `false` | OK |
| `slope_sg_window` | `15` | OK |
| `slope_sensitivity` | `1.0` | Too aggressive |
| `slope_negative_threshold` | `-0.1` | Triggers too early |
| `slope_smoothing_tau` | `0.02` | Too fast |

### Proposed Defaults (v0.7.1)
| Setting | Proposed Default | Rationale |
|---------|------------------|-----------|
| `slope_detection_enabled` | `false` | Keep disabled by default |
| `slope_sg_window` | `15` | Unchanged - good balance |
| `slope_sensitivity` | `0.5` | Less aggressive, more realistic |
| `slope_negative_threshold` | `-0.3` | Later trigger, fewer false positives |
| `slope_smoothing_tau` | `0.04` | Smoother transitions |

---

## Implementation Checklist

### Algorithm Fixes
- [ ] Adjust default `slope_sensitivity` to `0.5`
- [ ] Adjust default `slope_negative_threshold` to `-0.3`
- [ ] Adjust default `slope_smoothing_tau` to `0.04`
- [ ] Add safeguard when Lateral G Boost and Slope Detection are both enabled (consider warning or automatic adjustment)

### UI Fixes
- [ ] Fix Filter Window slider layout to follow two-column pattern
- [ ] Add tooltip to Filter Window slider
- [ ] Add Slope graph to graphs section
- [ ] Remove inline "Live Slope" text once graph is implemented (or keep both)

### Documentation Updates
- [ ] Update `docs/Slope_Detection_Guide.md` with new default values
- [ ] Add section about Lateral G Boost interaction
- [ ] Update troubleshooting section with these findings

### Testing
- [ ] Create test case for 60 kph zig-zag scenario
- [ ] Create test case for Lateral G Boost interaction
- [ ] Verify new defaults don't break existing use cases

---

## Appendix A: User Feedback Quotes (Verbatim)

1. > "I've just tried the new version with the Slope Detection. Suddenly my wheel was very heavy and notchy. I turned down the gain but it was still bad. (I was still using Aaron's presets though). I turned the slope detection off and things were back to how I raced with it."

2. > "I also just tried dynamic Slope Detection. Same for me, very heavy and notchy. In corners it fells like a strong low frequency rumble. When lowering understeer effect strength it gets less pronounced, and the notchy rumble is gone at 0% understeer effect."

3. > "At first I was also unable to find success with Slope Detection. I was getting huge oscillations both in corners and on the straights. I tried max filter, min filter, max smoothing, min sensitivity, lowest threshold and many combinations of it all. Nothing helped. But then I tried lowering and shutting off Lateral G Boost Slide and viola! This is lovely!"

4. > "I played with those settings, didn't help much. For a test I put in the pit speed limiter (60 kph) and watched the 'grip' indicator under 'advanced slop settings', it fluctuated between 20% and 100% when driving zig zag lines and I get strong hits in the ffb. Is this correct that the grip goes down to 20% at 60 kph?"

---

## Appendix B: Code References

### Key Files
- `src/FFBEngine.h` - Core slope detection algorithm (lines 789-861)
- `src/FFBEngine.h` - Grip calculation with slope detection toggle (lines 659-665)
- `src/FFBEngine.h` - Lateral G Boost calculation (lines 1307-1311)
- `src/FFBEngine.h` - Understeer effect application (lines 1070-1074)
- `src/GuiLayer.cpp` - Slope detection UI section (lines 1100-1171)
- `src/Config.h` - Preset defaults and synchronization

### Algorithm Flow
```
1. Telemetry Input (400Hz)
   ↓
2. calculate_grip() called with is_front=true
   ↓
3. If slope_detection_enabled && is_front:
   ↓
4. calculate_slope_grip() called
   ├── Update circular buffers with lat_g and slip_angle
   ├── Calculate SG derivatives (dG/dt, dAlpha/dt)
   ├── Compute slope = dG/dt / dAlpha/dt
   ├── If slope < threshold: compute grip_loss
   └── Apply smoothing and return grip factor
   ↓
5. ctx.avg_grip = result.value (affects understeer effect)
   ↓
6. calculate_sop_lateral() called
   ├── Calculate rear grip with is_front=false (static method)
   ├── Compute grip_delta = front_grip - rear_grip
   └── Apply Lateral G Boost if delta > 0
   ↓
7. Output FFB with understeer modulation
```

---

*Investigation completed: 2026-02-02*  
*Next steps: Create implementation plan for v0.7.1 fixes*
