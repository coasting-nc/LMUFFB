# Implementation Plan - Advanced Slope Detection (Torque & Slew)

## 1. Context
Following the stabilization ([Plan 1](./Slope%20Detection%20Fixes%20&%20Telemetry%20Enhancements%20v0.7.35.md)) and accuracy tooling ([Plan 2](./Slope%20Detection%20Accuracy%20Tools.md)) of the Slope Detection feature, this phase implements advanced signal processing techniques derived from deep research into the rFactor 2 / LMU physics engine.

**Goal:**
1.  **Leading Indicator (Pneumatic Trail):** Implement a secondary slope estimator based on `Steering Torque` vs. `Steering Angle`. This detects the drop in pneumatic trail *before* the car physically slides (Lateral G saturation), providing an "anticipatory" understeer cue.
2.  **Signal Hygiene (Slew Rate Limiter):** Implement a Slew Rate Limiter on the Lateral G input to physically reject non-steering events (curb strikes, suspension jolts) from the slope calculation, preventing false positives without relying on complex surface type logic.

**Reference Documents:**
*   Deep Research Report: [`docs/dev_docs/investigations/slope detection advanced features deep research.md`](../investigations/slope%20detection%20advanced%20features%20deep%20research.md)
*   Previous Plans: [`Slope Detection Fixes & Telemetry Enhancements v0.7.35.md`](./Slope%20Detection%20Fixes%20&%20Telemetry%20Enhancements%20v0.7.35.md), [`Slope Detection Accuracy Tools.md`](./Slope%20Detection%20Accuracy%20Tools.md)

## 2. Codebase Analysis

### 2.1 Architecture Overview
*   **Physics Engine (`src/FFBEngine.h`):** Currently calculates `m_slope_current` using Lateral G. Needs expansion to handle a second parallel estimator for Torque.
*   **Configuration (`src/Config.h`):** Needs new parameters to control the Slew Limiter and the Torque Slope sensitivity.
*   **Telemetry (`src/lmu_sm_interface/InternalsPlugin.hpp`):** We already have access to `mSteeringShaftTorque` and `mUnfilteredSteering`.

### 2.2 Impacted Functionalities
*   **Slope Calculation:** Will now involve two parallel derivative pipelines (G-Slope and Torque-Slope).
*   **Grip Factor Logic:** The final `ctx.grip_factor` will be a fusion of both estimators (likely a "min" function to prioritize whichever detects loss first).

## 3. FFB Effect Impact Analysis

| Effect | Technical Impact | User Perspective |
| :--- | :--- | :--- |
| **Understeer (Anticipatory)** | **New Behavior.** The FFB will lighten up *earlier* in the cornering phase. <br> **Mechanism:** Torque Slope detects the peak of the Self-Aligning Torque (SAT) curve, which occurs at lower slip angles than the Lateral Force peak. | **Faster Reaction Time.** <br> - Users will feel the wheel go light *before* the car starts to push wide. <br> - Provides a "warning" zone rather than just a "failure" zone. |
| **Curb Rejection** | **Filtering.** The Slew Rate Limiter prevents sudden G-force spikes (curbs) from triggering the understeer effect. | **Stability.** <br> - Hitting a curb won't cause the wheel to suddenly go limp (false understeer). <br> - Slope detection remains active and accurate even on bumpy tracks. |

## 4. Proposed Changes

### 4.1 File: `src/Config.h`

**A. Update `Preset` Struct**
Add parameters for the new features.
```cpp
struct Preset {
    // ... existing slope settings ...
    
    // New: Slew Rate Limiter (G-Force per second)
    float slope_g_slew_limit = 50.0f; // 50G/s allows fast turns but kills curb spikes
    
    // New: Torque Slope Settings
    bool slope_use_torque = true;
    float slope_torque_sensitivity = 0.5f;
    
    // ... setters and validation ...
};
```
*   **Synchronization Checklist:**
    *   [ ] Add to `Preset` struct.
    *   [ ] Add to `Preset::Apply()`.
    *   [ ] Add to `Preset::UpdateFromEngine()`.
    *   [ ] Add to `Preset::Validate()`.
    *   [ ] Add to `Config::Save()` / `Config::Load()` in `Config.cpp`.

### 4.2 File: `src/FFBEngine.h`

**A. Add Internal State Members**
```cpp
private:
    // Slew Limiter State
    double m_slope_lat_g_prev = 0.0;

    // Torque Slope Buffers & State
    std::array<double, SLOPE_BUFFER_MAX> m_slope_torque_buffer = {};
    std::array<double, SLOPE_BUFFER_MAX> m_slope_steer_buffer = {};
    double m_slope_torque_smoothed = 0.0;
    double m_slope_steer_smoothed = 0.0;
    
    // Torque Slope Result
    double m_slope_torque_current = 0.0;
```

**B. Implement `apply_slew_limiter`**
Helper function to clamp rate of change.
```cpp
double apply_slew_limiter(double input, double& prev_val, double limit, double dt) {
    double delta = input - prev_val;
    double max_change = limit * dt;
    delta = std::clamp(delta, -max_change, max_change);
    prev_val += delta;
    return prev_val;
}
```

**C. Update `calculate_slope_grip`**
1.  **Apply Slew Limiter:** Run `lateral_g` through `apply_slew_limiter` *before* the Low Pass Filter.
2.  **Calculate Torque Slope:**
    *   Smooth `SteeringTorque` and `SteeringAngle`.
    *   Update buffers.
    *   Calculate derivatives (`dTorque_dt`, `dSteer_dt`).
    *   Calculate Projected Slope: `(dTorque * dSteer) / (dSteer^2 + e)`.
3.  **Fusion Logic:**
    *   Calculate `grip_loss_G` (from G-Slope).
    *   Calculate `grip_loss_Torque` (from Torque-Slope).
    *   `final_grip_loss = max(grip_loss_G, grip_loss_Torque)`. (Conservative approach: if *either* indicates loss, reduce FFB).

### 4.3 File: `src/AsyncLogger.h`

**A. Update Logging**
Add `SlopeTorque` and `SlewLimitedG` to the log frame to visualize the new features.

### 4.4 File: `VERSION` & `src/Version.h`
*   Increment version (e.g., `0.7.37` -> `0.7.38`).

## 5. Test Plan (TDD)

**New Test File:** `tests/test_ffb_advanced_slope.cpp`

### Test 1: `test_slew_rate_limiter`
*   **Goal:** Verify curb spikes are rejected.
*   **Setup:**
    *   Configure `slope_g_slew_limit = 10.0`.
    *   Input: Steady G (1.0) -> Spike (5.0) -> Steady (1.0) over 3 frames.
*   **Assertion:**
    *   The value entering the slope buffer should ramp up slowly (1.0 -> 1.1 -> 1.2), ignoring the 5.0 spike.

### Test 2: `test_torque_slope_anticipation`
*   **Goal:** Verify Torque Slope drops before G Slope.
*   **Data Flow Script:**
    *   Simulate a "Pneumatic Trail" scenario:
    *   `SteeringAngle` increases linearly.
    *   `LateralG` increases linearly (Lagging).
    *   `SteeringTorque` increases then plateaus/drops (Leading).
*   **Assertion:**
    *   `m_slope_torque_current` should become negative/zero *before* `m_slope_current` (G-based).
    *   `grip_factor` should drop as soon as Torque Slope drops.

## 6. Deliverables

*   [ ] **Code:** Updated `src/Config.h` & `src/Config.cpp` (New settings).
*   [ ] **Code:** Updated `src/FFBEngine.h` (Slew Limiter & Torque Slope logic).
*   [ ] **Tests:** New `tests/test_ffb_advanced_slope.cpp`.
*   [ ] **Docs:** Update `docs/dev_docs/implementation_plans/Slope Detection Advanced Features.md`.

## Implementation Notes

- **Initialization Robustness**: The G-Slew limiter state (`m_slope_lat_g_prev`) is now initialized on the first frame to the current telemetry value. This prevents a large positive slope artifact (derivative spike) during the very first frames of a session.
- **Torque Slope Anticipation**: The pneumatic trail detector works as expected, detecting grip loss when torque drops while lateral force is still rising. This provides a leading indicator of understeer.
- **Fusion Logic**: The conservative "Max Loss" fusion correctly prioritizes the estimator (G or Torque) that detects grip loss first.

```json
{
  "status": "success",
  "plan_path": "docs/dev_docs/implementation_plans/Slope Detection Advanced Features.md",
  "backlog_items": []
}
```