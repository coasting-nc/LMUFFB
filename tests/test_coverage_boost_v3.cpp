#include "test_ffb_common.h"
#include "../src/FFBEngine.h"
#include "../src/Config.h"
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
    engine.m_speed_gate_upper = 2.0f;
    engine.apply_signal_conditioning(1.0, &data, ctx);

    // Trigger wheel_freq <= 1.0 branch
    engine.m_flatspot_suppression = true;
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
    engine.m_abs_pulse_enabled = false;
    engine.m_lockup_enabled = false;
    engine.m_spin_enabled = false;
    engine.m_slide_texture_enabled = false;
    engine.m_road_texture_enabled = false;
    engine.m_bottoming_enabled = false;
    engine.m_soft_lock_enabled = false;

    engine.calculate_force(&data, "GT3", "911", 0.0f);
}

TEST_CASE(test_engine_bottoming_fallback, "Physics") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry();
    FFBCalculationContext ctx;
    ctx.dt = 0.0025;

    engine.m_bottoming_enabled = true;
    engine.m_bottoming_method = 0;
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
        std::ofstream ofs("malformed_config.ini");
        ofs << "gain=abc\nundersteer=def\n";
        ofs.close();

        Config::Load(engine, "malformed_config.ini");
        std::remove("malformed_config.ini");
    }
}

TEST_CASE(test_config_migration_logic, "Config") {
    FFBEngine engine;

    // Test legacy understeer migration (> 2.0)
    {
        std::ofstream ofs("legacy_config.ini");
        ofs << "understeer=150.0\nmax_torque_ref=100.0\n";
        ofs.close();

        Config::Load(engine, "legacy_config.ini");
        ASSERT_NEAR(engine.m_understeer_effect, 1.5, 0.001);
        ASSERT_NEAR(engine.m_wheelbase_max_nm, 15.0, 0.001);
        ASSERT_NEAR(engine.m_target_rim_nm, 10.0, 0.001);

        std::remove("legacy_config.ini");
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
    // Create config with all possible keys to ensure Load() covers them all
    {
        std::ofstream ofs("exhaustive_config.ini");
        ofs << "ini_version=1.0\n";
        ofs << "always_on_top=1\nlast_device_guid=GUID\nlast_preset_name=Preset\n";
        ofs << "win_pos_x=0\nwin_pos_y=0\nwin_w_small=100\nwin_h_small=100\n";
        ofs << "win_w_large=200\nwin_h_large=200\nshow_graphs=1\n";
        ofs << "auto_start_logging=1\nlog_path=logs/\n";
        ofs << "gain=1.0\nsop_smoothing_factor=0.5\nsop_scale=1.0\n";
        ofs << "slip_angle_smoothing=0.01\ntexture_load_cap=2.0\nmax_load_factor=2.0\n";
        ofs << "brake_load_cap=2.0\nsmoothing=0.5\nundersteer=0.5\n";
        ofs << "base_force_mode=0\ntorque_source=0\ntorque_passthrough=true\n";
        ofs << "sop=0.5\nmin_force=0.01\noversteer_boost=1.0\ndynamic_weight_gain=0.5\n";
        ofs << "dynamic_weight_smoothing=0.1\ngrip_smoothing_steady=0.01\n";
        ofs << "grip_smoothing_fast=0.01\ngrip_smoothing_sensitivity=0.1\n";
        ofs << "lockup_enabled=1\nlockup_gain=1.0\nlockup_start_pct=5.0\n";
        ofs << "lockup_full_pct=15.0\nlockup_rear_boost=2.0\nlockup_gamma=1.0\n";
        ofs << "lockup_prediction_sens=50.0\nlockup_bump_reject=1.0\n";
        ofs << "abs_pulse_enabled=1\nabs_gain=1.0\nspin_enabled=1\nspin_gain=1.0\n";
        ofs << "slide_enabled=1\nslide_gain=1.0\nslide_freq=1.0\n";
        ofs << "road_enabled=1\nroad_gain=1.0\nsoft_lock_enabled=1\n";
        ofs << "soft_lock_stiffness=20.0\nsoft_lock_damping=0.5\ninvert_force=0\n";
        ofs << "wheelbase_max_nm=15.0\ntarget_rim_nm=10.0\nmax_torque_ref=15.0\n";
        ofs << "abs_freq=20.0\nlockup_freq_scale=1.0\nspin_freq_scale=1.0\n";
        ofs << "bottoming_method=0\nscrub_drag_gain=0.1\nrear_align_effect=1.0\n";
        ofs << "sop_yaw_gain=0.5\nsteering_shaft_gain=1.0\ningame_ffb_gain=1.0\n";
        ofs << "gyro_gain=0.5\nflatspot_suppression=1\nnotch_q=2.0\n";
        ofs << "flatspot_strength=1.0\nstatic_notch_enabled=1\nstatic_notch_freq=15.0\n";
        ofs << "static_notch_width=2.0\nyaw_kick_threshold=0.1\noptimal_slip_angle=0.1\n";
        ofs << "optimal_slip_ratio=0.12\nslope_detection_enabled=1\nslope_sg_window=15\n";
        ofs << "slope_sensitivity=1.0\nslope_negative_threshold=-0.5\nslope_smoothing_tau=0.05\n";
        ofs << "slope_min_threshold=-0.3\nslope_max_threshold=-2.0\nslope_alpha_threshold=0.02\n";
        ofs << "slope_decay_rate=5.0\nslope_confidence_enabled=1\nsteering_shaft_smoothing=0.01\n";
        ofs << "gyro_smoothing_factor=0.01\nyaw_accel_smoothing=0.01\nchassis_inertia_smoothing=0.01\n";
        ofs << "speed_gate_lower=1.0\nspeed_gate_upper=5.0\nroad_fallback_scale=0.05\n";
        ofs << "understeer_affects_sop=0\nslope_g_slew_limit=50.0\nslope_use_torque=1\n";
        ofs << "slope_torque_sensitivity=0.5\nslope_confidence_max_rate=0.1\n";
        ofs.close();

        Config::Load(engine, "exhaustive_config.ini");
        std::remove("exhaustive_config.ini");
    }
}

TEST_CASE(test_steering_utils_nan_inf, "Physics") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry();
    FFBCalculationContext ctx;

    // NaN steering
    data.mUnfilteredSteering = std::numeric_limits<float>::quiet_NaN();
    FFBEngineTestAccess::CallCalculateSoftLock(engine, &data, ctx);
    ASSERT_NEAR(ctx.soft_lock_force, 0.0, 0.001);

    // Inf steering
    data.mUnfilteredSteering = std::numeric_limits<float>::infinity();
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
