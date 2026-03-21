#include "test_ffb_common.h"
#include "StringUtils.h"

namespace FFBEngineTests {

TEST_CASE(test_approximate_load_fallback, "SlipGrip") {
    std::cout << "\nTest: Approximate Load Fallback (mSuspForce based)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    // Setup
    data.mWheel[0].mTireLoad = 0.0; // Trigger Fallback
    data.mWheel[1].mTireLoad = 0.0;
    data.mWheel[0].mSuspForce = 2000.0;
    data.mWheel[1].mSuspForce = 2000.0;
    data.mLocalVel.z = 20.0;
    data.mDeltaTime = 0.01;
    
    // Run multiple frames to trigger hysteresis (>20)
    for (int i=0; i<30; i++) {
        data.mElapsedTime += 0.01;
        engine.calculate_force(&data, "GT3", "TestCar");
    }
    
    auto batch = engine.GetDebugBatch();
    float load = batch.back().calc_front_load;
    
    // Improved Fallback v0.7.171
    // GT3: MR = 0.65, Offset = 500N
    // Expected: (2000 * 0.65) + 500 = 1800
    ASSERT_NEAR(load, 1800.0, 1.0);
}

TEST_CASE(test_combined_grip_loss, "SlipGrip") {
    std::cout << "\nTest: Combined Friction Circle" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    // Setup: Full Grip Telemetry (1.0), but we force fallback
    data.mWheel[0].mGripFract = 0.0; 
    data.mWheel[1].mGripFract = 0.0;
    data.mWheel[0].mTireLoad = 4000.0; // Load present
    data.mWheel[1].mTireLoad = 4000.0;
    data.mLocalVel.z = -20.0;
    
    // Case 1: Straight Line, No Slip
    data.mWheel[0].mStaticUndeflectedRadius = 30;
    data.mWheel[0].mRotation = 20.0 / 0.3; // Match speed
    data.mWheel[1].mStaticUndeflectedRadius = 30;
    data.mWheel[1].mRotation = 20.0 / 0.3;
    data.mDeltaTime = 0.01;
    
    engine.calculate_force(&data, "GT3", "TestCar");
    
    // Case 2: Braking Lockup (Slip Ratio -1.0)
    data.mWheel[0].mRotation = 0.0;
    data.mWheel[1].mRotation = 0.0;
    data.mWheel[0].mLongitudinalPatchVel = -20.0; // Full lock
    data.mWheel[1].mLongitudinalPatchVel = -20.0;
    
    // Issue #397: Flush and Measure
    PumpEngineTime(engine, data, 0.015);
    auto batch = engine.GetDebugBatch();
    float grip = batch.back().calc_front_grip;
    
    if (grip < 0.5) {
        std::cout << "[PASS] Grip dropped due to Longitudinal Slip (" << grip << ")" << std::endl;
        g_tests_passed++;
    } else {
        FAIL_TEST("Grip remained high despite lockup. Value: " << grip);
    }
}

TEST_CASE(test_rear_force_workaround, "SlipGrip") {
    std::cout << "\nTest: Rear Force Workaround (v0.4.10)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    engine.m_rear_axle.sop_effect = 1.0;
    engine.m_rear_axle.oversteer_boost = 1.0;
    engine.m_general.gain = 1.0;
    engine.m_rear_axle.sop_scale = 10.0;
    engine.m_rear_axle.rear_align_effect = 1.0f;
    engine.m_invert_force = false;
    engine.m_general.wheelbase_max_nm = 100.0f; engine.m_general.target_rim_nm = 100.0f;
    engine.m_grip_estimation.slip_angle_smoothing = 0.0f; // Instant

    data.mLocalVel.z = -20.0;
    data.mDeltaTime = 0.01;
    
    // Missing load on all wheels to trigger fallback
    for(int i=0; i<4; i++) data.mWheel[i].mTireLoad = 0.0;

    // Front Wheels
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;
    data.mWheel[0].mSuspForce = 4000.0;
    data.mWheel[1].mSuspForce = 4000.0;
    data.mWheel[0].mLongitudinalGroundVel = 20.0;
    data.mWheel[1].mLongitudinalGroundVel = 20.0;
    
    // Rear Wheels
    data.mWheel[2].mLateralForce = 0.0;
    data.mWheel[3].mLateralForce = 0.0;
    data.mWheel[2].mSuspForce = 3000.0;
    data.mWheel[3].mSuspForce = 3000.0;
    data.mWheel[2].mGripFract = 0.0; // Trigger approximation
    data.mWheel[3].mGripFract = 0.0;
    
    data.mWheel[2].mLateralPatchVel = 5.0; // Slip angle
    data.mWheel[3].mLateralPatchVel = 5.0;
    data.mWheel[2].mLongitudinalGroundVel = 20.0;
    data.mWheel[3].mLongitudinalGroundVel = 20.0;
    
    // Run for >20 frames to trigger load fallback
    for(int i=0; i<30; i++) {
        data.mElapsedTime += 0.01;
        engine.calculate_force(&data, "GT3", "TestCar");
    }
    
    auto batch = engine.GetDebugBatch();
    ASSERT_FALSE(batch.empty());
    FFBSnapshot snap = batch.back();
    
    std::cout << "  Rear Torque Snap: " << snap.ffb_rear_torque << std::endl;
    ASSERT_TRUE(std::abs(snap.ffb_rear_torque) > 1.0);
}

TEST_CASE(test_rear_align_effect, "SlipGrip") {
    std::cout << "\nTest: Rear Align Effect Decoupling (v0.4.11)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    engine.m_rear_axle.rear_align_effect = 2.0f;
    engine.m_rear_axle.oversteer_boost = 0.0f;
    engine.m_rear_axle.sop_effect = 0.0f;
    engine.m_general.wheelbase_max_nm = 100.0f; engine.m_general.target_rim_nm = 100.0f;
    engine.m_grip_estimation.slip_angle_smoothing = 0.0f;
    
    data.mLocalVel.z = -20.0;
    data.mDeltaTime = 0.01;
    
    // Missing load on all wheels
    for(int i=0; i<4; i++) data.mWheel[i].mTireLoad = 0.0;

    data.mWheel[0].mGripFract = 1.0; data.mWheel[1].mGripFract = 1.0;
    data.mWheel[0].mSuspForce = 4000.0; data.mWheel[1].mSuspForce = 4000.0;
    
    data.mWheel[2].mLateralForce = 0.0;
    data.mWheel[3].mLateralForce = 0.0;
    data.mWheel[2].mSuspForce = 3000.0;
    data.mWheel[3].mSuspForce = 3000.0;
    data.mWheel[2].mGripFract = 0.0;
    data.mWheel[3].mGripFract = 0.0;
    
    data.mWheel[2].mLateralPatchVel = 5.0;
    data.mWheel[3].mLateralPatchVel = 5.0;
    data.mWheel[2].mLongitudinalGroundVel = 20.0;
    data.mWheel[3].mLongitudinalGroundVel = 20.0;
    
    for(int i=0; i<30; i++) {
        data.mElapsedTime += 0.01;
        engine.calculate_force(&data, "GT3", "TestCar");
    }
    
    auto batch = engine.GetDebugBatch();
    ASSERT_FALSE(batch.empty());
    FFBSnapshot snap = batch.back();

    ASSERT_TRUE(std::abs(snap.ffb_rear_torque) > 1.0);
}

TEST_CASE(test_rear_grip_fallback, "SlipGrip") {
    std::cout << "\nTest: Rear Grip Fallback (v0.4.5)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    data.mWheel[0].mRideHeight = 0.1; data.mWheel[1].mRideHeight = 0.1;
    engine.m_rear_axle.sop_effect = 1.0;
    engine.m_rear_axle.oversteer_boost = 1.0;
    engine.m_general.gain = 1.0;
    engine.m_rear_axle.sop_scale = 10.0;
    engine.m_general.wheelbase_max_nm = 20.0f; engine.m_general.target_rim_nm = 20.0f;
    
    data.mLocalAccel.x = 9.81;
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;
    data.mWheel[0].mTireLoad = 4000.0;
    data.mWheel[1].mTireLoad = 4000.0;
    data.mWheel[0].mSuspForce = 4000.0;
    data.mWheel[1].mSuspForce = 4000.0;
    
    data.mWheel[2].mGripFract = 0.0;
    data.mWheel[3].mGripFract = 0.0;
    data.mWheel[2].mTireLoad = 4000.0;
    data.mWheel[3].mTireLoad = 4000.0;
    data.mWheel[2].mSuspForce = 4000.0;
    data.mWheel[3].mSuspForce = 4000.0;
    
    data.mWheel[2].mLongitudinalGroundVel = 20.0;
    data.mWheel[3].mLongitudinalGroundVel = 20.0;
    data.mWheel[2].mLongitudinalGroundVel = 20.0;
    data.mWheel[3].mLongitudinalGroundVel = 20.0;
    data.mWheel[2].mLateralPatchVel = 0.0;
    data.mWheel[3].mLateralPatchVel = 0.0;
    
    // Seeding call
    FFBEngineTestAccess::SetDerivativesSeeded(engine, false);
    engine.calculate_force(&data, "GT3", "TestCar");
    // Physics call
    data.mElapsedTime += 0.01;
    engine.calculate_force(&data, "GT3", "TestCar");
    
    ASSERT_TRUE(engine.m_grip_diag.rear_approximated);
    
    auto batch = engine.GetDebugBatch();
    ASSERT_FALSE(batch.empty());
    float boost = batch.back().oversteer_boost;
    ASSERT_NEAR(boost, 0.0f, 0.01f);
}

TEST_CASE(test_load_factor_edge_cases, "SlipGrip") {
    std::cout << "\nTest: Load Factor Edge Cases" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    data.mWheel[0].mRideHeight = 0.1; data.mWheel[1].mRideHeight = 0.1;
    engine.m_vibration.slide_enabled = true;
    engine.m_vibration.slide_gain = 1.0;
    engine.m_front_axle.understeer_effect = 0.0f;
    engine.m_rear_axle.sop_effect = 0.0f;
    engine.m_vibration.bottoming_enabled = false;
    
    data.mWheel[0].mLateralPatchVel = 5.1; // Changed to avoid exact zero in sawtooth
    data.mWheel[1].mLateralPatchVel = 5.1;
    data.mWheel[0].mGripFract = 0.0; // Low grip to trigger scale
    data.mWheel[1].mGripFract = 0.0;
    data.mLocalVel.z = 20.0;
    data.mDeltaTime = 0.01;
    engine.m_general.wheelbase_max_nm = 20.0f; engine.m_general.target_rim_nm = 20.0f;
    
    // Set low but non-zero suspension force
    data.mWheel[0].mSuspForce = 100.0;
    data.mWheel[1].mSuspForce = 100.0;
    data.mWheel[0].mTireLoad = 0.0;
    data.mWheel[1].mTireLoad = 0.0;
    
    for(int i=0; i<30; i++) {
        data.mElapsedTime += 0.01;
        engine.calculate_force(&data, "GT3", "TestCar");
    }

    auto batch = engine.GetDebugBatch();
    ASSERT_FALSE(batch.empty());
    float slide_texture = batch.back().texture_slide;
    std::cout << "  Slide Texture Snap: " << slide_texture << std::endl;
    ASSERT_TRUE(std::abs(slide_texture) > 0.00001);
}

TEST_CASE(test_missing_telemetry_warnings, "SlipGrip") {
    std::cout << "\nTest: Missing Telemetry Warnings (v0.6.3)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    StringUtils::SafeCopy(data.mVehicleName, sizeof(data.mVehicleName), "TestCar_GT3");

    std::stringstream logBuffer;
    Logger::Get().SetTestStream(&logBuffer);

    // Initial car setup (trigger car change reset)
    FFBEngineTestAccess::CallInitializeLoadReference(engine, "GT3", "TestCar_GT3");

    data.mElapsedTime += 0.01;
    engine.calculate_force(&data, "GT3", "TestCar_GT3");
    ASSERT_TRUE(logBuffer.str().find("Warning: Data for mGripFract") != std::string::npos);

    logBuffer.str("");
    for(int i=0; i<60; i++) {
        data.mElapsedTime += 0.01;
        for(int j=0; j<4; j++) data.mWheel[j].mSuspForce = 0.0;
        engine.calculate_force(&data, "GT3", "TestCar_GT3");
    }
    ASSERT_TRUE(logBuffer.str().find("Warning: Data for mSuspForce") != std::string::npos);

    logBuffer.str("");
    for(int i=0; i<60; i++) {
        data.mElapsedTime += 0.01;
        for(int j=0; j<4; j++) data.mWheel[j].mVerticalTireDeflection = 0.0;
        engine.calculate_force(&data, "GT3", "TestCar_GT3");
    }
    ASSERT_TRUE(logBuffer.str().find("[WARNING] mVerticalTireDeflection is missing") != std::string::npos);

    Logger::Get().SetTestStream(nullptr);
}

TEST_CASE(test_hysteresis_logic, "SlipGrip") {
    std::cout << "\nTest: Hysteresis Logic (Missing Data)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    data.mWheel[0].mRideHeight = 0.1; data.mWheel[1].mRideHeight = 0.1;
    data.mLocalVel.z = 10.0;
    engine.m_vibration.slide_enabled = true;
    engine.m_vibration.slide_gain = 1.0;
    
    data.mWheel[0].mTireLoad = 4000.0;
    data.mWheel[1].mTireLoad = 4000.0;
    data.mWheel[0].mLateralPatchVel = 5.0;
    data.mWheel[1].mLateralPatchVel = 5.0;
    data.mDeltaTime = 0.01;

    engine.calculate_force(&data, "GT3", "TestCar");
    ASSERT_TRUE(engine.m_missing_load_frames == 0);

    data.mWheel[0].mTireLoad = 0.0;
    data.mWheel[1].mTireLoad = 0.0;
    for (int i=0; i<5; i++) engine.calculate_force(&data, "GT3", "TestCar");
    ASSERT_TRUE(engine.m_missing_load_frames == 5);

    for (int i=0; i<20; i++) engine.calculate_force(&data, "GT3", "TestCar");
    ASSERT_TRUE(engine.m_missing_load_frames >= 25);
    ASSERT_TRUE(engine.m_warned_load);
}

} // namespace FFBEngineTests
