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

TEST_CASE(test_upsampling_torque_hw, "Upsampling") {
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

TEST_CASE(test_upsampling_interpolation, "Upsampling") { // Renamed from extrapolation
    FFBEngine engine;
    InitializeEngine(engine);

    TelemInfoV01 data1 = CreateBasicTestTelemetry(20.0, 0.0);
    data1.mElapsedTime = 100.0;
    data1.mWheel[0].mLateralPatchVel = 1.0;

    TelemInfoV01 data2 = data1;
    data2.mElapsedTime = 100.01;
    data2.mWheel[0].mLateralPatchVel = 2.0;

    // Process frames to establish interpolation window
    engine.calculate_force(&data1, "GT3", "Ferrari 296", 0.0f, true, 0.0025);

    // T=100.01: New frame arrives. Interpolator target moves to 2.0. Output starts at 1.0.
    engine.calculate_force(&data2, "GT3", "Ferrari 296", 0.0f, true, 0.0025);
    ASSERT_NEAR(engine.GetDebugBatch().back().raw_front_lat_patch_vel, 0.5f, 0.01); // (1.0 + 0)/2 = 0.5

    // T=100.0125: +2.5ms FFB time. Interpolator should be at 1.0 + (100 * 0.0025) = 1.25
    engine.calculate_force(&data2, "GT3", "Ferrari 296", 0.0f, true, 0.0025);

    FFBSnapshot snap = engine.GetDebugBatch().back();

    // snap.raw_front_lat_patch_vel is abs average of fl/fr.
    // FL=1.25, FR=0 (if default). Avg = 0.625.
    ASSERT_NEAR(snap.raw_front_lat_patch_vel, 0.625f, 0.01);
}
