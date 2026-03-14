#include "test_ffb_common.h"
#include "../src/ffb/FFBEngine.h"

namespace FFBEngineTests {

void test_soft_lock() {
    std::cout << "Test: Soft Lock Logic (Issue #117)" << std::endl;

    auto run_step = [](FFBEngine& engine, TelemInfoV01& data, double steer) {
        data.mUnfilteredSteering = steer;
        data.mDeltaTime = 0.0025; // Force 400Hz
        return engine.calculate_force(&data);
    };

    {
        FFBEngine engine;
        InitializeEngine(engine);
        TelemInfoV01 data = CreateBasicTestTelemetry();
        engine.m_soft_lock_enabled = true;
        engine.m_soft_lock_stiffness = 20.0f;
        engine.m_soft_lock_damping = 0.0f;
        engine.m_wheelbase_max_nm = 100.0f; engine.m_target_rim_nm = 100.0f;
        engine.m_gain = 1.0f;
        engine.m_invert_force = false;
        engine.m_steering_shaft_gain = 0.0f;

        // v0.7.67 Fix for Issue #152: Ensure normalization matches the test scaling
        FFBEngineTestAccess::SetSessionPeakTorque(engine, 100.0);
        FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 100.0);
        FFBEngineTestAccess::SetRollingAverageTorque(engine, 100.0);
        FFBEngineTestAccess::SetLastRawTorque(engine, 100.0);

        ASSERT_NEAR(run_step(engine, data, 0.5), 0.0, 0.001);
        ASSERT_NEAR(run_step(engine, data, 1.0), 0.0, 0.001);
        // stiffness = 20.0
        // excess_for_max = 5.0 / (20.0 * 100.0) = 0.0025
        // At 0.1% excess (1.001):
        // spring_nm = (0.001 / 0.0025) * 100.0 * 2.0 = 0.4 * 200.0 = 80.0 Nm
        // di_texture = 80.0 / 100.0 = 0.8
        ASSERT_NEAR(run_step(engine, data, 1.001), -0.8, 0.01);

        // At 0.25% excess (1.0025) it should hit the wall
        ASSERT_NEAR(run_step(engine, data, 1.0025), -1.0, 0.01);

        // At 10% excess (1.1) it's definitely -1.0
        ASSERT_NEAR(run_step(engine, data, 1.1), -1.0, 0.01);
        ASSERT_NEAR(run_step(engine, data, -1.1), 1.0, 0.01);
    }

    {
        std::cout << "  Sub-test: Soft Lock Wall Persistence (Zero Velocity)" << std::endl;
        FFBEngine engine;
        InitializeEngine(engine);
        TelemInfoV01 data = CreateBasicTestTelemetry();
        engine.m_soft_lock_enabled = true;
        engine.m_soft_lock_stiffness = 20.0f;
        engine.m_soft_lock_damping = 1.0f; // High damping
        engine.m_wheelbase_max_nm = 15.0f; engine.m_target_rim_nm = 10.0f;
        engine.m_gain = 1.0f;
        engine.m_invert_force = false;
        engine.m_steering_shaft_gain = 0.0f;

        // Force zero velocity
        FFBEngineTestAccess::SetSteeringVelocitySmoothed(engine, 0.0);

        // Even with high damping, if velocity is zero, only spring remains.
        // At 0.5% excess (1.005), spring reaches full wheelbase torque.
        ASSERT_NEAR(run_step(engine, data, 1.005), -1.0, 0.01);
    }

    {
        FFBEngine engine;
        InitializeEngine(engine);
        TelemInfoV01 data = CreateBasicTestTelemetry();
        engine.m_soft_lock_enabled = false;
        engine.m_soft_lock_stiffness = 20.0f;
        engine.m_steering_shaft_gain = 0.0f;
        ASSERT_NEAR(run_step(engine, data, 1.1), 0.0, 0.001);
    }

    {
        FFBEngine engine;
        InitializeEngine(engine);
        TelemInfoV01 data = CreateBasicTestTelemetry();
        engine.m_soft_lock_enabled = true;
        engine.m_soft_lock_stiffness = 0.0f;
        engine.m_soft_lock_damping = 0.1f; // Use larger damping for easier test
        engine.m_wheelbase_max_nm = 100.0f; engine.m_target_rim_nm = 100.0f;
        engine.m_gain = 1.0f;
        engine.m_invert_force = false;
        engine.m_steering_shaft_gain = 0.0f;

        // v0.7.67 Fix for Issue #152: Ensure normalization matches the test scaling
        FFBEngineTestAccess::SetSessionPeakTorque(engine, 100.0);
        FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 100.0);
        FFBEngineTestAccess::SetRollingAverageTorque(engine, 100.0);
        FFBEngineTestAccess::SetLastRawTorque(engine, 100.0);

        run_step(engine, data, 1.1); // Initial: sets prev_angle
        double force = run_step(engine, data, 1.2);

        // stiffness = 1.0 (clamped) -> excess_for_max = 0.05
        // spring_nm = (0.1 / 0.05) * 100 * 2 = 400 Nm (clamped to 200 via min(1, ...)*2*max)

        // steer_vel = (1.2 - 1.1) * (9.4247 / 2) / 0.0025 = 188.494 rad/s
        // damping_nm = 188.494 * 0.1 * 100 * 0.1 = 188.494 Nm

        // total_nm = -(200 + 188.494) = -388.494
        // di_texture = -388.494 / 100 = -3.88 -> -1.0
        ASSERT_NEAR(force, -1.0, 0.01);

        engine.m_soft_lock_damping = 0.0001f;
        // damping_nm = 188.494 * 0.0001 * 100 * 0.1 = 0.188494 Nm
        // total_nm = -(200 + 0.188) = -200.188
        // di_texture = -2.00 -> -1.0
        force = run_step(engine, data, 1.3);
        ASSERT_NEAR(force, -1.0, 0.01);
    }

    std::cout << "  [PASS] Soft Lock logic verified." << std::endl;
}

AutoRegister reg_soft_lock("Soft Lock Logic", "Internal", {"Physics", "Integration"}, test_soft_lock);

} // namespace
