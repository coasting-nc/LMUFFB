#include "test_ffb_common.h"
#include <fstream>
#include <cstdio>
#include "../src/Config.h"
#include "../src/Version.h"

namespace FFBEngineTests {

TEST_CASE(test_preset_version_persistence, "Config") {
    std::cout << "\nTest: Preset Version Persistence" << std::endl;
    
    // 1. Create a user preset
    Preset p;
    p.name = "VersionTestPreset";
    p.app_version = "0.7.12"; 
    
    // 2. Save it to a temporary INI
    const char* test_file = "test_version_presets.ini";
    std::string original_path = Config::m_config_path;
    Config::m_config_path = test_file;
    
    {
        std::ofstream file(test_file);
        file << "[Preset:VersionTestPreset]\n";
        file << "app_version=" << p.app_version << "\n";
        file << "gain=1.0\n";
        file.close();
    }

    // 3. Load it back and verify
    Config::LoadPresets();
    
    bool found = false;
    for (const auto& preset : Config::presets) {
        if (preset.name == "VersionTestPreset") {
            found = true;
            if (preset.app_version == "0.7.12") {
                std::cout << "[PASS] Preset app_version loaded correctly." << std::endl;
                g_tests_passed++;
            } else {
                std::cout << "[FAIL] Preset app_version mismatch. Got: " << preset.app_version << std::endl;
                g_tests_failed++;
            }
            break;
        }
    }
    
    if (!found) {
        std::cout << "[FAIL] VersionTestPreset not found after loading." << std::endl;
        g_tests_failed++;
    }

    Config::m_config_path = original_path;
    std::remove(test_file);
}

TEST_CASE(test_legacy_preset_migration, "Config") {
    std::cout << "\nTest: Legacy Preset Migration" << std::endl;
    
    const char* test_file = "test_legacy_presets.ini";
    std::string original_path = Config::m_config_path;
    Config::m_config_path = test_file;
    
    {
        std::ofstream file(test_file);
        file << "[Preset:LegacyPreset]\n";
        file << "gain=0.85\n"; // No app_version
        file.close();
    }

    // Load presets - should trigger migration
    Config::LoadPresets();
    
    bool found = false;
    for (const auto& preset : Config::presets) {
        if (preset.name == "LegacyPreset") {
            found = true;
            if (preset.app_version == LMUFFB_VERSION) {
                std::cout << "[PASS] Legacy preset migrated to current version: " << LMUFFB_VERSION << std::endl;
                g_tests_passed++;
            } else {
                std::cout << "[FAIL] Legacy preset NOT migrated. Version: " << preset.app_version << std::endl;
                g_tests_failed++;
            }
            break;
        }
    }
    
    if (!found) {
        std::cout << "[FAIL] LegacyPreset not found." << std::endl;
        g_tests_failed++;
    }

    Config::m_config_path = original_path;
    std::remove(test_file);
}

} // namespace FFBEngineTests
