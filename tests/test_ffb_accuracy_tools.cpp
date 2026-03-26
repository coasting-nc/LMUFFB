#include "test_ffb_common.h"
#include "../src/logging/AsyncLogger.h"
#include <fstream>
#include <thread>
#include <vector>

namespace FFBEngineTests {

TEST_CASE(test_surface_type_logging, "AccuracyTools") {
    std::cout << "\nTest: Surface Type Logging (Binary)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    // Stop any existing logging
    AsyncLogger::Get().Stop();

    // Setup session info
    SessionInfo info;
    info.vehicle_name = "SurfaceTestCar";
    info.track_name = "SurfaceTestTrack";
    info.app_version = "0.7.39-test";

    // Start logging
    AsyncLogger::Get().EnableCompression(false);
    AsyncLogger::Get().Start(info, "test_logs_accuracy");
    std::string filename = AsyncLogger::Get().GetFilename();

    // Create mock telemetry with specific surface types
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.05);
    data.mWheel[WHEEL_FL].mSurfaceType = 5; // Rumblestrip
    data.mWheel[WHEEL_FR].mSurfaceType = 0; // Dry
    data.mDeltaTime = 0.01;

    // Run enough frames to trigger logging
    for (int i = 0; i < 40; i++) {
        data.mElapsedTime = (double)i * 0.01;
        engine.calculate_force(&data);
    }

    // Give some time for the worker thread to write
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Stop logging to flush
    AsyncLogger::Get().Stop();

    // Read the file and verify
    std::ifstream file(filename, std::ios::binary);
    ASSERT_TRUE(file.is_open());

    std::string line;
    bool found_header = false;
    while (std::getline(file, line)) {
        if (line.find("SurfaceFL,SurfaceFR") != std::string::npos) {
            found_header = true;
        }
        if (line.find("[DATA_START]") != std::string::npos) {
            break;
        }
    }

    std::cout << "  Found Header: " << (found_header ? "Yes" : "No") << std::endl;
    ASSERT_TRUE(found_header);

    // Clear EOF if hit during getline
    file.clear();

    // Verify binary data
    std::vector<LogFrame> read_frames(100);
    file.read(reinterpret_cast<char*>(read_frames.data()), 100 * sizeof(LogFrame));

    size_t bytes_read = (size_t)file.gcount();
    size_t frames_read = bytes_read / sizeof(LogFrame);
    std::cout << "  Bytes Read: " << bytes_read << std::endl;
    std::cout << "  Frames Read: " << frames_read << std::endl;

    bool found_data = false;
    for (size_t i = 0; i < frames_read; ++i) {
        if (std::abs(read_frames[i].surface_type_fl - 5.0f) < 0.001f &&
            std::abs(read_frames[i].surface_type_fr - 0.0f) < 0.001f) {
            found_data = true;
            break;
        }
    }

    if (!found_data && frames_read > 0) {
        std::cout << "  First frame data: FL=" << read_frames[0].surface_type_fl << " FR=" << read_frames[0].surface_type_fr << std::endl;
    }

    ASSERT_TRUE(found_data);

    file.close();
    std::filesystem::remove_all("test_logs_accuracy");
}

} // namespace FFBEngineTests
