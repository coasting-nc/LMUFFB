#include "test_ffb_common.h"

namespace FFBEngineTests {

/**
 * @brief Targeted coverage tests for Gyro Damping and ABS Pulse.
 * These methods were identified as having gaps in coverage.
 * This test file ensures all branches and lines are executed.
 */
TEST_CASE(test_gyro_damping_target_coverage, "Coverage") {
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    FFBCalculationContext ctx;
    ctx.dt = 0.0025;
    ctx.car_speed = 50.0;
    
    engine.m_gyro_gain = 1.0f;
    engine.m_gyro_smoothing = 0.1f;
    
    // Path 1: Default range (range <= 0.0f)
    data.mPhysicalSteeringWheelRange = 0.0f;
    data.mUnfilteredSteering = 0.5f; 
    FFBEngineTestAccess::CallCalculateGyroDamping(engine, &data, ctx);
    ASSERT_TRUE(std::abs(ctx.gyro_force) > 0.001);
    
    // Path 2: Custom range (range > 0.0f)
    data.mPhysicalSteeringWheelRange = 9.4247f; // 540 deg
    FFBEngineTestAccess::CallCalculateGyroDamping(engine, &data, ctx);
    ASSERT_TRUE(std::abs(ctx.gyro_force) > 0.001);
    
    // Path 3: Minimal smoothing (tau < 0.0001)
    engine.m_gyro_smoothing = 0.0f;
    FFBEngineTestAccess::CallCalculateGyroDamping(engine, &data, ctx);
    ASSERT_TRUE(std::abs(ctx.gyro_force) > 0.001);
}

TEST_CASE(test_abs_pulse_target_coverage, "Coverage") {
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    FFBCalculationContext ctx;
    ctx.dt = 0.01;
    ctx.speed_gate = 1.0;
    
    // Path 1: Disabled (early return)
    engine.m_abs_pulse_enabled = false;
    FFBEngineTestAccess::CallCalculateABSPulse(engine, &data, ctx);
    ASSERT_EQ(ctx.abs_pulse_force, 0.0);
    
    // Path 2: Enabled but inactive (no pulse - pedal below threshold)
    engine.m_abs_pulse_enabled = true;
    data.mUnfilteredBrake = 0.1f; // Below 0.5 threshold
    FFBEngineTestAccess::CallCalculateABSPulse(engine, &data, ctx);
    ASSERT_EQ(ctx.abs_pulse_force, 0.0);
    
    // Path 3: Enabled but inactive (no pulse - pressure rate below threshold)
    data.mUnfilteredBrake = 1.0f; 
    for(int i=0; i<4; i++) data.mWheel[i].mBrakePressure = 0.0f; // No change from prev 0
    FFBEngineTestAccess::CallCalculateABSPulse(engine, &data, ctx);
    ASSERT_EQ(ctx.abs_pulse_force, 0.0);
    
    // Path 4: Enabled and active
    data.mUnfilteredBrake = 1.0f; 
    for(int i=0; i<4; i++) {
        data.mWheel[i].mBrakePressure = 1.0f; // Create delta of 1.0/0.01 = 100.0 > 2.0
    }
    FFBEngineTestAccess::CallCalculateABSPulse(engine, &data, ctx);
    ASSERT_TRUE(std::abs(ctx.abs_pulse_force) > 0.001);
    
    // Path 5: Phase wrapping (fmod)
    for(int i=0; i<1000; i++) {
        FFBEngineTestAccess::CallCalculateABSPulse(engine, &data, ctx);
    }
    ASSERT_TRUE(std::isfinite(ctx.abs_pulse_force));
}

/**
 * @brief Integrated tests via calculate_force to ensure state updates
 * like m_prev_steering_angle and m_prev_brake_pressure are hit
 * from the main entry point, which provides the final coverage.
 */
TEST_CASE(test_ffb_engine_full_integration_target, "Coverage") {
    FFBEngine engine;
    InitializeEngine(engine);
    
    // Force some gains so snapshots have non-zero results
    engine.m_gyro_gain = 1.0f;
    engine.m_abs_gain = 1.0f;
    engine.m_gain = 1.0f;
    
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.0025f;
    data.mLocalVel.z = 20.0f; // Speed for gyro (72 km/h)
    data.mPhysicalSteeringWheelRange = 9.4247f;
    
    // 1. Gyro Damping Integration
    data.mUnfilteredSteering = 0.1f;
    engine.calculate_force(&data); // First call to prime m_prev_steering_angle (Hits Line 1922)
    
    data.mUnfilteredSteering = 0.5f; // Large velocity change
    engine.calculate_force(&data); // Second call (Hits Line 1921 & 1929)
    
    // 2. ABS Pulse Integration
    engine.m_abs_pulse_enabled = true;
    data.mUnfilteredBrake = 1.0f;
    for(int i=0; i<4; i++) data.mWheel[i].mBrakePressure = 1.0f;
    engine.calculate_force(&data); // Hits Line 1612 (update m_prev_brake_pressure)
    
    for(int i=0; i<4; i++) data.mWheel[i].mBrakePressure = 10.0f; // High delta
    engine.calculate_force(&data); // Hits Line 1938 & 1947
    
    // Verify snapshots in batch
    // Snapshots are stored in m_debug_buffer (Line 1625)
    // and retrieved with GetDebugBatch (Line 594)
    auto batch = engine.GetDebugBatch();
    ASSERT_FALSE(batch.empty());
    
    bool found_gyro = false;
    bool found_abs = false;
    for(const auto& snap : batch) {
        if (std::abs(snap.ffb_gyro_damping) > 0.0f) found_gyro = true;
        if (std::abs(snap.ffb_abs_pulse) > 0.0f) found_abs = true;
    }
    
    ASSERT_TRUE(found_gyro);
    ASSERT_TRUE(found_abs);
}

TEST_CASE(test_parse_vehicle_class_coverage, "Coverage") {
    // FFBEngine engine; // Not needed for static method

    // Line 731: else if (cls.find("WEC") != std::string::npos)
    ASSERT_EQ((int)ParseVehicleClass("LMP2 WEC", "ORECA"), (int)ParsedVehicleClass::LMP2_RESTRICTED);
    
    // Line 733/734: else (LMP2 unspecified)
    ASSERT_EQ((int)ParseVehicleClass("LMP2", "ORECA"), (int)ParsedVehicleClass::LMP2_UNSPECIFIED);

    // Other branches
    ASSERT_EQ((int)ParseVehicleClass("LMP2 ELMS", "ORECA"), (int)ParsedVehicleClass::LMP2_UNRESTRICTED);
    ASSERT_EQ((int)ParseVehicleClass("HYPERCAR", ""), (int)ParsedVehicleClass::HYPERCAR);
}

TEST_CASE(test_calculate_slope_grip_torque_coverage, "Coverage") {
    FFBEngine engine;
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mSteeringShaftTorque = 1.0f;

    // Path 1: m_slope_use_torque = true, data != nullptr (Line 1203 and 1226)
    FFBEngineTestAccess::SetSlopeUseTorque(engine, true);
    double out = FFBEngineTestAccess::CallCalculateSlopeGrip(engine, 1.0, 0.1, 0.01, &data);
    ASSERT_TRUE(std::isfinite(out));

    // Path 2: m_slope_use_torque = false (Else of 1203 -> Line 1215)
    FFBEngineTestAccess::SetSlopeUseTorque(engine, false);
    out = FFBEngineTestAccess::CallCalculateSlopeGrip(engine, 1.0, 0.1, 0.01, &data);
    ASSERT_TRUE(std::isfinite(out));
    
    // Path 3: data == nullptr
    FFBEngineTestAccess::SetSlopeUseTorque(engine, true);
    out = FFBEngineTestAccess::CallCalculateSlopeGrip(engine, 1.0, 0.1, 0.01, nullptr);
    ASSERT_TRUE(std::isfinite(out));
}

} // namespace FFBEngineTests
