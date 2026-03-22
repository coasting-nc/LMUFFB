#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_config_persistence, "Config") {
    std::cout << "\nTest: Config Save/Load Persistence (TOML)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_general.gain = 1.23f;
    engine.m_rear_axle.sop_effect = 0.45f;
    engine.m_vibration.road_enabled = true;
    Config::Save(engine, "test_config_persistence.toml");

    FFBEngine engine_load;
    InitializeEngine(engine_load);
    Config::Load(engine_load, "test_config_persistence.toml");
    ASSERT_NEAR(engine_load.m_general.gain, 1.23f, 0.01);
    ASSERT_NEAR(engine_load.m_rear_axle.sop_effect, 0.45f, 0.01);
    ASSERT_TRUE(engine_load.m_vibration.road_enabled);
    std::remove("test_config_persistence.toml");
}

TEST_CASE(test_channel_stats, "Config") {
    std::cout << "\nTest: Channel Stats Logic" << std::endl;
    
    ChannelStats stats;
    
    // Sequence: 10, 20, 30
    stats.Update(10.0);
    stats.Update(20.0);
    stats.Update(30.0);
    
    // Verify Session Min/Max
    ASSERT_NEAR(stats.session_min, 10.0, 0.001);
    ASSERT_NEAR(stats.session_max, 30.0, 0.001);
    
    // Verify Interval Avg (Compatibility helper)
    ASSERT_NEAR(stats.Avg(), 20.0, 0.001);
    
    // Test Interval Reset (Session min/max should persist)
    stats.ResetInterval();
    if (stats.interval_count == 0) {
        std::cout << "[PASS] Interval Stats Reset." << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Interval Reset failed.");
    }
    
    // Min/Max should still be valid
    ASSERT_NEAR(stats.session_min, 10.0, 0.001);
    ASSERT_NEAR(stats.session_max, 30.0, 0.001);
    
    ASSERT_NEAR(stats.Avg(), 0.0, 0.001); // Handle divide by zero check
}

TEST_CASE(test_presets, "Config") {
    std::cout << "\nTest: Configuration Presets" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    Config::LoadPresets();
    
    // Use a known builtin instead of "Test: SoP Only" which might not exist in production
    int idx = -1;
    for(size_t i=0; i<Config::presets.size(); i++) {
        if(Config::presets[i].name == "Thrustmaster T300/TX") {
            idx = (int)i;
            break;
        }
    }
    
    if(idx != -1) {
        Config::ApplyPreset(idx, engine);
        ASSERT_NEAR(engine.m_general.min_force, 0.08f, 0.01);
    } else {
        FAIL_TEST("Preset 'Thrustmaster T300/TX' not found");
    }
}

TEST_CASE(test_preset_initialization, "Config") {
    std::cout << "\nTest: Built-in Preset Fidelity" << std::endl;
    Config::LoadPresets();
    
    // Updated names to match Config.cpp
    const char* preset_names[] = {
        "Default",
        "Logitech G25/G27/G29/G920",
        "Thrustmaster T300/TX",
        "Thrustmaster T-GT/T-GT II",
        "Thrustmaster TS-PC/TS-XW",
        "Fanatec CSL DD / GT DD Pro",
        "Fanatec Podium DD1/DD2",
        "Simucube 2 Sport/Pro/Ultimate",
        "Simagic Alpha/Alpha Mini/Alpha U",
        "Moza R5/R9/R16/R21"
    };
    
    for (int i = 0; i < 10; i++) {
        bool found = false;
        for (const auto& p : Config::presets) {
            if (p.name == preset_names[i]) {
                found = true;
                break;
            }
        }
        if (found) {
            std::cout << "[PASS] " << preset_names[i] << " verified." << std::endl;
            g_tests_passed++;
        } else {
            FAIL_TEST("Preset " << preset_names[i] << " not found!");
        }
    }
}

TEST_CASE(test_config_defaults_v057, "Config") {
    std::cout << "\nTest: Config Defaults" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    ASSERT_TRUE(Config::m_always_on_top);
}

TEST_CASE(test_config_safety_validation_v057, "Config") {
    std::cout << "\nTest: Config Safety Validation (TOML)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    // Test 1: optimal_slip_angle reset (legacy compatibility)
    {
        std::ofstream ofs("tmp_validation.toml");
        ofs << "[GripEstimation]\noptimal_slip_angle = 0.005\n";
        ofs.close();
        Config::Load(engine, "tmp_validation.toml");
        ASSERT_NEAR(engine.m_grip_estimation.optimal_slip_angle, 0.10f, 0.01);
    }

    // Test 2: optimal_slip_ratio reset
    {
        std::ofstream ofs("tmp_validation.toml");
        ofs << "[GripEstimation]\noptimal_slip_ratio = 0.005\n";
        ofs.close();
        Config::Load(engine, "tmp_validation.toml");
        ASSERT_NEAR(engine.m_grip_estimation.optimal_slip_ratio, 0.12f, 0.01);
    }
    std::remove("tmp_validation.toml");
}

TEST_CASE(test_config_safety_clamping, "Config") {
    std::cout << "\nTest: Config Safety Clamping (TOML)" << std::endl;
    
    FFBEngine engine;
    InitializeEngine(engine);
    
    // Set unsafe values
    engine.m_vibration.slide_gain = 5.0f;     // Clamp to 2.0
    engine.m_braking.lockup_gain = 8.0f;     // Clamp (no hard limit in Validate() but lets use a known one)
    engine.m_rear_axle.sop_yaw_gain = 20.0f; // Clamp to 1.0
    
    Config::Save(engine, "tmp_unsafe.toml");
    Config::Load(engine, "tmp_unsafe.toml");
    
    ASSERT_NEAR(engine.m_vibration.slide_gain, 2.0f, 0.001);
    // ASSERT_NEAR(engine.m_braking.lockup_gain, 3.0f, 0.001); // BrakingConfig doesn't clamp max gain
    ASSERT_NEAR(engine.m_rear_axle.sop_yaw_gain, 1.0f, 0.001);
    
    std::remove("tmp_unsafe.toml");
}

TEST_CASE(test_config_migration_logic, "Config") {
    std::cout << "\nTest: Config Migration Logic (INI -> Engine)" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    // Use MigrateFromLegacyIni directly to bypass TOML detection logic
    const char* test_file_dd = "tmp_migration_dd.ini";
    {
        std::ofstream file(test_file_dd);
        file << "max_torque_ref=100.0\n";
        file.close();
    }
    Config::MigrateFromLegacyIni(engine, test_file_dd);
    ASSERT_NEAR(engine.m_general.wheelbase_max_nm, 15.0f, 0.01);
    ASSERT_NEAR(engine.m_general.target_rim_nm, 10.0f, 0.01);
    std::remove(test_file_dd);

    // Case 2: understeer_effect migration (> 2.0f range)
    InitializeEngine(engine);
    const char* test_file_under = "tmp_migration_under.ini";
    {
        std::ofstream file(test_file_under);
        file << "understeer=50.0\n";
        file.close();
    }
    Config::MigrateFromLegacyIni(engine, test_file_under);
    ASSERT_NEAR(engine.m_front_axle.understeer_effect, 0.5f, 0.01);
    std::remove(test_file_under);
}

} // namespace FFBEngineTests
