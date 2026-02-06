#include "test_ffb_common.h"
#include "../src/PresetRegistry.h"
#include <fstream>
#include <cstdio>

namespace FFBEngineTests {

TEST_CASE(test_preset_version_persistence, "Presets") {
    std::cout << "\nTest: Preset Version Persistence" << std::endl;
    FFBEngine engine;
    PresetRegistry& registry = PresetRegistry::Get();
    
    const char* test_file = "test_version_presets.ini";
    {
        std::ofstream file(test_file);
        file << "[Presets]\n";
        file << "[Preset:VersionedTest]\n";
        file << "app_version=0.1.2\n";
        file << "gain=0.5\n";
        file.close();
    }
    
    registry.Load(test_file);
    bool found = false;
    for (const auto& preset : registry.GetPresets()) {
        if (preset.name == "VersionedTest") {
            ASSERT_TRUE(preset.app_version == "0.1.2");
            found = true;
        }
    }
    ASSERT_TRUE(found);
    std::remove(test_file);
}

TEST_CASE(test_legacy_preset_migration, "Presets") {
    std::cout << "\nTest: Legacy Preset Migration" << std::endl;
    FFBEngine engine;
    PresetRegistry& registry = PresetRegistry::Get();
    
    const char* test_file = "test_legacy_presets.ini";
    {
        std::ofstream file(test_file);
        file << "[Presets]\n";
        file << "[Preset:LegacyTest]\n";
        file << "gain=0.5\n"; // Missing version
        file.close();
    }
    
    registry.Load(test_file);
    bool found = false;
    for (const auto& preset : registry.GetPresets()) {
        if (preset.name == "LegacyTest") {
            ASSERT_TRUE(preset.app_version == LMUFFB_VERSION);
            found = true;
        }
    }
    ASSERT_TRUE(found);
    std::remove(test_file);
}

} // namespace FFBEngineTests
