#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_issue_211_config_gain_migration, "Migration") {
    std::cout << "\nTest: Issue #211 - Config Gain Migration (100Nm hack)" << std::endl;

    FFBEngine engine;
    InitializeEngine(engine);

    const char* test_file = "tmp_issue_211.ini";
    {
        std::ofstream file(test_file);
        file << "max_torque_ref=100.0\n";
        file << "gain=0.8\n";
        file.close();
    }

    // Direct migration call
    Config::MigrateFromLegacyIni(engine, test_file);

    // Expected: 0.8 * (15.0 / 100.0) = 0.12
    ASSERT_NEAR(engine.m_general.gain, 0.12f, 0.001);
    ASSERT_NEAR(engine.m_general.wheelbase_max_nm, 15.0f, 0.001);

    std::remove(test_file);
}

} // namespace FFBEngineTests
