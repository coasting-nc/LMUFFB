#include "test_ffb_common.h"
#include "../src/logging/Logger.h"
#include <filesystem>
#include <thread>
#include <chrono>
#include <fstream>
#include <set>

namespace FFBEngineTests {

TEST_CASE_TAGGED(test_issue_312_logger_uniqueness, "Diagnostics", std::vector<std::string>({"Logger", "Issue312"})) {
    std::cout << "\nTest: Logger Timestamp Uniqueness (Issue #312)" << std::endl;

    std::string test_dir = "test_logs_issue_312";
    if (std::filesystem::exists(test_dir)) {
        std::filesystem::remove_all(test_dir);
    }

    // Call Init twice with a small delay to ensure different timestamps (if resolution is seconds)
    // Actually, our timestamp has seconds resolution, so we need to wait 1s or mock the time.
    // Let's wait 1.1s to be sure.

    Logger::Get().Init("debug.log", test_dir);
    std::string file1 = Logger::Get().GetFilename();
    Logger::Get().Log("First session log");

    std::cout << "  File 1: " << file1 << std::endl;
    ASSERT_TRUE(std::filesystem::exists(file1));

    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    Logger::Get().Init("debug.log", test_dir);
    std::string file2 = Logger::Get().GetFilename();
    Logger::Get().Log("Second session log");

    std::cout << "  File 2: " << file2 << std::endl;
    ASSERT_TRUE(std::filesystem::exists(file2));

    ASSERT_NE(file1, file2);

    // Verify both files exist and have different content
    ASSERT_TRUE(std::filesystem::exists(file1));
    ASSERT_TRUE(std::filesystem::exists(file2));

    // Clean up
    Logger::Get().Close();
}

TEST_CASE_TAGGED(test_issue_312_logger_directory_creation, "Diagnostics", std::vector<std::string>({"Logger", "Issue312"})) {
    std::cout << "\nTest: Logger Directory Creation (Issue #312)" << std::endl;

    std::string nested_dir = "test_logs_nested/debug/session";
    if (std::filesystem::exists("test_logs_nested")) {
        std::filesystem::remove_all("test_logs_nested");
    }

    Logger::Get().Init("crash.log", nested_dir);
    std::string filename = Logger::Get().GetFilename();

    std::cout << "  Target file: " << filename << std::endl;
    ASSERT_TRUE(std::filesystem::exists(nested_dir));
    ASSERT_TRUE(std::filesystem::exists(filename));

    Logger::Get().Log("Testing nested directory creation");

    Logger::Get().Close();
}

TEST_CASE_TAGGED(test_issue_312_logger_extension_handling, "Diagnostics", std::vector<std::string>({"Logger", "Issue312"})) {
    std::cout << "\nTest: Logger Extension Handling (Issue #312)" << std::endl;

    std::string test_dir = "test_logs_ext";

    // Test with .txt extension
    Logger::Get().Init("my_debug.txt", test_dir);
    std::string file_txt = Logger::Get().GetFilename();
    std::cout << "  File with .txt: " << file_txt << std::endl;
    ASSERT_TRUE(file_txt.find(".txt") != std::string::npos);
    ASSERT_TRUE(file_txt.find(".log") == std::string::npos);

    // Test with no extension
    Logger::Get().Init("raw_log", test_dir);
    std::string file_raw = Logger::Get().GetFilename();
    std::cout << "  File with no ext: " << file_raw << std::endl;
    ASSERT_TRUE(file_raw.find(".log") != std::string::npos);

    Logger::Get().Close();
}

} // namespace FFBEngineTests
