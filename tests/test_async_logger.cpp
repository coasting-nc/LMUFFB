#include "test_ffb_common.h"
#include "../src/AsyncLogger.h"
#include <thread>
#include <chrono>

namespace FFBEngineTests {

TEST_CASE_TAGGED(test_logger_start_stop, "Diagnostics", {"Logger"}) {
    std::cout << "\nTest: AsyncLogger Start/Stop" << std::endl;
    AsyncLogger::Get().Stop(); // Ensure clean slate

    // Initially not logging
    ASSERT_TRUE(AsyncLogger::Get().IsLogging() == false);

    // Setup session info
    SessionInfo info;
    info.driver_name = "TestDriver";
    info.vehicle_name = "TestCar";
    info.track_name = "TestTrack";
    info.app_version = "0.7.3-test";
    
    // Start logging
    AsyncLogger::Get().Start(info, "test_logs");
    
    ASSERT_TRUE(AsyncLogger::Get().IsLogging() == true);
    
    // Stop logging
    AsyncLogger::Get().Stop();
    
    ASSERT_TRUE(AsyncLogger::Get().IsLogging() == false);
}

TEST_CASE_TAGGED(test_logger_frame_logging, "Diagnostics", {"Logger"}) {
    std::cout << "\nTest: AsyncLogger Frame Logging & Decimation" << std::endl;
    AsyncLogger::Get().Stop();
    
    SessionInfo info;
    info.driver_name = "TestDriver";
    info.vehicle_name = "TestCarFrame";
    info.track_name = "TestTrack";
    info.app_version = "0.7.3-test";
    
    AsyncLogger::Get().Start(info, "test_logs");
    
    LogFrame frame = {};
    // Decimation is 4. So we need 40 ticks to get 10 frames.
    for (int i = 0; i < 40; i++) {
        frame.timestamp = i * 0.01;
        AsyncLogger::Get().Log(frame);
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Allow worker some time
    AsyncLogger::Get().Stop();
    
    // Expected: 10 frames.
    std::cout << "Logged Frames: " << AsyncLogger::Get().GetFrameCount() << std::endl;
    ASSERT_NEAR((double)AsyncLogger::Get().GetFrameCount(), 10.0, 0.1); 
}

TEST_CASE_TAGGED(test_logger_marker, "Diagnostics", {"Logger"}) {
    std::cout << "\nTest: AsyncLogger Marker Bypass" << std::endl;
    AsyncLogger::Get().Stop();
    
    SessionInfo info;
    info.driver_name = "TestDriver";
    info.vehicle_name = "TestCarMarker";
    info.track_name = "TestTrack";
    info.app_version = "0.7.3-test";
    
    AsyncLogger::Get().Start(info, "test_logs");
    
    // Tick 1 (decimation counter 1): Not logged
    LogFrame frame = {};
    AsyncLogger::Get().Log(frame); 
    
    // Set Marker
    AsyncLogger::Get().SetMarker();
    
    // Tick 2 (decimation counter 2): normally skipped, but marker -> Logged!
    AsyncLogger::Get().Log(frame);
    
    AsyncLogger::Get().Stop();
    
    // Start + 1 frame logged. Frame count should be 1.
    // Wait, decimation counter init is 0.
    // Tick 1: ++c = 1. Return.
    // Tick 2: ++c = 2. PendingMarker=true. Log! c=0.
    std::cout << "Logged Frames (Marker): " << AsyncLogger::Get().GetFrameCount() << std::endl;
    ASSERT_GE(AsyncLogger::Get().GetFrameCount(), 1);
}

TEST_CASE_TAGGED(test_logger_filename_sanitization, "Diagnostics", {"Logger"}) {
    std::cout << "\nTest: AsyncLogger Filename Sanitization" << std::endl;
    AsyncLogger::Get().Stop();
    
    SessionInfo info;
    info.driver_name = "TestDriver";
    info.vehicle_name = "Porsche 911 GT3*R?";  // Contains invalid chars
    info.track_name = "Spa/Belgium<Test>";     // Contains invalid chars
    info.app_version = "0.7.9-test";
    
    AsyncLogger::Get().Start(info, "test_logs");
    
    std::string full_path = AsyncLogger::Get().GetFilename();
    std::cout << "Generated filename: " << full_path << std::endl;
    
    // Get just the filename part for character validation
    std::string filename = std::filesystem::path(full_path).filename().string();
    
    // Verify no invalid Windows filename characters in the filename component
    ASSERT_TRUE(filename.find('*') == std::string::npos);
    ASSERT_TRUE(filename.find('?') == std::string::npos);
    ASSERT_TRUE(filename.find('/') == std::string::npos);
    ASSERT_TRUE(filename.find('\\') == std::string::npos);
    ASSERT_TRUE(filename.find('<') == std::string::npos);
    ASSERT_TRUE(filename.find('>') == std::string::npos);
    ASSERT_TRUE(filename.find('|') == std::string::npos);
    ASSERT_TRUE(filename.find('"') == std::string::npos);
    
    // Verify underscores were used as replacements
    ASSERT_TRUE(filename.find('_') != std::string::npos);
    
    AsyncLogger::Get().Stop();
}

TEST_CASE_TAGGED(test_logger_performance_impact, "Diagnostics", {"Logger"}) {
    std::cout << "\nTest: AsyncLogger Performance Impact" << std::endl;
    AsyncLogger::Get().Stop();
    
    SessionInfo info;
    info.driver_name = "PerfTest";
    info.vehicle_name = "TestCar";
    info.track_name = "TestTrack";
    info.app_version = "0.7.9-test";
    
    const int iterations = 1000;
    LogFrame frame = {};
    frame.timestamp = 1.0;
    
    // Measure without logging
    auto start_no_log = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        // Simulate minimal work (just the check)
        if (AsyncLogger::Get().IsLogging()) {
            AsyncLogger::Get().Log(frame);
        }
    }
    auto end_no_log = std::chrono::high_resolution_clock::now();
    auto duration_no_log = std::chrono::duration_cast<std::chrono::microseconds>(end_no_log - start_no_log).count();
    
    // Measure with logging
    AsyncLogger::Get().Start(info, "test_logs");
    auto start_with_log = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        if (AsyncLogger::Get().IsLogging()) {
            AsyncLogger::Get().Log(frame);
        }
    }
    auto end_with_log = std::chrono::high_resolution_clock::now();
    auto duration_with_log = std::chrono::duration_cast<std::chrono::microseconds>(end_with_log - start_with_log).count();
    
    AsyncLogger::Get().Stop();
    
    double overhead_per_call = (double)(duration_with_log - duration_no_log) / iterations;
    std::cout << "  No logging: " << duration_no_log << " μs total" << std::endl;
    std::cout << "  With logging: " << duration_with_log << " μs total" << std::endl;
    std::cout << "  Overhead per call: " << overhead_per_call << " μs" << std::endl;
    
    // Verify overhead is less than 10 microseconds per call (as per plan)
    ASSERT_TRUE(overhead_per_call < 10.0);
}

} // namespace FFBEngineTests
