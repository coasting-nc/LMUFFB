
### The "Sweet Spot" Analysis

*   **The Issue:** Engine vibration in rFactor 2 / LMU is physically simulated based on RPM and chassis resonance. This is most violent at **Idle** (0 km/h) and **Clutch Bite / Low RPM** (1-15 km/h).
*   **The Trade-off:** We want to smooth this vibration out, but we don't want to smooth out useful FFB (like curb strikes or understeer) while driving.
*   **The Sweet Spot:** **15 km/h to 20 km/h**.
    *   **Why:** No racing corner in the world is taken at 20 km/h (even the Loews hairpin at Monaco is ~45-50 km/h).
    *   **Impact:** By setting the threshold to **18 km/h (5.0 m/s)**, we ensure the wheel is "calm" while parking, leaving the pit box, or recovering from a spin. We lose zero competitive information because you are not "racing" at 18 km/h.

### Updated Implementation (New Defaults)

Here are the updated snippets with the **18 km/h default**.

#### 1. Update `src/FFBEngine.h`

```cpp
// In FFBEngine class public members:

    // v0.6.23: User-Adjustable Speed Gate
    // CHANGED DEFAULTS:
    // Lower: 1.0 m/s (3.6 km/h) - Start fading in
    // Upper: 5.0 m/s (18.0 km/h) - Full strength / End smoothing
    // This ensures the "Violent Shaking" (< 15km/h) is covered by default.
    float m_speed_gate_lower = 1.0f; 
    float m_speed_gate_upper = 5.0f; 
    
    // ... rest of variables ...

// In calculate_force method:

        // ... [Automatic Idle Smoothing Logic] ...
        
        // Use the user-configured Upper Threshold
        // Default is now 5.0 m/s (18 km/h), which covers the user's "below 15km/h" issue.
        double idle_speed_threshold = (double)m_speed_gate_upper; 
        
        // Safety floor: Never go below 3.0 m/s even if user lowers the gate
        if (idle_speed_threshold < 3.0) idle_speed_threshold = 3.0;

        // ... [Rest of logic is the same] ...
```

#### 2. Update `src/Config.h`

```cpp
struct Preset {
    // ... existing members ...
    
    // v0.6.23 New Settings with HIGHER DEFAULTS
    float speed_gate_lower = 1.0f; // 3.6 km/h
    float speed_gate_upper = 5.0f; // 18.0 km/h (Fixes idle shake)
    float road_fallback_scale = 0.05f;
    bool understeer_affects_sop = false;
    
    // ...
};
```

#### 3. Update `src/GuiLayer.cpp`

I will also update the slider ranges to allow users to go even higher if needed (up to 50 km/h), just in case.

```cpp
// In DrawTuningWindow...

    // --- ADVANCED SETTINGS ---
    if (ImGui::CollapsingHeader("Advanced Settings")) {
        ImGui::Indent();
        
        if (ImGui::TreeNode("Stationary Vibration Gate")) {
            ImGui::TextWrapped("Controls when vibrations fade out and Idle Smoothing activates.");
            
            float lower_kmh = engine.m_speed_gate_lower * 3.6f;
            // Range: 0 to 20 km/h
            if (ImGui::SliderFloat("Mute Below", &lower_kmh, 0.0f, 20.0f, "%.1f km/h")) {
                engine.m_speed_gate_lower = lower_kmh / 3.6f;
                if (engine.m_speed_gate_upper <= engine.m_speed_gate_lower + 0.1f) 
                    engine.m_speed_gate_upper = engine.m_speed_gate_lower + 0.5f;
                selected_preset = -1;
            }

            float upper_kmh = engine.m_speed_gate_upper * 3.6f;
            // Range: 1 to 50 km/h (Increased max range to give users flexibility)
            if (ImGui::SliderFloat("Full Above", &upper_kmh, 1.0f, 50.0f, "%.1f km/h")) {
                engine.m_speed_gate_upper = upper_kmh / 3.6f;
                if (engine.m_speed_gate_upper <= engine.m_speed_gate_lower + 0.1f)
                    engine.m_speed_gate_upper = engine.m_speed_gate_lower + 0.5f;
                selected_preset = -1;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip(
                "Speed where vibrations reach full strength.\n"
                "CRITICAL: Speeds below this value will have SMOOTHING applied\n"
                "to eliminate engine idle vibration.\n"
                "Default: 18.0 km/h (Safe for all wheels).");
            
            ImGui::TreePop();
        }
        ImGui::Unindent();
    }
```

#### 4. Update `tests/test_ffb_engine.cpp`

Update the test to reflect the new default behavior.

```cpp
static void test_speed_gate_custom_thresholds() {
    std::cout << "\nTest: Speed Gate Custom Thresholds" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    // Verify new defaults
    if (engine.m_speed_gate_upper == 5.0f) {
        std::cout << "[PASS] Default upper threshold is 5.0 m/s (18 km/h)." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Default upper threshold is " << engine.m_speed_gate_upper << std::endl;
        g_tests_failed++;
    }

    // ... [Rest of test logic] ...
}
```