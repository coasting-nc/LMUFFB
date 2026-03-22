#include "test_ffb_common.h"
#include <fstream>
#include <filesystem>
#include "../src/core/Config.h"
#include "../src/Version.h"

namespace FFBEngineTests {

TEST_CASE(test_preset_version_persistence, "Config") {
    std::cout << "\nTest: Preset Version Persistence" << std::endl;
    
    // 1. Create a user preset
    Preset p;
    p.name = "VersionTestPreset";
    p.app_version = "0.7.147"; // Use the fixed version to avoid migration
    
    // 2. Save it to user_presets as TOML
    if (std::filesystem::exists("user_presets")) std::filesystem::remove_all("user_presets");
    std::filesystem::create_directories("user_presets");
    std::string test_file = "user_presets/VersionTestPreset.toml";
    
    {
        std::ofstream file(test_file);
        file << "name = \"VersionTestPreset\"\n";
        file << "app_version = \"" << p.app_version << "\"\n";
        file << "[General]\ngain = 1.0\n";
        file.close();
    }

    // 3. Load it back and verify
    Config::LoadPresets();
    
    bool found = false;
    for (const auto& preset : Config::presets) {
        if (preset.name == "VersionTestPreset") {
            found = true;
            if (preset.app_version == "0.7.147") {
                std::cout << "[PASS] Preset app_version loaded correctly." << std::endl;
                g_tests_passed++;
            } else {
                FAIL_TEST("Preset app_version mismatch. Got: " << preset.app_version);
            }
            break;
        }
    }
    
    if (!found) {
        FAIL_TEST("VersionTestPreset not found after loading.");
    }

    if (std::filesystem::exists(test_file)) std::filesystem::remove(test_file);
}

TEST_CASE(test_legacy_preset_migration, "Config") {
    std::cout << "\nTest: Legacy Preset Migration" << std::endl;
    
    // In Phase 3, legacy presets are imported via ImportPreset,
    // or migrated from config.toml. Direct LoadPresets only sees user_presets/*.toml.
    // Let's use ImportPreset to verify migration logic.
    
    std::string test_file = "test_legacy_presets.ini";
    {
        std::ofstream file(test_file);
        file << "[Preset:LegacyPreset]\n";
        file << "gain=0.85\n"; // No app_version
        file.close();
    }

    FFBEngine engine;
    Config::ImportPreset(test_file, engine);
    
    bool found = false;
    for (const auto& preset : Config::presets) {
        if (preset.name == "LegacyPreset") {
            found = true;
            if (preset.app_version == LMUFFB_VERSION) {
                std::cout << "[PASS] Legacy preset migrated to current version: " << LMUFFB_VERSION << std::endl;
                g_tests_passed++;
            } else {
                FAIL_TEST("Legacy preset NOT migrated. Version: " << preset.app_version);
            }
            break;
        }
    }
    
    if (!found) {
        FAIL_TEST("LegacyPreset not found.");
    }

    if (std::filesystem::exists(test_file)) std::filesystem::remove(test_file);
}

} // namespace FFBEngineTests
