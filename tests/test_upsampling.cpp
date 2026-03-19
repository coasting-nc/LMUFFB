#include "test_ffb_common.h"
#include <iostream>
#include <vector>

using namespace FFBEngineTests;

TEST_CASE(test_upsampling_logic, "Upsampling") {
    FFBEngine engine;
    InitializeEngine(engine);

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mElapsedTime = 100.0;
    data.mSteeringShaftTorque = 10.0;

    // Initial frame
    engine.calculate_force(&data, "GT3", "Ferrari 296", 0.0f, true, 0.0025);

    // Issue #397: Holt-Winters (Shaft Torque) will ramp to target.
    // We let it settle first.
    PumpEngineTime(engine, data, 1.0);

    // Simulate FFB loop running at 400Hz while telemetry is still at 100Hz (same timestamp)
    // Frame 2: Same telemetry, but 2.5ms later in FFB time
    double force2 = engine.calculate_force(&data, "GT3", "Ferrari 296", 0.0f, true, 0.0025);

    // Frame 3: Same telemetry, 5ms later
    double force3 = engine.calculate_force(&data, "GT3", "Ferrari 296", 0.0f, true, 0.0025);

    // Since torque is constant, and we use Holt-Winters/Extrapolation, it should stay stable
    // or smoothly transition if it was changing.
    // For constant input, it should be constant.
    ASSERT_NEAR(force2, force3, 0.001);
}

TEST_CASE(test_upsampling_interpolation, "Upsampling") {
    FFBEngine engine;
    InitializeEngine(engine);

    TelemInfoV01 data1 = CreateBasicTestTelemetry(20.0, 0.0);
    data1.mElapsedTime = 100.0;
    data1.mSteeringShaftTorque = 10.0;

    TelemInfoV01 data2 = data1;
    data2.mElapsedTime = 100.01; // 100Hz = 10ms gap
    data2.mSteeringShaftTorque = 20.0;

    // 1. Process data1
    engine.calculate_force(&data1, "GT3", "Ferrari 296", 0.0f, true, 0.0025);

    // 2. Process data2 (First sub-frame of new telemetry)
    // Holt-Winters will see the jump and start trending.
    double f1 = engine.calculate_force(&data2, "GT3", "Ferrari 296", 0.0f, true, 0.0025);

    // 3. Process data2 again (Second sub-frame, +2.5ms)
    double f2 = engine.calculate_force(&data2, "GT3", "Ferrari 296", 0.0f, true, 0.0025);

    // We expect f2 to be different from f1 as the upsampler moves towards the new target
    ASSERT_FALSE(std::abs(f1 - f2) < 1e-6);
}

TEST_CASE(test_upsampling_extrapolation, "Upsampling") {
    FFBEngine engine;
    InitializeEngine(engine);

    TelemInfoV01 data1 = CreateBasicTestTelemetry(20.0, 0.0);
    data1.mElapsedTime = 100.0;
    data1.mWheel[0].mLateralPatchVel = 1.0;

    TelemInfoV01 data2 = data1;
    data2.mElapsedTime = 100.01;
    data2.mWheel[0].mLateralPatchVel = 2.0;

    // Process two frames to establish velocity for extrapolation
    engine.calculate_force(&data1, "GT3", "Ferrari 296", 0.0f, true, 0.0025);
    engine.calculate_force(&data2, "GT3", "Ferrari 296", 0.0f, true, 0.0025); // This establishes dV/dt = (2-1)/0.01 = 100 m/s^2

    // Third call with SAME telemetry but +2.5ms FFB time
    // Interpolator should move from 1.0 towards 2.0.
    // Tick 1 (at data2 arrival): 1.0
    // Tick 2 (+2.5ms): 1.25
    engine.calculate_force(&data2, "GT3", "Ferrari 296", 0.0f, true, 0.0025);

    FFBSnapshot snap = engine.GetDebugBatch().back();

    // Verify that the interpolated value is increasing
    // We set data1.mWheel[0] = 1.0 and data2.mWheel[0] = 2.0.
    // The average (snap) started at 0.5 (1.0/2) and should move towards 1.0 (2.0/2).
    ASSERT_GT(snap.raw_front_lat_patch_vel, 0.5f);
}
