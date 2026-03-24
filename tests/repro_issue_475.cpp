#include "test_ffb_common.h"
#include "Config.h"
#include "ffb/FFBConfig.h"
#include <filesystem>
#include <fstream>

namespace FFBEngineTests {

TEST_CASE(repro_issue_475_lateral_load_clamping, "Config") {
    std::cout << "\nReproduction: Issue #475 - Lateral Load clamping on reload" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    // 1. Set Lateral Load to a value > 2.0 (GUI allows up to 10.0)
    float test_val = 5.0f;
    engine.m_load_forces.lat_load_effect = test_val;

    TestDirectoryGuard temp_dir("tmp_repro_475");
    std::string test_file = temp_dir.path() + "/config_repro.toml";

    // 2. Save config
    Config::Save(engine, test_file);

    // 3. Verify it was saved correctly in TOML (as 5.0)
    {
        std::ifstream ifs(test_file);
        std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        ASSERT_TRUE(content.find("lat_load_effect = 5.0") != std::string::npos);
    }

    // 4. Load config back into a clean engine
    FFBEngine engine2;
    InitializeEngine(engine2);
    Config::Load(engine2, test_file);

    // 5. Check value. Expecting 5.0, but issue reports it's not preserved (likely clamped to 2.0)
    std::cout << "Original set value: " << test_val << std::endl;
    std::cout << "Value after reload: " << engine2.m_load_forces.lat_load_effect << std::endl;

    // This assertion is expected to FAIL if the bug is present
    ASSERT_NEAR(engine2.m_load_forces.lat_load_effect, test_val, 0.0001f);
}

} // namespace FFBEngineTests
