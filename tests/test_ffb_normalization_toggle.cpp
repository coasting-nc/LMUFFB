#include "test_ffb_common.h"
#include <cmath>

namespace FFBEngineTests {

TEST_CASE(test_manual_scaling_100hz, "Scaling") {
    std::cout << "\nTest: Manual Scaling 100Hz (v0.7.109)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    engine.m_torque_source = 0; // Shaft Torque
    engine.m_dynamic_normalization_enabled = false;
    engine.m_car_max_torque_nm = 20.0f;
    engine.m_wheelbase_max_nm = 20.0f;
    engine.m_target_rim_nm = 20.0f;
    engine.m_gain = 1.0f;

    // Reset EMA for multiplier
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 20.0);

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mSteeringShaftTorque = 10.0f; // 50% of 20Nm

    double force = engine.calculate_force(&data);

    // Expected: (10.0 * 1/20) * (20/20) = 0.5
    ASSERT_NEAR(force, 0.5, 0.001);
}

TEST_CASE(test_manual_scaling_400hz, "Scaling") {
    std::cout << "\nTest: Manual Scaling 400Hz (v0.7.109)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    engine.m_torque_source = 1; // In-Game FFB (400Hz)
    engine.m_dynamic_normalization_enabled = false;
    engine.m_car_max_torque_nm = 30.0f;
    engine.m_wheelbase_max_nm = 15.0f;
    engine.m_target_rim_nm = 15.0f;
    engine.m_gain = 1.0f;

    // Reset EMA for multiplier
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 30.0);

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    float genFFBTorque = 0.5f; // 50%

    double force = engine.calculate_force(&data, nullptr, nullptr, genFFBTorque);

    // internal raw_torque = 0.5 * 30.0 = 15.0 Nm
    // structural_sum = 15.0
    // norm_structural = 15.0 * (1/30) = 0.5
    // di_structural = 0.5 * (15/15) = 0.5
    ASSERT_NEAR(force, 0.5, 0.001);
}

TEST_CASE(test_normalization_toggle_behavior, "Scaling") {
    std::cout << "\nTest: Normalization Toggle Behavior (v0.7.109)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    engine.m_torque_source = 0;
    engine.m_car_max_torque_nm = 50.0f;
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 25.0);

    // 1. Toggle OFF: Should use car_max_torque_nm (50)
    engine.m_dynamic_normalization_enabled = false;
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mSteeringShaftTorque = 10.0f;

    // Run multiple times to let EMA settle
    for(int i=0; i<100; i++) engine.calculate_force(&data);
    double force_off = engine.calculate_force(&data);
    ASSERT_NEAR(force_off, 10.0 / 50.0, 0.01);

    // 2. Toggle ON: Should use session_peak_torque (25)
    engine.m_dynamic_normalization_enabled = true;
    for(int i=0; i<100; i++) engine.calculate_force(&data);
    double force_on = engine.calculate_force(&data);
    ASSERT_NEAR(force_on, 10.0 / 25.0, 0.01);
}

TEST_CASE(test_config_persistence_normalization, "Persistence") {
    std::cout << "\nTest: Config Persistence Normalization (v0.7.109)" << std::endl;
    std::string test_file = "test_norm_config.ini";

    {
        FFBEngine engine;
        engine.m_dynamic_normalization_enabled = true;
        engine.m_car_max_torque_nm = 42.0f;
        Config::Save(engine, test_file);
    }

    {
        FFBEngine engine;
        Config::Load(engine, test_file);
        ASSERT_TRUE(engine.m_dynamic_normalization_enabled);
        ASSERT_NEAR(engine.m_car_max_torque_nm, 42.0f, 0.001);
    }

    std::remove(test_file.c_str());
}

} // namespace FFBEngineTests
