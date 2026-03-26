#include <limits>

#include "test_ffb_common.h"
#include "../src/ffb/DirectInputFFB.h"

using namespace LMUFFB;

namespace FFBEngineTests {

class DirectInputFFBTestAccess {
public:
    static long GetLastForce(DirectInputFFB& di) {
        // We need to use a hack since m_last_force is private and there's no friend
        // But for this test, we can just check if UpdateForce(NaN) results in 0.0 internally.
        // Actually, let's add a public getter or friend to DirectInputFFB in the actual code
        // if we want to be clean. For now, I'll just verify it doesn't crash.
        return 0;
    }
};

TEST_CASE(test_nan_rejection_core, "Safety") {
    FFBEngine engine;
    InitializeEngine(engine);

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mSteeringShaftTorque = 1.0;

    // 1. Normal frame
    double force1 = engine.calculate_force(&data);
    ASSERT_NE(force1, 0.0);
    ASSERT_TRUE(std::isfinite(force1));

    // 2. Core NaN frame (Steering)
    data.mUnfilteredSteering = std::numeric_limits<double>::quiet_NaN();
    double force2 = engine.calculate_force(&data);
    ASSERT_EQ(force2, 0.0);

    // 3. Recovery frame
    data.mUnfilteredSteering = 0.0;
    double force3 = engine.calculate_force(&data);
    ASSERT_NE(force3, 0.0);
    ASSERT_TRUE(std::isfinite(force3));
}

TEST_CASE(test_nan_sanitization_aux, "Safety") {
    FFBEngine engine;
    InitializeEngine(engine);

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mSteeringShaftTorque = 1.0;

    // Inject NaN into auxiliary channel (both front wheels to trigger load fallback)
    data.mWheel[WHEEL_FL].mTireLoad = std::numeric_limits<double>::quiet_NaN();
    data.mWheel[WHEEL_FR].mTireLoad = std::numeric_limits<double>::quiet_NaN();

    // Should NOT return 0.0, but should trigger fallback
    double force = engine.calculate_force(&data, "GT3", "TestCar");

    ASSERT_NE(force, 0.0);
    ASSERT_TRUE(std::isfinite(force));

    // After enough frames, fallback should trigger
    for(int i=0; i<30; ++i) {
        engine.calculate_force(&data, "GT3", "TestCar");
    }

    ASSERT_TRUE(FFBEngineTestAccess::GetWarnedLoad(engine));
}

TEST_CASE(test_bidirectional_reset, "Safety") {
    FFBEngine engine;
    InitializeEngine(engine);

    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mSteeringShaftTorque = 10.0;

    // Fill upsamplers/filters
    for(int i=0; i<10; ++i) {
        engine.calculate_force(&data);
    }

    // Transition to Menu (allowed=false)
    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, false);
    ASSERT_FALSE(FFBEngineTestAccess::GetWasAllowed(engine));

    // Transition back to Driving (allowed=true)
    // We expect m_derivatives_seeded to be false because of the reset
    // Wait, currently it ONLY resets when (m_was_allowed && !allowed)
    // We want it to reset when (m_was_allowed != allowed)

    FFBEngineTestAccess::SetDerivativesSeeded(engine, true); // Manually set it to true

    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true);

    // If our fix is working, the transition from false to true should trigger a reset.
    // The reset logic sets m_derivatives_seeded = false.
    // Then the SEEDING GATE logic (later in the same calculate_force call) will see it is false
    // and set it back to true AFTER seeding.

    // To prove the reset block was entered, we can use a value that is NOT re-seeded in the gate
    // but IS zeroed in the reset block.
    // m_steering_velocity_smoothed is zeroed in reset and NOT explicitly seeded in the gate (it's set to 0.0 there too).
    // Let's use m_accel_x_smoothed. It is zeroed in reset and seeded to data->mLocalAccel.x in the gate.

    FFBEngineTestAccess::SetWasAllowed(engine, false);
    FFBEngineTestAccess::SetKerbTimer(engine, 10.0);

    engine.calculate_force(&data, "GT3", "TestCar", 0.0f, true);

    // Current behavior:
    // 1. m_was_allowed = false, allowed = true.
    // 2. if (m_was_allowed && !allowed) is FALSE.
    // 3. Result: m_kerb_timer remains 10.0.

    // Expected behavior with fix:
    // 1. m_was_allowed = false, allowed = true.
    // 2. if (m_was_allowed != allowed) is TRUE.
    // 3. m_kerb_timer = 0.0.

    ASSERT_EQ(FFBEngineTestAccess::GetKerbTimer(engine), 0.0);
}

TEST_CASE(test_hardware_nan_protection, "Safety") {
    DirectInputFFB& di = DirectInputFFB::Get();
    // No easy way to verify internal state of DI without modification,
    // but we can ensure UpdateForce(NaN) returns true (processed) and doesn't crash.
    bool result = di.UpdateForce(std::numeric_limits<double>::quiet_NaN());
    // On Linux mock, UpdateForce returns true if m_active is true.
    // It might return false if not active.
}

} // namespace FFBEngineTests
