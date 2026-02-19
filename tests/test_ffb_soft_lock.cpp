#include "test_ffb_common.h"
#include "../src/FFBEngine.h"

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
        // BASE_NM_SOFT_LOCK = 50.0
        // Issue #153: decoupling_scale removed.
        // force_nm = -(0.1 * 20.0 * 50.0) = -100.0 Nm
        // norm_force = -100.0 / 100.0 = -1.0
        ASSERT_NEAR(run_step(engine, data, 1.1), -1.0, 0.01);
        ASSERT_NEAR(run_step(engine, data, -1.1), 1.0, 0.01);
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

        // steer_vel = (1.2 - 1.1) * (9.4247 / 2) / 0.0025 = 188.494 rad/s
        // damping_nm = -(188.494 * 0.1 * 50.0) = -942.47 Nm
        // norm_force = -942.47 / 100 = -9.42 -> clamped to -1.0
        ASSERT_NEAR(force, -1.0, 0.01);

        engine.m_soft_lock_damping = 0.001f;
        // damping_nm = -(188.494 * 0.001 * 50.0) = -9.4247 Nm
        // norm_force = -9.4247 / 100 = -0.0942
        force = run_step(engine, data, 1.3);
        ASSERT_NEAR(force, -0.0942, 0.01);
    }

    std::cout << "  [PASS] Soft Lock logic verified." << std::endl;
}

AutoRegister reg_soft_lock("Soft Lock Logic", "Internal", {"Physics", "Integration"}, test_soft_lock);

} // namespace
