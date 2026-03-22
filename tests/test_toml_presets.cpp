#include "test_ffb_common.h"
#include <filesystem>
#include <toml.hpp>
#include <fstream>

namespace fs = std::filesystem;

namespace FFBEngineTests {

TEST_CASE(test_phase3_embedded_builtins, "Presets") {
    std::cout << "\nTest: Phase 3 - Embedded Built-ins" << std::endl;

    // Clear presets to ensure we are testing LoadPresets behavior
    Config::presets.clear();

    Config::LoadPresets();

    bool found_t300 = false;
    for (const auto& p : Config::presets) {
        // Built-in presets are embedded in the binary as TOML strings.
        // Their names come from the 'name' key in TOML or the key in the BUILTIN_PRESETS map.
        if (p.name == "Thrustmaster T300/TX") {
            found_t300 = true;
            ASSERT_TRUE(p.is_builtin);
            // Verify some core values are loaded
            ASSERT_GT(p.general.wheelbase_max_nm, 0.0f);
        }
    }
    if (!found_t300) {
        std::cout << "  [FAIL] T300 preset not found. Available: ";
        for (const auto& p : Config::presets) std::cout << "'" << p.name << "' ";
        std::cout << std::endl;
    }
    ASSERT_TRUE(found_t300);
}

TEST_CASE(test_phase3_user_preset_file_io, "Presets") {
    std::cout << "\nTest: Phase 3 - User Preset File I/O" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    // Ensure user_presets directory is clean for this test
    if (fs::exists("user_presets")) {
        fs::remove_all("user_presets");
    }
    fs::create_directories("user_presets");

    std::string preset_name = "My Crazy Wheel: V2!";
    Config::AddUserPreset(preset_name, engine);

    // Verify file existence (sanitized name)
    // Proposed: "My Crazy Wheel: V2!" -> "My_Crazy_Wheel__V2_.toml"
    bool file_found = false;
    for (const auto& entry : fs::directory_iterator("user_presets")) {
        if (entry.path().extension() == ".toml") {
            file_found = true;
            std::cout << "Found sanitized file: " << entry.path().filename().string() << std::endl;
        }
    }
    ASSERT_TRUE(file_found);

    // Clear memory and reload
    Config::presets.clear();
    Config::LoadPresets();

    bool found_in_memory = false;
    for (const auto& p : Config::presets) {
        if (p.name == preset_name) {
            found_in_memory = true;
            ASSERT_FALSE(p.is_builtin);
        }
    }
    ASSERT_TRUE(found_in_memory);
}

TEST_CASE(test_phase3_migration_from_config_toml, "Presets") {
    std::cout << "\nTest: Phase 3 - Migration from config.toml" << std::endl;

    std::string test_config = "config_migration.toml";
    if (fs::exists("user_presets")) fs::remove_all("user_presets");
    fs::create_directories("user_presets");

    // Create a mock config.toml with a [Presets] table
    {
        std::ofstream f(test_config);
        f << "[General]\ngain = 1.0\n\n";
        f << "[Presets.\"Trapped Preset\"]\n";
        f << "app_version = \"0.7.218\"\n";
        f << "[Presets.\"Trapped Preset\".General]\n";
        f << "gain = 0.85\n";
        f.close();
    }

    FFBEngine engine;
    InitializeEngine(engine);

    // We need to point Config to use our test config
    std::string old_path = Config::m_config_path;
    Config::m_config_path = test_config;

    Config::Load(engine, test_config);

    // 1. Assert file created
    // Sanitized: "Trapped Preset" -> "Trapped_Preset.toml"
    ASSERT_TRUE(fs::exists("user_presets/Trapped_Preset.toml"));

    // 2. Assert [Presets] removed from config.toml file on disk
    toml::table tbl = toml::parse_file(test_config);
    ASSERT_FALSE(tbl.contains("Presets"));

    Config::m_config_path = old_path;
    fs::remove(test_config);
}

TEST_CASE(test_phase3_legacy_preset_import, "Presets") {
    std::cout << "\nTest: Phase 3 - Legacy Preset Import (.ini)" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);
    if (fs::exists("user_presets")) fs::remove_all("user_presets");
    fs::create_directories("user_presets");

    std::string ini_file = "legacy_import.ini";
    {
        std::ofstream f(ini_file);
        f << "[Preset:Old Drift]\n";
        f << "ini_version=0.7.60\n";
        f << "max_torque_ref=50.0\n";
        f << "gain=1.0\n";
        f.close();
    }

    Config::ImportPreset(ini_file, engine);

    bool found = false;
    for (const auto& p : Config::presets) {
        if (p.name == "Old Drift") {
            found = true;
            // Legacy math: if max_torque_ref > 40, wheelbase_max_nm = 15.0
            ASSERT_NEAR(p.general.wheelbase_max_nm, 15.0f, 0.01f);
        }
    }
    ASSERT_TRUE(found);
    ASSERT_TRUE(fs::exists("user_presets/Old_Drift.toml"));

    fs::remove(ini_file);
}

} // namespace FFBEngineTests
