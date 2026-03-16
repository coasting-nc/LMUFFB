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
    for (int i = 0; i < 4; i++) data.mWheel[i].mTireLoad = 0.0;
    // Missing vertical deflection
    for (int i = 0; i < 4; i++) data.mWheel[i].mVerticalTireDeflection = 0.0;
    // Missing suspension force
    for (int i = 0; i < 4; i++) data.mWheel[i].mSuspForce = 0.0;

    // Run for enough frames to trigger warnings (threshold is 20 for load, 50 for others)
    for (int i = 0; i < 60; i++) {
        data.mElapsedTime += 0.0025;
        engine.calculate_force(&data, "ClassA", "VehicleA");
    }

    // Verify warnings are active
    ASSERT_TRUE(FFBEngineTestAccess::GetMissingLoadFrames(engine) > 0);
    ASSERT_TRUE(FFBEngineTestAccess::GetWarnedLoad(engine));
    ASSERT_TRUE(FFBEngineTestAccess::GetMissingVertDeflectionFrames(engine) > 0);
    ASSERT_TRUE(FFBEngineTestAccess::GetWarnedVertDeflection(engine));
    ASSERT_TRUE(FFBEngineTestAccess::GetWarnedSuspForce(engine));

    std::cout << "  Warnings triggered for VehicleA. Switching to VehicleB..." << std::endl;

    // 2. Switch to Car B (with good telemetry)
    TelemInfoV01 dataB = CreateBasicTestTelemetry(20.0);
    // Good telemetry
    for (int i = 0; i < 4; i++) {
        dataB.mWheel[i].mTireLoad = 5000.0;
        dataB.mWheel[i].mVerticalTireDeflection = 0.01;
        dataB.mWheel[i].mSuspForce = 5000.0;
    }
    dataB.mElapsedTime = data.mElapsedTime + 0.1;

    // Trigger car change
    engine.calculate_force(&dataB, "ClassB", "VehicleB");

    // 3. Verify that counters and flags are reset
    // This is where the test is expected to FAIL before the fix
    int missing_load = FFBEngineTestAccess::GetMissingLoadFrames(engine);
    bool warned_load = FFBEngineTestAccess::GetWarnedLoad(engine);
    int missing_vert = FFBEngineTestAccess::GetMissingVertDeflectionFrames(engine);
    bool warned_vert = FFBEngineTestAccess::GetWarnedVertDeflection(engine);
    bool warned_susp = FFBEngineTestAccess::GetWarnedSuspForce(engine);

    std::cout << "  After switch: missing_load=" << missing_load
              << ", warned_load=" << warned_load
              << ", missing_vert=" << missing_vert
              << ", warned_vert=" << warned_vert
              << ", warned_susp=" << warned_susp << std::endl;

    ASSERT_EQ(missing_load, 0);
    ASSERT_FALSE(warned_load);
    ASSERT_EQ(missing_vert, 0);
    ASSERT_FALSE(warned_vert);
    ASSERT_FALSE(warned_susp);
}

} // namespace FFBEngineTests
