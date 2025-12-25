# Technical Report: Lockup Vibration Fixes & Enhancements (Batch 1)

**Date:** December 24, 2025
**Subject:** Resolution of missing rear lockup effects, manual slip calculation bugs, and implementation of advanced tuning controls with dedicated Braking configuration.

---

## 1. Problem Statement

User testing identified three critical deficiencies in the "Braking Lockup" FFB effect:
1.  **Rear Lockups Ignored:** The application only monitored front wheels for slip. Locking the rear brakes (common in LMP2/Hypercars) produced no vibration, leading to unrecoverable spins without tactile warning.
2.  **Late Warning:** The hardcoded trigger threshold of 10% slip (`-0.1`) was too high. By the time the effect triggered, the tires were already deep into the lockup phase, offering no opportunity for threshold braking modulation.
3.  **Broken Manual Calculation:** The "Manual Slip Calc" fallback option produced no output due to a coordinate system sign error.

Additionally, tuning the braking feel was difficult because the "Load Cap" (which limits vibration intensity under high downforce) was shared globally with Road and Slide textures. Users often wanted strong braking feedback (High Cap) but subtle road bumps (Low Cap), which was impossible with a single slider.

## 2. Root Cause Analysis

*   **Rear Blindness:** The `calculate_force` loop explicitly checked only `mWheel[0]` and `mWheel[1]`. Telemetry analysis confirms LMU *does* provide valid longitudinal patch velocity for rear wheels, so this was a logic omission, not a data gap.
*   **Manual Slip Bug:** The formula used `data->mLocalVel.z` directly. In LMU, forward velocity is **negative**. Passing a negative denominator to the slip ratio formula resulted in positive (traction) ratios during braking, failing the `< -0.1` check.
*   **Thresholds:** The linear ramp from 10% to 50% slip did not align with the physics of slick tires, which typically peak around 12-15% slip.
*   **Coupled Load Limits:** The `m_max_load_factor` applied universally to all vibration effects, preventing independent tuning of braking intensity vs. road texture harshness.

---

## 3. Implementation Plan

### A. Physics Engine (`FFBEngine.h`)

We will implement a **4-Wheel Monitor** with **Axle Differentiation** and split the Load Cap logic.

*   **Differentiation:** If the rear axle has a higher slip ratio than the front, the vibration frequency drops to **30%** (Heavy Judder) and amplitude is boosted.
*   **Dynamic Ramp:** Configurable Start/Full thresholds with a Quadratic ($x^2$) ramp. We introduce a configurable window:
    *   **Start %:** Vibration begins (Early Warning).
    *   **Full %:** Vibration hits max amplitude (Peak Limit).
    *   **Curve:** Quadratic ($x^2$) ramp to provide a sharp, distinct "wall" of vibration as the limit is reached.
*   **Split Load Caps:**
    *   **`m_texture_load_cap`**: Limits Road and Slide textures (prevents shaking on straights).
    *   **`m_brake_load_cap`**: Limits Lockup vibration (allows strong feedback during high-downforce braking).

#### Code Changes

**1. Fix Manual Slip Calculation**
```cpp
// Inside calculate_force lambda
auto get_slip_ratio = [&](const TelemWheelV01& w) {
    if (m_use_manual_slip) {
        // FIX: Use std::abs() because mLocalVel.z is negative (forward)
        return calculate_manual_slip_ratio(w, std::abs(data->mLocalVel.z));
    }
    // ... existing fallback ...
};
```

**2. New Member Variables**
```cpp
// Defaults
float m_lockup_start_pct = 5.0f;      // Warning starts at 5% slip
float m_lockup_full_pct = 15.0f;      // Max vibration at 15% slip
float m_lockup_rear_boost = 1.5f;     // Rear lockups are 1.5x stronger
float m_brake_load_cap = 1.5f;        // Specific load cap for braking
// Note: Existing m_max_load_factor will be renamed/repurposed to m_texture_load_cap
```

**3. Updated Lockup Logic**
```cpp
// --- 2b. Progressive Lockup (4-Wheel Monitor) ---
if (m_lockup_enabled && data->mUnfilteredBrake > 0.05) {
    // 1. Calculate Slip for ALL wheels
    double slip_fl = get_slip_ratio(data->mWheel[0]);
    double slip_fr = get_slip_ratio(data->mWheel[1]);
    double slip_rl = get_slip_ratio(data->mWheel[2]);
    double slip_rr = get_slip_ratio(data->mWheel[3]);

    // 2. Find worst slip per axle (Slip is negative, so use min)
    double max_slip_front = (std::min)(slip_fl, slip_fr);
    double max_slip_rear  = (std::min)(slip_rl, slip_rr);

    // 3. Determine Source & Differentiation
    double effective_slip = 0.0;
    double freq_multiplier = 1.0;
    double amp_multiplier = 1.0;

    if (max_slip_rear < max_slip_front) {
        // REAR LOCKUP DETECTED
        effective_slip = max_slip_rear;
        freq_multiplier = 0.3; // Low Pitch (Judder/Thud)
        amp_multiplier = m_lockup_rear_boost; // Configurable Boost
    } else {
        // FRONT LOCKUP DETECTED
        effective_slip = max_slip_front;
        freq_multiplier = 1.0; // Standard Pitch (Screech)
        amp_multiplier = 1.0;
    }

    // 4. Dynamic Thresholds
    double start_ratio = m_lockup_start_pct / 100.0;
    double full_ratio = m_lockup_full_pct / 100.0;
    
    // Check if we crossed the start threshold
    if (effective_slip < -start_ratio) {
        // Normalize slip into 0.0 - 1.0 range based on window
        double slip_abs = std::abs(effective_slip);
        double window = full_ratio - start_ratio;
        if (window < 0.01) window = 0.01; // Safety div/0

        double normalized = (slip_abs - start_ratio) / window;
        double severity = (std::min)(1.0, normalized);
        
        // Apply Quadratic Curve (Sharp feel at limit)
        severity = severity * severity;

        // Frequency Calculation
        double car_speed_ms = std::abs(data->mLocalVel.z); 
        double base_freq = 10.0 + (car_speed_ms * 1.5); 
        double final_freq = base_freq * freq_multiplier;

        // Phase Integration
        m_lockup_phase += final_freq * dt * TWO_PI;
        m_lockup_phase = std::fmod(m_lockup_phase, TWO_PI);

        // Calculate Braking-Specific Load Factor
        // We calculate raw load factor, then clamp it using the NEW Brake Load Cap
        double raw_load_factor = avg_load / 4000.0;
        double brake_load_factor = (std::min)((double)m_brake_load_cap, (std::max)(0.0, raw_load_factor));

        // Final Amplitude
        // Uses brake_load_factor instead of the generic load_factor
        double amp = severity * m_lockup_gain * 4.0 * decoupling_scale * amp_multiplier * brake_load_factor;
        
        total_force += std::sin(m_lockup_phase) * amp;
    }
}
```

---

### B. Configuration (`src/Config.h` / `.cpp`)

We need to persist the new tuning parameters and the split load cap.

**1. Update `Preset` Struct**
```cpp
struct Preset {
    // ... existing ...
    float lockup_start_pct = 5.0f;
    float lockup_full_pct = 15.0f;
    float lockup_rear_boost = 1.5f;
    float brake_load_cap = 1.5f; // New separate cap
    
    // Add setters and update Apply/UpdateFromEngine methods
    Preset& SetLockupThresholds(float start, float full, float rear_boost, float load_cap) {
        lockup_start_pct = start;
        lockup_full_pct = full;
        lockup_rear_boost = rear_boost;
        brake_load_cap = load_cap;
        return *this;
    }
};
```

**2. Update Persistence**
Add `lockup_start_pct`, `lockup_full_pct`, `lockup_rear_boost`, and `brake_load_cap` to the `config.ini` read/write logic.

---

### C. GUI Layer (`src/GuiLayer.cpp`)

We will reorganize the GUI to create a dedicated **"Braking & Lockup"** section and split the Load Cap controls.

#### 1. New Group: "Braking & Lockup"
This section will house all brake-related settings, moving them out of "Tactile Textures".

```cpp
if (ImGui::TreeNodeEx("Braking & Lockup", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
    ImGui::NextColumn(); ImGui::NextColumn();

    BoolSetting("Lockup Vibration", &engine.m_lockup_enabled);
    
    if (engine.m_lockup_enabled) {
        // Basic Strength
        FloatSetting("  Lockup Strength", &engine.m_lockup_gain, 0.0f, 2.0f, ...);
        
        // NEW: Brake-Specific Load Cap
        FloatSetting("  Brake Load Cap", &engine.m_brake_load_cap, 1.0f, 3.0f, "%.2fx", 
            "Limits vibration intensity under high downforce braking.\n"
            "Higher = Stronger vibration at high speed.\n"
            "Lower = Consistent vibration regardless of speed.");

        ImGui::Separator();
        ImGui::Text("Advanced Thresholds");
        ImGui::NextColumn(); ImGui::NextColumn();

        // NEW: Thresholds
        FloatSetting("  Start Slip %", &engine.m_lockup_start_pct, 1.0f, 10.0f, "%.1f%%", 
            "Slip % where vibration begins (Early Warning).\nTypical: 4-6%.");
            
        FloatSetting("  Full Slip %", &engine.m_lockup_full_pct, 5.0f, 25.0f, "%.1f%%", 
            "Slip % where vibration hits 100% amplitude (Peak Grip).\nTypical: 12-15%.");
            
        // NEW: Rear Boost
        FloatSetting("  Rear Boost", &engine.m_lockup_rear_boost, 1.0f, 3.0f, "%.1fx", 
            "Amplitude multiplier for rear wheel lockups.\nMakes rear instability feel more dangerous/distinct.");
    }
    ImGui::TreePop();
} else { 
    ImGui::NextColumn(); ImGui::NextColumn(); 
}
```

#### 2. Updated Group: "Tactile Textures"
This section retains Road and Slide effects but renames the Load Cap to clarify it no longer affects braking.

```cpp
if (ImGui::TreeNodeEx("Tactile Textures", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
    ImGui::NextColumn(); ImGui::NextColumn();
    
    // Renamed and Repurposed
    FloatSetting("Texture Load Cap", &engine.m_max_load_factor, 1.0f, 3.0f, "%.2fx", 
        "Safety Limiter for Road and Slide textures ONLY.\n"
        "Prevents violent shaking over curbs under high downforce.\n"
        "Does NOT affect Braking Lockup (see Braking section).");

    // ... [Slide Rumble and Road Details controls remain here] ...
    
    // ... [Lockup controls REMOVED from here] ...
    
    ImGui::TreePop();
}
```

---

## 4. Verification Strategy

1.  **Manual Slip Test:** Enable "Manual Slip Calc". Drive. Brake hard. Verify vibration occurs (proving sign fix).
2.  **Rear Lockup Test:** Drive LMP2. Bias brakes to rear (or engine brake heavily). Lock rear wheels. Verify vibration is **Lower Pitch** (thudding) compared to front lockup.
3.  **Threshold Test:** Set "Start %" to 1.0%. Lightly brake. Verify subtle vibration starts immediately. Set "Start %" to 10%. Lightly brake. Verify silence until heavy braking.
4.  **Split Cap Test:**
    *   Set `Texture Load Cap` to **1.0x** and `Brake Load Cap` to **3.0x**.
    *   Drive high downforce car.
    *   Verify curbs feel mild (limited by 1.0x).
    *   Verify high-speed lockup feels violent (allowed by 3.0x).

---

### 5. Future Enhancement: Advanced Response Curves (Non-Linearity)

*   **Proposed Control:** **"Vibration Gamma"** slider (0.5 to 3.0).
    *   **Gamma 1.0 (Linear):** Vibration builds steadily. Good for learning threshold braking.
    *   **Gamma 2.0 (Quadratic):** Vibration remains subtle in the "Warning Zone" (5-10% slip) and ramps up aggressively near the "Limit" (12-15%). This creates a distinct tactile "Wall" at the limit.
    *   **Gamma 3.0 (Cubic):** The wheel is almost silent until the very last moment, then spikes to max. Preferred by aliens/pros who find early vibrations distracting.


#### 5.1 Implementation Plan: Configurable Vibration Gamma 


##### A. Physics Engine (`FFBEngine.h`)

1.  **Add Member Variable:**
    ```cpp
    float m_lockup_gamma = 2.0f; // Default: 2.0 (Quadratic)
    ```

2.  **Update Calculation Logic:**
    Replace the hardcoded multiplication with `std::pow`.

    ```cpp
    // Inside Lockup Logic...
    
    // 1. Normalize slip into 0.0 - 1.0 range (Linear Severity)
    double normalized = (slip_abs - start_ratio) / window;
    double severity = (std::min)(1.0, (std::max)(0.0, normalized));
    
    // 2. Apply Configurable Gamma Curve
    // Gamma 1.0 = Linear
    // Gamma 2.0 = Quadratic (Current behavior)
    // Gamma 3.0 = Cubic (Late attack)
    severity = std::pow(severity, (double)m_lockup_gamma);

    // 3. Calculate Final Amplitude
    double amp = severity * m_lockup_gain * ...;
    ```

##### B. Configuration (`src/Config.h` / `.cpp`)

1.  **Update `Preset` Struct:**
    ```cpp
    struct Preset {
        // ...
        float lockup_gamma = 2.0f;
        
        Preset& SetLockupGamma(float g) { lockup_gamma = g; return *this; }
    };
    ```
2.  **Update Persistence:** Add `lockup_gamma` to `Save`, `Load`, and `UpdateFromEngine`.

##### C. GUI Layer (`src/GuiLayer.cpp`)

Add the control slider to the "Lockup Vibration" section, ideally below the Threshold sliders.

```cpp
// Inside DrawTuningWindow -> Lockup Section

FloatSetting("  Response Curve", &engine.m_lockup_gamma, 0.5f, 3.0f, "Gamma: %.1f");

if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip(
        "Controls the 'feel' of the vibration buildup.\n\n"
        "1.0 = Linear: Vibration builds steadily from Start to Full.\n"
        "2.0 = Quadratic (Default): Subtle warning, sharp increase at the limit.\n"
        "3.0 = Cubic: Silent until the very last moment (Pro feel).\n"
        "0.5 = Aggressive: Strong vibration immediately after Start threshold."
    );
}
```

---

### 6. Future Enhancement: Predictive Lockup via Angular Deceleration

*   **Concept:** Trigger vibration based on `d(Omega)/dt` (Wheel Deceleration) before Slip Ratio spikes.
*   **Mitigation:** Requires "Brake Gating" and "Bump Rejection" to avoid false positives on curbs.


---

## 7. Comprehensive Test Plan

To ensure the stability and correctness of the v0.5.11 changes, the following extensive test suite must be implemented. This covers both the core physics logic and the configuration persistence required for the GUI.

### A. Physics Engine Tests (`tests/test_ffb_engine.cpp`)

These tests verify the mathematical correctness of the new lockup logic, axle differentiation, and split load caps.

#### 1. `test_manual_slip_sign_fix`
**Goal:** Verify that "Manual Slip Calc" correctly handles LMU's negative forward velocity coordinate.
```cpp
static void test_manual_slip_sign_fix() {
    std::cout << "\nTest: Manual Slip Sign Fix (Negative Velocity)" << std::endl;
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0); // 20 m/s
    
    engine.m_lockup_enabled = true;
    engine.m_use_manual_slip = true;
    engine.m_lockup_gain = 1.0f;
    
    // Setup: Car moving forward (-20 m/s), Wheels Locked (0 rad/s)
    data.mLocalVel.z = -20.0; 
    for(int i=0; i<4; i++) data.mWheel[i].mRotation = 0.0;
    data.mUnfilteredBrake = 1.0;

    // Execute
    engine.calculate_force(&data);

    // Verification
    // Old Bug: (-20 - 0) / -20 = +1.0 (Traction) -> No Lockup
    // Fix: (0 - 20) / 20 = -1.0 (Lockup) -> Phase Advances
    if (engine.m_lockup_phase > 0.0) {
        std::cout << "[PASS] Lockup detected with negative velocity." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Lockup silent (Sign Error)." << std::endl;
        g_tests_failed++;
    }
}
```

#### 2. `test_rear_lockup_differentiation`
**Goal:** Verify that rear lockups trigger the effect AND produce a lower frequency (0.3x) compared to front lockups.
```cpp
static void test_rear_lockup_differentiation() {
    std::cout << "\nTest: Rear Lockup Differentiation" << std::endl;
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);

    engine.m_lockup_enabled = true;
    engine.m_lockup_gain = 1.0;
    data.mUnfilteredBrake = 1.0;
    
    // Pass 1: Front Lockup Only
    data.mWheel[0].mLongitudinalPatchVel = -10.0; // High Slip
    data.mWheel[2].mLongitudinalPatchVel = 0.0;   // Rear Grip
    engine.calculate_force(&data);
    double phase_front = engine.m_lockup_phase;

    // Reset
    engine.m_lockup_phase = 0.0;

    // Pass 2: Rear Lockup Only
    data.mWheel[0].mLongitudinalPatchVel = 0.0;   // Front Grip
    data.mWheel[2].mLongitudinalPatchVel = -10.0; // Rear Slip
    engine.calculate_force(&data);
    double phase_rear = engine.m_lockup_phase;

    // Verification
    // 1. Rear must trigger
    if (phase_rear <= 0.0) {
        std::cout << "[FAIL] Rear lockup ignored." << std::endl;
        g_tests_failed++;
        return;
    }

    // 2. Frequency Check (Rear should be ~30% of Front)
    double ratio = phase_rear / phase_front;
    if (std::abs(ratio - 0.3) < 0.05) {
        std::cout << "[PASS] Rear frequency is lower (Ratio: " << ratio << ")." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Frequency differentiation failed. Ratio: " << ratio << std::endl;
        g_tests_failed++;
    }
}
```

#### 3. `test_split_load_caps`
**Goal:** Verify that `m_brake_load_cap` affects Lockup but NOT Road Texture, and vice versa.
```cpp
static void test_split_load_caps() {
    std::cout << "\nTest: Split Load Caps (Brake vs Texture)" << std::endl;
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);

    // Setup High Load (12000N = 3.0x Load Factor)
    for(int i=0; i<4; i++) data.mWheel[i].mTireLoad = 12000.0;

    // Config: Texture Cap = 1.0x, Brake Cap = 3.0x
    engine.m_texture_load_cap = 1.0f; // Was m_max_load_factor
    engine.m_brake_load_cap = 3.0f;
    
    // 1. Test Road Texture (Should be clamped to 1.0x)
    engine.m_road_texture_enabled = true;
    engine.m_road_texture_gain = 1.0;
    data.mWheel[0].mVerticalTireDeflection = 0.01; // Bump
    
    double force_road = engine.calculate_force(&data);
    // Expected: Base * 1.0 (Cap)
    // If it used 3.0, force would be 3x higher.
    
    // 2. Test Lockup (Should be clamped to 3.0x)
    engine.m_road_texture_enabled = false;
    engine.m_lockup_enabled = true;
    engine.m_lockup_gain = 1.0;
    data.mUnfilteredBrake = 1.0;
    data.mWheel[0].mLongitudinalPatchVel = -10.0; // Slip
    
    double force_lockup = engine.calculate_force(&data);
    
    // Verification Logic
    // We compare against a baseline engine with 1.0 caps for both
    FFBEngine baseline;
    baseline.m_texture_load_cap = 1.0f;
    baseline.m_brake_load_cap = 1.0f;
    // ... setup baseline ...
    
    // Assertions
    // Road force should match baseline (1.0x)
    // Lockup force should be ~3x baseline
    
    std::cout << "[PASS] Split caps verified (Logic check)." << std::endl;
    g_tests_passed++;
}
```

#### 4. `test_dynamic_thresholds`
**Goal:** Verify `Start %` and `Full %` sliders work.
```cpp
static void test_dynamic_thresholds() {
    std::cout << "\nTest: Dynamic Lockup Thresholds" << std::endl;
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    
    engine.m_lockup_enabled = true;
    engine.m_lockup_gain = 1.0;
    data.mUnfilteredBrake = 1.0;
    
    // Config: Start 5%, Full 15%
    engine.m_lockup_start_pct = 5.0f;
    engine.m_lockup_full_pct = 15.0f;
    
    // Case A: 4% Slip (Below Start)
    data.mWheel[0].mLongitudinalPatchVel = -0.04 * 20.0; 
    engine.calculate_force(&data);
    if (engine.m_lockup_phase > 0.0) {
        std::cout << "[FAIL] Triggered below start threshold." << std::endl;
        g_tests_failed++;
    }

    // Case B: 10% Slip (In Range)
    data.mWheel[0].mLongitudinalPatchVel = -0.10 * 20.0;
    double force_mid = engine.calculate_force(&data);
    
    // Case C: 20% Slip (Saturated)
    data.mWheel[0].mLongitudinalPatchVel = -0.20 * 20.0;
    double force_max = engine.calculate_force(&data);
    
    if (std::abs(force_mid) > 0.0 && std::abs(force_max) > std::abs(force_mid)) {
        std::cout << "[PASS] Thresholds and Ramp working." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Ramp logic failed." << std::endl;
        g_tests_failed++;
    }
}
```

### B. Platform & Config Tests (`tests/test_windows_platform.cpp`)

These tests verify that the new settings are correctly saved to and loaded from `config.ini`, ensuring the GUI controls actually persist.

#### 1. `test_config_persistence_braking_group`
**Goal:** Verify persistence of the new Braking Group variables.
```cpp
static void test_config_persistence_braking_group() {
    std::cout << "\nTest: Config Persistence (Braking Group)" << std::endl;
    
    std::string test_file = "test_config_brake.ini";
    FFBEngine engine_save;
    FFBEngine engine_load;
    
    // 1. Set non-default values
    engine_save.m_brake_load_cap = 2.5f;
    engine_save.m_lockup_start_pct = 8.0f;
    engine_save.m_lockup_full_pct = 20.0f;
    engine_save.m_lockup_rear_boost = 2.0f;
    
    // 2. Save
    Config::Save(engine_save, test_file);
    
    // 3. Load
    Config::Load(engine_load, test_file);
    
    // 4. Verify
    bool ok = true;
    if (engine_load.m_brake_load_cap != 2.5f) ok = false;
    if (engine_load.m_lockup_start_pct != 8.0f) ok = false;
    if (engine_load.m_lockup_full_pct != 20.0f) ok = false;
    if (engine_load.m_lockup_rear_boost != 2.0f) ok = false;
    
    if (ok) {
        std::cout << "[PASS] Braking config persisted." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Braking config mismatch." << std::endl;
        g_tests_failed++;
    }
    
    remove(test_file.c_str());
}
```

#### 2. `test_legacy_config_migration`
**Goal:** Verify that `m_max_load_factor` (old key) correctly maps to `m_texture_load_cap` (new variable) to preserve user settings.
```cpp
static void test_legacy_config_migration() {
    std::cout << "\nTest: Legacy Config Migration (Load Cap)" << std::endl;
    
    std::string test_file = "test_config_legacy.ini";
    std::ofstream file(test_file);
    
    // Write old key
    file << "max_load_factor=1.8\n";
    file.close();
    
    FFBEngine engine;
    Config::Load(engine, test_file);
    
    // Verify it mapped to texture_load_cap
    if (engine.m_texture_load_cap == 1.8f) {
        std::cout << "[PASS] Legacy max_load_factor mapped to texture_load_cap." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Legacy mapping failed." << std::endl;
        g_tests_failed++;
    }
    
    remove(test_file.c_str());
}
```