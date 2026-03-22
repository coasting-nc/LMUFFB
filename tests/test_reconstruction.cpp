#include "test_ffb_common.h"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace FFBEngineTests {

// --- Issue #461: Standard Holt-Winters Math Verification ---

TEST_CASE(test_holtwinters_prediction_accuracy, "Math") {
    ffb_math::HoltWintersFilter filter;
    filter.Configure(0.8, 0.2, 0.01); // Standard settings, 100Hz game tick
    filter.SetZeroLatency(true);

    // Initial sample
    double out1 = filter.Process(10.0, 0.0025, true);
    ASSERT_NEAR(out1, 10.0, 0.001);

    // Next game frame arrives (ramp up to 20.0)
    // After 10ms (at 100Hz), if value went from 10 to 20, slope is 10/0.01 = 1000 units/s
    double out2 = filter.Process(20.0, 0.0025, true);

    // With alpha=0.8, Level = 0.8 * 20 + 0.2 * (10 + 0*0.01) = 16 + 2 = 18.0
    // With beta=0.2, Trend = 0.2 * (18 - 10)/0.01 + 0.8 * 0 = 0.2 * 800 = 160 units/s
    ASSERT_NEAR(out2, 18.0, 0.001);

    // Process intra-frame (2.5ms later)
    double out3 = filter.Process(20.0, 0.0025, false);

    // Zero Latency Predicts: Level + Trend * time_since_update
    // Prediction = 18.0 + 160 * 0.0025 = 18.0 + 0.4 = 18.4
    ASSERT_NEAR(out3, 18.4, 0.001);
}

TEST_CASE(test_holtwinters_interpolation_smooth, "Math") {
    ffb_math::HoltWintersFilter filter;
    filter.Configure(0.8, 0.2, 0.01);
    filter.SetZeroLatency(false); // Smooth mode

    filter.Process(10.0, 0.0025, true);

    // New frame arrives
    double out2 = filter.Process(20.0, 0.0025, true);

    // In Smooth mode, on a new frame, it returns m_prev_level
    // Prev level was 10.0
    ASSERT_NEAR(out2, 10.0, 0.001);

    // Process intra-frame (2.5ms later)
    double out3 = filter.Process(20.0, 0.0025, false);

    // Interpolates between 10.0 (prev_level) and 18.0 (level)
    // t = 0.0025 / 0.01 = 0.25
    // Prediction = 10.0 + 0.25 * (18.0 - 10.0) = 10.0 + 2.0 = 12.0
    ASSERT_NEAR(out3, 12.0, 0.001);
}

TEST_CASE(test_config_reconstruction_persistence_461, "Config") {
    FFBEngine engine;

    // 1. Set a non-default value
    engine.m_advanced.aux_telemetry_reconstruction = 1;

    // 2. Save to a temporary file
    std::string test_config = "test_config_461_v2.toml";
    Config::Save(engine, test_config);

    // 3. Reset engine state
    engine.m_advanced.aux_telemetry_reconstruction = 0;

    // 4. Load from the file
    Config::Load(engine, test_config);

    // 5. Verify
    ASSERT_EQ(engine.m_advanced.aux_telemetry_reconstruction, 1);

    // Cleanup
    if (std::filesystem::exists(test_config)) {
        std::filesystem::remove(test_config);
    }
}

// --- Issue #466: DSP Differentiation Verification ---

TEST_CASE(test_aux_channel_group_initialization, "DSP") {
    FFBEngine engine;

    // Group 1: Driver Inputs (Alpha 0.95, Beta 0.40)
    ASSERT_NEAR(engine.m_upsample_steering.GetAlpha(), 0.95, 0.001);
    ASSERT_NEAR(engine.m_upsample_steering.GetBeta(), 0.40, 0.001);
    ASSERT_NEAR(engine.m_upsample_throttle.GetAlpha(), 0.95, 0.001);
    ASSERT_NEAR(engine.m_upsample_brake.GetAlpha(), 0.95, 0.001);
    for(int i=0; i<4; i++) ASSERT_NEAR(engine.m_upsample_brake_pressure[i].GetAlpha(), 0.95, 0.001);

    // Group 2: Texture/Tire (Alpha 0.80, Beta 0.20)
    ASSERT_NEAR(engine.m_upsample_vert_deflection[0].GetAlpha(), 0.80, 0.001);
    ASSERT_NEAR(engine.m_upsample_lat_patch_vel[0].GetAlpha(), 0.80, 0.001);
    ASSERT_NEAR(engine.m_upsample_long_patch_vel[0].GetAlpha(), 0.80, 0.001);
    ASSERT_NEAR(engine.m_upsample_rotation[0].GetAlpha(), 0.80, 0.001);

    // Group 3: Chassis/Impacts (Alpha 0.50, Beta 0.00)
    ASSERT_NEAR(engine.m_upsample_local_accel_x.GetAlpha(), 0.50, 0.001);
    ASSERT_NEAR(engine.m_upsample_local_accel_x.GetBeta(), 0.00, 0.001);
    ASSERT_NEAR(engine.m_upsample_local_accel_y.GetAlpha(), 0.50, 0.001);
    ASSERT_NEAR(engine.m_upsample_local_accel_z.GetAlpha(), 0.50, 0.001);
    ASSERT_NEAR(engine.m_upsample_local_rot_accel_y.GetAlpha(), 0.50, 0.001);
    ASSERT_NEAR(engine.m_upsample_local_rot_y.GetAlpha(), 0.50, 0.001);
    for(int i=0; i<4; i++) ASSERT_NEAR(engine.m_upsample_susp_force[i].GetAlpha(), 0.50, 0.001);
}

TEST_CASE(test_aux_channel_mode_switching, "DSP") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry();

    // Test Mode 0: Zero Latency (User Toggle affects Group 2)
    engine.m_advanced.aux_telemetry_reconstruction = 0;
    engine.calculate_force(&data);

    // Group 1: ALWAYS True
    ASSERT_TRUE(engine.m_upsample_steering.GetZeroLatency());

    // Group 2: User Choice (True in Mode 0)
    ASSERT_TRUE(engine.m_upsample_vert_deflection[0].GetZeroLatency());

    // Group 3: ALWAYS False
    ASSERT_FALSE(engine.m_upsample_local_accel_x.GetZeroLatency());

    // Test Mode 1: Smooth (User Toggle affects Group 2)
    engine.m_advanced.aux_telemetry_reconstruction = 1;
    engine.calculate_force(&data);

    // Group 1: ALWAYS True
    ASSERT_TRUE(engine.m_upsample_steering.GetZeroLatency());

    // Group 2: User Choice (False in Mode 1)
    ASSERT_FALSE(engine.m_upsample_vert_deflection[0].GetZeroLatency());

    // Group 3: ALWAYS False
    ASSERT_FALSE(engine.m_upsample_local_accel_x.GetZeroLatency());
}

} // namespace FFBEngineTests
