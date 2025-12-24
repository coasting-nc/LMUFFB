# Technical Specification: Advanced Grip Estimation & Signal Smoothing

**Target Version:** v0.5.7
**Priority:** High (Physics Tuning & Signal Quality)
**Context:** Le Mans Ultimate Force Feedback (LMUFFB)

## 1. Introduction & Objectives

This document outlines the implementation plan for enhancing the physics customization and signal quality of the LMUFFB application. Currently, the application uses hardcoded constants for tire grip estimation (Optimal Slip Angle/Ratio), which assumes a "one-size-fits-all" physics model. However, Le Mans Ultimate features diverse vehicle classes (GT3 vs. Hypercars) that require distinct tire modeling parameters to produce accurate Force Feedback.

Additionally, users have reported "graininess" in the raw game signal (`mSteeringShaftTorque`). While we have smoothing for derived effects (SoP, Slip Angle), we lack a dedicated filter for the raw input signal itself.

**The objectives of this update are:**
1.  **Parameterize Grip Estimation:** Expose "Optimal Slip Angle" and "Optimal Slip Ratio" as user-configurable sliders to allow tuning for specific car classes (e.g., stiff Prototypes vs. compliant GT cars).
2.  **Implement Input Smoothing:** Add a dedicated Low Pass Filter (LPF) for the raw `mSteeringShaftTorque` to improve signal quality on lower-end hardware.
3.  **Enhance Documentation:** Clarify the usage of LMU 1.2 API specific fields within the codebase.

---

## 2. Technical Analysis

### A. Optimal Slip Angle & Ratio Logic

The `calculate_grip` function currently derives a "Grip Fraction" based on hardcoded thresholds (0.10 rad for lateral, 0.12 for longitudinal). By making these variables, we affect specific downstream FFB components while leaving others untouched.

**Components Affected by these Settings:**
*   **Understeer Effect (Grip Modulation):** This is the primary consumer. Lowering the Optimal Slip Angle makes the steering go light *earlier* in a corner (simulating a stiffer tire peak).
*   **Oversteer Boost:** Depends on the delta between Front and Rear grip. Changing the threshold alters the point at which the system detects the front axle has more grip than the rear.
*   **Slide Texture (Amplitude):** The "Work-Based Scrubbing" logic scales vibration amplitude based on `(1.0 - Grip)`. Lowering the threshold makes scrubbing vibrations appear earlier.

**Components NOT Affected (Independent):**
*   **Base Force (Raw):** The raw game force magnitude is not changed, only the modulation applied to it.
*   **SoP (Lateral G):** Derived purely from accelerometer data.
*   **Yaw Kick:** Derived purely from rotational acceleration.
*   **Rear Aligning Torque:** Uses the *calculated slip angle* (kinematic input), not the *grip fraction* (output).
*   **Lockup / Spin Vibrations:** Use internal hardcoded thresholds (e.g., `-0.1` slip ratio).

####  **Optimal Slip Ratio**

Based on the code analysis, here is the breakdown of what the **Optimal Slip Ratio** setting affects.

##### What it Affects (Grip Calculation)
This setting controls the **"Combined Friction Circle"** logic. It determines how much **Longitudinal Slip** (Braking or Acceleration) contributes to the calculated "Grip Loss."

1.  **Understeer Effect (Steering Weight during Braking/Accel):**
    *   **How:** If you lower this value (e.g., to 0.10), the system thinks you have lost grip earlier when you brake hard.
    *   **Result:** The steering wheel will **go light (lose weight)** more aggressively during heavy braking (lockups) or strong acceleration (wheelspin), simulating the loss of front-end bite.
2.  **Slide Texture (Amplitude/Strength):**
    *   **How:** The "Work-Based Scrubbing" logic scales the vibration strength based on `(1.0 - Grip)`.
    *   **Result:** If you lower this value, the "Scrubbing" vibration will become **stronger** during braking and acceleration events, because the system calculates a lower grip value.

##### What it Does NOT Affect (Independent Effects)
Crucially, this setting does **not** change the trigger points for the specific "Haptic Vibration" effects, which use their own internal hardcoded thresholds.

1.  **Lockup Vibration (The "Rumble"):**
    *   **Reason:** This effect has a hardcoded trigger of **-0.1** (10% slip). Changing the "Optimal Slip Ratio" slider will **not** change when the lockup rumble starts.
2.  **Wheel Spin Vibration (The "Revving" Vibe):**
    *   **Reason:** This effect has a hardcoded trigger of **0.2** (20% slip). It is independent of the general grip calculation.
3.  **Cornering Feel (Pure Lateral):**
    *   **Reason:** Pure cornering is determined by **Slip Angle**, not Slip Ratio. If you are coasting through a turn, this setting does nothing.

##### Summary for Tooltip
*   **Optimal Slip Ratio:** Controls how much braking/acceleration contributes to the "Lightening" of the steering wheel.
    *   **Lower:** Steering goes light earlier under braking.
    *   **Higher:** Steering stays heavy longer under braking.
    *   *Note:* Does not change the trigger point for Lockup/Spin vibrations.

### B. Steering Shaft Torque Rationale

We are introducing a smoothing filter specifically for `mSteeringShaftTorque`.

**Documentation Requirement:**
The code must explicitly document why `mSteeringShaftTorque` is used over `mSteeringArmForce`.
*   *Reasoning:* In the legacy rFactor 2 engine, `mSteeringArmForce` (Newtons) measured force on tie rods. In Le Mans Ultimate (v1.2+), especially for Hypercars, the physics engine often reports `0.0` or inaccurate values for this field due to complex power steering simulation. `mSteeringShaftTorque` (Newton-meters) represents the final torque at the column, accounting for mechanical trail, pneumatic trail, and power assist.

---

## 3. Implementation Guide

### Step 1: Core Physics Engine (`FFBEngine.h`)

**Action:** Add member variables for the new settings, implement the smoothing logic using a Time-Corrected LPF, and update the grip calculation to use the variables instead of constants.

```cpp
class FFBEngine {
public:
    // ... existing settings ...
    
    // NEW: Grip Estimation Settings
    float m_optimal_slip_angle = 0.10f; // Default 0.10 rad (5.7 deg)
    float m_optimal_slip_ratio = 0.12f; // Default 0.12 (12%)
    
    // NEW: Steering Shaft Smoothing
    float m_steering_shaft_smoothing = 0.0f; // Time constant in seconds (0.0 = off)
    
    // Internal state for smoothing
    double m_steering_shaft_torque_smoothed = 0.0;

    // ... existing code ...

    // Update calculate_grip to use the variables
    GripResult calculate_grip(const TelemWheelV01& w1, 
                              const TelemWheelV01& w2,
                              double avg_load,
                              bool& warned_flag,
                              double& prev_slip1,
                              double& prev_slip2,
                              double car_speed,
                              double dt) {
        // ... [Existing setup code] ...

        // Fallback condition
        if (result.value < 0.0001 && avg_load > 100.0) {
            result.approximated = true;
            
            if (car_speed < 5.0) {
                result.value = 1.0; 
            } else {
                // v0.4.38: Combined Friction Circle
                
                // 1. Lateral Component (Alpha)
                // USE CONFIGURABLE THRESHOLD
                double lat_metric = std::abs(result.slip_angle) / (double)m_optimal_slip_angle;

                // 2. Longitudinal Component (Kappa)
                double ratio1 = calculate_manual_slip_ratio(w1, car_speed);
                double ratio2 = calculate_manual_slip_ratio(w2, car_speed);
                double avg_ratio = (std::abs(ratio1) + std::abs(ratio2)) / 2.0;
                
                // USE CONFIGURABLE THRESHOLD
                double long_metric = avg_ratio / (double)m_optimal_slip_ratio;

                // ... [Rest of calculation] ...
            }
            // ...
        }
        // ...
        return result;
    }

    double calculate_force(const TelemInfoV01* data) {
        // ... [Setup] ...

        // Critical: Use mSteeringShaftTorque instead of mSteeringArmForce
        // Explanation: LMU 1.2 introduced mSteeringShaftTorque (Nm) as the definitive FFB output.
        // Legacy mSteeringArmForce (N) is often 0.0 or inaccurate for Hypercars due to 
        // complex power steering modeling in the new engine.
        double game_force = data->mSteeringShaftTorque;

        // --- NEW: Steering Shaft Smoothing ---
        if (m_steering_shaft_smoothing > 0.0001f) {
            double tau = (double)m_steering_shaft_smoothing;
            double alpha = dt / (tau + dt);
            // Safety clamp
            alpha = (std::min)(1.0, (std::max)(0.001, alpha));
            
            m_steering_shaft_torque_smoothed += alpha * (game_force - m_steering_shaft_torque_smoothed);
            game_force = m_steering_shaft_torque_smoothed;
        } else {
            m_steering_shaft_torque_smoothed = game_force; // Reset state
        }

        // ... [Rest of calculate_force] ...
    }
};
```

### Step 2: Configuration Structure (`src/Config.h`)

**Action:** Update the `Preset` struct to include the new fields, setters, and update logic.

```cpp
struct Preset {
    // ... existing members ...
    
    // NEW Settings
    float optimal_slip_angle = 0.10f;
    float optimal_slip_ratio = 0.12f;
    float steering_shaft_smoothing = 0.0f;

    // ... existing setters ...

    // NEW Setters
    Preset& SetOptimalSlip(float angle, float ratio) { 
        optimal_slip_angle = angle; 
        optimal_slip_ratio = ratio; 
        return *this; 
    }
    Preset& SetShaftSmoothing(float v) { steering_shaft_smoothing = v; return *this; }

    void Apply(FFBEngine& engine) const {
        // ... existing apply ...
        engine.m_optimal_slip_angle = optimal_slip_angle;
        engine.m_optimal_slip_ratio = optimal_slip_ratio;
        engine.m_steering_shaft_smoothing = steering_shaft_smoothing;
    }

    void UpdateFromEngine(const FFBEngine& engine) {
        // ... existing update ...
        optimal_slip_angle = engine.m_optimal_slip_angle;
        optimal_slip_ratio = engine.m_optimal_slip_ratio;
        steering_shaft_smoothing = engine.m_steering_shaft_smoothing;
    }
};
```

### Step 3: Persistence (`src/Config.cpp`)

**Action:** Update `Save` and `Load` methods to persist the new keys to `config.ini`.

```cpp
void Config::Save(const FFBEngine& engine, const std::string& filename) {
    // ...
    file << "optimal_slip_angle=" << engine.m_optimal_slip_angle << "\n";
    file << "optimal_slip_ratio=" << engine.m_optimal_slip_ratio << "\n";
    file << "steering_shaft_smoothing=" << engine.m_steering_shaft_smoothing << "\n";
    // ...
}

void Config::Load(FFBEngine& engine, const std::string& filename) {
    // ...
    // Inside loop
    else if (key == "optimal_slip_angle") engine.m_optimal_slip_angle = std::stof(value);
    else if (key == "optimal_slip_ratio") engine.m_optimal_slip_ratio = std::stof(value);
    else if (key == "steering_shaft_smoothing") engine.m_steering_shaft_smoothing = std::stof(value);
    // ...
}
```

### Step 4: User Interface (`src/GuiLayer.cpp`)

**Action:**
1.  Add "Steering Shaft Smooth" slider under "Front Axle". Include latency calculation display.
2.  Add "Optimal Slip Angle" and "Optimal Slip Ratio" sliders under "Grip & Slip Angle Estimation".

```cpp
// Inside DrawTuningWindow...

    // --- GROUP: FRONT AXLE ---
    if (ImGui::TreeNodeEx("Front Axle (Understeer)", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        ImGui::NextColumn(); ImGui::NextColumn();
        
        FloatSetting("Steering Shaft Gain", &engine.m_steering_shaft_gain, 0.0f, 1.0f, FormatPct(engine.m_steering_shaft_gain), "Attenuates raw game force without affecting telemetry.");
        
        // --- NEW: Steering Shaft Smoothing ---
        ImGui::Text("Steering Shaft Smooth");
        ImGui::NextColumn();
        
        int shaft_ms = (int)(engine.m_steering_shaft_smoothing * 1000.0f + 0.5f);
        ImVec4 shaft_color = (shaft_ms < 15) ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
        ImGui::TextColored(shaft_color, "Latency: %d ms - %s", shaft_ms, (shaft_ms < 15) ? "OK" : "High");
        
        ImGui::SetNextItemWidth(-1);
        if (ImGui::SliderFloat("##ShaftSmooth", &engine.m_steering_shaft_smoothing, 0.000f, 0.100f, "%.3f s")) selected_preset = -1;
        if (ImGui::IsItemHovered()) {
             ImGui::SetTooltip("Low Pass Filter applied ONLY to the raw game force.\nUse this to smooth out 'grainy' FFB from the game engine.\nWarning: Adds latency.");
        }
        ImGui::NextColumn();
        // -------------------------------------

        FloatSetting("Understeer Effect", &engine.m_understeer_effect, 0.0f, 50.0f, "%.2f", "Strength of the force drop when front grip is lost.");
        
        // ... existing code ...
    }

    // ...

    // --- GROUP: PHYSICS ---
    if (ImGui::TreeNodeEx("Grip & Slip Angle Estimation", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
        ImGui::NextColumn(); ImGui::NextColumn();
        
        // ... [Existing Slip Angle Smoothing Slider] ...

        // --- NEW: Optimal Slip Sliders ---
        FloatSetting("Optimal Slip Angle", &engine.m_optimal_slip_angle, 0.05f, 0.20f, "%.2f rad", 
            "The slip angle where peak grip occurs.\n"
            "Lower = Earlier understeer warning (Hypercars ~0.06).\n"
            "Higher = Later warning (GT3 ~0.10).\n"
            "Affects: Understeer Effect, Oversteer Boost, Slide Texture.");

        FloatSetting("Optimal Slip Ratio", &engine.m_optimal_slip_ratio, 0.05f, 0.20f, "%.2f", 
            "The longitudinal slip ratio (braking/accel) where peak grip occurs.\n"
            "Typical: 0.12 - 0.15 (12-15%).\n"
            "Affects: How much braking/acceleration contributes to calculated grip loss.");
        // ---------------------------------

        BoolSetting("Manual Slip Calc", &engine.m_use_manual_slip, "Uses local velocity instead of game's slip telemetry.");
        
        ImGui::TreePop();
    }
```

---

## 4. Comprehensive Testing Strategy

To ensure the new physics parameters function correctly and do not introduce regressions, the following unit tests must be implemented in `tests/test_ffb_engine.cpp`.

### Test A: Optimal Slip Angle Sensitivity
**Goal:** Verify that lowering the "Optimal Slip Angle" threshold causes the Understeer effect to trigger *earlier* for the same physical slip.

**Logic:**
1.  Set up a fixed lateral slip scenario (e.g., Slip Angle = 0.08 rad).
2.  **Run 1 (Default 0.10 rad):** Since $0.08 < 0.10$, the tire is within the grip limit. Calculated Grip should be high (~1.0).
3.  **Run 2 (Hypercar 0.06 rad):** Since $0.08 > 0.06$, the tire is past the limit. Calculated Grip should drop significantly.
4.  **Assert:** `Grip_Run2 < Grip_Run1`.

```cpp
static void test_optimal_slip_angle_sensitivity() {
    std::cout << "\nTest: Optimal Slip Angle Sensitivity" << std::endl;
    FFBEngine engine;
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    // Setup: Fixed Slip Angle of ~0.08 rad
    // atan(1.6 / 20.0) ~= 0.08
    data.mWheel[0].mLateralPatchVel = 1.6; 
    data.mWheel[1].mLateralPatchVel = 1.6;
    data.mWheel[0].mLongitudinalGroundVel = 20.0;
    data.mWheel[1].mLongitudinalGroundVel = 20.0;
    
    // Force fallback logic to use approximation
    data.mWheel[0].mGripFract = 0.0; 
    data.mWheel[1].mGripFract = 0.0;
    data.mWheel[0].mTireLoad = 4000.0;
    data.mWheel[1].mTireLoad = 4000.0;
    data.mLocalVel.z = 20.0;

    // Case 1: Default Threshold (0.10) -> 0.08 is SAFE
    engine.m_optimal_slip_angle = 0.10f;
    engine.calculate_force(&data);
    float grip_relaxed = engine.GetDebugBatch().back().calc_front_grip;

    // Case 2: Strict Threshold (0.06) -> 0.08 is SLIDING
    engine.m_optimal_slip_angle = 0.06f;
    engine.calculate_force(&data);
    float grip_strict = engine.GetDebugBatch().back().calc_front_grip;

    if (grip_strict < grip_relaxed) {
        std::cout << "[PASS] Lowering threshold reduced grip (" << grip_relaxed << " -> " << grip_strict << ")" << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Threshold change had no effect." << std::endl;
        g_tests_failed++;
    }
}
```

### Test B: Steering Shaft Smoothing (Step Response)
**Goal:** Verify that the new smoothing filter applies lag to the raw game force input.

**Logic:**
1.  Inject a step change in `mSteeringShaftTorque` (0 -> 10 Nm).
2.  **Run 1 (Smoothing 0.0):** Output should be 10 Nm immediately (Instant).
3.  **Run 2 (Smoothing 0.1s):** Output should be significantly less than 10 Nm on the first frame (Lag).

```cpp
static void test_steering_shaft_smoothing() {
    std::cout << "\nTest: Steering Shaft Smoothing Step Response" << std::endl;
    FFBEngine engine;
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    data.mSteeringShaftTorque = 10.0;
    data.mDeltaTime = 0.01; // 100Hz
    
    // Case 1: No Smoothing
    engine.m_steering_shaft_smoothing = 0.0f;
    engine.calculate_force(&data); // Reset state
    double force_raw = engine.GetDebugBatch().back().base_force;
    
    // Case 2: Heavy Smoothing (0.1s)
    engine.m_steering_shaft_smoothing = 0.1f;
    // Reset internal state by running once with 0 input or re-instantiating
    FFBEngine engine_smooth;
    engine_smooth.m_steering_shaft_smoothing = 0.1f;
    engine_smooth.calculate_force(&data);
    double force_smooth = engine_smooth.GetDebugBatch().back().base_force;

    // Raw should be ~10.0 (normalized). Smooth should be ~1.0 (alpha ~0.1).
    if (std::abs(force_raw) > std::abs(force_smooth) * 2.0) {
        std::cout << "[PASS] Smoothing applied lag (Raw: " << force_raw << ", Smooth: " << force_smooth << ")" << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Smoothing ineffective." << std::endl;
        g_tests_failed++;
    }
}
```

### Test C: Config Persistence
**Goal:** Verify the new parameters are saved and loaded correctly.

```cpp
static void test_new_config_params() {
    std::cout << "\nTest: Config Persistence (New Params)" << std::endl;
    FFBEngine e1, e2;
    
    e1.m_optimal_slip_angle = 0.123f;
    e1.m_optimal_slip_ratio = 0.456f;
    e1.m_steering_shaft_smoothing = 0.099f;
    
    Config::Save(e1, "test_config_new.ini");
    Config::Load(e2, "test_config_new.ini");
    
    bool match = true;
    if (std::abs(e2.m_optimal_slip_angle - 0.123f) > 0.001) match = false;
    if (std::abs(e2.m_optimal_slip_ratio - 0.456f) > 0.001) match = false;
    if (std::abs(e2.m_steering_shaft_smoothing - 0.099f) > 0.001) match = false;
    
    if (match) {
        std::cout << "[PASS] New parameters persisted." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Persistence mismatch." << std::endl;
        g_tests_failed++;
    }
    std::remove("test_config_new.ini");
}
```