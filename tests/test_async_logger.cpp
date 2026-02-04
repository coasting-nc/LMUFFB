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

} // namespace FFBEngineTests
