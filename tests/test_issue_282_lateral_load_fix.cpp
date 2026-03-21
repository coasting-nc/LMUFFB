#include "test_ffb_common.h"
#include <vector>
#include <string>
#include <cmath>

using namespace FFBEngineTests;

TEST_CASE_TAGGED(test_issue_282_transformations, "CorePhysics", (std::vector<std::string>{"Physics", "Issue282"})) {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_rear_axle.sop_effect = 0.0f;
    engine.m_load_forces.lat_load_effect = 1.0f;
    engine.m_rear_axle.sop_scale = 1.0f;
    engine.m_rear_axle.sop_smoothing_factor = 0.0f; // No smoothing for direct verification

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mLocalAccel.x = 0.0;

    auto get_lat_load_force = [&](LoadTransform transform, double fl_load, double fr_load) {
        engine.m_load_forces.lat_load_transform = static_cast<int>(transform);
        data.mWheel[0].mTireLoad = fl_load;
        data.mWheel[1].mTireLoad = fr_load;
        data.mWheel[2].mTireLoad = 4000.0; // Fixed rear
        data.mWheel[3].mTireLoad = 4000.0;
        engine.calculate_force(&data);
        auto snap = engine.GetDebugBatch().back();
        return snap.lat_load_force;
    };

    // total_load = 16000. left = 6000+4000=10000. right = 2000+4000=6000.
    // x = (right - left) / total = (6000 - 10000) / 16000 = -0.25
    double fl = 6000.0;
    double fr = 2000.0;
    double x = 0.25;

    // Linear: f(x) = x = 0.25
    ASSERT_NEAR(get_lat_load_force(LoadTransform::LINEAR, fl, fr), -0.25f, 0.01f);

    // Cubic: f(x) = 1.5x - 0.5x^3 = 1.5(0.25) - 0.5(0.25^3) = 0.375 - 0.5(0.015625) = 0.375 - 0.0078125 = 0.3671875
    ASSERT_NEAR(get_lat_load_force(LoadTransform::CUBIC, fl, fr), -0.3672f, 0.01f);

    // Quadratic: f(x) = 2x - x|x| = 2(0.25) - 0.25(0.25) = 0.5 - 0.0625 = 0.4375
    ASSERT_NEAR(get_lat_load_force(LoadTransform::QUADRATIC, fl, fr), -0.4375f, 0.01f);

    // Hermite: f(x) = x * (1 + |x| - x^2) = 0.25 * (1 + 0.25 - 0.0625) = 0.25 * (1.1875) = 0.296875
    ASSERT_NEAR(get_lat_load_force(LoadTransform::HERMITE, fl, fr), -0.2969f, 0.01f);

    // Verify symmetry (Negative x)
    // total = 16000. left = 2000+4000=6000. right = 6000+4000=10000.
    // x = (10000 - 6000) / 16000 = 0.25
    fl = 2000.0;
    fr = 6000.0;
    ASSERT_NEAR(get_lat_load_force(LoadTransform::LINEAR, fl, fr), 0.25f, 0.01f);
    ASSERT_NEAR(get_lat_load_force(LoadTransform::CUBIC, fl, fr), 0.3672f, 0.01f);
    ASSERT_NEAR(get_lat_load_force(LoadTransform::QUADRATIC, fl, fr), 0.4375f, 0.01f);
    ASSERT_NEAR(get_lat_load_force(LoadTransform::HERMITE, fl, fr), 0.2969f, 0.01f);

    // Verify limit (x = 1.0)
    // left = 8000+8000=16000. right=0. total=16000. x=1.0.
    auto get_lat_load_force_extreme = [&](LoadTransform transform) {
        engine.m_load_forces.lat_load_transform = static_cast<int>(transform);
        data.mWheel[0].mTireLoad = 8000.0;
        data.mWheel[1].mTireLoad = 0.0;
        data.mWheel[2].mTireLoad = 8000.0;
        data.mWheel[3].mTireLoad = 0.0;
        engine.calculate_force(&data);
        return engine.GetDebugBatch().back().lat_load_force;
    };
    ASSERT_NEAR(get_lat_load_force_extreme(LoadTransform::LINEAR), -1.0f, 0.01f);
    ASSERT_NEAR(get_lat_load_force_extreme(LoadTransform::CUBIC), -1.0f, 0.01f);
    ASSERT_NEAR(get_lat_load_force_extreme(LoadTransform::QUADRATIC), -1.0f, 0.01f);
    ASSERT_NEAR(get_lat_load_force_extreme(LoadTransform::HERMITE), -1.0f, 0.01f);
}

TEST_CASE_TAGGED(test_issue_282_sign_inversion, "CorePhysics", (std::vector<std::string>{"Physics", "Issue282"})) {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_rear_axle.sop_effect = 1.0f;
    engine.m_load_forces.lat_load_effect = 1.0f;
    engine.m_rear_axle.sop_scale = 1.0f;
    engine.m_rear_axle.sop_smoothing_factor = 0.0f;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);

    // Right Turn: Centrifugal force LEFT (+X), Load shift LEFT (FL > FR)
    data.mLocalAccel.x = 9.81;
    data.mWheel[0].mTireLoad = 6000.0; // FL
    data.mWheel[1].mTireLoad = 2000.0; // FR
    data.mWheel[2].mTireLoad = 4000.0; // RL
    data.mWheel[3].mTireLoad = 4000.0; // RR

    // Issue #397: Flush the 10ms transient ramp
    PumpEngineTime(engine, data, 0.015);
    auto snap = engine.GetDebugBatch().back();

    // Lat G accel = 1.0 -> sop_force (G-based) = 1.0
    // Lat Load norm = (6000 - 10000) / 16000 = -0.25 -> lat_load_force = -0.25
    // They should have OPPOSITE signs now (Issue #306: fixed inverted feel).
    std::cout << "[INFO] SoP Force (G): " << snap.sop_force << " | Lat Load Force: " << snap.lat_load_force << std::endl;
    ASSERT_GT(snap.sop_force, 0.0f);
    ASSERT_LT(snap.lat_load_force, 0.0f);
}

TEST_CASE_TAGGED(test_issue_282_decoupling, "CorePhysics", (std::vector<std::string>{"Physics", "Issue282"})) {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_rear_axle.sop_effect = 1.0f;
    engine.m_load_forces.lat_load_effect = 1.0f;
    engine.m_rear_axle.sop_scale = 1.0f;
    engine.m_rear_axle.sop_smoothing_factor = 0.0f;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mLocalAccel.x = 9.81;
    data.mWheel[0].mTireLoad = 6000.0;
    data.mWheel[1].mTireLoad = 2000.0;

    // Case 1: No boost
    engine.m_rear_axle.oversteer_boost = 0.0f;
    engine.calculate_force(&data);
    float lat_load_1 = engine.GetDebugBatch().back().lat_load_force;

    // Case 2: High boost
    engine.m_rear_axle.oversteer_boost = 2.0f;
    // We need to ensure we have front grip < rear grip or vice versa if boost logic depends on it,
    // but the point is lat_load_force should NOT be affected by the boost multiplier regardless.
    // In calculate_sop_lateral, sop_base is boosted, lat_load_force is NOT.
    engine.calculate_force(&data);
    float lat_load_2 = engine.GetDebugBatch().back().lat_load_force;

    ASSERT_NEAR(lat_load_1, lat_load_2, 0.001f);
}
