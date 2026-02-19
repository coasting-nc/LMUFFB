// tests/test_ffb_dynamic_weight.cpp
#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_load_weighted_grip, "Physics") {
    FFBEngine engine;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);

    // Wheel 0 (Outside): Load = 10000N, Grip = 0.8 (Sliding)
    data.mWheel[0].mTireLoad = 10000.0;
    data.mWheel[0].mGripFract = 0.8;

    // Wheel 1 (Inside): Load = 500N, Grip = 1.0 (Not sliding)
    data.mWheel[1].mTireLoad = 500.0;
    data.mWheel[1].mGripFract = 1.0;

    double prev_slip1 = 0.0;
    double prev_slip2 = 0.0;
    bool warned = false;

    GripResult result = engine.calculate_grip(
        data.mWheel[0], data.mWheel[1], 5250.0, warned,
        prev_slip1, prev_slip2, 20.0, 0.0025, "TestCar", &data, true
    );

    // Simple Average would be (0.8 + 1.0) / 2.0 = 0.9
    // Weighted Average should be (0.8 * 10000 + 1.0 * 500) / 10500 = (8000 + 500) / 10500 = 8500 / 10500 approx 0.8095

    std::cout << "[INFO] Load-Weighted Grip Result: " << result.original << " (Simple Average would be 0.9)" << std::endl;

    ASSERT_NEAR(result.original, 0.8095, 0.01);
}

TEST_CASE(test_dynamic_weight_scaling, "Physics") {
    FFBEngine engine;
    Preset p;
    p.dynamic_weight_gain = 1.0f; // Enable
    p.dynamic_weight_smoothing = 0.0f; // Disable smoothing for instant test
    p.steering_shaft_gain = 1.0f;
    p.understeer = 0.0f; // Disable understeer drop for pure gain test
    p.invert_force = false; // Easier to test
    p.wheelbase_max_nm = 100.0f;
    p.target_rim_nm = 100.0f;
    p.Apply(engine);

    // v0.7.67 Fix for Issue #152: Ensure consistent scaling for test
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 100.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 100.0);
    FFBEngineTestAccess::SetRollingAverageTorque(engine, 100.0);
    FFBEngineTestAccess::SetLastRawTorque(engine, 100.0);

    // Seed static load
    // Need to call calculate_force multiple times at low speed to learn static load
    TelemInfoV01 data = CreateBasicTestTelemetry(5.0, 0.0);
    data.mWheel[0].mTireLoad = 4000.0;
    data.mWheel[1].mTireLoad = 4000.0;
    data.mLocalAccel.x = 0.0;
    data.mSteeringShaftTorque = 5.0;

    // Learn for a few frames
    for(int i=0; i<100; ++i) {
        engine.calculate_force(&data);
    }

    // Now heavy braking (load transfer to front)
    data.mWheel[0].mTireLoad = 8000.0;
    data.mWheel[1].mTireLoad = 8000.0;
    data.mLocalAccel.z = 5.0; // Braking
    data.mLocalAccel.x = 0.0;
    data.mSteeringShaftTorque = 5.0;

    double output = engine.calculate_force(&data);

    // Master Gain = 1.0, MaxTorqueRef = 100.0 (from Default)
    // base_input = 5.0
    // Load Ratio = 8000 / 4000 = 2.0
    // Factor = 1.0 + (2.0 - 1.0) * 1.0 = 2.0
    // Total Nm = 5.0 * 2.0 = 10.0
    // Norm Force = 10.0 / 100.0 = 0.1

    std::cout << "[INFO] Dynamic Weight Output: " << output << " (Expected 0.1)" << std::endl;

    ASSERT_NEAR(output, 0.1, 0.01);
}

TEST_CASE(test_dynamic_weight_safety_gate, "Physics") {
    FFBEngine engine;
    Preset p;
    p.dynamic_weight_gain = 1.0f;
    p.invert_force = false;
    p.wheelbase_max_nm = 100.0f;
    p.target_rim_nm = 100.0f;
    p.Apply(engine);

    // v0.7.67 Fix for Issue #152: Ensure consistent scaling for test
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 100.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 100.0);
    FFBEngineTestAccess::SetRollingAverageTorque(engine, 100.0);
    FFBEngineTestAccess::SetLastRawTorque(engine, 100.0);

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mWheel[0].mTireLoad = 0.0; // Trigger fallback
    data.mWheel[1].mTireLoad = 0.0;
    data.mSteeringShaftTorque = 5.0;

    // Run multiple frames to trigger warned_load hysteresis
    for(int i=0; i<30; ++i) {
        engine.calculate_force(&data);
    }

    double output = engine.calculate_force(&data);

    // warned_load should be true, dynamic weight should be disabled (factor 1.0)
    // base_input = 5.0
    // factor = 1.0
    // Total Nm = 5.0
    // Norm Force = 0.05

    std::cout << "[INFO] Safety Gate Output: " << output << " (Expected 0.05)" << std::endl;
    ASSERT_NEAR(output, 0.05, 0.01);
}

} // namespace FFBEngineTests
