#include "test_ffb_common.h"
#include <fstream>
#include <filesystem>

namespace FFBEngineTests {

TEST_CASE(test_preset_export_import, "Config") {
    std::cout << "\nTest: Preset Export/Import" << std::endl;

    // 1. Setup a custom preset
    FFBEngine engine;
    InitializeEngine(engine);
    Config::LoadPresets();

    std::string test_preset_name = "ExportTestPreset";
    Preset p(test_preset_name);
    p.general.gain = 0.75f;
    p.rear_axle.sop_effect = 1.25f;
    p.vibration.road_enabled = true;
    p.vibration.road_gain = 0.5f;

    Config::presets.push_back(p);
    int export_idx = (int)Config::presets.size() - 1;

    std::string export_filename = "test_export_preset.ini";

    // 2. Export
    Config::ExportPreset(export_idx, export_filename);

    // Verify file exists
    std::ifstream file(export_filename);
    ASSERT_TRUE(file.is_open());

    bool found_header = false;
    bool found_gain = false;
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("[Preset:ExportTestPreset]") != std::string::npos) found_header = true;
        if (line.find("gain=0.75") != std::string::npos) found_gain = true;
    }
    file.close();

    ASSERT_TRUE(found_header);
    ASSERT_TRUE(found_gain);

    // 3. Import
    // We'll use a different name for the imported file to avoid confusion in the vector
    // But ImportPreset handles name collisions, so it should append (1)

    size_t count_before = Config::presets.size();
    bool imported = Config::ImportPreset(export_filename, engine);
    ASSERT_TRUE(imported);
    ASSERT_TRUE(Config::presets.size() == count_before + 1);

    Preset& imported_p = Config::presets.back();
    ASSERT_TRUE(imported_p.name.find("ExportTestPreset") != std::string::npos);
    ASSERT_NEAR(imported_p.general.gain, 0.75f, 0.001f);
    ASSERT_NEAR(imported_p.rear_axle.sop_effect, 1.25f, 0.001f);
    ASSERT_TRUE(imported_p.vibration.road_enabled);
    ASSERT_NEAR(imported_p.vibration.road_gain, 0.5f, 0.001f);

    // Clean up
    if (std::filesystem::exists(export_filename)) std::filesystem::remove(export_filename);
    std::cout << "[PASS] Preset Export/Import verified." << std::endl;
}

TEST_CASE(test_import_name_collision, "Config") {
    std::cout << "\nTest: Import Name Collision Handling" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);
    Config::LoadPresets();

    // Create a preset file
    std::string filename = "collision_test.ini";
    {
        std::ofstream file(filename);
        file << "[Preset:Default]\n";
        file << "gain=0.5\n";
        file.close();
    }

    // "Default" already exists in LoadPresets
    bool imported = Config::ImportPreset(filename, engine);
    ASSERT_TRUE(imported);

    Preset& imported_p = Config::presets.back();
    // Should be "Default (1)"
    ASSERT_TRUE(imported_p.name == "Default (1)");
    ASSERT_NEAR(imported_p.general.gain, 0.5f, 0.001f);

    // Clean up
    if (std::filesystem::exists(filename)) std::filesystem::remove(filename);
    std::cout << "[PASS] Name collision handled correctly." << std::endl;
}

} // namespace FFBEngineTests
