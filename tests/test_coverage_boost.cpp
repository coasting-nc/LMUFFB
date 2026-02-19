#include "test_ffb_common.h"
#include <thread>
#include <chrono>

namespace FFBEngineTests {

TEST_CASE(test_coverage_slope_torque, "Coverage") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry();
    FFBEngineTestAccess::SetSlopeUseTorque(engine, true);
    
    // Setup torque buffer to produce a negative derivative if possible, 
    // or just set m_slope_torque_current directly if we can (we can't yet).
    // Let's use the buffer. m_slope_torque_current is calculated in calculate_slope_grip.
    // Wait, m_slope_torque_current is calculated at line 1209.
    
    // Fill buffers to allow SG derivative calculation
    std::array<double, 41> torque_buf = {};
    std::array<double, 41> steer_buf = {};
    std::array<double, 41> slip_buf = {};
    std::array<double, 41> lat_g_buf = {};

    for(int i=0; i<41; i++) {
        torque_buf[i] = 100.0 - (i * 2.0); // Negative derivative
        steer_buf[i] = 0.5 + (i * 0.1);    // Positive derivative
        slip_buf[i] = 0.1 + (i * 0.01);
        lat_g_buf[i] = 5.0 + (i * 0.5);
    }
    
    FFBEngineTestAccess::SetSlopeTorqueBuffer(engine, torque_buf);
    FFBEngineTestAccess::SetSlopeSteerBuffer(engine, steer_buf);
    FFBEngineTestAccess::SetSlopeSlipBuffer(engine, slip_buf);
    FFBEngineTestAccess::SetSlopeBuffer(engine, lat_g_buf);
    FFBEngineTestAccess::SetSlopeBufferIndex(engine, 0);
    FFBEngineTestAccess::SetSlopeBufferCount(engine, 41);
    
    // Call calculation - this should update m_slope_torque_current and use it
    // dTorque_dt < 0, dSteer_dt > 0 => num < 0 => m_slope_torque_current < 0
    // This hits Line 1227: if (m_slope_torque_current < 0.0)
    double output = FFBEngineTestAccess::CallCalculateSlopeGrip(engine, 1.0, 0.1, 0.01, &data);
    ASSERT_TRUE(std::isfinite(output));

    // Coverage for Line 1210: else branch when dSteer_dt is small
    for(int i=0; i<41; i++) steer_buf[i] = 0.5; // Zero derivative
    FFBEngineTestAccess::SetSlopeSteerBuffer(engine, steer_buf);
    output = FFBEngineTestAccess::CallCalculateSlopeGrip(engine, 1.0, 0.1, 0.01, &data);
    ASSERT_TRUE(std::isfinite(output));

    // Coverage for Line 1225 area (else branch when torque disabled)
    FFBEngineTestAccess::SetSlopeUseTorque(engine, false);
    output = FFBEngineTestAccess::CallCalculateSlopeGrip(engine, 1.0, 0.1, 0.01, &data);
    ASSERT_TRUE(std::isfinite(output));

    // Coverage for data == nullptr
    output = FFBEngineTestAccess::CallCalculateSlopeGrip(engine, 1.0, 0.1, 0.01, nullptr);
    ASSERT_TRUE(std::isfinite(output));
}

TEST_CASE(test_coverage_stats_latching, "Coverage") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry();
    
    // Initialize stats with some data
    FFBEngineTestAccess::GetTorqueStats(engine).Update(50.0);
    data.mSteeringShaftTorque = 50.0f;
    
    // Set last_log_time to 2 seconds ago
    auto two_secs_ago = std::chrono::steady_clock::now() - std::chrono::seconds(2);
    FFBEngineTestAccess::SetLastLogTime(engine, two_secs_ago);
    
    // Call calculate_force (which calls the stats latching logic)
    engine.calculate_force(&data);
    
    // Verify that interval_count was reset to 0 (after latching into l_avg)
    ASSERT_EQ(FFBEngineTestAccess::GetTorqueStats(engine).interval_count, 0);
    ASSERT_NEAR(FFBEngineTestAccess::GetTorqueStats(engine).l_avg, 50.0, 0.001);
}

TEST_CASE(test_coverage_flatspot, "Coverage") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry(10.0); // 10 m/s
    FFBCalculationContext ctx;
    ctx.dt = 0.0025;
    ctx.car_speed = 10.0;
    
    // wheel freq = speed / circumference. 10 / (2*PI*0.33) approx 4.8 Hz > 1.0
    FFBEngineTestAccess::SetFlatspotSuppression(engine, true);
    FFBEngineTestAccess::SetFlatspotStrength(engine, 0.5f);
    
    double out = FFBEngineTestAccess::CallApplySignalConditioning(engine, 1.0, &data, ctx);
    // Should be filtered.
    ASSERT_TRUE(std::isfinite(out));

    // Coverage for Line 1825: else branch of if (wheel_freq > 1.0)
    ctx.car_speed = 0.5; // Very low speed -> wheel_freq < 1.0
    out = FFBEngineTestAccess::CallApplySignalConditioning(engine, 1.0, &data, ctx);
    ASSERT_TRUE(std::isfinite(out));

    // Coverage for m_flatspot_suppression = false
    FFBEngineTestAccess::SetFlatspotSuppression(engine, false);
    out = FFBEngineTestAccess::CallApplySignalConditioning(engine, 1.0, &data, ctx);
    ASSERT_TRUE(std::isfinite(out));
}

TEST_CASE(test_coverage_gyro_damping, "Coverage") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    FFBCalculationContext ctx;
    ctx.dt = 0.01;
    ctx.car_speed = 20.0;
    
    data.mPhysicalSteeringWheelRange = 10.0f; // 10 rad
    data.mUnfilteredSteering = 0.5f; // 0.5 * 5 = 2.5 rad
    
    FFBEngineTestAccess::CallCalculateGyroDamping(engine, &data, ctx);
    // ctx.gyro_force should be populated
    ASSERT_TRUE(std::isfinite(ctx.gyro_force));

    // Coverage for range <= 0.0f (Line 1917)
    data.mPhysicalSteeringWheelRange = 0.0f;
    FFBEngineTestAccess::CallCalculateGyroDamping(engine, &data, ctx);
    ASSERT_TRUE(std::isfinite(ctx.gyro_force));
}

TEST_CASE(test_coverage_abs_pulse, "Coverage") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry(10.0);
    FFBCalculationContext ctx;
    ctx.dt = 0.01;
    ctx.speed_gate = 1.0;
    
    FFBEngineTestAccess::SetABSPulseEnabled(engine, true);
    data.mUnfilteredBrake = 0.8f; // Above threshold 0.5
    
    // Need pressure delta > threshold 2.0
    // prev pressure is 0 by default. set current pressure to 1.0. 
    // delta = (1.0 - 0) / 0.01 = 100.0 > 2.0
    for(int i=0; i<4; i++) data.mWheel[i].mBrakePressure = 1.0f;
    
    FFBEngineTestAccess::CallCalculateABSPulse(engine, &data, ctx);
    ASSERT_TRUE(std::isfinite(ctx.abs_pulse_force));
    
    // Coverage for m_abs_pulse_enabled = false
    FFBEngineTestAccess::SetABSPulseEnabled(engine, false);
    FFBEngineTestAccess::CallCalculateABSPulse(engine, &data, ctx);
    ASSERT_TRUE(std::isfinite(ctx.abs_pulse_force));
}

TEST_CASE(test_coverage_integrated, "Coverage") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry(30.0); // 30 m/s
    data.mDeltaTime = 0.0025f;
    data.mSteeringShaftTorque = 1.0f;
    
    // 1. Verify ParseVehicleClass seeding (Line 730 WEC)
    engine.calculate_force(&data, "LMP2 WEC", "ORECA");
    ASSERT_NEAR(FFBEngineTestAccess::GetAutoPeakLoad(engine), 7500.0, 1.0);
    
    // 2. Verify ParseVehicleClass seeding (Line 733 Unspecified / Unrestricted)
    engine.calculate_force(&data, "LMP2 ELMS", "ORECA"); 
    ASSERT_NEAR(FFBEngineTestAccess::GetAutoPeakLoad(engine), 8500.0, 1.0);

    // 3. Verify calculate_gyro_damping fallback (Line 1917)
    data.mPhysicalSteeringWheelRange = 0.0f;
    engine.calculate_force(&data, "GT3", "M4 GT3");
    auto batch = engine.GetDebugBatch();
    ASSERT_FALSE(batch.empty());
    ASSERT_TRUE(std::isfinite(batch.back().ffb_gyro_damping));
    
    // 4. Verify calculate_abs_pulse (Line 1930)
    FFBEngineTestAccess::SetABSPulseEnabled(engine, true);
    FFBEngineTestAccess::SetAutoPeakLoad(engine, 100.0); // Extreme scaling for visibility
    data.mUnfilteredBrake = 0.8f;
    for(int i=0; i<4; i++) data.mWheel[i].mBrakePressure = 1.0f;
    
    engine.calculate_force(&data, "GT3", "M4 GT3"); // Prime prev_brake_pressure
    for(int i=0; i<4; i++) data.mWheel[i].mBrakePressure = 10.0f; // Rapid change
    
    engine.calculate_force(&data, "GT3", "M4 GT3"); 
    batch = engine.GetDebugBatch();
    ASSERT_FALSE(batch.empty());
    // ABS phase should have advanced and produced a pulse
    ASSERT_GT(std::abs(batch.back().ffb_abs_pulse), 0.001f);
    
    // 5. Verify calculate_slope_grip with torque fusion (Line 1225)
    FFBEngineTestAccess::SetSlopeDetectionEnabled(engine, true);
    FFBEngineTestAccess::SetSlopeUseTorque(engine, true);
    data.mWheel[0].mGripFract = 0.0f;
    data.mWheel[1].mGripFract = 0.0f;
    data.mWheel[0].mTireLoad = 1000.0f;
    data.mWheel[1].mTireLoad = 1000.0f;
    
    for(int i=0; i<45; i++) {
        data.mSteeringShaftTorque = 1.0f - (i * 0.1f); 
        data.mUnfilteredSteering = 0.1f + (i * 0.01f);
        data.mLocalAccel.x = 5.0f + (i * 0.5f); // Create G derivative
        data.mWheel[0].mLateralPatchVel = 1.0f + (i * 0.1f); // Create Slip derivative
        data.mWheel[1].mLateralPatchVel = 1.0f + (i * 0.1f);
        engine.calculate_force(&data, "GT3", "M4 GT3"); 
    }
    
    batch = engine.GetDebugBatch();
    ASSERT_FALSE(batch.empty());
    // Slope should have been detected and stored in snapshot
    ASSERT_GT(std::abs(batch.back().slope_current), 0.001f);
    
    // Ensure cleanup
    FFBEngineTestAccess::SetABSPulseEnabled(engine, false);
    FFBEngineTestAccess::SetSlopeDetectionEnabled(engine, false);
}

} // namespace FFBEngineTests
