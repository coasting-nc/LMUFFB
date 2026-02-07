#include "test_ffb_common.h"
#include "../src/PresetRegistry.h"
#include <fstream>
#include <cstdio>

namespace FFBEngineTests {

TEST_CASE(test_last_preset_persistence, "Presets") {
    std::cout << "\nTest: Last Preset Persistence" << std::endl;
    FFBEngine engine;
    PresetRegistry& registry = PresetRegistry::Get();
    registry.Load("non_existent.ini");

    if (registry.GetPresets().size() < 2) {
        std::cout << "[SKIP] Not enough presets for test" << std::endl;
        return;
    }

    registry.ApplyPreset(1, engine);
    std::string applied_name = registry.GetPresets()[1].name;

    Config::Save(engine, "test_preset_persistence.ini");

    registry.SetLastPresetName("");
    Config::Load(engine, "test_preset_persistence.ini");

    ASSERT_TRUE(registry.GetLastPresetName() == applied_name);

    std::remove("test_preset_persistence.ini");
}

TEST_CASE(test_engine_dirty_detection, "Presets") {
    std::cout << "\nTest: Engine Dirty Detection" << std::endl;
    FFBEngine engine;
    PresetRegistry& registry = PresetRegistry::Get();
    registry.Load("non_existent.ini");

    registry.ApplyPreset(0, engine);

    ASSERT_TRUE(!registry.IsDirty(0, engine));

    engine.m_gain += 0.05f;

    ASSERT_TRUE(registry.IsDirty(0, engine));

    engine.m_gain -= 0.05f;
    ASSERT_TRUE(!registry.IsDirty(0, engine));
}

TEST_CASE(test_duplicate_preset, "Presets") {
    std::cout << "\nTest: Duplicate Preset" << std::endl;
    FFBEngine engine;
    PresetRegistry& registry = PresetRegistry::Get();
    registry.Load("non_existent.ini");
    size_t initial_count = registry.GetPresets().size();

    registry.DuplicatePreset(0, engine);

    ASSERT_TRUE(registry.GetPresets().size() == initial_count + 1);

    int found_idx = -1;
    const auto& presets = registry.GetPresets();
    for (int i = 0; i < (int)presets.size(); i++) {
        if (presets[i].name.find("Default (Copy)") != std::string::npos) {
            found_idx = i;
            break;
        }
    }

    ASSERT_TRUE(found_idx != -1);
    ASSERT_TRUE(!presets[found_idx].is_builtin);
}

TEST_CASE(test_delete_user_preset, "Presets") {
    std::cout << "\nTest: Delete User Preset" << std::endl;
    FFBEngine engine;
    PresetRegistry& registry = PresetRegistry::Get();
    registry.Load("non_existent.ini");

    registry.AddUserPreset("ToDelete", engine);
    const auto& presets1 = registry.GetPresets();
    size_t count_after_add = presets1.size();
    int index_to_delete = -1;
    for (int i = 0; i < (int)presets1.size(); i++) {
        if (presets1[i].name == "ToDelete") {
            index_to_delete = i;
            break;
        }
    }
    ASSERT_TRUE(index_to_delete != -1);

    registry.DeletePreset(index_to_delete, engine);
    ASSERT_TRUE(registry.GetPresets().size() == count_after_add - 1);

    size_t count_before_builtin_delete = registry.GetPresets().size();
    registry.DeletePreset(0, engine);
    ASSERT_TRUE(registry.GetPresets().size() == count_before_builtin_delete);
}

TEST_CASE(test_delete_preset_preserves_global_config, "Presets") {
    std::cout << "\nTest: Delete Preset Preserves Global Config" << std::endl;
    FFBEngine engine;
    PresetRegistry& registry = PresetRegistry::Get();
    registry.Load("non_existent.ini");

    engine.m_gain = 0.55f;
    Config::Save(engine, "test_preservation.ini");

    registry.AddUserPreset("TempPreset", engine);
    int index = -1;
    const auto& presets = registry.GetPresets();
    for (int i = 0; i < (int)presets.size(); i++) {
        if (presets[i].name == "TempPreset") {
            index = i;
            break;
        }
    }
    ASSERT_TRUE(index != -1);

    registry.DeletePreset(index, engine);

    FFBEngine engine2;
    Config::Load(engine2, "test_preservation.ini");

    ASSERT_NEAR(engine2.m_gain, 0.55f, 0.001);

    std::remove("test_preservation.ini");
}

} // namespace FFBEngineTests
