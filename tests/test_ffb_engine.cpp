#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstring>
#include "../FFBEngine.h"
#include "../rF2Data.h"

// --- Simple Test Framework ---
int g_tests_passed = 0;
int g_tests_failed = 0;

#define ASSERT_TRUE(condition) \
    if (condition) { \
        std::cout << "[PASS] " << #condition << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #condition << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
        g_tests_failed++; \
    }

#define ASSERT_NEAR(a, b, epsilon) \
    if (std::abs((a) - (b)) < (epsilon)) { \
        std::cout << "[PASS] " << #a << " approx " << #b << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #a << " (" << (a) << ") != " << #b << " (" << (b) << ")" << std::endl; \
        g_tests_failed++; \
    }

// --- Tests ---

void test_zero_input() {
    std::cout << "\nTest: Zero Input" << std::endl;
    FFBEngine engine;
    rF2Telemetry data;
    std::memset(&data, 0, sizeof(data));
    
    // Set minimal grip to avoid divide by zero if any
    data.mWheels[0].mGripFract = 1.0;
    data.mWheels[1].mGripFract = 1.0;

    double force = engine.calculate_force(&data);
    ASSERT_NEAR(force, 0.0, 0.001);
}

void test_grip_modulation() {
    std::cout << "\nTest: Grip Modulation (Understeer)" << std::endl;
    FFBEngine engine;
    rF2Telemetry data;
    std::memset(&data, 0, sizeof(data));

    data.mSteeringArmForce = 2000.0; // Half of max ~4000
    // Disable SoP and Texture to isolate
    engine.m_sop_effect = 0.0;
    engine.m_slide_texture_enabled = false;
    engine.m_road_texture_enabled = false;

    // Case 1: Full Grip (1.0) -> Output should be 2000 / 4000 = 0.5
    data.mWheels[0].mGripFract = 1.0;
    data.mWheels[1].mGripFract = 1.0;
    double force_full = engine.calculate_force(&data);
    ASSERT_NEAR(force_full, 0.5, 0.001);

    // Case 2: Half Grip (0.5) -> Output should be 2000 * 0.5 = 1000 / 4000 = 0.25
    data.mWheels[0].mGripFract = 0.5;
    data.mWheels[1].mGripFract = 0.5;
    double force_half = engine.calculate_force(&data);
    ASSERT_NEAR(force_half, 0.25, 0.001);
}

void test_sop_effect() {
    std::cout << "\nTest: SoP Effect" << std::endl;
    FFBEngine engine;
    rF2Telemetry data;
    std::memset(&data, 0, sizeof(data));

    // Disable Game Force
    data.mSteeringArmForce = 0.0;
    engine.m_sop_effect = 0.5; // default
    
    // 0.5 G lateral (4.905 m/s2)
    data.mLocalAccel.x = 4.905;
    
    // Calculation: 
    // LatG = 4.905 / 9.81 = 0.5
    // SoP Force = 0.5 * 0.5 * 1000 = 250
    // Norm Force = 250 / 4000 = 0.0625
    
    double force = engine.calculate_force(&data);
    ASSERT_NEAR(force, 0.0625, 0.001);
}

void test_min_force() {
    std::cout << "\nTest: Min Force" << std::endl;
    FFBEngine engine;
    rF2Telemetry data;
    std::memset(&data, 0, sizeof(data));

    // Ensure we have minimal grip so calculation doesn't zero out somewhere else
    data.mWheels[0].mGripFract = 1.0;
    data.mWheels[1].mGripFract = 1.0;

    // Disable Noise/Textures to ensure they don't add random values
    engine.m_slide_texture_enabled = false;
    engine.m_road_texture_enabled = false;
    engine.m_sop_effect = 0.0;

    data.mSteeringArmForce = 10.0; // Very small force
    engine.m_min_force = 0.10; // 10% min force

    double force = engine.calculate_force(&data);
    // 10 / 4000 = 0.0025. 
    // This is > 0.0001 (deadzone check) but < 0.10.
    // Should be boosted to 0.10.
    
    // Debug print
    if (std::abs(force - 0.10) > 0.001) {
        std::cout << "Debug Min Force: Calculated " << force << " Expected 0.10" << std::endl;
    }
    
    ASSERT_NEAR(force, 0.10, 0.001);
}

void test_progressive_lockup() {
    std::cout << "\nTest: Progressive Lockup" << std::endl;
    FFBEngine engine;
    rF2Telemetry data;
    std::memset(&data, 0, sizeof(data));
    
    engine.m_lockup_enabled = true;
    engine.m_lockup_gain = 1.0;
    engine.m_sop_effect = 0.0;
    engine.m_slide_texture_enabled = false;
    
    data.mSteeringArmForce = 0.0;
    data.mUnfilteredBrake = 1.0;
    
    // Set DeltaTime for phase integration
    data.mDeltaTime = 0.01;
    data.mLocalVel.z = 20.0; // 20 m/s
    
    // Case 1: Low Slip (-0.15). Severity = (0.15 - 0.1) / 0.4 = 0.125
    data.mWheels[0].mSlipRatio = -0.15;
    data.mWheels[1].mSlipRatio = -0.15;
    
    // Ensure data.mDeltaTime is set! 
    data.mDeltaTime = 0.01;
    
    // DEBUG: Manually verify phase logic in test
    // freq = 10 + (20 * 1.5) = 40.0
    // dt = 0.01
    // step = 40 * 0.01 * 6.28 = 2.512
    
    engine.calculate_force(&data); // Frame 1
    // engine.m_lockup_phase should be approx 2.512
    
    double force_low = engine.calculate_force(&data); // Frame 2
    // engine.m_lockup_phase should be approx 5.024
    // sin(5.024) is roughly -0.95.
    // Amp should be non-zero.
    
    // Debug
    // std::cout << "Force Low: " << force_low << " Phase: " << engine.m_lockup_phase << std::endl;

    if (engine.m_lockup_phase == 0.0) {
         // Maybe frequency calculation is zero?
         // Freq = 10 + (20 * 1.5) = 40.
         // Dt = 0.01.
         // Accumulator += 40 * 0.01 * 6.28 = 2.5.
         std::cout << "[FAIL] Phase stuck at 0. Check data inputs." << std::endl;
    }

    ASSERT_TRUE(std::abs(force_low) > 0.00001);
    ASSERT_TRUE(engine.m_lockup_phase != 0.0);
    
    std::cout << "[PASS] Progressive Lockup calculated." << std::endl;
    g_tests_passed++;
}

void test_slide_texture() {
    std::cout << "\nTest: Slide Texture" << std::endl;
    FFBEngine engine;
    rF2Telemetry data;
    std::memset(&data, 0, sizeof(data));
    
    engine.m_slide_texture_enabled = true;
    engine.m_slide_texture_gain = 1.0;
    
    data.mSteeringArmForce = 0.0;
    data.mWheels[0].mSlipAngle = 0.2; // Slipping > 0.15
    data.mWheels[1].mSlipAngle = 0.2;
    data.mDeltaTime = 0.013; // Avoid 0.01 which lands exactly on zero-crossing for 125Hz
    data.mWheels[0].mLateralPatchVel = 5.0; // Use PatchVel as updated in engine
    data.mWheels[1].mLateralPatchVel = 5.0;
    data.mWheels[0].mTireLoad = 1000.0; // Some load
    data.mWheels[1].mTireLoad = 1000.0;
    
    // Run two frames to advance phase
    engine.calculate_force(&data);
    double force = engine.calculate_force(&data);
    
    // We just assert it's non-zero
    if (std::abs(force) > 0.00001) {
        std::cout << "[PASS] Slide texture generated non-zero force: " << force << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Slide texture force is zero" << std::endl;
        g_tests_failed++;
    }
}

void test_dynamic_tuning() {
    std::cout << "\nTest: Dynamic Tuning (GUI Simulation)" << std::endl;
    FFBEngine engine;
    rF2Telemetry data;
    std::memset(&data, 0, sizeof(data));
    
    // Default State: Full Game Force
    data.mSteeringArmForce = 2000.0;
    data.mWheels[0].mGripFract = 1.0;
    data.mWheels[1].mGripFract = 1.0;
    engine.m_understeer_effect = 0.0; // Disabled effect initially
    engine.m_sop_effect = 0.0;
    engine.m_slide_texture_enabled = false;
    engine.m_road_texture_enabled = false;
    
    double force_initial = engine.calculate_force(&data);
    // Should pass through 2000.0 (normalized: 0.5)
    ASSERT_NEAR(force_initial, 0.5, 0.001);
    
    // --- User drags Master Gain Slider to 2.0 ---
    engine.m_gain = 2.0;
    double force_boosted = engine.calculate_force(&data);
    // Should be 0.5 * 2.0 = 1.0
    ASSERT_NEAR(force_boosted, 1.0, 0.001);
    
    // --- User enables Understeer Effect ---
    // And grip drops
    engine.m_gain = 1.0; // Reset gain
    engine.m_understeer_effect = 1.0;
    data.mWheels[0].mGripFract = 0.5;
    data.mWheels[1].mGripFract = 0.5;
    
    double force_grip_loss = engine.calculate_force(&data);
    // 2000 * 0.5 = 1000 -> 0.25 normalized
    ASSERT_NEAR(force_grip_loss, 0.25, 0.001);
    
    std::cout << "[PASS] Dynamic Tuning verified." << std::endl;
    g_tests_passed++;
}

void test_suspension_bottoming() {
    std::cout << "\nTest: Suspension Bottoming (Fix Verification)" << std::endl;
    FFBEngine engine;
    rF2Telemetry data;
    std::memset(&data, 0, sizeof(data));

    // Enable Bottoming
    engine.m_bottoming_enabled = true;
    engine.m_bottoming_gain = 1.0;
    
    // Disable others
    engine.m_sop_effect = 0.0;
    engine.m_slide_texture_enabled = false;
    
    // Straight line condition: Zero steering force
    data.mSteeringArmForce = 0.0;
    
    // Massive Load Spike (10000N > 8000N threshold)
    data.mWheels[0].mTireLoad = 10000.0;
    data.mWheels[1].mTireLoad = 10000.0;
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

void test_oversteer_boost() {
    std::cout << "\nTest: Oversteer Boost (Rear Grip Loss)" << std::endl;
    FFBEngine engine;
    rF2Telemetry data;
    std::memset(&data, 0, sizeof(data));
    
    engine.m_sop_effect = 1.0;
    engine.m_oversteer_boost = 1.0;
    
    // Scenario: Front has grip, rear is sliding
    data.mWheels[0].mGripFract = 1.0; // FL
    data.mWheels[1].mGripFract = 1.0; // FR
    data.mWheels[2].mGripFract = 0.5; // RL (sliding)
    data.mWheels[3].mGripFract = 0.5; // RR (sliding)
    
    // Lateral G (cornering)
    data.mLocalAccel.x = 9.81; // 1G lateral
    
    // Rear lateral force (resisting slide)
    data.mWheels[2].mLateralForce = 2000.0;
    data.mWheels[3].mLateralForce = 2000.0;
    
    double force = engine.calculate_force(&data);
    
    // Expected: SoP boosted by grip delta (0.5) + rear torque
    // Base SoP = 1.0 * 1.0 * 1000 = 1000
    // Boost = 1.0 + (0.5 * 1.0 * 2.0) = 2.0x
    // SoP = 1000 * 2.0 = 2000
    // Rear Torque = 2000 * 0.05 * 1.0 = 100
    // Total SoP = 2100 / 4000 = 0.525
    
    ASSERT_NEAR(force, 0.525, 0.05);
}

void test_phase_wraparound() {
    std::cout << "\nTest: Phase Wraparound (Anti-Click)" << std::endl;
    FFBEngine engine;
    rF2Telemetry data;
    std::memset(&data, 0, sizeof(data));
    
    engine.m_lockup_enabled = true;
    engine.m_lockup_gain = 1.0;
    
    data.mUnfilteredBrake = 1.0;
    data.mWheels[0].mSlipRatio = -0.3;
    data.mWheels[1].mSlipRatio = -0.3;
    data.mLocalVel.z = 20.0; // 20 m/s
    data.mDeltaTime = 0.01;
    
    // Run for 100 frames (should wrap phase multiple times)
    double prev_phase = 0.0;
    int wrap_count = 0;
    
    for (int i = 0; i < 100; i++) {
        engine.calculate_force(&data);
        
        // Check for wraparound
        if (engine.m_lockup_phase < prev_phase) {
            wrap_count++;
            // Verify wrap happened near 2π
            // With freq=40Hz, dt=0.01, step is ~2.5 rad.
            // So prev_phase could be as low as 6.28 - 2.5 = 3.78.
            // We check it's at least > 3.0 to ensure it's not resetting randomly at 0.
            if (!(prev_phase > 3.0)) {
                 std::cout << "[FAIL] Wrapped phase too early: " << prev_phase << std::endl;
                 g_tests_failed++;
            }
        }
        prev_phase = engine.m_lockup_phase;
    }
    
    // Should have wrapped at least once
    if (wrap_count > 0) {
        std::cout << "[PASS] Phase wrapped " << wrap_count << " times without discontinuity." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Phase did not wrap" << std::endl;
        g_tests_failed++;
    }
}

void test_road_texture_state_persistence() {
    std::cout << "\nTest: Road Texture State Persistence" << std::endl;
    FFBEngine engine;
    rF2Telemetry data;
    std::memset(&data, 0, sizeof(data));
    
    engine.m_road_texture_enabled = true;
    engine.m_road_texture_gain = 1.0;
    
    // Frame 1: Initial deflection
    data.mWheels[0].mVerticalTireDeflection = 0.01;
    data.mWheels[1].mVerticalTireDeflection = 0.01;
    data.mWheels[0].mTireLoad = 4000.0;
    data.mWheels[1].mTireLoad = 4000.0;
    
    double force1 = engine.calculate_force(&data);
    // First frame: delta = 0.01 - 0.0 = 0.01
    // Expected force = (0.01 + 0.01) * 5000 * 1.0 * 1.0 = 100
    // Normalized = 100 / 4000 = 0.025
    
    // Frame 2: Bump (sudden increase)
    data.mWheels[0].mVerticalTireDeflection = 0.02;
    data.mWheels[1].mVerticalTireDeflection = 0.02;
    
    double force2 = engine.calculate_force(&data);
    // Delta = 0.02 - 0.01 = 0.01
    // Force should be same as frame 1
    
    ASSERT_NEAR(force2, force1, 0.001);
    
    // Frame 3: No change (flat road)
    double force3 = engine.calculate_force(&data);
    // Delta = 0.0, force should be near zero
    if (std::abs(force3) < 0.01) {
        std::cout << "[PASS] Road texture state preserved correctly." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Road texture state issue" << std::endl;
        g_tests_failed++;
    }
}

void test_multi_effect_interaction() {
    std::cout << "\nTest: Multi-Effect Interaction (Lockup + Spin)" << std::endl;
    FFBEngine engine;
    rF2Telemetry data;
    std::memset(&data, 0, sizeof(data));
    
    // Enable both lockup and spin
    engine.m_lockup_enabled = true;
    engine.m_lockup_gain = 1.0;
    engine.m_spin_enabled = true;
    engine.m_spin_gain = 1.0;
    
    // Scenario: Braking AND spinning (e.g., locked front, spinning rear)
    data.mUnfilteredBrake = 1.0;
    data.mUnfilteredThrottle = 0.5; // Partial throttle
    data.mWheels[0].mSlipRatio = -0.3; // Front locked
    data.mWheels[1].mSlipRatio = -0.3;
    data.mWheels[2].mSlipRatio = 0.5;  // Rear spinning
    data.mWheels[3].mSlipRatio = 0.5;
    data.mLocalVel.z = 20.0;
    data.mDeltaTime = 0.01;
    
    // Run multiple frames
    for (int i = 0; i < 10; i++) {
        engine.calculate_force(&data);
    }
    
    // Verify both phases advanced
    bool lockup_ok = engine.m_lockup_phase > 0.0;
    bool spin_ok = engine.m_spin_phase > 0.0;
    
    if (lockup_ok && spin_ok) {
         // Verify phases are different (independent oscillators)
        if (std::abs(engine.m_lockup_phase - engine.m_spin_phase) > 0.1) {
             std::cout << "[PASS] Multiple effects coexist without interference." << std::endl;
             g_tests_passed++;
        } else {
             std::cout << "[FAIL] Phases are identical?" << std::endl;
             g_tests_failed++;
        }
    } else {
        std::cout << "[FAIL] Effects did not trigger." << std::endl;
        g_tests_failed++;
    }
}

void test_load_factor_edge_cases() {
    std::cout << "\nTest: Load Factor Edge Cases" << std::endl;
    FFBEngine engine;
    rF2Telemetry data;
    std::memset(&data, 0, sizeof(data));
    
    engine.m_slide_texture_enabled = true;
    engine.m_slide_texture_gain = 1.0;
    
    // Setup slide condition
    data.mWheels[0].mSlipAngle = 0.2;
    data.mWheels[1].mSlipAngle = 0.2;
    data.mWheels[0].mLateralPatchVel = 5.0;
    data.mWheels[1].mLateralPatchVel = 5.0;
    data.mDeltaTime = 0.01;
    
    // Case 1: Zero load (airborne)
    data.mWheels[0].mTireLoad = 0.0;
    data.mWheels[1].mTireLoad = 0.0;
    
    double force_airborne = engine.calculate_force(&data);
    // Load factor = 0, slide texture should be silent
    ASSERT_NEAR(force_airborne, 0.0, 0.001);
    
    // Case 2: Extreme load (20000N)
    data.mWheels[0].mTireLoad = 20000.0;
    data.mWheels[1].mTireLoad = 20000.0;
    
    engine.calculate_force(&data); // Advance phase
    double force_extreme = engine.calculate_force(&data);
    
    // Load factor should be clamped at 1.5
    // Max expected: sawtooth * 300 * 1.5 = 450
    // Normalized: 450 / 4000 = 0.1125
    if (std::abs(force_extreme) < 0.15) {
        std::cout << "[PASS] Load factor clamped correctly." << std::endl;
        g_tests_passed++;
    } else {
         std::cout << "[FAIL] Load factor not clamped? Force: " << force_extreme << std::endl;
         g_tests_failed++;
    }
}

void test_spin_torque_drop_interaction() {
    std::cout << "\nTest: Spin Torque Drop with SoP" << std::endl;
    FFBEngine engine;
    rF2Telemetry data;
    std::memset(&data, 0, sizeof(data));
    
    engine.m_spin_enabled = true;
    engine.m_spin_gain = 1.0;
    engine.m_sop_effect = 1.0;
    
    // High SoP force
    data.mLocalAccel.x = 9.81; // 1G lateral
    data.mSteeringArmForce = 2000.0;
    
    // No spin initially
    data.mUnfilteredThrottle = 0.0;
    double force_no_spin = engine.calculate_force(&data);
    
    // Now trigger spin
    data.mUnfilteredThrottle = 1.0;
    data.mWheels[2].mSlipRatio = 0.7; // 70% slip (severe = 1.0)
    data.mWheels[3].mSlipRatio = 0.7;
    data.mLocalVel.z = 20.0;
    data.mDeltaTime = 0.01;
    
    double force_with_spin = engine.calculate_force(&data);
    
    // Torque drop: 1.0 - (1.0 * 1.0 * 0.6) = 0.4 (60% reduction)
    // Force should be significantly lower
    // Note: Spin also adds a rumble, which might increase the force slightly.
    // Base 0.25 -> Dropped 0.10. Rumble adds +/- 0.125.
    // So force could be up to 0.225 or down to -0.025.
    // We check that it is NOT equal to original (0.25) and somewhat reduced/modified.
    if (std::abs(force_with_spin - force_no_spin) > 0.05) {
        std::cout << "[PASS] Spin torque drop modifies total force." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Torque drop ineffective. Spin: " << force_with_spin << " NoSpin: " << force_no_spin << std::endl;
        g_tests_failed++;
    }
}

int main() {
    test_zero_input();
    test_suspension_bottoming();
    test_grip_modulation();
    test_sop_effect();
    test_min_force();
    test_progressive_lockup();
    test_slide_texture();
    test_dynamic_tuning();
    
    test_oversteer_boost();
    test_phase_wraparound();
    test_road_texture_state_persistence();
    test_multi_effect_interaction();
    test_load_factor_edge_cases();
    test_spin_torque_drop_interaction();

    std::cout << "\n----------------" << std::endl;
    std::cout << "Tests Passed: " << g_tests_passed << std::endl;
    std::cout << "Tests Failed: " << g_tests_failed << std::endl;
    
    return g_tests_failed > 0 ? 1 : 0;
}
