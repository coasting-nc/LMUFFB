# Implementation Plan: Slope Detection Algorithm for Dynamic Grip Estimation

## Context

This plan describes the implementation of the **Slope Detection Algorithm**, a physics-based approach to dynamically estimate tire grip levels in real-time. The algorithm replaces the current static, user-configured optimal slip angle/ratio thresholds with an adaptive signal processing model that monitors the derivative (slope) of the Self-Aligning Torque (SAT) vs. Slip Angle relationship.

### Problem Statement

The current lmuFFB implementation uses **static optimal slip values** (`m_optimal_slip_angle`, `m_optimal_slip_ratio`) that require manual tuning per car and do not adapt to:
- Tire temperature changes
- Tire wear
- Rain/wet conditions
- Aerodynamic load changes
- Different tire compounds

### Proposed Solution

The Slope Detection Algorithm monitors the **rate of change (derivative)** of lateral force/G-force vs. slip angle. Instead of asking "has the driver exceeded 5.7 degrees?", it asks "is more steering input producing more grip, or less?"

**Core Principle:**
- **Positive Slope (>0)**: Grip is building. No intervention needed.
- **Zero Slope (≈0)**: At peak grip. This is the optimal slip angle - detected automatically.
- **Negative Slope (<0)**: Past peak, tire is sliding. FFB should lighten to signal understeer.

---

## Reference Documents

1. **Research Reports:**
   - `docs/dev_docs/FFB Slope Detection for Grip Estimation.md` - Comprehensive analysis of slope detection theory
   - `docs/dev_docs/FFB Slope Detection for Grip Estimation2.md` - Signal processing and Savitzky-Golay filter analysis

2. **Preliminary Implementation Plans:**
   - `docs/dev_docs/slope_detection_implementation_plan.md` - Initial technical approach
   - `docs/dev_docs/slope_detection_implementation_plan2.md` - Detailed implementation specifications

3. **Related TODO Item:**
   - `docs/dev_docs/TODO.md` - Section "Optimal slip angle in real time"

---

## Codebase Analysis Summary

### Current Architecture

The FFB calculation pipeline in `FFBEngine.h` follows this flow:

1. **Telemetry Input** → Raw data from game (TelemInfoV01)
2. **Grip Calculation** → `calculate_grip()` function (lines 593-678)
3. **Effect Calculations** → Multiple effect functions use grip data
4. **Output Summation** → Forces combined and normalized
5. **FFB Output** → Sent to wheel hardware

### Key Existing Components

| Component | Location | Purpose |
|-----------|----------|---------|
| `calculate_grip()` | FFBEngine.h:593-678 | Main grip calculation function |
| `GripResult` struct | FFBEngine.h:167-172 | Return type with grip value + diagnostics |
| `FFBCalculationContext` | FFBEngine.h:178-215 | Shared context for all effect calculations |
| `m_optimal_slip_angle` | FFBEngine.h:282 | Static threshold setting (current) |
| `m_optimal_slip_ratio` | FFBEngine.h:283 | Static threshold setting (current) |
| `Preset` struct | Config.h:8-320 | All user settings including grip thresholds |

### Current Grip Calculation Flow (`calculate_grip()`)

```
1. Read raw mGripFract from telemetry (game-provided)
2. If mGripFract is zero AND car has load → Fallback approximation:
   a. Calculate slip angle (via calculate_slip_angle())
   b. Calculate slip ratio (via calculate_manual_slip_ratio())
   c. Compare against m_optimal_slip_angle and m_optimal_slip_ratio thresholds
   d. Compute combined friction circle metric
   e. Apply sigmoid decay for grip loss
3. Return GripResult with .value and diagnostics
```

**Critical Insight:** The slope detection algorithm will replace **step 2c/2d/2e** (the static threshold comparison and fixed decay curve) with a dynamic derivative-based approach.

---

## FFB Effects Impact Analysis

The following table shows **all FFB effects that consume grip data** and how they will be impacted by the Slope Detection Algorithm.

### Effects Using Front Grip (`ctx.avg_grip`)

| Effect | Location | How Grip is Used | Technical Impact | User Experience Impact |
|--------|----------|------------------|------------------|------------------------|
| **Understeer Effect** | FFBEngine.h:966-969 | `grip_loss = (1.0 - ctx.avg_grip) * m_understeer_effect` | Core consumer - directly affected | FFB lightens as front tires approach/exceed peak grip. With slope detection, this will be **smoother and more progressive** as it tracks the tire curve rather than a hard threshold. |
| **Slide Texture** | FFBEngine.h:1389-1391 | `grip_scale = max(0.0, 1.0 - ctx.avg_grip)` | Scales vibration intensity by grip loss | Texture intensity will scale more naturally with sliding severity. Less "on/off" feeling. |

### Effects Using Rear Grip (`ctx.avg_rear_grip`)

| Effect | Location | How Grip is Used | Technical Impact | User Experience Impact |
|--------|----------|------------------|------------------|------------------------|
| **Lateral G Boost (Oversteer)** | FFBEngine.h:1203-1206 | `grip_delta = ctx.avg_grip - ctx.avg_rear_grip` | Uses differential between front and rear grip | Oversteer boost will trigger more precisely when rear actually breaks traction vs. front. **Better oversteer detection in mixed conditions.** |

### Effects Using Slip Angle (Derived from Grip Calculation)

| Effect | Location | How Slip Angle is Used | Technical Impact | User Experience Impact |
|--------|----------|------------------------|------------------|------------------------|
| **Rear Aligning Torque** | FFBEngine.h:1214-1218 | `rear_slip_angle = m_grip_diag.rear_slip_angle` → `calc_rear_lat_force` | Uses slip angle from grip calculation | No direct change - slip angle calculation is preserved. Counter-steering cues remain accurate. |

### Effects NOT Using Grip Data (No Impact)

| Effect | Notes |
|--------|-------|
| Steering Shaft Gain | Uses raw game force (mSteeringShaftTorque) |
| Yaw Kick | Uses yaw acceleration (mLocalRotAccel.y) |
| Gyro Damping | Uses steering velocity only |
| Lockup Vibration | Uses wheel slip ratio, not grip factor |
| Wheel Spin | Uses wheel rotation deltas |
| Road Texture | Uses vertical tire deflection |
| ABS Pulse | Uses brake pressure modulation |
| Bottoming | Uses suspension force/deflection |
| Scrub Drag | Uses lateral patch velocity |

---

## User Experience Summary

### What Will Change (When Slope Detection Is Enabled)

| Aspect | Current (Static Threshold) | With Slope Detection |
|--------|---------------------------|---------------------|
| **Understeer Onset** | Force drops at fixed slip angle (e.g., 0.10 rad) | Force drops when tire curve inflects - adapts per car/condition |
| **Understeer Progression** | Fixed sigmoid decay curve | Proportional to how negative the slope is |
| **Oversteer Detection** | Based on fixed front/rear grip difference | Based on gradient comparison - more sensitive to rear breakaway |
| **Temperature/Wear Adaptation** | None - same threshold cold or worn | Automatic - tracks tire's changing peak |
| **Slide Texture Feel** | Binary-ish: intense above threshold | Graduated - scales with slide severity |

### What Will NOT Change

- Base steering feel (steering shaft torque)
- Counter-steering cues (rear aligning torque direction)
- Yaw kick timing and intensity
- Gyroscopic damping feel
- Lockup/spin vibration character
- Road texture detail
- ABS pulse behavior
- All other signal processing (notch filters, smoothing)

---

## Proposed Changes

### 1. New Configuration Parameters

**File: `src/Config.h`** - Add to `Preset` struct (~line 100, after `understeer_affects_sop`):

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `slope_detection_enabled` | `bool` | `false` | Enable dynamic slope detection |
| `slope_sg_window` | `int` | `15` | Savitzky-Golay window size (samples) |
| `slope_sensitivity` | `float` | `1.0f` | Sensitivity multiplier for slope-to-grip conversion |
| `slope_negative_threshold` | `float` | `-0.1f` | Slope below which grip loss is detected |
| `slope_smoothing_tau` | `float` | `0.02f` | Output smoothing time constant (seconds) |

**Preset Fluent Setter:**
```cpp
Preset& SetSlopeDetection(bool enabled, int window = 15, float sens = 1.0f) {
    slope_detection_enabled = enabled;
    slope_sg_window = window;
    slope_sensitivity = sens;
    return *this;
}
```

### 2. FFBEngine Core Changes

**File: `src/FFBEngine.h`**

#### 2.1 New Member Variables (public section, ~line 312, after `m_understeer_affects_sop`):

```cpp
// ===== SLOPE DETECTION (v0.7.0) =====
bool m_slope_detection_enabled = false;
int m_slope_sg_window = 15;
float m_slope_sensitivity = 1.0f;
float m_slope_negative_threshold = -0.1f;
float m_slope_smoothing_tau = 0.02f;
```

#### 2.2 Internal State Buffers (private section, ~line 410, after `m_static_notch_filter`):

```cpp
// Slope Detection Buffers (Circular) - v0.7.0
static constexpr int SLOPE_BUFFER_MAX = 41;  // Max window size supported
std::array<double, SLOPE_BUFFER_MAX> m_slope_lat_g_buffer = {};
std::array<double, SLOPE_BUFFER_MAX> m_slope_slip_buffer = {};
int m_slope_buffer_index = 0;
int m_slope_buffer_count = 0;

// Slope Detection State (Public for diagnostics) - v0.7.0
double m_slope_current = 0.0;
double m_slope_grip_factor = 1.0;
double m_slope_smoothed_output = 1.0;
```

#### 2.3 New Helper Functions (private section, ~line 1170):

```cpp
// Helper: Calculate Savitzky-Golay First Derivative - v0.7.0
// Uses closed-form coefficient generation for quadratic polynomial fit.
// Reference: docs/dev_docs/plans/savitzky-golay coefficients deep research report.md
//
// Mathematical basis (Quadratic First Derivative, N=2):
//   For a symmetric window with half-width M (total window = 2M+1 points),
//   the derivative weight for sample at position k is:
//     w_k = k / (S_2 * h)
//   where:
//     S_2 = M(M+1)(2M+1)/3  (sum of k^2 from -M to M)
//     h = sampling interval (dt)
//
// Key properties:
//   - Weights are anti-symmetric: w_{-k} = -w_k
//   - Center weight (k=0) is always 0
//   - Derivative has O(n) complexity per sample
//
double calculate_sg_derivative(const std::array<double, SLOPE_BUFFER_MAX>& buffer, 
                               int count, int window, double dt) {
    // Ensure we have enough samples
    if (count < window) return 0.0;
    
    int M = window / 2;  // Half-width (e.g., window=15 -> M=7)
    
    // Calculate S_2 = M(M+1)(2M+1)/3
    double S2 = (double)M * (M + 1.0) * (2.0 * M + 1.0) / 3.0;
    
    // Convolution: sum of w_k * y_k
    // Optimization: exploit anti-symmetry w_{-k} = -w_k
    //   sum = sum_{k=1}^{M} (k/S2) * (y_k - y_{-k}) / h
    double sum = 0.0;
    for (int k = 1; k <= M; ++k) {
        // Calculate buffer indices (circular buffer handling)
        int idx_pos = (m_slope_buffer_index - M + k + SLOPE_BUFFER_MAX) % SLOPE_BUFFER_MAX;
        int idx_neg = (m_slope_buffer_index - M - k + SLOPE_BUFFER_MAX) % SLOPE_BUFFER_MAX;
        
        double w_k = (double)k / S2;
        sum += w_k * (buffer[idx_pos] - buffer[idx_neg]);
    }
    
    // Divide by dt to get derivative in units/second
    return sum / dt;
}

// Helper: Calculate Grip Factor from Slope - v0.7.0
// Main slope detection algorithm entry point
double calculate_slope_grip(double lateral_g, double slip_angle, double dt);

#### 2.4 Modify `calculate_grip()` Function (~lines 630-665):

**Current Code (lines 638-664):**
```cpp
} else {
    // v0.4.38: Combined Friction Circle (Advanced Reconstruction)
    
    // 1. Lateral Component (Alpha)
    double lat_metric = std::abs(result.slip_angle) / (double)m_optimal_slip_angle;
    // ... rest of static threshold logic
}
```

**New Code (replacement):**
```cpp
} else {
    // v0.7.0: Slope Detection OR Static Threshold
    if (m_slope_detection_enabled) {
        // Dynamic grip estimation via derivative monitoring
        result.value = calculate_slope_grip(
            data->mLocalAccel.x / 9.81,  // Lateral G
            result.slip_angle,            // Slip angle (radians)
            ctx.dt
        );
    } else {
        // v0.4.38: Combined Friction Circle (Static Threshold - Legacy)
        double lat_metric = std::abs(result.slip_angle) / (double)m_optimal_slip_angle;
        // ... existing static threshold logic unchanged
    }
}
```

### 3. Config Persistence

**File: `src/Config.cpp`**

#### 3.1 Save Logic (~line 840, after `understeer_affects_sop`):
```cpp
// Slope Detection (v0.7.0)
file << "slope_detection_enabled=" << (engine.m_slope_detection_enabled ? "1" : "0") << "\n";
file << "slope_sg_window=" << engine.m_slope_sg_window << "\n";
file << "slope_sensitivity=" << engine.m_slope_sensitivity << "\n";
file << "slope_negative_threshold=" << engine.m_slope_negative_threshold << "\n";
file << "slope_smoothing_tau=" << engine.m_slope_smoothing_tau << "\n";
```

#### 3.2 Load Logic (~line 1055, after `understeer_affects_sop`):
```cpp
// Slope Detection (v0.7.0)
else if (key == "slope_detection_enabled") engine.m_slope_detection_enabled = (value == "1");
else if (key == "slope_sg_window") engine.m_slope_sg_window = std::stoi(value);
else if (key == "slope_sensitivity") engine.m_slope_sensitivity = std::stof(value);
else if (key == "slope_negative_threshold") engine.m_slope_negative_threshold = std::stof(value);
else if (key == "slope_smoothing_tau") engine.m_slope_smoothing_tau = std::stof(value);
```

#### 3.3 Validation Logic (~line 1075, new section):
```cpp
// Slope Detection Validation (v0.7.0)
if (engine.m_slope_sg_window < 5) engine.m_slope_sg_window = 5;
if (engine.m_slope_sg_window > 41) engine.m_slope_sg_window = 41;
if (engine.m_slope_sg_window % 2 == 0) engine.m_slope_sg_window++; // Must be odd
if (engine.m_slope_sensitivity < 0.1f) engine.m_slope_sensitivity = 0.1f;
if (engine.m_slope_sensitivity > 10.0f) engine.m_slope_sensitivity = 10.0f;
if (engine.m_slope_smoothing_tau < 0.001f) engine.m_slope_smoothing_tau = 0.02f;
```

#### 3.4 Preset Apply/UpdateFromEngine (Config.h):

**Add to `Apply()` method (~line 252):**
```cpp
engine.m_slope_detection_enabled = slope_detection_enabled;
engine.m_slope_sg_window = slope_sg_window;
engine.m_slope_sensitivity = slope_sensitivity;
engine.m_slope_negative_threshold = slope_negative_threshold;
engine.m_slope_smoothing_tau = slope_smoothing_tau;
```

**Add to `UpdateFromEngine()` method (~line 313):**
```cpp
slope_detection_enabled = engine.m_slope_detection_enabled;
slope_sg_window = engine.m_slope_sg_window;
slope_sensitivity = engine.m_slope_sensitivity;
slope_negative_threshold = engine.m_slope_negative_threshold;
slope_smoothing_tau = engine.m_slope_smoothing_tau;
```

### 4. GUI Integration

**File: `src/GuiLayer.cpp`** (~line 1100, after "Grip & Slip Angle Estimation" section):

```cpp
// --- SLOPE DETECTION (EXPERIMENTAL) ---
if (ImGui::TreeNodeEx("Slope Detection (Experimental)", ImGuiTreeNodeFlags_Framed)) {
    ImGui::NextColumn(); ImGui::NextColumn();
    
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "⚠ Dynamic grip estimation");
    ImGui::NextColumn(); ImGui::NextColumn();
    
    BoolSetting("Enable Slope Detection", &engine.m_slope_detection_enabled,
        "Replaces static 'Optimal Slip Angle' threshold with dynamic derivative monitoring.\n\n"
        "When enabled:\n"
        "• Grip is estimated by tracking the slope of lateral-G vs slip angle\n"
        "• Automatically adapts to tire temperature, wear, and conditions\n"
        "• 'Optimal Slip Angle' and 'Optimal Slip Ratio' settings are IGNORED\n\n"
        "When disabled:\n"
        "• Uses the static threshold method (default behavior)");
    
    if (engine.m_slope_detection_enabled) {
        // Filter Window
        int window = engine.m_slope_sg_window;
        if (ImGui::SliderInt("Filter Window", &window, 5, 41)) {
            if (window % 2 == 0) window++;  // Force odd
            engine.m_slope_sg_window = window;
            selected_preset = -1;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) Config::Save(engine);
        ImGui::SameLine();
        float latency_ms = (float)(engine.m_slope_sg_window / 2) * 2.5f;
        ImVec4 color = (latency_ms < 25.0f) ? ImVec4(0,1,0,1) : ImVec4(1,0.5f,0,1);
        ImGui::TextColored(color, "~%.0f ms latency", latency_ms);
        ImGui::NextColumn(); ImGui::NextColumn();
        
        FloatSetting("Sensitivity", &engine.m_slope_sensitivity, 0.1f, 5.0f, "%.1fx",
            "Multiplier for slope-to-grip conversion.\n"
            "Higher = More aggressive grip loss detection.\n"
            "Lower = Smoother, less pronounced effect.");
        
        // Advanced (Collapsed by Default)
        if (ImGui::TreeNode("Advanced")) {
            ImGui::NextColumn(); ImGui::NextColumn();
            FloatSetting("Slope Threshold", &engine.m_slope_negative_threshold, -1.0f, 0.0f, "%.2f",
                "Slope value below which grip loss begins.\n"
                "More negative = Later detection (safer).");
            FloatSetting("Output Smoothing", &engine.m_slope_smoothing_tau, 0.005f, 0.100f, "%.3f s",
                "Time constant for grip factor smoothing.\n"
                "Prevents abrupt FFB changes.");
            ImGui::TreePop();
        } else {
            ImGui::NextColumn(); ImGui::NextColumn();
        }
        
        // Diagnostics
        ImGui::Separator();
        ImGui::Text("Live Diagnostics");
        ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), 
            "Slope: %.3f | Grip: %.0f%%", 
            engine.m_slope_current, 
            engine.m_slope_smoothed_output * 100.0f);
        ImGui::NextColumn();
    }
    
    ImGui::TreePop();
} else {
    ImGui::NextColumn(); ImGui::NextColumn();
}
```

### 5. Built-in Preset Updates

**All existing presets** in Config.cpp receive these defaults (feature OFF):

```cpp
// In LoadPresets() or Preset constructors:
.SetSlopeDetection(false)  // Or rely on member defaults
```

The fluent setter `SetSlopeDetection()` can optionally be used for experimental presets:
```cpp
Preset("Experimental - Dynamic Grip", true)
    .SetSlopeDetection(true, 15, 1.0f)
    // ... other settings
```

---

## Test Plan (TDD-Ready)

The following tests should be written **BEFORE** implementing the feature code. Run them to verify they fail (Red Phase), then implement the code to make them pass (Green Phase).

### 1. Unit Tests: Slope Detection Buffer Initialization

**File:** `tests/test_ffb_engine.cpp`

```cpp
static void test_slope_detection_buffer_init()
```

| Aspect | Description |
|--------|-------------|
| **Purpose** | Verify slope detection buffers are properly initialized |
| **Input** | Freshly created `FFBEngine` instance |
| **Expected** | `m_slope_buffer_count == 0`, `m_slope_buffer_index == 0`, `m_slope_current == 0.0` |
| **Assertion** | All three conditions must be true |

### 2. Unit Tests: Savitzky-Golay Derivative Calculation

```cpp
static void test_slope_sg_derivative()
```

| Aspect | Description |
|--------|-------------|
| **Purpose** | Verify SG derivative calculation works for linear ramp |
| **Input** | Buffer filled with linear ramp (y = i * 0.1), window = 9, dt = 0.01 |
| **Expected** | Derivative ≈ 10.0 units/sec (0.1 per sample at 100 Hz) |
| **Assertion** | `abs(derivative - 10.0) < 1.0` |

### 3. Unit Tests: Grip at Peak (Zero Slope)

```cpp
static void test_slope_grip_at_peak()
```

| Aspect | Description |
|--------|-------------|
| **Purpose** | Verify grip factor is 1.0 when slope is zero (at peak) |
| **Input** | Constant lateral G (1.2), constant slip (0.05), dt = 0.0025 (400 Hz) |
| **Expected** | `m_slope_smoothed_output > 0.95` (near 1.0) |
| **Assertion** | Grip factor remains high with zero slope |

### 4. Unit Tests: Grip Past Peak (Negative Slope)

```cpp
static void test_slope_grip_past_peak()
```

| Aspect | Description |
|--------|-------------|
| **Purpose** | Verify grip factor reduces when slope is negative (past peak) |
| **Input** | Increasing slip (0.05 to 0.09), decreasing G (1.5 to 1.1) over 20 frames |
| **Expected** | `0.2 < m_slope_smoothed_output < 0.9` |
| **Assertion** | Grip factor is reduced but not below safety floor |

### 5. Unit Tests: Slope Detection vs Static Comparison

```cpp
static void test_slope_vs_static_comparison()
```

| Aspect | Description |
|--------|-------------|
| **Purpose** | Verify both slope and static methods detect grip loss |
| **Input** | Two engines configured differently, same telemetry with 12% slip |
| **Expected** | Both engines detect grip loss (< 0.95 for slope, < 0.8 for static) |
| **Assertion** | Both detection methods should agree on grip loss |

### 6. Unit Tests: Config Persistence

```cpp
static void test_slope_config_persistence()
```

| Aspect | Description |
|--------|-------------|
| **Purpose** | Verify slope settings save and load correctly |
| **Input** | Non-default values (enabled=true, window=21, sensitivity=2.5f) |
| **Expected** | Values survive save/load cycle |
| **Assertion** | All loaded values match saved values |

### 7. Unit Tests: Latency Characteristics

```cpp
static void test_slope_latency_characteristics()
```

| Aspect | Description |
|--------|-------------|
| **Purpose** | Verify buffer fills correctly and latency is as expected |
| **Input** | Window size = 15, dt = 0.0025 (400 Hz) |
| **Expected** | Buffer fills in exactly `window_size` frames, latency = ~17.5ms |
| **Assertion** | Frame count matches window size |

### 8. Unit Tests: Noise Rejection

```cpp
static void test_slope_noise_rejection()
```

| Aspect | Description |
|--------|-------------|
| **Purpose** | Verify SG filter rejects noise while preserving signal |
| **Input** | Constant G (1.2) + random noise (±0.1), 50 frames |
| **Expected** | `abs(m_slope_current) < 1.0` (near zero despite noise) |
| **Assertion** | Noise is filtered, slope remains stable |

---

## Documentation Updates

1. **CHANGELOG.md** - Add entry describing the new Slope Detection feature
2. **VERSION** - Increment version number (suggest v0.7.0 for this feature)
3. **README.md** - Add brief description in Features section (optional)

---

## User Settings & Presets Impact

### New Settings

Five new configurable parameters are being added (see Section 1). All have sensible defaults and the feature is **disabled by default** to ensure backward compatibility.

| Setting | GUI Location | Visibility |
|---------|--------------|------------|
| Enable Slope Detection | Grip & Slip section | Always |
| Filter Window | Slope Detection section | When enabled |
| Sensitivity | Slope Detection section | When enabled |
| Slope Threshold | Advanced (collapsed) | When enabled |
| Output Smoothing | Advanced (collapsed) | When enabled |

### Migration Logic

**No migration required.** When loading old configuration files:
- Missing settings will use default values
- `slope_detection_enabled = false` ensures existing users see no change in behavior
- Static threshold logic remains as fallback

### Settings Interaction

When Slope Detection is **enabled**:
- `m_optimal_slip_angle` is **IGNORED** (not used)
- `m_optimal_slip_ratio` is **IGNORED** (not used)
- UI should clearly indicate this (tooltip mentions settings are ignored)

### Preset Updates

All built-in presets should be updated to include the new settings with the feature disabled by default. This is a non-breaking change.

---

## Deliverables Checklist

### Code Changes
- [ ] `src/FFBEngine.h` - Add slope detection members and helper functions (~50 lines)
- [ ] `src/FFBEngine.h` - Modify `calculate_grip()` to use slope detection (~30 lines)
- [ ] `src/Config.h` - Add new settings to Preset struct (~15 lines)
- [ ] `src/Config.h` - Add fluent setter `SetSlopeDetection()` (~5 lines)
- [ ] `src/Config.h` - Update `Apply()` and `UpdateFromEngine()` (~10 lines)
- [ ] `src/Config.cpp` - Add save/load/validation logic (~30 lines)
- [ ] `src/GuiLayer.cpp` - Add GUI controls for slope detection (~60 lines)

### Test Files
- [ ] `tests/test_ffb_engine.cpp` - Add 8 new unit tests (~100 lines)

### Documentation
- [ ] `CHANGELOG.md` - Add feature entry
- [ ] `VERSION` - Update version number

### Verification
- [ ] All existing tests pass (no regressions)
- [ ] All new tests pass
- [ ] Build succeeds in Release mode
- [ ] GUI displays correctly and values persist on restart
- [ ] Understeer effect works correctly with static thresholds (feature OFF)
- [ ] Understeer effect works correctly with slope detection (feature ON)

---

## Technical Notes

### Expected Latency
At 400 Hz telemetry rate:
- Window 9: ~10ms latency
- Window 15: ~17.5ms latency (recommended)
- Window 25: ~30ms latency

### Signal Processing Rationale
Savitzky-Golay filter is chosen over simple moving average because:
1. Preserves peak shape (critical for detecting SAT drop-off)
2. Provides derivative as direct output
3. Superior noise rejection without excessive phase lag

### Savitzky-Golay Coefficient Generation

**Mathematical Formula (Quadratic Fit, First Derivative):**

For a symmetric window with half-width `M` (total window size = `2M+1` samples):

```
w_k = k / (S_2 × h)

where:
  S_2 = M(M+1)(2M+1)/3    (sum of k² from -M to M)
  h   = sampling interval (dt)
  k   = sample index relative to center (-M to +M)
```

**Key Insight (Linear-Quadratic Equivalence):**
For first derivative estimation on symmetric windows, quadratic (N=2) and linear (N=1) polynomial fits produce **identical coefficients**. The quadratic term captures curvature (second derivative) but does not affect the slope at the center point due to the orthogonality of basis functions.

**Implementation Strategy:**
- Coefficients are generated at **runtime** using the closed-form formula (O(1) computation)
- No pre-computed lookup tables required
- Convolution exploits **anti-symmetry** (`w_{-k} = -w_k`) for ~50% reduction in multiplications

**Reference:** `docs/dev_docs/plans/savitzky-golay coefficients deep research report.md`

### Boundary Handling (Shrinking Symmetric Window)

At signal boundaries (startup, buffer not filled), the algorithm uses a **shrinking symmetric window** strategy:

1. At index `i` where `i < M`, set effective window size: `M_eff = i`
2. This maintains the symmetric window property and preserves phase linearity
3. Trade-off: Reduced noise suppression at edges (smaller window = higher variance)
4. Minimum window: `M=2` (5 samples) - below this, return 0.0 (not enough data)

**Why not asymmetric fit?**
Asymmetric polynomial fits (e.g., using only forward samples at startup) are susceptible to "Runge's Phenomenon" - polynomial oscillation at interval edges. The shrinking symmetric approach avoids this instability.

### Safety Considerations
- Grip factor has a floor of 0.2 to prevent complete FFB loss
- Feature is disabled by default to avoid surprising existing users
- Static threshold logic remains as fallback
- Window size validation ensures odd number (required for SG filter)

### Interaction with Existing Features
- **Speed Gate**: Slope detection output is still multiplied by speed gate
- **Grip Approximation Fallback**: Slope detection only active when mGripFract is missing (same condition as static threshold)
- **Diagnostics**: Graph plots will show the same `calc_front_grip` value regardless of method

---

## Future Deprecation Path (Informational)

If Slope Detection proves superior after user testing:
1. **v0.7.x**: Gather user feedback, iterate on sensitivity defaults
2. **v0.8.0**: Slope Detection enabled by default for new users
3. **v0.9.0**: Static threshold sliders moved to "Legacy" section in GUI
4. **v1.0.0**: Static threshold logic removed (or kept as hidden fallback)

---

*Plan created: 2026-02-01*  
*Last updated: 2026-02-01 (added SG coefficient details)*  
*Estimated implementation effort: ~300 lines of C++ code across 4 files*

---

## Appendix: Reference Documents

1. **Savitzky-Golay Coefficients Deep Research Report**  
   `docs/dev_docs/plans/savitzky-golay coefficients deep research report.md`  
   Comprehensive mathematical derivation and C++ implementation patterns for SG filter coefficients.

---

## Implementation Notes

*Added: 2026-02-01 (post-implementation)*

### Unforeseen Issues

1. **Missing Preset-Engine Synchronization Fields**
   - **Issue:** Several parameters (`optimal_slip_angle`, `optimal_slip_ratio`, smoothing parameters) were declared in both `Preset` and `FFBEngine` but were missing from `Preset::Apply()` and `Preset::UpdateFromEngine()` methods.
   - **Consequence:** When `Preset::ApplyDefaultsToEngine()` was called, these fields remained at zero/uninitialized, triggering validation warnings like "Invalid optimal_slip_angle (0), resetting to default 0.10".
   - **Resolution:** Added all missing parameter synchronization to both methods in Config.h (lines 265-276 and 325-340).

2. **FFBEngine Constructor Initialization Order**
   - **Issue:** The plan did not address where `Preset::ApplyDefaultsToEngine()` should be called during FFBEngine construction. The FFBEngine constructor was defined inline in FFBEngine.h but needed to call methods from Config.h which includes FFBEngine.h—a circular dependency.
   - **Resolution:** Moved FFBEngine constructor definition to Config.h as an `inline` function after the Preset class definition, allowing it to call `Preset::ApplyDefaultsToEngine(*this)`.

3. **Unicode Encoding Issues with Test Files**
   - **Issue:** Some test files (`test_ffb_engine_HEAD.cpp`, `deleted_lines.txt`) had UTF-16LE encoding, causing `view_file` tool failures with "unsupported mime type text/plain; charset=utf-16le".
   - **Workaround:** Created UTF-8 copies of files using PowerShell: `Get-Content file.cpp | Out-File -Encoding utf8 file_utf8.cpp`
   - **Recommendation:** See `docs/dev_docs/unicode_encoding_issues.md` for detailed guidance.

### Plan Deviations

1. **Front-Only Slope Detection**
   - **Deviation:** Added `is_front` parameter to `calculate_grip()` function to restrict slope detection to front wheels only.
   - **Rationale:** Slope detection uses lateral G-force, which is a vehicle-level measurement. Applying the same slope calculation to rear wheels would give identical (and incorrect) results. Rear grip continues to use the static threshold method.

2. **Slope Fallback Behavior**
   - **Original Plan:** When slip angle isn't changing (`dAlpha_dt < 0.001`), set `m_slope_current = 20.0` (assume peak grip).
   - **Deviation:** Changed to retain previous slope value instead of resetting.
   - **Rationale:** The noise rejection test (`test_slope_noise_rejection`) expects the SG filter to smooth out G-force noise when slip angle is constant. Resetting slope to 20.0 every frame when slip isn't changing defeated this purpose.

3. **Grip Factor Safety Clamp**
   - **Added:** Explicit `(std::max)(0.2, (std::min)(1.0, current_grip_factor))` clamp.
   - **Rationale:** The plan mentioned a 0.2 floor but did not include explicit clamping code. Added to prevent edge cases where excessive negative slopes could produce negative grip factors.

### Challenges Encountered

1. **Test Telemetry Setup (`test_slope_vs_static_comparison`)**
   - **Challenge:** Original test only varied G-force while keeping slip angle constant. This meant `dAlpha_dt = 0`, which triggered the "no change in slip" branch rather than actually testing slope calculation.
   - **Resolution:** Modified test to vary both G-force and slip angle over time, simulating a realistic tire curve progression from increasing grip to grip loss past the peak.

2. **Buffer Indexing in Savitzky-Golay Calculation**
   - **Challenge:** The circular buffer indexing required careful handling when buffer count was less than window size. Initial implementation had edge cases where array indices could be negative before modulo.
   - **Resolution:** Used `(index + SLOPE_BUFFER_MAX) % SLOPE_BUFFER_MAX` pattern consistently to ensure always-positive indices.

3. **Debugging Build Failures**
   - **Challenge:** Initial build after implementing the plan had cryptic linker errors due to the FFBEngine constructor circular dependency issue.
   - **Resolution:** Systematic analysis of include order and moving constructor definition to the right location in the header dependency chain.

### Recommendations for Future Plans

1. **Explicitly Document Parameter Synchronization**
   - Future plans that add new parameters should explicitly list:
     - Declaration location in FFBEngine.h
     - Declaration location in Preset struct (Config.h)
     - Entry in `Preset::Apply()`
     - Entry in `Preset::UpdateFromEngine()`
     - Entry in `Config::Save()`
     - Entry in `Config::Load()`
   - Use a parameter checklist table format to ensure nothing is missed.

2. **Include Initialization Order Analysis**
   - When adding functionality that spans FFBEngine.h and Config.h, analyze the circular dependency implications upfront.
   - Document whether inline functions or out-of-class definitions are needed.

3. **Test Case Design Should Include Data Flow Analysis**
   - For derivative-based algorithms, explicitly document what telemetry inputs need to change and how.
   - Include "telemetry script" examples showing multi-frame progressions.

4. **Consider File Encoding in Agents Guidelines**
   - Add encoding handling guidance to agent instruction documents.
   - Prefer UTF-8 with BOM for new source files on Windows to avoid tooling issues.

5. **Add Boundary Condition Tests**
   - For buffer-based algorithms, include explicit tests for:
     - Empty buffer
     - Partially filled buffer
     - Exactly full buffer
     - Buffer wraparound

---

## Planned Enhancements (v0.7.1)

### Lower Minimum Filter Window to 3

**Rationale:**  
After reviewing the v0.7.0 implementation, we identified an opportunity to provide users with an ultra-low latency option while maintaining mathematical validity.

**Current State (v0.7.0):**
- Minimum window: 5 samples
- Minimum latency: 6.25ms

**Proposed Change (v0.7.1):**
- Minimum window: 3 samples
- Minimum latency: 3.75ms

**Technical Justification:**
- Savitzky-Golay quadratic polynomial fitting requires minimum 3 points
- Window=3 is mathematically valid, though noisier than window=5
- Provides 40% latency reduction for latency-sensitive users

**Implementation Details:**

1. **Lower validation minimum** (`Config.cpp` line 1093):
   ```cpp
   if (engine.m_slope_sg_window < 3) engine.m_slope_sg_window = 3;  // Changed from 5
   ```

2. **Update GUI slider range** (`GuiLayer.cpp` line 1117):
   ```cpp
   if (ImGui::SliderInt("  Filter Window", &window, 3, 41)) {  // Changed from 5
   ```

3. **Add dynamic tooltip warning** (`GuiLayer.cpp` after line 1122):
   ```cpp
   if (ImGui::IsItemHovered()) {
       if (engine.m_slope_sg_window <= 5) {
           ImGui::SetTooltip(
               "WARNING: Window sizes < 7 may produce noisy feedback.\n"
               "Recommended: 11-21 for smooth operation.\n\n"
               "Current latency: ~%.1f ms", latency_ms);
       } else {
           ImGui::SetTooltip(
               "Savitzky-Golay filter window size.\n"
               "Larger = Smoother but higher latency.\n"
               "Smaller = Faster response but noisier.\n\n"
               "Current latency: ~%.1f ms", latency_ms);
       }
   }
   ```

4. **Update user documentation** (`docs/Slope_Detection_Guide.md`):
   - Change minimum in all examples from 5 to 3
   - Add warning about noise with window < 7
   - Update latency calculations

**Testing Requirements:**
- Test window=3 with noisy telemetry (curbs, bumps)
- Validate SG coefficient calculation at edge case
- Ensure tooltip displays correctly
- Update `test_slope_latency_characteristics` to include window=3 case

**User Impact:**
- Advanced users gain access to 3.75ms latency option
- Default remains at 15 (conservative, well-tested)
- Warning tooltip prevents accidental misconfiguration

**Priority:** Low (Enhancement, not a fix)  
**Complexity:** 2/10 (Simple parameter changes)  
**Risk:** Low (Mathematically sound, optional feature)

---

### Buffer Reset on Toggle (Implemented in v0.7.0)

**Issue:**  
When slope detection is toggled ON mid-session, stale buffer data from when the feature was OFF could cause incorrect initial slope calculations.

**Solution (Implemented):**  
Added automatic buffer reset when `m_slope_detection_enabled` transitions from `false` → `true`:

```cpp
// GuiLayer.cpp - lines 1105-1126
if (!prev_slope_enabled && engine.m_slope_detection_enabled) {
    engine.m_slope_buffer_count = 0;
    engine.m_slope_buffer_index = 0;
    engine.m_slope_smoothed_output = 1.0;  // Start at full grip
    std::cout << "[SlopeDetection] Enabled - buffers cleared" << std::endl;
}
```

**User Impact:**
- Eliminates potential FFB glitches when enabling slope detection mid-drive
- Ensures clean slate for derivative calculation
- No impact on performance (happens only on toggle event)

**Status:** ✅ Implemented in v0.7.0 (GuiLayer.cpp)

---

*Implementation completed: 2026-02-01*  
*All 503 tests passing*  
*Total implementation time: ~2 hours*
