#include "test_ffb_common.h"

namespace FFBEngineTests {

// 1. Verify that high aerodynamic and cornering loads do NOT trigger the bottoming safety net.
// The old threshold was 2.5x static load, which was easily exceeded at Spa/Eau Rouge.
// The new threshold is 4.0x static load.
TEST_CASE(test_bottoming_high_aero_rejection, "Texture") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Enable bottoming and set a known static load
    FFBEngineTestAccess::SetBottomingEnabled(engine, true);
    FFBEngineTestAccess::SetBottomingGain(engine, 1.0f);
    FFBEngineTestAccess::SetBottomingMethod(engine, 1); // Ensure we are NOT in Method 0 (Ride Height)
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 4000.0);

    // Simulate high speed (70 m/s ~ 250 km/h)
    TelemInfoV01 data = CreateBasicTestTelemetry(70.0, 0.0);
    PumpEngineTime(engine, data, 0.5); // Settle filters

    // Simulate heavy aero + cornering load transfer (3.5x static load = 14000N)
    // This would have falsely triggered the old 2.5x threshold.
    data.mWheel[0].mTireLoad = 14000.0;
    data.mWheel[1].mTireLoad = 14000.0;
    data.mWheel[2].mTireLoad = 14000.0;
    data.mWheel[3].mTireLoad = 14000.0;

    // Explicitly set static load to bypass learning phase
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 4000.0);
    FFBEngineTestAccess::SetStaticLoadLatched(engine, true);

    // Enable bottoming
    FFBEngineTestAccess::SetBottomingEnabled(engine, true);
    FFBEngineTestAccess::SetBottomingMethod(engine, 0); // Method 0: Ride Height
    // isolating our test to the Safety Trigger (Raw Load Peak).
    data.mWheel[0].mSuspForce = 4000.0;
    data.mWheel[1].mSuspForce = 4000.0;

    // Call multiple times to advance sine phase
    PumpEngineTime(engine, data, 0.1);

    auto snaps = engine.GetDebugBatch();
    ASSERT_TRUE(!snaps.empty());

    // Expectation for fix: Bottoming crunch should be exactly 0.0 (No false trigger)
    // Current state (pre-fix): it will pass this assertion if the current threshold 2.5 is enough to reject 3.5.
    // Wait, 14000 / 4000 = 3.5. 3.5 > 2.5. So it SHOULD trigger in current code.
    bool triggered_any = false;
    for (const auto& snap : snaps) {
        if (std::abs(snap.texture_bottoming) > 0.0001f) {
            triggered_any = true;
            break;
        }
    }
    // We want this to FAIL now (it should be triggered in current code)
    // and PASS after the fix (it should not be triggered).
    // Expectation: it should NOT be triggered after the fix.
    ASSERT_FALSE(triggered_any);
}

// 2. Verify that a genuine massive impact still triggers the safety net.
TEST_CASE(test_bottoming_genuine_impact, "Texture") {
    FFBEngine engine;
    InitializeEngine(engine);

    FFBEngineTestAccess::SetBottomingEnabled(engine, true);
    FFBEngineTestAccess::SetBottomingGain(engine, 1.0f);
    FFBEngineTestAccess::SetBottomingMethod(engine, 0); // Method 0: Ride Height
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 4000.0);

    TelemInfoV01 data = CreateBasicTestTelemetry(70.0, 0.0);
    PumpEngineTime(engine, data, 0.5); // Settle filters

    // Simulate a massive curb strike / bottoming out
    data.mWheel[0].mRideHeight = 0.0001; // 0.1mm - should trigger Method 0
    data.mWheel[1].mRideHeight = 0.0001;

    // Call multiple times to advance sine phase
    PumpEngineTime(engine, data, 0.1);

    auto snaps = engine.GetDebugBatch();
    ASSERT_TRUE(!snaps.empty());

    // Expectation: Bottoming crunch should be active (non-zero in at least some frames)
    bool triggered_any = false;
    for (const auto& snap : snaps) {
        if (std::abs(snap.texture_bottoming) > 0.0001f) {
            triggered_any = true;
            break;
        }
    }
    ASSERT_TRUE(triggered_any);
}

// 3. Verify that the new UI controls (Enable Toggle and Gain Slider) work correctly.
TEST_CASE(test_bottoming_user_controls, "Texture") {
    FFBEngine engine;
    InitializeEngine(engine);

    engine.m_vibration.bottoming_method = 1;
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 4000.0);

    TelemInfoV01 data = CreateBasicTestTelemetry(70.0, 0.0);
    PumpEngineTime(engine, data, 0.5); // Settle filters

    // --- Test A: Disabled ---
    FFBEngineTestAccess::SetBottomingEnabled(engine, false);
    data.mWheel[0].mTireLoad = 18000.0; // Massive impact
    PumpEngineTime(engine, data, 0.1);

    auto snaps = engine.GetDebugBatch();
    bool triggered_any = false;
    for (const auto& snap : snaps) {
        if (std::abs(snap.texture_bottoming) > 0.0001f) triggered_any = true;
    }
    ASSERT_FALSE(triggered_any); // Should be muted

    // --- Test B: Enabled, Gain = 1.0 ---
    FFBEngineTestAccess::SetBottomingEnabled(engine, true);
    FFBEngineTestAccess::SetBottomingGain(engine, 1.0f);
    PumpEngineTime(engine, data, 0.1);

    snaps = engine.GetDebugBatch();
    float max_force_gain_1 = 0.0f;
    for (const auto& snap : snaps) {
        float f = std::abs(snap.texture_bottoming);
        if (f > max_force_gain_1) max_force_gain_1 = f;
    }
    ASSERT_GT(max_force_gain_1, 0.0f); // Should be active

    // --- Test C: Enabled, Gain = 0.5 ---
    FFBEngineTestAccess::SetBottomingGain(engine, 0.5f);
    // We want to compare magnitude. Since phase is continuous, we should run for a few frames
    // and check the peak or just check a single frame if we're careful.
    // Let's check peak over a short window.
    PumpEngineTime(engine, data, 0.1);
    snaps = engine.GetDebugBatch();
    float max_force_gain_half = 0.0f;
    for (const auto& snap : snaps) {
        float f = std::abs(snap.texture_bottoming);
        if (f > max_force_gain_half) max_force_gain_half = f;
    }

    ASSERT_GT(max_force_gain_half, 0.0f);
    // 0.5 gain should result in half the peak amplitude
    ASSERT_NEAR(max_force_gain_half, max_force_gain_1 * 0.5f, 0.01f);
}

} // namespace FFBEngineTests
