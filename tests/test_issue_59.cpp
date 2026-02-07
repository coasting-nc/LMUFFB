#include "test_ffb_common.h"
#include "../src/PresetRegistry.h"
#include <fstream>
#include <cstdio>
#include <algorithm>

namespace FFBEngineTests {

TEST_CASE(test_user_presets_ordering, "Issue 59") {
    std::cout << "\nTest: User Presets Ordering (Issue 59)" << std::endl;

    PresetRegistry& registry = PresetRegistry::Get();

    // 1. Create a temporary config file with a user preset
    std::string temp_config = "test_issue_59_ordering.ini";
    {
        std::ofstream file(temp_config);
        file << "[Presets]\n";
        file << "[Preset:MyUserPreset]\n";
        file << "gain=0.5\n";
        file.close();
    }

    // 2. Load presets
    registry.Load(temp_config);
    const auto& presets = registry.GetPresets();

    // 3. Verify order: Default, User Presets, other Built-ins
    ASSERT_TRUE(presets.size() > 2);

    ASSERT_TRUE(presets[0].name == "Default");
    ASSERT_TRUE(presets[0].is_builtin == true);

    // Find MyUserPreset
    int user_idx = -1;
    for (int i = 0; i < (int)presets.size(); i++) {
        if (presets[i].name == "MyUserPreset") {
            user_idx = i;
            break;
        }
    }

    ASSERT_TRUE(user_idx != -1);
    ASSERT_TRUE(user_idx == 1);
    ASSERT_TRUE(presets[user_idx].is_builtin == false);

    if (user_idx + 1 < (int)presets.size()) {
        ASSERT_TRUE(presets[user_idx + 1].is_builtin == true);
    }

    std::remove(temp_config.c_str());
}

TEST_CASE(test_add_user_preset_insertion_point, "Issue 59") {
    std::cout << "\nTest: Add User Preset Insertion Point (Issue 59)" << std::endl;

    PresetRegistry& registry = PresetRegistry::Get();
    FFBEngine engine;

    std::string temp_config = "test_issue_59_add.ini";
    registry.Load("non_existent.ini");

    size_t initial_size = registry.GetPresets().size();

    // Add a new user preset
    registry.AddUserPreset("NewUserPreset", engine);
    const auto& presets = registry.GetPresets();

    ASSERT_TRUE(presets.size() == initial_size + 1);

    int user_idx = -1;
    for (int i = 0; i < (int)presets.size(); i++) {
        if (presets[i].name == "NewUserPreset") {
            user_idx = i;
            break;
        }
    }

    ASSERT_TRUE(user_idx != -1);
    ASSERT_TRUE(user_idx == 1);
    if (user_idx + 1 < (int)presets.size()) {
        ASSERT_TRUE(presets[user_idx + 1].is_builtin == true);
    }

    std::remove(temp_config.c_str());
}

} // namespace FFBEngineTests
