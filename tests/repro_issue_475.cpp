#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(repro_issue_475_lateral_load_clamping, "Config") {
    std::cout << "\nTest: Repro Issue #475 - Lateral Load Clamping" << std::endl;

    FFBEngine engine;

    // 1. Test value at 10.0 (maximum GUI range)
    // The bug is that this gets clamped to 2.0
    engine.m_load_forces.lat_load_effect = 10.0f;
    engine.m_load_forces.Validate();

    std::cout << "Lateral Load after Validate (expected 10.0): " << engine.m_load_forces.lat_load_effect << std::endl;

    // This assertion should FAIL before the fix
    ASSERT_NEAR(engine.m_load_forces.lat_load_effect, 10.0f, 0.0001f);

    // 2. Test value above 10.0
    engine.m_load_forces.lat_load_effect = 11.0f;
    engine.m_load_forces.Validate();

    // This should still clamp to 10.0 after the fix
    ASSERT_NEAR(engine.m_load_forces.lat_load_effect, 10.0f, 0.0001f);
}

} // namespace FFBEngineTests
