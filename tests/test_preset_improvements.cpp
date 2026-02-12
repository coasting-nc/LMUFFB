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

TEST_CASE(test_add_user_preset_updates_existing, "Presets") {
    std::cout << "\nTest: Add User Preset Updates Existing" << std::endl;
    FFBEngine engine;
    Config::LoadPresets();

    // 1. Create a user preset
    Config::AddUserPreset("MyPreset", engine);
    size_t count_after_add = Config::presets.size();

    // 2. Change engine state
    engine.m_gain = 0.75f;

    // 3. Add same preset again (simulating Save Current Config)
    Config::AddUserPreset("MyPreset", engine);

    // 4. Verify count hasn't changed and values ARE updated
    ASSERT_TRUE(Config::presets.size() == count_after_add);

    int index = -1;
    for (int i = 0; i < (int)Config::presets.size(); i++) {
        if (Config::presets[i].name == "MyPreset") {
            index = i;
            break;
        }
    }
    ASSERT_TRUE(index != -1);
    ASSERT_NEAR(Config::presets[index].gain, 0.75f, 0.001);
}

TEST_CASE(test_global_save_does_not_update_presets, "Presets") {
    std::cout << "\nTest: Global Save Does Not Update Presets" << std::endl;
    FFBEngine engine;
    Config::LoadPresets();

    // 1. Create a user preset
    Config::AddUserPreset("MyPreset", engine);
    int index = -1;
    for (int i = 0; i < (int)Config::presets.size(); i++) {
        if (Config::presets[i].name == "MyPreset") {
            index = i;
            break;
        }
    }
    float original_gain = Config::presets[index].gain;

    // 2. Change engine state
    engine.m_gain = original_gain + 0.1f;

    // 3. Call global Save (simulating what happens when selected_preset == -1)
    Config::Save(engine, "test_global_save.ini");

    // 4. Reload presets from file (to be sure)
    Config::presets.clear();
    std::string old_path = Config::m_config_path;
    Config::m_config_path = "test_global_save.ini";
    Config::LoadPresets();

    // 5. Verify the preset in the file still has original gain
    int new_index = -1;
    for (int i = 0; i < (int)Config::presets.size(); i++) {
        if (Config::presets[i].name == "MyPreset") {
            new_index = i;
            break;
        }
    }
    ASSERT_TRUE(new_index != -1);
    ASSERT_NEAR(Config::presets[new_index].gain, original_gain, 0.001);

    std::remove("test_global_save.ini");
    Config::m_config_path = old_path;
}

} // namespace FFBEngineTests
