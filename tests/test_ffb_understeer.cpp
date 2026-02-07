#include "test_ffb_common.h"
#include "../src/PresetRegistry.h"

namespace FFBEngineTests {

TEST_CASE(test_optimal_slip_buffer_zone, "Understeer") {
    std::cout << "\nTest: Optimal Slip Buffer Zone (v0.6.28/v0.6.31)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_optimal_slip_angle = 0.10f;
    engine.m_understeer_effect = 1.0f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.06);
    data.mSteeringShaftTorque = 20.0;
    
    double force = 0.0;
    for (int i = 0; i < FILTER_SETTLING_FRAMES; i++) force = engine.calculate_force(&data);
    
    ASSERT_NEAR(force, 1.0, 0.001);
}

TEST_CASE(test_progressive_loss_curve, "Understeer") {
    std::cout << "\nTest: Progressive Loss Curve (v0.6.28/v0.6.31)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_optimal_slip_angle = 0.10f;
    engine.m_understeer_effect = 1.0f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.10);
    data.mSteeringShaftTorque = 20.0;
    double f10 = 0.0;
    for (int i = 0; i < FILTER_SETTLING_FRAMES; i++) f10 = engine.calculate_force(&data);
    
    data = CreateBasicTestTelemetry(20.0, 0.12);
    data.mSteeringShaftTorque = 20.0;
    double f12 = 0.0;
    for (int i = 0; i < FILTER_SETTLING_FRAMES; i++) f12 = engine.calculate_force(&data);
    
    data = CreateBasicTestTelemetry(20.0, 0.14);
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
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 10.0);
    data.mSteeringShaftTorque = 20.0;
    
    double force = 0.0;
    for (int i = 0; i < FILTER_SETTLING_FRAMES; i++) force = engine.calculate_force(&data);
    
    ASSERT_NEAR(force, 0.2, 0.001);
}

TEST_CASE(test_understeer_output_clamp, "Understeer") {
    std::cout << "\nTest: Understeer Output Clamp (v0.6.28/v0.6.31)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_optimal_slip_angle = 0.10f;
    engine.m_understeer_effect = 2.0f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.20);
    data.mSteeringShaftTorque = 20.0;
    
    double force = 0.0;
    for (int i = 0; i < FILTER_SETTLING_FRAMES; i++) force = engine.calculate_force(&data);
    
    ASSERT_NEAR(force, 0.0, 0.001);
}

TEST_CASE(test_preset_understeer_only_isolation, "Understeer") {
    std::cout << "\nTest: Preset 'Test: Understeer Only' Isolation (v0.6.31)" << std::endl;
    
    PresetRegistry& registry = PresetRegistry::Get();
    registry.Load("non_existent.ini");
    const auto& presets = registry.GetPresets();

    int preset_idx = -1;
    for (size_t i = 0; i < presets.size(); i++) {
        if (presets[i].name == "Test: Understeer Only") {
            preset_idx = (int)i;
            break;
        }
    }
    
    ASSERT_TRUE(preset_idx != -1);
    const Preset& p = presets[preset_idx];
    ASSERT_TRUE(p.understeer > 0.0f && p.understeer <= 2.0f);
    ASSERT_NEAR(p.sop, 0.0f, 0.001f);
}

TEST_CASE(test_all_presets_non_negative_speed_gate, "Understeer") {
    std::cout << "\nTest: All Presets Have Non-Negative Speed Gate Values (v0.6.32)" << std::endl;
    
    PresetRegistry& registry = PresetRegistry::Get();
    registry.Load("non_existent.ini");
    const auto& presets = registry.GetPresets();
    
    bool all_valid = true;
    for (size_t i = 0; i < presets.size(); i++) {
        const Preset& p = presets[i];
        if (p.speed_gate_lower < 0.0f) all_valid = false;
        if (p.speed_gate_upper < 0.0f) all_valid = false;
    }
    
    if (all_valid) {
        std::cout << "[PASS] All " << presets.size() << " presets have valid speed gate values" << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Speed gate invalid" << std::endl;
        g_tests_failed++;
    }
}

} // namespace FFBEngineTests
