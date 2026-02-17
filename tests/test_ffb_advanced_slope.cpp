#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_slew_rate_limiter, "AdvancedSlope") {
    std::cout << "\nTest: Slew Rate Limiter (Curb Rejection)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    engine.m_slope_g_slew_limit = 10.0f; // 10 G/s limit

    double dt = 0.01;
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = dt;

    // 1. Steady state (1.0G)
    // v0.4.19 Fix: Use Negative for Right Turn
    data.mLocalAccel.x = -1.0 * 9.81;
    for (int i = 0; i < 20; i++) engine.calculate_force(&data);

    // Internal prev_val now stores the INVERTED G (Positive)
    ASSERT_NEAR(engine.m_slope_lat_g_prev, 1.0, 0.02); // Allow small smoothing error

    // 2. Spike to 5.0G (Right Turn)
    data.mLocalAccel.x = -5.0 * 9.81;
    engine.calculate_force(&data);

    // Max change = limit * dt = 10 * 0.01 = 0.1G
    // New value should be ~1.1G, not 5.0G
    std::cout << "  After spike (5.0G): Slew limited G = " << engine.m_slope_lat_g_prev << std::endl;
    ASSERT_NEAR(engine.m_slope_lat_g_prev, 1.1, 0.02);

    // 3. One more frame
    engine.calculate_force(&data);
    ASSERT_NEAR(engine.m_slope_lat_g_prev, 1.2, 0.02);
}

TEST_CASE(test_torque_slope_anticipation, "AdvancedSlope") {
    std::cout << "\nTest: Torque Slope Anticipation (Pneumatic Trail)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    engine.m_slope_use_torque = true;
    engine.m_slope_sg_window = 9;

    double dt = 0.01;
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = dt;

    // Simulate a corner entry where Torque drops while G is still rising (peak SAT)
    // dSteer/dt constant
    // dG/dt constant positive
    // dTorque/dt starts positive, then becomes negative (peak SAT reached)

    for (int i = 0; i < 40; i++) {
        double steer = 0.01 + (double)i * 0.01; // Increasing linearly
        double g = 0.5 + (double)i * 0.05;      // Increasing linearly
        double slip = 0.01 + (double)i * 0.01;  // Increasing linearly

        double torque = 0.0;
        if (i < 20) torque = 1.0 + (double)i * 0.1; // Rising
        else torque = 3.0 - (double)(i - 20) * 0.2; // Falling (Anticipation!)

        data.mUnfilteredSteering = steer;
        // v0.4.19 Fix: Use Negative for Right Turn
        data.mLocalAccel.x = -g * 9.81;
        data.mSteeringShaftTorque = torque;
        data.mWheel[0].mLateralPatchVel = slip * 20.0;
        data.mWheel[1].mLateralPatchVel = slip * 20.0;

        engine.calculate_force(&data);

        if (i == 28) { // Give SG window time to settle on the falling edge
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
