#include "test_ffb_common.h"
#include "../src/AsyncLogger.h"
#include <filesystem>
#include <iostream>

namespace FFBEngineTests {

TEST_CASE_TAGGED(test_issue_257_log_filename_format, "Issue_257", (std::vector<std::string>{"Logger", "Issue_257"})) {
    std::cout << "\nTest: Issue #257 - Log Filename Format (Brand and Class)" << std::endl;
    AsyncLogger::Get().Stop(); // Ensure clean slate

    // Setup session info
    SessionInfo info;
    info.driver_name = "TestDriver";
    info.vehicle_name = "Ferrari 499P - #51 AF Corse";
    info.vehicle_class = "Hypercar";
    info.vehicle_brand = "Ferrari";
    info.track_name = "Le Mans";
    info.app_version = "0.7.133-test";

    // Start logging
    AsyncLogger::Get().Start(info, "test_logs_257");

    std::string full_path = AsyncLogger::Get().GetFilename();
    std::cout << "Generated filename: " << full_path << std::endl;

    std::string filename = std::filesystem::path(full_path).filename().string();

    // Format should be: lmuffb_log_<timestamp>_<brand>_<class>_<track>.bin
    // Example: lmuffb_log_2026-03-05_12-34-56_Ferrari_Hypercar_Le_Mans.bin

    ASSERT_TRUE(filename.find("lmuffb_log_") == 0);
    ASSERT_TRUE(filename.find("_Ferrari_Hypercar_Le_Mans.bin") != std::string::npos);
    ASSERT_TRUE(filename.find("Ferrari_499P") == std::string::npos); // Should NOT contain vehicle_name

    AsyncLogger::Get().Stop();

    // Cleanup
    if (std::filesystem::exists(full_path)) {
        std::filesystem::remove(full_path);
    }
}

} // namespace FFBEngineTests
