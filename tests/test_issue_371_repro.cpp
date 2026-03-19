#include "test_ffb_common.h"
#include <fstream>
#include <filesystem>
#include <thread>

namespace FFBEngineTests {

TEST_CASE(test_issue_371_repro, "Config") {
    std::cout << "\nTest: Issue #371 Repro (Imported profiles don't save)" << std::endl;

    std::string mock_config = "repro_371_config.ini";

    // 1. Create a mock config.ini with a user-defined preset
    {
        std::ofstream file(mock_config);
        file << "ini_version=0.7.203\n";
        file << "last_preset_name=MyUserPreset\n";
        file << "\n[Presets]\n";
        file << "[Preset:MyUserPreset]\n";
        file << "app_version=0.7.203\n";
        file << "gain=0.85\n";
        file << "sop=1.5\n";
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

    // BUG REPRO: At this point, Config::presets is expected to be EMPTY
    // because Load() does not call LoadPresets().
    // Only Default and built-ins might be there if they were added elsewhere,
    // but our specific "MyUserPreset" will definitely be missing if not loaded.

    // 5. Simulate an auto-save (e.g. session change or exiting)
    std::cout << "Step 2: Saving config (Simulated Auto-Save)..." << std::endl;
    Config::Save(engine, mock_config);

    // 6. Reload and check if "MyUserPreset" survived
    std::cout << "Step 3: Reloading to verify persistence..." << std::endl;
    Config::presets.clear();
    Config::LoadPresets(); // We use LoadPresets directly here to see what's in the file

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

    ASSERT_TRUE(found); // This will fail if the bug is present
}

} // namespace FFBEngineTests
