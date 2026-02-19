#include "test_ffb_common.h"

namespace FFBEngineTests {

// [Regression][Texture] Road texture toggle spike fix
TEST_CASE(test_regression_road_texture_toggle, "RoadTexture") {
    std::cout << "\nTest: Regression - Road Texture Toggle Spike [Regression][Texture]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    engine.m_road_texture_enabled = false;
    engine.calculate_force(&data);
    data.mWheel[0].mVerticalTireDeflection = 0.05;
    engine.m_road_texture_enabled = true;
    double f = engine.calculate_force(&data);
    ASSERT_TRUE(std::abs(f) < 0.1);
}

// [Regression][Texture] Bottoming method switch spike fix
TEST_CASE(test_regression_bottoming_switch, "RoadTexture") {
    std::cout << "\nTest: Regression - Bottoming Method Switch Spike [Regression][Texture]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    engine.m_bottoming_enabled = true;
    engine.m_bottoming_method = 0;
    engine.calculate_force(&data);
    engine.m_bottoming_method = 1;
    double f = engine.calculate_force(&data);
    ASSERT_NEAR(f, 0.0, 0.001);
}

// [Texture][Physics] Road texture teleport delta clamp
TEST_CASE(test_road_texture_teleport, "RoadTexture") {
    std::cout << "\nTest: Road Texture Teleport (Delta Clamp) [Texture][Physics]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    FFBEngineTestAccess::SetAutoPeakLoad(engine, 4000.0);
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.01;
    
    // Disable Bottoming
    engine.m_bottoming_enabled = false;
    data.mLocalVel.z = -20.0; // Moving fast (v0.6.21)

    engine.m_road_texture_enabled = true;
    engine.m_road_texture_gain = 1.0;
    engine.m_wheelbase_max_nm = 40.0f; engine.m_target_rim_nm = 40.0f;
    engine.m_gain = 1.0; // Ensure gain is 1.0
    engine.m_invert_force = false;
    
    // Frame 1: 0.0
    data.mWheel[0].mVerticalTireDeflection = 0.0;
    data.mWheel[1].mVerticalTireDeflection = 0.0;
    data.mWheel[0].mTireLoad = 4000.0; // Load Factor 1.0
    data.mWheel[1].mTireLoad = 4000.0;
    engine.calculate_force(&data);
    
    // Frame 2: Teleport (+0.1m)
    data.mWheel[0].mVerticalTireDeflection = 0.1;
    data.mWheel[1].mVerticalTireDeflection = 0.1;
    
    // Without Clamp:
    // Delta = 0.1. Sum = 0.2.
    // Force = 0.2 * 50.0 = 10.0.
    // Norm = 1.0 / 40.0 = 0.25.
    
    // With Clamp (+/- 0.01):
    // Delta clamped to 0.01. Sum = 0.02.
    // Force = 0.02 * 50.0 = 1.0 Nm.
    // Norm (Physical Target) = 1.0 / wheelbase_max = 1.0 / 40.0 = 0.025.
    
    double force = engine.calculate_force(&data);
    
    // Check if clamped
    if (std::abs(force - 0.025) < 0.001) {
        std::cout << "[PASS] Teleport spike clamped." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Teleport spike unclamped? Got " << force << " Expected 0.025." << std::endl;
        g_tests_failed++;
    }
}

// [Texture][Physics] Suspension bottoming effect
TEST_CASE(test_suspension_bottoming, "RoadTexture") {
    std::cout << "\nTest: Suspension Bottoming (Fix Verification) [Texture][Physics]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));

    // Enable Bottoming
    engine.m_bottoming_enabled = true;
    engine.m_bottoming_gain = 1.0;
    data.mLocalVel.z = -20.0; // Moving fast (v0.6.21)
    
    // Disable others
    engine.m_sop_effect = 0.0;
    engine.m_slide_texture_enabled = false;
    
    // Straight line condition: Zero steering force
    data.mSteeringShaftTorque = 0.0;
    
    // Massive Load Spike (10000N > 8000N threshold)
    data.mWheel[0].mTireLoad = 10000.0;
    data.mWheel[1].mTireLoad = 10000.0;
    data.mDeltaTime = 0.01;
    
    // Run multiple frames to check oscillation
    // Phase calculation: Freq=50. 50 * 0.01 * 2PI = 0.5 * 2PI = PI.
    // Frame 1: Phase = PI. Sin(PI) = 0. Force = 0.
    // Frame 2: Phase = 2PI (0). Sin(0) = 0. Force = 0.
    // Bad luck with 50Hz and 100Hz (0.01s).
    // Let's use dt = 0.005 (200Hz)
    data.mDeltaTime = 0.005; 
    
    // Frame 1: Phase += 50 * 0.005 * 2PI = 0.25 * 2PI = PI/2.
    // Sin(PI/2) = 1.0. 
    // Excess = 2000. Sqrt(2000) ~ 44.7. * 0.5 = 22.35.
    // Force should be approx +22.35 (normalized later by /4000)
    
    engine.calculate_force(&data); // Frame 1
    double force = engine.calculate_force(&data); // Frame 2 (Phase PI, sin 0?)
    
    // Let's check frame 1 explicitly by resetting
    FFBEngine engine2;
    InitializeEngine(engine2); // v0.5.12: Initialize with T300 defaults
    engine2.m_bottoming_enabled = true;
    engine2.m_bottoming_gain = 1.0;
    engine2.m_sop_effect = 0.0;
    engine2.m_slide_texture_enabled = false;
    data.mDeltaTime = 0.005;
    
    double force_f1 = engine2.calculate_force(&data); 
    // Expect ~ 22.35 / 4000 = 0.005
    
    if (std::abs(force_f1) > 0.0001) {
        std::cout << "[PASS] Bottoming effect active. Force: " << force_f1 << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Bottoming effect zero. Phase alignment?" << std::endl;
        g_tests_failed++;
    }
}

// [Texture][Integration] Road texture persistence
TEST_CASE(test_road_texture_state_persistence, "RoadTexture") {
    std::cout << "\nTest: Road Texture State Persistence [Texture][Integration]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_road_texture_enabled = true;
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    data.mWheel[0].mVerticalTireDeflection = 0.01;
    double f1 = engine.calculate_force(&data);
    double f2 = engine.calculate_force(&data);
    ASSERT_NEAR(f1, f2, 0.001);
}

// [Texture][Physics] Universal bottoming (Scrape & Spike)
TEST_CASE(test_universal_bottoming, "RoadTexture") {
    std::cout << "\nTest: Universal Bottoming [Texture][Physics]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_bottoming_enabled = true;
    engine.m_bottoming_gain = 1.0f;
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    
    // Method A: Ride Height (Scrape)
    engine.m_bottoming_method = 0;
    data.mWheel[0].mRideHeight = 0.001;
    
    // Set dt to ensure phase doesn't hit 0 crossing (50Hz)
    // 50Hz period = 0.02s. dt=0.01 is half period. PI. sin(PI)=0.
    // Use dt=0.005 (PI/2). sin(PI/2)=1.
    data.mDeltaTime = 0.005;

    double f1 = engine.calculate_force(&data);
    
    if (std::abs(f1) > 0.0001) {
        std::cout << "[PASS] Bottoming Method A (Scrape) Triggered. Force: " << f1 << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Bottoming Method A (Scrape) Silent." << std::endl;
        g_tests_failed++;
    }
    
    // Method B: Suspension Deflection (Spike) - Using 10000N logic from other test
    engine.m_bottoming_method = 1;
    data.mWheel[0].mRideHeight = 0.1; // Reset RH
    data.mWheel[0].mTireLoad = 10000.0; // Trigger spike
    data.mWheel[1].mTireLoad = 10000.0;
    data.mDeltaTime = 0.005; // 200Hz to catch phase
    
    // Reset Engine to clear phases
    FFBEngine engine2;
    InitializeEngine(engine2);
    engine2.m_bottoming_enabled = true;
    engine2.m_bottoming_gain = 1.0f;
    engine2.m_bottoming_method = 1;
    
    double f2 = engine2.calculate_force(&data);
    
    if (std::abs(f2) > 0.0001) {
        std::cout << "[PASS] Bottoming Method B (Spike) Triggered. Force: " << f2 << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Bottoming Method B (Spike) Silent." << std::endl;
        g_tests_failed++;
    }
}

// [Physics][Integration] Unconditional vertical accel update
TEST_CASE(test_unconditional_vert_accel_update, "RoadTexture") {
    std::cout << "\nTest: Unconditional m_prev_vert_accel Update (v0.6.36) [Physics][Integration]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    engine.m_road_texture_enabled = false;
    data.mLocalAccel.y = 5.5;
    engine.m_prev_vert_accel = 0.0;
    engine.calculate_force(&data);
    ASSERT_NEAR(engine.m_prev_vert_accel, 5.5, 0.01);
}

// [Texture][Physics] Scrub drag fade-in
TEST_CASE(test_scrub_drag_fade, "RoadTexture") {
    std::cout << "\nTest: Scrub Drag Fade-In [Texture][Physics]" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine); // v0.5.12: Initialize with T300 defaults
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    // Disable Bottoming to avoid noise
    engine.m_bottoming_enabled = false;
    // Disable Slide Texture (enabled by default)
    engine.m_slide_texture_enabled = false;

    engine.m_road_texture_enabled = true;
    engine.m_scrub_drag_gain = 1.0;
    
    data.mWheel[0].mLateralPatchVel = 0.25;
    data.mWheel[1].mLateralPatchVel = 0.25;
    data.mLocalVel.z = -20.0; // Moving fast (v0.6.22)
    engine.m_wheelbase_max_nm = 40.0f; engine.m_target_rim_nm = 40.0f;
    engine.m_gain = 1.0;
    
    // v0.7.67 Fix for Issue #152: Ensure normalization matches the test scaling
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 40.0);
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 40.0);
    FFBEngineTestAccess::SetRollingAverageTorque(engine, 40.0);
    FFBEngineTestAccess::SetLastRawTorque(engine, 40.0);

    double force = engine.calculate_force(&data);
    
    // Check absolute magnitude
    // Issue #153: Scrub drag is structural, mapped via target_rim / wheelbase_max.
    // Full force = 5.0 Nm. Fade at 0.25m/s = 50% -> 2.5 Nm.
    // session_peak = 40.0 -> norm_structural = 2.5 / 40.0 = 0.0625.
    // di_structural = 0.0625 * (40/40) = 0.0625.
    if (std::abs(std::abs(force) - 0.0625) < 0.001) {
        std::cout << "[PASS] Scrub drag faded correctly (50%)." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Scrub drag fade incorrect. Got " << force << " Expected 0.0625." << std::endl;
        g_tests_failed++;
    }
}



} // namespace FFBEngineTests
