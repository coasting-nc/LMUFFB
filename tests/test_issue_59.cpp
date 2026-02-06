#include "test_ffb_common.h"
#include <fstream>
#include <cstdio>
#include <algorithm>

namespace FFBEngineTests {

TEST_CASE(test_user_presets_ordering, "Issue 59") {
    std::cout << "\nTest: User Presets Ordering (Issue 59)" << std::endl;

    // 1. Create a temporary config file with a user preset
    std::string temp_config = "test_issue_59_ordering.ini";
    {
        std::ofstream file(temp_config);
        file << "[Presets]\n";
        file << "[Preset:MyUserPreset]\n";
        file << "gain=0.5\n";
        file.close();
    }

    std::string original_path = Config::m_config_path;
    Config::m_config_path = temp_config;

    // 2. Load presets
    Config::LoadPresets();

    // 3. Verify order: Default, User Presets, other Built-ins
    ASSERT_TRUE(Config::presets.size() > 2); // Default, MyUserPreset, and at least one other built-in

    ASSERT_TRUE(Config::presets[0].name == "Default");
    ASSERT_TRUE(Config::presets[0].is_builtin == true);

    // Find MyUserPreset
    int user_idx = -1;
    for (int i = 0; i < (int)Config::presets.size(); i++) {
        if (Config::presets[i].name == "MyUserPreset") {
            user_idx = i;
            break;
        }
    }

    ASSERT_TRUE(user_idx != -1);
    // DESIRED: it should be at index 1 if it's the only user preset.
    // In current implementation, it will be at the end.
    ASSERT_TRUE(user_idx == 1);
    ASSERT_TRUE(Config::presets[user_idx].is_builtin == false);

    // Verify that the next one is a built-in
    if (user_idx + 1 < (int)Config::presets.size()) {
        ASSERT_TRUE(Config::presets[user_idx + 1].is_builtin == true);
    }

    // Cleanup
    Config::m_config_path = original_path;
    std::remove(temp_config.c_str());
}

TEST_CASE(test_add_user_preset_insertion_point, "Issue 59") {
    std::cout << "\nTest: Add User Preset Insertion Point (Issue 59)" << std::endl;

    std::string temp_config = "test_issue_59_add.ini";
    std::string original_path = Config::m_config_path;
    Config::m_config_path = temp_config;

    FFBEngine engine;
    Config::LoadPresets();

    size_t initial_size = Config::presets.size();

    // Add a new user preset
    Config::AddUserPreset("NewUserPreset", engine);

    ASSERT_TRUE(Config::presets.size() == initial_size + 1);

    // Find its index
    int user_idx = -1;
    for (int i = 0; i < (int)Config::presets.size(); i++) {
        if (Config::presets[i].name == "NewUserPreset") {
            user_idx = i;
            break;
        }
    }

    ASSERT_TRUE(user_idx != -1);
    // DESIRED: should be in the user section (index 1 if it's the only one)
    ASSERT_TRUE(user_idx == 1);
    // Verify it's followed by a built-in (unless there are more user presets, but here there's only one)
    if (user_idx + 1 < (int)Config::presets.size()) {
        ASSERT_TRUE(Config::presets[user_idx + 1].is_builtin == true);
    }

    // Cleanup
    Config::m_config_path = original_path;
    std::remove(temp_config.c_str());
}

} // namespace FFBEngineTests
