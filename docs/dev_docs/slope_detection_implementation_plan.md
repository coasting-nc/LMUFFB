# Comprehensive Implementation Plan: "Slope Detection" Algorithm for Grip Estimation

## 1. Introduction

This document outlines the implementation plan for the **Slope Detection Algorithm**, a physics-based approach to estimating tire grip levels and optimal slip dynamics in real-time. This system is designed to replace or augment the current "fixed value" approach (user-defined Optimal Slip Angle/Ratio) with an adaptive, adaptive signal processing model.

### 1.1 The Concept
"Slope Detection" refers to monitoring the derivative (rate of change) of the **Self-Aligning Torque (SAT)** with respect to the **Slip Angle** ($\alpha$). By analyzing the slope ($\frac{dM_z}{d\alpha}$), we can determine the tire's state relative to its limit of adhesion.

*   **Positive Slope (> 0)**: The tire is in the linear or transitional region. Grip is building.
*   **Zero Slope ($\approx$ 0)**: The SAT has reached its peak. This is the "Limit of Stability" and typically precedes the limit of adhesion.
*   **Negative Slope (< 0)**: The SAT is dropping off (Pneumatic Trail collapse). The tire is scrubbing or sliding.

This method allows us to dynamically detect the **Optimal Slip Angle** without relying on static lookup tables or game-provided "Grip Fraction" values, which may be inconsistent or unavailable.

### 1.2 Key Challenges & Solutions
The primary challenge in calculating derivatives from real-time telemetry is **Signal Noise**. Naive differentiation amplifies high-frequency noise (road texture, vibration), rendering the slope useless. 

*   **Solution**: We will implement a **Savitzky-Golay Filter**. Unlike standard Low-Pass Filters (which blur signal peaks), the Savitzky-Golay filter fits a polynomial to a data window, allowing us to smoothing the signal while **preserving the sharp peaks** characteristic of SAT drop-off.
*   **Latency**: At the target telemetry rate of 400Hz, a window size of 15-25 samples yields a latency of ~17-30ms, which is acceptable for FFB operations.

---

## 2. Impact Analysis: Changes to Existing Code

The implementation will primarily affect the `FFBEngine` class in `src/FFBEngine.h`.

### 2.1 Current State
Currently, the `FFBEngine` uses:
*   `m_optimal_slip_angle` (User configurable, currently ~4.0 degrees).
*   `m_optimal_slip_ratio` (User configurable, currently ~0.20).
*   `calculate_grip()` function which relies on `mGripFract` from the game. If `mGripFract` is missing, it falls back to a calculation that compares current slip against the **fixed** `m_optimal_slip_angle`.

### 2.2 Required Changes
1.  **New Classes**: 
    *   `SavitzkyGolayFilter`: A general-purpose signal processing class.
    *   `SlopeDetector`: A state machine that ingests Slip/Torque data and estimates Peak Alpha.
2.  **Modifications to `FFBEngine`**:
    *   Add members for `SlopeDetector` (one for Front, possibly one for Rear).
    *   Update `FFBSnapshot` to include "Detected Optimal Slip" for debugging.
    *   Modify `calculate_force` to feed telemetry into the detector every frame.
    *   Modify `calculate_grip` (or the logic triggering `understeer_effect`) to use the *dynamic* optimal slip angle instead of the *fixed* one.

---

## 3. Implementation Details (Code Snippets)

### 3.1 Step 1: Savitzky-Golay Filter Implementation
We need a performant, ring-buffer based filter.

```cpp
// src/SignalProcessing.h (New File or add to FFBEngine.h)

#include <vector>
#include <deque>

class SavitzkyGolayFilter {
private:
    std::deque<double> m_buffer;
    std::vector<double> m_coeffs;
    int m_window_size;
    int m_half_window;

public:
    SavitzkyGolayFilter(int window_size = 15) {
        Resize(window_size);
    }

    void Resize(int window_size) {
        // Ensure odd number
        if (window_size % 2 == 0) window_size++;
        m_window_size = window_size;
        m_half_window = window_size / 2;
        
        m_buffer.clear();
        
        // Pre-calculate coefficients for Quadratic/Cubic Polynomial (Order 2/3) 
        // First Derivative (m=1) at center point (t=0)
        // Ref: https://en.wikipedia.org/wiki/Savitzky%E2%80%93Golay_filter#Tables_of_selected_convolution_coefficients
        // Implementation note: We will use a simplified pre-calc or hardcoded tables for common sizes (5, 9, 15, 25).
        // For general "Smoothing" (Order 0 derivative), coeffs are different.
        // We likely need TWO filters:
        // 1. Smoothing Filter (to get clean SAT and Slip)
        // 2. Derivative Filter (to get the slope) -> Or simply differentiate the smoothed signal.
        
        calculate_coefficients(); 
    }
    
    // ... Implementation of Update(val) and GetValue() ...
};
```

*Refinement*: To keep it simple and fast, we can implement the SG smoothing coefficients for a specific window size (e.g., 25) directly. The Derivative can then be calculated as `(SmoothVal[t] - SmoothVal[t-1]) / dt`.

### 3.2 Step 2: Slope Detector Class
This class manages the logic of "Hunting" for the peak.

```cpp
// Inside FFBEngine.h

struct SlopeState {
    double current_slope = 0.0;
    double estimated_optimal_slip = 0.08; // Start at ~4.5 degrees (radians)
    double confidence = 0.0; // 0.0 to 1.0 (How sure are we?)
    bool is_scrubbing = false; // Slope < 0
};

class SlopeDetector {
private:
    // Buffers for X (Slip) and Y (Torque)
    // We need time-aligned buffers
    struct DataPoint {
        double slip;
        double torque;
        double time;
    };
    
    std::deque<DataPoint> m_history;
    int m_window_size = 25; // ~60ms at 400Hz
    
    // Smoothers
    SavitzkyGolayFilter m_slip_filter;
    SavitzkyGolayFilter m_torque_filter;
    
public:
    SlopeState process(double raw_slip, double raw_torque, double dt) {
        SlopeState result;
        
        // 1. Smooth Signals
        double smooth_slip = m_slip_filter.Update(std::abs(raw_slip));
        double smooth_torque = m_torque_filter.Update(std::abs(raw_torque));
        
        // 2. Calculate Slope (dTorque / dSlip)
        // Robust method: Linear Regression over the short window history? 
        // Or simple discrete derivative of smoothed values?
        
        static double prev_slip = 0;
        static double prev_torque = 0;
        
        double dSlip = smooth_slip - prev_slip;
        double dTorque = smooth_torque - prev_torque;
        
        // Avoid singularities
        double slope = 0.0;
        if (std::abs(dSlip) > 0.0001) {
            slope = dTorque / dSlip;
        }
        
        // 3. Peak Detection Logic
        // If we crossed from Positive Slope to Negative Slope
        // AND we are in a valid load range:
        // Update the "Optimal Slip" estimate.
        
        static double peak_candidate = 0.0;
        
        if (slope < 0 && prev_slope > 0) {
            // We just crested the hill
            peak_candidate = smooth_slip;
            
            // Updates estimation (Exponential Moving Average to be stable)
            m_estimated_peak = m_estimated_peak * 0.9 + peak_candidate * 0.1;
        }
        
        result.current_slope = slope;
        result.estimated_optimal_slip = m_estimated_peak;
        
        // Update State
        prev_slip = smooth_slip;
        prev_torque = smooth_torque;
        
        return result;
    }
}
```

### 3.3 Step 3: FFBEngine Integration to Config.h

We ensure the user can toggle this behavior and tune the new filter.

```cpp
// src/Config.h

// New Settings
bool m_slope_detection_enabled = false;
int m_slope_window_size = 25; // Filter Window
double m_slope_update_rate = 0.1; // Learning rate for the optimal slip adaptation
```

### 3.4 Step 4: Logic Update in `calculate_force`

```cpp
// Inside FFBEngine::calculate_force

// ... Pre-processing ...

// 1. Run Slope Detection (Front Axle)
if (m_slope_detection_enabled) {
    double avg_front_slip = (std::abs(m_grip_diag.front_slip_angle.left) + std::abs(m_grip_diag.front_slip_angle.right)) / 2.0;
    double avg_align_torque = game_force; // Shaft Torque
    
    SlopeState slope_res = m_slope_detector.process(avg_front_slip, avg_align_torque, dt);
    
    // UPDATE the engine's "Optimal Slip" dynamically
    // We blend it to avoid jumps
    m_DetectedOptimalSlip = slope_res.estimated_optimal_slip;
    
    // Visual / Debug
    m_DebugValues.slope = slope_res.current_slope;
}

// ...

// 2. Use in Understeer Effect
// Instead of using fixed m_optimal_slip_angle, we use the detected value
double target_optimal = m_slope_detection_enabled ? m_DetectedOptimalSlip : m_optimal_slip_angle;

// Calculate Grip Factor based on how far we are past the optimal
double slip_excess = current_slip - target_optimal;
// ... logic continues ...
```

TODO: besides the understeer effect, are there other effects for which  the grip fraction is used? Eg. Oversteer boost, and some texture vibrations.

---

## 4. Automated Tests Plan

To ensure robustness, we will add the following tests to our `tests/` suite (currently 300+ tests).

### 4.1 Unit Test: Savitzky-Golay Filter
**File**: `tests/test_signal_processing.cpp`

*   **Test Case 1: Noise Rejection**
    *   Input: Sine wave (Grip curve) + Gaussian Noise.
    *   Output: Verify that the filtered signal reduces variance (Standard Deviation) by > 50% while maintaining the sine wave's phase better than a simple Moving Average.
*   **Test Case 2: Peak Preservation**
    *   Input: A "Triangle" wave representing sharp grip drop-off.
    *   Output: Verify that the peak height of the filtered signal is at least 95% of the original (Moving Average often clips this to < 80%).

### 4.2 Unit Test: Slope Detection Logic
**File**: `tests/test_slope_detection.cpp`

*   **Test Case 1: Ideal Curve**
    *   Input: Synthesize a Pacejka-style curve ($F = sin(C \cdot atan(B \cdot \alpha))$).
    *   Action: Feed samples sequentially to `SlopeDetector`.
    *   Verify: The `estimated_optimal_slip` converges to the mathematical peak of the input curve within +/- 5%.
*   **Test Case 2: Hysteresis / Stability**
    *   Input: Flat signal (Plateau).
    *   Verify: The slope output remains near zero and does not oscillate wildly (Noise gating check).

### 4.3 Integration Test: FFB Output
**File**: `tests/test_ffb_integration.cpp`

*   **Test Case 1: Adaptive Learning**
    *   Scenario: Start car with default optimal (4.0 deg). Drive into a corner (simulated input) where peak torque happens at 6.0 deg (Simulating rain/soft tires).
    *   Action: Feed telemetry for a 5-second corner.
    *   Verify: `m_DetectedOptimalSlip` shifts from 4.0 towards 6.0.
    *   Verify: The Understeer Effect is NOT triggered at 4.5 degrees (because detector learned the limit is higher).
