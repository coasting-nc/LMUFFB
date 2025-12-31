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
    std::remove("config.ini");
    FFBEngine engine;
    Preset::ApplyDefaultsToEngine(engine);
    
    engine.m_texture_load_cap = 2.8f;
    
    // Clear existing presets to be sure
    Config::presets.clear();
    Config::AddUserPreset("TextureCapTest", engine);
    
    ASSERT_TRUE(FileContains("config.ini", "[Preset:TextureCapTest]"));
    ASSERT_TRUE(FileContains("config.ini", "texture_load_cap=2.8"));
    
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
    std::remove("config.ini");
    FFBEngine engine;
    Preset::ApplyDefaultsToEngine(engine);
    
    engine.m_texture_load_cap = 2.2f;
    engine.m_speed_gate_lower = 3.0f;
    engine.m_speed_gate_upper = 9.0f;
    engine.m_road_fallback_scale = 0.08f;
    engine.m_understeer_affects_sop = true;
    
    Config::presets.clear();
    Config::AddUserPreset("AllFieldsTest", engine);
    
    ASSERT_TRUE(FileContains("config.ini", "[Preset:AllFieldsTest]"));
    ASSERT_TRUE(FileContains("config.ini", "texture_load_cap=2.2"));
    ASSERT_TRUE(FileContains("config.ini", "speed_gate_lower=3"));
    ASSERT_TRUE(FileContains("config.ini", "speed_gate_upper=9"));
    ASSERT_TRUE(FileContains("config.ini", "road_fallback_scale=0.08"));
    ASSERT_TRUE(FileContains("config.ini", "understeer_affects_sop=1"));
    
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
    
    // Manually write to config.ini
    {
        std::ofstream file("config.ini");
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
    
    // Manually write to config.ini
    {
        std::ofstream file("config.ini");
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
    std::remove("config.ini");
    FFBEngine engine;
    Preset::ApplyDefaultsToEngine(engine);
    
    engine.m_gain = 0.77f;
    engine.m_understeer_effect = 44.4f;
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
    ASSERT_NEAR(engine2.m_understeer_effect, 44.4f, 0.001f);
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
        ASSERT_NEAR(engine3.m_understeer_effect, 44.4f, 0.001f);
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

    std::cout << "\n--- Persistence & Versioning Test Summary ---" << std::endl;
    std::cout << "Tests Passed: " << g_tests_passed << std::endl;
    std::cout << "Tests Failed: " << g_tests_failed << std::endl;
}

} // namespace PersistenceTests
