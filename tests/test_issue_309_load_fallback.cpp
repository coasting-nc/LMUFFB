#include "test_ffb_common.h"
#include "FFBEngine.h"
#include <iostream>

namespace FFBEngineTests {

TEST_CASE(test_issue_309_load_fallback_accuracy, "Physics") {
    std::cout << "\nTest: Issue #309 - Load Fallback Accuracy" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    // Enable Lateral Load effect to check if it uses the correct load
    engine.m_load_forces.lat_load_effect = 1.0f;
    engine.m_rear_axle.sop_scale = 1.0f;
    engine.m_rear_axle.sop_smoothing_factor = 0.0f; // No smoothing to get direct results

    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.01;
    data.mElapsedTime = 1.0;

    // Set some suspension forces
    data.mWheel[WHEEL_FL].mSuspForce = 2000.0; // FL
    data.mWheel[WHEEL_FR].mSuspForce = 2100.0; // FR
    data.mWheel[WHEEL_RL].mSuspForce = 1800.0; // RL
    data.mWheel[WHEEL_RR].mSuspForce = 1900.0; // RR

    // Tire loads are missing (0)
    data.mWheel[WHEEL_FL].mTireLoad = 0.0;
    data.mWheel[WHEEL_FR].mTireLoad = 0.0;
    data.mWheel[WHEEL_RL].mTireLoad = 0.0;
    data.mWheel[WHEEL_RR].mTireLoad = 0.0;

    data.mLocalVel.z = 20.0; // Speed to trigger missing load detection

    // We need enough frames to trigger MISSING_LOAD_WARN_THRESHOLD (default 20)
    for (int i = 0; i < 25; i++) {
        data.mElapsedTime += 0.01;
        engine.calculate_force(&data, "GT3", "TestCar");
    }

    auto snaps = engine.GetDebugBatch();
    ASSERT_FALSE(snaps.empty());
    const auto& last_snap = snaps.back();

    ASSERT_TRUE(last_snap.warn_load);

    // v0.7.171: GT3 MR = 0.65, Front Offset = 500, Rear Offset = 550
    double expected_fl = (2000.0 * 0.65) + 500.0; // 1800
    double expected_fr = (2100.0 * 0.65) + 500.0; // 1865
    double expected_rl = (1800.0 * 0.65) + 550.0; // 1720
    double expected_rr = (1900.0 * 0.65) + 550.0; // 1785

    double expected_front_load = (expected_fl + expected_fr) / 2.0; // 1832.5
    double expected_rear_load = (expected_rl + expected_rr) / 2.0;  // 1752.5

    std::cout << "  Front Load Snap: " << last_snap.calc_front_load << " Expected: " << expected_front_load << std::endl;
    std::cout << "  Rear Load Snap: " << last_snap.calc_rear_load << " Expected: " << expected_rear_load << std::endl;

    // Verify Lateral Load Force in Snap
    // lat_load_norm = (total_right - total_left) / total_all
    // left = fl + rl = 1800 + 1720 = 3520
    // right = fr + rr = 1865 + 1785 = 3650
    // total = 7170
    // norm = (3650 - 3520) / 7170 = 130 / 7170 = 0.0181311

    double expected_lat_load_force = (3650.0 - 3520.0) / 7170.0;
    std::cout << "  Lat Load Force Snap: " << last_snap.lat_load_force << " Expected: " << expected_lat_load_force << std::endl;

    ASSERT_NEAR(last_snap.calc_front_load, expected_front_load, 1.0);
    ASSERT_NEAR(last_snap.calc_rear_load, expected_rear_load, 1.0);
    ASSERT_NEAR(last_snap.lat_load_force, expected_lat_load_force, 0.001);
}

} // namespace FFBEngineTests
