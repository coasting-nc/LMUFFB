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
    // Extrapolator should predict 2.0 + (100 * 0.0025) = 2.25
    engine.calculate_force(&data2, "GT3", "Ferrari 296", 0.0f, true, 0.0025);

    FFBSnapshot snap = engine.GetDebugBatch().back();
    // snap.raw_front_lat_patch_vel is usually abs average of fl/fr
    // Here we only set fl, fr is 0 from CreateBasicTestTelemetry (needs check)
    // CreateBasicTestTelemetry usually sets some defaults.

    // Verification via snapshot
    // We want to see if the internal m_working_info was actually upsampled.
    // Since we can't easily see m_working_info, we check effects that depend on it.
    // Slide texture depends on mLateralPatchVel.
    ASSERT_GT(snap.raw_front_lat_patch_vel, 1.0f);
}
