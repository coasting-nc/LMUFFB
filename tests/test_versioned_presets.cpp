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
    TestDirectoryGuard temp_dir("tmp_test_version_persistence");
    ScopedConfigPathGuard path_guard;
    Config::m_user_presets_path = temp_dir.path() + "/user_presets";
    
    std::filesystem::create_directories(Config::m_user_presets_path);
    std::string test_file = Config::m_user_presets_path + "/VersionTestPreset.toml";
    
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
}

TEST_CASE(test_legacy_preset_migration, "Config") {
    std::cout << "\nTest: Legacy Preset Migration" << std::endl;
    
    // In Phase 3, legacy presets are imported via ImportPreset,
    // or migrated from config.toml. Direct LoadPresets only sees user_presets/*.toml.
    // Let's use ImportPreset to verify migration logic.
    
    TestDirectoryGuard temp_dir("tmp_test_legacy_migration");
    ScopedConfigPathGuard path_guard;
    Config::m_user_presets_path = temp_dir.path() + "/user_presets";
    
    std::string test_file = temp_dir.path() + "/test_legacy_presets.ini";
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
}

} // namespace FFBEngineTests
