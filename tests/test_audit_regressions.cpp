#include "test_ffb_common.h"
#include <fstream>
#include <filesystem>

// Declaration of the helper function from main.cpp
namespace LMUFFB {
void PopulateSessionInfo(SessionInfo& info, const VehicleScoringInfoV01& scoring, const char* trackName, const FFBEngine& engine);
}

namespace FFBEngineTests {

TEST_CASE(test_audit_config_load_validation, "AuditRegressions") {
    FFBEngine engine;

    // 1. Create a config file with malicious/invalid values
    std::string test_config = "test_audit_config.ini";
    {
        std::ofstream f(test_config);
        f << "[System]\n";
        f << "gain=-5.0\n";
        f << "wheelbase_max_nm=0.5\n";
        f << "target_rim_nm=-10.0\n";
        f << "min_force=-1.0\n";

        f << "understeer=5.0\n"; // Should clamp to 2.0
        f << "understeer_gamma=0.05\n"; // Should clamp to 0.1
        f << "torque_source=5\n"; // Should clamp to 1

        f << "oversteer_boost=-1.0\n"; // Should clamp to 0.0
        f << "sop_scale=0.0\n"; // Should clamp to 0.01

        f << "lateral_load_effect=-2.0\n"; // Should clamp to 0.0
        f << "lat_load_transform=10\n"; // Should clamp to 3

        f << "optimal_slip_angle=0.005\n"; // Should reset to 0.10f (legacy)
        f << "grip_smoothing_sensitivity=0.0\n"; // Should clamp to 0.001

        f << "slope_sg_window=2\n"; // Should clamp to 5
        f << "slope_alpha_threshold=0.0\n"; // Should reset to 0.02f (legacy)

        f << "lockup_gain=-1.0\n"; // Should clamp to 0.0
        f << "brake_load_cap=20.0\n"; // Should clamp to 10.0

        f << "vibration_gain=-1.0\n"; // Should clamp to 0.0
        f << "bottoming_method=2\n"; // Should clamp to 1

        f << "gyro_gain=2.0\n"; // Should clamp to 1.0
        f << "rest_api_port=0\n"; // Should clamp to 1

        f << "safety_gain_reduction=1.5\n"; // Should clamp to 1.0
        f << "stutter_threshold=1.0\n"; // Should clamp to 1.01
        f.close();
    }

    // 2. Load the config
    Config::Load(engine, test_config);

    // 3. Verify clamping
    ASSERT_GE(engine.m_general.gain, 0.0f);
    ASSERT_GE(engine.m_general.wheelbase_max_nm, 1.0f);
    ASSERT_GE(engine.m_general.target_rim_nm, 1.0f);
    ASSERT_GE(engine.m_general.min_force, 0.0f);

    ASSERT_LE(engine.m_front_axle.understeer_effect, 2.0f);
    ASSERT_GE(engine.m_front_axle.understeer_gamma, 0.1f);
    ASSERT_LE(engine.m_front_axle.torque_source, 1);

    ASSERT_GE(engine.m_rear_axle.oversteer_boost, 0.0f);
    ASSERT_GE(engine.m_rear_axle.sop_scale, 0.01f);

    ASSERT_GE(engine.m_load_forces.lat_load_effect, 0.0f);
    ASSERT_LE(engine.m_load_forces.lat_load_transform, 3);

    ASSERT_NEAR(engine.m_grip_estimation.optimal_slip_angle, 0.10f, 0.0001);
    ASSERT_GE(engine.m_grip_estimation.grip_smoothing_sensitivity, 0.001f);

    ASSERT_GE(engine.m_slope_detection.sg_window, 5);
    ASSERT_EQ(engine.m_slope_detection.sg_window % 2, 1);
    ASSERT_NEAR(engine.m_slope_detection.alpha_threshold, 0.02f, 0.0001);

    ASSERT_GE(engine.m_braking.lockup_gain, 0.0f);
    ASSERT_LE(engine.m_braking.brake_load_cap, 10.0f);

    ASSERT_GE(engine.m_vibration.vibration_gain, 0.0f);
    ASSERT_LE(engine.m_vibration.bottoming_method, 1);

    ASSERT_LE(engine.m_advanced.gyro_gain, 1.0f);
    ASSERT_GE(engine.m_advanced.rest_api_port, 1);

    ASSERT_LE(engine.m_safety.m_config.gain_reduction, 1.0f);
    ASSERT_GE(engine.m_safety.m_config.stutter_threshold, 1.01f);

    // Cleanup
    std::filesystem::remove(test_config);
}

TEST_CASE(test_audit_logger_header_completeness, "AuditRegressions") {
    SessionInfo info;
    info.driver_name = "AuditTest";
    info.vehicle_name = "TestCar";
    info.vehicle_class = "GT3";
    info.vehicle_brand = "Ferrari";
    info.track_name = "Spa";
    info.app_version = LMUFFB_VERSION;

    // Populate all categories with recognizable values
    info.general.gain = 0.88f;
    info.front_axle.understeer_effect = 1.11f;
    info.rear_axle.sop_effect = 0.22f;
    info.load_forces.lat_load_effect = 0.33f;
    info.load_forces.long_load_effect = 0.44f;
    info.grip_estimation.optimal_slip_angle = 0.55f;
    info.grip_estimation.optimal_slip_ratio = 0.66f;
    info.slope_detection.enabled = true;
    info.slope_detection.sensitivity = 0.77f;
    info.braking.lockup_gain = 0.99f;
    info.vibration.vibration_gain = 1.23f;
    info.advanced.gyro_gain = 0.45f;
    info.safety.gain_reduction = 0.15f;

    AsyncLogger::Get().Stop(); // Ensure clean state
    std::string log_dir = "audit_header_logs";
    AsyncLogger::Get().Start(info, log_dir);

    LogFrame frame = {};
    AsyncLogger::Get().Log(frame);

    AsyncLogger::Get().Stop();

    std::string filename = AsyncLogger::Get().GetFilename();
    ASSERT_FALSE(filename.empty());

    std::ifstream f(filename);
    ASSERT_TRUE(f.is_open());

    std::string line;
    std::vector<std::string> expected_patterns = {
        "# Gain: 0.88",
        "# Understeer Effect: 1.11",
        "# SoP Effect: 0.22",
        "# Lateral Load Effect: 0.33",
        "# Long Load Effect: 0.44",
        "# Optimal Slip Angle: 0.55",
        "# Optimal Slip Ratio: 0.66",
        "# Slope Detection: Enabled",
        "# Slope Sensitivity: 0.77",
        "# Lockup Gain: 0.99",
        "# Vibration Gain: 1.23",
        "# Gyro Gain: 0.45",
        "# Safety Gain Reduction: 0.15"
    };

    std::vector<bool> found(expected_patterns.size(), false);
    while (std::getline(f, line)) {
        for (size_t i = 0; i < expected_patterns.size(); ++i) {
            if (line.find(expected_patterns[i]) != std::string::npos) {
                found[i] = true;
            }
        }
        if (line == "[DATA_START]") break;
    }

    for (size_t i = 0; i < expected_patterns.size(); ++i) {
        if (!found[i]) {
            std::string msg = "Missing expected header pattern: " + expected_patterns[i];
            FAIL_TEST(msg);
        }
    }

    f.close();
    std::filesystem::remove_all(log_dir);
}

TEST_CASE(test_audit_populate_session_info, "AuditRegressions") {
    FFBEngine engine;
    VehicleScoringInfoV01 scoring = {};
    StringUtils::SafeCopy(scoring.mVehicleName, 64, "Ferrari 296 GT3");
    StringUtils::SafeCopy(scoring.mVehicleClass, 64, "GT3");
    const char* trackName = "Spa-Francorchamps";

    // Initialize engine with unique values in all 10 categories
    engine.m_general.gain = 0.123f;
    engine.m_front_axle.understeer_effect = 0.456f;
    engine.m_rear_axle.sop_effect = 0.789f;
    engine.m_load_forces.lat_load_effect = 1.011f;
    engine.m_grip_estimation.optimal_slip_angle = 0.121f;
    engine.m_slope_detection.sensitivity = 0.314f;
    engine.m_braking.lockup_gain = 0.516f;
    engine.m_vibration.vibration_gain = 0.718f;
    engine.m_advanced.gyro_gain = 0.920f;
    engine.m_safety.m_config.gain_reduction = 0.222f;

    SessionInfo info;
    LMUFFB::PopulateSessionInfo(info, scoring, trackName, engine);

    // Verify all 10 categories correctly populated
    ASSERT_NEAR(info.general.gain, 0.123f, 0.0001);
    ASSERT_NEAR(info.front_axle.understeer_effect, 0.456f, 0.0001);
    ASSERT_NEAR(info.rear_axle.sop_effect, 0.789f, 0.0001);
    ASSERT_NEAR(info.load_forces.lat_load_effect, 1.011f, 0.0001);
    ASSERT_NEAR(info.grip_estimation.optimal_slip_angle, 0.121f, 0.0001);
    ASSERT_NEAR(info.slope_detection.sensitivity, 0.314f, 0.0001);
    ASSERT_NEAR(info.braking.lockup_gain, 0.516f, 0.0001);
    ASSERT_NEAR(info.vibration.vibration_gain, 0.718f, 0.0001);
    ASSERT_NEAR(info.advanced.gyro_gain, 0.920f, 0.0001);
    ASSERT_NEAR(info.safety.gain_reduction, 0.222f, 0.0001);

    // Verify metadata
    ASSERT_EQ_STR(info.vehicle_name.c_str(), "Ferrari 296 GT3");
    ASSERT_EQ_STR(info.track_name.c_str(), "Spa-Francorchamps");
    ASSERT_EQ_STR(info.app_version.c_str(), LMUFFB_VERSION);
}

}
