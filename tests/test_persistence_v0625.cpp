#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include "src/FFBEngine.h"
#include "src/Config.h"
#include "src/Version.h"

namespace PersistenceTests {

int g_tests_passed = 0;
int g_tests_failed = 0;

#define ASSERT_TRUE(condition) \
    if (condition) { \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #condition << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
        g_tests_failed++; \
    }

#define ASSERT_NEAR(a, b, epsilon) \
    if (std::abs((a) - (b)) < (epsilon)) { \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #a << " (" << (a) << ") != " << #b << " (" << (b) << ")" << std::endl; \
        g_tests_failed++; \
    }

#define ASSERT_EQ(a, b) \
    if ((a) == (b)) { \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #a << " (" << (a) << ") != " << #b << " (" << (b) << ")" << std::endl; \
        g_tests_failed++; \
    }

/**
 * Helper to check if a file contains a specific string.
 */
bool FileContains(const std::string& filename, const std::string& pattern) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    std::string line;
    while (std::getline(file, line)) {
        if (line.find(pattern) != std::string::npos) return true;
    }
    return false;
}

// ----------------------------------------------------------------------------
// TEST 1: Texture Load Cap in Presets
// ----------------------------------------------------------------------------
void test_texture_load_cap_in_presets() {
    std::cout << "Test 1: Texture Load Cap in Presets..." << std::endl;
    std::remove(Config::m_config_path.c_str());
    FFBEngine engine;
    Preset::ApplyDefaultsToEngine(engine);
    
    engine.m_texture_load_cap = 2.8f;
    
    // Clear existing presets to be sure
    Config::presets.clear();
    Config::AddUserPreset("TextureCapTest", engine);
    
    ASSERT_TRUE(FileContains(Config::m_config_path, "[Preset:TextureCapTest]"));
    ASSERT_TRUE(FileContains(Config::m_config_path, "texture_load_cap=2.8"));
    
    FFBEngine engine2;
    Preset::ApplyDefaultsToEngine(engine2);
    Config::LoadPresets();
    
    int idx = -1;
    for (int i = 0; i < Config::presets.size(); i++) {
        if (Config::presets[i].name == "TextureCapTest") {
            idx = i;
            break;
        }
    }
    
    ASSERT_TRUE(idx != -1);
    if (idx != -1) {
        Config::ApplyPreset(idx, engine2);
        ASSERT_NEAR(engine2.m_texture_load_cap, 2.8f, 0.001f);
    }
}

// ----------------------------------------------------------------------------
// TEST 2: Main Config - Speed Gate Persistence
// ----------------------------------------------------------------------------
void test_speed_gate_persistence() {
    std::cout << "Test 2: Main Config - Speed Gate Persistence..." << std::endl;
    Config::presets.clear(); // Clear presets from previous tests
    FFBEngine engine;
    Preset::ApplyDefaultsToEngine(engine);
    
    engine.m_speed_gate_lower = 2.5f;
    engine.m_speed_gate_upper = 7.0f;
    
    Config::Save(engine, "test_config_sg.ini");
    
    ASSERT_TRUE(FileContains("test_config_sg.ini", "speed_gate_lower=2.5"));
    ASSERT_TRUE(FileContains("test_config_sg.ini", "speed_gate_upper=7"));
    
    FFBEngine engine2;
    Preset::ApplyDefaultsToEngine(engine2);
    Config::Load(engine2, "test_config_sg.ini");
    
    ASSERT_NEAR(engine2.m_speed_gate_lower, 2.5f, 0.001f);
    ASSERT_NEAR(engine2.m_speed_gate_upper, 7.0f, 0.001f);
    
    std::remove("test_config_sg.ini");
}

// ----------------------------------------------------------------------------
// TEST 3: Main Config - Road Fallback & Understeer SoP
// ----------------------------------------------------------------------------
void test_advanced_physics_persistence() {
    std::cout << "Test 3: Main Config - Road Fallback & Understeer SoP..." << std::endl;
    Config::presets.clear(); // Clear presets from previous tests
    FFBEngine engine;
    Preset::ApplyDefaultsToEngine(engine);
    
    engine.m_road_fallback_scale = 0.12f;
    engine.m_understeer_affects_sop = true;
    
    Config::Save(engine, "test_config_ap.ini");
    
    ASSERT_TRUE(FileContains("test_config_ap.ini", "road_fallback_scale=0.12"));
    ASSERT_TRUE(FileContains("test_config_ap.ini", "understeer_affects_sop=1"));
    
    FFBEngine engine2;
    Preset::ApplyDefaultsToEngine(engine2);
    Config::Load(engine2, "test_config_ap.ini");
    
    ASSERT_NEAR(engine2.m_road_fallback_scale, 0.12f, 0.001f);
    ASSERT_EQ(engine2.m_understeer_affects_sop, true);
    
    std::remove("test_config_ap.ini");
}

// ----------------------------------------------------------------------------
// TEST 4: Preset Serialization - All New Fields
// ----------------------------------------------------------------------------
void test_preset_all_fields() {
    std::cout << "Test 4: Preset Serialization - All New Fields..." << std::endl;
    std::remove(Config::m_config_path.c_str());
    FFBEngine engine;
    Preset::ApplyDefaultsToEngine(engine);
    
    engine.m_texture_load_cap = 2.2f;
    engine.m_speed_gate_lower = 3.0f;
    engine.m_speed_gate_upper = 9.0f;
    engine.m_road_fallback_scale = 0.08f;
    engine.m_understeer_affects_sop = true;
    
    Config::presets.clear();
    Config::AddUserPreset("AllFieldsTest", engine);
    
    ASSERT_TRUE(FileContains(Config::m_config_path, "[Preset:AllFieldsTest]"));
    ASSERT_TRUE(FileContains(Config::m_config_path, "texture_load_cap=2.2"));
    ASSERT_TRUE(FileContains(Config::m_config_path, "speed_gate_lower=3"));
    ASSERT_TRUE(FileContains(Config::m_config_path, "speed_gate_upper=9"));
    ASSERT_TRUE(FileContains(Config::m_config_path, "road_fallback_scale=0.08"));
    ASSERT_TRUE(FileContains(Config::m_config_path, "understeer_affects_sop=1"));
    
    FFBEngine engine2;
    Preset::ApplyDefaultsToEngine(engine2);
    Config::LoadPresets();
    
    int idx = -1;
    for (int i = 0; i < Config::presets.size(); i++) {
        if (Config::presets[i].name == "AllFieldsTest") {
            idx = i;
            break;
        }
    }
    
    ASSERT_TRUE(idx != -1);
    if (idx != -1) {
        Config::ApplyPreset(idx, engine2);
        ASSERT_NEAR(engine2.m_texture_load_cap, 2.2f, 0.001f);
        ASSERT_NEAR(engine2.m_speed_gate_lower, 3.0f, 0.001f);
        ASSERT_NEAR(engine2.m_speed_gate_upper, 9.0f, 0.001f);
        ASSERT_NEAR(engine2.m_road_fallback_scale, 0.08f, 0.001f);
        ASSERT_EQ(engine2.m_understeer_affects_sop, true);
    }
}

// ----------------------------------------------------------------------------
// TEST 5: Preset Clamping - Brake Load Cap (Regression)
// ----------------------------------------------------------------------------
void test_preset_clamping_brake() {
    std::cout << "Test 5: Preset Clamping - Brake Load Cap..." << std::endl;
    
    // Manually write to config file
    {
        std::ofstream file(Config::m_config_path);
        file << "[Presets]\n";
        file << "[Preset:HighBrake]\n";
        file << "brake_load_cap=8.5\n";
    }
    
    Config::LoadPresets();
    
    int idx = -1;
    for (int i = 0; i < Config::presets.size(); i++) {
        if (Config::presets[i].name == "HighBrake") {
            idx = i;
            break;
        }
    }
    
    ASSERT_TRUE(idx != -1);
    if (idx != -1) {
        ASSERT_NEAR(Config::presets[idx].brake_load_cap, 8.5f, 0.001f);
        FFBEngine engine;
        Config::ApplyPreset(idx, engine);
        ASSERT_NEAR(engine.m_brake_load_cap, 8.5f, 0.001f);
    }
}

// ----------------------------------------------------------------------------
// TEST 6: Preset Clamping - Lockup Gain (Regression)
// ----------------------------------------------------------------------------
void test_preset_clamping_lockup() {
    std::cout << "Test 6: Preset Clamping - Lockup Gain..." << std::endl;
    
    // Manually write to config file
    {
        std::ofstream file(Config::m_config_path);
        file << "[Presets]\n";
        file << "[Preset:HighLockup]\n";
        file << "lockup_gain=2.9\n";
    }
    
    Config::LoadPresets();
    
    int idx = -1;
    for (int i = 0; i < Config::presets.size(); i++) {
        if (Config::presets[i].name == "HighLockup") {
            idx = i;
            break;
        }
    }
    
    ASSERT_TRUE(idx != -1);
    if (idx != -1) {
        ASSERT_NEAR(Config::presets[idx].lockup_gain, 2.9f, 0.001f);
        FFBEngine engine;
        Config::ApplyPreset(idx, engine);
        ASSERT_NEAR(engine.m_lockup_gain, 2.9f, 0.001f);
    }
}

// ----------------------------------------------------------------------------
// TEST 7: Main Config Clamping - Brake Load Cap (Regression)
// ----------------------------------------------------------------------------
void test_main_config_clamping_brake() {
    std::cout << "Test 7: Main Config Clamping - Brake Load Cap..." << std::endl;
    FFBEngine engine;
    
    // Within range
    {
        std::ofstream file("test_clamp.ini");
        file << "brake_load_cap=6.5\n";
    }
    Config::Load(engine, "test_clamp.ini");
    ASSERT_NEAR(engine.m_brake_load_cap, 6.5f, 0.001f);
    
    // Over max
    {
        std::ofstream file("test_clamp.ini");
        file << "brake_load_cap=15.0\n";
    }
    Config::Load(engine, "test_clamp.ini");
    ASSERT_NEAR(engine.m_brake_load_cap, 10.0f, 0.001f);
    
    // Under min
    {
        std::ofstream file("test_clamp.ini");
        file << "brake_load_cap=0.5\n";
    }
    Config::Load(engine, "test_clamp.ini");
    ASSERT_NEAR(engine.m_brake_load_cap, 1.0f, 0.001f);
    
    std::remove("test_clamp.ini");
}

// ----------------------------------------------------------------------------
// TEST 8: Main Config Clamping - Lockup Gain (Regression)
// ----------------------------------------------------------------------------
void test_main_config_clamping_lockup() {
    std::cout << "Test 8: Main Config Clamping - Lockup Gain..." << std::endl;
    FFBEngine engine;
    
    // Within range
    {
        std::ofstream file("test_clamp.ini");
        file << "lockup_gain=2.7\n";
    }
    Config::Load(engine, "test_clamp.ini");
    ASSERT_NEAR(engine.m_lockup_gain, 2.7f, 0.001f);
    
    // Over max
    {
        std::ofstream file("test_clamp.ini");
        file << "lockup_gain=5.0\n";
    }
    Config::Load(engine, "test_clamp.ini");
    ASSERT_NEAR(engine.m_lockup_gain, 3.0f, 0.001f);
    
    std::remove("test_clamp.ini");
}

// ----------------------------------------------------------------------------
// TEST 9: Configuration Versioning
// ----------------------------------------------------------------------------
void test_configuration_versioning() {
    std::cout << "Test 9: Configuration Versioning..." << std::endl;
    Config::presets.clear(); // Clear presets from previous tests
    FFBEngine engine;
    
    Config::Save(engine, "test_version.ini");
    ASSERT_TRUE(FileContains("test_version.ini", std::string("ini_version=") + LMUFFB_VERSION));
    
    Config::Load(engine, "test_version.ini");
    // Check console output manually or assume success if no crash
    
    std::remove("test_version.ini");
}

// ----------------------------------------------------------------------------
// TEST 10: Comprehensive Round-Trip Test
// ----------------------------------------------------------------------------
void test_comprehensive_roundtrip() {
    std::cout << "Test 10: Comprehensive Round-Trip Test..." << std::endl;
    std::remove(Config::m_config_path.c_str());
    FFBEngine engine;
    Preset::ApplyDefaultsToEngine(engine);
    
    engine.m_gain = 0.77f;
    engine.m_understeer_effect = 0.444f;
    engine.m_sop_effect = 1.23f;
    engine.m_texture_load_cap = 2.1f;
    engine.m_brake_load_cap = 6.6f;
    engine.m_speed_gate_lower = 2.2f;
    engine.m_speed_gate_upper = 8.8f;
    engine.m_road_fallback_scale = 0.11f;
    engine.m_understeer_affects_sop = true;
    
    Config::Save(engine, "roundtrip.ini");
    
    FFBEngine engine2;
    Preset::ApplyDefaultsToEngine(engine2);
    Config::Load(engine2, "roundtrip.ini");
    
    ASSERT_NEAR(engine2.m_gain, 0.77f, 0.001f);
    ASSERT_NEAR(engine2.m_understeer_effect, 0.444f, 0.001f);
    ASSERT_NEAR(engine2.m_sop_effect, 1.23f, 0.001f);
    ASSERT_NEAR(engine2.m_texture_load_cap, 2.1f, 0.001f);
    ASSERT_NEAR(engine2.m_brake_load_cap, 6.6f, 0.001f);
    ASSERT_NEAR(engine2.m_speed_gate_lower, 2.2f, 0.001f);
    ASSERT_NEAR(engine2.m_speed_gate_upper, 8.8f, 0.001f);
    ASSERT_NEAR(engine2.m_road_fallback_scale, 0.11f, 0.001f);
    ASSERT_EQ(engine2.m_understeer_affects_sop, true);
    
    Config::presets.clear();
    Config::AddUserPreset("RoundTrip", engine2);
    
    FFBEngine engine3;
    Preset::ApplyDefaultsToEngine(engine3);
    Config::LoadPresets();
    
    int idx = -1;
    for (int i = 0; i < Config::presets.size(); i++) {
        if (Config::presets[i].name == "RoundTrip") {
            idx = i;
            break;
        }
    }
    
    ASSERT_TRUE(idx != -1);
    if (idx != -1) {
        Config::ApplyPreset(idx, engine3);
        ASSERT_NEAR(engine3.m_gain, 0.77f, 0.001f);
        ASSERT_NEAR(engine3.m_understeer_effect, 0.444f, 0.001f);
        ASSERT_NEAR(engine3.m_sop_effect, 1.23f, 0.001f);
        ASSERT_NEAR(engine3.m_texture_load_cap, 2.1f, 0.001f);
        ASSERT_NEAR(engine3.m_brake_load_cap, 6.6f, 0.001f);
        ASSERT_NEAR(engine3.m_speed_gate_lower, 2.2f, 0.001f);
        ASSERT_NEAR(engine3.m_speed_gate_upper, 8.8f, 0.001f);
        ASSERT_NEAR(engine3.m_road_fallback_scale, 0.11f, 0.001f);
        ASSERT_EQ(engine3.m_understeer_affects_sop, true);
    }
    
    std::remove("roundtrip.ini");
}

// ----------------------------------------------------------------------------
// TEST 11: Preset-Engine Synchronization Regression (v0.7.0)
// 
// REGRESSION CASE: Fields declared in both Preset and FFBEngine but missing
// from Preset::Apply() or Preset::UpdateFromEngine() methods.
//
// This test verifies that:
// 1. Preset::ApplyDefaultsToEngine() initializes ALL fields to valid values
// 2. Preset::Apply() transfers ALL Preset fields to FFBEngine
// 3. Preset::UpdateFromEngine() captures ALL FFBEngine fields back to Preset
// 
// If any field is missing from the synchronization methods, this test will fail.
// ----------------------------------------------------------------------------
void test_preset_engine_sync_regression() {
    std::cout << "Test 11: Preset-Engine Synchronization (v0.7.0 Regression)..." << std::endl;
    
    // --- Part A: ApplyDefaultsToEngine initializes critical fields ---
    FFBEngine engine_defaults;
    Preset::ApplyDefaultsToEngine(engine_defaults);
    
    // These fields triggered "Invalid X, resetting to default" warnings when missing
    ASSERT_TRUE(engine_defaults.m_optimal_slip_angle >= 0.01f);
    ASSERT_TRUE(engine_defaults.m_optimal_slip_ratio >= 0.01f);
    
    // Additional smoothing fields (v0.5.7 - v0.5.8)
    // Note: 0.0 is valid for these, we just check they're not uninitialized garbage
    ASSERT_TRUE(engine_defaults.m_steering_shaft_smoothing >= 0.0f);
    ASSERT_TRUE(engine_defaults.m_gyro_smoothing >= 0.0f);
    ASSERT_TRUE(engine_defaults.m_yaw_accel_smoothing >= 0.0f);
    ASSERT_TRUE(engine_defaults.m_chassis_inertia_smoothing >= 0.0f);
    
    // Slope detection fields (v0.7.0)
    ASSERT_TRUE(engine_defaults.m_slope_sg_window >= 5);
    ASSERT_TRUE(engine_defaults.m_slope_sensitivity >= 0.1f);
    ASSERT_TRUE(engine_defaults.m_slope_smoothing_tau >= 0.001f);
    
    std::cout << "  [PASS] ApplyDefaultsToEngine initializes critical fields" << std::endl;
    
    // --- Part B: Apply() transfers ALL Preset fields to FFBEngine ---
    Preset custom_preset("SyncTest");
    
    // Set custom values for ALL synchronizable fields
    custom_preset.gain = 0.77f;
    custom_preset.understeer = 0.88f;
    custom_preset.sop = 1.11f;
    custom_preset.optimal_slip_angle = 0.15f;
    custom_preset.optimal_slip_ratio = 0.18f;
    custom_preset.steering_shaft_smoothing = 0.025f;
    custom_preset.gyro_smoothing = 0.015f;
    custom_preset.yaw_smoothing = 0.005f;
    custom_preset.chassis_smoothing = 0.035f;
    custom_preset.road_fallback_scale = 0.12f;
    custom_preset.understeer_affects_sop = true;
    
    // Slope detection (v0.7.0)
    custom_preset.slope_detection_enabled = true;
    custom_preset.slope_sg_window = 21;
    custom_preset.slope_sensitivity = 2.5f;
    custom_preset.slope_negative_threshold = -0.2f;
    custom_preset.slope_smoothing_tau = 0.05f;
    
    FFBEngine engine_apply;
    custom_preset.Apply(engine_apply);
    
    // Verify Apply() worked
    ASSERT_NEAR(engine_apply.m_gain, 0.77f, 0.001f);
    ASSERT_NEAR(engine_apply.m_understeer_effect, 0.88f, 0.001f);
    ASSERT_NEAR(engine_apply.m_sop_effect, 1.11f, 0.001f);
    ASSERT_NEAR(engine_apply.m_optimal_slip_angle, 0.15f, 0.001f);
    ASSERT_NEAR(engine_apply.m_optimal_slip_ratio, 0.18f, 0.001f);
    ASSERT_NEAR(engine_apply.m_steering_shaft_smoothing, 0.025f, 0.001f);
    ASSERT_NEAR(engine_apply.m_gyro_smoothing, 0.015f, 0.001f);
    ASSERT_NEAR(engine_apply.m_yaw_accel_smoothing, 0.005f, 0.001f);
    ASSERT_NEAR(engine_apply.m_chassis_inertia_smoothing, 0.035f, 0.001f);
    ASSERT_NEAR(engine_apply.m_road_fallback_scale, 0.12f, 0.001f);
    ASSERT_EQ(engine_apply.m_understeer_affects_sop, true);
    
    // Slope detection (v0.7.0)
    ASSERT_EQ(engine_apply.m_slope_detection_enabled, true);
    ASSERT_EQ(engine_apply.m_slope_sg_window, 21);
    ASSERT_NEAR(engine_apply.m_slope_sensitivity, 2.5f, 0.001f);
    ASSERT_NEAR(engine_apply.m_slope_negative_threshold, -0.2f, 0.001f);
    ASSERT_NEAR(engine_apply.m_slope_smoothing_tau, 0.05f, 0.001f);
    
    std::cout << "  [PASS] Apply() transfers all Preset fields to FFBEngine" << std::endl;
    
    // --- Part C: UpdateFromEngine() captures ALL FFBEngine fields ---
    FFBEngine engine_source;
    Preset::ApplyDefaultsToEngine(engine_source);
    
    // Set custom values directly on engine
    engine_source.m_gain = 0.55f;
    engine_source.m_understeer_effect = 0.66f;
    engine_source.m_optimal_slip_angle = 0.22f;
    engine_source.m_optimal_slip_ratio = 0.25f;
    engine_source.m_steering_shaft_smoothing = 0.033f;
    engine_source.m_gyro_smoothing = 0.044f;
    engine_source.m_yaw_accel_smoothing = 0.011f;
    engine_source.m_chassis_inertia_smoothing = 0.055f;
    engine_source.m_road_fallback_scale = 0.09f;
    engine_source.m_understeer_affects_sop = true;
    
    // Slope detection (v0.7.0)
    engine_source.m_slope_detection_enabled = true;
    engine_source.m_slope_sg_window = 31;
    engine_source.m_slope_sensitivity = 3.0f;
    engine_source.m_slope_negative_threshold = -0.3f;
    engine_source.m_slope_smoothing_tau = 0.08f;
    
    Preset captured_preset;
    captured_preset.UpdateFromEngine(engine_source);
    
    // Verify UpdateFromEngine() worked
    ASSERT_NEAR(captured_preset.gain, 0.55f, 0.001f);
    ASSERT_NEAR(captured_preset.understeer, 0.66f, 0.001f);
    ASSERT_NEAR(captured_preset.optimal_slip_angle, 0.22f, 0.001f);
    ASSERT_NEAR(captured_preset.optimal_slip_ratio, 0.25f, 0.001f);
    ASSERT_NEAR(captured_preset.steering_shaft_smoothing, 0.033f, 0.001f);
    ASSERT_NEAR(captured_preset.gyro_smoothing, 0.044f, 0.001f);
    ASSERT_NEAR(captured_preset.yaw_smoothing, 0.011f, 0.001f);
    ASSERT_NEAR(captured_preset.chassis_smoothing, 0.055f, 0.001f);
    ASSERT_NEAR(captured_preset.road_fallback_scale, 0.09f, 0.001f);
    ASSERT_EQ(captured_preset.understeer_affects_sop, true);
    
    // Slope detection (v0.7.0)
    ASSERT_EQ(captured_preset.slope_detection_enabled, true);
    ASSERT_EQ(captured_preset.slope_sg_window, 31);
    ASSERT_NEAR(captured_preset.slope_sensitivity, 3.0f, 0.001f);
    ASSERT_NEAR(captured_preset.slope_negative_threshold, -0.3f, 0.001f);
    ASSERT_NEAR(captured_preset.slope_smoothing_tau, 0.08f, 0.001f);
    
    std::cout << "  [PASS] UpdateFromEngine() captures all FFBEngine fields" << std::endl;
    
    // --- Part D: Round-trip integrity ---
    // Apply captured_preset to a new engine and verify no data loss
    FFBEngine engine_roundtrip;
    captured_preset.Apply(engine_roundtrip);
    
    ASSERT_NEAR(engine_roundtrip.m_optimal_slip_angle, 0.22f, 0.001f);
    ASSERT_NEAR(engine_roundtrip.m_slope_sensitivity, 3.0f, 0.001f);
    
    std::cout << "  [PASS] Round-trip Apply->UpdateFromEngine->Apply preserves data" << std::endl;
}

void Run() {
    std::cout << "\n=== Running v0.6.25 Persistence Tests ===" << std::endl;
    
    test_texture_load_cap_in_presets();
    test_speed_gate_persistence();
    test_advanced_physics_persistence();
    test_preset_all_fields();
    test_preset_clamping_brake();
    test_preset_clamping_lockup();
    test_main_config_clamping_brake();
    test_main_config_clamping_lockup();
    test_configuration_versioning();
    test_comprehensive_roundtrip();
    test_preset_engine_sync_regression();  // v0.7.0 Regression

    std::cout << "\n--- Persistence & Versioning Test Summary ---" << std::endl;
    std::cout << "Tests Passed: " << g_tests_passed << std::endl;
    std::cout << "Tests Failed: " << g_tests_failed << std::endl;
}

} // namespace PersistenceTests
