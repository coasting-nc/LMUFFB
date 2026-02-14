#include "test_ffb_common.h"
#include "../src/AsyncLogger.h"
#include <fstream>
#include <thread>

namespace FFBEngineTests {

TEST_CASE(test_surface_type_logging, "AccuracyTools") {
    std::cout << "\nTest: Surface Type Logging" << std::endl;
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
    AsyncLogger::Get().Start(info, "test_logs");
    std::string filename = AsyncLogger::Get().GetFilename();

    // Create mock telemetry with specific surface types
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.05);
    data.mWheel[0].mSurfaceType = 5; // Rumblestrip
    data.mWheel[1].mSurfaceType = 0; // Dry
    data.mDeltaTime = 0.01;

    // Run enough frames to trigger logging (decimation factor is 4)
    // We need at least 4 ticks for one logged frame.
    for (int i = 0; i < 10; i++) {
        data.mElapsedTime = (double)i * 0.01;
        engine.calculate_force(&data);
    }

    // Give some time for the worker thread to write
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop logging to flush
    AsyncLogger::Get().Stop();

    // Read the CSV and verify the columns
    std::ifstream file(filename);
    ASSERT_TRUE(file.is_open());

    std::string line;
    bool found_header = false;
    bool found_data = false;

    while (std::getline(file, line)) {
        // Check for new columns in header
        if (line.find("SurfaceFL,SurfaceFR") != std::string::npos) {
            found_header = true;
        }
        // Check for specific values in data rows
        // Note: The new columns should be after CalcGripRear,GripDelta,dG_dt,dAlpha_dt,SlopeCurrent,SlopeRaw,SlopeNum,SlopeDenom,HoldTimer,InputSlipSmooth,SlopeSmoothed,Confidence
        // Actually I don't know the exact position yet, but let's just search for the values.
        if (line.find("5.0000,0.0000") != std::string::npos) {
            found_data = true;
        }
    }

    std::cout << "  Found Header: " << (found_header ? "Yes" : "No") << std::endl;
    std::cout << "  Found Data (5.0, 0.0): " << (found_data ? "Yes" : "No") << std::endl;

    ASSERT_TRUE(found_header);
    ASSERT_TRUE(found_data);

    file.close();
    std::remove(filename.c_str());
}

} // namespace FFBEngineTests
