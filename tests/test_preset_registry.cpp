#include "test_ffb_common.h"
#include "../src/PresetRegistry.h"

namespace FFBEngineTests {

TEST_CASE(test_preset_registry_singleton, "Registry") {
    std::cout << "\nTest: PresetRegistry Singleton" << std::endl;
    PresetRegistry& r1 = PresetRegistry::Get();
    PresetRegistry& r2 = PresetRegistry::Get();
    ASSERT_TRUE(&r1 == &r2);
}

TEST_CASE(test_preset_registry_ordering, "Registry") {
    std::cout << "\nTest: PresetRegistry Ordering" << std::endl;
    PresetRegistry& r = PresetRegistry::Get();

    // 1. Setup a dummy config with a user preset
    std::string test_ini = "test_registry_ordering.ini";
    {
        std::ofstream file(test_ini);
        file << "[Presets]\n";
        file << "[Preset:UserPreset1]\n";
        file << "gain=0.123\n";
        file.close();
    }

    r.Load(test_ini);
    const auto& presets = r.GetPresets();

    // We expect: [0] Default, [1] UserPreset1, [2] T300, ...
    ASSERT_TRUE(presets.size() >= 3);
    ASSERT_TRUE(presets[0].name == "Default");
    ASSERT_TRUE(presets[0].is_builtin);

    ASSERT_TRUE(presets[1].name == "UserPreset1");
    ASSERT_TRUE(!presets[1].is_builtin);

    ASSERT_TRUE(presets[2].is_builtin); // Vendor preset

    std::remove(test_ini.c_str());
}

TEST_CASE(test_preset_registry_insertion, "Registry") {
    std::cout << "\nTest: PresetRegistry Insertion" << std::endl;
    PresetRegistry& r = PresetRegistry::Get();
    FFBEngine engine;

    r.Load("non_existent.ini"); // Just built-ins
    size_t base_size = r.GetPresets().size();

    r.AddUserPreset("NewUser", engine);
    const auto& presets = r.GetPresets();

    ASSERT_TRUE(presets.size() == base_size + 1);
    ASSERT_TRUE(presets[1].name == "NewUser");
    ASSERT_TRUE(!presets[1].is_builtin);
}

TEST_CASE(test_preset_registry_dirty_state, "Registry") {
    std::cout << "\nTest: PresetRegistry Dirty State" << std::endl;
    PresetRegistry& r = PresetRegistry::Get();
    FFBEngine engine;

    r.Load("non_existent.ini");
    r.ApplyPreset(0, engine); // Apply Default

    ASSERT_TRUE(!r.IsDirty(0, engine));

    engine.m_gain += 0.1f;
    ASSERT_TRUE(r.IsDirty(0, engine));
}

} // namespace FFBEngineTests
