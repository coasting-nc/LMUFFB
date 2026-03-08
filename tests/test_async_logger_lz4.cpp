#include "test_ffb_common.h"
#include "../src/AsyncLogger.h"
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <filesystem>

namespace FFBEngineTests {

TEST_CASE_TAGGED(test_logger_lz4_compression, "Diagnostics", (std::vector<std::string>{"Logger", "LZ4"})) {
    std::cout << "\nTest: AsyncLogger LZ4 Compression Verification" << std::endl;

    SessionInfo info = {};
    info.driver_name = "TestDriver";
    info.vehicle_name = "TestCar";
    info.track_name = "TestTrack";
    info.app_version = "0.7.127";

    AsyncLogger& logger = AsyncLogger::Get();
    logger.Stop(); // Ensure clean slate
    logger.EnableCompression(true);

    std::string test_dir = "test_logs_lz4";
    std::filesystem::create_directories(test_dir);

    logger.Start(info, test_dir);

    // Log a bunch of frames with repeating data (highly compressible)
    int frame_count = 1000;
    for (int i = 0; i < frame_count; ++i) {
        LogFrame frame = {};
        frame.timestamp = i * 0.0025;
        frame.speed = 100.0f;
        frame.steering = 0.5f;
        logger.Log(frame);
    }

    // Stop logging will flush everything
    logger.Stop();

    std::string filename = logger.GetFilename();

    // Verify file exists
    ASSERT_TRUE(std::filesystem::exists(filename));

    // Read the file and check for LZ4 blocks
    std::string content;
    {
        std::ifstream file(filename, std::ios::binary);
        content = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    } // file is closed here — Windows locks open files, preventing remove_all later

    size_t marker_pos = content.find("[DATA_START]\n");
    ASSERT_TRUE(marker_pos != std::string::npos);

    // Iterate through blocks
    size_t data_pos = marker_pos + 13;
    uint32_t total_uncompressed_size = 0;
    int block_count = 0;

    while (data_pos + 8 <= content.size()) {
        uint32_t sizes[2];
        memcpy(sizes, &content[data_pos], 8);
        uint32_t compressed_size = sizes[0];
        uint32_t uncompressed_size = sizes[1];

        std::cout << "  Block " << block_count << ": Comp=" << compressed_size << ", Uncomp=" << uncompressed_size << std::endl;

        ASSERT_GT(compressed_size, 0u);
        ASSERT_LT(compressed_size, uncompressed_size);
        ASSERT_EQ(uncompressed_size % sizeof(LogFrame), 0u);

        total_uncompressed_size += uncompressed_size;
        data_pos += 8 + compressed_size;
        block_count++;
    }

    std::cout << "  Total Blocks:            " << block_count << std::endl;
    std::cout << "  Total Uncompressed Size: " << total_uncompressed_size << " bytes" << std::endl;
    std::cout << "  Expected Size:           " << frame_count * sizeof(LogFrame) << " bytes" << std::endl;

    ASSERT_EQ(total_uncompressed_size, (uint32_t)(frame_count * sizeof(LogFrame)));

    // Cleanup — wrap in try/catch so a cleanup failure is diagnostic, not fatal
    try {
        std::filesystem::remove_all(test_dir);
    } catch (const std::exception& e) {
        std::cout << "  [WARNING] Could not clean up test dir '" << test_dir << "': " << e.what() << std::endl;
    }

    // Disable compression for subsequent tests
    logger.EnableCompression(false);
}

} // namespace FFBEngineTests
