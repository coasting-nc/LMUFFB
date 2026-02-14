#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_slope_directional_symmetry, "SlopeEdgeCases") {
    std::cout << "\nTest: Slope Directional Symmetry (Left vs Right Turn)" << std::endl;
    FFBEngine engine_right;
    InitializeEngine(engine_right);
    engine_right.m_slope_detection_enabled = true;
    engine_right.m_slope_use_torque = true;

    FFBEngine engine_left;
    InitializeEngine(engine_left);
    engine_left.m_slope_detection_enabled = true;
    engine_left.m_slope_use_torque = true;

    double dt = 0.01;
    TelemInfoV01 data_right = CreateBasicTestTelemetry(20.0);
    data_right.mDeltaTime = dt;

    TelemInfoV01 data_left = CreateBasicTestTelemetry(20.0);
    data_left.mDeltaTime = dt;

    // Ramp up both
    for (int i = 0; i < 20; i++) {
        double steer = (double)i * 0.01;
        double torque = (double)i * 0.1;
        double g = (double)i * 0.05;
        double slip = (double)i * 0.01;

        data_right.mUnfilteredSteering = steer;
        data_right.mSteeringShaftTorque = torque;
        data_right.mLocalAccel.x = g * 9.81;
        data_right.mWheel[0].mLateralPatchVel = slip * 20.0;
        data_right.mWheel[1].mLateralPatchVel = slip * 20.0;

        data_left.mUnfilteredSteering = -steer;
        data_left.mSteeringShaftTorque = -torque;
        data_left.mLocalAccel.x = -g * 9.81;
        data_left.mWheel[0].mLateralPatchVel = -slip * 20.0;
        data_left.mWheel[1].mLateralPatchVel = -slip * 20.0;

        engine_right.calculate_force(&data_right);
        engine_left.calculate_force(&data_left);
    }

    fprintf(stderr, "  Right Turn: slope_torque=%f grip=%f\n", engine_right.m_slope_torque_current, engine_right.m_slope_smoothed_output);
    fprintf(stderr, "  Left Turn: slope_torque=%f grip=%f\n", engine_left.m_slope_torque_current, engine_left.m_slope_smoothed_output);

    // Should be symmetric
    ASSERT_NEAR(engine_right.m_slope_smoothed_output, engine_left.m_slope_smoothed_output, 0.01);
}

TEST_CASE(test_slope_slow_hands_confidence, "SlopeEdgeCases") {
    std::cout << "\nTest: Slope Slow Hands Confidence" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    engine.m_slope_alpha_threshold = 0.02f;
    // Current hardcoded confidence limit is 0.10.
    // If we have dAlpha_dt = 0.04, confidence would be (0.04-0.02)/(0.10-0.02) = 0.25 (ish)

    double dt = 0.01;
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = dt;

    // Ramp steering slowly: dAlpha/dt = 0.04 rad/s
    for (int i = 0; i < 50; i++) {
        double alpha = (double)i * 0.04 * dt;
        data.mWheel[0].mLateralPatchVel = alpha * 20.0;
        data.mWheel[1].mLateralPatchVel = alpha * 20.0;

        // Simulate massive grip loss
        data.mLocalAccel.x = 0.1 * 9.81;

        engine.calculate_force(&data);
    }

    std::cout << "  Slow Hands Grip Factor: " << engine.m_slope_smoothed_output << std::endl;

    // If confidence is low, grip factor won't drop much.
    // We want it to drop even with slow hands if we make it configurable.
    // For now, let's see where it is.
    // ASSERT_LE(engine.m_slope_smoothed_output, 0.5);
}

TEST_CASE(test_torque_vs_g_timing, "SlopeEdgeCases") {
    std::cout << "\nTest: Torque vs G Timing (Anticipation)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    engine.m_slope_use_torque = true;
    engine.m_slope_sg_window = 9;

    double dt = 0.01;
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = dt;

    for (int i = 0; i < 40; i++) {
        double steer = 0.01 + (double)i * 0.01;
        double g = 0.5 + (double)i * 0.05;
        double slip = 0.01 + (double)i * 0.01;

        double torque = 0.0;
        if (i < 20) torque = 1.0 + (double)i * 0.1;
        else torque = 3.0 - (double)(i - 20) * 0.2; // Torque drops at i=20

        data.mUnfilteredSteering = steer;
        data.mLocalAccel.x = g * 9.81; // G keeps rising
        data.mSteeringShaftTorque = torque;
        data.mWheel[0].mLateralPatchVel = slip * 20.0;
        data.mWheel[1].mLateralPatchVel = slip * 20.0;

        engine.calculate_force(&data);

        if (i == 25) {
            std::cout << "  Frame 25: Grip Factor=" << engine.m_slope_smoothed_output << std::endl;
            // Torque slope should have detected loss, while G slope hasn't.
            ASSERT_TRUE(engine.m_slope_smoothed_output < 0.95);
        }
    }
}

} // namespace FFBEngineTests
