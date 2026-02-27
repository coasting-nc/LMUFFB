#include "test_ffb_common.h"
#include "../src/FFBEngine.h"

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

    bool allowed = engine.IsFFBAllowed(scoring, 5);
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
    bool allowed = engine.IsFFBAllowed(scoring, 5);

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

} // namespace
