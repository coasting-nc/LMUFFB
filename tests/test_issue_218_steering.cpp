#include "test_ffb_common.h"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

namespace FFBEngineTests {

TEST_CASE(test_issue_218_steering_calculations, "Issue_218") {
    FFBEngine engine;
    InitializeEngine(engine);

    TelemInfoV01 data = CreateBasicTestTelemetry(10.0, 0.0);
    // 900 degrees = 15.7079632679 radians
    data.mPhysicalSteeringWheelRange = 15.7079632679f;
    data.mUnfilteredSteering = 0.5f; // 50% to the right

    // Process one frame
    engine.calculate_force(&data, "GT3", "Ferrari 296 GT3");

    auto batch = engine.GetDebugBatch();
    ASSERT_FALSE(batch.empty());
    if (!batch.empty()) {
        const auto& snap = batch.back();
        // 900 deg range, 0.5 input should be 225 deg angle
        ASSERT_NEAR(snap.steering_range_deg, 900.0f, 0.1f);
        ASSERT_NEAR(snap.steering_angle_deg, 225.0f, 0.1f);
    }
}

TEST_CASE(test_issue_218_invalid_range_warning, "Issue_218") {
    FFBEngine engine;
    InitializeEngine(engine);

    std::stringstream logBuffer;
    Logger::Get().SetTestStream(&logBuffer);

    TelemInfoV01 data = CreateBasicTestTelemetry(10.0, 0.0);
    StringUtils::SafeCopy(data.mVehicleName, sizeof(data.mVehicleName), "Ferrari 296 GT3");
    data.mPhysicalSteeringWheelRange = 0.0f; // Invalid

    // Frame 1: Should issue warning
    engine.calculate_force(&data, "GT3", "Ferrari 296 GT3");
    ASSERT_TRUE(logBuffer.str().find("[WARNING] Invalid PhysicalSteeringWheelRange") != std::string::npos);

    // Clear buffer
    logBuffer.str("");
    logBuffer.clear();

    // Frame 2: Should NOT issue warning again (if it's the same car)
    engine.calculate_force(&data, "GT3", "Ferrari 296 GT3");
    // Check specifically for the warning message, ignoring other logs (like REST API)
    ASSERT_TRUE(logBuffer.str().find("[WARNING] Invalid PhysicalSteeringWheelRange") == std::string::npos);

    // Reset warning via car change (using different class to trigger seeding)
    // 1st call with new class: detects car change, resets m_warned_invalid_range, then checks range and warns.
    StringUtils::SafeCopy(data.mVehicleName, sizeof(data.mVehicleName), "Porsche 911 RSR GTE");
    engine.calculate_force(&data, "GTE", "Porsche 911 RSR GTE");
    ASSERT_TRUE(logBuffer.str().find("[WARNING] Invalid PhysicalSteeringWheelRange") != std::string::npos);

    Logger::Get().SetTestStream(nullptr);
}

} // namespace FFBEngineTests
