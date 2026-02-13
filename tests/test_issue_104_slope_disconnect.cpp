#include "test_ffb_common.h"
#include "src/Config.h"
#include <fstream>

namespace FFBEngineTests {

TEST_CASE(test_slope_config_migration, "Regression") {
    std::cout << "Test: Slope Config Migration (Issue #104)" << std::endl;

    // 1. Create a legacy config file
    std::ofstream file("test_legacy_slope.ini");
    file << "[Presets]" << std::endl;
    file << "[Preset:LegacyTest]" << std::endl;
    file << "slope_negative_threshold=-0.88" << std::endl; // Legacy key
    file << "slope_detection_enabled=1" << std::endl;
    file.close();

    // 2. Load it
    FFBEngine engine;
    // Reset to defaults first to ensure we don't have lingering state
    engine.m_slope_min_threshold = -0.3f; 
    
    Config::m_config_path = "test_legacy_slope.ini";
    Config::LoadPresets(); // Should parse and migrate
    Config::ApplyPreset((int)Config::presets.size() - 1, engine); // Apply the last loaded preset (LegacyTest)

    // 3. Verify Migration
    // The legacy key should have populated the new variable
    ASSERT_NEAR(engine.m_slope_min_threshold, -0.88f, 0.001f);
    
    // Verify Struct
    ASSERT_NEAR(Config::presets.back().slope_min_threshold, -0.88f, 0.001f);

    std::remove("test_legacy_slope.ini");
}

TEST_CASE(test_slope_persistence_new_key, "Regression") {
    std::cout << "Test: Slope Persistence New Key (Issue #104)" << std::endl;

    FFBEngine engine;
    engine.m_slope_min_threshold = -0.55f;
    engine.m_slope_detection_enabled = true;

    // Save to a new file
    Config::Save(engine, "test_slope_save.ini");

    // Read back manually to check keys
    std::ifstream infile("test_slope_save.ini");
    std::string line;
    bool found_new_key = false;
    bool found_old_key = false;

    while (std::getline(infile, line)) {
        if (line.find("slope_min_threshold=-0.55") != std::string::npos) {
            found_new_key = true;
        }
        if (line.find("slope_negative_threshold=") != std::string::npos) {
            found_old_key = true;
        }
    }
    infile.close();

    ASSERT_TRUE(found_new_key);
    ASSERT_TRUE(!found_old_key); // Should NOT write deprecated key

    std::remove("test_slope_save.ini");
}

} // namespace FFBEngineTests
