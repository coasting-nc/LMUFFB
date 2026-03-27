#include "test_ffb_common.h"

namespace FFBEngineTests {

namespace {
// Helper function to simulate continuous steering and measure peak gyro force.
static double MeasureGyroForceRepro(FFBEngine& engine, TelemInfoV01& data, double target_lat_g, double steer_vel_rad_s) {
    // 1. Set Lateral G and let the upsampler and chassis inertia filters settle
    data.mLocalAccel.x = (float)(target_lat_g * 9.81);

    // We need to pump enough time for both derived acceleration (if used) and upsamplers to settle.
    for (int i = 0; i < 200; i++) {
        data.mElapsedTime += 0.0025;
        engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.0025);
    }

    // 2. Steer continuously for 0.5 seconds to get a stable velocity reading
    double dt_game = 0.01; // 100Hz telemetry tick
    double range_rad = data.mPhysicalSteeringWheelRange;
    if (range_rad <= 0.0) range_rad = 9.4247; // Default 540 deg

    double peak_gyro = 0.0;

    for (int i = 0; i < 50; i++) {
        double delta_norm = (steer_vel_rad_s * dt_game) / (range_rad / 2.0);
        data.mUnfilteredSteering += (float)delta_norm;
        data.mElapsedTime += dt_game;

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
} // anonymous namespace

TEST_CASE(test_gyro_lat_g_gate, "Issue511") {
    std::cout << "\nTest Issue #511: Gyro Lateral G Gate" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mPhysicalSteeringWheelRange = 9.4247f;

    engine.m_advanced.gyro_gain = 1.0f;
    engine.m_advanced.gyro_smoothing = 0.015f;

    engine.m_front_axle.understeer_effect = 0.0f;
    engine.m_rear_axle.sop_effect = 0.0f;
    engine.m_rear_axle.rear_align_effect = 0.0f;
    engine.m_rear_axle.sop_yaw_gain = 0.0f;

    // 1. Straight Line (0.0G): Full damping
    double gyro_0g = MeasureGyroForceRepro(engine, data, 0.0, 1.0);
    std::cout << "  Damping at 0.0G: " << gyro_0g << " Nm" << std::endl;
    ASSERT_GT(gyro_0g, 0.5);

    // 2. Lower threshold (0.1G): Still full damping
    double gyro_01g = MeasureGyroForceRepro(engine, data, 0.1, 1.0);
    std::cout << "  Damping at 0.1G: " << gyro_01g << " Nm" << std::endl;
    ASSERT_NEAR(gyro_01g, gyro_0g, 0.05);

    // 3. Mid-Gate (0.25G): Reduced damping (smoothstep fade out)
    double gyro_025g = MeasureGyroForceRepro(engine, data, 0.25, 1.0);
    std::cout << "  Damping at 0.25G: " << gyro_025g << " Nm" << std::endl;
    ASSERT_TRUE(gyro_025g < gyro_0g * 0.9);
    ASSERT_GT(gyro_025g, 0.1);

    // 4. Above Upper threshold (0.4G): Zero damping
    double gyro_04g = MeasureGyroForceRepro(engine, data, 0.4, 1.0);
    std::cout << "  Damping at 0.4G: " << gyro_04g << " Nm" << std::endl;
    ASSERT_NEAR(gyro_04g, 0.0, 0.001);
}

TEST_CASE(test_gyro_velocity_deadzone, "Issue511") {
    std::cout << "\nTest Issue #511: Gyro Velocity Deadzone" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mPhysicalSteeringWheelRange = 9.4247f;

    engine.m_advanced.gyro_gain = 1.0f;
    engine.m_advanced.gyro_smoothing = 0.015f;

    double gyro_in_dz = MeasureGyroForceRepro(engine, data, 0.0, 0.4);
    std::cout << "  Peak damping at 0.4 rad/s (deadzone 0.5): " << gyro_in_dz << " Nm" << std::endl;
    ASSERT_NEAR(gyro_in_dz, 0.0, 0.001);

    double gyro_out_dz = MeasureGyroForceRepro(engine, data, 0.0, 1.0);
    std::cout << "  Peak damping at 1.0 rad/s (deadzone 0.5): " << gyro_out_dz << " Nm" << std::endl;
    ASSERT_GT(gyro_out_dz, 0.1);
}

TEST_CASE(test_gyro_force_cap, "Issue511") {
    std::cout << "\nTest Issue #511: Gyro Force Cap" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mPhysicalSteeringWheelRange = 9.4247f;

    engine.m_advanced.gyro_gain = 10.0f;
    engine.m_advanced.gyro_smoothing = 0.015f;

    double gyro_capped = MeasureGyroForceRepro(engine, data, 0.0, 5.0);
    std::cout << "  Peak damping force with massive gain: " << gyro_capped << " Nm" << std::endl;
    ASSERT_NEAR(gyro_capped, 2.0, 0.01);
}

} // namespace FFBEngineTests
