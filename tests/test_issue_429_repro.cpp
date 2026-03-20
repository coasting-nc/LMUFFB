#include "test_ffb_common.h"
#include <fstream>
#include <filesystem>

namespace FFBEngineTests {

TEST_CASE(test_issue_429_repro_load, "Config") {
    std::cout << "\nTest: Issue #429 Repro - Multi-preset Load from config.ini" << std::endl;

    std::string mock_config = "repro_429_config.ini";
    std::string original_path = Config::m_config_path;
    Config::m_config_path = mock_config;

    // 1. Create a mock config.ini with multiple custom presets
    {
        std::ofstream file(mock_config);
        file << "ini_version=" << LMUFFB_VERSION << "\n";
        file << "last_preset_name=PresetB\n";
        file << "\n[Presets]\n";
        file << "[Preset:PresetA]\n";
        file << "gain=0.7\n";
        file << "[Preset:PresetB]\n";
        file << "gain=0.8\n";
        file.close();
    }

    // 2. Clear memory state and load
    Config::presets.clear();
    Config::LoadPresets();

    bool foundA = false;
    bool foundB = false;
    for (const auto& p : Config::presets) {
        if (p.name == "PresetA") foundA = true;
        if (p.name == "PresetB") foundB = true;
    }

    std::cout << "Presets loaded: " << Config::presets.size() << std::endl;
    for (const auto& p : Config::presets) {
        std::cout << " - " << p.name << (p.is_builtin ? " (builtin)" : "") << std::endl;
    }

    // Restore original path
    Config::m_config_path = original_path;
    if (std::filesystem::exists(mock_config)) std::filesystem::remove(mock_config);

    ASSERT_TRUE(foundA);
    ASSERT_TRUE(foundB);
}

TEST_CASE(test_issue_429_repro_import, "Config") {
    std::cout << "\nTest: Issue #429 Repro - Multi-preset Import" << std::endl;

    std::string mock_import = "repro_429_import.ini";
    FFBEngine engine;
    InitializeEngine(engine);

    // 1. Create a mock import file with multiple presets
    {
        std::ofstream file(mock_import);
        file << "[Preset:ImportA]\n";
        file << "gain=0.9\n";
        file << "[Preset:ImportB]\n";
        file << "gain=1.0\n";
        file.close();
    }

    // 2. Clear presets and import
    Config::presets.clear();
    // Add Default so we don't have empty presets which might trigger LoadPresets if not careful
    Config::presets.push_back(Preset("Default", true));

    bool result = Config::ImportPreset(mock_import, engine);

    bool foundA = false;
    bool foundB = false;
    for (const auto& p : Config::presets) {
        if (p.name == "ImportA") foundA = true;
        if (p.name == "ImportB") foundB = true;
    }

    std::cout << "Import result: " << (result ? "Success" : "Failure") << std::endl;
    std::cout << "Presets after import: " << Config::presets.size() << std::endl;
    for (const auto& p : Config::presets) {
        std::cout << " - " << p.name << (p.is_builtin ? " (builtin)" : "") << std::endl;
    }

    if (std::filesystem::exists(mock_import)) std::filesystem::remove(mock_import);

    ASSERT_TRUE(result);
    ASSERT_TRUE(foundA);
    ASSERT_TRUE(foundB);
}

} // namespace FFBEngineTests
