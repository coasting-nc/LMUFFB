#include "test_ffb_common.h"

namespace FFBEngineTests {

// Helper function to simulate continuous steering and measure peak gyro force.
// This correctly simulates the 100Hz telemetry updates being processed by the 400Hz engine,
// allowing the velocity derivative and the Lateral G upsamplers to settle properly.
static double MeasureGyroForce(FFBEngine& engine, TelemInfoV01& data, double target_lat_g, double steer_vel_rad_s) {
    // 1. Set Lateral G and let the upsampler and chassis inertia filters settle
    data.mLocalAccel.x = (float)(target_lat_g * 9.81);
    PumpEngineTime(engine, data, 0.5); // 0.5s is plenty for the 100ms LPF to settle

    // 2. Steer continuously for 0.5 seconds to get a stable velocity reading
    double dt_game = 0.01; // 100Hz telemetry tick
    double range_rad = data.mPhysicalSteeringWheelRange;
    if (range_rad <= 0.0) range_rad = 9.4247; // Default 540 deg

    double peak_gyro = 0.0;

    for (int i = 0; i < 50; i++) {
        // Calculate how much normalized steering[-1, 1] to add per 10ms tick
        // to achieve the target steer_vel_rad_s.
        double delta_norm = (steer_vel_rad_s * dt_game) / (range_rad / 2.0);
        data.mUnfilteredSteering += (float)delta_norm;

        data.mElapsedTime += dt_game;

        // Pump 4 physics ticks (400Hz) per 1 game tick (100Hz)
        for (int j = 0; j < 4; j++) {
            engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.0025);
            auto batch = engine.GetDebugBatch();
            if (!batch.empty()) {
                double current_gyro = std::abs(batch.back().ffb_gyro_damping);
                if (current_gyro > peak_gyro) {
                    peak_gyro = current_gyro;
                }
            }
        }
    }
    return peak_gyro;
}

TEST_CASE(test_gyro_lat_g_gate, "Issue511") {
    std::cout << "\nTest Issue #511: Gyro Lateral G Gate" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);

    // Setup Gyro
    engine.m_advanced.gyro_gain = 1.0f;
    engine.m_advanced.gyro_smoothing = 0.015f; // 15ms smoothing

    // Disable other effects to isolate gyro
    engine.m_front_axle.understeer_effect = 0.0f;
    engine.m_rear_axle.sop_effect = 0.0f;
    engine.m_rear_axle.rear_align_effect = 0.0f;
    engine.m_rear_axle.sop_yaw_gain = 0.0f;

    // 1. Straight Line (0.0G): Full damping
    engine.m_advanced.gyro_lat_g_gate_lower = 0.1f;
    engine.m_advanced.gyro_lat_g_gate_upper = 0.4f;
    engine.m_advanced.gyro_max_nm = 50.0f;
    engine.m_advanced.gyro_vel_deadzone = 0.0f;

    double gyro_0g = MeasureGyroForce(engine, data, 0.0, 1.0);
    std::cout << "  Damping at 0.0G: " << gyro_0g << " Nm" << std::endl;
    ASSERT_GT(gyro_0g, 0.1);

    // 2. Lower threshold (0.1G): Still full damping
    double gyro_01g = MeasureGyroForce(engine, data, 0.1, 1.0);
    std::cout << "  Damping at 0.1G: " << gyro_01g << " Nm" << std::endl;
    ASSERT_NEAR(gyro_01g, gyro_0g, 0.05);

    // 3. Mid-Gate (0.25G): Reduced damping (smoothstep fade out)
    double gyro_025g = MeasureGyroForce(engine, data, 0.25, 1.0);
    std::cout << "  Damping at 0.25G: " << gyro_025g << " Nm" << std::endl;
    ASSERT_TRUE(gyro_025g < gyro_0g * 0.9);
    ASSERT_GT(gyro_025g, 0.01);

    // 4. Above Upper threshold (0.4G): Zero damping
    double gyro_04g = MeasureGyroForce(engine, data, 0.4, 1.0);
    std::cout << "  Damping at 0.4G: " << gyro_04g << " Nm" << std::endl;
    ASSERT_NEAR(gyro_04g, 0.0, 0.001);

    g_tests_passed++;
}

TEST_CASE(test_gyro_velocity_deadzone, "Issue511") {
    std::cout << "\nTest Issue #511: Gyro Velocity Deadzone" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);

    engine.m_advanced.gyro_gain = 1.0f;
    engine.m_advanced.gyro_smoothing = 0.015f;
    engine.m_advanced.gyro_vel_deadzone = 0.5f;
    engine.m_advanced.gyro_lat_g_gate_lower = 10.0f; // Disable gating

    // 1. Velocity within deadzone: 0.4 rad/s (Deadzone is 0.5 rad/s)
    double gyro_in_dz = MeasureGyroForce(engine, data, 0.0, 0.4);
    std::cout << "  Peak damping at 0.4 rad/s (deadzone 0.5): " << gyro_in_dz << " Nm" << std::endl;
    ASSERT_NEAR(gyro_in_dz, 0.0, 0.001);

    // 2. Velocity outside deadzone: 1.0 rad/s
    double gyro_out_dz = MeasureGyroForce(engine, data, 0.0, 1.0);
    std::cout << "  Peak damping at 1.0 rad/s (deadzone 0.5): " << gyro_out_dz << " Nm" << std::endl;
    ASSERT_GT(gyro_out_dz, 0.01);

    g_tests_passed++;
}

TEST_CASE(test_gyro_force_cap, "Issue511") {
    std::cout << "\nTest Issue #511: Gyro Force Cap" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);

    engine.m_advanced.gyro_gain = 10.0f; // Massive gain to force the cap
    engine.m_advanced.gyro_smoothing = 0.015f;
    engine.m_advanced.gyro_max_nm = 2.0f;
    engine.m_advanced.gyro_vel_deadzone = 0.0f;
    engine.m_advanced.gyro_lat_g_gate_lower = 10.0f; // Disable gating

    // Fast steering movement: 5.0 rad/s
    double gyro_capped = MeasureGyroForce(engine, data, 0.0, 5.0);

    std::cout << "  Peak damping force with massive gain: " << gyro_capped << " Nm" << std::endl;

    // Should be capped at exactly 2.0 Nm (hardcoded safety limit)
    ASSERT_NEAR(gyro_capped, 2.0, 0.01);

    g_tests_passed++;
}

} // namespace FFBEngineTests
