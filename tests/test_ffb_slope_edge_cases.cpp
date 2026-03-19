#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_slope_asymmetry_fix, "SlopeEdgeCases") {
    std::cout << "\nTest: Slope Asymmetry Fix (Left vs Right)" << std::endl;

    auto simulate_turn = [&](bool right) {
        FFBEngine test_engine;
        InitializeEngine(test_engine);
        test_engine.m_slope_detection_enabled = true;
        test_engine.m_slope_sg_window = 9;
        test_engine.m_slope_alpha_threshold = 0.02f;

        TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
        data.mDeltaTime = 0.01;

        double dir = right ? 1.0 : -1.0;

        for (int i = 0; i < 60; i++) {
            double steer = (double)i * 0.01 * dir;
            double g = (double)i * 0.05 * dir;
            double torque = (double)i * 0.1 * dir;
            double slip = (double)i * 0.01;

            data.mUnfilteredSteering = steer;
            data.mLocalAccel.x = g * 9.81;
            data.mSteeringShaftTorque = torque;
            data.mWheel[0].mLateralPatchVel = slip * 20.0;
            data.mWheel[1].mLateralPatchVel = slip * 20.0;

            // Pump 4 ticks per frame
            for(int j=0; j<4; j++) {
                test_engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.0025);
            }
            data.mElapsedTime += 0.01;
        }
        return test_engine.m_slope_smoothed_output;
    };

    double grip_right = simulate_turn(true);
    double grip_left = simulate_turn(false);

    std::cout << "  Grip Factor Right: " << grip_right << std::endl;
    std::cout << "  Grip Factor Left:  " << grip_left << std::endl;

    ASSERT_NEAR(grip_right, grip_left, 0.001);
}

TEST_CASE(test_slope_confidence_tuning, "SlopeEdgeCases") {
    std::cout << "\nTest: Slope Confidence Tuning" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    engine.m_slope_alpha_threshold = 0.02f;

    double dAlpha_dt = 0.05;

    // Case 1: Default (max_rate = 0.10)
    engine.m_slope_confidence_max_rate = 0.10f;
    double conf1 = engine.calculate_slope_confidence(dAlpha_dt);

    // Case 2: Tuned (max_rate = 0.05)
    engine.m_slope_confidence_max_rate = 0.05f;
    double conf2 = engine.calculate_slope_confidence(dAlpha_dt);

    std::cout << "  Confidence (max_rate=0.10): " << conf1 << std::endl;
    std::cout << "  Confidence (max_rate=0.05): " << conf2 << std::endl;

    ASSERT_TRUE(conf2 > conf1);
    ASSERT_NEAR(conf2, 1.0, 0.001);
}

TEST_CASE(test_torque_slope_timing, "SlopeEdgeCases") {
    std::cout << "\nTest: Torque Slope Timing (Anticipation) [Issue #397 Remediation]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    engine.m_slope_use_torque = true;
    engine.m_slope_sg_window = 9;
    engine.m_slope_alpha_threshold = 0.02f;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);

    // Simulate peak SAT: Torque drops while G is still rising
    for (int i = 0; i < 60; i++) {
        double steer = 0.01 + (double)i * 0.01;
        double g = 0.5 + (double)i * 0.05;
        double slip = 0.01 + (double)i * 0.01;

        double torque = (i < 30) ? (1.0 + i * 0.1) : (4.0 - (i - 30) * 0.2);

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

        if (i == 40) {
            std::cout << "  Frame 40: Torque Slope = " << engine.m_slope_torque_current << std::endl;
            ASSERT_TRUE(engine.m_slope_torque_current < 0.0);
            ASSERT_TRUE(engine.m_slope_smoothed_output < 0.99);
        }
    }
}

} // namespace FFBEngineTests
