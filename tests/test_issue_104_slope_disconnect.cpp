#include "test_ffb_common.h"
#include "src/core/Config.h"
#include <fstream>

namespace FFBEngineTests {

TEST_CASE(test_slope_config_migration, "Regression") {
    std::cout << "Test: Slope Config Migration (Issue #104)" << std::endl;

    // 1. Create a legacy config file
    std::string ini_file = "test_legacy_slope.ini";
    {
        std::ofstream file(ini_file);
        file << "[Preset:LegacyTest]" << std::endl;
        file << "slope_negative_threshold=-0.88" << std::endl; // Legacy key
        file << "slope_detection_enabled=1" << std::endl;
        file.close();
    }

    // 2. Load it via ImportPreset which still supports legacy INI
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.min_threshold = -0.3f;
    
    Config::presets.clear();
    Config::ImportPreset(ini_file, engine);

    int idx = -1;
    for(int i=0; i<(int)Config::presets.size(); ++i) {
        if (Config::presets[i].name == "LegacyTest") { idx = i; break; }
    }
    ASSERT_TRUE(idx != -1);
    Config::ApplyPreset(idx, engine);

    // 3. Verify Migration
    ASSERT_NEAR(engine.m_slope_detection.min_threshold, -0.88f, 0.001f);
    ASSERT_NEAR(Config::presets[idx].slope_detection.min_threshold, -0.88f, 0.001f);

    std::remove(ini_file.c_str());
}

TEST_CASE(test_slope_persistence_new_key, "Regression") {
    std::cout << "Test: Slope Persistence (TOML)" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.min_threshold = -0.55f;
    engine.m_slope_detection.enabled = true;

    // Save to a new file
    std::string test_file = "test_slope_save.toml";
    Config::Save(engine, test_file);

    ASSERT_TRUE(IsInLog(test_file, "[SlopeDetection]"));
    ASSERT_TRUE(IsInLog(test_file, "min_threshold"));

    // Verify it doesn't use the legacy flat key
    ASSERT_FALSE(IsInLog(test_file, "slope_min_threshold"));

    std::remove(test_file.c_str());
}

} // namespace FFBEngineTests
