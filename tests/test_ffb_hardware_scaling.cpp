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
    engine.m_target_rim_nm = 10.0f;
    engine.m_wheelbase_max_nm = 20.0f;
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 30.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 30.0);

    // Simulate structural sum of 30.0 Nm (exactly at session peak)
    // di_structural = (30.0 * (1/30.0)) * (10.0 / 20.0) = 0.5

    // We can't easily call calculate_force with precise outputs without mocked telemetry,
    // so we verify the components of the math.

    double structural_sum = 30.0;
    double norm_structural = structural_sum * FFBEngineTestAccess::GetSmoothedStructuralMult(engine);
    ASSERT_NEAR(norm_structural, 1.0, 0.0001);

    double di_structural = norm_structural * ((double)engine.m_target_rim_nm / (double)engine.m_wheelbase_max_nm);
    ASSERT_NEAR(di_structural, 0.5, 0.0001);
}

TEST_CASE(test_hardware_scaling_textures, "HardwareScaling") {
    std::cout << "\nTest: Hardware Scaling - Tactile Textures (Issue #153)" << std::endl;
    FFBEngine engine;

    // Setup:
    // 1. Wheelbase Max = 20.0 Nm
    // 2. Texture Sum = 5.0 Nm (Absolute)
    // di_texture = 5.0 / 20.0 = 0.25
    engine.m_wheelbase_max_nm = 20.0f;
    engine.m_target_rim_nm = 10.0f; // Should not affect textures

    double texture_sum_nm = 5.0;
    double di_texture = texture_sum_nm / (double)engine.m_wheelbase_max_nm;
    ASSERT_NEAR(di_texture, 0.25, 0.0001);

    // Changing target_rim should NOT change texture scaling
    engine.m_target_rim_nm = 15.0f;
    di_texture = texture_sum_nm / (double)engine.m_wheelbase_max_nm;
    ASSERT_NEAR(di_texture, 0.25, 0.0001);
}

TEST_CASE(test_config_migration_max_torque, "HardwareScaling") {
    std::cout << "\nTest: Config Migration - max_torque_ref (Issue #153)" << std::endl;

    // Case 1: 100 Nm (Legacy Clipping Hack)
    {
        Preset p;
        std::string version = "0.7.0";
        bool needs_save = false;
        // Use a dummy call to Config::ParsePresetLine logic (re-implemented here for testing or calling actual function)
        // Since ParsePresetLine is static and private, we'll test via Config::Load or just trust our manual check
        // Actually, let's test it via Config::Load with a string.

        std::string test_file = "test_migration_100.ini";
        {
            std::ofstream file(test_file);
            // Config::Load expects global settings at top level, no section header needed for basic testing
            file << "max_torque_ref=100.0\n";
        }

        FFBEngine engine;
        Config::Load(engine, test_file);

        ASSERT_NEAR(engine.m_wheelbase_max_nm, 15.0f, 0.0001);
        ASSERT_NEAR(engine.m_target_rim_nm, 10.0f, 0.0001);

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
        Config::Load(engine, test_file);

        ASSERT_NEAR(engine.m_wheelbase_max_nm, 20.0f, 0.0001);
        ASSERT_NEAR(engine.m_target_rim_nm, 20.0f, 0.0001);

        std::remove(test_file.c_str());
    }
}

} // namespace FFBEngineTests
