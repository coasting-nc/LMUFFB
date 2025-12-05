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

void test_slide_texture() {
    std::cout << "\nTest: Slide Texture" << std::endl;
    FFBEngine engine;
    rF2Telemetry data;
    std::memset(&data, 0, sizeof(data));
    
    engine.m_slide_texture_enabled = true;
    engine.m_slide_texture_gain = 1.0;
    
    data.mSteeringArmForce = 0.0;
    data.mWheels[0].mSlipAngle = 0.2; // Slipping > 0.1
    data.mWheels[1].mSlipAngle = 0.2;
    data.mElapsedTime = 1.0; // Phase shift for sin wave
    
    // sin(500.0) * 1.0 * 200.0
    // 500 radians = 79.577... cycles. 
    // sin(500) = -0.467...
    // noise = -0.467 * 200 = -93.5
    // norm = -93.5 / 4000 = -0.023
    
    double force = engine.calculate_force(&data);
    
    // We just assert it's non-zero
    if (std::abs(force) > 0.001) {
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

int main() {
    test_zero_input();
    test_grip_modulation();
    test_sop_effect();
    test_min_force();
    test_slide_texture();
    test_dynamic_tuning();

    std::cout << "\n----------------" << std::endl;
    std::cout << "Tests Passed: " << g_tests_passed << std::endl;
    std::cout << "Tests Failed: " << g_tests_failed << std::endl;
    
    return g_tests_failed > 0 ? 1 : 0;
}
