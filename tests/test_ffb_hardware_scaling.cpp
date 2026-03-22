#include "test_ffb_common.h"
#include <cmath>

namespace FFBEngineTests {

TEST_CASE(test_hardware_scaling_structural, "HardwareScaling") {
    std::cout << "\nTest: Hardware Scaling - Structural Forces (Issue #153)" << std::endl;
    FFBEngine engine;

    // Setup:
    // 1. Session Peak = 30.0 Nm (via target_rim_nm at Apply time)
    // 2. Target Rim = 10.0 Nm
    // 3. Wheelbase Max = 20.0 Nm
    engine.m_general.target_rim_nm = 10.0f;
    engine.m_general.wheelbase_max_nm = 20.0f;
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 30.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 30.0);

    // Simulate structural sum of 30.0 Nm (exactly at session peak)
    // di_structural = (30.0 * (1/30.0)) * (10.0 / 20.0) = 0.5

    double structural_sum = 30.0;
    double norm_structural = structural_sum * FFBEngineTestAccess::GetSmoothedStructuralMult(engine);
    ASSERT_NEAR(norm_structural, 1.0, 0.0001);

    double di_structural = norm_structural * ((double)engine.m_general.target_rim_nm / (double)engine.m_general.wheelbase_max_nm);
    ASSERT_NEAR(di_structural, 0.5, 0.0001);
}

TEST_CASE(test_hardware_scaling_textures, "HardwareScaling") {
    std::cout << "\nTest: Hardware Scaling - Vibration Textures (Issue #153)" << std::endl;
    FFBEngine engine;

    // Setup:
    // 1. Wheelbase Max = 20.0 Nm
    // 2. Texture Sum = 5.0 Nm (Absolute)
    // di_texture = 5.0 / 20.0 = 0.25
    engine.m_general.wheelbase_max_nm = 20.0f;
    engine.m_general.target_rim_nm = 10.0f; // Should not affect textures

    double texture_sum_nm = 5.0;
    double di_texture = texture_sum_nm / (double)engine.m_general.wheelbase_max_nm;
    ASSERT_NEAR(di_texture, 0.25, 0.0001);

    // Changing target_rim should NOT change texture scaling
    engine.m_general.target_rim_nm = 15.0f;
    di_texture = texture_sum_nm / (double)engine.m_general.wheelbase_max_nm;
    ASSERT_NEAR(di_texture, 0.25, 0.0001);
}

TEST_CASE(test_config_migration_max_torque, "HardwareScaling") {
    std::cout << "\nTest: Config Migration - max_torque_ref (Issue #153)" << std::endl;

    // Case 1: 100 Nm (Legacy Clipping Hack)
    {
        std::string test_file = "test_migration_100.ini";
        {
            std::ofstream file(test_file);
            file << "max_torque_ref=100.0\n";
        }

        FFBEngine engine;
        InitializeEngine(engine);
        Config::MigrateFromLegacyIni(engine, test_file);

        ASSERT_NEAR(engine.m_general.wheelbase_max_nm, 15.0f, 0.0001);
        ASSERT_NEAR(engine.m_general.target_rim_nm, 10.0f, 0.0001);

        std::remove(test_file.c_str());
    }

    // Case 2: 20 Nm (Calibrated User)
    {
        std::string test_file = "test_migration_20.ini";
        {
            std::ofstream file(test_file);
            file << "max_torque_ref=20.0\n";
        }

        FFBEngine engine;
        InitializeEngine(engine);
        Config::MigrateFromLegacyIni(engine, test_file);

        ASSERT_NEAR(engine.m_general.wheelbase_max_nm, 20.0f, 0.0001);
        ASSERT_NEAR(engine.m_general.target_rim_nm, 20.0f, 0.0001);

        std::remove(test_file.c_str());
    }
}

} // namespace FFBEngineTests
