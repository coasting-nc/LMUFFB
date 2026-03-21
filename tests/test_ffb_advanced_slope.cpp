#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_slew_rate_limiter, "AdvancedSlope") {
    std::cout << "\nTest: Slew Rate Limiter (Curb Rejection) [Issue #397 Remediation]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;
    engine.m_slope_detection.g_slew_limit = 10.0f; // 10 G/s limit

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);

    // 1. Steady state (1.0G)
    data.mLocalAccel.x = 1.0 * 9.81;
    PumpEngineTime(engine, data, 0.5);

    ASSERT_NEAR(engine.m_slope_lat_g_prev, 1.0, 0.02);

    // 2. Spike to 5.0G
    data.mLocalAccel.x = 5.0 * 9.81;
    data.mElapsedTime += 0.01; // Advance time to trigger upsampler update

    // v0.7.198 uses fixed 0.0025 internal_dt for slew
    // Max change = limit * 0.0025 = 10 * 0.0025 = 0.025G per tick

    // Execute upsampler ticks.
    // Tick 0 (is_new_frame): Returns 1.0G (previous frame).
    // Tick 1: Returns 1.0 + 400*0.0025 = 2.0G. Slew limits to 1.025.

    engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.0025); // Tick 0
    engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.0025); // Tick 1

    std::cout << "  After 2 ticks (5.0G): Slew limited G = " << engine.m_slope_lat_g_prev << std::endl;
    ASSERT_NEAR(engine.m_slope_lat_g_prev, 1.025, 0.001);

    // Execute 3 more ticks (total 5 ticks)
    for(int i=0; i<3; i++) engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.0025);

    // After 5 ticks (1 game frame transition + 4 ramp ticks): 1.0 + 4 * 0.025 = 1.1G
    std::cout << "  After 5 ticks (5.0G): Slew limited G = " << engine.m_slope_lat_g_prev << std::endl;
    ASSERT_NEAR(engine.m_slope_lat_g_prev, 1.1, 0.001);
}

TEST_CASE(test_torque_slope_anticipation, "AdvancedSlope") {
    std::cout << "\nTest: Torque Slope Anticipation (Pneumatic Trail)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;
    engine.m_slope_detection.use_torque = true;
    engine.m_slope_detection.sg_window = 9;

    double dt = 0.01;
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = dt;

    // Simulate a corner entry where Torque drops while G is still rising (peak SAT)
    // dSteer/dt constant
    // dG/dt constant positive
    // dTorque/dt starts positive, then becomes negative (peak SAT reached)

    for (int i = 0; i < 60; i++) {
        double steer = 0.01 + (double)i * 0.01;
        double g = 0.5 + (double)i * 0.05;
        double slip = 0.01 + (double)i * 0.01;

        double torque = 0.0;
        if (i < 30) torque = 1.0 + (double)i * 0.1; // Rising
        else torque = 4.0 - (double)(i - 30) * 0.2; // Falling

        data.mUnfilteredSteering = steer;
        data.mLocalAccel.x = g * 9.81;
        data.mSteeringShaftTorque = torque;
        data.mWheel[0].mLateralPatchVel = slip * 20.0;
        data.mWheel[1].mLateralPatchVel = slip * 20.0;

        // Pump 4 ticks per frame
        for(int j=0; j<4; j++) {
            engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.0025);
        }
        data.mElapsedTime += 0.01;

        if (i == 40) { // Give SG window time to settle on the falling edge
            // At i=28, Torque has been falling for 8 frames.
            // G is still rising.
            // Torque Slope should be negative.
            // G Slope should be positive.
            std::cout << "  Frame 28: G-Slope=" << engine.m_slope_current << " Torque-Slope=" << engine.m_slope_torque_current << std::endl;
            ASSERT_TRUE(engine.m_slope_torque_current < 0.0);
            ASSERT_TRUE(engine.m_slope_current > 0.0);

            // Fusion logic should prioritize the grip loss (the negative slope)
            ASSERT_TRUE(engine.m_slope_smoothed_output < 0.99);
        }
    }
}

} // namespace FFBEngineTests
