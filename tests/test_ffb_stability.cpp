#include "test_ffb_common.h"
#include <cmath>

namespace FFBEngineTests {

TEST_CASE(test_negative_parameter_safety, "Stability") {
    std::cout << "\nTest: Negative Parameter Safety (v0.7.16)" << std::endl;
    FFBEngine engine;
    Preset p("KillerPreset", false);

    // Set some dangerous negative or zero values
    p.lockup_gamma = -1.0f;
    p.notch_q = -5.0f;
    p.wheelbase_max_nm = -100.0f;
    p.optimal_slip_angle = -0.1f;
    p.optimal_slip_ratio = 0.0f;
    p.slope_alpha_threshold = -0.01f;
    p.slope_decay_rate = -5.0f;
    p.slope_smoothing_tau = -0.04f;
    p.gain = -1.0f;

    // Apply preset - should clamp everything
    p.Apply(engine);

    // Verify clamping in engine
    ASSERT_GE(engine.m_lockup_gamma, 0.1f);
    ASSERT_GE(engine.m_notch_q, 0.1f);
    ASSERT_GE(engine.m_wheelbase_max_nm, 1.0f);
    ASSERT_GE(engine.m_optimal_slip_angle, 0.01f);
    ASSERT_GE(engine.m_optimal_slip_ratio, 0.01f);
    ASSERT_GE(engine.m_slope_alpha_threshold, 0.001f);
    ASSERT_GE(engine.m_slope_decay_rate, 0.1f);
    ASSERT_GE(engine.m_slope_smoothing_tau, 0.001f);
    ASSERT_GE(engine.m_gain, 0.0f);

    // Run a frame to ensure no crash or NaN
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.05);
    double force = engine.calculate_force(&data);

    ASSERT_TRUE(!std::isnan(force));
    ASSERT_TRUE(!std::isinf(force));
}

TEST_CASE(test_config_load_validation, "Stability") {
    std::cout << "\nTest: Config Load Validation (v0.7.16)" << std::endl;
    std::string test_file = "test_bad_config.ini";
    {
        std::ofstream file(test_file);
        file << "gain=-1.5\n";
        file << "max_torque_ref=0.0\n";
        file << "lockup_gamma=-2.0\n";
        file << "optimal_slip_angle=0.0\n";
        file << "slope_sg_window=4\n"; // Even and too small
    }

    FFBEngine engine;
    Config::Load(engine, test_file);

    ASSERT_GE(engine.m_gain, 0.0f);
    ASSERT_GE(engine.m_wheelbase_max_nm, 1.0f);
    ASSERT_GE(engine.m_lockup_gamma, 0.1f);
    ASSERT_GE(engine.m_optimal_slip_angle, 0.01f);
    ASSERT_TRUE(engine.m_slope_sg_window >= 5);
    ASSERT_TRUE(engine.m_slope_sg_window % 2 != 0);

    std::remove(test_file.c_str());
}

TEST_CASE(test_engine_robustness_to_static_telemetry, "Stability") {
    std::cout << "\nTest: Engine Robustness to Static Telemetry (v0.7.16)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    // Disable any remaining oscillators that might cause non-constant force
    engine.m_bottoming_enabled = false;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.05);
    data.mDeltaTime = 0.0025;

    double first_force = engine.calculate_force(&data);

    // Run many frames with IDENTICAL telemetry (simulating game freeze)
    for (int i = 0; i < 100; i++) {
        double force = engine.calculate_force(&data);
        ASSERT_TRUE(!std::isnan(force));
        // Force should be constant since all oscillators and smoothing are disabled
        ASSERT_NEAR(force, first_force, 0.0001);
    }
}

} // namespace FFBEngineTests
