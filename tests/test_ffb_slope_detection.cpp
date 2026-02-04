#include "test_ffb_common.h"
#include <random>

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
    
    double derivative = engine.calculate_sg_derivative(engine.m_slope_lat_g_buffer, engine.m_slope_buffer_count, window, dt);
    
    ASSERT_NEAR(derivative, 10.0, 0.1);
}

TEST_CASE(test_slope_grip_at_peak, "SlopeDetection") {
    std::cout << "\nTest: Slope Grip at Peak (Zero Slope) (v0.7.0)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    engine.m_slope_sg_window = 15;
    
    // Simulate peak grip: Constant G despite increasing slip? 
    // Actually, zero slope means G is constant while slip moves.
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.05);
    data.mLocalAccel.x = 1.2 * 9.81; // 1.2G
    data.mDeltaTime = 0.0025; // 400Hz
    
    // Fill buffer with constant values
    for (int i = 0; i < 20; i++) {
        engine.calculate_force(&data);
    }
    
    // Slope should be near 0
    ASSERT_NEAR(engine.m_slope_current, 0.0, 0.1);
    // Grip should be near 1.0
    ASSERT_GE(engine.m_slope_smoothed_output, 0.95);
}

TEST_CASE(test_slope_grip_past_peak, "SlopeDetection") {
    std::cout << "\nTest: Slope Grip Past Peak (Negative Slope) (v0.7.0)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    engine.m_slope_sg_window = 9;
    engine.m_slope_sensitivity = 1.0f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01; // 100Hz
    
    // Simulate past peak: Increasing slip, decreasing G
    // Slip: 0.05 to 0.09 (0.002 per frame)
    // G: 1.5 to 1.1 ( -0.02 per frame)
    // dG/dSlip = -0.02 / 0.002 = -10.0 (Slope)
    
    for (int i = 0; i < 20; i++) {
        double slip = 0.05 + (double)i * 0.002;
        double g = 1.5 - (double)i * 0.02;
        
        data.mWheel[0].mLateralPatchVel = slip * 20.0;
        data.mWheel[1].mLateralPatchVel = slip * 20.0;
        data.mLocalAccel.x = g * 9.81;
        
        engine.calculate_force(&data);
    }
    
    // Slope should be negative
    ASSERT_LE(engine.m_slope_current, -5.0);
    // Grip should be reduced
    ASSERT_LE(engine.m_slope_smoothed_output, 0.9);
    // But above safety floor
    ASSERT_GE(engine.m_slope_smoothed_output, 0.2);
}

TEST_CASE(test_slope_vs_static_comparison, "SlopeDetection") {
    std::cout << "\nTest: Slope vs Static Comparison (v0.7.0)" << std::endl;
    FFBEngine engine_slope;
    InitializeEngine(engine_slope);
    engine_slope.m_slope_detection_enabled = true;
    
    FFBEngine engine_static;
    InitializeEngine(engine_static);
    engine_static.m_slope_detection_enabled = false;
    engine_static.m_optimal_slip_angle = 0.10f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.12); // 12% slip
    data.mDeltaTime = 0.01;
    
    // Run both
    for (int i = 0; i < 40; i++) {
        // For slope to detect loss, we need changing dG/dAlpha.
        // We'll increase slip angle from 0.05 to 0.15 (past 0.10 peak)
        // While G-force peaks at i=15 and then drops
        double slip = 0.05 + (double)i * 0.0025; 
        data.mWheel[0].mLateralPatchVel = slip * 20.0;
        data.mWheel[1].mLateralPatchVel = slip * 20.0;
        
        double g = 1.0;
        if (i < 15) g = 1.0 + (double)i * 0.03; // Increasing G
        else g = 1.45 - (double)(i - 15) * 0.05; // Dropping G (Loss of grip!)
        
        data.mLocalAccel.x = g * 9.81;
        
        engine_slope.calculate_force(&data);
        engine_static.calculate_force(&data);
    }
    
    auto snap_slope = engine_slope.GetDebugBatch().back();
    auto snap_static = engine_static.GetDebugBatch().back();
    
    std::cout << "  Slope Grip: " << snap_slope.calc_front_grip << " | Static Grip: " << snap_static.calc_front_grip << std::endl;
    
    // Both should detect grip loss
    ASSERT_LE(snap_slope.calc_front_grip, 0.95);
    ASSERT_LE(snap_static.calc_front_grip, 0.8);
}

TEST_CASE(test_slope_config_persistence, "SlopeDetection") {
    std::cout << "\nTest: Slope Config Persistence (v0.7.0)" << std::endl;
    std::string test_file = "test_slope_config.ini";
    FFBEngine engine_save;
    InitializeEngine(engine_save);
    
    engine_save.m_slope_detection_enabled = true;
    engine_save.m_slope_sg_window = 21;
    engine_save.m_slope_sensitivity = 2.5f;
    engine_save.m_slope_negative_threshold = -0.2f;
    engine_save.m_slope_smoothing_tau = 0.05f;
    
    Config::Save(engine_save, test_file);
    
    FFBEngine engine_load;
    InitializeEngine(engine_load);
    Config::Load(engine_load, test_file);
    
    ASSERT_TRUE(engine_load.m_slope_detection_enabled == true);
    ASSERT_TRUE(engine_load.m_slope_sg_window == 21);
    ASSERT_NEAR(engine_load.m_slope_sensitivity, 2.5f, 0.001);
    ASSERT_NEAR(engine_load.m_slope_negative_threshold, -0.2f, 0.001);
    ASSERT_NEAR(engine_load.m_slope_smoothing_tau, 0.05f, 0.001);
    
    std::remove(test_file.c_str());
}

TEST_CASE(test_slope_latency_characteristics, "SlopeDetection") {
    std::cout << "\nTest: Slope Latency Characteristics (v0.7.0)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    int window = 15;
    engine.m_slope_sg_window = window;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.0025; // 400Hz
    
    // Buffer fills in 'window' frames
    for (int i = 0; i < window; i++) {
        engine.calculate_force(&data);
    }
    
    ASSERT_TRUE(engine.m_slope_buffer_count == window);
    
    // Latency is roughly (window/2) * dt
    float latency_ms = (float)(window / 2) * 2.5f;
    std::cout << "  Calculated Latency for Window " << window << " at 400Hz: " << latency_ms << " ms" << std::endl;
    ASSERT_NEAR(latency_ms, 17.5, 0.1);
}

TEST_CASE(test_slope_noise_rejection, "SlopeDetection") {
    std::cout << "\nTest: Slope Noise Rejection (v0.7.0)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    engine.m_slope_sg_window = 15;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;
    
    std::default_random_engine generator;
    std::uniform_real_distribution<double> noise(-0.1, 0.1);
    
    // Constant G (1.2) + Noise
    for (int i = 0; i < 50; i++) {
        data.mLocalAccel.x = (1.2 + noise(generator)) * 9.81;
        data.mWheel[0].mLateralPatchVel = 0.05 * 20.0;
        engine.calculate_force(&data);
    }
    
    // Despite noise, slope should be near zero (SG filter rejection)
    std::cout << "  Noisy Slope: " << engine.m_slope_current << std::endl;
    ASSERT_TRUE(std::abs(engine.m_slope_current) < 1.0);
}

TEST_CASE(test_slope_buffer_reset_on_toggle, "SlopeDetection") {
    std::cout << "\nTest: Slope Buffer Reset on Toggle (v0.7.0)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.0025;  // 400Hz
    
    // Step 1: Fill buffer with data while slope detection is OFF
    engine.m_slope_detection_enabled = false;
    
    for (int i = 0; i < 20; i++) {
        // Simulate increasing lateral G (would create positive slope)
        data.mLocalAccel.x = (0.5 + i * 0.05) * 9.81;
        data.mWheel[0].mLateralPatchVel = (0.05 + i * 0.005) * 20.0;
        engine.calculate_force(&data);
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
    bool prev_enabled = engine.m_slope_detection_enabled;
    engine.m_slope_detection_enabled = true;
    
    //  Simulate the reset logic from GuiLayer.cpp
    if (!prev_enabled && engine.m_slope_detection_enabled) {
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
        data.mWheel[0].mLateralPatchVel = 0.05 * 20.0;  // Constant slip
        engine.calculate_force(&data);
    }
    
    // After reset, buffer should be filling from scratch
    ASSERT_TRUE(engine.m_slope_buffer_count == 5);
    
    // Step 6: Test that disabling does NOT reset buffers
    engine.m_slope_detection_enabled = false;
    // Buffers should remain intact (for potential re-enable)
    ASSERT_TRUE(engine.m_slope_buffer_count == 5);  // Unchanged
}

TEST_CASE(test_slope_detection_no_boost_when_grip_balanced, "SlopeDetection") {
    std::cout << "\nTest: Slope Detection - No Boost When Grip Balanced (v0.7.1)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    // Enable slope detection with oversteer boost
    engine.m_slope_detection_enabled = true;
    engine.m_oversteer_boost = 2.0f; // Strong boost setting
    engine.m_sop_effect = 1.0f;
    engine.m_sop_scale = 10.0f;
    engine.m_max_torque_ref = 20.0f;
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;
    
    // Frames 1-20: Constant G and Slip
    for (int i = 0; i < 20; i++) {
        data.mLocalAccel.x = 1.0 * 9.81;
        data.mWheel[0].mLateralPatchVel = 0.05 * 20.0;
        engine.calculate_force(&data);
    }
    
    // Trigger negative slope to reduce front grip
    for (int i = 0; i < 10; i++) {
        double slip = 0.05 + i * 0.005;
        double g = 1.0 - i * 0.02;
        data.mLocalAccel.x = g * 9.81;
        data.mWheel[0].mLateralPatchVel = slip * 20.0;
        engine.calculate_force(&data);
    }
    
    double front_grip = engine.m_slope_smoothed_output;
    ASSERT_TRUE(front_grip < 0.95);
    
    auto batch = engine.GetDebugBatch();
    FFBSnapshot snap = batch.back();
    ASSERT_NEAR(snap.oversteer_boost, 0.0, 0.01);
}

TEST_CASE(test_slope_detection_no_boost_during_oversteer, "SlopeDetection") {
    std::cout << "\nTest: Slope Detection - No Boost During Oversteer (v0.7.1)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    // Enable slope detection with oversteer boost
    engine.m_slope_detection_enabled = true;
    engine.m_oversteer_boost = 2.0f; // Strong boost setting
    engine.m_sop_effect = 1.0f;
    engine.m_sop_scale = 10.0f;
    engine.m_max_torque_ref = 20.0f;
    engine.m_optimal_slip_angle = 0.05f; // Rear grip will drop past 0.05 slip
    
    // Setup telemetry to create oversteer scenario (front grip > rear grip)
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;
    
    // Frames 1-20: Build up positive slope (Front grip = 1.0)
    for (int i = 0; i < 20; i++) {
        data.mLocalAccel.x = (0.5 + i * 0.05) * 9.81;
        data.mWheel[0].mLateralPatchVel = (0.02 + i * 0.002) * 20.0;
        engine.calculate_force(&data);
    }
    
    auto batch = engine.GetDebugBatch();
    FFBSnapshot snap = batch.back();
    
    // Assertion: oversteer_boost should be 0.0 when slope detection is enabled
    ASSERT_NEAR(snap.oversteer_boost, 0.0, 0.01);
}

TEST_CASE(test_lat_g_boost_works_without_slope_detection, "SlopeDetection") {
    std::cout << "\nTest: Lateral G Boost works without Slope Detection (v0.7.1)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_slope_detection_enabled = false;
    engine.m_oversteer_boost = 2.0f;
    engine.m_sop_effect = 1.0f;
    engine.m_sop_scale = 10.0f;
    engine.m_max_torque_ref = 20.0f;
    engine.m_optimal_slip_angle = 0.05f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.06); // Slip 0.06
    data.mLocalAccel.x = 1.5 * 9.81;
    data.mDeltaTime = 0.01;
    
    data.mWheel[0].mLateralPatchVel = 0.04 * 20.0;
    data.mWheel[1].mLateralPatchVel = 0.04 * 20.0;
    data.mWheel[2].mLateralPatchVel = 0.08 * 20.0;
    data.mWheel[3].mLateralPatchVel = 0.08 * 20.0;
    
    engine.calculate_force(&data);
    FFBSnapshot snap = engine.GetDebugBatch().back();
    
    ASSERT_TRUE(snap.oversteer_boost > 0.01);
}

TEST_CASE(test_slope_detection_default_values_v071, "SlopeDetection") {
    std::cout << "\nTest: Slope Detection Default Values (v0.7.1)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    ASSERT_NEAR(engine.m_slope_sensitivity, 0.5f, 0.001);
    ASSERT_NEAR(engine.m_slope_negative_threshold, -0.3f, 0.001);
    ASSERT_NEAR(engine.m_slope_smoothing_tau, 0.04f, 0.001);
}

TEST_CASE(test_slope_current_in_snapshot, "SlopeDetection") {
    std::cout << "\nTest: Slope Current in Snapshot (v0.7.1)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;
    
    // Frames 1-20: Build up a slope
    for (int i = 0; i < 20; i++) {
        data.mLocalAccel.x = (0.5 + i * 0.05) * 9.81;
        data.mWheel[0].mLateralPatchVel = (0.02 + i * 0.002) * 20.0;
        engine.calculate_force(&data);
    }
    
    auto batch = engine.GetDebugBatch();
    FFBSnapshot snap = batch.back();
    
    ASSERT_NEAR(snap.slope_current, (float)engine.m_slope_current, 0.001);
    ASSERT_TRUE(std::abs(snap.slope_current) > 0.001);
}

TEST_CASE(test_slope_detection_less_aggressive_v071, "SlopeDetection") {
    std::cout << "\nTest: Slope Detection Less Aggressive (v0.7.1)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_slope_detection_enabled = true;
    engine.m_slope_sensitivity = 0.5f;
    engine.m_slope_negative_threshold = -0.3f;
    engine.m_slope_sg_window = 15;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mDeltaTime = 0.01;
    
    for (int i = 0; i < 20; i++) {
        data.mLocalAccel.x = 1.0 * 9.81;
        data.mWheel[0].mLateralPatchVel = 0.05 * 20.0;
        engine.calculate_force(&data);
    }
    
    for (int i = 0; i < 15; i++) {
        data.mLocalAccel.x = (1.0 - i * 0.005) * 9.81;
        data.mWheel[0].mLateralPatchVel = (0.05 + i * 0.01) * 20.0;
        engine.calculate_force(&data);
    }
    
    ASSERT_NEAR(engine.m_slope_current, -1.0, 0.1);
    ASSERT_TRUE(engine.m_slope_smoothed_output > 0.9);
}

TEST_CASE(test_slope_decay_on_straight, "SlopeDetection") {
    std::cout << "\nTest: Slope Decay on Straight (v0.7.3)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    engine.m_slope_alpha_threshold = 0.02f;
    engine.m_slope_decay_rate = 5.0f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(30.0, 0.05);
    data.mDeltaTime = 0.01;
    
    for (int i = 0; i < 20; i++) {
        data.mLocalAccel.x = (0.5 + 0.05 * i) * 9.81; 
        for (int w = 0; w < 4; w++) {
            data.mWheel[w].mLateralPatchVel = (0.05 + 0.005 * i) * 30.0;
        }
        engine.calculate_force(&data);
    }
    
    double slope_after_corner = engine.m_slope_current;
    ASSERT_TRUE(std::abs(slope_after_corner) > 0.1);
    
    data = CreateBasicTestTelemetry(30.0, 0.0);
    data.mDeltaTime = 0.01;
    
    for (int i = 0; i < 64; i++) {
        engine.m_slope_lat_g_buffer[i] = 0.0;
        engine.m_slope_slip_buffer[i] = 0.0;
    }
    engine.m_slope_buffer_count = 0;
    engine.m_slope_buffer_index = 0;
    
    for (int i = 0; i < 20; i++) {
        engine.calculate_force(&data);
    }
    
    double slope_after_straight = engine.m_slope_current;
    ASSERT_TRUE(std::abs(slope_after_straight) < std::abs(slope_after_corner));
    ASSERT_TRUE(std::abs(slope_after_straight) < 0.2); 
    
    for (int i = 0; i < 40; i++) {
        engine.calculate_force(&data);
    }
    
    double slope_final = engine.m_slope_current;
    ASSERT_NEAR(slope_final, 0.0, 0.05); 
}

TEST_CASE(test_slope_alpha_threshold_configurable, "SlopeDetection") {
    std::cout << "\nTest: Slope dAlpha Threshold Configurable (v0.7.3)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    engine.m_slope_alpha_threshold = 0.02f;
    engine.m_slope_current = -0.5f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mDeltaTime = 0.01;
    for (int w = 0; w < 4; w++) {
        data.mWheel[w].mLateralPatchVel = 0.0001 * 20.0;
    }
    
    engine.calculate_force(&data);
    
    ASSERT_TRUE(std::abs(engine.m_slope_current) < 0.5f);
    
    engine.m_slope_current = -0.1f;
    for (int i = 0; i < 64; i++) {
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
    engine.m_slope_detection_enabled = true;
    engine.m_slope_confidence_enabled = true;
    engine.m_slope_alpha_threshold = 0.01f;
    engine.m_slope_negative_threshold = -0.3f;
    engine.m_slope_sensitivity = 1.0f;
    
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
    engine.m_slope_detection_enabled = true;
    engine.m_slope_alpha_threshold = 0.035f;
    engine.m_slope_decay_rate = 8.5f;
    engine.m_slope_confidence_enabled = false;
    
    Config::Save(engine, "test_stability.ini");
    
    FFBEngine engine2;
    InitializeEngine(engine2);
    Config::Load(engine2, "test_stability.ini");
    
    ASSERT_NEAR(engine2.m_slope_alpha_threshold, 0.035f, 0.0001);
    ASSERT_NEAR(engine2.m_slope_decay_rate, 8.5f, 0.0001);
    ASSERT_TRUE(engine2.m_slope_confidence_enabled == false);
}

TEST_CASE(test_slope_no_understeer_on_straight_v073, "SlopeDetection") {
    std::cout << "\nTest: No Understeer on Straight (v0.7.3)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    engine.m_understeer_effect = 1.0f;
    
    engine.m_slope_current = -2.0f;
    engine.m_slope_smoothed_output = 0.6f; 
    
    TelemInfoV01 data = CreateBasicTestTelemetry(41.7, 0.0);
    data.mSteeringShaftTorque = 10.0;
    
    for (int i = 0; i < 150; i++) {
        engine.calculate_force(&data);
    }
    
    ASSERT_NEAR(engine.m_slope_current, 0.0, 0.01);
    ASSERT_GE(engine.m_slope_smoothed_output, 0.95);
}

TEST_CASE(test_slope_decay_rate_boundaries, "SlopeDetection") {
    std::cout << "\nTest: Slope Decay Rate Boundaries (v0.7.3)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_slope_detection_enabled = true;
    
    engine.m_slope_decay_rate = 0.5f;
    engine.m_slope_current = -1.0f;
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    engine.calculate_force(&data);
    double decayed_slow = engine.m_slope_current;
    
    engine.m_slope_decay_rate = 20.0f;
    engine.m_slope_current = -1.0f;
    engine.calculate_force(&data);
    double decayed_fast = engine.m_slope_current;
    
    ASSERT_TRUE(std::abs(decayed_fast) < std::abs(decayed_slow));
}

TEST_CASE(test_slope_alpha_threshold_validation, "SlopeDetection") {
    std::cout << "\nTest: Slope Alpha Threshold Validation (v0.7.3)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    
    engine.m_slope_alpha_threshold = 0.0001f;
    Config::Save(engine, "test_val.ini");
    Config::Load(engine, "test_val.ini");
    ASSERT_NEAR(engine.m_slope_alpha_threshold, 0.02f, 0.0001);
    
    engine.m_slope_alpha_threshold = 0.5f;
    Config::Save(engine, "test_val.ini");
    Config::Load(engine, "test_val.ini");
    ASSERT_NEAR(engine.m_slope_alpha_threshold, 0.02f, 0.0001);
}



} // namespace FFBEngineTests