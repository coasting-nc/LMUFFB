#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_issue_42_ffb_inversion_persistence, "Config") {
    std::cout << "\nTest: Issue #42 - FFB Inversion Persistence Across Presets" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);
    Config::LoadPresets();

    // Case 1: Start with Invert = True
    FFBEngineTestAccess::SetInvertForce(engine, true);
    ASSERT_TRUE(engine.m_invert_force);

    // Apply "Default" preset
    Config::ApplyPreset(0, engine);
    ASSERT_TRUE(engine.m_invert_force); // Should remain true

    // Apply "T300" preset (which used to set it to true explicitly)
    int t300_idx = -1;
    for(int i=0; i<(int)Config::presets.size(); i++) {
        if(Config::presets[i].name == "T300") {
            t300_idx = i;
            break;
        }
    }
    if(t300_idx != -1) {
        Config::ApplyPreset(t300_idx, engine);
        ASSERT_TRUE(engine.m_invert_force);
    }

    // Case 2: Start with Invert = False
    FFBEngineTestAccess::SetInvertForce(engine, false);
    ASSERT_FALSE(engine.m_invert_force);

    // Apply "Default" preset
    Config::ApplyPreset(0, engine);
    ASSERT_FALSE(engine.m_invert_force); // Should remain false!

    if(t300_idx != -1) {
        Config::ApplyPreset(t300_idx, engine);
        ASSERT_FALSE(engine.m_invert_force); // Should remain false!
    }

    // Case 3: Verify Preset::UpdateFromEngine doesn't capture it (indirectly verified by lack of member)
    // But we can check that if we change engine inversion, and update a preset, then change engine inversion back,
    // applying the preset doesn't revert it.

    Preset user_preset("UserTest", false);
    FFBEngineTestAccess::SetInvertForce(engine, true);
    user_preset.UpdateFromEngine(engine);

    FFBEngineTestAccess::SetInvertForce(engine, false);
    user_preset.Apply(engine);
    ASSERT_FALSE(engine.m_invert_force); // Apply should NOT have changed it to true
}

} // namespace FFBEngineTests
