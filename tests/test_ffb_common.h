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

#include "../src/FFBEngine.h"
#include "../src/lmu_sm_interface/InternalsPlugin.hpp"
#include "../src/lmu_sm_interface/LmuSharedMemoryWrapper.h"
#include "../src/Config.h"

namespace FFBEngineTests {

// --- Test Counters (defined in test_ffb_common.cpp) ---
extern int g_tests_passed;
extern int g_tests_failed;

// --- Assert Macros ---
#define ASSERT_TRUE(condition) \
    if (condition) { \
        std::cout << "[PASS] " << #condition << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #condition << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
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

// --- Sub-Runner Declarations ---
void Run_CorePhysics();
void Run_SlipGrip();
void Run_Understeer();
void Run_SlopeDetection();
void Run_RoadTexture();
void Run_Texture();
void Run_LockupBraking();
void Run_YawGyro();
void Run_Coordinates();
void Run_Config();
void Run_SpeedGate();
void Run_Internal(); // New runner for internal tests
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
};

} // namespace FFBEngineTests
