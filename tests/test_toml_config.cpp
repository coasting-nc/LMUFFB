#include "test_ffb_common.h"
#include <toml.hpp>

namespace FFBEngineTests {

TEST_CASE(test_toml_roundtrip_serialization, "Config") {
    std::cout << "\nTest: TOML Roundtrip Serialization" << std::endl;
    FFBEngine engine1;
    InitializeEngine(engine1);
    
    // Set non-default values
    engine1.m_general.gain = 1.234f;
    engine1.m_front_axle.understeer_effect = 0.567f;
    engine1.m_rear_axle.sop_effect = 0.891f;
    engine1.m_load_forces.lat_load_effect = 1.123f;
    engine1.m_grip_estimation.optimal_slip_angle = 0.15f;
    engine1.m_slope_detection.enabled = true;
    engine1.m_braking.lockup_gain = 2.345f;
    engine1.m_vibration.vibration_gain = 0.678f;
    engine1.m_advanced.gyro_gain = 0.901f;
    engine1.m_safety.m_config.gain_reduction = 0.25f;

    std::string test_file = "test_roundtrip.toml";
    Config::Save(engine1, test_file);

    FFBEngine engine2;
    InitializeEngine(engine2);
    Config::Load(engine2, test_file);

    ASSERT_NEAR(engine1.m_general.gain, engine2.m_general.gain, 0.0001f);
    ASSERT_NEAR(engine1.m_front_axle.understeer_effect, engine2.m_front_axle.understeer_effect, 0.0001f);
    ASSERT_NEAR(engine1.m_rear_axle.sop_effect, engine2.m_rear_axle.sop_effect, 0.0001f);
    ASSERT_NEAR(engine1.m_load_forces.lat_load_effect, engine2.m_load_forces.lat_load_effect, 0.0001f);
    ASSERT_NEAR(engine1.m_grip_estimation.optimal_slip_angle, engine2.m_grip_estimation.optimal_slip_angle, 0.0001f);
    ASSERT_TRUE(engine1.m_slope_detection.enabled == engine2.m_slope_detection.enabled);
    ASSERT_NEAR(engine1.m_braking.lockup_gain, engine2.m_braking.lockup_gain, 0.0001f);
    ASSERT_NEAR(engine1.m_vibration.vibration_gain, engine2.m_vibration.vibration_gain, 0.0001f);
    ASSERT_NEAR(engine1.m_advanced.gyro_gain, engine2.m_advanced.gyro_gain, 0.0001f);
    ASSERT_NEAR(engine1.m_safety.m_config.gain_reduction, engine2.m_safety.m_config.gain_reduction, 0.0001f);

    std::remove(test_file.c_str());
}

TEST_CASE(test_toml_missing_keys_fallback, "Config") {
    std::cout << "\nTest: TOML Missing Keys Fallback" << std::endl;
    std::string test_file = "test_missing.toml";
    {
        std::ofstream file(test_file);
        file << "[General]\ngain = 0.5\n";
        file.close();
    }

    FFBEngine engine;
    InitializeEngine(engine);
    // Set a non-default value to see if it gets overwritten by default if missing in TOML
    engine.m_front_axle.understeer_effect = 0.123f; 
    
    std::cout << "Loading TOML..." << std::endl;
    Config::Load(engine, test_file);
    std::cout << "TOML Loaded." << std::endl;

    ASSERT_NEAR(engine.m_general.gain, 0.5f, 0.0001f);
    ASSERT_NEAR(engine.m_front_axle.understeer_effect, 0.123f, 0.0001f);

    std::remove(test_file.c_str());
}

TEST_CASE(test_toml_type_safety, "Config") {
    std::cout << "\nTest: TOML Type Safety (Invalid Data)" << std::endl;
    std::string test_file = "test_bad_types.toml";
    {
        std::ofstream file(test_file);
        file << "[General]\ngain = \"high\"\n";
        file << "[System]\ninvert_force = 5\n";
        file.close();
    }

    FFBEngine engine;
    InitializeEngine(engine);
    float default_gain = engine.m_general.gain;
    
    // Force a specific state before loading bad data
    engine.m_invert_force = true; 
    bool state_before = engine.m_invert_force;

    // Should not crash and should NOT overwrite with junk (keep current state)
    Config::Load(engine, test_file);

    ASSERT_NEAR(engine.m_general.gain, default_gain, 0.0001f);
    ASSERT_TRUE(engine.m_invert_force == state_before);

    // Item 4: System config safe extraction (e.g. win_pos_x as string)
    {
        std::ofstream file(test_file);
        file << "[System]\nwin_pos_x = \"left\"\n";
        file.close();
        
        int x_before = 456;
        Config::win_pos_x = x_before;
        Config::Load(engine, test_file);
        ASSERT_TRUE(Config::win_pos_x == x_before);
    }

    std::remove(test_file.c_str());
}

TEST_CASE(test_toml_static_loads_numeric_types, "Config") {
    std::cout << "\nTest: TOML StaticLoads Numeric Types (Int/Float mix)" << std::endl;
    std::string test_file = "test_static_loads.toml";
    {
        std::ofstream file(test_file);
        file << "[System]\n";
        file << "gain = 1.0\n";
        file << "app_version = \"0.7.218\"\n";
        file << "[StaticLoads]\n";
        file << "\"Integer Car\" = 1200\n";
        file << "\"Float Car\" = 1350.5\n";
        file.close();
    }

    FFBEngine engine;
    Config::m_saved_static_loads.clear();
    Config::Load(engine, test_file);

    double val = 0.0;
    ASSERT_TRUE(Config::GetSavedStaticLoad("Integer Car", val));
    ASSERT_NEAR((float)val, 1200.0f, 0.001f);
    
    ASSERT_TRUE(Config::GetSavedStaticLoad("Float Car", val));
    ASSERT_NEAR((float)val, 1350.5f, 0.001f);

    std::remove(test_file.c_str());
}

TEST_CASE(test_builtin_preset_fidelity, "Config") {
    std::cout << "\nTest: Built-in Preset Fidelity (T300/Simagic/Moza)" << std::endl;
    Config::LoadPresets();

    auto check_preset = [](const std::string& name, float expected_max, float expected_target) {
        bool found = false;
        for (const auto& p : Config::presets) {
            if (p.name == name) {
                found = true;
                ASSERT_NEAR(p.general.wheelbase_max_nm, expected_max, 0.01f);
                ASSERT_NEAR(p.general.target_rim_nm, expected_target, 0.01f);
                break;
            }
        }
        if (!found) std::cout << "  [FAIL] Preset not found: " << name << std::endl;
        ASSERT_TRUE(found);
    };

    // NOTE: In tests, InitializeEngine sets wheelbase_max_nm = 15 and target_rim_nm = 10.
    // Presets like G25/T300 inherit these because they don't override them.
    // However, the DD presets (Moza, Simagic) explicitly override them to 15/10, 21/12 etc.
    check_preset("Thrustmaster T300/TX", 15.0f, 10.0f);
    check_preset("GT3 DD 15 Nm (Simagic Alpha)", 15.0f, 10.0f);
    check_preset("GM DD 21 Nm (Moza R21 Ultra)", 21.0f, 12.0f);
}

TEST_CASE(test_toml_preset_bridge, "Config") {
    std::cout << "\nTest: TOML Preset Bridge" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    if (std::filesystem::exists("user_presets")) std::filesystem::remove_all("user_presets");

    Config::presets.clear();
    Config::presets.push_back(Preset("Default", true));
    
    Preset custom("CustomUserPreset", false);
    custom.general.gain = 0.77f;
    // In Phase 3, presets are saved to individual files
    Config::AddUserPreset("CustomUserPreset", engine);

    Config::presets.clear();
    Config::LoadPresets();

    bool found = false;
    for (const auto& p : Config::presets) {
        if (p.name == "CustomUserPreset") {
            // Note: gain comes from engine state during AddUserPreset
            // InitializeEngine sets it to 1.0 usually.
            ASSERT_FALSE(p.is_builtin);
            found = true;
        }
    }
    ASSERT_TRUE(found);
    ASSERT_TRUE(std::filesystem::exists("user_presets/CustomUserPreset.toml"));
}

TEST_CASE(test_toml_migration_from_ini, "Config") {
    std::cout << "\nTest: Migration from config.ini to config.toml" << std::endl;
    
    // 1. Create a legacy config.ini
    std::string ini_file = "config.ini";
    std::string toml_file = "config.toml";
    std::remove(toml_file.c_str()); // Ensure it doesn't exist

    {
        std::ofstream file(ini_file);
        file << "gain=0.65\n";
        file << "sop=0.42\n";
        file << "[StaticLoads]\n";
        file << "Ferrari 488 GT3=1.23\n";
        file.close();
    }

    FFBEngine engine;
    InitializeEngine(engine);
    
    // 2. Load (should trigger migration)
    Config::m_config_path = toml_file;
    Config::Load(engine, toml_file);

    // 3. Verify values migrated
    ASSERT_NEAR(engine.m_general.gain, 0.65f, 0.0001f);
    ASSERT_NEAR(engine.m_rear_axle.sop_effect, 0.42f, 0.0001f);
    
    double load = 0.0;
    ASSERT_TRUE(Config::GetSavedStaticLoad("Ferrari 488 GT3", load));
    ASSERT_NEAR((float)load, 1.23f, 0.0001f);

    // 4. Verify config.toml was created
    ASSERT_TRUE(std::filesystem::exists(toml_file));
    
    // 5. Verify it's valid TOML and has the values
    toml::table tbl = toml::parse_file(toml_file);
    ASSERT_NEAR((float)((*tbl.get_as<toml::table>("General"))["gain"].value_or(0.0)), 0.65f, 0.0001f);

    std::remove(ini_file.c_str());
    std::remove(toml_file.c_str());
}

} // namespace FFBEngineTests
