#include "test_ffb_common.h"
#include "../src/ffb/FFBEngine.h"

namespace FFBEngineTests {

TEST_CASE(test_issue_185_fix_repro, "Internal") {
    std::cout << "Test: Issue #185 Fix Verification (Garage FFB - Repro Case)" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    engine.m_gain = 1.0f;
    engine.m_min_force = 0.05f;
    engine.m_steering_shaft_gain = 1.0f;
    engine.m_wheelbase_max_nm = 10.0f;
    engine.m_target_rim_nm = 10.0f;

    FFBEngineTestAccess::SetSessionPeakTorque(engine, 10.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 10.0);

    VehicleScoringInfoV01 scoring;
    memset(&scoring, 0, sizeof(scoring));
    scoring.mIsPlayer = true;
    scoring.mControl = 0;
    scoring.mInGarageStall = true;

    double speed = 0.0;
    TelemInfoV01 data = CreateBasicTestTelemetry(speed, 0.0);
    data.mSteeringShaftTorque = 0.1;

    bool allowed = engine.m_safety.IsFFBAllowed(scoring, 5);
    ASSERT_FALSE(allowed);

    double force = engine.calculate_force(&data, nullptr, nullptr, 0.0f, allowed);
    std::cout << "  Force (should be 0): " << force << std::endl;

    ASSERT_NEAR(force, 0.0, 1e-7);
}

TEST_CASE(test_issue_185_fix_soft_lock, "Internal") {
    std::cout << "Test: Issue #185 Fix Verification (Soft Lock preservation)" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    engine.m_gain = 1.0f;
    engine.m_min_force = 0.05f;
    engine.m_wheelbase_max_nm = 100.0f;
    engine.m_target_rim_nm = 100.0f;

    FFBEngineTestAccess::SetSessionPeakTorque(engine, 100.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 100.0);

    VehicleScoringInfoV01 scoring;
    memset(&scoring, 0, sizeof(scoring));
    scoring.mIsPlayer = true;
    scoring.mControl = 0;
    scoring.mInGarageStall = true;

    double speed = 0.0;
    TelemInfoV01 data = CreateBasicTestTelemetry(speed, 0.0);
    bool allowed = engine.m_safety.IsFFBAllowed(scoring, 5);

    // Trigger Soft Lock: 10% excess -> stiffness 20.0 * 0.1 * 50 = 100 Nm.
    data.mUnfilteredSteering = 1.1;
    engine.m_soft_lock_enabled = true;
    engine.m_soft_lock_stiffness = 20.0f;

    double force = engine.calculate_force(&data, nullptr, nullptr, 0.0f, allowed);
    double abs_force = std::abs(force);
    std::cout << "  Force with soft lock (100Nm): " << force << " (abs: " << abs_force << ")" << std::endl;

    // norm = 100 / 100 = 1.0.
    ASSERT_NEAR(abs_force, 1.0, 0.01);
}

TEST_CASE(test_issue_235_garage_noise, "Internal") {
    std::cout << "Test: Issue #235 Verification (Garage Noise Rejection)" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    engine.m_gain = 1.0f;
    engine.m_min_force = 0.05f;
    engine.m_steering_shaft_gain = 1.0f;
    engine.m_wheelbase_max_nm = 10.0f;
    engine.m_target_rim_nm = 10.0f;
    engine.m_soft_lock_enabled = true;
    engine.m_soft_lock_stiffness = 20.0f;

    FFBEngineTestAccess::SetSessionPeakTorque(engine, 10.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 10.0);

    VehicleScoringInfoV01 scoring;
    memset(&scoring, 0, sizeof(scoring));
    scoring.mIsPlayer = true;
    scoring.mControl = 0;
    scoring.mInGarageStall = true;

    double speed = 0.0;
    TelemInfoV01 data = CreateBasicTestTelemetry(speed, 0.0);

    // Scenario 1: Tiny steering jitter (Issue #235 suspect)
    // Small noise reported by game: 0.001 steering offset (e.g. 0.5 degrees)
    // This should NOT trigger Soft Lock and should stay 0.0.
    data.mUnfilteredSteering = 0.001;

    bool allowed = engine.m_safety.IsFFBAllowed(scoring, 5);
    double force = engine.calculate_force(&data, nullptr, nullptr, 0.0f, allowed);
    std::cout << "  Force with 0.1%% steering noise: " << force << std::endl;
    ASSERT_EQ(force, 0.0);

    // Scenario 2: Significant Soft Lock (Real rack limit)
    // User actually turns wheel to limit.
    data.mUnfilteredSteering = 1.1;
    force = engine.calculate_force(&data, nullptr, nullptr, 0.0f, allowed);
    std::cout << "  Force with 10%% excess steering: " << force << std::endl;
    ASSERT_GT(std::abs(force), 0.5);

    // Scenario 3: Transition Reset
    // Move from allowed to muted. High frequency filter state should be cleared.
    engine.calculate_force(&data, nullptr, nullptr, 0.0f, true); // driving
    engine.calculate_force(&data, nullptr, nullptr, 0.0f, false); // exit to garage

    // Verify velocity is zeroed (filter reset)
    double vel = FFBEngineTestAccess::GetSteeringVelocitySmoothed(engine);
    std::cout << "  Steering velocity after transition reset: " << vel << std::endl;
    ASSERT_EQ(vel, 0.0);
}

} // namespace
