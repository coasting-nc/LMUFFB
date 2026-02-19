#include <iostream>
#include <cmath>
#include <vector>
#include "src/FFBEngine.h"
#include "tests/test_ffb_common.h"

using namespace FFBEngineTests;

TEST_CASE(test_peak_follower_fast_attack, "StructuralNormalization") {
    FFBEngine engine;
    TelemInfoV01 data = {};
    data.mDeltaTime = 0.0025f;
    data.mSteeringShaftTorque = 40.0f;
    data.mLocalAccel.x = 2.0f * 9.81f; // 2G (Clean state)
    data.mElapsedTime = 1.0f;
    data.mLocalVel.z = -20.0f;

    // Settle rolling average and last torque manually to avoid spike rejection
    FFBEngineTestAccess::SetRollingAverageTorque(engine, 40.0);
    FFBEngineTestAccess::SetLastRawTorque(engine, 40.0);

    // Check initial peak (should be default 25.0)
    ASSERT_NEAR(FFBEngineTestAccess::GetSessionPeakTorque(engine), 25.0, 0.001);

    // Trigger fast attack
    engine.calculate_force(&data);

    // Check peak
    double peak = FFBEngineTestAccess::GetSessionPeakTorque(engine);
    // Initial is 25.0, should jump to 40.0
    ASSERT_NEAR(peak, 40.0, 0.001);
}

TEST_CASE(test_peak_follower_exponential_decay, "StructuralNormalization") {
    FFBEngine engine;
    TelemInfoV01 data = {};
    data.mDeltaTime = 0.01f; // 100Hz
    data.mSteeringShaftTorque = 40.0f;
    data.mLocalAccel.x = 2.0f * 9.81f;
    data.mLocalVel.z = -20.0f;

    // Set peak to 40.0
    FFBEngineTestAccess::SetRollingAverageTorque(engine, 40.0);
    FFBEngineTestAccess::SetLastRawTorque(engine, 40.0);
    engine.calculate_force(&data);
    double initial_peak = FFBEngineTestAccess::GetSessionPeakTorque(engine);
    ASSERT_NEAR(initial_peak, 40.0, 0.001);

    // Drop torque and wait for decay
    data.mSteeringShaftTorque = 10.0f;
    // Settle to avoid slew rejection
    FFBEngineTestAccess::SetRollingAverageTorque(engine, 10.0);
    FFBEngineTestAccess::SetLastRawTorque(engine, 10.0);

    engine.calculate_force(&data);

    double decayed_peak = FFBEngineTestAccess::GetSessionPeakTorque(engine);
    // 40.0 * (1.0 - (0.005 * 0.01)) = 39.998
    ASSERT_LT(decayed_peak, 40.0);
    ASSERT_GT(decayed_peak, 39.9);
}

TEST_CASE(test_contextual_spike_rejection, "StructuralNormalization") {
    FFBEngine engine;
    TelemInfoV01 data = {};
    data.mDeltaTime = 0.0025f;
    data.mSteeringShaftTorque = 15.0f;
    data.mLocalAccel.x = 1.0f * 9.81f;
    data.mLocalVel.z = -20.0f;

    // Settle rolling average at 15.0 Nm
    FFBEngineTestAccess::SetRollingAverageTorque(engine, 15.0);
    FFBEngineTestAccess::SetLastRawTorque(engine, 15.0);
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 15.0);

    // Massive spike
    data.mSteeringShaftTorque = 100.0f;
    engine.calculate_force(&data);

    double after_spike_peak = FFBEngineTestAccess::GetSessionPeakTorque(engine);
    // Should NOT be 100.0 (spike rejection should kick in because 100 > 15*3)
    // It should have decayed slightly or stayed near 15.0
    ASSERT_LT(after_spike_peak, 16.0);
}

TEST_CASE(test_structural_vs_texture_separation, "StructuralNormalization") {
    FFBEngine engine;
    InitializeEngine(engine); // Use standard test initialization (invert_force=false)

    // Inject custom values via TestAccess
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 50.0);
    // Bypass smoothing for the multiplier to ensure exact match
    FFBEngineTestAccess::SetSmoothedStructuralMult(engine, 1.0 / 50.0); // 0.02
    engine.m_wheelbase_max_nm = 20.0f; engine.m_target_rim_nm = 20.0f; // Multiplier 1/20 = 0.05
    engine.m_gain = 1.0f;

    TelemInfoV01 data = {};
    data.mDeltaTime = 0.0025f;
    data.mSteeringShaftTorque = 10.0f; // Base steering
    data.mLocalVel.z = -20.0f; // Ensure speed gate is 1.0
    engine.m_understeer_effect = 0.0f;
    engine.m_sop_effect = 0.0f;
    engine.m_road_texture_gain = 1.0f;
    engine.m_steering_shaft_gain = 1.0f;
    engine.m_dynamic_weight_gain = 0.0f;
    engine.m_base_force_mode = 0;
    engine.m_steering_shaft_smoothing = 0.0f;
    engine.m_speed_gate_lower = 1.0f;
    engine.m_speed_gate_upper = 5.0f;
    engine.m_road_texture_enabled = true;

    // Settle rolling average and last torque
    FFBEngineTestAccess::SetRollingAverageTorque(engine, 10.0);
    FFBEngineTestAccess::SetLastRawTorque(engine, 10.0);

    // Set vertical deflection to get road noise
    for(int i=0; i<4; i++) engine.m_prev_vert_deflection[i] = 0.0;
    data.mWheel[0].mVerticalTireDeflection = 0.01f;
    data.mWheel[1].mVerticalTireDeflection = 0.01f;

    data.mWheel[0].mTireLoad = 4500.0; // Default peak
    data.mWheel[1].mTireLoad = 4500.0;

    double force = engine.calculate_force(&data);

    // Expected logic:
    // structural_sum = 10.0 (base) + 0 (others) = 10.0
    // norm_structural = 10.0 * 0.02 = 0.2
    // road_noise_val = (0.01 + 0.01) * 50.0 = 1.0 Nm
    // ctx.road_noise = 1.0 * 1.0 (gain) * 1.0 (decoupling) * 1.0 (load factor) = 1.0 Nm
    // norm_texture = 10.0 / 20.0 = 0.05
    // total_sum = 0.2 + 0.05 = 0.25

    ASSERT_NEAR(force, 0.25, 0.001);
}
