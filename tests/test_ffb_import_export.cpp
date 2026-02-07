#include "test_ffb_common.h"
#include "../src/PresetRegistry.h"
#include <fstream>
#include <cstdio>

namespace FFBEngineTests {

TEST_CASE(test_preset_export_import, "ImportExport") {
    std::cout << "\nTest: Preset Export/Import" << std::endl;
    FFBEngine engine;
    PresetRegistry& registry = PresetRegistry::Get();
    registry.Load("non_existent.ini");

    Preset p("ExportTest", false);
    p.gain = 0.88f;
    registry.AddUserPreset(p.name, engine);
    // Find its index
    int idx = -1;
    const auto& presets = registry.GetPresets();
    for(int i=0; i<(int)presets.size(); i++) if(presets[i].name == "ExportTest") { idx = i; break; }

    registry.ExportPreset(idx, "test_export.ini");
    ASSERT_TRUE(registry.ImportPreset("test_export.ini", engine));

    std::remove("test_export.ini");
}

} // namespace FFBEngineTests
