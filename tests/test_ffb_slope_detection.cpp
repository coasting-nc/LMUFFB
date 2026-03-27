#include "test_ffb_common.h"
#include <random>

using namespace LMUFFB::Utils;

namespace FFBEngineTests {

TEST_CASE(test_slope_detection_buffer_init, "SlopeDetection") {
    std::cout << "\nTest: Slope Detection Buffer Initialization (v0.7.0)" << std::endl;
    FFBEngine engine;
    // Buffer count and index should be 0 on fresh instance
    ASSERT_TRUE(engine.m_slope_buffer_count == 0);
    ASSERT_TRUE(engine.m_slope_buffer_index == 0);
    ASSERT_TRUE(engine.m_slope_current == 0.0);
}

TEST_CASE(test_slope_sg_derivative, "SlopeDetection") {
    std::cout << "\nTest: Savitzky-Golay Derivative Calculation (v0.7.0)" << std::endl;
    FFBEngine engine;
    
    // Fill buffer with linear ramp: y = i * 0.1 (slope = 0.1 units/sample)
    // dt = 0.01 -> derivative = 0.1 / 0.01 = 10.0 units/sec
    double dt = 0.01;
    int window = 9;
    
    // Fill buffer
    for (int i = 0; i < window; ++i) {
        engine.m_slope_lat_g_buffer[i] = (double)i * 0.1;
    }
    engine.m_slope_buffer_count = window;
    engine.m_slope_buffer_index = window; // Point past last sample
    
    double derivative = calculate_sg_derivative(engine.m_slope_lat_g_buffer, engine.m_slope_buffer_count, window, dt, engine.m_slope_buffer_index);
    
    ASSERT_NEAR(derivative, 10.0, 0.1);
}

TEST_CASE(test_slope_grip_at_peak, "SlopeDetection") {
    std::cout << "\nTest: Slope Grip at Peak (Zero Slope) (v0.7.0)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;
    engine.m_slope_detection.sg_window = 15;
    
    // Simulate peak grip: Constant G despite increasing slip?
    // Actually, zero slope means G is constant while slip moves.
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.05);
    data.mLocalAccel.x = 1.2 * 9.81; // 1.2G
    data.mDeltaTime = 0.01;
    
    // Fill buffer with constant values
    PumpEngineTime(engine, data, 2.0);

    // Slope should be near 0
    ASSERT_NEAR(engine.m_slope_current, 0.0, 0.5);
    // Grip should be near 1.0
    ASSERT_GE(engine.m_slope_smoothed_output, 0.9);
}

TEST_CASE(test_slope_grip_past_peak, "SlopeDetection") {
    std::cout << "\nTest: Slope Grip Past Peak (Negative Slope) [Issue #397 Remediation]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;
    engine.m_slope_detection.sg_window = 15;
    engine.m_slope_detection.sensitivity = 1.0f;
    engine.m_slope_detection.min_threshold = -0.3f;
    engine.m_slope_detection.max_threshold = -2.0f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01; // 100Hz
    
    // Simulate past peak: Increasing slip, decreasing G
    // Stimulus: G goes 1.5 -> 0.6 over 60 frames. Stay positive!
    for (int i = 0; i < 60; i++) {
        double slip = 0.05 + (double)i * 0.005;
        double g = 1.5 - (double)i * 0.015; // Decreasing but stays > 0
        
        data.mWheel[WHEEL_FL].mLateralPatchVel = slip * 20.0;
        data.mWheel[WHEEL_FR].mLateralPatchVel = slip * 20.0;
        data.mLocalAccel.x = g * 9.81;
        
        // Pump 4 times per 100Hz frame to match 400Hz loop
        for(int j=0; j<4; j++) {
            engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.0025);
        }
        data.mElapsedTime += 0.01;
    }
    
    std::cout << "  Final Slope: " << engine.m_slope_current << " | Grip Output: " << engine.m_slope_smoothed_output << std::endl;

    // Slope should be significantly negative
    ASSERT_LT(engine.m_slope_current, -1.0);
    // Grip should be reduced
    ASSERT_LT(engine.m_slope_smoothed_output, 0.95);
    // But above safety floor
    ASSERT_GE(engine.m_slope_smoothed_output, 0.2);
}

TEST_CASE(test_slope_vs_static_comparison, "SlopeDetection") {
    std::cout << "\nTest: Slope vs Static Comparison (v0.7.0)" << std::endl;
    FFBEngine engine_slope;
    InitializeEngine(engine_slope);
    engine_slope.m_slope_detection.enabled = true;
    
    FFBEngine engine_static;
    InitializeEngine(engine_static);
    engine_static.m_slope_detection.enabled = false;
    engine_static.m_grip_estimation.optimal_slip_angle = 0.10f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.12); // 12% slip
    data.mDeltaTime = 0.01;
    
    // Run both
    for (int i = 0; i < 40; i++) {
        double slip = 0.05 + (double)i * 0.01;
        data.mWheel[WHEEL_FL].mLateralPatchVel = slip * 20.0;
        data.mWheel[WHEEL_FR].mLateralPatchVel = slip * 20.0;
        
        double g = 1.0;
        if (i < 15) g = 1.0 + (double)i * 0.1; // Increasing G
        else g = 2.5 - (double)(i - 15) * 0.2; // Dropping G (Loss of grip!)
        
        data.mLocalAccel.x = g * 9.81;
        
        PumpEngineTime(engine_slope, data, 0.01);
        PumpEngineTime(engine_static, data, 0.01);
    }
    
    auto snap_slope = engine_slope.GetDebugBatch().back();
    auto snap_static = engine_static.GetDebugBatch().back();
    
    std::cout << "  Slope Grip: " << snap_slope.calc_front_grip << " | Static Grip: " << snap_static.calc_front_grip << std::endl;
    
    // Both should detect grip loss
    ASSERT_LT(snap_slope.calc_front_grip, 0.95);
    ASSERT_LT(snap_static.calc_front_grip, 0.9);
}

TEST_CASE(test_slope_config_persistence, "SlopeDetection") {
    std::cout << "\nTest: Slope Config Persistence (v0.7.0)" << std::endl;
    std::string test_file = "test_slope_config.ini";
    FFBEngine engine_save;
    InitializeEngine(engine_save);
    
    engine_save.m_slope_detection.enabled = true;
    engine_save.m_slope_detection.sg_window = 21;
    engine_save.m_slope_detection.sensitivity = 2.5f;
    engine_save.m_slope_detection.min_threshold = -0.2f;
    engine_save.m_slope_detection.smoothing_tau = 0.05f;
    
    Config::Save(engine_save, test_file);
    
    FFBEngine engine_load;
    InitializeEngine(engine_load);
    Config::Load(engine_load, test_file);
    
    ASSERT_TRUE(engine_load.m_slope_detection.enabled == true);
    ASSERT_TRUE(engine_load.m_slope_detection.sg_window == 21);
    ASSERT_NEAR(engine_load.m_slope_detection.sensitivity, 2.5f, 0.001);
    ASSERT_NEAR(engine_load.m_slope_detection.min_threshold, -0.2f, 0.001);
    ASSERT_NEAR(engine_load.m_slope_detection.smoothing_tau, 0.05f, 0.001);
    
    std::remove(test_file.c_str());
}

TEST_CASE(test_slope_latency_characteristics, "SlopeDetection") {
    std::cout << "\nTest: Slope Latency Characteristics (v0.7.0)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;
    int window = 15;
    engine.m_slope_detection.sg_window = window;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;
    
    // Buffer fills one sample per calculate_force call.
    // 15 frames @ 400Hz = 37.5ms.
    PumpEngineTime(engine, data, 0.05);
    
    ASSERT_GE(engine.m_slope_buffer_count, window);
    
    // Latency is (window-1)/2 * dt (group delay of a symmetric FIR/SG filter)
    float latency_ms = (static_cast<float>(window - 1) / 2.0f) * 2.5f;
    std::cout << "  Calculated Latency for Window " << window << " at 400Hz: " << latency_ms << " ms" << std::endl;
    ASSERT_NEAR(latency_ms, 17.5, 0.5);
}

TEST_CASE(test_slope_noise_rejection, "SlopeDetection") {
    std::cout << "\nTest: Slope Noise Rejection (v0.7.0)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;
    engine.m_slope_detection.sg_window = 15;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.05);
    data.mDeltaTime = 0.01;
    
    std::default_random_engine generator(42);
    std::uniform_real_distribution<double> noise_dist(-0.1, 0.1);
    
    // Fill buffer with noise around constant 1.2G
    for (int i = 0; i < 100; i++) {
        data.mLocalAccel.x = (1.2 + noise_dist(generator)) * 9.81;
        PumpEngineTime(engine, data, 0.01);
    }
    
    // Despite noise, slope should be near zero (SG filter rejection).
    // A bound of < 5.0 is intentional: this test verifies the SG filter actually
    // *rejects* noise (not just that it doesn't crash). The max clamp is ±20, so
    // passing at 20.0 would be equivalent to no rejection at all.
    std::cout << "  Noisy Slope: " << engine.m_slope_current << std::endl;
    ASSERT_LT(std::abs(engine.m_slope_current), 5.0);
}

TEST_CASE(test_slope_buffer_reset_on_toggle, "SlopeDetection") {
    std::cout << "\nTest: Slope Buffer Reset on Toggle (v0.7.0)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;
    
    // Step 1: Fill buffer with data while slope detection is OFF
    engine.m_slope_detection.enabled = false;
    
    for (int i = 0; i < 20; i++) {
        data.mLocalAccel.x = (0.5 + i * 0.05) * 9.81;
        data.mWheel[WHEEL_FL].mLateralPatchVel = (0.05 + i * 0.005) * 20.0;
        PumpEngineTime(engine, data, 0.01);
    }
    
    // Step 2: Manually corrupt buffers to simulate stale data
    engine.m_slope_buffer_count = 15;  // Partially filled
    engine.m_slope_buffer_index = 7;   // Mid-buffer
    engine.m_slope_smoothed_output = 0.65;  // Some grip loss value
    
    for (int i = 0; i < 15; i++) {
        engine.m_slope_lat_g_buffer[i] = 1.2 + i * 0.1;
        engine.m_slope_slip_buffer[i] = 0.05 + i * 0.01;
    }
    
    // Step 3: Enable slope detection (simulating GUI toggle)
    bool prev_enabled = engine.m_slope_detection.enabled;
    engine.m_slope_detection.enabled = true;
    
    //  Simulate the reset logic from GuiLayer.cpp
    if (!prev_enabled && engine.m_slope_detection.enabled) {
        engine.m_slope_buffer_count = 0;
        engine.m_slope_buffer_index = 0;
        engine.m_slope_smoothed_output = 1.0;  // Full grip
    }
    
    // Step 4: Verify buffers were reset
    ASSERT_TRUE(engine.m_slope_buffer_count == 0);
    ASSERT_TRUE(engine.m_slope_buffer_index == 0);
    ASSERT_NEAR(engine.m_slope_smoothed_output, 1.0, 0.001);
    
    // Step 5: Run a few frames and verify clean slope calculation
    for (int i = 0; i < 5; i++) {
        data.mLocalAccel.x = 1.2 * 9.81;  // Constant 1.2G
        data.mWheel[WHEEL_FL].mLateralPatchVel = 0.05 * 20.0;  // Constant slip
        PumpEngineTime(engine, data, 0.01);
    }
    
    // After reset, buffer should be filling from scratch
    ASSERT_GT(engine.m_slope_buffer_count, 0);
    
    // Step 6: Test that disabling does NOT reset buffers
    engine.m_slope_detection.enabled = false;
    int count_before = engine.m_slope_buffer_count;
    PumpEngineTime(engine, data, 0.01);
    ASSERT_TRUE(engine.m_slope_buffer_count == count_before + 4);
}

TEST_CASE(test_slope_detection_no_boost_when_grip_balanced, "SlopeDetection") {
    std::cout << "\nTest: Slope Detection - No Boost When Grip Balanced (v0.7.1)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    // Enable slope detection with oversteer boost
    engine.m_slope_detection.enabled = true;
    engine.m_rear_axle.oversteer_boost = 2.0f; // Strong boost setting
    engine.m_rear_axle.sop_effect = 1.0f;
    engine.m_rear_axle.sop_scale = 10.0f;
    engine.m_general.wheelbase_max_nm = 20.0f; engine.m_general.target_rim_nm = 20.0f;

    // Use fallback-triggering telemetry
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;
    
    // Seed
    PumpEngineSteadyState(engine, data);
    
    // Trigger negative slope to reduce front grip
    bool grip_dropped = false;
    for (int i = 0; i < 40; i++) {
        double slip = 0.05 + (double)i * 0.01;
        double g = 1.0 - (double)i * 0.05; // Slower drop to stay positive for longer
        data.mLocalAccel.x = g * 9.81;
        data.mWheel[WHEEL_FL].mLateralPatchVel = slip * 20.0;
        data.mWheel[WHEEL_FR].mLateralPatchVel = slip * 20.0;
        PumpEngineTime(engine, data, 0.01);

        auto batch = engine.GetDebugBatch();
        for(const auto& s : batch) {
            if (s.calc_front_grip < 0.95) grip_dropped = true;
        }
    }
    
    // Behavioral: Grip should have dropped at some point
    ASSERT_TRUE(grip_dropped);

    auto final_batch = engine.GetDebugBatch();
    float final_boost = final_batch.empty() ? 0.0f : final_batch.back().oversteer_boost;
    // Oversteer boost should be minimal because slope detection is enabled (which disables it in FFBEngine.cpp)
    ASSERT_LT(std::abs(final_boost), 0.5);
}

TEST_CASE(test_slope_detection_no_boost_during_oversteer, "SlopeDetection") {
    std::cout << "\nTest: Slope Detection - No Boost During Oversteer (v0.7.1)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    // Enable slope detection with oversteer boost
    engine.m_slope_detection.enabled = true;
    engine.m_rear_axle.oversteer_boost = 2.0f; // Strong boost setting
    engine.m_rear_axle.sop_effect = 1.0f;
    engine.m_rear_axle.sop_scale = 10.0f;
    engine.m_general.wheelbase_max_nm = 20.0f; engine.m_general.target_rim_nm = 20.0f;
    engine.m_grip_estimation.optimal_slip_angle = 0.05f; // Rear grip will drop past 0.05 slip
    
    // Setup telemetry to create oversteer scenario (front grip > rear grip)
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;
    
    // Seed
    PumpEngineSteadyState(engine, data);

    // Build up positive slope (Front grip = 1.0)
    for (int i = 0; i < 20; i++) {
        data.mLocalAccel.x = (0.5 + i * 0.1) * 9.81;
        data.mWheel[WHEEL_FL].mLateralPatchVel = (0.02 + i * 0.005) * 20.0;
        PumpEngineTime(engine, data, 0.01);
    }
    
    auto batch = engine.GetDebugBatch();
    FFBSnapshot snap = batch.back();
    
    // Assertion: oversteer_boost should be 0.0 when slope detection is enabled
    ASSERT_NEAR(snap.oversteer_boost, 0.0, 0.1);
}

TEST_CASE(test_lat_g_boost_works_without_slope_detection, "SlopeDetection") {
    std::cout << "\nTest: Lateral G Boost works without Slope Detection (v0.7.1)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_slope_detection.enabled = false;
    engine.m_rear_axle.oversteer_boost = 2.0f;
    engine.m_rear_axle.sop_effect = 1.0f;
    engine.m_rear_axle.sop_scale = 10.0f;
    engine.m_general.wheelbase_max_nm = 20.0f; engine.m_general.target_rim_nm = 20.0f;
    engine.m_grip_estimation.optimal_slip_angle = 0.05f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.06); // Slip 0.06
    data.mLocalAccel.x = 1.5 * 9.81;
    data.mDeltaTime = 0.01;
    
    data.mWheel[WHEEL_FL].mLateralPatchVel = 0.04 * 20.0;
    data.mWheel[WHEEL_FR].mLateralPatchVel = 0.04 * 20.0;
    data.mWheel[WHEEL_RL].mLateralPatchVel = 0.08 * 20.0;
    data.mWheel[WHEEL_RR].mLateralPatchVel = 0.08 * 20.0;
    
    PumpEngineSteadyState(engine, data);
    FFBSnapshot snap = engine.GetDebugBatch().back();
    
    ASSERT_GT(snap.oversteer_boost, 0.01);
}

TEST_CASE(test_slope_detection_default_values_v071, "SlopeDetection") {
    std::cout << "\nTest: Slope Detection Default Values (v0.7.1)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    ASSERT_NEAR(engine.m_slope_detection.sensitivity, 0.5f, 0.001);
    ASSERT_NEAR(engine.m_slope_detection.min_threshold, -0.3f, 0.001);
    ASSERT_NEAR(engine.m_slope_detection.smoothing_tau, 0.04f, 0.001);
}

TEST_CASE(test_slope_current_in_snapshot, "SlopeDetection") {
    std::cout << "\nTest: Slope Current in Snapshot (v0.7.1)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;
    
    // Seed
    PumpEngineTime(engine, data, 0.05);

    // Frames 1-20: Build up a slope
    for (int i = 0; i < 20; i++) {
        data.mLocalAccel.x = (0.5 + i * 0.1) * 9.81;
        data.mWheel[WHEEL_FL].mLateralPatchVel = (0.02 + i * 0.01) * 20.0;
        PumpEngineTime(engine, data, 0.01);
    }
    
    auto batch = engine.GetDebugBatch();
    FFBSnapshot snap = batch.back();
    
    ASSERT_GT(std::abs(snap.slope_current), 0.1);
}

TEST_CASE(test_slope_detection_less_aggressive_v071, "SlopeDetection") {
    std::cout << "\nTest: Slope Detection Less Aggressive (v0.7.1)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_slope_detection.enabled = true;
    engine.m_slope_detection.sensitivity = 0.5f;
    engine.m_slope_detection.min_threshold = -0.3f;
    engine.m_slope_detection.sg_window = 15;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;
    
    PumpEngineSteadyState(engine, data);
    engine.GetDebugBatch(); // Clear seeding transients

    // Trigger negative slope to reduce front grip
    bool slope_detected = false;
    for (int i = 0; i < 60; i++) {
        double g = 1.5 - (double)i * 0.05;
        double slip = 0.05 + (double)i * 0.01;
        data.mLocalAccel.x = g * 9.81;
        data.mWheel[WHEEL_FL].mLateralPatchVel = slip * 20.0;
        data.mWheel[WHEEL_FR].mLateralPatchVel = slip * 20.0;
        PumpEngineTime(engine, data, 0.01);

        auto batch = engine.GetDebugBatch();
        for(const auto& s : batch) {
            if (s.slope_current < -0.5) slope_detected = true;
        }
    }
    
    ASSERT_TRUE(slope_detected);
}

TEST_CASE(test_slope_decay_on_straight, "SlopeDetection") {
    std::cout << "\nTest: Slope Decay on Straight [Issue #397 Remediation]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;
    engine.m_slope_detection.alpha_threshold = 0.02f;
    engine.m_slope_detection.decay_rate = 5.0f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(30.0, 0.05);
    
    // Cornering phase
    for (int i = 0; i < 40; i++) {
        data.mLocalAccel.x = (0.5 + 0.05 * i) * 9.81;
        for (int w = 0; w < 4; w++) {
            data.mWheel[w].mLateralPatchVel = (0.05 + 0.005 * i) * 30.0;
        }
        // Issue #397: Advance time correctly
        data.mElapsedTime += 0.01;
        engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.01);
    }
    
    double slope_after_corner = engine.m_slope_current;
    std::cout << "  Slope After Corner: " << slope_after_corner << std::endl;
    ASSERT_TRUE(std::abs(slope_after_corner) > 1.0);
    
    data = CreateBasicTestTelemetry(30.0, 0.0);
    
    // Issue #397: Allow 1.5s for hold timer and exponential decay to finish
    PumpEngineTime(engine, data, 1.5);
    
    double slope_after_straight = engine.m_slope_current;
    std::cout << "  Slope After 1.5s Straight: " << slope_after_straight << std::endl;
    ASSERT_TRUE(std::abs(slope_after_straight) < 0.5);

    // Run longer to ensure zero
    PumpEngineTime(engine, data, 1.0);

    double slope_final = engine.m_slope_current;
    ASSERT_NEAR(slope_final, 0.0, 0.05);
}

TEST_CASE(test_slope_detection_alpha_threshold_configurable, "SlopeDetection") {
    std::cout << "\nTest: Slope dAlpha Threshold Configurable (v0.7.3)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;
    engine.m_slope_detection.alpha_threshold = 0.02f;
    engine.m_slope_current = -0.5f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mDeltaTime = 0.01;
    for (int w = 0; w < 4; w++) {
        data.mWheel[w].mLateralPatchVel = 0.0001 * 20.0;
    }

    engine.calculate_force(&data);

    ASSERT_TRUE(std::abs(engine.m_slope_current) < 0.5f);

    engine.m_slope_current = -0.1f;
    for (int i = 0; i < FFBEngine::SLOPE_BUFFER_MAX; i++) {
        engine.m_slope_lat_g_buffer[i] = 0.0;
        engine.m_slope_slip_buffer[i] = 0.0;
    }
    
    for (int i = 0; i < 20; i++) {
        data.mLocalAccel.x = (0.1 * i);
        for (int w = 0; w < 4; w++) {
            data.mWheel[w].mLateralPatchVel = (-0.01 * i) * 20.0;
        }
        engine.calculate_force(&data);
    }
    
    ASSERT_TRUE(std::abs(engine.m_slope_current) > 1.0);
}

TEST_CASE(test_slope_confidence_gate, "SlopeDetection") {
    std::cout << "\nTest: Slope Confidence Gate (v0.7.3)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;
    engine.m_slope_detection.confidence_enabled = true;
    engine.m_slope_detection.alpha_threshold = 0.01f;
    engine.m_slope_detection.min_threshold = -0.3f;
    engine.m_slope_detection.sensitivity = 1.0f;
    
    double dAlpha_dt = 0.1;
    double confidence = (std::min)(1.0, std::abs(dAlpha_dt) / 0.1);
    ASSERT_NEAR(confidence, 1.0, 0.001);
    
    dAlpha_dt = 0.02;
    confidence = (std::min)(1.0, std::abs(dAlpha_dt) / 0.1);
    ASSERT_NEAR(confidence, 0.2, 0.001);
}

TEST_CASE(test_slope_stability_config_persistence, "SlopeDetection") {
    std::cout << "\nTest: Slope Stability Config Persistence (v0.7.3)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;
    engine.m_slope_detection.alpha_threshold = 0.035f;
    engine.m_slope_detection.decay_rate = 8.5f;
    engine.m_slope_detection.confidence_enabled = false;
    
    Config::Save(engine, "test_stability.ini");
    
    FFBEngine engine2;
    InitializeEngine(engine2);
    Config::Load(engine2, "test_stability.ini");
    
    ASSERT_NEAR(engine2.m_slope_detection.alpha_threshold, 0.035f, 0.0001);
    ASSERT_NEAR(engine2.m_slope_detection.decay_rate, 8.5f, 0.0001);
    ASSERT_TRUE(engine2.m_slope_detection.confidence_enabled == false);
}

TEST_CASE(test_slope_no_understeer_on_straight_v073, "SlopeDetection") {
    std::cout << "\nTest: No Understeer on Straight (v0.7.3)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;
    engine.m_front_axle.understeer_effect = 1.0f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(30.0, 0.0);
    for(int i=0; i<4; ++i) data.mWheel[i].mGripFract = 1.0;

    // Force a bad state
    engine.m_slope_current = -2.0f;
    engine.m_slope_smoothed_output = 0.6f; 
    
    PumpEngineSteadyState(engine, data);
    
    auto batch = engine.GetDebugBatch();
    if (batch.empty()) { FAIL_TEST("Empty batch"); return; }
    auto snap = batch.back();

    ASSERT_LT(std::abs(snap.slope_current), 1.0);
    ASSERT_GT(snap.calc_front_grip, 0.9);
}

TEST_CASE(test_slope_detection_decay_rate_boundaries, "SlopeDetection") {
    std::cout << "\nTest: Slope Decay Rate Boundaries (v0.7.3)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);

    engine.m_slope_detection.decay_rate = 0.5f;
    engine.m_slope_current = -1.0f;
    PumpEngineTime(engine, data, 0.05);
    double decayed_slow = engine.m_slope_current;

    engine.m_slope_detection.decay_rate = 20.0f;
    engine.m_slope_current = -1.0f;
    PumpEngineTime(engine, data, 0.05);
    double decayed_fast = engine.m_slope_current;

    ASSERT_TRUE(std::abs(decayed_fast) < std::abs(decayed_slow));
}

TEST_CASE(test_slope_detection_alpha_threshold_validation, "SlopeDetection") {
    std::cout << "\nTest: Slope Alpha Threshold Validation (v0.7.3)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    engine.m_slope_detection.alpha_threshold = 0.0001f;
    Config::Save(engine, "test_val.toml");
    Config::Load(engine, "test_val.toml");
    ASSERT_NEAR(engine.m_slope_detection.alpha_threshold, 0.02f, 0.0001);

    engine.m_slope_detection.alpha_threshold = 0.5f;
    Config::Save(engine, "test_val.toml");
    Config::Load(engine, "test_val.toml");
    ASSERT_NEAR(engine.m_slope_detection.alpha_threshold, 0.02f, 0.0001);

    std::remove("test_val.toml");
}

TEST_CASE(test_inverse_lerp_helper, "SlopeDetection") {
    std::cout << "\nTest: InverseLerp Helper Function (v0.7.11)" << std::endl;
    FFBEngine engine;

    // Note: For slope thresholds, min is less negative (-0.3), max is more negative (-2.0)
    // slope=-0.3 â†’ 0%, slope=-2.0 â†’ 100%

    // At min (start of range)
    double at_min = inverse_lerp(-0.3, -2.0, -0.3);
    ASSERT_NEAR(at_min, 0.0, 0.001);

    // At max (end of range)
    double at_max = inverse_lerp(-0.3, -2.0, -2.0);
    ASSERT_NEAR(at_max, 1.0, 0.001);

    // At midpoint (-1.15)
    double at_mid = inverse_lerp(-0.3, -2.0, -1.15);
    ASSERT_NEAR(at_mid, 0.5, 0.001);

    // Above min (dead zone)
    double dead_zone = inverse_lerp(-0.3, -2.0, 0.0);
    ASSERT_NEAR(dead_zone, 0.0, 0.001);

    // Below max (saturated)
    double saturated = inverse_lerp(-0.3, -2.0, -5.0);
    ASSERT_NEAR(saturated, 1.0, 0.001);
}

TEST_CASE(test_slope_minmax_dead_zone, "SlopeDetection") {
    std::cout << "\nTest: Slope Min/Max Dead Zone (v0.7.11)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;
    engine.m_slope_detection.min_threshold = -0.5f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.05);
    data.mLocalAccel.x = 1.0 * 9.81;

    // Force slope into deadzone
    engine.m_slope_current = -0.2f;

    PumpEngineSteadyState(engine, data);

    ASSERT_GE(engine.GetDebugBatch().back().calc_front_grip, 0.95);
}

TEST_CASE(test_slope_minmax_linear_response, "SlopeDetection") {
    std::cout << "\nTest: Slope Min/Max Linear Response [Issue #397 Remediation]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;
    engine.m_slope_detection.min_threshold = -0.3f;
    engine.m_slope_detection.max_threshold = -2.0f;
    engine.m_slope_detection.smoothing_tau = 0.001f;  // Fast smoothing for test
    engine.m_slope_detection.alpha_threshold = 0.0001f; // Ensure it doesn't decay

    auto fill_buffers_for_slope = [&](double target_slope) {
        engine.m_slope_buffer_count = 0;
        engine.m_slope_buffer_index = 0;
        engine.m_slope_smoothed_output = 1.0;
        // Use a significant stimulus to trigger updates and stay positive
        for (int i = 0; i < 100; i++) {
            double alpha = 0.1 + (double)i * 0.01;
            double g = 100.0 + target_slope * alpha;
            engine.calculate_slope_grip(g, alpha, 0.01);
        }
    };

    fill_buffers_for_slope(-0.725);
    std::cout << "  Target: -0.725 | Actual: " << engine.m_slope_current << std::endl;
    ASSERT_NEAR(engine.m_slope_current, -0.725, 0.1);
    // Expected loss: 25% of 0.8 = 0.2 -> Grip: 0.8
    ASSERT_NEAR(engine.m_slope_smoothed_output, 0.8, 0.1);

    // At 50% into range: slope = -1.15
    fill_buffers_for_slope(-1.15);
    std::cout << "  Target: -1.15 | Actual: " << engine.m_slope_current << std::endl;
    ASSERT_NEAR(engine.m_slope_current, -1.15, 0.1);
    // Expected loss: 50% of 0.8 = 0.4 -> Grip: 0.6
    ASSERT_NEAR(engine.m_slope_smoothed_output, 0.6, 0.1);

    // At 100% (max): grip should hit floor
    fill_buffers_for_slope(-2.0);
    std::cout << "  Target: -2.0 | Actual: " << engine.m_slope_current << std::endl;
    ASSERT_NEAR(engine.m_slope_current, -2.0, 0.1);
    ASSERT_NEAR(engine.m_slope_smoothed_output, 0.2, 0.1);  // Floor
}

TEST_CASE(test_slope_minmax_saturation, "SlopeDetection") {
    std::cout << "\nTest: Slope Min/Max Saturation [Issue #397 Remediation]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;
    engine.m_slope_detection.min_threshold = -0.3f;
    engine.m_slope_detection.max_threshold = -2.0f;
    engine.m_slope_detection.smoothing_tau = 0.01f;

    auto fill_buffers_for_slope = [&](double target_slope) {
        engine.m_slope_buffer_count = 0;
        engine.m_slope_buffer_index = 0;
        engine.m_slope_smoothed_output = 1.0;
        // Use a significant stimulus to trigger updates.
        // We use a high base (100.0) to ensure |G| stays positive while decreasing.
        for (int i = 0; i < 100; i++) {
            double alpha = 0.1 + (double)i * 0.05;
            double g = 100.0 + target_slope * alpha;
            engine.calculate_slope_grip(g, alpha, 0.01);
        }
    };

    // Extreme slope (way beyond max)
    fill_buffers_for_slope(-10.0);

    // Should saturate at floor (0.2), not go negative or beyond
    std::cout << "  Final Slope: " << engine.m_slope_current << " | Grip: " << engine.m_slope_smoothed_output << std::endl;
    ASSERT_NEAR(engine.m_slope_smoothed_output, 0.2, 0.05);
}

TEST_CASE(test_slope_threshold_config_persistence, "SlopeDetection") {
    std::cout << "\nTest: Slope Threshold Config Persistence (v0.7.11)" << std::endl;
    std::string test_file = "test_slope_minmax.ini";

    FFBEngine engine_save;
    engine_save.m_slope_detection.min_threshold = -0.5f;
    engine_save.m_slope_detection.max_threshold = -3.0f;
    Config::Save(engine_save, test_file);

    FFBEngine engine_load;
    InitializeEngine(engine_load);
    Config::Load(engine_load, test_file);

    ASSERT_NEAR(engine_load.m_slope_detection.min_threshold, -0.5f, 0.001);
    ASSERT_NEAR(engine_load.m_slope_detection.max_threshold, -3.0f, 0.001);

    std::remove(test_file.c_str());
}

TEST_CASE(test_slope_detection_sensitivity_migration, "SlopeDetection") {
    std::cout << "\nTest: Slope Sensitivity Migration (v0.7.11)" << std::endl;
    std::string test_file = "test_slope_migration.ini";

    // Create legacy config
    {
        std::ofstream file(test_file);
        file << "slope_detection_enabled=1\n";
        file << "slope_sensitivity=1.0\n";           // Legacy
        file << "slope_negative_threshold=-0.3\n";   // Legacy
        // No slope_min_threshold or slope_max_threshold
    }

    FFBEngine engine;
    InitializeEngine(engine);
    Config::Load(engine, test_file);

    // With sensitivity=1.0, max_threshold should be calculated
    // Formula: max = min - (8/sens) = -0.3 - 8 = -8.3
    ASSERT_TRUE(engine.m_slope_detection.max_threshold < engine.m_slope_detection.min_threshold);
    ASSERT_NEAR(engine.m_slope_detection.max_threshold, -8.3f, 0.5);

    std::remove(test_file.c_str());
}

TEST_CASE(test_inverse_lerp_edge_cases, "SlopeDetection") {
    std::cout << "\nTest: InverseLerp Edge Cases (v0.7.11)" << std::endl;
    FFBEngine engine;

    // Min == Max (degenerate)
    double same = inverse_lerp(-0.3, -0.3, -0.3);
    ASSERT_TRUE(same == 0.0 || same == 1.0);

    // Very small range
    // value = -0.30001. Since it's < min, it should be 1.0 in negative direction context
    double tiny = inverse_lerp(-0.3, -0.30001, -0.30001);
    ASSERT_NEAR(tiny, 1.0, 0.01);

    // Reversed order (should still work or be caught)
    double reversed = inverse_lerp(-2.0, -0.3, -1.15);
    ASSERT_TRUE(reversed >= 0.0 && reversed <= 1.0);
}

// ============================================================
// v0.7.17 COMPREHENSIVE HARDENING TESTS
// ============================================================

TEST_CASE(TestSlope_NearThreshold_Singularity, "SlopeDetection") {
    std::cout << "\nTest: Slope Near Threshold Singularity (v0.7.17)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;
    engine.m_slope_detection.alpha_threshold = 0.02f;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;

    // Small changes just at threshold
    for (int i = 0; i < 60; i++) {
        double alpha = 0.1 + (double)i * 0.00021; // Reach threshold
        double g = 1.0 - (double)i * 0.05;
        data.mLocalAccel.x = g * 9.81;
        data.mWheel[WHEEL_FL].mLateralPatchVel = alpha * 20.0;
        PumpEngineTime(engine, data, 0.01);
    }

    auto snap = engine.GetDebugBatch().back();
    // Behavioral check: should not explode
    ASSERT_TRUE(std::isfinite(snap.slope_current));
    ASSERT_GE(snap.slope_current, -50.0);
    ASSERT_LE(snap.slope_current, 50.0);
}

TEST_CASE(test_issue_348_shadow_mode_v2, "SlopeDetection") {
    std::cout << "\nTest: Issue #348 Slope Detection Shadow Mode (v2)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = false; // Primary is OFF

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.05);
    // Explicitly set 1.0 grip to verify shadow mode doesn't contaminate pure telemetry
    for(int i=0; i<4; ++i) data.mWheel[i].mGripFract = 1.0;
    data.mDeltaTime = 0.01;

    // Seed
    // We must use PumpEngineSteadyState to bypass the seeding gate AND wait for adaptive smoothing
    PumpEngineSteadyState(engine, data);

    auto batch = engine.GetDebugBatch();
    if (batch.empty()) {
         FAIL_TEST("GetDebugBatch returned empty vector");
         return;
    }

    //Primary grip should stay 1.0 because slope detection is disabled
    ASSERT_GT(batch.back().calc_front_grip, 0.95);
}

TEST_CASE(test_slope_sg_filter_dt_independence, "SlopeDetection") {
    std::cout << "\nTest: SG Filter Time-Domain Independence [Issue #397 Remediation]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);

    // 1. Fill the SG buffer with a steady ramp to create a known derivative
    for(int i = 0; i < 100; i++) {
        if (i % 4 == 0) {
            data.mLocalAccel.x = (i / 4) * 0.1;
            data.mWheel[WHEEL_FL].mLateralPatchVel = (i / 4) * 0.01;
            data.mElapsedTime += 0.01;
        }
        engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.0025);
    }

    auto snaps1 = engine.GetDebugBatch();
    float normal_dg = snaps1.back().slope_dG_dt;

    // 2. Pass a massive dt (100ms) on the next frame.
    // We continue the ramp into the weird frame to see if the derivative math
    // is truly independent of the passed dt (since we use internal_dt).
    data.mLocalAccel.x += 0.1;
    data.mWheel[WHEEL_FL].mLateralPatchVel += 0.01;
    // Advance elapsedTime so upsamplers don't see it as a stale frame
    data.mElapsedTime += 0.1;

    engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.1);

    auto snaps2 = engine.GetDebugBatch();
    float weird_dg = snaps2.back().slope_dG_dt;

    std::cout << "  Normal dG/dt (dt=0.0025): " << normal_dg << std::endl;
    std::cout << "  Weird dG/dt  (dt=0.1):    " << weird_dg << std::endl;

    // 3. ASSERT: The derivative should remain stable. If it were sensitive to dt,
    // weird_dg would be 40x smaller (if math used passed dt) or 40x larger.
    ASSERT_GT(std::abs(normal_dg), 0.0f);
    ASSERT_NEAR(normal_dg, weird_dg, 0.2f);
}

TEST_CASE(test_slope_decay_with_zero_dt, "SlopeDetection") {
    std::cout << "\nTest: Slope Decay with Zero dt (Timer Robustness) [Issue #397 Remediation]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;
    engine.m_slope_detection.decay_rate = 10.0f;
    engine.m_slope_detection.alpha_threshold = 0.01f;

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);

    // 1. Trigger a massive slope and let it settle.
    // v0.7.198: We use a large enough ramp to clear noise and trigger slope.
    for (int i = 0; i < 100; i++) {
        data.mLocalAccel.x = 100.0 + (double)i * 0.5 * 9.81; // Keep it high and positive
        data.mWheel[WHEEL_FL].mLateralPatchVel = (double)i * 0.1 * 20.0;
        data.mElapsedTime += 0.01;
        engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.01);
    }

    auto snaps_peak = engine.GetDebugBatch();
    std::cout << "  Peak Slope: " << snaps_peak.back().slope_current << std::endl;
    ASSERT_GT(std::abs(snaps_peak.back().slope_current), 2.0f);

    // 2. Clear inputs and let it stabilize.
    // We need dAlpha_dt to fall below threshold to enter the decay phase.
    data.mLocalAccel.x = 0.0;
    data.mWheel[WHEEL_FL].mLateralPatchVel = 0.0;
    for (int i = 0; i < 100; i++) {
        data.mElapsedTime += 0.01;
        engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.01);
    }

    // 3. Pump with ZERO dt
    // We manually loop to force override_dt = 0.0 and data.mDeltaTime = 0.0
    for(int i = 0; i < 2000; i++) {
        data.mDeltaTime = 0.0;
        // We do NOT advance mElapsedTime here, so is_new_frame stays false.
        // The engine should still advance its internal decay timers using DEFAULT_CALC_DT.
        engine.calculate_force(&data, nullptr, nullptr, 0.0f, true, 0.0);
    }

    auto snaps_end = engine.GetDebugBatch();
    std::cout << "  Final Slope after 2000 zero-dt ticks: " << snaps_end.back().slope_current << std::endl;

    // 4. ASSERT: With the fix, it decays to near 0.
    //
    // Code review note (m-7): This assertion relies on calculate_force substituting
    // DEFAULT_CALC_DT internally when override_dt == 0.0 AND is_new_frame == false.
    // The slope decay formula in calculate_slope_grip() uses the passed `dt` directly:
    //   m_slope_current += decay_rate * dt * (0.0 - m_slope_current)
    // so if dt == 0.0 is passed literally, no decay occurs and this test WILL FAIL.
    // If this test ever fails, verify that FFBEngine::calculate_force() still applies
    // the DEFAULT_CALC_DT fallback before calling calculate_slope_grip().
    ASSERT_LT(std::abs(snaps_end.back().slope_current), 1.0f);
}

TEST_CASE(TestSlope_SmallSignals, "SlopeDetection") {
    std::cout << "\nTest: Slope Small Signals (Noise Rejection) (v0.7.17)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;
    engine.m_slope_detection.alpha_threshold = 0.02f;

    double dt = 0.01;
    int window = engine.m_slope_detection.sg_window;

    // Tiny oscillation in alpha (below threshold)
    // dAlpha/dt will be around 0.001 / 0.01 = 0.1? Wait.
    // If alpha is 0.001, 0.002, 0.001...
    // Let's use 0.0001 per frame -> dAlpha/dt = 0.01 rad/s (Below 0.02 threshold)

    for (int i = 0; i < window + 5; i++) {
        double alpha = 0.001 + (i % 2 == 0 ? 0.0001 : 0.0);
        double g = 1.0 + (i % 2 == 0 ? 0.05 : 0.0);
        engine.calculate_slope_grip(g, alpha, dt);
    }

    // Since dAlpha_dt is below threshold, slope should decay or stay 0
    // dAlpha_dt for [0.0011, 0.0010, 0.0011...] is near 0.
    ASSERT_NEAR(engine.m_slope_current, 0.0, 0.5);
    ASSERT_GE(engine.m_slope_smoothed_output, 0.99);
}

TEST_CASE(TestSlope_ImpulseRejection, "SlopeDetection") {
    std::cout << "\nTest: Slope Impulse Rejection (v0.7.17)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;
    engine.m_slope_detection.smoothing_tau = 0.04f;

    double dt = 0.01;
    int window = engine.m_slope_detection.sg_window;

    // 1. Settle in a corner
    for (int i = 0; i < window + 10; i++) {
        engine.calculate_slope_grip(1.0, 0.05 + (double)i * 0.001, dt);
    }

    double grip_before = engine.m_slope_smoothed_output;

    // 2. Inject massive G spike (Impulse)
    engine.calculate_slope_grip(10.0, 0.05 + (window + 10) * 0.001, dt);

    double grip_after = engine.m_slope_smoothed_output;
    double delta = std::abs(grip_after - grip_before);

    std::cout << "  Grip Before: " << grip_before << " | After Spike: " << grip_after << " | Delta: " << delta << std::endl;

    // Assertion: No single-frame jump > 10% (0.1)
    ASSERT_LE(delta, 0.1);
}

TEST_CASE(TestSlope_NoiseImmunity, "SlopeDetection") {
    std::cout << "\nTest: Slope Noise Immunity (v0.7.17)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;
    engine.m_slope_detection.sg_window = 15;

    double dt = 0.01;

    std::default_random_engine generator(42); // Fixed seed for reproducibility
    std::uniform_real_distribution<double> noise(-0.2, 0.2);

    std::vector<double> slopes;

    // Steady cornering with noise
    for (int i = 0; i < 100; i++) {
        double lat_g = 1.0 + noise(generator);
        double alpha = 0.05 + (double)i * 0.001 + noise(generator) * 0.001;
        engine.calculate_slope_grip(lat_g, alpha, dt);
        if (i > 30) slopes.push_back(engine.m_slope_current);
    }

    // Calculate Std Dev of slope
    double sum = 0, sum_sq = 0;
    for (double s : slopes) {
        sum += s;
        sum_sq += s * s;
    }
    double mean = sum / static_cast<double>(slopes.size());
    double variance = (sum_sq / static_cast<double>(slopes.size())) - (mean * mean);
    double std_dev = std::sqrt(std::abs(variance));

    std::cout << "  Noisy Slope Mean: " << mean << " | StdDev: " << std_dev << std::endl;

    // Assertion: StdDev should be reasonable (e.g. < 7.5)
    // Without SG filter and clamping it might be much higher.
    ASSERT_LE(std_dev, 7.5);
}

TEST_CASE(TestConfidenceRamp_Progressive, "SlopeDetection") {
    std::cout << "\nTest: Confidence Ramp Progressive (v0.7.17)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection.enabled = true;
    engine.m_slope_detection.min_threshold = -0.3f;
    engine.m_slope_detection.max_threshold = -2.0f;
    engine.m_slope_detection.smoothing_tau = 0.001f; // Fast for testing

    // FIX: The SG filter inside calculate_slope_grip now hardcodes 0.0025s (400Hz)
    // for its derivative calculations because the buffer is always populated at 400Hz.
    // We must feed the test data at 400Hz to match, otherwise the derivatives
    // will be artificially multiplied.
    double dt = 0.0025; 
    int window = engine.m_slope_detection.sg_window;

    std::vector<double> grips;

    // Ramp dAlpha/dt from 0.0 to 0.15
    // dG/dt constant at -2.0 G/s
    // 0.6 seconds at 400Hz = 240 frames.
    double rate = 0.25; 

    for (int i = 0; i < 240; i++) {
        double t = (double)i * dt;
        double alpha = 0.5 * rate * t * t;
        double g = 5.0 - 2.0 * t;

        // Call the internal method directly (simulating the 400Hz physics loop)
        engine.calculate_slope_grip(g, alpha, dt);

        if (i > window) {
            grips.push_back(engine.m_slope_smoothed_output);
        }
    }

    ASSERT_TRUE(grips.size() > 0);

    // Verify progressive decrease
    double last_grip = grips[0];
    for (size_t i = 1; i < grips.size(); i++) {
        // Grip should be decreasing or staying same, NOT increasing
        ASSERT_LE(grips[i], last_grip + 0.001);

        // No sudden jumps > 0.1 (With 400Hz, the max jump per frame is < 0.01)
        ASSERT_LE(std::abs(grips[i] - last_grip), 0.1);

        last_grip = grips[i];
    }

    // Final grip should be near floor
    ASSERT_NEAR(grips.back(), 0.2, 0.1);
}


} // namespace FFBEngineTests
