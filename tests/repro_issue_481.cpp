#include "test_ffb_common.h"
#include <filesystem>
#include <fstream>

namespace FFBEngineTests {

TEST_CASE(repro_issue_481_preset_persistence, "Config") {
    std::cout << "\nTest: Repro Issue #481 - Preset Persistence" << std::endl;

    TestDirectoryGuard temp_dir("tmp_test_repro_481");
    std::string toml_file = temp_dir.path() + "/config.toml";
    std::string user_presets_dir = temp_dir.path() + "/user_presets";

    std::string original_config_path = Config::m_config_path;
    std::string original_user_presets_path = Config::m_user_presets_path;

    Config::m_config_path = toml_file;
    Config::m_user_presets_path = user_presets_dir;

    // RESET STATIC STATE
    Config::m_last_preset_name = "Default";
    Config::presets.clear();

    FFBEngine engine;
    InitializeEngine(engine);

    // 1. Initial state: Default preset
    Config::LoadPresets();
    ASSERT_EQ(Config::m_last_preset_name, "Default");

    // 2. Add a user preset and apply it
    std::string custom_preset_name = "My Custom Preset";
    engine.m_general.gain = 0.75f;
    Config::AddUserPreset(custom_preset_name, engine);

    ASSERT_EQ(Config::m_last_preset_name, custom_preset_name);

    // Verify it was saved to config.toml
    {
        toml::table tbl = toml::parse_file(toml_file);
        auto sys = tbl["System"].as_table();
        ASSERT_TRUE(sys != nullptr);
        ASSERT_EQ((*sys)["last_preset_name"].value_or(""), custom_preset_name);

        auto gen = tbl["General"].as_table();
        ASSERT_TRUE(gen != nullptr);
        ASSERT_NEAR((float)(*gen)["gain"].value_or(0.0), 0.75f, 0.0001f);
    }

    // 3. Restart the app (simulated)
    FFBEngine engine2;
    // main.cpp does this:
    Preset::ApplyDefaultsToEngine(engine2);
    // Then loads
    Config::m_last_preset_name = "Default"; // Reset to initial state
    Config::Load(engine2, toml_file);

    // 4. Verify m_last_preset_name is restored
    ASSERT_EQ(Config::m_last_preset_name, custom_preset_name);

    // 5. Verify engine settings are restored
    ASSERT_NEAR(engine2.m_general.gain, 0.75f, 0.0001f);

    // 6. Verify UI matching logic
    int selected_preset = -1;
    for (int i = 0; i < (int)Config::presets.size(); i++) {
        if (Config::presets[i].name == Config::m_last_preset_name) {
            selected_preset = i;
            break;
        }
    }

    ASSERT_GE(selected_preset, 0);
    ASSERT_EQ(Config::presets[selected_preset].name, custom_preset_name);

    // Cleanup
    Config::m_config_path = original_config_path;
    Config::m_user_presets_path = original_user_presets_path;
}

} // namespace FFBEngineTests
