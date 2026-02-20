#include <iostream>
#include <cmath>
#include <vector>
#include "src/FFBEngine.h"
#include "tests/test_ffb_common.h"

using namespace FFBEngineTests;

TEST_CASE(test_normalization_regression_limpness, "Regression_Issue175") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Setup standard Physical Target Model: 15Nm wheelbase, 10Nm rim torque.
    engine.m_wheelbase_max_nm = 15.0f;
    engine.m_target_rim_nm = 10.0f;
    engine.m_steering_shaft_gain = 1.0f;
    engine.m_gain = 1.0f;
    engine.m_torque_source = 0; // 100Hz Legacy
    engine.m_dynamic_normalization_enabled = true; // ENABLE TO SHOW LIMPNESS

    TelemInfoV01 data = {};
    data.mDeltaTime = 0.0025f;
    data.mLocalVel.z = -20.0f; // Fast enough for full FFB
    data.mLocalAccel.x = 2.0f * 9.81f; // 2G

    // Scenario 1: Initial state (session peak initialized to target_rim_nm = 10.0)
    data.mSteeringShaftTorque = 10.0f;
    FFBEngineTestAccess::SetRollingAverageTorque(engine, 10.0);
    FFBEngineTestAccess::SetLastRawTorque(engine, 10.0);

    // Force initialize session peak to match target_rim_nm for a clean start
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 10.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 10.0);

    // Process many frames to settle the smoothed multiplier
    for(int i=0; i<200; i++) engine.calculate_force(&data);

    double force_initial = engine.calculate_force(&data);

    // Initial force check (doesn't need to match exact theoretical value due to settled EMA and test defaults)
    ASSERT_GT(force_initial, 0.01);

    // Scenario 2: High torque event (e.g. driving an LMP2 with 40Nm peak)
    data.mSteeringShaftTorque = 40.0f;
    FFBEngineTestAccess::SetRollingAverageTorque(engine, 40.0);
    FFBEngineTestAccess::SetLastRawTorque(engine, 40.0);

    for(int i=0; i<200; i++) engine.calculate_force(&data);

    double peak = FFBEngineTestAccess::GetSessionPeakTorque(engine);
    ASSERT_NEAR(peak, 40.0, 0.5);

    // Scenario 3: Back to 10Nm torque (e.g. a lighter corner or straight)
    data.mSteeringShaftTorque = 10.0f;
    FFBEngineTestAccess::SetRollingAverageTorque(engine, 10.0);
    FFBEngineTestAccess::SetLastRawTorque(engine, 10.0);

    for(int i=0; i<200; i++) engine.calculate_force(&data);

    double force_after_peak = engine.calculate_force(&data);

    // EXPECTED REGRESSION: Force should drop significantly (approx by half since peak doubled from 20 to 40)
    // Wait, peak doubled from 20 to 40? No, from 10 to 40. So 1/4 force.
    ASSERT_LT(force_after_peak, force_initial * 0.5);

    std::cout << "[Test] Force Initial: " << force_initial
              << ", Force After Peak: " << force_after_peak
              << " (Reduction: " << (1.0 - force_after_peak/force_initial)*100.0 << "%)" << std::endl;
}

TEST_CASE(test_normalization_toggle_restores_consistency, "Regression_Issue175") {
    FFBEngine engine;
    InitializeEngine(engine);

    engine.m_wheelbase_max_nm = 15.0f;
    engine.m_target_rim_nm = 10.0f;
    engine.m_steering_shaft_gain = 1.0f;
    engine.m_gain = 1.0f;
    engine.m_torque_source = 0; // 100Hz Legacy
    engine.m_dynamic_normalization_enabled = false; // DISABLED FIX

    TelemInfoV01 data = {};
    data.mDeltaTime = 0.0025f;
    data.mLocalVel.z = -20.0f;

    // Process many frames to settle the smoothed multiplier
    data.mSteeringShaftTorque = 10.0f;
    for(int i=0; i<200; i++) engine.calculate_force(&data);

    double force_initial = engine.calculate_force(&data);

    // High torque event
    data.mSteeringShaftTorque = 50.0f;
    for(int i=0; i<100; i++) engine.calculate_force(&data);

    // Back to 10Nm
    data.mSteeringShaftTorque = 10.0f;
    for(int i=0; i<200; i++) engine.calculate_force(&data);

    double force_after_peak = engine.calculate_force(&data);

    // With dynamic normalization disabled, it should remain CONSISTENT!
    std::cout << "[Test] force_initial (Off): " << force_initial << ", force_after_peak (Off): " << force_after_peak << std::endl;
    ASSERT_NEAR(force_after_peak, force_initial, 0.01);

    std::cout << "[Test] Consistent Force (Normalization Off): " << force_after_peak << std::endl;
}
