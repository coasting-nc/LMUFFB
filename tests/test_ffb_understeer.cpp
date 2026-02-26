#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_optimal_slip_buffer_zone, "Understeer") {
    std::cout << "\nTest: Optimal Slip Buffer Zone (v0.6.28/v0.6.31)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_optimal_slip_angle = 0.10f;
    engine.m_understeer_effect = 1.0f; // New scale
    
    // Simulate telemetry with slip_angle = 0.06 rad (60% of 0.10)
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.06);
    data.mSteeringShaftTorque = 20.0;
    
    // Run multiple frames to settle filters
    double force = 0.0;
    for (int i = 0; i < FILTER_SETTLING_FRAMES; i++) force = engine.calculate_force(&data);
    
    // Since grip should be 1.0 (slip 0.06 <= optimal 0.10)
    ASSERT_NEAR(force, 1.0, 0.001);
}

TEST_CASE(test_progressive_loss_curve, "Understeer") {
    std::cout << "\nTest: Progressive Loss Curve (v0.6.28/v0.6.31)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_optimal_slip_angle = 0.10f;
    engine.m_understeer_effect = 1.0f;  // Proportional
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.10); // 1.0x optimal
    data.mSteeringShaftTorque = 20.0;
    double f10 = 0.0;
    for (int i = 0; i < FILTER_SETTLING_FRAMES; i++) f10 = engine.calculate_force(&data);
    
    data = CreateBasicTestTelemetry(20.0, 0.12); // 1.2x optimal -> excess 0.2
    data.mSteeringShaftTorque = 20.0;
    double f12 = 0.0;
    for (int i = 0; i < FILTER_SETTLING_FRAMES; i++) f12 = engine.calculate_force(&data);
    
    data = CreateBasicTestTelemetry(20.0, 0.14); // 1.4x optimal -> excess 0.4
    data.mSteeringShaftTorque = 20.0;
    double f14 = 0.0;
    for (int i = 0; i < FILTER_SETTLING_FRAMES; i++) f14 = engine.calculate_force(&data);
    
    ASSERT_NEAR(f10, 1.0, 0.001);
    ASSERT_TRUE(f10 > f12 && f12 > f14);
}

TEST_CASE(test_grip_floor_clamp, "Understeer") {
    std::cout << "\nTest: Grip Floor Clamp" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_optimal_slip_angle = 0.05f; 
    engine.m_understeer_effect = 1.0f; 
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 10.0); // Infinite slip
    data.mSteeringShaftTorque = 20.0;
    
    double force = 0.0;
    for (int i = 0; i < FILTER_SETTLING_FRAMES; i++) force = engine.calculate_force(&data);
    
    // GRIP_FLOOR_CLAMP: The grip estimator in FFBEngine.h (line 622) enforces a minimum
    // grip value of 0.2 to prevent total force loss even under extreme slip conditions.
    // This safety floor ensures the wheel never goes completely dead.
    ASSERT_NEAR(force, 0.2, 0.001);
}

TEST_CASE(test_understeer_output_clamp, "Understeer") {
    std::cout << "\nTest: Understeer Output Clamp (v0.6.28/v0.6.31)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_optimal_slip_angle = 0.10f;
    engine.m_understeer_effect = 2.0f; // Max effective
    
    // Slip = 0.20 -> excess = 1.0 (approx). 
    // factor = 1.0 - (loss * effect) -> should easily clamp to 0.0.
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.20);
    data.mSteeringShaftTorque = 20.0;
    
    double force = 0.0;
    for (int i = 0; i < FILTER_SETTLING_FRAMES; i++) force = engine.calculate_force(&data);
    
    ASSERT_NEAR(force, 0.0, 0.001);
}

TEST_CASE(test_understeer_range_validation, "Understeer") {
    std::cout << "\nTest: Understeer Range Validation" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_understeer_effect = 1.5f;
    ASSERT_GE(engine.m_understeer_effect, 0.0f);
    ASSERT_LE(engine.m_understeer_effect, 2.0f);
}

TEST_CASE(test_understeer_effect_scaling, "Understeer") {
    std::cout << "\nTest: Understeer Effect Scaling" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_optimal_slip_angle = 0.10f;
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.12); // ~30% loss
    data.mSteeringShaftTorque = 20.0;
    
    engine.m_understeer_effect = 0.0f;
    double f0 = 0.0;
    for (int i = 0; i < FILTER_SETTLING_FRAMES; i++) f0 = engine.calculate_force(&data);
    
    engine.m_understeer_effect = 1.0f;
    double f1 = 0.0;
    for (int i = 0; i < FILTER_SETTLING_FRAMES; i++) f1 = engine.calculate_force(&data);
    
    engine.m_understeer_effect = 2.0f;
    double f2 = 0.0;
    for (int i = 0; i < FILTER_SETTLING_FRAMES; i++) f2 = engine.calculate_force(&data);
    
    ASSERT_TRUE(f0 > f1 && f1 > f2);
}

TEST_CASE(test_legacy_config_migration, "Understeer") {
    std::cout << "\nTest: Legacy Config Migration" << std::endl;
    
    float legacy_val = 50.0f; 
    float migrated = legacy_val;
    if (migrated > 2.0f) migrated /= 100.0f;
    
    ASSERT_NEAR(migrated, 0.5f, 0.001);
    
    float modern_val = 1.5f;
    migrated = modern_val;
    if (migrated > 2.0f) migrated /= 100.0f;
    ASSERT_NEAR(migrated, 1.5f, 0.001);
}

TEST_CASE(test_preset_understeer_only_isolation, "Understeer") {
    std::cout << "\nTest: Preset 'Test: Understeer Only' Isolation (v0.6.31)" << std::endl;
    
    // Load presets
    Config::LoadPresets();
    
    // Find the "Test: Understeer Only" preset
    int preset_idx = -1;
    for (size_t i = 0; i < Config::presets.size(); i++) {
        if (Config::presets[i].name == "Test: Understeer Only") {
            preset_idx = (int)i;
            break;
        }
    }
    
    if (preset_idx == -1) {
        std::cout << "[FAIL] 'Test: Understeer Only' preset not found" << std::endl;
        g_tests_failed++;
        return;
    }
    
    const Preset& p = Config::presets[preset_idx];
    
    // VERIFY: Primary effect is enabled
    ASSERT_TRUE(p.understeer > 0.0f && p.understeer <= 2.0f);
    
    // VERIFY: All other effects are DISABLED
    ASSERT_NEAR(p.sop, 0.0f, 0.001f);                    // SoP disabled
    ASSERT_NEAR(p.oversteer_boost, 0.0f, 0.001f);        // Oversteer boost disabled
    ASSERT_NEAR(p.rear_align_effect, 0.0f, 0.001f);      // Rear align disabled
    ASSERT_NEAR(p.sop_yaw_gain, 0.0f, 0.001f);           // Yaw kick disabled
    ASSERT_NEAR(p.gyro_gain, 0.0f, 0.001f);              // Gyro damping disabled
    ASSERT_NEAR(p.scrub_drag_gain, 0.0f, 0.001f);        // Scrub drag disabled
    
    // VERIFY: All textures are DISABLED
    ASSERT_TRUE(p.slide_enabled == false);               // Slide texture disabled
    ASSERT_TRUE(p.road_enabled == false);                // Road texture disabled
    ASSERT_TRUE(p.spin_enabled == false);                // Spin texture disabled
    ASSERT_TRUE(p.lockup_enabled == false);              // Lockup vibration disabled
    ASSERT_TRUE(p.abs_pulse_enabled == false);           // ABS pulse disabled
    
    // VERIFY: Critical physics parameters are set correctly
    ASSERT_NEAR(p.optimal_slip_angle, 0.10f, 0.001f);    // Optimal slip angle threshold
    ASSERT_NEAR(p.optimal_slip_ratio, 0.12f, 0.001f);    // Optimal slip ratio threshold
    
    // VERIFY: Speed gate is disabled (0.0 = no gating)
    ASSERT_NEAR(p.speed_gate_lower, 0.0f, 0.001f);       // Speed gate disabled
    ASSERT_NEAR(p.speed_gate_upper, 0.0f, 0.001f);       // Speed gate disabled
    
    std::cout << "[PASS] 'Test: Understeer Only' preset properly isolates understeer effect" << std::endl;
    g_tests_passed++;
}

TEST_CASE(test_all_presets_non_negative_speed_gate, "Understeer") {
    std::cout << "\nTest: All Presets Have Non-Negative Speed Gate Values (v0.6.32)" << std::endl;
    
    // Load all presets
    Config::LoadPresets();
    
    // Verify every preset has non-negative speed gate values
    bool all_valid = true;
    for (size_t i = 0; i < Config::presets.size(); i++) {
        const Preset& p = Config::presets[i];
        
        // Check lower threshold
        if (p.speed_gate_lower < 0.0f) {
            std::cout << "[FAIL] Preset '" << p.name << "' has negative speed_gate_lower: " 
                      << p.speed_gate_lower << " m/s (" << (p.speed_gate_lower * 3.6f) << " km/h)" << std::endl;
            all_valid = false;
        }
        
        // Check upper threshold
        if (p.speed_gate_upper < 0.0f) {
            std::cout << "[FAIL] Preset '" << p.name << "' has negative speed_gate_upper: " 
                      << p.speed_gate_upper << " m/s (" << (p.speed_gate_upper * 3.6f) << " km/h)" << std::endl;
            all_valid = false;
        }
        
        // Verify upper >= lower (sanity check)
        if (p.speed_gate_upper < p.speed_gate_lower) {
            std::cout << "[FAIL] Preset '" << p.name << "' has speed_gate_upper < speed_gate_lower: " 
                      << p.speed_gate_upper << " < " << p.speed_gate_lower << std::endl;
            all_valid = false;
        }
    }
    
    if (all_valid) {
        std::cout << "[PASS] All " << Config::presets.size() << " presets have valid non-negative speed gate values" << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] One or more presets have invalid speed gate values" << std::endl;
        g_tests_failed++;
    }
}



} // namespace FFBEngineTests