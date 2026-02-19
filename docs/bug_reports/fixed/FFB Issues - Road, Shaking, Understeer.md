To address the issues reported by the user, I will implement three key improvements:

1.  **User-Adjustable Speed Gate**: To fix the "violent shaking below 15km/h", I will expose the stationary speed gate thresholds. This allows the user to raise the "Mute Below" speed to 15 km/h, silencing the idle vibrations.
2.  **Road Texture Fallback Tuning**: To address the weak road feel on Sebring (likely due to encrypted car telemetry), I will expose the fallback scaling factor. This allows the user to boost the acceleration-based road noise.
3.  **Understeer on SoP**: To fix the "cannot feel wheel getting lighter" issue, I will add an option to apply the Understeer reduction to the SoP (Seat of Pants) forces as well. Currently, strong SoP forces can mask the grip loss sensation; this option ensures the entire FFB signal drops when grip is lost.

Here are the code snippets to implement these changes.

### 1. Update `src/FFBEngine.h`

```cpp
// In FFBEngine class public members (add new variables)
    // ... existing members ...
    
    // v0.6.23: Speed Gate Tuning
    float m_speed_gate_lower = 0.5f; // Start fading (m/s)
    float m_speed_gate_upper = 2.0f; // Full strength (m/s)
    
    // v0.6.23: Road Texture Fallback Scaling
    float m_road_texture_fallback_scale = 0.05f; 
    
    // v0.6.23: Understeer Options
    bool m_understeer_affects_sop = false;

// In calculate_force method:

        // ... [Inside Frequency Estimator / Dynamic Notch Logic] ...
        
        // 1. Calculate Stationary Gate (Fade out vibrations at low speed)
        // REPLACED hardcoded values with user-adjustable members
        double gate_lower = (double)m_speed_gate_lower;
        double gate_upper = (double)m_speed_gate_upper;
        double gate_window = gate_upper - gate_lower;
        
        // Safety: Prevent division by zero
        if (gate_window < 0.1) gate_window = 0.1;

        double speed_gate = (car_v_long - gate_lower) / gate_window;
        speed_gate = (std::max)(0.0, (std::min)(1.0, speed_gate));

        // ... [Inside Road Texture Logic] ...

            if (deflection_active || car_v_long < 5.0) {
                // Standard Logic
                road_noise_val = (delta_l + delta_r) * 50.0;
            } else {
                // Fallback: Use Vertical Acceleration (Heave)
                double vert_accel = data->mLocalAccel.y;
                double delta_accel = vert_accel - m_prev_vert_accel;
                
                // REPLACED hardcoded 0.05 with m_road_texture_fallback_scale
                road_noise_val = delta_accel * (double)m_road_texture_fallback_scale * 50.0; 
            }

        // ... [Inside SoP Logic, after calculating sop_total] ...
        
        // v0.6.23: Optional Understeer on SoP
        // If enabled, grip loss reduces SoP forces too, preventing them from masking the understeer feel.
        if (m_understeer_affects_sop) {
            sop_total *= grip_factor;
        }
        
        double total_force = output_force + sop_total;
```

### 2. Update `src/Config.h`

```cpp
// In Preset struct
struct Preset {
    // ... existing members ...
    
    // v0.6.23 New Settings
    float speed_gate_lower = 0.5f;
    float speed_gate_upper = 2.0f;
    float road_fallback_scale = 0.05f;
    bool understeer_affects_sop = false;
    
    // ... existing methods ...
    
    // Update Apply()
    void Apply(FFBEngine& engine) const {
        // ... existing ...
        engine.m_speed_gate_lower = speed_gate_lower;
        engine.m_speed_gate_upper = speed_gate_upper;
        engine.m_road_texture_fallback_scale = road_fallback_scale;
        engine.m_understeer_affects_sop = understeer_affects_sop;
    }
    
    // Update UpdateFromEngine()
    void UpdateFromEngine(const FFBEngine& engine) {
        // ... existing ...
        speed_gate_lower = engine.m_speed_gate_lower;
        speed_gate_upper = engine.m_speed_gate_upper;
        road_fallback_scale = engine.m_road_texture_fallback_scale;
        understeer_affects_sop = engine.m_understeer_affects_sop;
    }
};
```

### 3. Update `src/Config.cpp`

```cpp
// In Config::Save()
    file << "speed_gate_lower=" << preset.speed_gate_lower << "\n";
    file << "speed_gate_upper=" << preset.speed_gate_upper << "\n";
    file << "road_fallback_scale=" << preset.road_fallback_scale << "\n";
    file << "understeer_affects_sop=" << preset.understeer_affects_sop << "\n";

// In Config::Load()
    else if (key == "speed_gate_lower") preset.speed_gate_lower = std::stof(value);
    else if (key == "speed_gate_upper") preset.speed_gate_upper = std::stof(value);
    else if (key == "road_fallback_scale") preset.road_fallback_scale = std::stof(value);
    else if (key == "understeer_affects_sop") preset.understeer_affects_sop = std::stoi(value);

    // Validation
    if (preset.speed_gate_upper <= preset.speed_gate_lower + 0.1f) {
        preset.speed_gate_upper = preset.speed_gate_lower + 0.5f;
    }
```

### 4. Update `src/GuiLayer.cpp`

```cpp
// In DrawTuningWindow...

    // --- GROUP: FRONT AXLE ---
    if (ImGui::TreeNodeEx("Front Axle (Understeer)", ...)) {
        // ... existing sliders ...
        
        FloatSetting("Understeer Effect", &engine.m_understeer_effect, 0.0f, 200.0f, "%.2f", "...");
        
        // NEW: Understeer affects SoP
        BoolSetting("  Apply to SoP", &engine.m_understeer_affects_sop, 
            "If checked, the Understeer Effect also reduces Seat of Pants forces.\n"
            "Use this if strong SoP forces are masking the feeling of grip loss.");
            
        // ...
    }

    // --- GROUP: TEXTURES ---
    if (ImGui::TreeNodeEx("Tactile Textures", ...)) {
        // ... existing sliders ...
        
        if (engine.m_road_texture_enabled) {
             FloatSetting("  Road Gain", &engine.m_road_texture_gain, 0.0f, 2.0f, ...);
             
             // NEW: Fallback Tuning
             if (ImGui::TreeNode("  Advanced: Encrypted Content Fallback")) {
                ImGui::TextWrapped("Adjusts road texture for cars with blocked telemetry (DLC).");
                FloatSetting("    Fallback Sensitivity", &engine.m_road_texture_fallback_scale, 0.01f, 0.20f, "%.2f", 
                    "Scales vertical G-force to road noise.\nIncrease if road feel is too weak on DLC cars.");
                ImGui::TreePop();
             }
        }
        // ...
    }

    // --- ADVANCED SETTINGS ---
    if (ImGui::CollapsingHeader("Advanced Settings")) {
        ImGui::Indent();
        
        if (ImGui::TreeNode("Stationary Vibration Gate")) {
            ImGui::TextWrapped("Controls when vibrations fade out to prevent idle shaking.");
            
            float lower_kmh = engine.m_speed_gate_lower * 3.6f;
            if (ImGui::SliderFloat("Mute Below", &lower_kmh, 0.0f, 20.0f, "%.1f km/h")) {
                engine.m_speed_gate_lower = lower_kmh / 3.6f;
                if (engine.m_speed_gate_upper <= engine.m_speed_gate_lower + 0.1f) 
                    engine.m_speed_gate_upper = engine.m_speed_gate_lower + 0.5f;
                selected_preset = -1;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Speed below which vibrations are muted.\nIncrease this if your wheel shakes while stopped.");

            float upper_kmh = engine.m_speed_gate_upper * 3.6f;
            if (ImGui::SliderFloat("Full Above", &upper_kmh, 1.0f, 30.0f, "%.1f km/h")) {
                engine.m_speed_gate_upper = upper_kmh / 3.6f;
                if (engine.m_speed_gate_upper <= engine.m_speed_gate_lower + 0.1f)
                    engine.m_speed_gate_upper = engine.m_speed_gate_lower + 0.5f;
                selected_preset = -1;
            }
            
            ImGui::TreePop();
        }
        ImGui::Unindent();
    }
```

### 5. Update `tests/test_ffb_engine.cpp`

```cpp
static void test_speed_gate_custom_thresholds() {
    std::cout << "\nTest: Speed Gate Custom Thresholds" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_road_texture_enabled = true;
    engine.m_road_texture_gain = 1.0;
    
    // Set custom gate: Mute below 10 m/s
    engine.m_speed_gate_lower = 10.0f;
    engine.m_speed_gate_upper = 12.0f;
    
    // Input: 5 m/s (Should be muted)
    TelemInfoV01 data = CreateBasicTestTelemetry(5.0);
    data.mWheel[0].mVerticalTireDeflection = 0.002; // Bump
    data.mWheel[1].mVerticalTireDeflection = 0.002;
    
    double force = engine.calculate_force(&data);
    ASSERT_NEAR(force, 0.0, 0.0001);
    
    // Input: 15 m/s (Should be active)
    data.mLocalVel.z = -15.0;
    double force_active = engine.calculate_force(&data);
    if (std::abs(force_active) > 0.001) {
        std::cout << "[PASS] Speed gate custom thresholds working." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Speed gate failed to open." << std::endl;
        g_tests_failed++;
    }
}

static void test_understeer_affects_sop() {
    std::cout << "\nTest: Understeer Affects SoP" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    
    engine.m_sop_effect = 1.0;
    engine.m_sop_scale = 10.0;
    engine.m_understeer_effect = 1.0; // Full cut
    engine.m_gain = 1.0;
    
    // Setup: High SoP (1G), Zero Grip
    data.mLocalAccel.x = 9.81;
    data.mWheel[0].mGripFract = 0.0;
    data.mWheel[1].mGripFract = 0.0;
    
    // Case 1: Disabled (Default) - SoP should remain strong
    engine.m_understeer_affects_sop = false;
    double force_default = engine.calculate_force(&data);
    
    // Case 2: Enabled - SoP should be cut by grip loss
    engine.m_understeer_affects_sop = true;
    double force_cut = engine.calculate_force(&data);
    
    if (std::abs(force_cut) < std::abs(force_default)) {
        std::cout << "[PASS] SoP reduced by understeer setting." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] SoP not affected by understeer." << std::endl;
        g_tests_failed++;
    }
}
```