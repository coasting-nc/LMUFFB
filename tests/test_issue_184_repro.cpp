#include "test_ffb_common.h"
#include "../src/FFBEngine.h"

namespace FFBEngineTests {

void test_soft_lock_stationary_not_allowed() {
    std::cout << "Test: Soft Lock Stationary Not Allowed (Issue #184)" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    // Enable soft lock
    engine.m_soft_lock_enabled = true;
    engine.m_soft_lock_stiffness = 20.0f;
    engine.m_soft_lock_damping = 0.0f;
    engine.m_wheelbase_max_nm = 100.0f;
    engine.m_target_rim_nm = 100.0f;
    engine.m_gain = 1.0f;
    engine.m_steering_shaft_gain = 1.0f;

    // Set speed to 0
    double speed = 0.0;
    TelemInfoV01 data = CreateBasicTestTelemetry(speed, 0.0);

    // Ensure speed gate is at default
    engine.m_speed_gate_lower = 3.0f;
    engine.m_speed_gate_upper = 6.0f;

    // Normalization setup
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 100.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 100.0);

    // Try to trigger soft lock (steer = 1.1) with allowed = false
    data.mUnfilteredSteering = 1.1;
    double force = engine.calculate_force(&data, nullptr, nullptr, 0.0f, false); // allowed = false

    std::cout << "  Stationary (Speed=0) Force at 1.1 steer (allowed=false): " << force << std::endl;

    // We expect soft lock to work even if not allowed.
    // force_nm = -(0.1 * 20.0 * 50.0) = -100.0 Nm
    // norm_force = -100.0 / 100.0 = -1.0
    ASSERT_NEAR(force, -1.0, 0.01);

    // Verify that other forces are muted (e.g. if we add some steering shaft torque)
    data.mSteeringShaftTorque = 50.0; // Should be muted because allowed=false
    force = engine.calculate_force(&data, nullptr, nullptr, 0.0f, false);

    // Total should still be -1.0 (from soft lock) and not affected by shaft torque
    ASSERT_NEAR(force, -1.0, 0.01);

    // Verify snapshot
    auto batch = engine.GetDebugBatch();
    if (!batch.empty()) {
        auto& snap = batch.back();
        // snap.ffb_soft_lock is in Nm
        ASSERT_NEAR(snap.ffb_soft_lock, -100.0, 0.1);
        ASSERT_NEAR(snap.total_output, -1.0, 0.01);
        ASSERT_NEAR(snap.base_force, 0.0, 0.01); // base_input should be 0 because !allowed
    } else {
        std::cout << "[FAIL] No snapshot captured" << std::endl;
        g_tests_failed++;
    }
}

AutoRegister reg_soft_lock_stationary_not_allowed("Soft Lock Stationary Not Allowed", "Internal", {"Physics", "Issue184"}, test_soft_lock_stationary_not_allowed);

} // namespace
