#include "test_ffb_common.h"
#include <cmath>

namespace FFBEngineTests {

TEST_CASE(test_ingame_ffb_scaling_fix, "InGameFFB") {
    std::cout << "\nTest: In-Game FFB Scaling Fix (Issue #160)" << std::endl;
    FFBEngine engine;

    // Setup a clean state
    engine.m_gain = 1.0f;
    engine.m_understeer_effect = 0.0f;
    engine.m_sop_effect = 0.0f;
    engine.m_oversteer_boost = 0.0f;
    engine.m_rear_align_effect = 0.0f;
    engine.m_sop_yaw_gain = 0.0f;
    engine.m_gyro_gain = 0.0f;
    engine.m_road_texture_gain = 0.0f;
    engine.m_slide_texture_gain = 0.0f;
    engine.m_spin_gain = 0.0f;
    engine.m_lockup_gain = 0.0f;
    engine.m_abs_gain = 0.0f;
    engine.m_scrub_drag_gain = 0.0f;
    engine.m_bottoming_gain = 0.0f;
    engine.m_soft_lock_enabled = false;
    engine.m_min_force = 0.0f;
    engine.m_invert_force = false;

    // 1. In-Game FFB Source (m_torque_source = 1)
    // 2. Wheelbase Max = 20.0 Nm
    // 3. Target Rim = 10.0 Nm
    // 4. In-Game Gain = 1.0
    // 5. Game Input = 1.0 (Full strength)
    engine.m_torque_source = 1;
    engine.m_wheelbase_max_nm = 20.0f;
    engine.m_target_rim_nm = 10.0f;
    engine.m_ingame_ffb_gain = 1.0f;
    engine.m_torque_passthrough = true;

    float genFFBTorque = 1.0f;

    // Mock Telemetry
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.0025f;
    data.mElapsedTime = 0.0025f;
    data.mLocalVel.z = 10.0f;

    // Force the multiplier to the correct target
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 20.0);

    double output = engine.calculate_force(&data, "GT3", "911 GT3 R", genFFBTorque);

    // Expected:
    // raw_torque_input = 1.0 * 20.0 = 20.0 Nm
    // target_structural_mult = 1.0 / 20.0 = 0.05
    // norm_structural = 20.0 * 1.0 (ingame gain) * 0.05 = 1.0
    // di_structural = 1.0 * (10.0 / 20.0) = 0.5
    // total = 0.5 * 1.0 (master gain) = 0.5

    std::cout << "  In-Game FFB Output: " << output << " (Expected: 0.5)" << std::endl;
    ASSERT_NEAR(output, 0.5, 0.001);

    // Test adjustment via In-Game Gain
    engine.m_ingame_ffb_gain = 0.5f;
    output = engine.calculate_force(&data, "GT3", "911 GT3 R", genFFBTorque);

    // Expected:
    // structural_sum = 20.0 * 0.5 = 10.0 Nm
    // norm_structural = 10.0 * 0.05 = 0.5
    // di_structural = 0.5 * (10.0 / 20.0) = 0.25
    std::cout << "  In-Game FFB Output (Gain 50%): " << output << " (Expected: 0.25)" << std::endl;
    ASSERT_NEAR(output, 0.25, 0.001);
}

} // namespace FFBEngineTests
