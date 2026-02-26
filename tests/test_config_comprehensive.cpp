#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_config_comprehensive_import, "Config") {
    std::cout << "\nTest: Comprehensive Config Import" << std::endl;

    const char* test_file = "tmp_comprehensive_import.ini";
    {
        std::ofstream file(test_file);
        file << "[Preset:Comprehensive]\n";
        file << "ini_version=0.7.82\n";
        file << "gain=1.1\n";
        file << "min_force=0.02\n";
        file << "understeer=0.6\n";
        file << "oversteer_boost=2.5\n";
        file << "sop=0.5\n";
        file << "sop_smoothing=0.1\n";
        file << "sop_scale=1.2\n";
        file << "lockup_enabled=1\n";
        file << "lockup_gain=2.1\n";
        file << "lockup_start_pct=2.0\n";
        file << "lockup_full_pct=6.0\n";
        file << "lockup_rear_boost=0.3\n";
        file << "lockup_gamma=1.5\n";
        file << "lockup_prediction_sens=11.0\n";
        file << "lockup_bump_reject=0.05\n";
        file << "brake_load_cap=9.0\n";
        file << "texture_load_cap=1.6\n";
        file << "abs_pulse_enabled=1\n";
        file << "abs_gain=0.8\n";
        file << "spin_enabled=1\n";
        file << "spin_gain=1.2\n";
        file << "slide_enabled=1\n";
        file << "slide_gain=1.3\n";
        file << "slide_freq=15.0\n";
        file << "road_enabled=1\n";
        file << "road_gain=1.4\n";
        file << "soft_lock_enabled=1\n";
        file << "soft_lock_stiffness=0.9\n";
        file << "soft_lock_damping=0.4\n";
        file << "invert_force=0\n";
        file << "wheelbase_max_nm=18.0\n";
        file << "target_rim_nm=12.0\n";
        file << "abs_freq=22.0\n";
        file << "lockup_freq_scale=1.1\n";
        file << "spin_freq_scale=1.1\n";
        file << "bottoming_method=1\n";
        file << "scrub_drag_gain=0.2\n";
        file << "rear_align_effect=1.1\n";
        file << "sop_yaw_gain=0.6\n";
        file << "steering_shaft_gain=0.9\n";
        file << "ingame_ffb_gain=1.05\n";
        file << "slip_angle_smoothing=0.05\n";
        file << "torque_source=1\n";
        file << "torque_passthrough=1\n";
        file << "gyro_gain=0.4\n";
        file << "flatspot_suppression=1\n";
        file << "notch_q=2.5\n";
        file << "flatspot_strength=0.7\n";
        file << "static_notch_enabled=1\n";
        file << "static_notch_freq=12.0\n";
        file << "static_notch_width=3.0\n";
        file << "yaw_kick_threshold=1.5\n";
        file << "optimal_slip_angle=0.11\n";
        file << "optimal_slip_ratio=0.13\n";
        file << "slope_detection_enabled=1\n";
        file << "slope_sg_window=17\n";
        file << "slope_sensitivity=0.85\n";
        file << "slope_negative_threshold=0.1\n";
        file << "slope_smoothing_tau=0.06\n";
        file << "slope_min_threshold=0.05\n";
        file << "slope_max_threshold=0.95\n";
        file << "slope_alpha_threshold=0.03\n";
        file << "slope_decay_rate=0.4\n";
        file << "slope_confidence_enabled=1\n";
        file << "steering_shaft_smoothing=0.01\n";
        file << "gyro_smoothing_factor=0.02\n";
        file << "yaw_accel_smoothing=0.006\n";
        file << "chassis_inertia_smoothing=0.03\n";
        file << "speed_gate_lower=10.0\n";
        file << "speed_gate_upper=150.0\n";
        file << "road_fallback_scale=0.8\n";
        file << "understeer_affects_sop=1\n";
        file << "slope_g_slew_limit=5.0\n";
        file << "slope_use_torque=1\n";
        file << "slope_torque_sensitivity=0.6\n";
        file << "slope_confidence_max_rate=0.15\n";
        file.close();
    }

    FFBEngine engine;
    InitializeEngine(engine);

    size_t initial_presets = Config::presets.size();
    ASSERT_TRUE(Config::ImportPreset(test_file, engine));
    ASSERT_EQ(Config::presets.size(), initial_presets + 1);

    const Preset& imported = Config::presets.back();
    ASSERT_EQ_STR(imported.name, "Comprehensive");
    ASSERT_NEAR(imported.gain, 1.1f, 0.01);
    ASSERT_NEAR(imported.understeer, 0.6f, 0.01);
    ASSERT_EQ(imported.lockup_enabled, true);
    ASSERT_NEAR(imported.wheelbase_max_nm, 18.0f, 0.01);

    std::remove(test_file);
}

TEST_CASE(test_config_comprehensive_load_v2, "Config") {
    std::cout << "\nTest: Comprehensive Config Load V2" << std::endl;

    const char* test_file = "tmp_comprehensive_v2.ini";
    {
        std::ofstream file(test_file);
        // Global Settings (No section header)
        file << "gain=1.1\n";
        file << "min_force=0.02\n";
        file << "understeer=0.6\n";
        file << "oversteer_boost=2.5\n";
        file << "sop=0.5\n";
        file << "sop_smoothing_factor=0.1\n";
        file << "sop_scale=1.2\n";
        file << "lockup_enabled=1\n";
        file << "lockup_gain=2.1\n";
        file << "lockup_start_pct=2.0\n";
        file << "lockup_full_pct=6.0\n";
        file << "lockup_rear_boost=0.3\n";
        file << "lockup_gamma=1.5\n";
        file << "lockup_prediction_sens=11.0\n";
        file << "lockup_bump_reject=0.05\n";
        file << "brake_load_cap=9.0\n";
        file << "texture_load_cap=1.6\n";
        file << "abs_pulse_enabled=1\n";
        file << "abs_gain=0.8\n";
        file << "spin_enabled=1\n";
        file << "spin_gain=1.2\n";
        file << "slide_enabled=1\n";
        file << "slide_gain=1.3\n";
        file << "slide_freq=15.0\n";
        file << "road_enabled=1\n";
        file << "road_gain=1.4\n";
        file << "soft_lock_enabled=1\n";
        file << "soft_lock_stiffness=0.9\n";
        file << "soft_lock_damping=0.4\n";
        file << "invert_force=0\n";
        file << "wheelbase_max_nm=18.0\n";
        file << "target_rim_nm=12.0\n";
        file << "abs_freq=22.0\n";
        file << "lockup_freq_scale=1.1\n";
        file << "spin_freq_scale=1.1\n";
        file << "bottoming_method=1\n";
        file << "scrub_drag_gain=0.2\n";
        file << "rear_align_effect=1.1\n";
        file << "sop_yaw_gain=0.6\n";
        file << "steering_shaft_gain=0.9\n";
        file << "ingame_ffb_gain=1.05\n";
        file << "slip_angle_smoothing=0.05\n";
        file << "torque_source=1\n";
        file << "torque_passthrough=1\n";
        file << "gyro_gain=0.4\n";
        file << "flatspot_suppression=1\n";
        file << "notch_q=2.5\n";
        file << "flatspot_strength=0.7\n";
        file << "static_notch_enabled=1\n";
        file << "static_notch_freq=12.0\n";
        file << "static_notch_width=3.0\n";
        file << "yaw_kick_threshold=1.5\n";
        file << "optimal_slip_angle=0.11\n";
        file << "optimal_slip_ratio=0.13\n";
        file << "slope_detection_enabled=1\n";
        file << "slope_sg_window=17\n";
        file << "slope_sensitivity=0.85\n";
        file << "slope_negative_threshold=0.1\n";
        file << "slope_smoothing_tau=0.06\n";
        file << "slope_min_threshold=0.05\n";
        file << "slope_max_threshold=0.95\n";
        file << "slope_alpha_threshold=0.03\n";
        file << "slope_decay_rate=0.4\n";
        file << "slope_confidence_enabled=1\n";
        file << "steering_shaft_smoothing=0.01\n";
        file << "gyro_smoothing_factor=0.02\n";
        file << "yaw_accel_smoothing=0.006\n";
        file << "chassis_inertia_smoothing=0.03\n";
        file << "speed_gate_lower=10.0\n";
        file << "speed_gate_upper=150.0\n";
        file << "road_fallback_scale=0.8\n";
        file << "understeer_affects_sop=1\n";
        file << "slope_g_slew_limit=5.0\n";
        file << "slope_use_torque=1\n";
        file << "slope_torque_sensitivity=0.6\n";
        file << "slope_confidence_max_rate=0.15\n";

        file << "always_on_top=1\n";
        file << "dark_mode=1\n";
        file << "start_minimized=1\n";
        file << "check_updates=1\n";
        file << "last_preset_name=Comprehensive\n";

        file << "[StaticLoads]\n";
        file << "Ferrari 488 GTE=4200.5\n";

        file.close();
    }

    FFBEngine engine;
    InitializeEngine(engine);
    Config::Load(engine, test_file);

    // Verify some key fields
    ASSERT_NEAR(engine.m_gain, 1.1f, 0.01);
    ASSERT_NEAR(engine.m_understeer_effect, 0.6f, 0.01);
    ASSERT_EQ(engine.m_lockup_enabled, true);
    ASSERT_NEAR(engine.m_wheelbase_max_nm, 18.0f, 0.01);
    ASSERT_EQ(Config::m_always_on_top, true);

    double saved_load;
    ASSERT_TRUE(Config::GetSavedStaticLoad("Ferrari 488 GTE", saved_load));
    ASSERT_NEAR(saved_load, 4200.5, 0.001);

    std::remove(test_file);
}

} // namespace FFBEngineTests
