#include "test_ffb_common.h"

namespace FFBEngineTests {

// Helper to setup a wheel with exact slip angle and ratio
void SetupWheelForGripTest(TelemWheelV01& w, double speed_ms, double slip_angle_rad, double slip_ratio, double load_n) {
    w.mLongitudinalGroundVel = speed_ms;
    // y = x * tan(angle) for exact atan2 recovery
    w.mLateralPatchVel = speed_ms * std::tan(slip_angle_rad); 
    w.mLongitudinalPatchVel = slip_ratio * speed_ms;
    w.mTireLoad = load_n;
    w.mSuspForce = load_n; // Keep simple for tests
    w.mGripFract = 0.0;    // Force fallback approximation
    w.mStaticUndeflectedRadius = 33; // 0.33m
    w.mRotation = (speed_ms + w.mLongitudinalPatchVel) / 0.33;
}

TEST_CASE(test_grip_estimation_raw_passthrough, "SlipGrip") {
    FFBEngine engine;
    InitializeEngine(engine);
    
    TelemWheelV01 w1, w2;
    SetupWheelForGripTest(w1, 20.0, 0.0, 0.0, 4000.0);
    SetupWheelForGripTest(w2, 20.0, 0.0, 0.0, 4000.0);
    
    // Set raw grip to valid values
    w1.mGripFract = 0.85;
    w2.mGripFract = 0.75;
    
    bool warned = false;
    double prev_slip1 = 0.0, prev_slip2 = 0.0;
    double prev_load1 = 4000.0, prev_load2 = 4000.0;
    
    GripResult res = engine.calculate_axle_grip(w1, w2, 4000.0, warned, prev_slip1, prev_slip2, prev_load1, prev_load2, 20.0, 0.0025, "TestCar", nullptr, true);
    
    ASSERT_FALSE(res.approximated);
    ASSERT_NEAR(res.value, 0.80, 0.001); // Average of 0.85 and 0.75
}

TEST_CASE(test_grip_estimation_continuous_falloff, "SlipGrip") {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_optimal_slip_angle = 0.10f;
    engine.m_optimal_slip_ratio = 0.12f;
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 4000.0);
    
    TelemWheelV01 w1, w2;
    // Exactly at optimal slip angle (0.10 rad)
    SetupWheelForGripTest(w1, 20.0, 0.10, 0.0, 4000.0);
    SetupWheelForGripTest(w2, 20.0, 0.10, 0.0, 4000.0);
    
    bool warned = false;
    double prev_slip1 = 0.10, prev_slip2 = 0.10; // Pre-warm slip LPF
    double prev_load1 = 4000.0, prev_load2 = 4000.0; // Pre-warm load EMA
    
    GripResult res = engine.calculate_axle_grip(w1, w2, 4000.0, warned, prev_slip1, prev_slip2, prev_load1, prev_load2, 20.0, 0.0025, "TestCar", nullptr, true);
    
    ASSERT_TRUE(res.approximated);
    
    // Math check:
    // combined_slip = 0.10 / 0.10 = 1.0
    // cs4 = 1.0^4 = 1.0
    // grip = 0.05 + (0.95 / (1.0 + 1.0)) = 0.05 + 0.475 = 0.525
    ASSERT_NEAR(res.value, 0.525, 0.005);
}

TEST_CASE(test_grip_estimation_load_sensitivity, "SlipGrip") {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_optimal_slip_angle = 0.10f;
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 4000.0);
    
    TelemWheelV01 w1, w2;
    // 2x Static Load (e.g., heavy aero downforce)
    SetupWheelForGripTest(w1, 20.0, 0.10, 0.0, 8000.0);
    SetupWheelForGripTest(w2, 20.0, 0.10, 0.0, 8000.0);
    
    bool warned = false;
    double prev_slip1 = 0.10, prev_slip2 = 0.10;
    double prev_load1 = 8000.0, prev_load2 = 8000.0; // Pre-warm load EMA to simulate sustained aero
    
    GripResult res = engine.calculate_axle_grip(w1, w2, 8000.0, warned, prev_slip1, prev_slip2, prev_load1, prev_load2, 20.0, 0.0025, "TestCar", nullptr, true);
    
    // Math check:
    // load_ratio = 8000 / 4001 = 1.9995
    // dynamic_slip_angle = 0.10 * pow(1.9995, 0.333) = 0.12598
    // lat_metric = 0.10 / 0.12598 = 0.7937
    // cs4 = 0.7937^4 = 0.3971
    // grip = 0.05 + (0.95 / (1.0 + 0.3971)) = 0.7299
    
    // Notice how grip is ~0.73 here, compared to 0.525 in the previous test at the exact same slip angle!
    ASSERT_NEAR(res.value, 0.730, 0.01);
}

TEST_CASE(test_grip_estimation_load_smoothing, "SlipGrip") {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_optimal_slip_angle = 0.10f;
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 4000.0);
    
    TelemWheelV01 w1, w2;
    // Massive instantaneous load spike (e.g., hitting a sausage curb)
    SetupWheelForGripTest(w1, 20.0, 0.10, 0.0, 12000.0);
    SetupWheelForGripTest(w2, 20.0, 0.10, 0.0, 12000.0);
    
    bool warned = false;
    double prev_slip1 = 0.10, prev_slip2 = 0.10;
    // Load EMA is currently at static weight (4000 N)
    double prev_load1 = 4000.0, prev_load2 = 4000.0; 
    
    GripResult res = engine.calculate_axle_grip(w1, w2, 12000.0, warned, prev_slip1, prev_slip2, prev_load1, prev_load2, 20.0, 0.0025, "TestCar", nullptr, true);
    
    // Math check:
    // alpha = 0.0025 / (0.050 + 0.0025) = 0.047619
    // smoothed_load = 4000 + 0.047619 * (12000 - 4000) = 4380.95
    // load_ratio = 4380.95 / 4001 = 1.0949
    // dynamic_slip_angle = 0.10 * pow(1.0949, 0.333) = 0.1030
    // lat_metric = 0.10 / 0.1030 = 0.9708
    // cs4 = 0.9708^4 = 0.888
    // grip = 0.05 + (0.95 / (1.0 + 0.888)) = 0.553
    
    // If smoothing failed, grip would jump to ~0.83. Because of smoothing, it stays near 0.55.
    ASSERT_NEAR(res.value, 0.553, 0.01);
    
    // Verify the state variable was updated correctly for the next frame
    ASSERT_NEAR(prev_load1, 4380.95, 1.0);
}

TEST_CASE(test_grip_estimation_sliding_asymptote, "SlipGrip") {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_optimal_slip_angle = 0.10f;
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 4000.0);
    
    TelemWheelV01 w1, w2;
    // Massive slip angle (1.0 rad = ~57 degrees, fully sideways)
    SetupWheelForGripTest(w1, 20.0, 1.0, 0.0, 4000.0);
    SetupWheelForGripTest(w2, 20.0, 1.0, 0.0, 4000.0);
    
    bool warned = false;
    double prev_slip1 = 1.0, prev_slip2 = 1.0;
    double prev_load1 = 4000.0, prev_load2 = 4000.0;
    
    GripResult res = engine.calculate_axle_grip(w1, w2, 4000.0, warned, prev_slip1, prev_slip2, prev_load1, prev_load2, 20.0, 0.0025, "TestCar", nullptr, true);
    
    // Should asymptote to MIN_SLIDING_GRIP (0.05), not 0.0
    ASSERT_NEAR(res.value, 0.05, 0.001);
}

TEST_CASE(test_grip_estimation_combined_slip, "SlipGrip") {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_optimal_slip_angle = 0.10f;
    engine.m_optimal_slip_ratio = 0.12f;
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 4000.0);
    
    TelemWheelV01 w1, w2;
    // Trail braking scenario: 80% of optimal slip angle, 80% of optimal slip ratio
    SetupWheelForGripTest(w1, 20.0, 0.08, 0.096, 4000.0);
    SetupWheelForGripTest(w2, 20.0, 0.08, 0.096, 4000.0);
    
    bool warned = false;
    double prev_slip1 = 0.08, prev_slip2 = 0.08;
    double prev_load1 = 4000.0, prev_load2 = 4000.0;
    
    GripResult res = engine.calculate_axle_grip(w1, w2, 4000.0, warned, prev_slip1, prev_slip2, prev_load1, prev_load2, 20.0, 0.0025, "TestCar", nullptr, true);
    
    // Math check:
    // lat_metric = 0.08 / 0.10 = 0.8
    // long_metric = 0.096 / 0.12 = 0.8
    // combined_slip = sqrt(0.8^2 + 0.8^2) = sqrt(1.28) = 1.131
    // cs4 = 1.131^4 = 1.6384
    // grip = 0.05 + (0.95 / (1.0 + 1.6384)) = 0.05 + 0.360 = 0.410
    
    // If it only looked at lateral slip (0.8), grip would be ~0.72. 
    // Because of combined slip (trail braking), grip drops to ~0.41.
    ASSERT_NEAR(res.value, 0.410, 0.01);
}

} // namespace FFBEngineTests
