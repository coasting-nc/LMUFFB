// test_ffb_common.h
#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <cstdio>
#include <random>
#include <sstream>
#include <functional>

#include "../src/FFBEngine.h"
#include "../src/lmu_sm_interface/InternalsPlugin.hpp"
#include "../src/lmu_sm_interface/LmuSharedMemoryWrapper.h"
#include "../src/Config.h"

namespace FFBEngineTests {

// --- Test Counters (defined in test_ffb_common.cpp) ---
extern int g_tests_passed;
extern int g_tests_failed;
extern int g_test_cases_run;
extern int g_test_cases_passed;
extern int g_test_cases_failed;

// --- Assert Macros ---
#define ASSERT_TRUE(condition) \
    if (condition) { \
        std::cout << "[PASS] " << #condition << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #condition << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
        g_tests_failed++; \
    }

#define ASSERT_FALSE(condition) \
    if (!(condition)) { \
        std::cout << "[PASS] !" << #condition << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] !" << #condition << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
        g_tests_failed++; \
    }

#define ASSERT_NEAR(a, b, epsilon) \
    if (std::abs((a) - (b)) < (epsilon)) { \
        std::cout << "[PASS] " << #a << " approx " << #b << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #a << " (" << (a) << ") != " << #b << " (" << (b) << ")" << std::endl; \
        g_tests_failed++; \
    }

#define ASSERT_GT(a, b) \
    if ((a) > (b)) { \
        std::cout << "[PASS] " << #a << " > " << #b << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #a << " (" << (a) << ") <= " << #b << " (" << (b) << ")" << std::endl; \
        g_tests_failed++; \
    }

#define ASSERT_EQ(a, b) \
    if ((a) == (b)) { \
        std::cout << "[PASS] " << #a << " == " << #b << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #a << " (" << (a) << ") != " << #b << " (" << (b) << ")" << std::endl; \
        g_tests_failed++; \
    }

#define ASSERT_GE(a, b) \
    if ((a) >= (b)) { \
        std::cout << "[PASS] " << #a << " >= " << #b << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #a << " (" << (a) << ") < " << #b << " (" << (b) << ")" << std::endl; \
        g_tests_failed++; \
    }

#define ASSERT_LE(a, b) \
    if ((a) <= (b)) { \
        std::cout << "[PASS] " << #a << " <= " << #b << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #a << " (" << (a) << ") > " << #b << " (" << (b) << ")" << std::endl; \
        g_tests_failed++; \
    }

#define ASSERT_LT(a, b) \
    if ((a) < (b)) { \
        std::cout << "[PASS] " << #a << " < " << #b << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #a << " (" << (a) << ") >= " << #b << " (" << (b) << ")" << std::endl; \
        g_tests_failed++; \
    }

#define ASSERT_EQ_STR(a, b) \
    if (std::string(a) == std::string(b)) { \
        std::cout << "[PASS] " << #a << " == " << #b << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #a << " (" << a << ") != " << #b << " (" << b << ")" << std::endl; \
        g_tests_failed++; \
    }

// --- Test Constants ---
const int FILTER_SETTLING_FRAMES = 40;

// --- Test Tagging System ---
// Global tag filter (set via command line arguments)
extern std::vector<std::string> g_tag_filter;
extern std::vector<std::string> g_tag_exclude;
extern std::vector<std::string> g_category_filter;
extern bool g_enable_tag_filtering;

// Tag checking helper
inline bool ShouldRunTest(const std::vector<std::string>& test_tags, const std::string& category) {
    if (!g_enable_tag_filtering) return true;
    
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


void Run(); // Main runner

// --- Friend Access for Testing ---
class FFBEngineTestAccess {
public:
    static bool HasWarnings(const FFBEngine& engine) {
        return engine.m_warned_load || engine.m_warned_grip || engine.m_warned_dt;
    }
    static void test_unit_sop_lateral();
    static void test_unit_gyro_damping();
    static void test_unit_abs_pulse();

    // Load Normalization Test Access
    static double GetAutoPeakLoad(const FFBEngine& e) { return e.m_auto_peak_load; }
    static void SetAutoPeakLoad(FFBEngine& e, double val) { e.m_auto_peak_load = val; }
    static void SetAutoNormalizationEnabled(FFBEngine& e, bool enabled) { e.m_auto_load_normalization_enabled = enabled; }

    // Smoothing Test Access
    static double GetDynamicWeightSmoothed(const FFBEngine& e) { return e.m_dynamic_weight_smoothed; }
    static void SetDynamicWeightSmoothed(FFBEngine& e, double val) { e.m_dynamic_weight_smoothed = val; }
    static double GetFrontGripSmoothedState(const FFBEngine& e) { return e.m_front_grip_smoothed_state; }
    static void SetFrontGripSmoothedState(FFBEngine& e, double val) { e.m_front_grip_smoothed_state = val; }
    static void SetStaticFrontLoad(FFBEngine& e, double val) { e.m_static_front_load = val; }
    static double GetStaticFrontLoad(const FFBEngine& e) { return e.m_static_front_load; }
    static bool GetStaticLoadLatched(const FFBEngine& e) { return e.m_static_load_latched; }
    static void SetStaticLoadLatched(FFBEngine& e, bool val) { e.m_static_load_latched = val; }
    static double GetSmoothedTactileMult(const FFBEngine& e) { return e.m_smoothed_tactile_mult; }
    static void SetSmoothedTactileMult(FFBEngine& e, double val) { e.m_smoothed_tactile_mult = val; }
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
    static void SetFlatspotSuppression(FFBEngine& e, bool val) { e.m_flatspot_suppression = val; }
    static void SetFlatspotStrength(FFBEngine& e, float val) { e.m_flatspot_strength = val; }
    static void SetABSPulseEnabled(FFBEngine& e, bool val) { e.m_abs_pulse_enabled = val; }
    static void SetLastLogTime(FFBEngine& e, std::chrono::steady_clock::time_point t) { e.last_log_time = t; }
    static ChannelStats& GetTorqueStats(FFBEngine& e) { return e.s_torque; }
    
    // Coverage Restoration Accessors
    static void CallUpdateStaticLoadReference(FFBEngine& e, double load, double speed, double dt) {
        e.update_static_load_reference(load, speed, dt);
    }
    static void CallInitializeLoadReference(FFBEngine& e, const char* vehicleClass, const char* vehicleName) {
        e.InitializeLoadReference(vehicleClass, vehicleName);
    }
    static void CallCalculateWheelSpin(FFBEngine& e, const TelemInfoV01* data, FFBCalculationContext& ctx) {
        e.calculate_wheel_spin(data, ctx);
    }
    static void SetTorqueSource(FFBEngine& e, int val) { e.m_torque_source = val; }
    static void SetInvertForce(FFBEngine& e, bool val) { e.m_invert_force = val; }
    static void SetMinForce(FFBEngine& e, float val) { e.m_min_force = val; }
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
    static void SetLastRawTorque(FFBEngine& e, double val) { e.m_last_raw_torque = val; }
    static void AddSnapshot(FFBEngine& e, const FFBSnapshot& s) {
        std::lock_guard<std::mutex> lock(e.m_debug_mutex);
        e.m_debug_buffer.push_back(s);
    }
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

