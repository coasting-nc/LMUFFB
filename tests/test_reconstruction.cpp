#include "test_ffb_common.h"
#include <fstream>
#include <filesystem>

namespace FFBEngineTests {

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

}
