#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_dynamic_normalization_toggle, "CorePhysics") {
    std::cout << "\nTest: Dynamic Normalization Toggle (Issue #180)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.01; // 100Hz
    data.mLocalVel.z = 20.0;
    data.mWheel[0].mRideHeight = 0.1;
    data.mWheel[1].mRideHeight = 0.1;
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;
    data.mWheel[0].mTireLoad = 4000.0;
    data.mWheel[1].mTireLoad = 4000.0;

    engine.m_torque_source = 0; // Shaft Torque
    engine.m_wheelbase_max_nm = 20.0f;
    engine.m_target_rim_nm = 10.0f;
    engine.m_gain = 1.0f;
    engine.m_steering_shaft_gain = 1.0f;
    engine.m_understeer_effect = 0.0f;
    engine.m_sop_effect = 0.0f;
    engine.m_invert_force = false;

    // Initial state
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 10.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 10.0);

    // Case 1: Dynamic Normalization ENABLED (Default)
    engine.m_dynamic_normalization_enabled = true;
    data.mSteeringShaftTorque = 15.0; // High torque spike

    // Run several frames to allow peak follower and mult to update
    for (int i = 0; i < 50; i++) engine.calculate_force(&data);

    double peak_enabled = FFBEngineTestAccess::GetSessionPeakTorque(engine);
    ASSERT_GT(peak_enabled, 10.001); // Should have increased

    // Case 2: Dynamic Normalization DISABLED
    engine.m_dynamic_normalization_enabled = false;
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 10.0); // Reset
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 10.0);

    data.mSteeringShaftTorque = 25.0; // Even higher torque
    for (int i = 0; i < 50; i++) engine.calculate_force(&data);

    double peak_disabled = FFBEngineTestAccess::GetSessionPeakTorque(engine);
    ASSERT_NEAR(peak_disabled, 10.0, 0.001); // Should NOT have changed

    // Case 3: Verify Force Scaling when DISABLED
    // target_structural_mult should use m_wheelbase_max_nm (20.0)
    // raw_torque = 25.0
    // di_structural = (25.0 / 20.0) * (10.0 / 20.0) = 1.25 * 0.5 = 0.625
    // After slew and smoothing it might be slightly off, but let's check steady state
    for (int i = 0; i < 100; i++) engine.calculate_force(&data);
    double force = engine.calculate_force(&data);

    // Expected: (raw / wheelbase_max) * (target_rim / wheelbase_max)
    // = (25.0 / 20.0) * (10.0 / 20.0) = 1.25 * 0.5 = 0.625
    ASSERT_NEAR(force, 0.625, 0.01);

    // Case 4: Verify Force Scaling when ENABLED
    engine.m_dynamic_normalization_enabled = true;
    // Let it learn the 25.0 peak
    for (int i = 0; i < 100; i++) engine.calculate_force(&data);
    double peak_learned = FFBEngineTestAccess::GetSessionPeakTorque(engine);
    ASSERT_NEAR(peak_learned, 25.0, 0.1);

    // Expected: (25.0 / 25.0) * (10.0 / 20.0) = 1.0 * 0.5 = 0.5
    force = engine.calculate_force(&data);
    ASSERT_NEAR(force, 0.5, 0.02);
}

} // namespace FFBEngineTests
