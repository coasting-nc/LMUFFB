#include "test_ffb_common.h"
#include "../src/AsyncLogger.h"
#include <fstream>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace FFBEngineTests {

TEST_CASE(test_async_logger_binary_integrity, "Logging") {
    AsyncLogger& logger = AsyncLogger::Get();

    // Cleanup any existing test logs
    std::string test_dir = "test_logs_binary";
    if (fs::exists(test_dir)) fs::remove_all(test_dir);
    fs::create_directories(test_dir);

    SessionInfo info;
    info.driver_name = "Test Driver";
    info.vehicle_name = "Test Car";
    info.track_name = "Test Track";
    info.app_version = "0.7.126";
    info.gain = 1.0f;
    info.understeer_effect = 0.5f;
    info.sop_effect = 0.5f;
    info.slope_enabled = true;
    info.slope_sensitivity = 0.5f;
    info.slope_threshold = -0.3f;
    info.slope_alpha_threshold = 0.01f;
    info.slope_decay_rate = 0.1f;
    info.torque_passthrough = false;

    logger.Start(info, test_dir);
    std::string filename = logger.GetFilename();
    ASSERT_TRUE(filename.find(".bin") != std::string::npos);

    // Log 10 frames with recognizable data
    std::vector<LogFrame> sent_frames;
    for (int i = 0; i < 10; ++i) {
        LogFrame frame = {};
        frame.timestamp = 100.0 + i * 0.0025; // 400Hz
        frame.delta_time = 0.0025;
        frame.speed = 50.0f + i;
        frame.steering = (float)i / 10.0f;
        frame.clipping = (i % 2 == 0) ? 1 : 0;
        frame.marker = (i == 5) ? 1 : 0;

        logger.Log(frame);
        sent_frames.push_back(frame);
    }

    // Wait for worker thread to flush
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    logger.Stop();

    // Verify file exists
    ASSERT_TRUE(fs::exists(filename));

    // Read file and verify
    std::ifstream file(filename, std::ios::binary);
    ASSERT_TRUE(file.is_open());

    // 1. Verify Text Header
    std::string line;
    bool found_marker = false;
    while (std::getline(file, line)) {
        if (line.find("[DATA_START]") != std::string::npos) {
            found_marker = true;
            break;
        }
        // Verify some metadata
        if (line.find("Driver: Test Driver") != std::string::npos) {
            g_tests_passed++;
        }
        if (line.find("Vehicle: Test Car") != std::string::npos) {
            g_tests_passed++;
        }
    }
    ASSERT_TRUE(found_marker);

    // 2. Verify Binary Data
    // After std::getline, the file pointer should be at the start of binary data
    std::vector<LogFrame> read_frames(10);
    file.read(reinterpret_cast<char*>(read_frames.data()), 10 * sizeof(LogFrame));

    ASSERT_EQ((size_t)file.gcount(), 10 * sizeof(LogFrame));

    for (int i = 0; i < 10; ++i) {
        ASSERT_NEAR(read_frames[i].timestamp, sent_frames[i].timestamp, 0.0001);
        ASSERT_NEAR(read_frames[i].speed, sent_frames[i].speed, 0.0001f);
        ASSERT_NEAR(read_frames[i].steering, sent_frames[i].steering, 0.0001f);
        ASSERT_EQ(read_frames[i].clipping, sent_frames[i].clipping);
        ASSERT_EQ(read_frames[i].marker, sent_frames[i].marker);
    }

    file.close();

    // Cleanup
    fs::remove_all(test_dir);
}

TEST_CASE(test_log_frame_packing, "Logging") {
    // Verify that LogFrame is packed correctly (no padding)
    ASSERT_EQ(sizeof(LogFrame), (size_t)238);
}

} // namespace FFBEngineTests
