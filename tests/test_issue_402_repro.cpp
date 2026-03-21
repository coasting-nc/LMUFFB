#include "test_ffb_common.h"

namespace FFBEngineTests {

// [Regression][Texture] Road Texture Time-Domain Independence (Issue #402)
TEST_CASE(test_road_texture_time_domain_independence, "Texture") {
    std::cout << "\nTest: Road Texture Time-Domain Independence (Issue #402) [Regression][Texture]" << std::endl;
    FFBEngine engine_100hz;
    FFBEngine engine_400hz;
    InitializeEngine(engine_100hz);
    InitializeEngine(engine_400hz);

    engine_100hz.m_vibration.road_enabled = true;
    engine_100hz.m_vibration.road_gain = 1.0f;
    engine_400hz.m_vibration.road_enabled = true;
    engine_400hz.m_vibration.road_gain = 1.0f;

    // Use a class with known seed load (GT3 -> 4500N)
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mWheel[0].mTireLoad = 4500.0;
    data.mWheel[1].mTireLoad = 4500.0;
    data.mWheel[2].mTireLoad = 4500.0;
    data.mWheel[3].mTireLoad = 4500.0;

    // Simulate a constant suspension velocity of 0.5 m/s
    double susp_velocity = 0.5;

    // 1. Run at 100Hz (dt = 0.01)
    data.mWheel[0].mVerticalTireDeflection = 0.0;
    data.mWheel[1].mVerticalTireDeflection = 0.0;
    data.mElapsedTime = 0.0;
    engine_100hz.calculate_force(&data, "GT3", "911", 0.0f, true, 0.01); // Seed

    // Pump 1 second of simulation to stabilize EMA
    for(int i=0; i<100; i++) {
        data.mWheel[0].mVerticalTireDeflection += susp_velocity * 0.01;
        data.mWheel[1].mVerticalTireDeflection += susp_velocity * 0.01;
        data.mElapsedTime += 0.01;
        engine_100hz.calculate_force(&data, "GT3", "911", 0.0f, true, 0.01);
    }
    float force_100hz = engine_100hz.GetDebugBatch().back().texture_road;

    // 2. Run at 400Hz (dt = 0.0025)
    data.mWheel[0].mVerticalTireDeflection = 0.0;
    data.mWheel[1].mVerticalTireDeflection = 0.0;
    data.mElapsedTime = 0.0;
    engine_400hz.calculate_force(&data, "GT3", "911", 0.0f, true, 0.0025); // Seed

    // Pump 1 second of simulation to stabilize EMA
    for(int i=0; i<400; i++) {
        data.mWheel[0].mVerticalTireDeflection += susp_velocity * 0.0025;
        data.mWheel[1].mVerticalTireDeflection += susp_velocity * 0.0025;
        data.mElapsedTime += 0.0025;
        engine_400hz.calculate_force(&data, "GT3", "911", 0.0f, true, 0.0025);
    }
    float force_400hz = engine_400hz.GetDebugBatch().back().texture_road;

    // 3. ASSERT: The output force should be identical regardless of the loop rate
    std::cout << "Force at 100Hz: " << force_100hz << " Force at 400Hz: " << force_400hz << std::endl;

    ASSERT_GT(std::abs(force_100hz), 0.01f); // Ensure effect triggered
    ASSERT_NEAR(force_100hz, force_400hz, 0.01f);
}

} // namespace FFBEngineTests
