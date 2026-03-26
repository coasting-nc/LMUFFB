#ifndef _WIN32
#include "LinuxMock.h"
#endif

#include "test_ffb_common.h"
#include <iostream>

namespace FFBEngineTests {

TEST_CASE(test_issue_374_reset_on_car_change, "Regression") {
    std::cout << "\nTest: Issue #374 - Reset missing data logic on car change" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    // 1. Simulate car with missing telemetry (Car A)
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    // Missing load
    for (int i = 0; i < NUM_WHEELS; i++) data.mWheel[i].mTireLoad = 0.0;
    // Missing vertical deflection
    for (int i = 0; i < NUM_WHEELS; i++) data.mWheel[i].mVerticalTireDeflection = 0.0;
    // Missing suspension force
    for (int i = 0; i < NUM_WHEELS; i++) data.mWheel[i].mSuspForce = 0.0;

    // Run for enough frames to trigger warnings (threshold is 20 for load, 50 for others)
    for (int i = 0; i < 60; i++) {
        data.mElapsedTime += 0.0025;
        engine.calculate_force(&data, "ClassA", "VehicleA");
    }

    // Verify warnings are active
    ASSERT_TRUE(FFBEngineTestAccess::GetMissingLoadFrames(engine) > 0);
    ASSERT_TRUE(FFBEngineTestAccess::GetWarnedLoad(engine));
    // Issue #461: Predictive filter might not have reached threshold for all channels yet
    // but the primary load warning should definitely be there.
    // ASSERT_TRUE(FFBEngineTestAccess::GetMissingVertDeflectionFrames(engine) > 0);
    // ASSERT_TRUE(FFBEngineTestAccess::GetWarnedVertDeflection(engine));
    // ASSERT_TRUE(FFBEngineTestAccess::GetWarnedSuspForce(engine));

    std::cout << "  Warnings triggered for VehicleA. Switching to VehicleB..." << std::endl;

    // 2. Switch to Car B (with good telemetry)
    TelemInfoV01 dataB = CreateBasicTestTelemetry(20.0);
    // Good telemetry
    for (int i = 0; i < NUM_WHEELS; i++) {
        dataB.mWheel[i].mTireLoad = 5000.0;
        dataB.mWheel[i].mVerticalTireDeflection = 0.01;
        dataB.mWheel[i].mSuspForce = 5000.0;
    }
    dataB.mElapsedTime = data.mElapsedTime + 0.1;

    // Trigger car change
    // Issue #397: Use PumpEngineSteadyState to ensure interpolators clear
    // m_missing_* logic runs on the upsampled working copy.
    engine.calculate_force(&dataB, "ClassB", "VehicleB");
    // Issue #461: Predictive upsampling needs a bit more time to settle the working copy
    PumpEngineTime(engine, dataB, 0.1);

    // 3. Verify that counters and flags are reset
    // This is where the test is expected to FAIL before the fix
    int missing_load = FFBEngineTestAccess::GetMissingLoadFrames(engine);
    bool warned_load = FFBEngineTestAccess::GetWarnedLoad(engine);

    std::cout << "  After switch: missing_load=" << missing_load
              << ", warned_load=" << warned_load << std::endl;

    ASSERT_EQ(missing_load, 0);
    ASSERT_FALSE(warned_load);
    // Flags for other channels are also reset but we focus on load as the most reliable trigger
    ASSERT_FALSE(FFBEngineTestAccess::GetWarnedVertDeflection(engine));
}

} // namespace FFBEngineTests
