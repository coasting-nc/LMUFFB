#include "test_ffb_common.h"
#include <thread>

namespace FFBEngineTests {

TEST_CASE(test_ffb_engine_snapshot_buffer_overflow, "Diagnostics") {
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    
    // DEBUG_BUFFER_CAP is 100. 
    // We need to fill it to hit the branch: if (m_debug_buffer.size() < DEBUG_BUFFER_CAP)
    for (int i = 0; i < 105; i++) {
        engine.calculate_force(&data);
    }
    
    auto batch = engine.GetDebugBatch();
    ASSERT_EQ(batch.size(), 100); // Should be capped at 100
    
    // Buffer should be empty now
    auto empty_batch = engine.GetDebugBatch();
    ASSERT_EQ(empty_batch.size(), 0);
}

TEST_CASE(test_ffb_engine_signal_conditioning_combinatorial, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    FFBCalculationContext ctx;
    ctx.dt = 0.01;
    ctx.car_speed = 20.0;

    // 1. Static Notch Enabled + Width < MIN (1.0)
    engine.m_front_axle.static_notch_enabled = true;
    engine.m_front_axle.static_notch_width = 0.5f; // < 1.0
    FFBEngineTestAccess::CallApplySignalConditioning(engine, 1.0, &data, ctx);
    
    // 2. Interplay: Flatspot + Static Notch
    FFBEngineTestAccess::SetFlatspotSuppression(engine, true);
    FFBEngineTestAccess::CallApplySignalConditioning(engine, 1.0, &data, ctx);
    
    // 3. Static Notch Disabled (reset path)
    engine.m_front_axle.static_notch_enabled = false;
    FFBEngineTestAccess::CallApplySignalConditioning(engine, 1.0, &data, ctx);
}

TEST_CASE(test_ffb_engine_shaft_smoothing_branches, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    
    // Trigger shaft smoothing branches in calculate_force
    engine.m_front_axle.steering_shaft_smoothing = 0.1f;
    engine.calculate_force(&data); // Frame 1: Seed
    engine.calculate_force(&data); // Frame 2: Apply
}

TEST_CASE(test_ffb_engine_understeer_edge_cases, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    
    // 1. Gamma = 1.0 (Linear loss)
    engine.m_front_axle.understeer_gamma = 1.0f;
    engine.m_front_axle.understeer_effect = 1.0f;
    
    // Force low grip
    data.mWheel[0].mGripFract = 0.0;
    data.mWheel[1].mGripFract = 0.0;
    engine.calculate_force(&data);
    
    // 2. Grip = 0.0 exactly (Floor logic)
    // calculate_axle_grip might return slightly above 0 due to epsilon, 
    // but we want to hit std::max(0.0, 1.0 - grip_loss)
    engine.m_front_axle.understeer_effect = 2.0f; // Loss = 1.0 * 2.0 = 2.0. Factor = 1.0 - 2.0 = -1.0 -> 0.0
    engine.calculate_force(&data);
}

TEST_CASE(test_async_logger_sanitization_extended, "Diagnostics") {
    // Test logger filename sanitization with illegal characters
    // This exercises the regex/replacement branches in AsyncLogger
    std::string bad_name = "test_logs/illegal:?*<>|\"chars.bin";
    SessionInfo session = {};
    session.vehicle_name = "TestCar";
    session.track_name = "TestTrack";
    AsyncLogger::Get().Start(session, bad_name);
    
    if (AsyncLogger::Get().IsLogging()) {
        LogFrame frame = {};
        AsyncLogger::Get().Log(frame);
        AsyncLogger::Get().Stop();
    }
}

TEST_CASE(test_ffb_engine_steady_clock_latching, "Diagnostics") {
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    
    // First call sets last_log_time
    engine.calculate_force(&data);
    
    // We need to wait 1 second to trigger the latching branch
    // Or we can manually set the last_log_time back in time
    auto way_back = std::chrono::steady_clock::now() - std::chrono::seconds(2);
    FFBEngineTestAccess::SetLastLogTime(engine, way_back);
    
    // Second call should trigger: if (duration >= 1) { ResetInterval(); }
    engine.calculate_force(&data);
}

} // namespace FFBEngineTests
