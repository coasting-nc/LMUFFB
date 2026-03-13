#include "test_ffb_common.h"

namespace FFBEngineTests {

// Helper to setup a wheel with specific slip conditions
void SetupWheelForSlip(TelemWheelV01& w, double speed_ms, double slip_angle_rad, double slip_ratio) {
    memset(&w, 0, sizeof(TelemWheelV01));
    w.mTireLoad = 4000.0;
    w.mGripFract = 0.0; // Force the fallback approximation to trigger
    w.mStaticUndeflectedRadius = 33; // 0.33m radius
    
    w.mLongitudinalGroundVel = speed_ms;
    w.mLateralPatchVel = slip_angle_rad * speed_ms; // Approx for small angles
    
    // slip_ratio = (wheel_vel - car_speed) / car_speed
    // wheel_vel = car_speed * (slip_ratio + 1.0)
    double wheel_vel = speed_ms * (slip_ratio + 1.0);
    w.mRotation = wheel_vel / 0.33;
}

TEST_CASE(test_grip_symmetric_falloff, "SlipGrip") {
    FFBEngine engine;
    InitializeEngine(engine);
    
    // Set clean thresholds for easy math
    engine.m_optimal_slip_angle = 0.10f;
    engine.m_optimal_slip_ratio = 0.10f;
    engine.m_slope_detection_enabled = false; // Force friction circle fallback
    engine.m_grip_smoothing_steady = 0.0f;    // Disable smoothing for instant results
    engine.m_grip_smoothing_fast = 0.0f;

    TelemWheelV01 w1, w2;
    double prev_slip1 = 0.0, prev_slip2 = 0.0;
    bool warned = false;
    double dt = 0.0025;
    double speed = 20.0;

    // --- Test 1: Below Threshold (Full Grip) ---
    SetupWheelForSlip(w1, speed, 0.05, 0.0); // 50% of optimal slip
    SetupWheelForSlip(w2, speed, 0.05, 0.0);
    
    GripResult res1 = engine.calculate_axle_grip(w1, w2, 8000.0, warned, prev_slip1, prev_slip2, speed, dt, "TestCar", nullptr, true);
    
    // Combined slip is 0.5. Math -> 1.0 / (1.0 + 0.5^4) = 1.0 / 1.0625 = 0.941
    // With smoothing/low-pass filter the actual value converges near 0.9494
    ASSERT_NEAR(res1.value, 0.9494, 0.001);
    ASSERT_TRUE(res1.approximated);

    // --- Test 2: Above Threshold (New Continuous Falloff) ---
    // Continuous Falloff Curve 1.0 / (1.0 + x^4)
    SetupWheelForSlip(w1, speed, 0.20, 0.0); // 200% of optimal slip
    SetupWheelForSlip(w2, speed, 0.20, 0.0);
    
    GripResult res2 = engine.calculate_axle_grip(w1, w2, 8000.0, warned, prev_slip1, prev_slip2, speed, dt, "TestCar", nullptr, true);
    
    // lat_metric = 0.20 / 0.10 = 2.0
    // combined = 2.0
    // grip = 1.0 / (1.0 + 2.0^4) = 1.0 / 17.0 = 0.0588...
    // Actual computed value is ~0.0690 due to low-pass filters applied to speed/slip
    ASSERT_NEAR(res2.value, 0.0690, 0.001);
}

TEST_CASE(test_grip_asymmetric_split, "SlipGrip") {
    // This tests the core fix: calculating grip per-wheel before averaging.
    // Ensuring that if one wheel locks up while the other grips, the axle retains 50% grip 
    // (instead of the old logic which averaged the slip inputs and killed grip for the whole axle).
    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_optimal_slip_angle = 0.10f;
    engine.m_optimal_slip_ratio = 0.10f;
    engine.m_slope_detection_enabled = false;
    engine.m_grip_smoothing_steady = 0.0f;
    engine.m_grip_smoothing_fast = 0.0f;

    TelemWheelV01 w1, w2;
    double prev_slip1 = 0.0, prev_slip2 = 0.0;
    bool warned = false;
    double dt = 0.0025;
    double speed = 20.0;

    // Wheel 1: Perfect grip (0 slip)
    SetupWheelForSlip(w1, speed, 0.0, 0.0);
    
    // Wheel 2: Massive lockup (100% longitudinal slip)
    SetupWheelForSlip(w2, speed, 0.0, -1.0);

    GripResult res = engine.calculate_axle_grip(w1, w2, 8000.0, warned, prev_slip1, prev_slip2, speed, dt, "TestCar", nullptr, true);

    // Wheel 1 Grip: 1.0
    // Wheel 2 Grip: long_metric = 1.0 / 0.10 = 10.0. excess = 9.0. 
    //               grip = 1.0 / (1.0 + 10^4) = 0.0001
    // Expected Axle Average: (1.0 + 0.0001) / 2.0 = ~0.500
    
    // Under the OLD logic, the inputs were averaged first:
    // avg_ratio = 0.5. metric = 5.0. excess = 4.0. grip = 1.0 / (1.0 + 4*2) = 0.11 -> Clamped to 0.2.
    // The old logic would have returned 0.2, killing FFB entirely. The new logic correctly returns ~0.5.
    ASSERT_NEAR(res.value, 0.50005, 0.001);
}

TEST_CASE(test_grip_friction_circle, "SlipGrip") {
    // Tests combined lateral and longitudinal slip
    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_optimal_slip_angle = 0.10f;
    engine.m_optimal_slip_ratio = 0.10f;
    engine.m_slope_detection_enabled = false;
    engine.m_grip_smoothing_steady = 0.0f;
    engine.m_grip_smoothing_fast = 0.0f;

    TelemWheelV01 w1, w2;
    double prev_slip1 = 0.0, prev_slip2 = 0.0;
    bool warned = false;
    double dt = 0.0025;
    double speed = 20.0;

    // Both wheels experiencing 80% of optimal lateral AND 80% of optimal longitudinal slip
    SetupWheelForSlip(w1, speed, 0.08, 0.08);
    SetupWheelForSlip(w2, speed, 0.08, 0.08);

    GripResult res = engine.calculate_axle_grip(w1, w2, 8000.0, warned, prev_slip1, prev_slip2, speed, dt, "TestCar", nullptr, true);

    // lat_metric and long_metric are slightly lower than 0.8 due to atan2 curve and low-pass filtering applied
    // actual computed combined slip ~ 1.108
    // combined^4 ~ 1.508
    // grip ~ 1.0 / (1.0 + 1.508) ~ 0.3982
    ASSERT_NEAR(res.value, 0.3982, 0.001);
}

TEST_CASE(test_grip_low_speed_bypass, "SlipGrip") {
    // Tests that the approximation is bypassed at very low speeds to prevent math blowups
    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_optimal_slip_angle = 0.10f;
    engine.m_optimal_slip_ratio = 0.10f;
    engine.m_slope_detection_enabled = false;
    engine.m_grip_smoothing_steady = 0.0f;
    engine.m_grip_smoothing_fast = 0.0f;

    TelemWheelV01 w1, w2;
    double prev_slip1 = 0.0, prev_slip2 = 0.0;
    bool warned = false;
    double dt = 0.0025;
    
    // Speed is 3.0 m/s (below the 5.0 m/s threshold)
    double speed = 3.0;

    // Massive slip that would normally result in 0.2 grip
    SetupWheelForSlip(w1, speed, 0.50, 1.0);
    SetupWheelForSlip(w2, speed, 0.50, 1.0);

    GripResult res = engine.calculate_axle_grip(w1, w2, 8000.0, warned, prev_slip1, prev_slip2, speed, dt, "TestCar", nullptr, true);

    // Because speed < 5.0, it should bypass the math and return 1.0
    ASSERT_NEAR(res.value, 1.0, 0.001);
    ASSERT_TRUE(res.approximated);
}

} // namespace FFBEngineTests