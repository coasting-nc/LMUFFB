#include "test_ffb_common.h"
#include "../src/ffb/FFBEngine.h"
#include "../src/core/Config.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <csignal>
#include <limits>

namespace FFBEngineTests {

TEST_CASE(test_engine_debug_batch_empty, "Physics") {
    FFBEngine engine;
    auto batch = engine.GetDebugBatch();
    ASSERT_TRUE(batch.empty());
}

TEST_CASE(test_engine_signal_conditioning_thresholds, "Physics") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry();
    FFBCalculationContext ctx;
    ctx.dt = 0.0025;
    ctx.car_speed = 1.0; // Below idle_speed_threshold (default 3.0)

    // Trigger idle_speed_threshold < 3.0 branch
    engine.m_advanced.speed_gate_upper = 2.0f;
    engine.apply_signal_conditioning(1.0, &data, ctx);

    // Trigger wheel_freq <= 1.0 branch
    engine.m_front_axle.flatspot_suppression = true;
    data.mWheel[0].mStaticUndeflectedRadius = 250; // Larger radius -> Lower frequency, avoid overflow
    engine.apply_signal_conditioning(1.0, &data, ctx);
}

TEST_CASE(test_engine_calculate_force_transitions, "Physics") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry();

    // Auto-normalization enabled
    engine.calculate_force(&data, "GT3", "911", 0.0f);

    // seeded transition (changing vehicle class)
    engine.calculate_force(&data, "LMP2", "Oreca", 0.0f);
}

TEST_CASE(test_engine_disabled_effects, "Physics") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry();

    // Disable all effects to hit early returns
    engine.m_braking.abs_pulse_enabled = false;
    engine.m_braking.lockup_enabled = false;
    engine.m_vibration.spin_enabled = false;
    engine.m_vibration.slide_enabled = false;
    engine.m_vibration.road_enabled = false;
    engine.m_vibration.bottoming_enabled = false;
    engine.m_advanced.soft_lock_enabled = false;

    engine.calculate_force(&data, "GT3", "911", 0.0f);
}

TEST_CASE(test_engine_bottoming_fallback, "Physics") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry();
    FFBCalculationContext ctx;
    ctx.dt = 0.0025;

    engine.m_vibration.bottoming_enabled = true;
    engine.m_vibration.bottoming_method = 0;
    data.mWheel[0].mRideHeight = 1.0f; // No bottoming by ride height

    // Trigger safety fallback via raw load peak
    engine.m_static_front_load = 1000.0;
    data.mWheel[0].mTireLoad = 5000.0; // > static * 2.5

    engine.calculate_force(&data, "GT3", "911", 0.0f);
}

TEST_CASE(test_config_malformed_input, "Config") {
    FFBEngine engine;

    // Create config with malformed numeric values
    {
        TestDirectoryGuard temp_dir("tmp_test_malformed");
        std::string test_file = temp_dir.path() + "/malformed_config.toml";
        std::ofstream ofs(test_file);
        ofs << "[General]\ngain = \"abc\"\n[FrontAxle]\nundersteer = \"def\"\n";
        ofs.close();

        Config::Load(engine, test_file);
    }
}

TEST_CASE(test_config_migration_logic_coverage, "Config") {
    FFBEngine engine;

    // Test legacy understeer migration (> 2.0)
    {
        TestDirectoryGuard temp_dir("tmp_test_legacy");
        std::string test_file = temp_dir.path() + "/legacy_config.ini";
        std::ofstream ofs(test_file);
        ofs << "understeer=150.0\nmax_torque_ref=100.0\n";
        ofs.close();

        // Must call MigrateFromLegacyIni directly as Load expects TOML
        Config::MigrateFromLegacyIni(engine, test_file);
        ASSERT_NEAR(engine.m_front_axle.understeer_effect, 1.5, 0.001);
        ASSERT_NEAR(engine.m_general.wheelbase_max_nm, 15.0, 0.001);
        ASSERT_NEAR(engine.m_general.target_rim_nm, 10.0, 0.001);
    }
}

TEST_CASE(test_config_out_of_bounds_indices, "Config") {
    FFBEngine engine;
    Config::LoadPresets();

    // Exercise out-of-bounds indices in various methods
    Config::DeletePreset(-1, engine);
    Config::DeletePreset(1000, engine);

    Config::DuplicatePreset(-1, engine);
    Config::DuplicatePreset(1000, engine);

    Config::ApplyPreset(-1, engine);
    Config::ApplyPreset(1000, engine);

    Config::ExportPreset(-1, "dummy.ini");
    Config::ExportPreset(1000, "dummy.ini");

    ASSERT_FALSE(Config::IsEngineDirtyRelativeToPreset(-1, engine));
    ASSERT_FALSE(Config::IsEngineDirtyRelativeToPreset(1000, engine));
}

TEST_CASE(test_config_import_preset_error, "Config") {
    FFBEngine engine;
    // Non-existent file
    ASSERT_FALSE(Config::ImportPreset("non_existent_preset.ini", engine));
}

TEST_CASE(test_config_exhaustive_keys, "Config") {
    FFBEngine engine;
    // Create config with all possible keys in TOML format to ensure Load() covers them all
    {
        TestDirectoryGuard temp_dir("tmp_test_exhaustive");
        std::string test_file = temp_dir.path() + "/exhaustive_config.toml";
        std::ofstream ofs(test_file);
        ofs << "[System]\n";
        ofs << "app_version = \"1.0\"\n";
        ofs << "always_on_top = true\nlast_device_guid = \"GUID\"\nlast_preset_name = \"Preset\"\n";
        ofs << "win_pos_x = 0\nwin_pos_y = 0\nwin_w_small = 100\nwin_h_small = 100\n";
        ofs << "win_w_large = 200\nwin_h_large = 200\nshow_graphs = true\n";
        ofs << "auto_start_logging = true\nlog_path = \"logs/\"\n";

        ofs << "[General]\n";
        ofs << "gain = 1.0\nmin_force = 0.01\nwheelbase_max_nm = 15.0\ntarget_rim_nm = 10.0\n";
        ofs << "dynamic_normalization_enabled = true\nauto_load_normalization_enabled = true\n";

        ofs << "[FrontAxle]\n";
        ofs << "steering_shaft_gain = 1.0\ningame_ffb_gain = 1.0\nsteering_shaft_smoothing = 0.01\n";
        ofs << "understeer = 0.5\nundersteer_gamma = 1.0\ntorque_source = 0\nsteering_100hz_reconstruction = 0\n";
        ofs << "torque_passthrough = true\nflatspot_suppression = true\nnotch_q = 2.0\nflatspot_strength = 1.0\n";
        ofs << "static_notch_enabled = true\nstatic_notch_freq = 15.0\nstatic_notch_width = 2.0\n";

        ofs << "[RearAxle]\n";
        ofs << "oversteer_boost = 1.0\nsop = 0.5\nsop_scale = 1.0\nsop_smoothing_factor = 0.5\n";
        ofs << "rear_align_effect = 1.0\nkerb_strike_rejection = 0.1\nsop_yaw_gain = 0.5\nyaw_kick_threshold = 0.1\n";
        ofs << "yaw_accel_smoothing = 0.01\nunloaded_yaw_gain = 0.1\nunloaded_yaw_threshold = 0.2\nunloaded_yaw_sens = 1.0\n";
        ofs << "unloaded_yaw_gamma = 0.5\nunloaded_yaw_punch = 0.05\npower_yaw_gain = 0.1\npower_yaw_threshold = 0.2\n";
        ofs << "power_slip_threshold = 0.1\npower_yaw_gamma = 0.5\npower_yaw_punch = 0.05\n";

        ofs << "[LoadForces]\n";
        ofs << "lateral_load_effect = 0.5\nlat_load_transform = 0\nlong_load_effect = 0.5\n";
        ofs << "long_load_smoothing = 0.1\nlong_load_transform = 0\n";

        ofs << "[GripEstimation]\n";
        ofs << "optimal_slip_angle = 0.1\noptimal_slip_ratio = 0.12\nslip_angle_smoothing = 0.01\n";
        ofs << "chassis_inertia_smoothing = 0.01\nload_sensitivity_enabled = true\ngrip_smoothing_steady = 0.01\n";
        ofs << "grip_smoothing_fast = 0.01\ngrip_smoothing_sensitivity = 0.1\n";

        ofs << "[SlopeDetection]\n";
        ofs << "enabled = true\nsg_window = 15\nsensitivity = 1.0\nsmoothing_tau = 0.05\n";
        ofs << "alpha_threshold = 0.02\ndecay_rate = 5.0\nconfidence_enabled = true\nconfidence_max_rate = 0.1\n";
        ofs << "min_threshold = -0.3\nmax_threshold = -2.0\ng_slew_limit = 50.0\nuse_torque = true\ntorque_sensitivity = 0.5\n";

        ofs << "[Braking]\n";
        ofs << "lockup_enabled = true\nlockup_gain = 1.0\nlockup_start_pct = 5.0\nlockup_full_pct = 15.0\n";
        ofs << "lockup_rear_boost = 2.0\nlockup_gamma = 1.0\nlockup_prediction_sens = 50.0\nlockup_bump_reject = 1.0\n";
        ofs << "brake_load_cap = 2.0\nlockup_freq_scale = 1.0\nabs_pulse_enabled = true\nabs_gain = 1.0\nabs_freq = 20.0\n";

        ofs << "[Vibration]\n";
        ofs << "vibration_gain = 1.0\ntexture_load_cap = 2.0\nslide_enabled = true\nslide_gain = 1.0\n";
        ofs << "slide_freq = 1.0\nroad_enabled = true\nroad_gain = 1.0\nspin_enabled = true\nspin_gain = 1.0\n";
        ofs << "spin_freq_scale = 1.0\nscrub_drag_gain = 0.1\nbottoming_enabled = true\nbottoming_gain = 1.0\nbottoming_method = 0\n";

        ofs << "[Advanced]\n";
        ofs << "gyro_gain = 0.5\ngyro_smoothing = 0.01\nstationary_damping = 0.5\nsoft_lock_enabled = true\n";
        ofs << "soft_lock_stiffness = 20.0\nsoft_lock_damping = 0.5\nspeed_gate_lower = 1.0\nspeed_gate_upper = 5.0\n";
        ofs << "rest_api_enabled = true\nrest_api_port = 6397\nroad_fallback_scale = 0.05\nundersteer_affects_sop = false\n";

        ofs << "[Safety]\n";
        ofs << "window_duration = 0.01\ngain_reduction = 0.3\nsmoothing_tau = 0.2\nspike_detection_threshold = 500.0\n";
        ofs << "immediate_spike_threshold = 1500.0\nslew_full_scale_time_s = 1.0\nstutter_safety_enabled = true\nstutter_threshold = 1.5\n";

        ofs << "[CurrentPhysics]\n";
        ofs << "invert_force = false\n";

        ofs.close();

        Config::Load(engine, test_file);
    }
}

TEST_CASE(test_steering_utils_nan_inf, "Physics") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry();
    FFBCalculationContext ctx;

    // NaN steering
    data.mUnfilteredSteering = std::numeric_limits<double>::quiet_NaN();
    FFBEngineTestAccess::CallCalculateSoftLock(engine, &data, ctx);
    ASSERT_NEAR(ctx.soft_lock_force, 0.0, 0.001);

    // Inf steering
    data.mUnfilteredSteering = std::numeric_limits<double>::infinity();
    FFBEngineTestAccess::CallCalculateSoftLock(engine, &data, ctx);
    ASSERT_NEAR(ctx.soft_lock_force, 0.0, 0.001);
}

TEST_CASE(test_async_logger_errors, "Diagnostics") {
    AsyncLogger::Get().Stop();

    SessionInfo info;
    info.vehicle_name = "TestCar";

    // Directory creation failure (using an invalid path or a file that already exists as a directory)
    {
        std::ofstream ofs("dummy_file");
        ofs.close();
        AsyncLogger::Get().Start(info, "dummy_file"); // Should catch filesystem error
        std::remove("dummy_file");
    }

    AsyncLogger::Get().Stop();
}

} // namespace FFBEngineTests

#ifndef _WIN32
extern std::atomic<bool> g_running;
void handle_sigterm(int sig);

namespace FFBEngineTests {
TEST_CASE(test_main_signal_handler, "System") {
    g_running = true;
    handle_sigterm(SIGTERM);
    ASSERT_FALSE(g_running);
}
}
#endif
