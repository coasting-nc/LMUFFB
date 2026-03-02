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

    // Redirect stdout to a stringstream to capture the warning
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

    TelemInfoV01 data = CreateBasicTestTelemetry(10.0, 0.0);
    strncpy(data.mVehicleName, "Ferrari 296 GT3", 63);
    data.mPhysicalSteeringWheelRange = 0.0f; // Invalid

    // Frame 1: Should issue warning
    engine.calculate_force(&data, "GT3", "Ferrari 296 GT3");
    std::string output1 = buffer.str();
    ASSERT_TRUE(output1.find("[WARNING] Invalid PhysicalSteeringWheelRange") != std::string::npos);

    // Clear buffer
    buffer.str("");
    buffer.clear();

    // Frame 2: Should NOT issue warning again
    engine.calculate_force(&data, "GT3", "Ferrari 296 GT3");
    std::string output2 = buffer.str();
    ASSERT_TRUE(output2.empty());

    // Reset warning via car change (using different class to trigger seeding)
    buffer.str("");
    buffer.clear();
    // 1st call with new class: detects car change, resets m_warned_invalid_range, then checks range and warns.
    strncpy(data.mVehicleName, "Porsche 911 RSR GTE", 63);
    engine.calculate_force(&data, "GTE", "Porsche 911 RSR GTE");
    std::string output3 = buffer.str();
    ASSERT_TRUE(output3.find("[WARNING] Invalid PhysicalSteeringWheelRange") != std::string::npos);

    // Restore stdout
    std::cout.rdbuf(old);
}

} // namespace FFBEngineTests
