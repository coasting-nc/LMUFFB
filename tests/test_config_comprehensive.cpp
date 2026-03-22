#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_config_comprehensive_import, "Config") {
    std::cout << "\nTest: Comprehensive Config Import (TOML)" << std::endl;

    const char* test_file = "tmp_comprehensive_import.toml";
    {
        std::ofstream file(test_file);
        file << "[Presets.Comprehensive]\n";
        file << "app_version = \"0.7.218\"\n";
        file << "[Presets.Comprehensive.General]\n";
        file << "gain = 1.1\n";
        file << "wheelbase_max_nm = 18.0\n";
        file << "[Presets.Comprehensive.FrontAxle]\n";
        file << "understeer = 0.6\n";
        file << "[Presets.Comprehensive.Braking]\n";
        file << "lockup_enabled = true\n";
        file.close();
    }

    FFBEngine engine;
    InitializeEngine(engine);

    Config::presets.clear();
    Config::presets.push_back(Preset("Default", true));

    // In Phase 3, presets are extracted from config files during Config::Load
    Config::Load(engine, test_file);

    bool found = false;
    for (const auto& p : Config::presets) {
        if (p.name == "Comprehensive") {
            ASSERT_NEAR(p.general.gain, 1.1f, 0.01);
            ASSERT_NEAR(p.front_axle.understeer_effect, 0.6f, 0.01);
            ASSERT_EQ(p.braking.lockup_enabled, true);
            ASSERT_NEAR(p.general.wheelbase_max_nm, 18.0f, 0.01);
            found = true;
        }
    }
    ASSERT_TRUE(found);
    std::remove(test_file);
}

TEST_CASE(test_config_comprehensive_load_v2, "Config") {
    std::cout << "\nTest: Comprehensive Config Load V2 (TOML)" << std::endl;

    const char* test_file = "tmp_comprehensive_v2.toml";
    {
        std::ofstream file(test_file);
        file << "[System]\n";
        file << "always_on_top = true\n";
        file << "[General]\n";
        file << "gain = 1.1\n";
        file << "wheelbase_max_nm = 18.0\n";
        file << "[FrontAxle]\n";
        file << "understeer = 0.6\n";
        file << "[Braking]\n";
        file << "lockup_enabled = true\n";
        file << "[StaticLoads]\n";
        file << "\"Ferrari 488 GTE\" = 4200.5\n";
        file.close();
    }

    FFBEngine engine;
    InitializeEngine(engine);
    Config::Load(engine, test_file);

    ASSERT_NEAR(engine.m_general.gain, 1.1f, 0.01);
    ASSERT_NEAR(engine.m_front_axle.understeer_effect, 0.6f, 0.01);
    ASSERT_EQ(engine.m_braking.lockup_enabled, true);
    ASSERT_NEAR(engine.m_general.wheelbase_max_nm, 18.0f, 0.01);
    ASSERT_EQ(Config::m_always_on_top, true);

    double saved_load;
    ASSERT_TRUE(Config::GetSavedStaticLoad("Ferrari 488 GTE", saved_load));
    ASSERT_NEAR(saved_load, 4200.5, 0.001);

    std::remove(test_file);
}

} // namespace FFBEngineTests
