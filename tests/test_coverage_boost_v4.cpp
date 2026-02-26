#include "test_ffb_common.h"
#include "../src/FFBEngine.h"
#include "../src/Config.h"
#include "../src/AsyncLogger.h"
#include "../src/GuiLayer.h"
#ifdef ENABLE_IMGUI
#include "imgui.h"
#endif
#include <iostream>
#include <cmath>
#include <vector>
#include <limits>
#include <fstream>
#include <filesystem>

class GuiLayerTestAccess {
public:
    static void DrawTuningWindow(FFBEngine& engine) { GuiLayer::DrawTuningWindow(engine); }
    static void DrawDebugWindow(FFBEngine& engine) { GuiLayer::DrawDebugWindow(engine); }
};

namespace FFBEngineTests {

TEST_CASE(test_config_legacy_migrations, "Config") {
    FFBEngine engine;

    // 1. Understeer > 2.0 migration
    {
        const char* f = "tmp_legacy_understeer.ini";
        std::ofstream ofs(f);
        ofs << "understeer=150.0\n";
        ofs.close();
        Config::Load(engine, f);
        ASSERT_NEAR(engine.m_understeer_effect, 1.5, 0.001);
        std::remove(f);
    }

    // 2. max_torque_ref > 40.0 migration
    {
        const char* f = "tmp_legacy_torque_high.ini";
        std::ofstream ofs(f);
        ofs << "max_torque_ref=50.0\n";
        ofs.close();
        Config::Load(engine, f);
        ASSERT_NEAR(engine.m_wheelbase_max_nm, 15.0, 0.001);
        ASSERT_NEAR(engine.m_target_rim_nm, 10.0, 0.001);
        std::remove(f);
    }

    // 3. max_torque_ref <= 40.0 migration
    {
        const char* f = "tmp_legacy_torque_low.ini";
        std::ofstream ofs(f);
        ofs << "max_torque_ref=20.0\n";
        ofs.close();
        Config::Load(engine, f);
        ASSERT_NEAR(engine.m_wheelbase_max_nm, 20.0, 0.001);
        ASSERT_NEAR(engine.m_target_rim_nm, 20.0, 0.001);
        std::remove(f);
    }
}

TEST_CASE(test_config_invalid_validation, "Config") {
    FFBEngine engine;

    // 1. optimal_slip_angle < 0.01 validation
    {
        const char* f = "tmp_invalid_slip.ini";
        std::ofstream ofs(f);
        ofs << "optimal_slip_angle=0.005\n";
        ofs.close();
        Config::Load(engine, f);
        ASSERT_NEAR(engine.m_optimal_slip_angle, 0.10, 0.001);
        std::remove(f);
    }

    // 2. slope_sg_window validation (min 5, max 41, must be odd)
    {
        const char* f = "tmp_invalid_slope_win.ini";
        std::ofstream ofs(f);
        ofs << "slope_sg_window=4\n"; // Should be clamped to 5
        ofs.close();
        Config::Load(engine, f);
        ASSERT_EQ(engine.m_slope_sg_window, 5);
        std::remove(f);
    }
    {
        const char* f = "tmp_invalid_slope_win_even.ini";
        std::ofstream ofs(f);
        ofs << "slope_sg_window=10\n"; // Should be made odd -> 11
        ofs.close();
        Config::Load(engine, f);
        ASSERT_EQ(engine.m_slope_sg_window, 11);
        std::remove(f);
    }
}

TEST_CASE(test_engine_ffb_allowed, "Physics") {
    FFBEngine engine;
    VehicleScoringInfoV01 scoring;
    memset(&scoring, 0, sizeof(scoring));

    // 1. Not player
    scoring.mIsPlayer = false;
    ASSERT_FALSE(engine.IsFFBAllowed(scoring, 5));

    // 2. AI controlled
    scoring.mIsPlayer = true;
    scoring.mControl = 1; // 1 = AI
    ASSERT_FALSE(engine.IsFFBAllowed(scoring, 5));

    // 3. DQ status
    scoring.mIsPlayer = true;
    scoring.mControl = 0; // Local player
    scoring.mFinishStatus = 3; // 3 = DQ
    ASSERT_FALSE(engine.IsFFBAllowed(scoring, 5));

    // 4. Allowed
    scoring.mFinishStatus = 0; // None
    ASSERT_TRUE(engine.IsFFBAllowed(scoring, 5));
}

TEST_CASE(test_engine_safety_slew_edge, "Physics") {
    FFBEngine engine;

    // 1. NaN/Inf input
    ASSERT_NEAR(engine.ApplySafetySlew(std::numeric_limits<double>::quiet_NaN(), 0.0025, false), 0.0, 0.001);
    ASSERT_NEAR(engine.ApplySafetySlew(std::numeric_limits<double>::infinity(), 0.0025, false), 0.0, 0.001);

    // 2. Restricted slew (e.g. after finish)
    engine.m_last_output_force = 0.0;
    // target = 1.0, max_slew = 100.0, dt = 0.0025 -> max_change = 0.25
    double force = engine.ApplySafetySlew(1.0, 0.0025, true);
    ASSERT_NEAR(force, 0.25, 0.001);
}

TEST_CASE(test_engine_calculate_force_fallbacks, "Physics") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry();
    InitializeEngine(engine);

    // 1. Trigger missing load fallback (m_missing_load_frames > 20)
    data.mWheel[0].mTireLoad = 0.0;
    data.mWheel[1].mTireLoad = 0.0;
    data.mLocalVel.z = 10.0; // speed > 1.0
    for (int i = 0; i < 25; i++) {
        engine.calculate_force(&data, "GT3", "911", 0.0f);
    }
    // Speed <= 1.0 to hit false branch
    data.mLocalVel.z = 0.0;
    engine.calculate_force(&data, "GT3", "911", 0.0f);
    data.mLocalVel.z = 10.0;

    // 2. Trigger missing suspension force warning
    data.mWheel[0].mSuspForce = 0.0;
    data.mWheel[1].mSuspForce = 0.0;
    for (int i = 0; i < 55; i++) {
        engine.calculate_force(&data, "GT3", "911", 0.0f);
    }
    // Hit false branches
    data.mWheel[0].mSuspForce = 1000.0;
    engine.calculate_force(&data, "GT3", "911", 0.0f);
    data.mWheel[0].mSuspForce = 0.0;

    // 3. Trigger missing suspension deflection warning
    data.mWheel[0].mSuspensionDeflection = 0.0;
    data.mWheel[1].mSuspensionDeflection = 0.0;
    for (int i = 0; i < 55; i++) {
        engine.calculate_force(&data, "GT3", "911", 0.0f);
    }
    // Hit false branch
    data.mWheel[0].mSuspensionDeflection = 0.01;
    engine.calculate_force(&data, "GT3", "911", 0.0f);
}

TEST_CASE(test_engine_calculate_force_not_allowed, "Physics") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry();
    InitializeEngine(engine);

    // When allowed=false, most forces should be zeroed except Soft Lock
    engine.m_soft_lock_enabled = true;
    data.mUnfilteredSteering = 1.1f; // Trigger soft lock
    engine.m_soft_lock_stiffness = 10.0f;

    double force = engine.calculate_force(&data, "GT3", "911", 0.0f, false);
    ASSERT_TRUE(std::abs(force) > 0.0); // Soft lock force should still be present
}

TEST_CASE(test_engine_extra_branches, "Physics") {
    FFBEngine engine;
    TelemInfoV01 data = CreateBasicTestTelemetry();
    InitializeEngine(engine);

    // 1. GetDebugBatch when NOT empty and buffer size limit
    auto batch_empty = engine.GetDebugBatch(); // Hit empty branch
    ASSERT_TRUE(batch_empty.empty());

    for (int i = 0; i < 110; i++) {
        engine.calculate_force(&data, "GT3", "911", 0.1f);
    }
    auto batch = engine.GetDebugBatch();
    ASSERT_FALSE(batch.empty());
    ASSERT_EQ((int)batch.size(), 100);

    // 2. Frequency crossing branch in apply_signal_conditioning
    FFBCalculationContext ctx;
    ctx.dt = 0.0025;
    ctx.car_speed = 10.0;

    // Alternating torque to trigger crossings
    data.mElapsedTime = 0.1;
    FFBEngineTestAccess::CallApplySignalConditioning(engine, 1.0, &data, ctx);
    data.mElapsedTime = 0.2;
    FFBEngineTestAccess::CallApplySignalConditioning(engine, -1.0, &data, ctx);
    data.mElapsedTime = 0.3;
    FFBEngineTestAccess::CallApplySignalConditioning(engine, 1.0, &data, ctx);

    // 3. Wheel radius branch
    data.mWheel[0].mStaticUndeflectedRadius = 5; // Very small
    FFBEngineTestAccess::CallApplySignalConditioning(engine, 1.0, &data, ctx);

    // 4. Speed gate branches
    engine.m_speed_gate_lower = 10.0f;
    engine.m_speed_gate_upper = 5.0f; // Invalid: upper < lower
    engine.calculate_force(&data, "GT3", "911", 0.1f);

    // 5. Logging branch in calculate_force
    SessionInfo info;
    info.vehicle_name = "LogCar";
    AsyncLogger::Get().Start(info, "tmp_log_extra");
    engine.calculate_force(&data, "GT3", "911", 0.1f);
    AsyncLogger::Get().Stop();
    std::filesystem::remove_all("tmp_log_extra");

    // 6. Warn DT branch
    data.mDeltaTime = 0.0;
    engine.calculate_force(&data, "GT3", "911", 0.1f);

    // 7. Rear grip approximated branch
    data.mWheel[2].mTireLoad = 0.0;
    data.mWheel[3].mTireLoad = 0.0;
    engine.calculate_force(&data, "GT3", "911", 0.1f);
}

TEST_CASE(test_async_logger_api_boost, "Diagnostics") {
    SessionInfo info;
    info.vehicle_name = "BoostCar";
    info.track_name = "BoostTrack";
    const char* log_dir = "tmp_boost_logs/"; // Test trailing slash

    // 1. Start already running
    AsyncLogger::Get().Start(info, log_dir);
    AsyncLogger::Get().Start(info, log_dir); // Should return early

    // 2. Log decimation and marker
    LogFrame frame;
    memset(&frame, 0, sizeof(frame));

    // Trigger marker
    AsyncLogger::Get().SetMarker();
    AsyncLogger::Get().Log(frame); // Should log because of marker despite decimation

    // Test marker in frame
    frame.marker = true;
    AsyncLogger::Get().Log(frame);

    // Test decimation
    frame.marker = false;
    for(int i = 0; i < 10; i++) {
        AsyncLogger::Get().Log(frame);
    }

    // 3. Trigger buffer threshold notification
    // DECIMATION_FACTOR = 4, BUFFER_THRESHOLD = 200
    // We need 201 * 4 frames to fill the buffer if decimation is active
    for(int i = 0; i < 850; i++) {
        AsyncLogger::Get().Log(frame);
    }

    // 4. Log when not running
    AsyncLogger::Get().Stop();
    AsyncLogger::Get().Stop(); // Double stop
    AsyncLogger::Get().Log(frame);

    std::filesystem::remove_all("tmp_boost_logs");
}

#ifdef ENABLE_IMGUI
TEST_CASE(test_gui_diverse_engine_states, "GUI") {
    IMGUI_CHECKVERSION();
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1920, 1080);
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    FFBEngine engine;
    InitializeEngine(engine);

    // Exercise UI with varied engine states
    ImGui::NewFrame();

    engine.m_torque_source = 1; // In-Game FFB
    GuiLayerTestAccess::DrawTuningWindow(engine);

    engine.m_torque_source = 0; // Shaft Torque
    GuiLayerTestAccess::DrawTuningWindow(engine);

    // Logging active
    SessionInfo info;
    info.vehicle_name = "GUICar";
    const char* log_dir = "tmp_gui_logs";
    AsyncLogger::Get().Start(info, log_dir);
    GuiLayerTestAccess::DrawTuningWindow(engine);
    AsyncLogger::Get().Stop();
    std::filesystem::remove_all(log_dir);

    // Slope detection ON
    engine.m_slope_detection_enabled = true;
    GuiLayerTestAccess::DrawTuningWindow(engine);

    ImGui::EndFrame();
    ImGui::DestroyContext(ctx);
}
#endif

} // namespace FFBEngineTests
