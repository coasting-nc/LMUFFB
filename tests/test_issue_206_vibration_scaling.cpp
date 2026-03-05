#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_issue_206_vibration_scaling, "Functional") {
    FFBEngine engine;
    InitializeEngine(engine);

    // 1. Setup baseline: All vibration effects enabled
    engine.m_road_texture_enabled = true;
    engine.m_road_texture_gain = 1.0f;
    engine.m_slide_texture_enabled = true;
    engine.m_slide_texture_gain = 1.0f;
    engine.m_lockup_enabled = true;
    engine.m_lockup_gain = 1.0f;
    engine.m_abs_pulse_enabled = true;
    engine.m_abs_gain = 1.0f;
    engine.m_spin_enabled = true;
    engine.m_spin_gain = 1.0f;
    engine.m_bottoming_enabled = true;
    engine.m_bottoming_gain = 1.0f;
    engine.m_soft_lock_enabled = true;

    // Trigger soft lock condition (steering beyond limit)
    TelemInfoV01 tel = CreateBasicTestTelemetry(50.0);
    tel.mUnfilteredSteering = 1.01f; // 1% excess
    tel.mPhysicalSteeringWheelRange = 9.4247f; // ~540 deg

    // Trigger Road Texture (delta deflection)
    tel.mWheel[0].mVerticalTireDeflection = 0.01f;

    // Trigger Lockup (slip > threshold)
    tel.mWheel[0].mRotation = 0.0f;
    tel.mWheel[1].mRotation = 0.0f;

    // Trigger ABS Pulse (high pedal + high pressure rate)
    tel.mUnfilteredBrake = 1.0f;
    tel.mWheel[0].mBrakePressure = 10.0f; // High delta from 0.0

    // Trigger Slide (patch vel)
    tel.mWheel[0].mLateralPatchVel = 5.0f;

    // Trigger Bottoming (ride height)
    tel.mWheel[0].mRideHeight = 0.001f; // < 2mm

    // Case 1: Vibration Gain = 1.0 (Baseline)
    engine.m_vibration_gain = 1.0f;
    double force_100 = engine.calculate_force(&tel);

    // Case 2: Vibration Gain = 0.5 (Half)
    engine.m_vibration_gain = 0.5f;
    double force_50 = engine.calculate_force(&tel);

    // Case 3: Vibration Gain = 2.0 (Double)
    engine.m_vibration_gain = 2.0f;
    double force_200 = engine.calculate_force(&tel);

    // Case 4: Vibration Gain = 0.0 (Off)
    engine.m_vibration_gain = 0.0f;
    double force_0 = engine.calculate_force(&tel);

    // Increase wheelbase max Nm to avoid clipping for scaling verification
    engine.m_wheelbase_max_nm = 1000.0f;

    std::cout << "[INFO] Force (100%): " << force_100 << std::endl;
    std::cout << "[INFO] Force (50%): " << force_50 << std::endl;
    std::cout << "[INFO] Force (200%): " << force_200 << std::endl;
    std::cout << "[INFO] Force (0%): " << force_0 << std::endl;

    // Verify scaling: (Force - SoftLock) should scale.
    // However, it's easier to verify snapshots if available.
    std::vector<FFBSnapshot> batch = engine.GetDebugBatch();
    ASSERT_GE(batch.size(), 4u);

    // Use snapshots to verify individual components
    const auto& s100 = batch[0];
    const auto& s50 = batch[1];
    const auto& s200 = batch[2];
    const auto& s0 = batch[3];

    // Total vibration sum in NM should scale perfectly
    float vibration_100 = s100.texture_road + s100.texture_slide + s100.texture_lockup + s100.texture_spin + s100.texture_bottoming + s100.ffb_abs_pulse;
    float vibration_50 = s50.texture_road + s50.texture_slide + s50.texture_lockup + s50.texture_spin + s50.texture_bottoming + s50.ffb_abs_pulse;
    float vibration_200 = s200.texture_road + s200.texture_slide + s200.texture_lockup + s200.texture_spin + s200.texture_bottoming + s200.ffb_abs_pulse;
    float vibration_0 = s0.texture_road + s0.texture_slide + s0.texture_lockup + s0.texture_spin + s0.texture_bottoming + s0.ffb_abs_pulse;

    // NOTE: The textures themselves in the snapshot represent the internal absolute NM value
    // calculated in calculate_road_texture() etc.
    // BUT calculate_force() summates them and applies m_vibration_gain.
    // Wait, let me check FFBEngine.cpp logic again.

    /*
    double vibration_sum_nm = ctx.road_noise + ctx.slide_noise + ctx.spin_rumble + ctx.bottoming_crunch + ctx.abs_pulse_force + ctx.lockup_rumble;
    double final_texture_nm = (vibration_sum_nm * (double)m_vibration_gain) + ctx.soft_lock_force;
    double di_texture = final_texture_nm / wheelbase_max_safe;
    double norm_force = (di_structural + di_texture) * m_gain;
    */

    // The individual texture_road, texture_slide etc in FFBSnapshot are BEFORE scaling by m_vibration_gain.
    // They are raw absolute NM values.
    // To verify scaling, we must look at total output or introduce scaled values to snapshot.
    // Actually, I didn't change the snapshot members.

    // Let's verify via total output while holding structural constant.
    // We can zero out structural to make it easier.
    engine.m_steering_shaft_gain = 0.0f;
    engine.m_sop_effect = 0.0f;
    engine.m_oversteer_boost = 0.0f;
    engine.m_rear_align_effect = 0.0f;
    engine.m_sop_yaw_gain = 0.0f;
    engine.m_gyro_gain = 0.0f;
    engine.m_scrub_drag_gain = 0.0f;

    // Run again with zero structural
    engine.m_vibration_gain = 1.0f;
    double f100 = engine.calculate_force(&tel);
    engine.m_vibration_gain = 0.5f;
    double f50 = engine.calculate_force(&tel);
    engine.m_vibration_gain = 0.0f;
    double f0 = engine.calculate_force(&tel);

    batch = engine.GetDebugBatch();
    const auto& s_sl = batch.back();
    double sl_nm = (double)s_sl.ffb_soft_lock;
    double sl_di = sl_nm / (double)engine.m_wheelbase_max_nm;
    double sl_total = std::clamp(sl_di * (double)engine.m_gain, -1.0, 1.0);
    if (engine.m_invert_force) sl_total *= -1.0;

    std::cout << "[INFO] f100: " << f100 << ", f50: " << f50 << ", f0: " << f0 << ", sl_total: " << sl_total << std::endl;

    // f0 should be exactly the soft lock component (since other textures are multiplied by 0.0)
    ASSERT_NEAR(f0, sl_total, 0.001);

    // (f100 - f0) should be 2x (f50 - f0)
    double texture_part_100 = f100 - f0;
    double texture_part_50 = f50 - f0;
    ASSERT_NEAR(texture_part_100, texture_part_50 * 2.0, 0.001);

    // Verify soft lock is not zero if textures are zeroed
    ASSERT_GT(std::abs(f0), 0.0);
}

} // namespace FFBEngineTests
