#include "test_ffb_common.h"
#include <fstream>
#include <cstdio>
#include "../src/Config.h"
#include "../src/Version.h"

namespace FFBEngineTests {

TEST_CASE(test_issue_211_preset_gain_migration, "Config") {
    std::cout << "\nTest: Issue #211 Preset Gain Migration" << std::endl;

    const char* test_file = "test_issue_211_presets.ini";
    std::string original_path = Config::m_config_path;
    Config::m_config_path = test_file;

    // 1. Create a legacy preset (v0.7.66) with 100Nm hack
    {
        std::ofstream file(test_file);
        file << "[Preset:Legacy100Nm]\n";
        file << "app_version=0.7.66\n";
        file << "gain=1.0\n";
        file << "max_torque_ref=100.0\n";
        file.close();
    }

    // 2. Load presets
    Config::LoadPresets();

    bool found = false;
    for (const auto& preset : Config::presets) {
        if (preset.name == "Legacy100Nm") {
            found = true;
            // Current code (before fix) will reset wheelbase_max_nm to 15.0 but KEEP gain at 1.0
            // We expect gain to be scaled to 0.15f after fix.
            if (preset.wheelbase_max_nm == 15.0f) {
                if (std::abs(preset.gain - 0.15f) < 0.001f) {
                    std::cout << "[PASS] Legacy preset gain migrated correctly: " << preset.gain << std::endl;
                    g_tests_passed++;
                } else {
                    std::cout << "[FAIL] Legacy preset gain NOT migrated. Got: " << preset.gain << " (Expected ~0.15)" << std::endl;
                    g_tests_failed++;
                }
            } else {
                std::cout << "[FAIL] Legacy preset wheelbase_max_nm NOT reset. Got: " << preset.wheelbase_max_nm << std::endl;
                g_tests_failed++;
            }
            break;
        }
    }

    if (!found) {
        std::cout << "[FAIL] Legacy100Nm preset not found." << std::endl;
        g_tests_failed++;
    }

    Config::m_config_path = original_path;
    std::remove(test_file);
}

TEST_CASE(test_issue_211_config_gain_migration, "Config") {
    std::cout << "\nTest: Issue #211 Config Gain Migration" << std::endl;

    const char* test_file = "test_issue_211_config.ini";
    FFBEngine engine;

    // 1. Create a legacy config (v0.7.66) with 100Nm hack
    {
        std::ofstream file(test_file);
        file << "ini_version=0.7.66\n";
        file << "gain=0.8\n";
        file << "max_torque_ref=100.0\n";
        file.close();
    }

    // 2. Load config
    Config::Load(engine, test_file);

    // Expected gain: 0.8 * 0.15 = 0.12
    if (std::abs(engine.m_gain - 0.12f) < 0.001f) {
        std::cout << "[PASS] Legacy config gain migrated correctly: " << engine.m_gain << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Legacy config gain NOT migrated. Got: " << engine.m_gain << " (Expected ~0.12)" << std::endl;
        g_tests_failed++;
    }

    std::remove(test_file);
}

} // namespace FFBEngineTests
