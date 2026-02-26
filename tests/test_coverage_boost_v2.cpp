#include "test_ffb_common.h"
#include "../src/Config.h"
#include "../src/GameConnector.h"
#include "../src/DirectInputFFB.h"
#include <iostream>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>

#ifndef _WIN32
#include "../src/lmu_sm_interface/LinuxMock.h"
#endif

namespace FFBEngineTests {

TEST_CASE(test_engine_branch_boost, "Physics") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry();

    // 1. calculate_force with data = nullptr
    ASSERT_NEAR(engine.calculate_force(nullptr), 0.0, 0.001);

    // 2. calculate_force with NaN torque
    data.mSteeringShaftTorque = std::numeric_limits<float>::quiet_NaN();
    ASSERT_NEAR(engine.calculate_force(&data, "GT3", "911", 0.0f), 0.0, 0.001);
    data.mSteeringShaftTorque = 0.1f;

    // 3. calculate_force with dt <= 0
    data.mDeltaTime = 0.0f;
    engine.calculate_force(&data, "GT3", "911", 0.0f);

    // Trigger the other branch of dt check
    data.mDeltaTime = 0.0025f;
    engine.calculate_force(&data, "GT3", "911", 0.0f);

    // 4. In-Game FFB source (m_torque_source = 1)
    FFBEngineTestAccess::SetTorqueSource(engine, 1);
    engine.m_wheelbase_max_nm = 10.0f;
    engine.calculate_force(&data, "GT3", "911", 0.5f);
    FFBEngineTestAccess::SetTorqueSource(engine, 0);

    // 5. Allowed = false (Stationary/Garage Soft Lock)
    FFBEngineTestAccess::SetSoftLockEnabled(engine, true);
    engine.calculate_force(&data, "GT3", "911", 0.0f, false);

    // 6. Invert Force
    FFBEngineTestAccess::SetInvertForce(engine, true);
    engine.calculate_force(&data, "GT3", "911", 0.1f);
    FFBEngineTestAccess::SetInvertForce(engine, false);

    // 7. Min Force clamping
    FFBEngineTestAccess::SetMinForce(engine, 0.1f);
    engine.m_gain = 1.0f;
    engine.m_steering_shaft_gain = 1.0f;
    data.mSteeringShaftTorque = 0.001f;
    data.mDeltaTime = 0.0025f;
    double res = engine.calculate_force(&data, "GT3", "911", 0.0f);
    ASSERT_NEAR(std::abs(res), 0.1, 0.05);

    // 8. Class Seeding (m_current_class_name change)
    engine.calculate_force(&data, nullptr, nullptr, 0.0f);
    engine.calculate_force(&data, "GT3", "911", 0.0f);
    engine.calculate_force(&data, "GT3", "911", 0.0f);
    engine.calculate_force(&data, "LMP2", "Oreca", 0.0f);
    engine.calculate_force(&data, "GTE", "Ferrari", 0.0f);

    // 9. Telemetry Fallbacks (m_missing_..._frames)
    // a. Missing Load Fallback
    data.mLocalVel.z = 10.0f;
    data.mWheel[0].mTireLoad = 0.0f;
    data.mWheel[1].mTireLoad = 0.0f;
    for (int i = 0; i < 25; i++) {
        engine.calculate_force(&data, "GT3", "911", 0.0f);
    }

    // b. Missing Susp Force
    data.mWheel[0].mSuspForce = 0.0f;
    data.mWheel[1].mSuspForce = 0.0f;
    for (int i = 0; i < 55; i++) {
        engine.calculate_force(&data, "GT3", "911", 0.0f);
    }

    // c. Missing Susp Deflection
    data.mWheel[0].mSuspensionDeflection = 0.0f;
    data.mWheel[1].mSuspensionDeflection = 0.0f;
    for (int i = 0; i < 55; i++) {
        engine.calculate_force(&data, "GT3", "911", 0.0f);
    }

    // d. Missing Lat Force
    data.mLocalAccel.x = 5.0f;
    data.mWheel[0].mLateralForce = 0.0f;
    data.mWheel[1].mLateralForce = 0.0f;
    for (int i = 0; i < 55; i++) {
        engine.calculate_force(&data, "GT3", "911", 0.0f);
    }

    // e. Missing Vert Deflection
    data.mWheel[0].mVerticalTireDeflection = 0.0f;
    data.mWheel[1].mVerticalTireDeflection = 0.0f;
    for (int i = 0; i < 55; i++) {
        engine.calculate_force(&data, "GT3", "911", 0.0f);
    }

    // 10. calculate_abs_pulse
    FFBEngineTestAccess::SetABSPulseEnabled(engine, false);
    engine.calculate_force(&data, "GT3", "911", 0.0f);
    FFBEngineTestAccess::SetABSPulseEnabled(engine, true);
    data.mUnfilteredBrake = 0.8f;
    data.mWheel[0].mBrakePressure = 1.0f;
    engine.calculate_force(&data, "GT3", "911", 0.0f);

    // 11. calculate_lockup_vibration
    FFBEngineTestAccess::SetLockupEnabled(engine, false);
    engine.calculate_force(&data, "GT3", "911", 0.0f);
    FFBEngineTestAccess::SetLockupEnabled(engine, true);
    data.mWheel[0].mRotation = 0.0f; // Full lock
    engine.calculate_force(&data, "GT3", "911", 0.0f);

    // Rear lockup
    data.mWheel[0].mRotation = 100.0f;
    data.mWheel[2].mRotation = 0.0f; // Rear lock
    engine.calculate_force(&data, "GT3", "911", 0.0f);
}

TEST_CASE(test_config_branch_boost, "Config") {
    FFBEngine engine;

    // 1. Config::Load with non-existent file
    Config::Load(engine, "non_existent_file_12345.ini");

    // 2. Comprehensive Load
    {
        std::ofstream ofs("mega_config.ini");
        ofs << "[System]\nini_version=0.7.82\nalways_on_top=1\nshow_graphs=1\n";
        ofs << "win_pos_x=100\nwin_pos_y=100\nwin_w_small=500\nwin_h_small=600\n";
        ofs << "win_w_large=1000\nwin_h_large=800\nlast_preset_name=Default\n";
        ofs << "last_device_guid={12345678-1234-1234-1234-1234567890AB}\n";
        ofs << "auto_start_logging=1\nlog_path=./logs\n";
        ofs << "[Tuning]\ngain=1.0\nundersteer=0.5\nsop=1.0\nsop_scale=1.0\n";
        ofs << "min_force=0.01\noversteer_boost=2.0\ndynamic_weight_gain=0.1\n";
        ofs << "dynamic_weight_smoothing=0.1\ngrip_smoothing_steady=0.02\n";
        ofs << "grip_smoothing_fast=0.001\ngrip_smoothing_sensitivity=0.05\n";
        ofs << "lockup_enabled=1\nlockup_gain=0.5\nlockup_start_pct=5.0\n";
        ofs << "lockup_full_pct=15.0\nlockup_rear_boost=1.5\nlockup_gamma=2.0\n";
        ofs << "lockup_prediction_sens=50.0\nlockup_bump_reject=1.0\n";
        ofs << "brake_load_cap=2.0\ntexture_load_cap=1.5\nabs_pulse_enabled=1\n";
        ofs << "abs_gain=1.0\nabs_freq=20.0\nspin_enabled=1\nspin_gain=0.5\n";
        ofs << "spin_freq_scale=1.0\nslide_enabled=1\nslide_gain=0.5\nslide_freq=1.0\n";
        ofs << "road_enabled=1\nroad_gain=0.5\nsoft_lock_enabled=1\n";
        ofs << "soft_lock_stiffness=20.0\nsoft_lock_damping=0.5\ninvert_force=1\n";
        ofs << "wheelbase_max_nm=15.0\ntarget_rim_nm=10.0\nlockup_freq_scale=1.0\n";
        ofs << "bottoming_method=1\nscrub_drag_gain=0.1\nrear_align_effect=0.5\n";
        ofs << "sop_yaw_gain=0.3\ngyro_gain=0.2\nsteering_shaft_gain=1.0\n";
        ofs << "ingame_ffb_gain=1.0\nbase_force_mode=0\ntorque_source=1\n";
        ofs << "torque_passthrough=0\nflatspot_suppression=1\nnotch_q=2.0\n";
        ofs << "flatspot_strength=0.8\nstatic_notch_enabled=1\nstatic_notch_freq=15.0\n";
        ofs << "static_notch_width=3.0\nyaw_kick_threshold=0.1\nspeed_gate_lower=2.0\n";
        ofs << "speed_gate_upper=10.0\nroad_fallback_scale=0.1\nundersteer_affects_sop=1\n";
        ofs << "slope_detection_enabled=1\nslope_sg_window=15\nslope_sensitivity=1.0\n";
        ofs << "slope_smoothing_tau=0.05\nslope_alpha_threshold=0.03\nslope_decay_rate=6.0\n";
        ofs << "slope_confidence_enabled=1\nslope_min_threshold=-0.5\nslope_max_threshold=-3.0\n";
        ofs << "slope_g_slew_limit=60.0\nslope_use_torque=1\nslope_torque_sensitivity=0.6\n";
        ofs << "slope_confidence_max_rate=0.15\nsmoothing=0.9\n";
        ofs << "steering_shaft_smoothing=0.05\ngyro_smoothing_factor=0.01\n";
        ofs << "yaw_accel_smoothing=0.02\nchassis_inertia_smoothing=0.03\n";
        ofs << "[Presets]\nPreset1=mega_config.ini\n";
        ofs << "[StaticLoads]\nCar1=4000.0\n";
        ofs.close();
        Config::Load(engine, "mega_config.ini");
        std::remove("mega_config.ini");
    }

    // 3. Config::ApplyPreset with out-of-bounds index
    Config::LoadPresets();
    int initial_size = (int)Config::presets.size();
    Config::ApplyPreset(-1, engine);
    Config::ApplyPreset(initial_size + 10, engine);

    // 4. Config::DuplicatePreset with counter loop
    Config::presets.clear();
    Preset p;
    p.name = "MyPreset";
    Config::presets.push_back(p);

    Preset p_copy = p;
    p_copy.name = "MyPreset (Copy)";
    Config::presets.push_back(p_copy);

    Preset p_copy1 = p;
    p_copy1.name = "MyPreset (Copy) 1";
    Config::presets.push_back(p_copy1);

    Config::DuplicatePreset(0, engine);
    ASSERT_EQ_STR(Config::presets.back().name.c_str(), "MyPreset (Copy) 2");

    // 5. Config::DeletePreset on builtin
    Config::presets[0].is_builtin = true;
    size_t size_before = Config::presets.size();
    Config::DeletePreset(0, engine);
    ASSERT_EQ((int)Config::presets.size(), (int)size_before);

    // 6. Config::SetSavedStaticLoad and GetSavedStaticLoad
    Config::SetSavedStaticLoad("TestCar", 1234.5);
    double val = 0.0;
    ASSERT_TRUE(Config::GetSavedStaticLoad("TestCar", val));
    ASSERT_NEAR(val, 1234.5, 0.001);
    ASSERT_FALSE(Config::GetSavedStaticLoad("UnknownCar", val));
}

TEST_CASE(test_game_connector_branch_boost, "System") {
    GameConnector& conn = GameConnector::Get();
    conn.Disconnect();

    #ifndef _WIN32
    MockSM::GetMaps()["LMU_Data"].resize(sizeof(SharedMemoryLayout));
    SharedMemoryLayout* layout = (SharedMemoryLayout*)MockSM::GetMaps()["LMU_Data"].data();
    layout->data.generic.appInfo.mAppWindow = (HWND)3;

    conn.TryConnect();
    ASSERT_FALSE(conn.IsConnected());

    layout->data.generic.appInfo.mAppWindow = (HWND)1;
    conn.TryConnect();
    layout->data.telemetry.playerHasVehicle = false;
    SharedMemoryObjectOut dest;
    conn.CopyTelemetry(dest);
    ASSERT_FALSE(dest.telemetry.playerHasVehicle);

    #endif
}

TEST_CASE(test_direct_input_branch_boost, "System") {
    DirectInputFFB& di = DirectInputFFB::Get();

    GUID g1 = di.StringToGuid("no-braces");
    ASSERT_TRUE(g1.Data1 == 0);

    GUID g2 = di.StringToGuid("{too-short}");
    ASSERT_TRUE(g2.Data1 == 0);

    GUID g3 = di.StringToGuid("{ZZZZZZZZ-ZZZZ-ZZZZ-ZZZZ-ZZZZZZZZZZZZ}");
    ASSERT_TRUE(g3.Data1 == 0);
}

} // namespace FFBEngineTests
