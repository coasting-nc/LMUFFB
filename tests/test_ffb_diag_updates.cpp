#include "test_ffb_common.h"
#include "FFBEngine.h"

namespace FFBEngineTests {

TEST_CASE(TestFFBTorqueSnapshot, "Diagnostics") {
    FFBEngine engine;
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));

    data.mDeltaTime = 0.0025;
    data.mElapsedTime = 10.0;
    data.mSteeringShaftTorque = 5.67;

    float genFFBTorque = 12.34f;

    // Run physics tick
    engine.calculate_force(&data, "GT3", "Ferrari 296 GT3", genFFBTorque);

    // Retrieve snapshots
    auto snapshots = engine.GetDebugBatch();

    ASSERT_GT((int)snapshots.size(), 0);

    const auto& snap = snapshots.back();

    // Verify torque propagation
    ASSERT_NEAR(snap.raw_shaft_torque, 5.67f, 0.001f);
    ASSERT_NEAR(snap.raw_gen_torque, 12.34f, 0.001f);

    // Verify steer_force depends on selected source (default is 0: Shaft Torque)
    ASSERT_NEAR(snap.steer_force, 5.67f, 0.001f);

    // Switch source to 1: Direct Torque
    engine.m_torque_source = 1;
    engine.m_wheelbase_max_nm = 1.0f; engine.m_target_rim_nm = 1.0f; // Scale by 1.0 for easy verification
    engine.calculate_force(&data, "GT3", "Ferrari 296 GT3", genFFBTorque);

    snapshots = engine.GetDebugBatch();
    ASSERT_GT((int)snapshots.size(), 0);
    const auto& snap2 = snapshots.back();

    ASSERT_NEAR(snap2.raw_shaft_torque, 5.67f, 0.001f);
    ASSERT_NEAR(snap2.raw_gen_torque, 12.34f, 0.001f);
    ASSERT_NEAR(snap2.steer_force, 12.34f, 0.001f);
}

} // namespace FFBEngineTests
