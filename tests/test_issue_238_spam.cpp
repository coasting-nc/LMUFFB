#include "test_ffb_common.h"
#include <iostream>
#include <sstream>
#include <string>

using namespace FFBEngineTests;

TEST_CASE_TAGGED(test_issue_238_repro_spam, "BugFix", (std::vector<std::string>{"Issue238", "Regression"})) {
    FFBEngine engine;
    InitializeEngine(engine);

    std::stringstream logBuffer;
    Logger::Get().SetTestStream(&logBuffer);

    TelemInfoV01 data = CreateBasicTestTelemetry();
    // Simulate a mismatch between scoring vehicle name and telemetry vehicle name
    const char* scoringVehicleName = "Iron Lynx - Proton #9:ELMS25";
    const char* telemetryVehicleName = "Iron Lynx - Proton #9"; // Slightly different

    StringUtils::SafeCopy(data.mVehicleName, sizeof(data.mVehicleName), telemetryVehicleName);
    const char* vehicleClass = "LMP2_ELMS";

    // Call calculate_force multiple times
    for (int i = 0; i < 5; ++i) {
        engine.calculate_force(&data, vehicleClass, scoringVehicleName, 0.0f, true, 0.0025);
    }

    std::string output = logBuffer.str();

    // Count occurrences of "[FFB] Normalization state reset"
    size_t pos = 0;
    int count = 0;
    std::string target = "[FFB] Normalization state reset";
    while ((pos = output.find(target, pos)) != std::string::npos) {
        count++;
        pos += target.length();
    }

    std::cout << "Occurrences of '" << target << "': " << count << std::endl;

    // It should be 1 if fixed, but will be 5 if broken
    ASSERT_EQ(count, 1);

    Logger::Get().SetTestStream(nullptr);
}

TEST_CASE_TAGGED(test_issue_238_steering_range_warning_spam, "BugFix", (std::vector<std::string>{"Issue238", "Regression"})) {
    FFBEngine engine;
    InitializeEngine(engine);

    std::stringstream logBuffer;
    Logger::Get().SetTestStream(&logBuffer);

    TelemInfoV01 data = CreateBasicTestTelemetry();
    data.mPhysicalSteeringWheelRange = 0.0f; // Invalid range
    const char* vehicleName = "Test Car";
    const char* vehicleClass = "GT3";

    StringUtils::SafeCopy(data.mVehicleName, sizeof(data.mVehicleName), vehicleName);

    // Call calculate_force multiple times
    for (int i = 0; i < 5; ++i) {
        engine.calculate_force(&data, vehicleClass, vehicleName, 0.0f, true, 0.0025);
    }

    std::string output = logBuffer.str();

    // Count occurrences of "[WARNING] Invalid PhysicalSteeringWheelRange"
    size_t pos = 0;
    int count = 0;
    std::string target = "[WARNING] Invalid PhysicalSteeringWheelRange";
    while ((pos = output.find(target, pos)) != std::string::npos) {
        count++;
        pos += target.length();
    }

    std::cout << "Occurrences of '" << target << "': " << count << std::endl;

    // It should be 1 if fixed, but if ResetNormalization is called repeatedly, it will reset m_warned_invalid_range
    ASSERT_EQ(count, 1);

    Logger::Get().SetTestStream(nullptr);
}
