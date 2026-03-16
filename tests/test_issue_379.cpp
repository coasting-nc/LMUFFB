#include "test_ffb_common.h"
#include "ffb/FFBEngine.h"
#include "physics/VehicleUtils.h"
#include "io/RestApiProvider.h"
#include <mutex>

extern std::recursive_mutex g_engine_mutex;

namespace FFBEngineTests {

class FFBEngineTestAccess379 {
public:
    static void SetSlopeBufferCount(FFBEngine& e, int count) { e.m_slope_buffer_count = count; }
    static int GetSlopeBufferCount(const FFBEngine& e) { return e.m_slope_buffer_count; }
    static void SetLastRawTorque(FFBEngine& e, double val) { e.m_last_raw_torque = val; }
    static double GetLastRawTorque(const FFBEngine& e) { return e.m_last_raw_torque; }
    static void SetStaticRearLoad(FFBEngine& e, double val) { e.m_static_rear_load = val; }
    static double GetStaticRearLoad(const FFBEngine& e) { return e.m_static_rear_load; }
    static bool GetDerivativesSeeded(const FFBEngine& e) { return e.m_derivatives_seeded; }
    static double GetPrevVertDeflection(const FFBEngine& e, int idx) { return e.m_prev_vert_deflection[idx]; }
    static void InitializeLoadReference(FFBEngine& e, const char* cls, const char* name) { e.InitializeLoadReference(cls, name); }
};

TEST_CASE(test_issue_379_resets, "Regression") {
    FFBEngine engine;
    TelemInfoV01 data = {};
    data.mDeltaTime = 0.0025f;
    data.mElapsedTime = 1.0;
    data.mWheel[0].mStaticUndeflectedRadius = 33;
    data.mPhysicalSteeringWheelRange = 9.4247f; // 540 deg

    // 1. Simulate some driving to populate states
    engine.calculate_force(&data, "HYPERCAR", "VehicleA", 0.0f, true);

    // Manually mess with some states that should be reset
    {
        std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
        FFBEngineTestAccess379::SetSlopeBufferCount(engine, 10);
        FFBEngineTestAccess379::SetLastRawTorque(engine, 50.0);
        FFBEngineTestAccess379::SetStaticRearLoad(engine, 5000.0);
        RestApiProvider::Get().RequestSteeringRange(6397); // This sets m_isRequesting and maybe a fallback
    }

    // 2. Simulate Car Change
    FFBEngineTestAccess379::InitializeLoadReference(engine, "HYPERCAR", "FERRARI 499P");

    // Verify resets from Bug 1 & 4
    {
        std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
        ASSERT_EQ(FFBEngineTestAccess379::GetSlopeBufferCount(engine), 0);
        ASSERT_EQ(FFBEngineTestAccess379::GetLastRawTorque(engine), 0.0);

        double actual_rear_load = FFBEngineTestAccess379::GetStaticRearLoad(engine);
        // m_static_rear_load is reset to m_auto_peak_front_load * 0.5 if no saved load
        // Default HYPERCAR load is 9500N, so 4750N.
        ASSERT_NEAR(actual_rear_load, 4750.0, 1.0);
    }

    // Verify reset from Bug 2
    ASSERT_EQ(RestApiProvider::Get().GetFallbackRangeDeg(), 0.0f);

    // 3. Simulate Teleport (Allowed transition)
    // First, set m_was_allowed to true and allowed to false
    engine.calculate_force(&data, "HYPERCAR", "FERRARI 499P", 0.0f, false);
    ASSERT_FALSE(FFBEngineTestAccess379::GetDerivativesSeeded(engine));

    // Now teleport to track
    data.mElapsedTime += 0.0025;
    data.mWheel[0].mVerticalTireDeflection = 0.05f; // Jump from 0.0
    engine.calculate_force(&data, "HYPERCAR", "FERRARI 499P", 0.0f, true);
    ASSERT_TRUE(FFBEngineTestAccess379::GetDerivativesSeeded(engine));
    ASSERT_NEAR(FFBEngineTestAccess379::GetPrevVertDeflection(engine, 0), 0.05, 0.0001);
}

} // namespace FFBEngineTests
