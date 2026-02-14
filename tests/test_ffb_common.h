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

#define ASSERT_NEAR(a, b, epsilon) \
    if (std::abs((a) - (b)) < (epsilon)) { \
        std::cout << "[PASS] " << #a << " approx " << #b << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #a << " (" << (a) << ") != " << #b << " (" << (b) << ")" << std::endl; \
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

