#include "test_ffb_common.h"
#include <fstream>
#include <filesystem>

namespace FFBEngineTests {

TEST_CASE(test_issue_371_repro, "Config") {
    std::cout << "\nTest: Issue #371 - Imported profiles preserved on early save" << std::endl;

    std::string test_config = "test_issue_371.ini";

    // 1. Create a config file with a user preset
    {
        std::ofstream file(test_config);
        file << "ini_version=0.7.191\n";
        file << "gain=1.0\n";
        file << "\n[Presets]\n";
        file << "[Preset:Imported Car]\n";
        file << "gain=0.85\n";
        file << "sop=1.5\n";
        file.close();
    }

    // 2. Simulate fresh application start (empty presets vector)
    Config::presets.clear();
    FFBEngine engine;
    InitializeEngine(engine);

    // 3. Call Load (should now call LoadPresets internally)
    Config::Load(engine, test_config);

    // Verify presets vector is populated
    std::cout << "Presets count after Load: " << Config::presets.size() << std::endl;

    // 4. Simulate an auto-save (e.g. from static load latching)
    Config::Save(engine, test_config);

    // 5. Verify if the preset still exists in the file
    {
        std::ifstream file(test_config);
        std::string line;
        bool found_preset = false;
        while (std::getline(file, line)) {
            if (line.find("[Preset:Imported Car]") != std::string::npos) {
                found_preset = true;
                break;
            }
        }
        file.close();

        if (!found_preset) {
            FAIL_TEST("Preset 'Imported Car' was lost after Save!");
        }
    }

    // Clean up
    if (std::filesystem::exists(test_config)) std::filesystem::remove(test_config);
    std::cout << "[PASS] Issue #371 verification finished." << std::endl;
}

} // namespace FFBEngineTests
