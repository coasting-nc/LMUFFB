#include "test_ffb_common.h"
#include "FFBEngine.h"
#include <iostream>

namespace FFBEngineTests {

TEST_CASE(test_issue_309_load_fallback_accuracy, "Physics") {
    std::cout << "\nTest: Issue #309 - Load Fallback Accuracy" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    // Enable Lateral Load effect to check if it uses the correct load
    engine.m_lat_load_effect = 1.0f;
    engine.m_sop_scale = 1.0f;
    engine.m_sop_smoothing_factor = 0.0f; // No smoothing to get direct results

    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.01;
    data.mElapsedTime = 1.0;

    // Set some suspension forces
    data.mWheel[0].mSuspForce = 2000.0; // FL
    data.mWheel[1].mSuspForce = 2100.0; // FR
    data.mWheel[2].mSuspForce = 1800.0; // RL
    data.mWheel[3].mSuspForce = 1900.0; // RR

    // Tire loads are missing (0)
    data.mWheel[0].mTireLoad = 0.0;
    data.mWheel[1].mTireLoad = 0.0;
    data.mWheel[2].mTireLoad = 0.0;
    data.mWheel[3].mTireLoad = 0.0;

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

    // approximate_load is suspForce + 300.
    double expected_fl = 2000.0 + 300.0; // 2300
    double expected_fr = 2100.0 + 300.0; // 2400
    double expected_rl = 1800.0 + 300.0; // 2100
    double expected_rr = 1900.0 + 300.0; // 2200

    double expected_front_load = (expected_fl + expected_fr) / 2.0; // 2350.0
    double expected_rear_load = (expected_rl + expected_rr) / 2.0;  // 2150.0

    std::cout << "  Front Load Snap: " << last_snap.calc_front_load << " Expected: " << expected_front_load << std::endl;
    std::cout << "  Rear Load Snap: " << last_snap.calc_rear_load << " Expected: " << expected_rear_load << std::endl;

    // Verify Lateral Load Force in Snap
    // lat_load_norm = (total_right - total_left) / total_all
    // left = fl + rl = 2300 + 2100 = 4400
    // right = fr + rr = 2400 + 2200 = 4600
    // total = 9000
    // norm = (4600 - 4400) / 9000 = 200 / 9000 = 0.02222...
    // lat_load_force = norm * m_lat_load_effect * m_sop_scale = 0.02222 * 1.0 * 1.0 = 0.02222

    double expected_lat_load_force = (4600.0 - 4400.0) / 9000.0;
    std::cout << "  Lat Load Force Snap: " << last_snap.lat_load_force << " Expected: " << expected_lat_load_force << std::endl;

    ASSERT_NEAR(last_snap.calc_front_load, expected_front_load, 1.0);
    ASSERT_NEAR(last_snap.calc_rear_load, expected_rear_load, 1.0);
    ASSERT_NEAR(last_snap.lat_load_force, expected_lat_load_force, 0.001);
}

} // namespace FFBEngineTests
