#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include "src/FFBEngine.h"
#include "src/Config.h"
#include "src/Version.h"

namespace PersistenceTests_v0628 {

int g_tests_passed = 0;
int g_tests_failed = 0;

#define ASSERT_TRUE(condition) \
    if (condition) { \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #condition << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
        g_tests_failed++; \
    }

#define ASSERT_NEAR(a, b, epsilon) \
    if (std::abs((a) - (b)) < (epsilon)) { \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #a << " (" << (a) << ") != " << #b << " (" << (b) << ")" << std::endl; \
        g_tests_failed++; \
    }

#define ASSERT_EQ(a, b) \
    if ((a) == (b)) { \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #a << " (" << (a) << ") != " << #b << " (" << (b) << ")" << std::endl; \
        g_tests_failed++; \
    }

/**
 * Helper to check if a file contains a specific string.
 */
bool FileContains(const std::string& filename, const std::string pattern) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    std::string line;
    while (std::getline(file, line)) {
        if (line.find(pattern) != std::string::npos) return true;
    }
    return false;
}

/**
 * Helper to get the line number of a pattern in a file (1-indexed).
 */
int GetLineNumber(const std::string& filename, const std::string& pattern) {
    std::ifstream file(filename);
    if (!file.is_open()) return -1;
    std::string line;
    int line_num = 1;
    while (std::getline(file, line)) {
        if (line.find(pattern) != std::string::npos) return line_num;
        line_num++;
    }
    return -1;
}

// ----------------------------------------------------------------------------
// TEST 1: Load Stops At Presets Header
// ----------------------------------------------------------------------------
void test_load_stops_at_presets() {
    std::cout << "Test 1: Load Stops At Presets Header..." << std::endl;
    Config::presets.clear();
    
    std::string test_file = "test_isolation.ini";
    {
        std::ofstream file(test_file);
        file << "gain=0.5\n";
        file << "[Presets]\n";
        file << "gain=2.0\n";
    }
    
    FFBEngine engine;
    Config::Load(engine, test_file);
    
    // In the buggy version, it would be 2.0
    ASSERT_NEAR(engine.m_gain, 0.5f, 0.001f);
    
    std::remove(test_file.c_str());
}

// ----------------------------------------------------------------------------
// TEST 2: Save Follows Defined Order
// ----------------------------------------------------------------------------
void test_save_order() {
    std::cout << "Test 2: Save Follows Defined Order..." << std::endl;
    Config::presets.clear();
    FFBEngine engine;
    Preset::ApplyDefaultsToEngine(engine);
    
    std::string test_file = "test_order.ini";
    Config::Save(engine, test_file);
    
    int line_win = GetLineNumber(test_file, "win_pos_x");
    int line_gain = GetLineNumber(test_file, "gain");
    int line_understeer = GetLineNumber(test_file, "understeer=");
    int line_boost = GetLineNumber(test_file, "oversteer_boost");
    int line_presets = GetLineNumber(test_file, "[Presets]");
    
    ASSERT_TRUE(line_win != -1);
    ASSERT_TRUE(line_gain != -1);
    ASSERT_TRUE(line_understeer != -1);
    ASSERT_TRUE(line_boost != -1);
    ASSERT_TRUE(line_presets != -1);
    
    ASSERT_TRUE(line_win < line_gain);
    ASSERT_TRUE(line_gain < line_understeer);
    ASSERT_TRUE(line_understeer < line_boost);
    ASSERT_TRUE(line_boost < line_presets);
    
    std::remove(test_file.c_str());
}

// ----------------------------------------------------------------------------
// TEST 3: Load Supports Legacy Keys
// ----------------------------------------------------------------------------
void test_legacy_keys() {
    std::cout << "Test 3: Load Supports Legacy Keys..." << std::endl;
    Config::presets.clear();
    
    std::string test_file = "test_legacy.ini";
    {
        std::ofstream file(test_file);
        file << "smoothing=0.1\n";
        file << "max_load_factor=2.0\n";
    }
    
    FFBEngine engine;
    Config::Load(engine, test_file);
    
    ASSERT_NEAR(engine.m_sop_smoothing_factor, 0.1f, 0.001f);
    ASSERT_NEAR(engine.m_texture_load_cap, 2.0f, 0.001f);
    
    std::remove(test_file.c_str());
}

// ----------------------------------------------------------------------------
// TEST 4: Structure Includes Comments
// ----------------------------------------------------------------------------
void test_structure_comments() {
    std::cout << "Test 4: Structure Includes Comments..." << std::endl;
    Config::presets.clear();
    FFBEngine engine;
    
    std::string test_file = "test_comments.ini";
    Config::Save(engine, test_file);
    
    ASSERT_TRUE(FileContains(test_file, "; --- System & Window ---"));
    ASSERT_TRUE(FileContains(test_file, "; --- General FFB ---"));
    ASSERT_TRUE(FileContains(test_file, "; --- Front Axle (Understeer) ---"));
    ASSERT_TRUE(FileContains(test_file, "; --- Rear Axle (Oversteer) ---"));
    
    std::remove(test_file.c_str());
}

void Run() {
    std::cout << "\n=== Running v0.6.28 Persistence Tests (Reordering) ===" << std::endl;
    
    test_load_stops_at_presets();
    test_save_order();
    test_legacy_keys();
    test_structure_comments();

    std::cout << "\n--- Persistence v0.6.28 Test Summary ---" << std::endl;
    std::cout << "Tests Passed: " << g_tests_passed << std::endl;
    std::cout << "Tests Failed: " << g_tests_failed << std::endl;
}

} // namespace PersistenceTests_v0628
