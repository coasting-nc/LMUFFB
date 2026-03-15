#include "test_ffb_common.h"
#include "VehicleUtils.h"

namespace FFBEngineTests {

TEST_CASE(test_issue_355_motion_ratio_normalization, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Test Case 1: Hypercar (MR 0.5)
    FFBEngineTestAccess::CallInitializeLoadReference(engine, "HYPERCAR", "Cadillac 963");
    double mr_hyper = GetMotionRatioForClass(ParsedVehicleClass::HYPERCAR);
    ASSERT_EQ(mr_hyper, 0.50);

    // Test Case 2: GT3 (MR 0.65)
    FFBEngineTestAccess::CallInitializeLoadReference(engine, "GT3", "Ferrari 296");
    double mr_gt3 = GetMotionRatioForClass(ParsedVehicleClass::LMGT3);
    ASSERT_EQ(mr_gt3, 0.65);
}

TEST_CASE(test_issue_355_load_approximation_centralized, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Hypercar front load
    FFBEngineTestAccess::CallInitializeLoadReference(engine, "HYPERCAR", "Cadillac");
    TelemWheelV01 w;
    w.mSuspForce = 10000.0;
    // Expected: (10000 * 0.5) + 400 = 5400.0
    ASSERT_NEAR(engine.approximate_load(w), 5400.0, 0.1);

    // GT3 front load
    FFBEngineTestAccess::CallInitializeLoadReference(engine, "GT3", "Ferrari");
    w.mSuspForce = 10000.0;
    // Expected: (10000 * 0.65) + 500 = 7000.0
    ASSERT_NEAR(engine.approximate_load(w), 7000.0, 0.1);
}

TEST_CASE(test_issue_355_lockup_grounding_robustness, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    FFBEngineTestAccess::SetLockupEnabled(engine, true);
    engine.m_lockup_gain = 1.0f;
    engine.m_lockup_start_pct = 5.0f;
    engine.m_lockup_full_pct = 15.0f;
    engine.m_lockup_freq_scale = 1.0f;
    engine.m_speed_gate_lower = -1.0f;
    engine.m_speed_gate_upper = -0.5f;

    TelemInfoV01 data = CreateBasicTestTelemetry(10.0);
    data.mUnfilteredBrake = 1.0;

    FFBCalculationContext ctx;
    ctx.dt = 0.0025;
    ctx.speed_gate = 1.0;
    ctx.brake_load_factor = 1.0;
    ctx.frame_warn_load = true;

    // Scenario: Encrypted car where mTireLoad is 0 but mSuspForce is active.
    data.mWheel[0].mTireLoad = 0.0;
    data.mWheel[0].mSuspForce = 1000.0;

    data.mWheel[0].mLongitudinalGroundVel = 10.0;
    data.mWheel[0].mLongitudinalPatchVel = -10.0; // 100% lockup

    FFBEngineTestAccess::CallCalculateLockup_Vibration(engine, &data, ctx);
    ASSERT_GT(std::abs(ctx.lockup_rumble), 1e-6);

    // Now make it really airborne (approximated load < 50)
    for (int i=0; i<4; i++) {
        data.mWheel[i].mSuspForce = -1000.0;
        data.mWheel[i].mTireLoad = 0.0;
    }

    // We must pass frame_warn_load through FFBCalculationContext correctly.
    // Call calculate_force to ensure internal state is consistent if needed,
    // but here we call the helper directly.

    // In calculate_lockup_vibration:
    // double current_load = ctx.frame_warn_load ? approximate_load(w) : w.mTireLoad;
    // bool is_grounded = (current_load > PREDICTION_LOAD_THRESHOLD);
    // If !is_grounded, trigger_threshold = full_threshold (0.15).
    // abs(slip) = 1.0. 1.0 > 0.15 => TRIGGERED?

    // WAIT. If it's NOT grounded, trigger_threshold is STILL full_threshold.
    // The predictive trigger is what depends on is_grounded.
    // If it's FULLY locked up, it triggers REGARDLESS of grounding.

    // Physical reasoning: if a wheel is locked in the air, it still vibrates?
    // Probably not, but the current code only uses is_grounded for the PREDICTIVE trigger.

    /*
        if (brake_active && is_grounded && !is_bumpy) {
            // Predictive Trigger
            if (...) trigger_threshold = start_threshold;
        }
        if (slip_abs > trigger_threshold) { ... }
    */

    // To test grounding robustness, we need a slip that is between start and full.
    // start = 0.05, full = 0.15.
    // Let's use slip = 0.10.

    data.mWheel[0].mLongitudinalPatchVel = -1.0; // 10% lockup

    // 1. Grounded: triggers due to predictive decel
    data.mWheel[0].mSuspForce = 1000.0;
    data.mWheel[0].mRotation = 0.0; // Decelerating rapidly
    // Need to seed prev_rotation
    engine.calculate_force(&data);
    data.mWheel[0].mRotation = -100.0; // High deceleration

    FFBEngineTestAccess::CallCalculateLockup_Vibration(engine, &data, ctx);
    ASSERT_GT(std::abs(ctx.lockup_rumble), 1e-6);

    // 2. Airborne: does NOT trigger predictive
    for (int i=0; i<4; i++) data.mWheel[i].mSuspForce = -1000.0;
    ctx.lockup_rumble = 0.0;
    FFBEngineTestAccess::CallCalculateLockup_Vibration(engine, &data, ctx);
    ASSERT_NEAR(ctx.lockup_rumble, 0.0, 1e-9);
}

TEST_CASE(test_issue_355_bottoming_impulse_normalization, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    FFBEngineTestAccess::SetBottomingEnabled(engine, true);
    FFBEngineTestAccess::SetBottomingMethod(engine, 1);
    engine.m_bottoming_gain = 1.0f;
    engine.m_speed_gate_lower = -1.0f;
    engine.m_speed_gate_upper = -0.5f;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    FFBCalculationContext ctx;
    ctx.dt = 0.0025;
    ctx.speed_gate = 1.0;

    // 1. Hypercar (MR 0.5)
    FFBEngineTestAccess::CallInitializeLoadReference(engine, "HYPERCAR", "Cadillac");
    engine.calculate_force(&data);

    data.mWheel[0].mSuspForce += 600.0;
    FFBEngineTestAccess::CallCalculateSuspensionBottoming(engine, &data, ctx);
    ASSERT_GT(std::abs(ctx.bottoming_crunch), 1e-6);

    // 2. GT3 (MR 0.65)
    FFBEngineTestAccess::CallInitializeLoadReference(engine, "GT3", "Ferrari");
    engine.calculate_force(&data);

    data.mWheel[0].mSuspForce += 461.5;
    ctx.bottoming_crunch = 0.0;
    FFBEngineTestAccess::CallCalculateSuspensionBottoming(engine, &data, ctx);
    ASSERT_GT(std::abs(ctx.bottoming_crunch), 1e-6);
}

TEST_CASE(test_issue_355_bottoming_safety_fallback, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    FFBEngineTestAccess::SetBottomingEnabled(engine, true);
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 5000.0);
    engine.m_bottoming_gain = 1.0f;
    engine.m_speed_gate_lower = -1.0f;
    engine.m_speed_gate_upper = -0.5f;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    FFBCalculationContext ctx;
    ctx.dt = 0.0025;
    ctx.speed_gate = 1.0;
    ctx.frame_warn_load = true;
    data.mWheel[0].mTireLoad = 0.0;
    data.mWheel[0].mSuspForce = 20000.0;

    FFBEngineTestAccess::CallCalculateSuspensionBottoming(engine, &data, ctx);
    ASSERT_GT(std::abs(ctx.bottoming_crunch), 1e-6);
}

}
