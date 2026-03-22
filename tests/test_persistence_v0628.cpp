#include "test_ffb_common.h"
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include "src/ffb/FFBEngine.h"
#include "src/core/Config.h"
#include "src/Version.h"
#include <toml.hpp>

namespace FFBEngineTests {

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
// TEST 1: Load Stops At Presets Table (Phase 2 TOML)
// ----------------------------------------------------------------------------
TEST_CASE(test_load_stops_at_presets, "Persistence") {
    std::cout << "Test 1: Load Isolation..." << std::endl;
    Config::presets.clear();
    
    std::string test_file = "test_isolation.toml";
    {
        std::ofstream file(test_file);
        file << "[General]\ngain = 0.5\n";
        file << "[Presets.SomePreset.General]\ngain = 2.0\n";
    }
    
    FFBEngine engine;
    Config::Load(engine, test_file);
    
    // Config::Load should only load the main settings, not presets
    ASSERT_NEAR(engine.m_general.gain, 0.5f, 0.001f);
    
    std::remove(test_file.c_str());
}

// ----------------------------------------------------------------------------
// TEST 2: Save verified via TOML parsing (Category existence)
// ----------------------------------------------------------------------------
TEST_CASE(test_save_order, "Persistence") {
    std::cout << "Test 2: Category Verification (TOML)..." << std::endl;
    Config::presets.clear();
    FFBEngine engine;
    InitializeEngine(engine);
    
    std::string test_file = "test_order.toml";
    Config::Save(engine, test_file);
    
    try {
        toml::table tbl = toml::parse_file(test_file);
        ASSERT_TRUE(tbl.contains("System"));
        ASSERT_TRUE(tbl.contains("General"));
        ASSERT_TRUE(tbl.contains("FrontAxle"));
        ASSERT_TRUE(tbl.contains("RearAxle"));
        // Presets are no longer in config.toml in Phase 3
        ASSERT_FALSE(tbl.contains("Presets"));

        auto sys = tbl["System"].as_table();
        ASSERT_TRUE(sys != nullptr);
        ASSERT_TRUE(sys->contains("app_version"));
    } catch (const toml::parse_error& err) {
        FAIL_TEST("TOML parse error: " << err.description());
    }
    
    std::remove(test_file.c_str());
}

// ----------------------------------------------------------------------------
// TEST 3: Load Supports Legacy Keys via MigrateFromLegacyIni
// ----------------------------------------------------------------------------
TEST_CASE(test_legacy_keys, "Persistence") {
    std::cout << "Test 3: Legacy Key Support..." << std::endl;
    Config::presets.clear();
    
    std::string ini_file = "test_legacy.ini";
    {
        std::ofstream file(ini_file);
        file << "smoothing=0.1\n";
        file << "max_load_factor=2.0\n";
        file.close();
    }
    
    FFBEngine engine;
    Config::MigrateFromLegacyIni(engine, ini_file);
    
    ASSERT_NEAR(engine.m_rear_axle.sop_smoothing_factor, 0.1f, 0.001f);
    ASSERT_NEAR(engine.m_vibration.texture_load_cap, 2.0f, 0.001f);
    
    std::remove(ini_file.c_str());
}

// ----------------------------------------------------------------------------
// TEST 4: Structure (TOML headers)
// ----------------------------------------------------------------------------
TEST_CASE(test_structure_comments, "Persistence") {
    std::cout << "Test 4: TOML Headers..." << std::endl;
    Config::presets.clear();
    FFBEngine engine;
    
    std::string test_file = "test_headers.toml";
    Config::Save(engine, test_file);
    
    ASSERT_TRUE(FileContains(test_file, "[System]"));
    ASSERT_TRUE(FileContains(test_file, "[General]"));
    ASSERT_TRUE(FileContains(test_file, "[FrontAxle]"));
    ASSERT_TRUE(FileContains(test_file, "[RearAxle]"));
    
    std::remove(test_file.c_str());
}

} // namespace FFBEngineTests
