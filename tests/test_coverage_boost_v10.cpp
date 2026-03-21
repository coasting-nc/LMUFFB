#include "test_ffb_common.h"
#include "../src/io/RestApiProvider.h"
#include "../src/utils/StringUtils.h"

namespace FFBEngineTests {

TEST_CASE(test_ffb_engine_rest_api_fallback, "Safety") {
    FFBEngine engine;
    InitializeEngine(engine);
    
    // 1. Enable REST API and setup invalid physical range
    FFBEngineTestAccess::SetRestApiEnabled(engine, true);
    FFBEngineTestAccess::SetRestApiPort(engine, 8080);
    
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    data.mPhysicalSteeringWheelRange = 0.0f; // Invalid range triggers fallback logic
    
    // First, test with an invalid fallback from REST API (<= 0.0)
    RestApiProviderTestAccess::SetFallbackRange(0.0f);
    
    // Call calculate_force to hit lines 417, 858-860
    engine.calculate_force(&data, "GT3", "911");
    
    // Now, test with a valid fallback from REST API (> 0.0)
    RestApiProviderTestAccess::SetFallbackRange(540.0f);
    
    // Call calculate_force again
    engine.calculate_force(&data, "GT3", "911");
}

TEST_CASE(test_ffb_engine_extreme_edge_cases, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    
    // 1. Wheelbase max NM < 1.0 (Line 608)
    engine.m_general.wheelbase_max_nm = 0.5f;
    
    // 2. Wheel radius < RADIUS_FALLBACK_MIN_M (Line 188 context)
    // Note: circumference > 0 is always true because fallback is 0.33, 
    // but this exercises the defensive logic.
    data.mWheel[0].mStaticUndeflectedRadius = 0.05f * 100.0f; // 5cm in cm
    
    engine.calculate_force(&data, "GT3", "911");
}

TEST_CASE(test_ffb_engine_dt_and_slip_window_minimums, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    FFBCalculationContext ctx;
    ctx.dt = 0.0; // Force dt <= 1e-6
    ctx.car_speed = 20.0;
    
    // 1. DeltaTime checks in various helpers (Lines 1245, 1258, 1363)
    // - calculate_sop_lateral (yaw_accel, yaw_jerk use dt)
    FFBEngineTestAccess::CallCalculateSopLateral(engine, &data, ctx);
    
    // - update_static_load_reference (if we had access, but we can call it)
    FFBEngineTestAccess::CallUpdateStaticLoadReference(engine, 1000.0, 1000.0, 20.0, 0.0);
    
    // 2. Slip Window Minimums (Line 1460)
    // Trigger lockup calculation with a very small window
    FFBEngineTestAccess::SetLockupEnabled(engine, true);
    engine.m_braking.lockup_full_pct = 15.0f;
    engine.m_braking.lockup_start_pct = 14.999f; // Window = 0.00001 < 0.01 (MIN_SLIP_WINDOW)
    
    data.mWheel[0].mBrakePressure = 1.0;
    data.mWheel[0].mLongitudinalPatchVel = 10.0; // Force slip
    data.mWheel[0].mRotation = 50.0;
    
    FFBEngineTestAccess::CallCalculateLockup_Vibration(engine, &data, ctx);
}

TEST_CASE(test_ffb_engine_vehicle_name_change, "Logic") {
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0, 0.0);
    
    // Initial call
    engine.calculate_force(&data, "GT3", "VehicleA");
    
    // Frame 2: Change vehicle name to trigger StringUtils::SafeCopy
    engine.calculate_force(&data, "GT3", "VehicleB");
}

} // namespace FFBEngineTests
