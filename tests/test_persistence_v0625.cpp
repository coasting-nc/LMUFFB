#include "test_ffb_common.h"
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include "src/ffb/FFBEngine.h"
#include "src/core/Config.h"
#include "src/Version.h"
#include <toml.hpp>

namespace FFBEngineTests {

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
// TEST 1: Texture Load Cap in Presets (Phase 2 TOML)
// ----------------------------------------------------------------------------
TEST_CASE(test_texture_load_cap_in_presets, "Persistence") {
    std::cout << "Test 1: Texture Load Cap in Presets..." << std::endl;
    TestDirectoryGuard temp_dir("tmp_test_texture");
    std::string test_file = temp_dir.path() + "/test_config_texture.toml";
    std::string original_path = Config::m_config_path;
    std::string original_user_presets = Config::m_user_presets_path;
    
    Config::m_config_path = test_file;
    Config::m_user_presets_path = temp_dir.path() + "/user_presets";

    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_vibration.texture_load_cap = 2.8f;
    
    Config::presets.clear();
    Config::AddUserPreset("TextureCapTest", engine);
    Config::Save(engine, test_file);
    
    // Explicitly check for file
    if (!std::filesystem::exists(test_file)) {
        FAIL_TEST("Test file not created: " << test_file << " (Current Dir: " << std::filesystem::current_path() << ")");
        return;
    }

    // In Phase 3, preset is in its own file
    std::string preset_file = Config::m_user_presets_path + "/TextureCapTest.toml";
    if (!std::filesystem::exists(preset_file)) {
        FAIL_TEST("User preset file not created: " << preset_file);
        return;
    }

    try {
        toml::table tbl = toml::parse_file(preset_file);
        ASSERT_TRUE(tbl.contains("Vibration"));
        auto vib_node = tbl["Vibration"].as_table();
        ASSERT_TRUE(vib_node != nullptr);
        ASSERT_TRUE(vib_node->contains("texture_load_cap"));
    } catch (const toml::parse_error& err) {
        FAIL_TEST("TOML parse error in preset: " << err.description());
    }
    
    FFBEngine engine2;
    InitializeEngine(engine2);
    Config::LoadPresets();
    
    int idx = -1;
    for (int i = 0; i < (int)Config::presets.size(); i++) {
        if (Config::presets[i].name == "TextureCapTest") {
            idx = i;
            break;
        }
    }
    
    ASSERT_TRUE(idx != -1);
    if (idx != -1) {
        Config::ApplyPreset(idx, engine2);
        ASSERT_NEAR(engine2.m_vibration.texture_load_cap, 2.8f, 0.001f);
    }
    Config::m_config_path = original_path;
    Config::m_user_presets_path = original_user_presets;
}

// ----------------------------------------------------------------------------
// TEST 2: Main Config - Speed Gate Persistence (Phase 2 TOML)
// ----------------------------------------------------------------------------
TEST_CASE(test_speed_gate_persistence, "Persistence") {
    std::cout << "Test 2: Main Config - Speed Gate Persistence..." << std::endl;
    Config::presets.clear();
    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_advanced.speed_gate_lower = 2.5f;
    engine.m_advanced.speed_gate_upper = 7.0f;
    
    TestDirectoryGuard temp_dir("tmp_test_sg");
    std::string test_file = temp_dir.path() + "/test_config_sg.toml";
    Config::Save(engine, test_file);
    
    ASSERT_TRUE(FileContains(test_file, "speed_gate_lower"));
    ASSERT_TRUE(FileContains(test_file, "speed_gate_upper"));
    
    FFBEngine engine2;
    InitializeEngine(engine2);
    Config::Load(engine2, test_file);
    
    ASSERT_NEAR(engine2.m_advanced.speed_gate_upper, 7.0f, 0.001f);
}

// ----------------------------------------------------------------------------
// TEST 3: Main Config - Road Fallback & Understeer SoP (Phase 2 TOML)
// ----------------------------------------------------------------------------
TEST_CASE(test_advanced_physics_persistence, "Persistence") {
    std::cout << "Test 3: Main Config - Road Fallback & Understeer SoP..." << std::endl;
    Config::presets.clear();
    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_advanced.road_fallback_scale = 0.12f;
    engine.m_advanced.understeer_affects_sop = true;
    
    TestDirectoryGuard temp_dir("tmp_test_ap");
    std::string test_file = temp_dir.path() + "/test_config_ap.toml";
    Config::Save(engine, test_file);
    
    ASSERT_TRUE(FileContains(test_file, "road_fallback_scale"));
    ASSERT_TRUE(FileContains(test_file, "understeer_affects_sop = true"));
    
    FFBEngine engine2;
    InitializeEngine(engine2);
    Config::Load(engine2, test_file);
    
    ASSERT_EQ(engine2.m_advanced.understeer_affects_sop, true);
}

// ----------------------------------------------------------------------------
// TEST 4: Preset Serialization - All New Fields (Phase 2 TOML)
// ----------------------------------------------------------------------------
TEST_CASE(test_preset_all_fields, "Persistence") {
    std::cout << "Test 4: Preset Serialization - All New Fields..." << std::endl;
    TestDirectoryGuard temp_dir("tmp_test_all");
    std::string test_file = temp_dir.path() + "/test_config_all.toml";
    std::string original_path = Config::m_config_path;
    std::string original_user_presets = Config::m_user_presets_path;
    
    Config::m_config_path = test_file;
    Config::m_user_presets_path = temp_dir.path() + "/user_presets";

    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_vibration.texture_load_cap = 2.2f;
    engine.m_advanced.speed_gate_lower = 3.0f;
    engine.m_advanced.speed_gate_upper = 9.0f;
    engine.m_advanced.road_fallback_scale = 0.08f;
    engine.m_advanced.understeer_affects_sop = true;
    
    Config::presets.clear();
    Config::AddUserPreset("AllFieldsTest", engine);
    
    ASSERT_TRUE(FileContains(test_file, "AllFieldsTest"));
    ASSERT_TRUE(FileContains(test_file, "texture_load_cap"));
    ASSERT_TRUE(FileContains(test_file, "speed_gate_lower"));
    ASSERT_TRUE(FileContains(test_file, "understeer_affects_sop = true"));
    
    FFBEngine engine2;
    InitializeEngine(engine2);
    Config::LoadPresets();
    
    int idx = -1;
    for (int i = 0; i < (int)Config::presets.size(); i++) {
        if (Config::presets[i].name == "AllFieldsTest") {
            idx = i;
            break;
        }
    }
    
    ASSERT_TRUE(idx != -1);
    if (idx != -1) {
        Config::ApplyPreset(idx, engine2);
        ASSERT_NEAR(engine2.m_vibration.texture_load_cap, 2.2f, 0.001f);
        ASSERT_NEAR(engine2.m_advanced.speed_gate_lower, 3.0f, 0.001f);
        ASSERT_NEAR(engine2.m_advanced.speed_gate_upper, 9.0f, 0.001f);
        ASSERT_NEAR(engine2.m_advanced.road_fallback_scale, 0.08f, 0.001f);
        ASSERT_EQ(engine2.m_advanced.understeer_affects_sop, true);
    }
    Config::m_config_path = original_path;
    Config::m_user_presets_path = original_user_presets;
}

// ----------------------------------------------------------------------------
// TEST 5: Preset Clamping - Brake Load Cap (Regression)
// ----------------------------------------------------------------------------
TEST_CASE(test_preset_clamping_brake, "Persistence") {
    std::cout << "Test 5: Preset Clamping - Brake Load Cap..." << std::endl;
    TestDirectoryGuard temp_dir("tmp_test_high_brake");
    std::string test_file = temp_dir.path() + "/user_presets/HighBrake.toml";
    std::string original_user_presets = Config::m_user_presets_path;
    Config::m_user_presets_path = temp_dir.path() + "/user_presets";
    
    // Manually write to config file
    {
        std::filesystem::create_directories(Config::m_user_presets_path);
        std::ofstream file(test_file);
        file << "[Braking]\n";
        file << "brake_load_cap = 8.5\n";
    }
    
    Config::LoadPresets();
    
    int idx = -1;
    for (int i = 0; i < (int)Config::presets.size(); i++) {
        if (Config::presets[i].name == "HighBrake") {
            idx = i;
            break;
        }
    }
    
    ASSERT_TRUE(idx != -1);
    if (idx != -1) {
        ASSERT_NEAR(Config::presets[idx].braking.brake_load_cap, 8.5f, 0.001f);
        FFBEngine engine;
        Config::ApplyPreset(idx, engine);
        ASSERT_NEAR(engine.m_braking.brake_load_cap, 8.5f, 0.001f);
    }
    Config::m_user_presets_path = original_user_presets;
}

// ----------------------------------------------------------------------------
// TEST 6: Preset Clamping - Lockup Gain (Regression)
// ----------------------------------------------------------------------------
TEST_CASE(test_preset_clamping_lockup, "Persistence") {
    std::cout << "Test 6: Preset Clamping - Lockup Gain..." << std::endl;
    TestDirectoryGuard temp_dir("tmp_test_high_lockup");
    std::string test_file = temp_dir.path() + "/user_presets/HighLockup.toml";
    std::string original_user_presets = Config::m_user_presets_path;
    Config::m_user_presets_path = temp_dir.path() + "/user_presets";
    
    // Manually write to config file
    {
        std::filesystem::create_directories(Config::m_user_presets_path);
        std::ofstream file(test_file);
        file << "[Braking]\n";
        file << "lockup_gain = 2.9\n";
    }
    
    Config::LoadPresets();
    
    int idx = -1;
    for (int i = 0; i < (int)Config::presets.size(); i++) {
        if (Config::presets[i].name == "HighLockup") {
            idx = i;
            break;
        }
    }
    
    ASSERT_TRUE(idx != -1);
    if (idx != -1) {
        ASSERT_NEAR(Config::presets[idx].braking.lockup_gain, 2.9f, 0.001f);
        FFBEngine engine;
        Config::ApplyPreset(idx, engine);
        ASSERT_NEAR(engine.m_braking.lockup_gain, 2.9f, 0.001f);
    }
    Config::m_user_presets_path = original_user_presets;
}

// ----------------------------------------------------------------------------
// TEST 7: Main Config Clamping - Brake Load Cap (Regression)
// ----------------------------------------------------------------------------
TEST_CASE(test_main_config_clamping_brake, "Persistence") {
    std::cout << "Test 7: Main Config Clamping - Brake Load Cap..." << std::endl;
    FFBEngine engine;
    TestDirectoryGuard temp_dir("tmp_test_clamp_brake");
    std::string test_file = temp_dir.path() + "/test_clamp_brake.toml";
    
    // Within range
    {
        std::ofstream file(test_file);
        file << "[Braking]\nbrake_load_cap = 6.5\n";
    }
    Config::Load(engine, test_file);
    ASSERT_NEAR(engine.m_braking.brake_load_cap, 6.5f, 0.001f);
    
    // Over max
    {
        std::ofstream file(test_file);
        file << "[Braking]\nbrake_load_cap = 15.0\n";
    }
    Config::Load(engine, test_file);
    ASSERT_NEAR(engine.m_braking.brake_load_cap, 10.0f, 0.001f);
    
    // Under min
    {
        std::ofstream file(test_file);
        file << "[Braking]\nbrake_load_cap = 0.5\n";
    }
    Config::Load(engine, test_file);
    ASSERT_NEAR(engine.m_braking.brake_load_cap, 1.0f, 0.001f);
}

// ----------------------------------------------------------------------------
// TEST 8: Main Config Clamping - Lockup Gain (Regression)
// ----------------------------------------------------------------------------
TEST_CASE(test_main_config_clamping_lockup, "Persistence") {
    std::cout << "Test 8: Main Config Clamping - Lockup Gain..." << std::endl;
    FFBEngine engine;
    TestDirectoryGuard temp_dir("tmp_test_clamp_lockup");
    std::string test_file = temp_dir.path() + "/test_clamp_lockup.toml";
    
    // Within range
    {
        std::ofstream file(test_file);
        file << "[Braking]\nlockup_gain = 2.7\n";
    }
    Config::Load(engine, test_file);
    ASSERT_NEAR(engine.m_braking.lockup_gain, 2.7f, 0.001f);
    
    // Over max
    {
        std::ofstream file(test_file);
        file << "[Braking]\nlockup_gain = 5.0\n";
    }
    Config::Load(engine, test_file);
    ASSERT_NEAR(engine.m_braking.lockup_gain, 3.0f, 0.001f);
}

// ----------------------------------------------------------------------------
// TEST 9: Configuration Versioning
// ----------------------------------------------------------------------------
TEST_CASE(test_configuration_versioning, "Persistence") {
    std::cout << "Test 9: Configuration Versioning..." << std::endl;
    Config::presets.clear();
    FFBEngine engine;
    
    TestDirectoryGuard temp_dir("tmp_test_version");
    std::string test_file = temp_dir.path() + "/test_version.toml";
    Config::Save(engine, test_file);
    ASSERT_TRUE(FileContains(test_file, "app_version"));
    
    Config::Load(engine, test_file);
}

// ----------------------------------------------------------------------------
// TEST 10: Comprehensive Round-Trip Test (Phase 2 TOML)
// ----------------------------------------------------------------------------
TEST_CASE(test_comprehensive_roundtrip, "Persistence") {
    std::cout << "Test 10: Comprehensive Round-Trip Test..." << std::endl;
    TestDirectoryGuard temp_dir("tmp_test_roundtrip");
    std::string test_file = temp_dir.path() + "/roundtrip.toml";
    std::string original_user_presets = Config::m_user_presets_path;
    Config::m_user_presets_path = temp_dir.path() + "/user_presets";

    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_general.gain = 0.77f;
    engine.m_front_axle.understeer_effect = 0.444f;
    engine.m_rear_axle.sop_effect = 1.23f;
    engine.m_vibration.texture_load_cap = 2.1f;
    engine.m_braking.brake_load_cap = 6.6f;
    engine.m_advanced.speed_gate_lower = 2.2f;
    engine.m_advanced.speed_gate_upper = 8.8f;
    engine.m_advanced.road_fallback_scale = 0.11f;
    engine.m_advanced.understeer_affects_sop = true;
    
    Config::Save(engine, test_file);
    
    FFBEngine engine2;
    InitializeEngine(engine2);
    Config::Load(engine2, test_file);
    
    ASSERT_NEAR(engine2.m_general.gain, 0.77f, 0.001f);
    ASSERT_NEAR(engine2.m_front_axle.understeer_effect, 0.444f, 0.001f);
    ASSERT_NEAR(engine2.m_rear_axle.sop_effect, 1.23f, 0.001f);
    ASSERT_NEAR(engine2.m_vibration.texture_load_cap, 2.1f, 0.001f);
    ASSERT_NEAR(engine2.m_braking.brake_load_cap, 6.6f, 0.001f);
    ASSERT_NEAR(engine2.m_advanced.speed_gate_lower, 2.2f, 0.001f);
    ASSERT_NEAR(engine2.m_advanced.speed_gate_upper, 8.8f, 0.001f);
    ASSERT_NEAR(engine2.m_advanced.road_fallback_scale, 0.11f, 0.001f);
    ASSERT_EQ(engine2.m_advanced.understeer_affects_sop, true);
    
    Config::presets.clear();
    Config::AddUserPreset("RoundTrip", engine2);
    Config::Save(engine2, test_file);
    
    FFBEngine engine3;
    InitializeEngine(engine3);
    Config::LoadPresets();
    
    int idx = -1;
    for (int i = 0; i < (int)Config::presets.size(); i++) {
        if (Config::presets[i].name == "RoundTrip") {
            idx = i;
            break;
        }
    }
    
    ASSERT_TRUE(idx != -1);
    if (idx != -1) {
        Config::ApplyPreset(idx, engine3);
        ASSERT_NEAR(engine3.m_general.gain, 0.77f, 0.001f);
        ASSERT_NEAR(engine3.m_front_axle.understeer_effect, 0.444f, 0.001f);
        ASSERT_NEAR(engine3.m_rear_axle.sop_effect, 1.23f, 0.001f);
        ASSERT_NEAR(engine3.m_vibration.texture_load_cap, 2.1f, 0.001f);
        ASSERT_NEAR(engine3.m_braking.brake_load_cap, 6.6f, 0.001f);
        ASSERT_NEAR(engine3.m_advanced.speed_gate_lower, 2.2f, 0.001f);
        ASSERT_NEAR(engine3.m_advanced.speed_gate_upper, 8.8f, 0.001f);
        ASSERT_NEAR(engine3.m_advanced.road_fallback_scale, 0.11f, 0.001f);
        ASSERT_EQ(engine3.m_advanced.understeer_affects_sop, true);
    }
    Config::m_user_presets_path = original_user_presets;
}

// ----------------------------------------------------------------------------
// TEST 11: Preset-Engine Synchronization Regression (v0.7.0)
// ----------------------------------------------------------------------------
TEST_CASE(test_preset_engine_sync_regression, "Persistence") {
    std::cout << "Test 11: Preset-Engine Synchronization (v0.7.0 Regression)..." << std::endl;
    
    FFBEngine engine_defaults;
    Preset::ApplyDefaultsToEngine(engine_defaults);
    
    ASSERT_TRUE(engine_defaults.m_grip_estimation.optimal_slip_angle >= 0.01f);
    ASSERT_TRUE(engine_defaults.m_grip_estimation.optimal_slip_ratio >= 0.01f);
    ASSERT_TRUE(engine_defaults.m_front_axle.steering_shaft_smoothing >= 0.0f);
    ASSERT_TRUE(engine_defaults.m_advanced.gyro_smoothing >= 0.0f);
    ASSERT_TRUE(engine_defaults.m_rear_axle.yaw_accel_smoothing >= 0.0f);
    ASSERT_TRUE(engine_defaults.m_grip_estimation.chassis_inertia_smoothing >= 0.0f);
    ASSERT_TRUE(engine_defaults.m_slope_detection.sg_window >= 5);
    ASSERT_TRUE(engine_defaults.m_slope_detection.sensitivity >= 0.1f);
    ASSERT_TRUE(engine_defaults.m_slope_detection.smoothing_tau >= 0.001f);
    
    Preset custom_preset("SyncTest");
    custom_preset.general.gain = 0.77f;
    custom_preset.front_axle.understeer_effect = 0.88f;
    custom_preset.rear_axle.sop_effect = 1.11f;
    custom_preset.grip_estimation.optimal_slip_angle = 0.15f;
    custom_preset.grip_estimation.optimal_slip_ratio = 0.18f;
    custom_preset.front_axle.steering_shaft_smoothing = 0.025f;
    custom_preset.advanced.gyro_smoothing = 0.015f;
    custom_preset.rear_axle.yaw_accel_smoothing = 0.005f;
    custom_preset.grip_estimation.chassis_inertia_smoothing = 0.035f;
    custom_preset.advanced.road_fallback_scale = 0.12f;
    custom_preset.advanced.understeer_affects_sop = true;
    custom_preset.slope_detection.enabled = true;
    custom_preset.slope_detection.sg_window = 21;
    custom_preset.slope_detection.sensitivity = 2.5f;
    custom_preset.slope_detection.min_threshold = -0.2f;
    custom_preset.slope_detection.smoothing_tau = 0.05f;
    
    FFBEngine engine_apply;
    custom_preset.Apply(engine_apply);
    
    ASSERT_NEAR(engine_apply.m_general.gain, 0.77f, 0.001f);
    ASSERT_NEAR(engine_apply.m_front_axle.understeer_effect, 0.88f, 0.001f);
    ASSERT_NEAR(engine_apply.m_rear_axle.sop_effect, 1.11f, 0.001f);
    ASSERT_NEAR(engine_apply.m_grip_estimation.optimal_slip_angle, 0.15f, 0.001f);
    ASSERT_NEAR(engine_apply.m_grip_estimation.optimal_slip_ratio, 0.18f, 0.001f);
    ASSERT_NEAR(engine_apply.m_front_axle.steering_shaft_smoothing, 0.025f, 0.001f);
    ASSERT_NEAR(engine_apply.m_advanced.gyro_smoothing, 0.015f, 0.001f);
    ASSERT_NEAR(engine_apply.m_rear_axle.yaw_accel_smoothing, 0.005f, 0.001f);
    ASSERT_NEAR(engine_apply.m_grip_estimation.chassis_inertia_smoothing, 0.035f, 0.001f);
    ASSERT_NEAR(engine_apply.m_advanced.road_fallback_scale, 0.12f, 0.001f);
    ASSERT_EQ(engine_apply.m_advanced.understeer_affects_sop, true);
    ASSERT_EQ(engine_apply.m_slope_detection.enabled, true);
    ASSERT_EQ(engine_apply.m_slope_detection.sg_window, 21);
    ASSERT_NEAR(engine_apply.m_slope_detection.sensitivity, 2.5f, 0.001f);
    ASSERT_NEAR(engine_apply.m_slope_detection.min_threshold, -0.2f, 0.001f);
    ASSERT_NEAR(engine_apply.m_slope_detection.smoothing_tau, 0.05f, 0.001f);
    
    FFBEngine engine_source;
    InitializeEngine(engine_source);
    engine_source.m_general.gain = 0.55f;
    engine_source.m_front_axle.understeer_effect = 0.66f;
    engine_source.m_grip_estimation.optimal_slip_angle = 0.22f;
    engine_source.m_grip_estimation.optimal_slip_ratio = 0.25f;
    engine_source.m_front_axle.steering_shaft_smoothing = 0.033f;
    engine_source.m_advanced.gyro_smoothing = 0.044f;
    engine_source.m_rear_axle.yaw_accel_smoothing = 0.011f;
    engine_source.m_grip_estimation.chassis_inertia_smoothing = 0.055f;
    engine_source.m_advanced.road_fallback_scale = 0.09f;
    engine_source.m_advanced.understeer_affects_sop = true;
    engine_source.m_slope_detection.enabled = true;
    engine_source.m_slope_detection.sg_window = 31;
    engine_source.m_slope_detection.sensitivity = 3.0f;
    engine_source.m_slope_detection.min_threshold = -0.3f;
    engine_source.m_slope_detection.smoothing_tau = 0.08f;
    
    Preset captured_preset;
    captured_preset.UpdateFromEngine(engine_source);
    
    ASSERT_NEAR(captured_preset.general.gain, 0.55f, 0.001f);
    ASSERT_NEAR(captured_preset.front_axle.understeer_effect, 0.66f, 0.001f);
    ASSERT_NEAR(captured_preset.grip_estimation.optimal_slip_angle, 0.22f, 0.001f);
    ASSERT_NEAR(captured_preset.grip_estimation.optimal_slip_ratio, 0.25f, 0.001f);
    ASSERT_NEAR(captured_preset.front_axle.steering_shaft_smoothing, 0.033f, 0.001f);
    ASSERT_NEAR(captured_preset.advanced.gyro_smoothing, 0.044f, 0.001f);
    ASSERT_NEAR(captured_preset.rear_axle.yaw_accel_smoothing, 0.011f, 0.001f);
    ASSERT_NEAR(captured_preset.grip_estimation.chassis_inertia_smoothing, 0.055f, 0.001f);
    ASSERT_NEAR(captured_preset.advanced.road_fallback_scale, 0.09f, 0.001f);
    ASSERT_EQ(captured_preset.advanced.understeer_affects_sop, true);
    ASSERT_EQ(captured_preset.slope_detection.enabled, true);
    ASSERT_EQ(captured_preset.slope_detection.sg_window, 31);
    ASSERT_NEAR(captured_preset.slope_detection.sensitivity, 3.0f, 0.001f);
    ASSERT_NEAR(captured_preset.slope_detection.min_threshold, -0.3f, 0.001f);
    ASSERT_NEAR(captured_preset.slope_detection.smoothing_tau, 0.08f, 0.001f);
    
    FFBEngine engine_roundtrip;
    captured_preset.Apply(engine_roundtrip);
    ASSERT_NEAR(engine_roundtrip.m_grip_estimation.optimal_slip_angle, 0.22f, 0.001f);
    ASSERT_NEAR(engine_roundtrip.m_slope_detection.sensitivity, 3.0f, 0.001f);
}

} // namespace FFBEngineTests
