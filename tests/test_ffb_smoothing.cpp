#include "test_ffb_common.h"

using namespace FFBEngineTests;

TEST_CASE(test_adaptive_smoothing_logic, "Physics") {
    std::cout << "\nTest: Adaptive Smoothing Logic (v0.7.47)" << std::endl;
    FFBEngine engine;
    double prev_out = 0.0;
    double dt = 0.01;
    double slow_tau = 0.1;
    double fast_tau = 0.0;
    double sensitivity = 1.0;

    // Action 1 (Steady): Input 0.1 (Delta 0.1 < Sensitivity)
    // Expected: Output should move slowly towards 0.1 (High Tau / Small Alpha)
    double input1 = 0.1;
    double out1 = FFBEngineTestAccess::CallApplyAdaptiveSmoothing(engine, input1, prev_out, dt, slow_tau, fast_tau, sensitivity);

    // Alpha = 0.01 / (0.1 + 0.01) = 1/11 approx 0.0909
    // prev_out = 0.0 + 0.0909 * (0.1 - 0.0) = 0.00909
    ASSERT_NEAR(out1, 0.00909, 0.001);
    ASSERT_EQ(prev_out, out1);

    // Action 2 (Transient): Input 10.0 (Delta 10.0 >> Sensitivity)
    // Expected: Output should jump almost instantly to 10.0 (Fast Tau / Large Alpha)
    double input2 = 10.0;
    double out2 = FFBEngineTestAccess::CallApplyAdaptiveSmoothing(engine, input2, prev_out, dt, slow_tau, fast_tau, sensitivity);

    // Delta = 10.0 - 0.00909 = 9.99091
    // t = 9.99091 / 1.0 = 9.99091 -> clamped to 1.0
    // tau = 0.1 + 1.0 * (0.0 - 0.1) = 0.0
    // Alpha = 0.01 / (0.0 + 0.01) = 1.0
    // prev_out = 0.00909 + 1.0 * (10.0 - 0.00909) = 10.0
    ASSERT_NEAR(out2, 10.0, 0.0001);
}

TEST_CASE(test_dynamic_weight_lpf, "Physics") {
    std::cout << "\nTest: Dynamic Weight LPF (v0.7.47)" << std::endl;
    FFBEngine engine;
    engine.m_dynamic_weight_gain = 1.0f;
    engine.m_dynamic_weight_smoothing = 1.0f; // Very slow
    FFBEngineTestAccess::SetDynamicWeightSmoothed(engine, 1.0);
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 4000.0);

    // Setup telemetry with higher load
    TelemInfoV01 data = CreateBasicTestTelemetry(10.0, 0.0);
    data.mDeltaTime = 0.01f;
    data.mWheel[0].mTireLoad = 8000.0f; // 2x load
    data.mWheel[1].mTireLoad = 8000.0f;

    // Action: Run calculate_force
    engine.calculate_force(&data);

    // Load Ratio = 8000 / 4000 = 2.0
    // dynamic_weight_factor (target) = 1.0 + (2.0 - 1.0) * 1.0 = 2.0
    // Alpha = 0.01 / (1.0 + 0.01) approx 0.01
    // smoothed = 1.0 + 0.01 * (2.0 - 1.0) = 1.01

    double smoothed = FFBEngineTestAccess::GetDynamicWeightSmoothed(engine);
    ASSERT_NEAR(smoothed, 1.01, 0.001);
}

TEST_CASE(test_grip_smoothing_integration, "Physics") {
    std::cout << "\nTest: Grip Smoothing Integration (v0.7.47)" << std::endl;
    FFBEngine engine;
    engine.m_grip_smoothing_steady = 1.0f; // Very slow
    engine.m_grip_smoothing_fast = 1.0f;   // Also slow for this test
    engine.m_grip_smoothing_sensitivity = 1.0f;
    FFBEngineTestAccess::SetFrontGripSmoothedState(engine, 1.0);

    TelemWheelV01 w1, w2;
    w1.mGripFract = 0.5f;
    w1.mTireLoad = 1000.0f;
    w2.mGripFract = 0.5f;
    w2.mTireLoad = 1000.0f;

    double prev_slip1 = 0.0, prev_slip2 = 0.0;
    bool warned = false;

    // Action: Run calculate_grip
    auto res = engine.calculate_grip(w1, w2, 2000.0, warned, prev_slip1, prev_slip2, 20.0, 0.01, "Test", nullptr, true);

    // Expected: result.value should be close to 1.0, not 0.5, due to 1.0s smoothing
    // Alpha = 0.01 / (1.0 + 0.01) approx 0.01
    // value = 1.0 + 0.01 * (0.5 - 1.0) = 0.995
    ASSERT_NEAR(res.value, 0.995, 0.001);
}
