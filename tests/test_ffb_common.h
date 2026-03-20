// test_ffb_common.h
#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <random>
#include <sstream>
#include <functional>

#include "../src/ffb/FFBEngine.h"
#include "../src/io/lmu_sm_interface/InternalsPlugin.hpp"
#include "../src/io/lmu_sm_interface/LmuSharedMemoryWrapper.h"
#include "../src/core/Config.h"
#include "../src/logging/Logger.h"
#include "../src/io/GameConnector.h"
#include "../src/utils/StringUtils.h"
#include "../src/io/RestApiProvider.h"
#include "test_performance_types.h"

class RestApiProviderTestAccess {
public:
    static void SetFallbackRange(float val) {
        RestApiProvider::Get().m_fallbackRangeDeg = val;
    }
    static void ResetRequestState() {
        RestApiProvider::Get().m_isRequesting = false;
    }
    static float ParseSteeringLock(RestApiProvider& p, const std::string& json) {
        return p.ParseSteeringLock(json);
    }
};

namespace FFBEngineTests {

// --- Test Counters (defined in test_ffb_common.cpp) ---
extern int g_tests_passed;
extern int g_tests_failed_DO_NOT_USE_DIRECTLY_USE_FAIL_TEST_MACRO;
extern int g_test_cases_run;
extern int g_test_cases_passed;
extern int g_test_cases_failed;
extern std::vector<std::string> g_failure_log; // New
extern std::string g_current_test_name; // Set by Run() before each test

extern std::vector<TestDuration> g_test_durations;

// --- Assert Macros ---

#define FAIL_TEST(msg) do { \
    std::stringstream ss_fail; \
    ss_fail << "[FAIL] " << FFBEngineTests::g_current_test_name << ": " << msg << " (" << __FILE__ << ":" << __LINE__ << ")"; \
    std::cout << ss_fail.str() << std::endl; \
    FFBEngineTests::g_failure_log.push_back(ss_fail.str()); \
    FFBEngineTests::g_tests_failed_DO_NOT_USE_DIRECTLY_USE_FAIL_TEST_MACRO++; \
} while(0)

// Passing assertions are silent. Failing assertions print [FAIL] with the
// current test name, condition details, and source location.
#define ASSERT_TRUE(condition) \
do { \
        if (condition) { \
            FFBEngineTests::g_tests_passed++; \
        } else { \
            std::stringstream ss_fail; \
            ss_fail << "[FAIL] " << FFBEngineTests::g_current_test_name << ": " << #condition << " is false" \
                      << " (" << __FILE__ << ":" << __LINE__ << ")"; \
            std::cout << ss_fail.str() << std::endl; \
            FFBEngineTests::g_failure_log.push_back(ss_fail.str()); \
            FFBEngineTests::g_tests_failed_DO_NOT_USE_DIRECTLY_USE_FAIL_TEST_MACRO++; \
        } \
} while(0)

#define ASSERT_FALSE(condition) \
do { \
        if (!(condition)) { \
            FFBEngineTests::g_tests_passed++; \
        } else { \
            std::stringstream ss_fail; \
            ss_fail << "[FAIL] " << FFBEngineTests::g_current_test_name << ": " << #condition << " is true (expected false)" \
                      << " (" << __FILE__ << ":" << __LINE__ << ")"; \
            std::cout << ss_fail.str() << std::endl; \
            FFBEngineTests::g_failure_log.push_back(ss_fail.str()); \
            FFBEngineTests::g_tests_failed_DO_NOT_USE_DIRECTLY_USE_FAIL_TEST_MACRO++; \
        } \
} while(0)

#define ASSERT_NEAR(a, b, epsilon) \
do { \
        if (std::abs((a) - (b)) < (epsilon)) { \
            FFBEngineTests::g_tests_passed++; \
        } else { \
            std::stringstream ss_fail; \
            ss_fail << "[FAIL] " << FFBEngineTests::g_current_test_name << ": " << #a << " (" << (a) << ") not near " \
                      << #b << " (" << (b) << ") within " << (epsilon) \
                      << " (" << __FILE__ << ":" << __LINE__ << ")"; \
            std::cout << ss_fail.str() << std::endl; \
            FFBEngineTests::g_failure_log.push_back(ss_fail.str()); \
            FFBEngineTests::g_tests_failed_DO_NOT_USE_DIRECTLY_USE_FAIL_TEST_MACRO++; \
        } \
} while(0)

#define ASSERT_GT(a, b) \
do { \
        if ((a) > (b)) { \
            FFBEngineTests::g_tests_passed++; \
        } else { \
            std::stringstream ss_fail; \
            ss_fail << "[FAIL] " << FFBEngineTests::g_current_test_name << ": " << #a << " (" << (a) << ") <= " \
                      << #b << " (" << (b) << ")" \
                      << " (" << __FILE__ << ":" << __LINE__ << ")"; \
            std::cout << ss_fail.str() << std::endl; \
            FFBEngineTests::g_failure_log.push_back(ss_fail.str()); \
            FFBEngineTests::g_tests_failed_DO_NOT_USE_DIRECTLY_USE_FAIL_TEST_MACRO++; \
        } \
} while(0)

#define ASSERT_EQ(a, b) \
do { \
        if ((a) == (b)) { \
            FFBEngineTests::g_tests_passed++; \
        } else { \
            std::stringstream ss_fail; \
            ss_fail << "[FAIL] " << FFBEngineTests::g_current_test_name << ": " << #a << " (" << (a) << ") != " \
                      << #b << " (" << (b) << ")" \
                      << " (" << __FILE__ << ":" << __LINE__ << ")"; \
            std::cout << ss_fail.str() << std::endl; \
            FFBEngineTests::g_failure_log.push_back(ss_fail.str()); \
            FFBEngineTests::g_tests_failed_DO_NOT_USE_DIRECTLY_USE_FAIL_TEST_MACRO++; \
        } \
} while(0)

#define ASSERT_GE(a, b) \
do { \
        if ((a) >= (b)) { \
            FFBEngineTests::g_tests_passed++; \
        } else { \
            std::stringstream ss_fail; \
            ss_fail << "[FAIL] " << FFBEngineTests::g_current_test_name << ": " << #a << " (" << (a) << ") < " \
                      << #b << " (" << (b) << ")" \
                      << " (" << __FILE__ << ":" << __LINE__ << ")"; \
            std::cout << ss_fail.str() << std::endl; \
            FFBEngineTests::g_failure_log.push_back(ss_fail.str()); \
            FFBEngineTests::g_tests_failed_DO_NOT_USE_DIRECTLY_USE_FAIL_TEST_MACRO++; \
        } \
} while(0)

#define ASSERT_LE(a, b) \
do { \
        if ((a) <= (b)) { \
            FFBEngineTests::g_tests_passed++; \
        } else { \
            std::stringstream ss_fail; \
            ss_fail << "[FAIL] " << FFBEngineTests::g_current_test_name << ": " << #a << " (" << (a) << ") > " \
                      << #b << " (" << (b) << ")" \
                      << " (" << __FILE__ << ":" << __LINE__ << ")"; \
            std::cout << ss_fail.str() << std::endl; \
            FFBEngineTests::g_failure_log.push_back(ss_fail.str()); \
            FFBEngineTests::g_tests_failed_DO_NOT_USE_DIRECTLY_USE_FAIL_TEST_MACRO++; \
        } \
} while(0)

#define ASSERT_LT(a, b) \
do { \
        if ((a) < (b)) { \
            FFBEngineTests::g_tests_passed++; \
        } else { \
            std::stringstream ss_fail; \
            ss_fail << "[FAIL] " << FFBEngineTests::g_current_test_name << ": " << #a << " (" << (a) << ") >= " \
                      << #b << " (" << (b) << ")" \
                      << " (" << __FILE__ << ":" << __LINE__ << ")"; \
            std::cout << ss_fail.str() << std::endl; \
            FFBEngineTests::g_failure_log.push_back(ss_fail.str()); \
            FFBEngineTests::g_tests_failed_DO_NOT_USE_DIRECTLY_USE_FAIL_TEST_MACRO++; \
        } \
} while(0)

#define ASSERT_EQ_STR(a, b) \
do { \
        if (std::string(a) == std::string(b)) { \
            FFBEngineTests::g_tests_passed++; \
        } else { \
            std::stringstream ss_fail; \
            ss_fail << "[FAIL] " << FFBEngineTests::g_current_test_name << ": " << #a << " (\"" << (a) << "\") != " \
                      << #b << " (\"" << (b) << "\")" \
                      << " (" << __FILE__ << ":" << __LINE__ << ")"; \
            std::cout << ss_fail.str() << std::endl; \
            FFBEngineTests::g_failure_log.push_back(ss_fail.str()); \
            FFBEngineTests::g_tests_failed_DO_NOT_USE_DIRECTLY_USE_FAIL_TEST_MACRO++; \
        } \
} while(0)

#define ASSERT_NE(a, b) \
do { \
        if ((a) != (b)) { \
            FFBEngineTests::g_tests_passed++; \
        } else { \
            std::stringstream ss_fail; \
            ss_fail << "[FAIL] " << FFBEngineTests::g_current_test_name << ": " << #a << " (" << (a) << ") == " \
                      << #b << " (" << (b) << ")" \
                      << " (" << __FILE__ << ":" << __LINE__ << ")"; \
            std::cout << ss_fail.str() << std::endl; \
            FFBEngineTests::g_failure_log.push_back(ss_fail.str()); \
            FFBEngineTests::g_tests_failed_DO_NOT_USE_DIRECTLY_USE_FAIL_TEST_MACRO++; \
        } \
} while(0)

// --- Test Constants ---
const int FILTER_SETTLING_FRAMES = 40;

// --- Test Tagging System ---
// Global tag filter (set via command line arguments)
extern std::vector<std::string> g_tag_filter;
extern std::vector<std::string> g_name_filter; // New: for --filter
extern std::vector<std::string> g_tag_exclude;
extern std::vector<std::string> g_category_filter;
extern bool g_enable_tag_filtering;

// Tag checking helper
inline bool ShouldRunTest(const std::string& test_name, const std::vector<std::string>& test_tags, const std::string& category) {
    if (!g_enable_tag_filtering) return true;

    // Name or Tag Filter check (Substring match on test name or exact tag match)
    if (!g_name_filter.empty() || !g_tag_filter.empty()) {
        bool match = false;
        if (!g_name_filter.empty()) {
            for (const auto& filter : g_name_filter) {
                if (test_name.find(filter) != std::string::npos) {
                    match = true;
                    break;
                }
            }
        }
        if (!match && !g_tag_filter.empty()) {
            for (const auto& filter_tag : g_tag_filter) {
                for (const auto& test_tag : test_tags) {
                    if (test_tag == filter_tag) {
                        match = true;
                        break;
                    }
                }
                if (match) break;
            }
        }
        if (!match) return false;
    }
    
    // Category filter (if specified)
    if (!g_category_filter.empty()) {
        bool category_match = false;
        for (const auto& cat : g_category_filter) {
            if (cat == category) {
                category_match = true;
                break;
            }
        }
        if (!category_match) return false;
    }
    
    // Tag exclude filter (if specified)
    if (!g_tag_exclude.empty()) {
        for (const auto& exclude_tag : g_tag_exclude) {
            for (const auto& test_tag : test_tags) {
                if (test_tag == exclude_tag) return false;
            }
        }
    }
    
    // Tag include filter (if specified)
    if (!g_tag_filter.empty()) {
        for (const auto& filter_tag : g_tag_filter) {
            for (const auto& test_tag : test_tags) {
                if (test_tag == filter_tag) return true;
            }
        }
        return false; // No matching tags found
    }
    
    return true; // No filters, run all tests
}

// Parse command line arguments for tag filtering
void ParseTagArguments(int argc, char* argv[]);

// --- Helper Functions ---
TelemInfoV01 CreateBasicTestTelemetry(double speed = 20.0, double slip_angle = 0.0);
void InitializeEngine(FFBEngine& engine);

// Time-advancement helpers for the 400Hz FFB pipeline (Issue #397).
//
// PREFERRED USAGE GUIDELINES (S-2):
//
//   PumpEngineTime(engine, data, N_seconds)
//     - General-purpose: runs N seconds of 400Hz ticks, advancing mElapsedTime
//       every 10ms to simulate 100Hz telemetry correctly.
//     - Use this for most tests: interpolator ramp-up (>= 0.015s), settling a
//       specific effect, or advancing time to check timer/decay values.
//     - AVOID the inline pattern: for(int i=0;i<4;i++) calculate_force(dt=0.0025)
//       unless you specifically need to capture transients *within* a single
//       100Hz game frame (i.e., the test cares about sub-frame behaviour).
//
//   PumpEngineSteadyState(engine, data)
//     - Runs 3 seconds; ensures all filters and decay paths are fully settled.
//     - Use when the test needs a clean, filter-settled baseline BEFORE exercising
//       the actual behaviour under test (e.g. test_gyro_damping Direction and
//       Speed-Scaling sub-cases, slope no-understeer tests, etc.).
//     - Prefer PumpEngineTime(engine, data, 0.5) for fast-converging effects
//       (LPF, shaft-torque HW) to avoid unnecessary CI slowdown.
double PumpEngineTime(FFBEngine& engine, TelemInfoV01& data, double time_to_advance_s);
void PumpEngineSteadyState(FFBEngine& engine, TelemInfoV01& data);

// Orientation Matrix Helper (v0.7.121)
// Verifies that a physical scenario (e.g. Right Turn) produces the correct
// internal signal signs and final FFB output direction.
struct OrientationScenario {
    double lat_accel_x; // Game coords (+X = Left)
    double fl_load;
    double fr_load;
    const char* description;
};

void VerifyOrientation(FFBEngine& engine, const OrientationScenario& scenario, float expected_sop_sign, float expected_total_ffb_sign);

bool IsInLog(const std::string& filename, const std::string& pattern);
int CountInLog(const std::string& filename, const std::string& pattern);

void Run(); // Main runner

// --- Friend Access for Testing ---
class GameConnectorTestAccessor {
public:
    static void Reset(::GameConnector& gc);
    static void SetSharedMem(::GameConnector& gc, struct SharedMemoryLayout* layout);
    static void SetSessionActive(::GameConnector& gc, bool val);
    static void SetInRealtime(::GameConnector& gc, bool val);
    static void SetSessionType(::GameConnector& gc, long val);
    static void SetGamePhase(::GameConnector& gc, unsigned char val);
    static void SetPlayerControl(::GameConnector& gc, signed char val);
    static void InjectTransitions(::GameConnector& gc, const struct SharedMemoryObjectOut& data);
};

class FFBEngineTestAccess {
public:
    static bool HasWarnings(const FFBEngine& engine) {
        return engine.m_warned_load || engine.m_warned_grip || engine.m_warned_dt;
    }
    static int GetMissingLoadFrames(const FFBEngine& e) { return e.m_missing_load_frames; }
    static bool GetWarnedLoad(const FFBEngine& e) { return e.m_warned_load; }
    static int GetMissingVertDeflectionFrames(const FFBEngine& e) { return e.m_missing_vert_deflection_frames; }
    static bool GetWarnedVertDeflection(const FFBEngine& e) { return e.m_warned_vert_deflection; }
    static bool GetWarnedSuspForce(const FFBEngine& e) { return e.m_warned_susp_force; }
    static void test_unit_sop_lateral();
    static void test_unit_gyro_damping();
    static void test_unit_abs_pulse();

    // Load Normalization Test Access
    static double GetAutoPeakLoad(const FFBEngine& e) { return e.m_auto_peak_front_load; }
    static void SetAutoPeakLoad(FFBEngine& e, double val) { e.m_auto_peak_front_load = val; }
    static void SetAutoNormalizationEnabled(FFBEngine& e, bool enabled) { e.m_general.auto_load_normalization_enabled = enabled; }
    static void SetDynamicNormalizationEnabled(FFBEngine& e, bool enabled) { e.m_general.dynamic_normalization_enabled = enabled; }

    // Smoothing Test Access
    static double GetLongitudinalLoadSmoothed(const FFBEngine& e) { return e.m_long_load_smoothed; }
    static void SetLongitudinalLoadSmoothed(FFBEngine& e, double val) { e.m_long_load_smoothed = val; }
    static double GetFrontGripSmoothedState(const FFBEngine& e) { return e.m_front_grip_smoothed_state; }
    static void SetFrontGripSmoothedState(FFBEngine& e, double val) { e.m_front_grip_smoothed_state = val; }
    static void SetStaticFrontLoad(FFBEngine& e, double val) { e.m_static_front_load = val; }
    static void SetStaticRearLoad(FFBEngine& e, double val) { e.m_static_rear_load = val; }
    static double GetStaticFrontLoad(const FFBEngine& e) { return e.m_static_front_load; }
    static double GetStaticRearLoad(const FFBEngine& e) { return e.m_static_rear_load; }
    static double GetKerbTimer(const FFBEngine& e) { return e.m_kerb_timer; }
    static void SetKerbTimer(FFBEngine& e, double val) { e.m_kerb_timer = val; }
    static bool GetStaticLoadLatched(const FFBEngine& e) { return e.m_static_load_latched; }
    static void SetStaticLoadLatched(FFBEngine& e, bool val) { e.m_static_load_latched = val; }
    static double GetSmoothedVibrationMult(const FFBEngine& e) { return e.m_smoothed_vibration_mult; }
    static void SetSmoothedVibrationMult(FFBEngine& e, double val) { e.m_smoothed_vibration_mult = val; }
    // Wrappers for extracted utilities removed. Tests invoke them directly.
    static void SetSlopeDetectionEnabled(FFBEngine& e, bool val) { e.m_slope_detection_enabled = val; }
    static void SetSlopeBufferIndex(FFBEngine& e, int idx) { e.m_slope_buffer_index = idx; }
    static void SetSlopeBuffer(FFBEngine& e, const std::array<double, 41>& lat_g) { e.m_slope_lat_g_buffer = lat_g; }
    static void SetSlopeBufferCount(FFBEngine& e, int count) { e.m_slope_buffer_count = count; }
    static void SetSlopeTorqueBuffer(FFBEngine& e, const std::array<double, 41>& torque) { e.m_slope_torque_buffer = torque; }
    static void SetSlopeSteerBuffer(FFBEngine& e, const std::array<double, 41>& steer) { e.m_slope_steer_buffer = steer; }
    static void SetSlopeSlipBuffer(FFBEngine& e, const std::array<double, 41>& slip) { e.m_slope_slip_buffer = slip; }
    static void SetSlopeUseTorque(FFBEngine& e, bool val) { e.m_slope_use_torque = val; }
    static double CallCalculateSlopeGrip(FFBEngine& e, double lat_g, double slip, double dt, const TelemInfoV01* data) {
        return e.calculate_slope_grip(lat_g, slip, dt, data);
    }
    static double CallApplySignalConditioning(FFBEngine& e, double raw_torque, const TelemInfoV01* data, FFBCalculationContext& ctx) {
        return e.apply_signal_conditioning(raw_torque, data, ctx);
    }
    static void CallCalculateGyroDamping(FFBEngine& e, const TelemInfoV01* data, FFBCalculationContext& ctx) {
        e.calculate_gyro_damping(data, ctx);
    }
    static void CallCalculateABSPulse(FFBEngine& e, const TelemInfoV01* data, FFBCalculationContext& ctx) {
        e.calculate_abs_pulse(data, ctx);
    }
    static void CallCalculateLockup_Vibration(FFBEngine& e, const TelemInfoV01* data, FFBCalculationContext& ctx) {
        e.calculate_lockup_vibration(data, ctx);
    }
    static void SetFlatspotSuppression(FFBEngine& e, bool val) { e.m_flatspot_suppression = val; }
    static void SetFlatspotStrength(FFBEngine& e, float val) { e.m_flatspot_strength = val; }
    static void SetABSPulseEnabled(FFBEngine& e, bool val) { e.m_abs_pulse_enabled = val; }
    static void SetLastLogTime(FFBEngine& e, std::chrono::steady_clock::time_point t) { e.last_log_time = t; }
    static ChannelStats& GetTorqueStats(FFBEngine& e) { return e.s_torque; }
    static void SetRestApiEnabled(FFBEngine& e, bool val) { e.m_rest_api_enabled = val; }
    static void SetRestApiPort(FFBEngine& e, int val) { e.m_rest_api_port = val; }
    
    static void SetWasAllowed(FFBEngine& e, bool val) { e.m_was_allowed = val; }
    static bool GetWasAllowed(const FFBEngine& e) { return e.m_was_allowed; }
    static void CallCalculateSopLateral(FFBEngine& e, const TelemInfoV01* data, FFBCalculationContext& ctx) {
        e.calculate_sop_lateral(data, ctx);
    }

    // Coverage Restoration Accessors
    static void CallUpdateStaticLoadReference(FFBEngine& e, double front_load, double rear_load, double speed, double dt) {
        e.update_static_load_reference(front_load, rear_load, speed, dt);
    }
    static void CallInitializeLoadReference(FFBEngine& e, const char* vehicleClass, const char* vehicleName) {
        e.InitializeLoadReference(vehicleClass, vehicleName);
    }
    static void CallCalculateWheelSpin(FFBEngine& e, const TelemInfoV01* data, FFBCalculationContext& ctx) {
        e.calculate_wheel_spin(data, ctx);
    }
    static void SetTorqueSource(FFBEngine& e, int val) { e.m_torque_source = val; }
    static void SetSteering100HzReconstruction(FFBEngine& e, int val) { e.m_steering_100hz_reconstruction = val; }
    static void SetInvertForce(FFBEngine& e, bool val) { e.m_invert_force = val; }
    static void SetMinForce(FFBEngine& e, float val) { e.m_general.min_force = val; }
    static void SetSoftLockEnabled(FFBEngine& e, bool val) { e.m_soft_lock_enabled = val; }
    static void SetLockupEnabled(FFBEngine& e, bool val) { e.m_lockup_enabled = val; }
    static void CallCalculateSlideTexture(FFBEngine& e, const TelemInfoV01* data, FFBCalculationContext& ctx) {
        e.calculate_slide_texture(data, ctx);
    }
    static void CallCalculateRoadTexture(FFBEngine& e, const TelemInfoV01* data, FFBCalculationContext& ctx) {
        e.calculate_road_texture(data, ctx);
    }
    static void CallCalculateSuspensionBottoming(FFBEngine& e, const TelemInfoV01* data, FFBCalculationContext& ctx) {
        e.calculate_suspension_bottoming(data, ctx);
    }
    static void CallCalculateSoftLock(FFBEngine& e, const TelemInfoV01* data, FFBCalculationContext& ctx) {
        e.calculate_soft_lock(data, ctx);
    }
    static void SetScrubDragGain(FFBEngine& e, float val) { e.m_scrub_drag_gain = val; }
    static void SetBottomingEnabled(FFBEngine& e, bool val) { e.m_bottoming_enabled = val; }
    static void SetBottomingGain(FFBEngine& e, float val) { e.m_bottoming_gain = val; }
    static void SetBottomingMethod(FFBEngine& e, int val) { e.m_bottoming_method = val; }

    // Dynamic Normalization Test Access
    static double GetSessionPeakTorque(const FFBEngine& e) { return e.m_session_peak_torque; }
    static void SetSessionPeakTorque(FFBEngine& e, double val) { e.m_session_peak_torque = val; }
    static double GetSmoothedStructuralMult(const FFBEngine& e) { return e.m_smoothed_structural_mult; }
    static void SetSmoothedStructuralMult(FFBEngine& e, double val) { e.m_smoothed_structural_mult = val; }
    static void SetRollingAverageTorque(FFBEngine& e, double val) { e.m_rolling_average_torque = val; }
    static double GetRollingAverageTorque(const FFBEngine& e) { return e.m_rolling_average_torque; }
    static void SetLastRawTorque(FFBEngine& e, double val) { e.m_last_raw_torque = val; }
    static double GetLastRawTorque(const FFBEngine& e) { return e.m_last_raw_torque; }
    static double GetSteeringVelocitySmoothed(const FFBEngine& e) { return e.m_steering_velocity_smoothed; }
    static void SetSteeringVelocitySmoothed(FFBEngine& e, double val) { e.m_steering_velocity_smoothed = val; }
    static void AddSnapshot(FFBEngine& e, const FFBSnapshot& s) {
        e.m_debug_buffer.Push(s);
    }
    static void ResetYawDerivedState(FFBEngine& e) {
        e.m_yaw_rate_seeded = false;
        e.m_prev_yaw_rate = 0.0;
        e.m_yaw_accel_smoothed = 0.0;
    }
    static double GetYawAccelSmoothed(const FFBEngine& e) { return e.m_yaw_accel_smoothed; }
    static void SetLastOutputForce(FFBEngine& e, double val) { 
        e.m_safety.SetSafetySmoothedForce(val);
    }
    static void SetDerivativesSeeded(FFBEngine& e, bool val) { e.m_derivatives_seeded = val; }

    static void ResetSafety(FFBEngine& engine) {
        engine.m_safety = FFBSafetyMonitor();
        engine.m_safety.m_safety_window_duration = 2.0f; // Reset to legacy default for test compatibility
        engine.m_safety.SetLastControl(-2);
        // Restore connection to time source
        engine.m_safety.SetTimePtr(&engine.m_working_info.mElapsedTime);
    }
    static const FFBSafetyMonitor& GetSafety(const FFBEngine& engine) {
        return engine.m_safety;
    }
    static double GetLastCoreNanLogTime(const FFBEngine& engine) { return engine.m_last_core_nan_log_time; }
    static void SetLastCoreNanLogTime(FFBEngine& engine, double time) { engine.m_last_core_nan_log_time = time; }
};

} // namespace FFBEngineTests

// ============================================================
// Auto-Registration System (v0.7.8)
// ============================================================

namespace FFBEngineTests {

struct TestEntry {
    std::string name;
    std::string category;
    std::vector<std::string> tags;
    std::function<void()> func;
    int order_hint; // For sorting within a category
};

class TestRegistry {
public:
    static TestRegistry& Instance();
    void Register(const std::string& name, 
                  const std::string& category, 
                  const std::vector<std::string>& tags,
                  std::function<void()> func,
                  int order = 0);
    const std::vector<TestEntry>& GetTests() const;
    void SortByCategory(); 

private:
    std::vector<TestEntry> m_tests;
    bool m_sorted = false;
};

// Helper class for static registration
struct AutoRegister {
    AutoRegister(const std::string& name, 
                 const std::string& category, 
                 const std::vector<std::string>& tags,
                 std::function<void()> func,
                 int order = 0);
};

} // namespace FFBEngineTests

// Usage: TEST_CASE(test_my_feature, "CorePhysics", {"Physics", "Regression"})
#define TEST_CASE_TAGGED(test_name, category, tags) \
    static void test_name(); \
    static FFBEngineTests::AutoRegister reg_##test_name( \
        #test_name, category, tags, test_name); \
    static void test_name()

// Simple version without tags (defaults to {"Functional"})
#define TEST_CASE(test_name, category_name) \
    TEST_CASE_TAGGED(test_name, category_name, {"Functional"})

