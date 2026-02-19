#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_direct_torque_scaling, "Issue142") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Setup for Direct Torque (m_torque_source = 1)
    engine.m_torque_source = 1;
    engine.m_wheelbase_max_nm = 50.0f; engine.m_target_rim_nm = 50.0f; // Nm
    engine.m_gain = 1.0f;
    engine.m_steering_shaft_gain = 1.0f;
    engine.m_understeer_effect = 0.0f; // Disable modulation for scaling test
    engine.m_dynamic_weight_gain = 0.0f;
    engine.m_sop_effect = 0.0f; // Disable other effects
    engine.m_road_texture_enabled = false;

    TelemInfoV01 telem = CreateBasicTestTelemetry(20.0, 0.0);
    float genFFBTorque = 1.0f; // Max normalized FFB

    double output = engine.calculate_force(&telem, "GT3", "Ferrari 488", genFFBTorque);

    // Expected logic:
    // raw_torque_input = 1.0 * 50.0 = 50.0 Nm
    // base_input = 50.0
    // output_force = 50.0 * 1.0 * 1.0 * 1.0 = 50.0
    // norm_force = 50.0 / 50.0 * 1.0 = 1.0

    ASSERT_NEAR(output, 1.0, 0.01);
}

TEST_CASE(test_torque_passthrough_enabled, "Issue142") {
    FFBEngine engine;
    InitializeEngine(engine);

    engine.m_torque_source = 1;
    engine.m_torque_passthrough = true;
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f;
    engine.m_gain = 1.0f;

    // Enable strong understeer effect
    engine.m_understeer_effect = 1.0f;

    // Create telemetry with high slip angle (0.5 rad) to trigger grip loss
    TelemInfoV01 telem = CreateBasicTestTelemetry(20.0, 0.5);
    // Force grip to 0.5 manually if needed, but calculate_grip should do it based on slip
    // With 0.5 rad slip and default optimal_slip_angle=0.1, combined_slip=5.0
    // grip_factor = 1.0 / (1.0 + 4.0 * 2.0) = 1.0 / 9.0 approx 0.11

    float genFFBTorque = 1.0f;

    double output = engine.calculate_force(&telem, "GT3", "Ferrari 488", genFFBTorque);

    // With Passthrough ENABLED, output should still be ~1.0 despite high slip
    ASSERT_NEAR(output, 1.0, 0.01);

    // Verify snapshot shows no understeer drop
    std::vector<FFBSnapshot> batch = engine.GetDebugBatch();
    if (!batch.empty()) {
        ASSERT_NEAR(batch.back().understeer_drop, 0.0, 0.001);
    }
}

TEST_CASE(test_torque_passthrough_disabled, "Issue142") {
    FFBEngine engine;
    InitializeEngine(engine);

    engine.m_torque_source = 1;
    engine.m_torque_passthrough = false;
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f;
    engine.m_gain = 1.0f;

    engine.m_understeer_effect = 1.0f;

    // Create telemetry with high slip angle to trigger grip loss
    TelemInfoV01 telem = CreateBasicTestTelemetry(20.0, 0.5);

    float genFFBTorque = 1.0f;

    double output = engine.calculate_force(&telem, "GT3", "Ferrari 488", genFFBTorque);

    // With Passthrough DISABLED, output should be significantly reduced by understeer
    ASSERT_LT(output, 0.9);

    // Verify snapshot shows understeer drop
    std::vector<FFBSnapshot> batch = engine.GetDebugBatch();
    if (!batch.empty()) {
        ASSERT_GT(batch.back().understeer_drop, 0.1);
    }
}

TEST_CASE(test_dynamic_weight_passthrough, "Issue142") {
    FFBEngine engine;
    InitializeEngine(engine);

    engine.m_torque_source = 1;
    engine.m_torque_passthrough = true;
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f;
    engine.m_gain = 1.0f;

    // Enable strong dynamic weight
    engine.m_dynamic_weight_gain = 1.0f;

    // Create telemetry with high load to trigger weight gain
    TelemInfoV01 telem = CreateBasicTestTelemetry(20.0, 0.0);
    telem.mWheel[0].mTireLoad = 10000.0; // High load
    telem.mWheel[1].mTireLoad = 10000.0;

    // We need to settle the static load reference first
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 4000.0);
    FFBEngineTestAccess::SetDynamicWeightSmoothed(engine, 1.0);

    float genFFBTorque = 1.0f;

    // Run multiple frames to allow smoothing to settle if not bypassed
    for(int i=0; i<10; ++i) {
        engine.calculate_force(&telem, "GT3", "Ferrari 488", genFFBTorque);
    }
    double output = engine.calculate_force(&telem, "GT3", "Ferrari 488", genFFBTorque);

    // With Passthrough ENABLED, output should be 1.0 (no weight gain)
    ASSERT_NEAR(output, 1.0, 0.01);
}

} // namespace FFBEngineTests
