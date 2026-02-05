#include "test_ffb_common.h"
#include <fstream>
#include <cstdio>

namespace FFBEngineTests {

TEST_CASE(test_last_preset_persistence, "Presets") {
    std::cout << "\nTest: Last Preset Persistence" << std::endl;
    FFBEngine engine;
    Config::LoadPresets();

    // 1. Apply a preset
    if (Config::presets.size() < 2) {
        std::cout << "[SKIP] Not enough presets for test" << std::endl;
        return;
    }

    Config::ApplyPreset(1, engine);
    std::string applied_name = Config::presets[1].name;

    // 2. Save config
    Config::Save(engine, "test_preset_persistence.ini");

    // 3. Clear and reload
    Config::m_last_preset_name = "";
    Config::Load(engine, "test_preset_persistence.ini");

    ASSERT_TRUE(Config::m_last_preset_name == applied_name);

    std::remove("test_preset_persistence.ini");
}

TEST_CASE(test_engine_dirty_detection, "Presets") {
    std::cout << "\nTest: Engine Dirty Detection" << std::endl;
    FFBEngine engine;
    Config::LoadPresets();

    // Apply Default (index 0)
    Config::ApplyPreset(0, engine);

    // Should NOT be dirty initially
    ASSERT_TRUE(!Config::IsEngineDirtyRelativeToPreset(0, engine));

    // Modify engine
    engine.m_gain += 0.05f;

    // Should now BE dirty
    ASSERT_TRUE(Config::IsEngineDirtyRelativeToPreset(0, engine));

    // Reset and check again
    engine.m_gain -= 0.05f;
    ASSERT_TRUE(!Config::IsEngineDirtyRelativeToPreset(0, engine));
}

TEST_CASE(test_duplicate_preset, "Presets") {
    std::cout << "\nTest: Duplicate Preset" << std::endl;
    FFBEngine engine;
    Config::LoadPresets();
    size_t initial_count = Config::presets.size();

    Config::DuplicatePreset(0, engine); // Duplicate "Default"

    ASSERT_TRUE(Config::presets.size() == initial_count + 1);
    ASSERT_TRUE(Config::presets.back().name.find("Default") != std::string::npos);
    ASSERT_TRUE(Config::presets.back().name.find("(Copy)") != std::string::npos);
    ASSERT_TRUE(!Config::presets.back().is_builtin);
}

TEST_CASE(test_delete_user_preset, "Presets") {
    std::cout << "\nTest: Delete User Preset" << std::endl;
    FFBEngine engine;
    Config::LoadPresets();

    // 1. Create a user preset
    Config::AddUserPreset("ToDelete", engine);
    size_t count_after_add = Config::presets.size();
    int index_to_delete = -1;
    for (int i = 0; i < (int)Config::presets.size(); i++) {
        if (Config::presets[i].name == "ToDelete") {
            index_to_delete = i;
            break;
        }
    }
    ASSERT_TRUE(index_to_delete != -1);

    // 2. Delete it
    Config::DeletePreset(index_to_delete, engine);
    ASSERT_TRUE(Config::presets.size() == count_after_add - 1);

    // 3. Try to delete a builtin (should be ignored or handled safely)
    size_t count_before_builtin_delete = Config::presets.size();
    Config::DeletePreset(0, engine); // Default is builtin
    ASSERT_TRUE(Config::presets.size() == count_before_builtin_delete);
}

TEST_CASE(test_delete_preset_preserves_global_config, "Presets") {
    std::cout << "\nTest: Delete Preset Preserves Global Config" << std::endl;
    FFBEngine engine;
    Config::LoadPresets();

    // 1. Set a non-default global value
    engine.m_gain = 0.55f;
    Config::Save(engine, "test_preservation.ini");
    Config::m_config_path = "test_preservation.ini";

    // 2. Add and then delete a preset
    Config::AddUserPreset("TempPreset", engine);
    int index = -1;
    for (int i = 0; i < (int)Config::presets.size(); i++) {
        if (Config::presets[i].name == "TempPreset") {
            index = i;
            break;
        }
    }
    ASSERT_TRUE(index != -1);

    Config::DeletePreset(index, engine);

    // 3. Reload and verify global value
    FFBEngine engine2;
    Config::Load(engine2, "test_preservation.ini");

    ASSERT_NEAR(engine2.m_gain, 0.55f, 0.001);

    std::remove("test_preservation.ini");
    Config::m_config_path = "config.ini"; // Reset
}

} // namespace FFBEngineTests
