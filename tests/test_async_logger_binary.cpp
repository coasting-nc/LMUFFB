#include "test_ffb_common.h"
#include "../src/logging/AsyncLogger.h"
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
    info.general.gain = 1.0f;
    info.front_axle.understeer_effect = 0.5f;
    info.sop_effect = 0.5f;
    info.slope_enabled = true;
    info.slope_sensitivity = 0.5f;
    info.slope_threshold = -0.3f;
    info.slope_alpha_threshold = 0.01f;
    info.slope_decay_rate = 0.1f;
    info.front_axle.torque_passthrough = false;

    logger.EnableCompression(false);
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
    // v0.7.129 Expanded augmented struct
    // double(8)*2 + float(4)*123 + uint8(1)*3 = 16 + 492 + 3 = 511 bytes
    // timestamp(8), dt(8) = 16
    // PROCESSED: speed, lat, long, yaw, steer, throttle, brake (7) = 28
    // RAW: steer, throttle, brake, lat, long, game_yaw, game_shaft, game_gen (8) = 32
    // RAW WHEELS: load(4), slip_lat(4), slip_long(4), ride(4), susp_def(4), susp_force(4), brake_pres(4), rot(4) (32) = 128
    // ALGO: slip_angle(4), slip_ratio(4), grip(4), load(4), ride(4), susp_def(4) (24) = 96
    // ALGO CALC: calc_slip_angle(2), calc_grip(2), grip_delta(1), calc_rear_lat_force(1) (6) = 24
    // ALGO MISC: smoothed_yaw, lat_load_norm (2) = 8
    // SLOPE: dG_dt, dAlpha_dt, slope_current, slope_raw, slope_num, slope_denom, hold, input_slip, slope_smooth, confidence (10) = 40
    // SLOPE MISC: surface_fl/fr, slope_torque, slew_lat_g (4) = 16
    // SETTINGS: session_peak, long_load, structural_mult, vibration_mult, steer_angle, steer_range, debug_freq, tire_radius (8) = 32
    // FFB COMPONENTS: total, base, us_drop, os_boost, sop, rear_torque, scrub, yaw_kick, gyro, stat_damping, road, slide, lockup, spin, bottoming, abs, soft_lock (17) = 68
    // NEW CHANNELS: extrapolated_yaw_accel, derived_yaw_accel (2) = 8
    // FFB MISC: shaft, gen, grip_factor, speed_gate, load_peak (5) = 20
    // DIAGNOSTIC: approx_load_fl/fr/rl/rr (4) = 16
    // SYSTEM: physics_rate (1) = 4
    // SYSTEM PACKED: clipping(1), warn_bits(1), marker(1) = 3
    // Total = 16 + 28 + 32 + 128 + 96 + 24 + 8 + 40 + 16 + 32 + 68 + 8 + 20 + 16 + 4 + 3 = 539 bytes

    std::cout << "sizeof(LogFrame): " << sizeof(LogFrame) << std::endl;
    ASSERT_EQ(sizeof(LogFrame), (size_t)539);
}

} // namespace FFBEngineTests
