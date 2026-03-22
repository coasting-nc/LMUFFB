#include "test_ffb_common.h"
#include <fstream>
#include <filesystem>
#include <thread>

namespace FFBEngineTests {

TEST_CASE(test_issue_371_repro, "Config") {
    std::cout << "\nTest: Issue #371 Repro (Imported profiles don't save)" << std::endl;

    std::string mock_config = "repro_371_config.toml";

    // 1. Create a mock config.toml with a user-defined preset
    {
        std::ofstream file(mock_config);
        file << "[System]\nlast_preset_name = \"MyUserPreset\"\n";
        file << "[Presets.MyUserPreset.General]\ngain = 0.85\n";
        file << "[Presets.MyUserPreset.RearAxle]\nsop_effect = 1.5\n";
        file.close();
    }

    // 2. Point Config to our mock file
    std::string original_path = Config::m_config_path;
    Config::m_config_path = mock_config;

    // 3. Clear memory state
    Config::presets.clear();
    FFBEngine engine;
    InitializeEngine(engine);

    // 4. Simulate application startup LOAD
    std::cout << "Step 1: Loading config..." << std::endl;
    Config::Load(engine, mock_config);

    // 5. Simulate an auto-save (e.g. session change or exiting)
    std::cout << "Step 2: Saving config (Simulated Auto-Save)..." << std::endl;
    Config::Save(engine, mock_config);

    // 6. Reload and check if "MyUserPreset" survived
    std::cout << "Step 3: Reloading to verify persistence..." << std::endl;
    Config::presets.clear();
    Config::LoadPresets();

    bool found = false;
    for (const auto& p : Config::presets) {
        if (p.name == "MyUserPreset") {
            found = true;
            break;
        }
    }

    // Restore original path
    Config::m_config_path = original_path;

    if (found) {
        std::cout << "[INFO] Preset found. Bug NOT reproduced or already fixed." << std::endl;
    } else {
        std::cout << "[REPRO] Preset LOST! Bug successfully reproduced." << std::endl;
    }

    // Clean up
    if (std::filesystem::exists(mock_config)) std::filesystem::remove(mock_config);

    ASSERT_TRUE(found);
}

} // namespace FFBEngineTests
