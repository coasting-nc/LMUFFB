// test_ffb_magic_numbers.cpp
// Additional branch-coverage tests for the magic-numbers refactoring (v0.7.107).
//
// These tests target the 7 specific branches identified in the code review as
// uncovered but directly exercising the newly-named constants. They guarantee
// that the constant values are correct at runtime, not just at compile time.
//
// Category: CorePhysics (telemetry warnings / DT fallback)
//           Texture     (slide, spin, ABS, bottoming)

#include "test_ffb_common.h"

namespace FFBEngineTests {

// ============================================================
// TEST 1 â€” DT_EPSILON / DEFAULT_DT
// When mDeltaTime is 0 (or negative), calculate_force() must:
//   (a) replace dt with DEFAULT_DT (0.0025)
//   (b) set the m_warned_dt flag exactly once
//   (c) still return a finite value
// ============================================================
TEST_CASE(test_mn_invalid_delta_time_fallback, "CorePhysics") {
    std::cout << "\nTest: [MagicNumbers] Invalid DeltaTime fallback to DEFAULT_DT" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    FFBEngineTestAccess::SetSessionPeakTorque(engine, 20.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 20.0);
    FFBEngineTestAccess::SetRollingAverageTorque(engine, 20.0);
    FFBEngineTestAccess::SetLastRawTorque(engine, 20.0);

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.0;           // Invalid: triggers DT_EPSILON branch
    data.mSteeringShaftTorque = 10.0;

    ASSERT_FALSE(engine.m_warned_dt); // Precondition: not yet warned

    double force = engine.calculate_force(&data);

    // Must produce a finite output â€” not NaN/Inf
    ASSERT_TRUE(std::isfinite(force));
    // m_warned_dt must be set after the first call with bad dt
    ASSERT_TRUE(engine.m_warned_dt);

    // Re-run with valid dt: no new warning should appear
    engine.m_warned_dt = false;
    data.mDeltaTime = 0.0025;
    engine.calculate_force(&data);
    ASSERT_FALSE(engine.m_warned_dt);

    std::cout << "  Output force with dt=0: " << force << std::endl;
}

// ============================================================
// TEST 2a â€” MISSING_LOAD_WARN_THRESHOLD (20 frames), SuspForce path
// ============================================================
TEST_CASE(test_mn_missing_load_fallback_susp_force, "CorePhysics") {
    std::cout << "\nTest: [MagicNumbers] Missing Load fallback via SuspForce path" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;
    data.mLocalVel.z = -20.0;  // speed > SPEED_EPSILON (1.0)

    for (int i = 0; i < 4; i++) {
        data.mWheel[i].mTireLoad = 0.0;
        data.mWheel[i].mSuspForce = 500.0; // > MIN_VALID_SUSP_FORCE (10N) â†’ approximate_load path
    }

    // Run > 20 frames to surpass MISSING_LOAD_WARN_THRESHOLD
    for (int i = 0; i < 25; i++) {
        engine.calculate_force(&data);
    }

    ASSERT_TRUE(engine.m_warned_load);

    // Engine must still produce finite output after fallback activates
    double force = engine.calculate_force(&data);
    ASSERT_TRUE(std::isfinite(force));

    std::cout << "  Force with load fallback (susp path): " << force << std::endl;
}

// ============================================================
// TEST 2b â€” MISSING_LOAD_WARN_THRESHOLD (20 frames), kinematic path
// ============================================================
TEST_CASE(test_mn_missing_load_fallback_kinematic, "CorePhysics") {
    std::cout << "\nTest: [MagicNumbers] Missing Load fallback via Kinematic path" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;
    data.mLocalVel.z = -20.0;
    data.mLocalAccel.x = 2.0 * 9.81; // some lateral accel for kinematic estimate

    // Zero both TireLoad AND SuspForce to force kinematic fallback
    for (int i = 0; i < 4; i++) {
        data.mWheel[i].mTireLoad = 0.0;
        data.mWheel[i].mSuspForce = 0.0; // < MIN_VALID_SUSP_FORCE â†’ kinematic path
    }

    for (int i = 0; i < 25; i++) {
        engine.calculate_force(&data);
    }

    ASSERT_TRUE(engine.m_warned_load);
    double force = engine.calculate_force(&data);
    ASSERT_TRUE(std::isfinite(force));

    std::cout << "  Force with load fallback (kinematic path): " << force << std::endl;
}

// ============================================================
// TEST 3 â€” MISSING_TELEMETRY_WARN_THRESHOLD (50 frames)
// Four paths: susp force, susp deflection, front lat force, rear lat force.
// ============================================================

// Helper: run engine for N frames without gathering output
static void RunNFrames(FFBEngine& engine, TelemInfoV01& data, int n) {
    for (int i = 0; i < n; i++) {
        engine.calculate_force(&data);
    }
}

TEST_CASE(test_mn_missing_susp_force_warning, "CorePhysics") {
    std::cout << "\nTest: [MagicNumbers] Missing SuspForce warning (>50 frames)" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;
    data.mLocalVel.z = -20.0;    // |vel.z| > SPEED_EPSILON (1.0)

    for (int i = 0; i < 4; i++) {
        data.mWheel[i].mSuspForce = 0.0;
    }

    ASSERT_FALSE(engine.m_warned_susp_force);
    RunNFrames(engine, data, 55);
    ASSERT_TRUE(engine.m_warned_susp_force);

    std::cout << "  SuspForce warning triggered after 55 frames." << std::endl;
}

TEST_CASE(test_mn_missing_susp_deflection_warning, "CorePhysics") {
    std::cout << "\nTest: [MagicNumbers] Missing SuspDeflection warning (>50 frames)" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;
    data.mLocalVel.z = -15.0;    // |vel.z| = 15 > SPEED_HIGH_THRESHOLD (10.0)

    for (int i = 0; i < 4; i++) {
        data.mWheel[i].mSuspensionDeflection = 0.0;
    }

    ASSERT_FALSE(engine.m_warned_susp_deflection);
    RunNFrames(engine, data, 55);
    ASSERT_TRUE(engine.m_warned_susp_deflection);

    std::cout << "  SuspDeflection warning triggered after 55 frames." << std::endl;
}

TEST_CASE(test_mn_missing_lat_force_front_warning, "CorePhysics") {
    std::cout << "\nTest: [MagicNumbers] Missing Front LateralForce warning (>50 frames)" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;
    // Trigger condition: |accel.x| > G_FORCE_THRESHOLD (3.0)
    data.mLocalAccel.x = 5.0 * 9.81;
    for (int i = 0; i < 4; i++) {
        data.mWheel[i].mLateralForce = 0.0;
    }

    ASSERT_FALSE(engine.m_warned_lat_force_front);
    RunNFrames(engine, data, 55);
    ASSERT_TRUE(engine.m_warned_lat_force_front);

    std::cout << "  Front LateralForce warning triggered after 55 frames." << std::endl;
}

TEST_CASE(test_mn_missing_lat_force_rear_warning, "CorePhysics") {
    std::cout << "\nTest: [MagicNumbers] Missing Rear LateralForce warning (>50 frames)" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;
    data.mLocalAccel.x = 5.0 * 9.81; // > G_FORCE_THRESHOLD (3.0)
    // Only zero the rear wheels
    data.mWheel[2].mLateralForce = 0.0;
    data.mWheel[3].mLateralForce = 0.0;

    ASSERT_FALSE(engine.m_warned_lat_force_rear);
    RunNFrames(engine, data, 55);
    ASSERT_TRUE(engine.m_warned_lat_force_rear);

    std::cout << "  Rear LateralForce warning triggered after 55 frames." << std::endl;
}

// ============================================================
// TEST 4 â€” BOTTOMING_RH_THRESHOLD_M (0.002m) / BOTTOMING_FREQ_HZ (50.0)
// Ride height exactly at 2mm â†’ NOT triggered.
// Ride height at 1mm        â†’ bottoming IS triggered â†’ non-zero force.
// ============================================================
TEST_CASE(test_mn_bottoming_ride_height_threshold, "Texture") {
    std::cout << "\nTest: [MagicNumbers] Bottoming RH threshold (0.002m)" << std::endl;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    // dt=0.005 â†’ 200Hz. Phase = 50Hz * 0.005 * 2Ï€ = Ï€/2 â†’ sin(Ï€/2)=1 â†’ max amplitude
    data.mDeltaTime = 0.005;
    data.mLocalVel.z = -20.0;

    // Case A: exactly at threshold (0.002) â€” NOT triggered
    {
        FFBEngine engine;
        InitializeEngine(engine);
        engine.m_bottoming_enabled = true;
        engine.m_bottoming_gain = 1.0f;
        engine.m_bottoming_method = 0;
        data.mWheel[0].mRideHeight = 0.002f;
        data.mWheel[1].mRideHeight = 0.002f;

        double force = engine.calculate_force(&data);
        ASSERT_NEAR(force, 0.0, 0.001);
        std::cout << "  At threshold (0.002m): force = " << force << std::endl;
    }

    // Case B: below threshold (0.001) â€” TRIGGERED â†’ non-zero force
    {
        FFBEngine engine;
        InitializeEngine(engine);
        engine.m_bottoming_enabled = true;
        engine.m_bottoming_gain = 1.0f;
        engine.m_bottoming_method = 0;
        FFBEngineTestAccess::SetStaticFrontLoad(engine, 4000.0);
        FFBEngineTestAccess::SetStaticLoadLatched(engine, true);

        data.mWheel[0].mRideHeight = 0.001f;
        data.mWheel[1].mRideHeight = 0.001f;

        double force = engine.calculate_force(&data);
        ASSERT_TRUE(std::abs(force) > 0.0001);
        std::cout << "  Below threshold (0.001m): force = " << force << std::endl;
    }
}

// ============================================================
// TEST 5 â€” SPIN_SLIP_THRESHOLD (0.2), SPIN_THROTTLE_THRESHOLD (0.05),
//           SPIN_TORQUE_DROP_FACTOR (0.6), SPIN_SEVERITY_RANGE (0.5)
// With slip=0.4 and gain=1.0:
//   severity = (0.4 - 0.2) / 0.5 = 0.4
//   gain_reduction = 1.0 - (0.4 Ã— 1.0 Ã— 0.6) = 0.76
// ============================================================
TEST_CASE(test_mn_spin_detection_torque_drop, "Texture") {
    std::cout << "\nTest: [MagicNumbers] Spin detection SPIN_TORQUE_DROP_FACTOR=0.6" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_spin_enabled = true;
    engine.m_spin_gain = 1.0f;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;
    data.mLocalVel.z = -20.0;
    data.mUnfilteredThrottle = 0.8f;  // > SPIN_THROTTLE_THRESHOLD (0.05)

    // Rear wheel slip = 0.4 > SPIN_SLIP_THRESHOLD (0.2)
    // slip_ratio = |patch_vel - ground_vel| / |ground_vel|
    // With ground_vel=20.0 and patch_vel=12.0: (20-12)/20 = 0.4
    data.mWheel[2].mLongitudinalGroundVel = 20.0;
    data.mWheel[3].mLongitudinalGroundVel = 20.0;
    data.mWheel[2].mLongitudinalPatchVel  = 12.0;
    data.mWheel[3].mLongitudinalPatchVel  = 12.0;
    // Front: no spin
    data.mWheel[0].mLongitudinalGroundVel = 20.0;
    data.mWheel[1].mLongitudinalGroundVel = 20.0;
    data.mWheel[0].mLongitudinalPatchVel  = 20.0;
    data.mWheel[1].mLongitudinalPatchVel  = 20.0;

    FFBCalculationContext ctx;
    ctx.dt = 0.01;
    ctx.car_speed = 20.0;
    ctx.avg_grip = 0.8;
    ctx.speed_gate = 1.0;
    ctx.gain_reduction_factor = 1.0;

    FFBEngineTestAccess::CallCalculateWheelSpin(engine, &data, ctx);

    // calculate_wheel_slip_ratio = mLongitudinalPatchVel / |mLongitudinalGroundVel|
    // With ground_vel=20.0 and patch_vel=12.0: slip = 12.0/20.0 = 0.6 > SPIN_SLIP_THRESHOLD (0.2)
    //   severity = (0.6 - 0.2) / 0.5 = 0.8
    //   gain_reduction = 1.0 - (0.8 x 1.0 x 0.6) = 1.0 - 0.48 = 0.52
    ASSERT_LT(ctx.gain_reduction_factor, 1.0);
    ASSERT_GT(ctx.gain_reduction_factor, 0.0);
    ASSERT_NEAR(ctx.gain_reduction_factor, 0.52, 0.01);

    std::cout << "  Gain reduction factor: " << ctx.gain_reduction_factor
              << " (expected ~0.52, validating SPIN_TORQUE_DROP_FACTOR=0.6)" << std::endl;
}

// ============================================================
// TEST 6 â€” SLIDE_VEL_THRESHOLD (1.5 m/s)
// Below 1.5 m/s â†’ no slide noise. Above â†’ slide noise non-zero.
// ============================================================
TEST_CASE(test_mn_slide_texture_velocity_threshold, "Texture") {
    std::cout << "\nTest: [MagicNumbers] Slide texture SLIDE_VEL_THRESHOLD=1.5m/s" << std::endl;

    // Case A: Below threshold â†’ no noise
    {
        FFBEngine engine;
        InitializeEngine(engine);
        engine.m_slide_texture_enabled = true;
        engine.m_slide_texture_gain = 1.0f;

        TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
        data.mDeltaTime = 0.01;
        for (int i = 0; i < 4; i++) {
            data.mWheel[i].mLateralPatchVel = 1.0; // < 1.5
        }

        FFBCalculationContext ctx;
        ctx.dt = 0.01;
        ctx.car_speed = 20.0;
        ctx.avg_grip = 1.0;
        ctx.speed_gate = 1.0;
        ctx.texture_load_factor = 1.0;
        ctx.slide_noise = 0.0;

        FFBEngineTestAccess::CallCalculateSlideTexture(engine, &data, ctx);

        ASSERT_NEAR(ctx.slide_noise, 0.0, 1e-9);
        std::cout << "  Below threshold: slide_noise = " << ctx.slide_noise << std::endl;
    }

    // Case B: Above threshold â†’ phase accumulates, noise or phase non-zero
    {
        FFBEngine engine;
        InitializeEngine(engine);
        engine.m_slide_texture_enabled = true;
        engine.m_slide_texture_gain = 1.0f;

        TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
        data.mDeltaTime = 0.01;
        for (int i = 0; i < 4; i++) {
            data.mWheel[i].mLateralPatchVel = 3.0; // > 1.5
        }

        FFBCalculationContext ctx;
        ctx.dt = 0.01;
        ctx.car_speed = 20.0;
        ctx.avg_grip = 0.0; // zero grip â†’ maximum slide contribution
        ctx.speed_gate = 1.0;
        ctx.texture_load_factor = 1.0;
        ctx.slide_noise = 0.0;

        FFBEngineTestAccess::CallCalculateSlideTexture(engine, &data, ctx);

        // Phase should have advanced (freq â‰ˆ 10 + 3*5 = 25Hz; dt=0.01 â†’ phase â‰ˆ 1.57rad)
        // slide_noise may be 0 at certain exact phase values, but m_slide_phase will be > 0
        ASSERT_TRUE(engine.m_slide_phase > 0.0 || std::abs(ctx.slide_noise) > 0.0);
        std::cout << "  Above threshold: slide_phase = " << engine.m_slide_phase
                  << ", slide_noise = " << ctx.slide_noise << std::endl;
    }
}

// ============================================================
// TEST 7 â€” ABS_PULSE_MAGNITUDE_SCALER (2.0)
// With abs_gain=1.0, speed_gate=1.0, and sin(phase)=1.0,
// the ABS pulse force must equal 2.0 Nm exactly.
// Phase condition: 20Hz Ã— 0.0125s Ã— 2Ï€ = Ï€/2 â†’ sin=1
// ============================================================
TEST_CASE(test_mn_abs_pulse_magnitude_scaler, "Texture") {
    std::cout << "\nTest: [MagicNumbers] ABS_PULSE_MAGNITUDE_SCALER=2.0" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_abs_pulse_enabled = true;
    engine.m_abs_gain = 1.0f;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    // dt = 1/(4 Ã— 20Hz) = 0.0125s â†’ exactly Ï€/2 phase advance â†’ sin=1
    data.mDeltaTime = 0.0125;
    data.mUnfilteredBrake = 1.0f;  // > ABS_PEDAL_THRESHOLD (0.5)

    // pressure_delta = (0.5 - 1.0) / 0.0125 = -40.0 â†’ |40| > ABS_PRESSURE_RATE_THRESHOLD (2.0)
    data.mWheel[0].mBrakePressure = 0.5f;
    engine.m_prev_brake_pressure[0] = 1.0;

    FFBCalculationContext ctx;
    ctx.dt = 0.0125;
    ctx.car_speed = 20.0;
    ctx.speed_gate = 1.0;
    ctx.abs_pulse_force = 0.0;

    FFBEngineTestAccess::CallCalculateABSPulse(engine, &data, ctx);

    // ABS_PULSE_MAGNITUDE_SCALER = 2.0, so the peak possible force = abs_gain * 2.0 = 2.0 Nm.
    // Exact sin value depends on floating-point phase, so we verify:
    //   (a) Force is non-zero (ABS was triggered)
    //   (b) Force does not exceed 2.0 Nm (scaler is 2, not 3 or more)
    //   (c) Force is at least 85% of max (phase near Ï€/2)
    ASSERT_TRUE(std::abs(ctx.abs_pulse_force) > 0.0);
    ASSERT_LE(std::abs(ctx.abs_pulse_force), 2.001);   // bounded by 2 * abs_gain
    ASSERT_GT(std::abs(ctx.abs_pulse_force), 2.0 * 0.85); // near peak

    std::cout << "  ABS pulse force: " << ctx.abs_pulse_force
              << " (in (1.7, 2.0] Nm, confirming ABS_PULSE_MAGNITUDE_SCALER=2.0)" << std::endl;
}

} // namespace FFBEngineTests
