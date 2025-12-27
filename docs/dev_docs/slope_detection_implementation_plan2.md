# Slope Detection Algorithm: Comprehensive Implementation Plan

## Executive Summary

This document provides a comprehensive implementation plan for replacing the current static optimal slip angle/ratio grip estimation system with a dynamic **Slope Detection Algorithm**. This algorithm monitors the real-time derivative of force vs. slip to automatically detect when the tire is at peak grip, eliminating the need for user-configured optimal slip values and providing a more accurate, adaptive FFB response to grip changes.

---

## 1. Introduction and Problem Statement

### 1.1 The Current Implementation: Static Threshold Approach

The lmuFFB application currently estimates tire grip using a **fixed optimal slip angle** and **fixed optimal slip ratio**, which are user-configurable parameters:

*   `m_optimal_slip_angle` (Default: 0.10 rad / ~5.7°)
*   `m_optimal_slip_ratio` (Default: 0.12 / 12%)

These static thresholds are used in the `calculate_grip()` function to determine when the tire has exceeded its peak grip:

```cpp
// Current implementation in FFBEngine.h (lines 576-601)
double lat_metric = std::abs(result.slip_angle) / (double)m_optimal_slip_angle;
double long_metric = avg_ratio / (double)m_optimal_slip_ratio;
double combined_slip = std::sqrt((lat_metric * lat_metric) + (long_metric * long_metric));

if (combined_slip > 1.0) {
    double excess = combined_slip - 1.0;
    result.value = 1.0 / (1.0 + excess * 2.0);  // Sigmoid drop-off
} else {
    result.value = 1.0;
}
```

### 1.2 Limitations of the Static Approach

The static threshold approach has significant limitations:

1. **Setup-Specific Variation**: The optimal slip angle and ratio vary significantly based on:
   - Vehicle type (GT3 ~7°, Hypercar ~4°, Formula ~5°)
   - Tire compound (Soft vs. Hard)
   - Aerodynamic load (downforce increases load, shifting peak slip angle lower)
   - Suspension geometry (camber, caster)

2. **Dynamic Environmental Factors**: Even with the same car and setup, the optimal peak shifts during a session:
   - **Tire Temperature**: Cold tires peak at lower slip angles; overheated tires become "spongy"
   - **Tire Wear**: Worn tires behave more like slicks (lower peak angle)
   - **Rain/Wet Track**: Wet surfaces peak very early and drop off sharply
   - **Track Rubbering**: The peak shifts as the track evolves

3. **User Calibration Burden**: Users must manually tune these values for each car, which is non-intuitive and requires domain expertise.

### 1.3 The Solution: Slope Detection Algorithm

The **Slope Detection Algorithm** eliminates the need for static thresholds by monitoring the **rate of change (derivative)** of the relationship between lateral force (or lateral G-force) and slip angle. Instead of asking "have you exceeded 5.7 degrees?", the algorithm asks "is more steering input producing more grip, or less?"

#### Core Principle

The relationship between lateral force ($F_y$) and slip angle ($\alpha$) follows a characteristic curve:

1. **Linear Region** (Slope > 0): More steering → More grip. The steering feels progressive and weighted.
2. **Peak Grip** (Slope ≈ 0): The tire is saturated. Maximum lateral force is achieved.
3. **Frictional/Sliding Region** (Slope < 0): More steering → Less grip. The contact patch is sliding.

By monitoring the slope (derivative) $\frac{\Delta F_y}{\Delta \alpha}$:
- **Positive Slope**: The driver is building grip. No intervention needed.
- **Zero Slope**: The driver is at peak. This is the "optimal slip angle" - detected automatically.
- **Negative Slope**: The driver has exceeded the peak. FFB should lighten to signal understeer.

#### Why Slope Detection is Superior

| Feature                    | Static Threshold (Current)          | Slope Detection (Proposed)           |
|----------------------------|-------------------------------------|--------------------------------------|
| **Calibration Required**   | Yes (per-car tuning)                | No (self-calibrating)                |
| **Adapts to Temperature**  | No                                  | Yes (reacts to real-time physics)    |
| **Adapts to Wear**         | No                                  | Yes                                  |
| **Adapts to Rain**         | No (wet requires different values)  | Yes (detects flattened curve)        |
| **Accuracy at High Speed** | Fixed (may underestimate peak)      | Dynamic (accounts for aero load)     |

---

## 2. Theoretical Foundation

### 2.1 The Self-Aligning Torque (SAT) and Pneumatic Trail

The force a driver feels through the steering wheel is not the lateral force ($F_y$) directly, but the **Self-Aligning Torque (SAT)**, also known as Aligning Moment ($M_z$):

$$M_z = F_y \cdot (t_p + t_m)$$

Where:
- $t_p$ = Pneumatic Trail (distance from wheel center to force centroid)
- $t_m$ = Mechanical Trail (from suspension geometry)

**Critical Insight**: The pneumatic trail ($t_p$) collapses as the contact patch saturates. This causes $M_z$ to peak **before** $F_y$ reaches its maximum. Typically:
- SAT peaks at 3°-6° slip angle
- Lateral Force ($F_y$) peaks at 6°-10° slip angle

This "offset" is functionally desirable because the SAT peak represents the **limit of stability**, providing an early warning before the tire completely breaks away.

### 2.2 The Derivative as a Haptic Cue

Slope Detection monitors the derivative of the SAT or lateral force:

```
Slope = ΔM_z / Δα  (or ΔF_y / Δα, or ΔLateralG / Δα)
```

- **Phase 1 (Linear)**: Slope is positive and constant. Steering feels weighted.
- **Phase 2 (Peak)**: Slope transitions through zero. This is the "perfect" driving zone.
- **Phase 3 (Drop-off)**: Slope becomes negative. Steering lightens dramatically.

### 2.3 The "Slip-Slope" Correlation for Grip Estimation

Research into autonomous vehicle dynamics confirms that the slope of the initial linear region of the friction-slip curve is a robust predictor of the surface friction coefficient (μ_max). A steep slope indicates high grip; a shallow slope indicates low grip.

This means Slope Detection can estimate **both**:
1. **Absolute Grip Level**: By measuring the steepness of the initial build-up
2. **Limit of Adhesion**: By detecting the transition to negative slope

---

## 3. Signal Processing Challenges

### 3.1 The Derivative-Noise Dilemma

Numerical differentiation amplifies high-frequency noise. The derivative operator acts as a high-pass filter:

$$\text{If } x(t) = \text{signal} + \epsilon(t), \text{ then } x'(t) = \text{signal'} + \epsilon'(t)$$

Noise ($\epsilon$) fluctuates rapidly, so its derivative ($\epsilon'$) can be orders of magnitude larger than the actual signal derivative. In lmuFFB, telemetry data from LMU has:
- Quantization jitter
- Road texture noise (macrotexture)
- Suspension micro-oscillations

A naive derivative calculation would produce a "jagged" slope estimate, triggering false positives for grip loss.

### 3.2 Latency vs. Filtering Trade-Off

To combat noise, filtering is mandatory. However, all causal filters introduce phase delay (latency):

| Filter Type          | Noise Rejection | Latency    | Peak Preservation |
|----------------------|-----------------|------------|-------------------|
| Moving Average (LPF) | Good            | Moderate   | Poor (flattens)   |
| Savitzky-Golay       | Excellent       | Moderate   | **Excellent**     |
| Kalman Filter (EKF)  | Excellent       | Low        | Excellent         |

**Savitzky-Golay (SG) Filter** is the recommended approach for this implementation because:
1. It fits a polynomial to a window of samples, preserving peak shape
2. It provides the derivative as a direct output (no separate differentiation step)
3. At 400 Hz telemetry (2.5ms per sample), a 15-sample window introduces only ~17ms latency

### 3.3 Expected Latency at 400 Hz Telemetry

| Window Size (Samples) | Latency (ms) | Filtering Characteristic                     |
|-----------------------|--------------|----------------------------------------------|
| **9**                 | **10.0**     | Light smoothing, some noise                  |
| **15**                | **17.5**     | **Recommended**. Good noise rejection.       |
| **25**                | **30.0**     | Strong smoothing. For very noisy signals.    |
| **41**                | **50.0**     | Noticeable delay. Not recommended for FFB.   |

---

## 4. Algorithm Design

### 4.1 Core Slope Detection Logic

```cpp
// Proposed Algorithm
double CalculateGripFromSlope(double lateral_g, double slip_angle, double dt) {
    // 1. Update Slope Buffers (Circular buffer for SG filter)
    m_lat_g_buffer.push(lateral_g);
    m_slip_buffer.push(slip_angle);
    
    // 2. Calculate Smoothed Derivative using Savitzky-Golay
    double dLateralG_dSlip = 0.0;
    if (m_lat_g_buffer.size() >= SG_WINDOW_SIZE) {
        double d_lat_g = SavitzkyGolayDerivative(m_lat_g_buffer, dt);
        double d_slip = SavitzkyGolayDerivative(m_slip_buffer, dt);
        
        // Avoid division by zero
        if (std::abs(d_slip) > 0.001) {
            dLateralG_dSlip = d_lat_g / d_slip;
        }
    }
    
    // 3. Classify Grip State based on Slope
    //    Positive slope → grip building
    //    Zero slope → at peak
    //    Negative slope → past peak (understeer)
    
    double grip_factor = 1.0;
    
    if (dLateralG_dSlip < SLOPE_NEGATIVE_THRESHOLD) {
        // Past peak - calculate grip reduction
        double excess = std::abs(dLateralG_dSlip) / SLOPE_SENSITIVITY;
        grip_factor = 1.0 / (1.0 + excess);  // Sigmoid response
    }
    
    return grip_factor;
}
```

### 4.2 Configuration Parameters

| Parameter                     | Type   | Default | Description                                             |
|-------------------------------|--------|---------|----------------------------------------------------------|
| `m_slope_detection_enabled`   | bool   | false   | Enable/disable Slope Detection (allows fallback to static) |
| `m_slope_sg_window`           | int    | 15      | Savitzky-Golay window size (samples)                     |
| `m_slope_sensitivity`         | float  | 1.0     | Multiplier for slope-to-grip conversion                  |
| `m_slope_negative_threshold`  | float  | -0.1    | Slope below which grip loss is detected                  |
| `m_slope_smoothing_tau`       | float  | 0.02    | Additional LPF time constant for output (seconds)        |

### 4.3 Hybrid Mode: Slope + Static

To maintain backward compatibility and provide a fallback:

```cpp
double final_grip = 1.0;

if (m_slope_detection_enabled) {
    final_grip = CalculateGripFromSlope(lateral_g, slip_angle, dt);
} else {
    // Existing static threshold logic
    final_grip = CalculateGripFromStaticThreshold(slip_angle, slip_ratio);
}
```

Users can toggle between the two modes via a GUI checkbox.

---

## 5. Code Changes Required

### 5.1 Files to Modify

| File                  | Changes Required                                                       |
|-----------------------|------------------------------------------------------------------------|
| `src/FFBEngine.h`     | Add Slope Detection members, buffers, and helper functions             |
| `src/Config.h`        | Add configuration parameters to `Preset` struct                        |
| `src/Config.cpp`      | Add save/load logic for new parameters                                 |
| `src/GuiLayer.cpp`    | Add UI controls for Slope Detection settings                           |
| `tests/test_ffb_engine.cpp` | Add comprehensive unit tests                                     |

### 5.2 FFBEngine.h Changes

#### 5.2.1 New Member Variables

Add these to the `FFBEngine` class public section (after line ~244):

```cpp
// ===== SLOPE DETECTION (v0.7.0) =====
bool m_slope_detection_enabled = false;  // Enable dynamic slope detection
int m_slope_sg_window = 15;              // Savitzky-Golay window size (samples)
float m_slope_sensitivity = 1.0f;        // Sensitivity multiplier
float m_slope_negative_threshold = -0.1f; // Slope below which grip loss is detected
float m_slope_smoothing_tau = 0.02f;     // Output smoothing time constant (seconds)
```

#### 5.2.2 Internal State Buffers

Add these to the private section (after line ~350):

```cpp
// Slope Detection Buffers (Circular)
static constexpr int SLOPE_BUFFER_MAX = 41;  // Max window size
std::array<double, SLOPE_BUFFER_MAX> m_slope_lat_g_buffer = {};
std::array<double, SLOPE_BUFFER_MAX> m_slope_slip_buffer = {};
int m_slope_buffer_index = 0;
int m_slope_buffer_count = 0;

// Slope Detection State
double m_slope_current = 0.0;            // Current estimated slope
double m_slope_grip_factor = 1.0;        // Smoothed grip factor from slope
double m_slope_smoothed_output = 1.0;    // LPF output
```

#### 5.2.3 New Helper Functions

Add these public methods:

```cpp
// Savitzky-Golay Derivative Calculation (Order 2, Window configurable)
// Returns the derivative (slope) at the center point of the buffer
double CalculateSGDerivative(const std::array<double, SLOPE_BUFFER_MAX>& buffer, 
                              int count, int window_size, double dt) {
    if (count < window_size) return 0.0;
    
    // Savitzky-Golay coefficients for 1st derivative (polynomial degree 2)
    // These are for a 15-sample window. Precomputed for efficiency.
    // For flexibility, we use the simplified formula for symmetric windows.
    // Derivative = sum(i * x[center + i]) / normalization
    
    int half_window = window_size / 2;
    double sum = 0.0;
    double norm = 0.0;
    
    for (int i = -half_window; i <= half_window; i++) {
        int idx = (m_slope_buffer_index - half_window + i + SLOPE_BUFFER_MAX) % SLOPE_BUFFER_MAX;
        sum += i * buffer[idx];
        norm += i * i;
    }
    
    // Derivative per sample, convert to derivative per second
    return (sum / norm) / dt;
}

// Calculate grip factor from slope detection
double CalculateSlopeGrip(double lateral_g, double slip_angle, double dt) {
    // 1. Push new samples into circular buffers
    m_slope_lat_g_buffer[m_slope_buffer_index] = lateral_g;
    m_slope_slip_buffer[m_slope_buffer_index] = slip_angle;
    m_slope_buffer_index = (m_slope_buffer_index + 1) % SLOPE_BUFFER_MAX;
    if (m_slope_buffer_count < SLOPE_BUFFER_MAX) m_slope_buffer_count++;
    
    // 2. Check if we have enough samples
    if (m_slope_buffer_count < m_slope_sg_window) {
        return 1.0;  // Not enough data yet
    }
    
    // 3. Calculate derivatives using Savitzky-Golay
    double d_lat_g = CalculateSGDerivative(m_slope_lat_g_buffer, 
                                            m_slope_buffer_count, 
                                            m_slope_sg_window, dt);
    double d_slip = CalculateSGDerivative(m_slope_slip_buffer, 
                                           m_slope_buffer_count, 
                                           m_slope_sg_window, dt);
    
    // 4. Calculate slope (dG/dSlip)
    // Avoid division by zero - require minimum slip rate change
    if (std::abs(d_slip) > 0.001) {
        m_slope_current = d_lat_g / d_slip;
    }
    // else: retain previous slope (stale but better than noise spike)
    
    // 5. Convert slope to grip factor
    double grip_factor = 1.0;
    
    if (m_slope_current < m_slope_negative_threshold) {
        // Past peak - grip is decreasing with more slip
        double excess = std::abs(m_slope_current - m_slope_negative_threshold);
        excess *= m_slope_sensitivity;
        grip_factor = 1.0 / (1.0 + excess);
        grip_factor = (std::max)(0.2, grip_factor);  // Safety floor
    }
    
    // 6. Apply output smoothing (Time-Corrected LPF)
    double tau = (double)m_slope_smoothing_tau;
    if (tau < 0.001) tau = 0.001;
    double alpha = dt / (tau + dt);
    m_slope_smoothed_output += alpha * (grip_factor - m_slope_smoothed_output);
    
    return m_slope_smoothed_output;
}
```

#### 5.2.4 Modify calculate_grip() Function

Replace the static threshold logic with conditional Slope Detection (around lines 576-601):

```cpp
// In calculate_grip() function, replace the else block starting at "if (car_speed < 5.0)"

if (result.value < 0.0001 && avg_load > 100.0) {
    result.approximated = true;
    
    if (car_speed < 5.0) {
        result.value = 1.0;
    } else {
        // v0.7.0: Use Slope Detection if enabled
        if (m_slope_detection_enabled) {
            result.value = CalculateSlopeGrip(data->mLocalAccel.x / 9.81, 
                                               result.slip_angle, dt);
        } else {
            // Original static threshold logic (fallback)
            double lat_metric = std::abs(result.slip_angle) / (double)m_optimal_slip_angle;
            double ratio1 = calculate_manual_slip_ratio(w1, car_speed);
            double ratio2 = calculate_manual_slip_ratio(w2, car_speed);
            double avg_ratio = (std::abs(ratio1) + std::abs(ratio2)) / 2.0;
            double long_metric = avg_ratio / (double)m_optimal_slip_ratio;
            double combined_slip = std::sqrt((lat_metric * lat_metric) + 
                                             (long_metric * long_metric));

            if (combined_slip > 1.0) {
                double excess = combined_slip - 1.0;
                result.value = 1.0 / (1.0 + excess * 2.0);
            } else {
                result.value = 1.0;
            }
        }
    }
    
    result.value = (std::max)(0.2, result.value);  // Safety clamp
    
    if (!warned_flag) {
        std::cout << "Warning: Data for mGripFract from the game seems to be missing for this car ("
                  << vehicleName << "). A fallback estimation will be used." << std::endl;
        warned_flag = true;
    }
}
```

### 5.3 Config.h Changes

Add these fields to the `Preset` struct (around line 70):

```cpp
// Slope Detection Settings (v0.7.0)
bool slope_detection_enabled = false;
int slope_sg_window = 15;
float slope_sensitivity = 1.0f;
float slope_negative_threshold = -0.1f;
float slope_smoothing_tau = 0.02f;
```

Add to the `ApplyDefaultsToEngine()` method (around line 230):

```cpp
engine.m_slope_detection_enabled = slope_detection_enabled;
engine.m_slope_sg_window = slope_sg_window;
engine.m_slope_sensitivity = slope_sensitivity;
engine.m_slope_negative_threshold = slope_negative_threshold;
engine.m_slope_smoothing_tau = slope_smoothing_tau;
```

Add to the `SyncFromEngine()` method (around line 285):

```cpp
slope_detection_enabled = engine.m_slope_detection_enabled;
slope_sg_window = engine.m_slope_sg_window;
slope_sensitivity = engine.m_slope_sensitivity;
slope_negative_threshold = engine.m_slope_negative_threshold;
slope_smoothing_tau = engine.m_slope_smoothing_tau;
```

### 5.4 Config.cpp Changes

Add save logic (around line 470):

```cpp
// Slope Detection Settings
file << "slope_detection_enabled=" << (engine.m_slope_detection_enabled ? "1" : "0") << "\n";
file << "slope_sg_window=" << engine.m_slope_sg_window << "\n";
file << "slope_sensitivity=" << engine.m_slope_sensitivity << "\n";
file << "slope_negative_threshold=" << engine.m_slope_negative_threshold << "\n";
file << "slope_smoothing_tau=" << engine.m_slope_smoothing_tau << "\n";
```

Add load logic (around line 622):

```cpp
else if (key == "slope_detection_enabled") engine.m_slope_detection_enabled = (value == "1");
else if (key == "slope_sg_window") engine.m_slope_sg_window = std::stoi(value);
else if (key == "slope_sensitivity") engine.m_slope_sensitivity = std::stof(value);
else if (key == "slope_negative_threshold") engine.m_slope_negative_threshold = std::stof(value);
else if (key == "slope_smoothing_tau") engine.m_slope_smoothing_tau = std::stof(value);
```

Add validation logic (around line 640):

```cpp
// Slope Detection Validation
if (engine.m_slope_sg_window < 5 || engine.m_slope_sg_window > 41) {
    std::cerr << "[Config] Invalid slope_sg_window (" << engine.m_slope_sg_window 
              << "). Clamping to 5-41 range." << std::endl;
    engine.m_slope_sg_window = std::max(5, std::min(41, engine.m_slope_sg_window));
}
if (engine.m_slope_sensitivity < 0.1f || engine.m_slope_sensitivity > 10.0f) {
    engine.m_slope_sensitivity = std::max(0.1f, std::min(10.0f, engine.m_slope_sensitivity));
}
if (engine.m_slope_smoothing_tau < 0.001f) {
    engine.m_slope_smoothing_tau = 0.02f;  // Reset to default
}
```

### 5.5 GuiLayer.cpp Changes

Add a new collapsible section in the GUI for Slope Detection (around line 1140, after the Optimal Slip settings):

```cpp
// ===== SLOPE DETECTION (Experimental) =====
ImGui::Separator();
ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f)); // Yellow for experimental
if (ImGui::CollapsingHeader("Slope Detection (Experimental)")) {
    ImGui::PopStyleColor();
    
    ImGui::TextWrapped("Dynamic grip estimation using real-time slope analysis. "
                       "Replaces static optimal slip values with automatic peak detection.");
    ImGui::Dummy(ImVec2(0, 5));
    
    // Enable Toggle
    if (ImGui::Checkbox("Enable Slope Detection", &engine.m_slope_detection_enabled)) {
        // When enabled, the static optimal slip sliders become irrelevant
    }
    ImGui::SameLine();
    HelpMarker("When enabled, the system automatically detects the tire's peak grip point "
               "by monitoring the derivative of G-force vs slip angle. "
               "This adapts to tire temperature, wear, and rain conditions.");
    
    // Only show tuning options if enabled
    if (engine.m_slope_detection_enabled) {
        ImGui::Indent();
        
        // Window Size
        ImGui::SliderInt("Filter Window", &engine.m_slope_sg_window, 5, 41,
                         "%d samples");
        ImGui::SameLine();
        HelpMarker("Savitzky-Golay filter window size.\n"
                   "Larger = smoother but more latency.\n"
                   "Recommended: 15 samples (~17ms at 400Hz)");
        
        // Sensitivity
        ImGui::SliderFloat("Sensitivity", &engine.m_slope_sensitivity, 0.1f, 5.0f, 
                           "%.2fx");
        ImGui::SameLine();
        HelpMarker("How aggressively the wheel lightens past the grip peak.\n"
                   "Higher = more dramatic understeer feel.");
        
        // Advanced (collapsed by default)
        if (ImGui::TreeNode("Advanced Settings")) {
            ImGui::SliderFloat("Slope Threshold", &engine.m_slope_negative_threshold, 
                               -1.0f, 0.0f, "%.2f");
            ImGui::SameLine();
            HelpMarker("Slope value below which grip loss is detected.\n"
                       "Closer to 0 = more sensitive (earlier warning).");
            
            FloatSetting("Output Smoothing", &engine.m_slope_smoothing_tau,
                         0.005f, 0.1f, "%.3f s",
                         "Time constant for output smoothing to avoid flicker.");
            
            ImGui::TreePop();
        }
        
        // Live Diagnostics
        ImGui::Separator();
        ImGui::Text("Live Diagnostics:");
        ImGui::Text("  Current Slope: %.3f", engine.m_slope_current);
        ImGui::Text("  Grip Factor: %.2f%%", engine.m_slope_smoothed_output * 100.0);
        
        ImGui::Unindent();
    } else {
        ImGui::TextDisabled("Enable Slope Detection to access tuning options.");
    }
    
} else {
    ImGui::PopStyleColor();
}
```

---

## 6. Automated Tests

The following tests should be added to `tests/test_ffb_engine.cpp`:

### 6.1 Test: Slope Detection Buffer Initialization

```cpp
static void test_slope_detection_buffer_init() {
    std::cout << "\nTest: Slope Detection Buffer Initialization (v0.7.0)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    // Verify buffers start empty
    if (engine.m_slope_buffer_count == 0 && 
        engine.m_slope_buffer_index == 0 &&
        engine.m_slope_current == 0.0) {
        std::cout << "[PASS] Slope detection buffers initialized correctly." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Slope detection buffers not initialized." << std::endl;
        g_tests_failed++;
    }
}
```

### 6.2 Test: Slope Detection Derivative Calculation

```cpp
static void test_slope_sg_derivative() {
    std::cout << "\nTest: Savitzky-Golay Derivative Calculation (v0.7.0)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    // Fill buffer with linear ramp: y = x
    // Derivative should be constant = 1.0
    engine.m_slope_sg_window = 9;
    double dt = 0.01; // 100 Hz
    
    for (int i = 0; i < 20; i++) {
        engine.m_slope_lat_g_buffer[i] = (double)i * 0.1;
        engine.m_slope_slip_buffer[i] = (double)i * 0.1;
        engine.m_slope_buffer_index = (i + 1) % engine.SLOPE_BUFFER_MAX;
        engine.m_slope_buffer_count = i + 1;
    }
    
    double derivative = engine.CalculateSGDerivative(
        engine.m_slope_lat_g_buffer,
        engine.m_slope_buffer_count,
        engine.m_slope_sg_window,
        dt
    );
    
    // Linear ramp of 0.1 per sample at 100 Hz = 10 units/sec
    double expected = 10.0;
    if (std::abs(derivative - expected) < 1.0) {
        std::cout << "[PASS] SG derivative correct for linear ramp (Got: " 
                  << derivative << ", Expected: ~" << expected << ")" << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] SG derivative incorrect. Got: " << derivative 
                  << ", Expected: " << expected << std::endl;
        g_tests_failed++;
    }
}
```

### 6.3 Test: Slope Detection Grip at Peak (Zero Slope)

```cpp
static void test_slope_grip_at_peak() {
    std::cout << "\nTest: Slope Detection Grip at Peak (v0.7.0)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    
    // Simulate constant lateral G (at peak - zero derivative)
    double constant_g = 1.2;  // 1.2G
    double constant_slip = 0.05;  // 5 degrees (approx)
    double dt = 0.0025;  // 400 Hz
    
    // Fill buffer with constant values
    for (int i = 0; i < 20; i++) {
        double grip = engine.CalculateSlopeGrip(constant_g, constant_slip, dt);
    }
    
    // With zero slope (constant G vs constant slip), grip should be 1.0
    double grip = engine.m_slope_smoothed_output;
    
    if (grip > 0.95) {
        std::cout << "[PASS] Grip at peak (zero slope) is near 1.0 (Got: " 
                  << grip << ")" << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Grip at peak should be ~1.0. Got: " << grip << std::endl;
        g_tests_failed++;
    }
}
```

### 6.4 Test: Slope Detection Grip Past Peak (Negative Slope)

```cpp
static void test_slope_grip_past_peak() {
    std::cout << "\nTest: Slope Detection Grip Past Peak (v0.7.0)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    engine.m_slope_sg_window = 9;  // Smaller window for faster response
    
    double dt = 0.0025;  // 400 Hz
    
    // Simulate: Slip increasing, but Lateral G DECREASING (past peak)
    // This creates a negative slope
    for (int i = 0; i < 20; i++) {
        double slip = 0.05 + i * 0.002;  // Increasing slip
        double lat_g = 1.5 - i * 0.02;   // Decreasing G (past peak)
        engine.CalculateSlopeGrip(lat_g, slip, dt);
    }
    
    double grip = engine.m_slope_smoothed_output;
    
    // With negative slope, grip should be < 1.0
    if (grip < 0.9 && grip > 0.2) {
        std::cout << "[PASS] Grip past peak (negative slope) reduced (Got: " 
                  << grip << ")" << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Grip past peak should be reduced. Got: " << grip << std::endl;
        g_tests_failed++;
    }
}
```

### 6.5 Test: Slope Detection vs Static Threshold Comparison

```cpp
static void test_slope_vs_static_comparison() {
    std::cout << "\nTest: Slope Detection vs Static Threshold Comparison (v0.7.0)" << std::endl;
    
    // Create two engines: one with slope detection, one with static
    FFBEngine engine_slope;
    InitializeEngine(engine_slope);
    engine_slope.m_slope_detection_enabled = true;
    
    FFBEngine engine_static;
    InitializeEngine(engine_static);
    engine_static.m_slope_detection_enabled = false;
    engine_static.m_optimal_slip_angle = 0.08f;  // 4.6 degrees
    
    TelemInfoV01 data = CreateBasicTestTelemetry(30.0);  // 30 m/s
    
    // Simulate moderate understeer (slip > optimal)
    data.mWheel[0].mLateralPatchVel = 0.12 * 30.0;  // 12% lat velocity
    data.mWheel[1].mLateralPatchVel = 0.12 * 30.0;
    data.mWheel[0].mGripFract = 0.0;  // Force fallback path
    data.mWheel[1].mGripFract = 0.0;
    
    // Prime both engines
    for (int i = 0; i < 30; i++) {
        engine_slope.calculate_force(&data);
        engine_static.calculate_force(&data);
    }
    
    // Both should detect grip loss (< 1.0)
    // The exact values will differ, but both should indicate understeer
    bool slope_detected_loss = engine_slope.m_slope_smoothed_output < 0.95;
    bool static_detected_loss = engine_static.m_grip_diag.front_original < 0.8;
    
    if (slope_detected_loss && static_detected_loss) {
        std::cout << "[PASS] Both methods detect grip loss at 12% slip." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Detection mismatch. Slope: " << slope_detected_loss 
                  << ", Static: " << static_detected_loss << std::endl;
        g_tests_failed++;
    }
}
```

### 6.6 Test: Slope Detection Config Persistence

```cpp
static void test_slope_config_persistence() {
    std::cout << "\nTest: Slope Detection Config Persistence (v0.7.0)" << std::endl;
    
    const char* test_file = "tmp_slope_config_test.ini";
    
    // Create engine with non-default slope settings
    FFBEngine engine1;
    InitializeEngine(engine1);
    engine1.m_slope_detection_enabled = true;
    engine1.m_slope_sg_window = 21;
    engine1.m_slope_sensitivity = 2.5f;
    engine1.m_slope_negative_threshold = -0.05f;
    engine1.m_slope_smoothing_tau = 0.03f;
    
    // Save config
    Config::Save(engine1, test_file);
    
    // Load into new engine
    FFBEngine engine2;
    InitializeEngine(engine2);
    Config::Load(engine2, test_file);
    
    // Verify all values match
    bool all_match = true;
    all_match &= (engine2.m_slope_detection_enabled == true);
    all_match &= (engine2.m_slope_sg_window == 21);
    all_match &= (std::abs(engine2.m_slope_sensitivity - 2.5f) < 0.001f);
    all_match &= (std::abs(engine2.m_slope_negative_threshold - (-0.05f)) < 0.001f);
    all_match &= (std::abs(engine2.m_slope_smoothing_tau - 0.03f) < 0.001f);
    
    if (all_match) {
        std::cout << "[PASS] Slope detection settings saved and loaded correctly." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Slope detection config persistence failed." << std::endl;
        g_tests_failed++;
    }
    
    std::remove(test_file);
}
```

### 6.7 Test: Slope Detection Latency Characteristics

```cpp
static void test_slope_latency_characteristics() {
    std::cout << "\nTest: Slope Detection Latency (v0.7.0)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    engine.m_slope_sg_window = 15;  // 15 samples
    
    double dt = 0.0025;  // 400 Hz -> Latency = (15-1)/2 * 2.5ms = 17.5ms
    
    // Count frames to fill buffer
    int frames_to_fill = 0;
    while (engine.m_slope_buffer_count < engine.m_slope_sg_window) {
        engine.CalculateSlopeGrip(1.0, 0.05, dt);
        frames_to_fill++;
    }
    
    // Expected frames to fill = window size
    if (frames_to_fill == engine.m_slope_sg_window) {
        double latency_ms = (engine.m_slope_sg_window - 1) / 2.0 * dt * 1000.0;
        std::cout << "[PASS] Buffer fills in " << frames_to_fill << " frames. "
                  << "Group delay: " << latency_ms << " ms" << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Buffer fill count mismatch. Got: " << frames_to_fill 
                  << ", Expected: " << engine.m_slope_sg_window << std::endl;
        g_tests_failed++;
    }
}
```

### 6.8 Test: Slope Detection Noise Rejection

```cpp
static void test_slope_noise_rejection() {
    std::cout << "\nTest: Slope Detection Noise Rejection (v0.7.0)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    engine.m_slope_sg_window = 15;
    
    double dt = 0.0025;
    
    // Feed signal with noise: constant G (1.2) + random noise (±0.1)
    // Derivative of constant should be ~0 despite noise
    std::srand(42);  // Deterministic seed
    
    for (int i = 0; i < 50; i++) {
        double noise = ((double)std::rand() / RAND_MAX - 0.5) * 0.2;  // ±0.1
        double lat_g = 1.2 + noise;
        double slip = 0.05 + noise * 0.01;  // Small slip noise
        engine.CalculateSlopeGrip(lat_g, slip, dt);
    }
    
    // Slope should be near zero (constant G with noise filtered)
    if (std::abs(engine.m_slope_current) < 1.0) {  // Tolerance for noise
        std::cout << "[PASS] Noise rejected. Slope near zero: " 
                  << engine.m_slope_current << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Noise amplified. Slope: " << engine.m_slope_current << std::endl;
        g_tests_failed++;
    }
}
```

---

## 7. Testing Strategy

### 7.1 Unit Tests (Automated)

The 8 tests above should be added to the test suite, bringing the total to approximately 308+ tests.

### 7.2 Integration Tests

1. **Full FFB Loop Test**: Verify that `calculate_force()` produces correct output with Slope Detection enabled.
2. **Config Round-Trip Test**: Verify save/load preserves all settings.
3. **Preset Application Test**: Verify Slope Detection settings are applied from presets.

### 7.3 Manual Testing Checklist

| Test Case                                | Expected Behavior                                              |
|------------------------------------------|----------------------------------------------------------------|
| Enable Slope Detection                   | GUI checkbox works, static sliders are visually deprioritized  |
| Straight-line driving                    | Slope near zero, grip factor near 1.0                          |
| Light cornering (below peak)             | Positive slope, grip factor = 1.0                              |
| Heavy cornering (at peak)                | Slope transitions through zero                                 |
| Understeer (past peak)                   | Negative slope, wheel lightens                                 |
| Cold tires → Hot tires transition        | Slope adapts automatically                                     |
| Dry → Wet transition                     | Slope adapts (earlier peak detection in wet)                   |
| Window size adjustment                   | Larger window = smoother but more latency                      |
| Sensitivity adjustment                   | Higher = more dramatic understeer feel                         |

---

## 8. Migration and Backward Compatibility

### 8.1 Default Configuration

- `m_slope_detection_enabled = false` (off by default)
- Existing users continue to use static thresholds until they opt-in

### 8.2 Preset Updates

Update the built-in presets to include Slope Detection as an option:

```cpp
// T300 Preset (belt-driven, benefits from stronger cues)
preset.slope_detection_enabled = false;  // Conservative default
preset.slope_sg_window = 15;
preset.slope_sensitivity = 1.5f;  // Slightly boosted for belt-driven feel

// Direct Drive Preset (high resolution, lower latency acceptable)
preset.slope_detection_enabled = false;  // Conservative default  
preset.slope_sg_window = 9;  // Lower latency
preset.slope_sensitivity = 1.0f;  // Accurate feel
```

### 8.3 Future Deprecation Path

If Slope Detection proves superior after user testing:
1. v0.8.0: Slope Detection enabled by default
2. v0.9.0: Static threshold sliders moved to "Legacy" section
3. v1.0.0: Static threshold logic removed (or kept as hidden fallback)

---

## 9. References

1. **FFB Slope Detection for Grip Estimation.md** - Comprehensive analysis of slope detection theory
2. **FFB Slope Detection for Grip Estimation2.md** - Signal processing details and implementation considerations
3. **TODO.md Section "Optimal slip angle in real time"** - Original feature request and notes
4. Pacejka, H. B. (2006). *Tire and Vehicle Dynamics*. Butterworth-Heinemann.
5. arXiv:2509.15423 - "Online Slip Detection and Friction Coefficient Estimation for Autonomous Racing"

---

## 10. Conclusion

The Slope Detection Algorithm represents a significant upgrade to lmuFFB's grip estimation capabilities. By monitoring the real-time derivative of force vs. slip, the system can:

1. **Automatically detect peak grip** without user calibration
2. **Adapt to changing conditions** (temperature, wear, rain)
3. **Provide more accurate understeer feedback** by reacting to actual physics

The implementation adds approximately 200 lines of C++ code to `FFBEngine.h`, with supporting changes in `Config.h`, `Config.cpp`, and `GuiLayer.cpp`. A comprehensive test suite ensures correctness and regression protection.

The feature is designed to be backward-compatible, with the static threshold approach remaining as a fallback for users who prefer manual tuning.
