#include "test_ffb_common.h"

using namespace FFBEngineTests;

TEST_CASE(test_static_load_latching, "Normalization") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Ensure initial state
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 0.0);
    FFBEngineTestAccess::SetStaticLoadLatched(engine, false);
    FFBEngineTestAccess::SetAutoPeakLoad(engine, 8000.0);

    // 1. Valid learning speed (10 m/s)
    FFBEngineTestAccess::CallUpdateStaticLoadReference(engine, 4000.0, 10.0, 0.0025);
    ASSERT_GT(FFBEngineTestAccess::GetStaticFrontLoad(engine), 0.0);
    ASSERT_FALSE(FFBEngineTestAccess::GetStaticLoadLatched(engine));

    // Seed it to a known value for easier observation of latching
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 4000.0);

    // 2. High speed (> 15 m/s) should latch
    FFBEngineTestAccess::CallUpdateStaticLoadReference(engine, 8000.0, 20.0, 0.0025);
    ASSERT_TRUE(FFBEngineTestAccess::GetStaticLoadLatched(engine));

    double latched_val = FFBEngineTestAccess::GetStaticFrontLoad(engine);

    // 3. Subsequent calls (even at valid speeds) should be ignored
    FFBEngineTestAccess::CallUpdateStaticLoadReference(engine, 2000.0, 10.0, 0.0025);
    ASSERT_EQ(FFBEngineTestAccess::GetStaticFrontLoad(engine), latched_val);
}

TEST_CASE(test_soft_knee_linear_region, "Normalization") {
    FFBEngine engine;
    InitializeEngine(engine);

    FFBEngineTestAccess::SetStaticFrontLoad(engine, 4000.0);
    FFBEngineTestAccess::SetSmoothedTactileMult(engine, 1.0);

    TelemInfoV01 data = CreateBasicTestTelemetry();
    data.mDeltaTime = 0.0025;
    data.mWheel[0].mTireLoad = 4000.0;
    data.mWheel[1].mTireLoad = 4000.0; // avg_load = 4000

    // Ratio x = 1.0. T=1.5, W=0.5 -> lower_bound = 1.25.
    // x < lower_bound, so compressed_load_factor = 1.0.

    // Run many frames to settle EMA
    for(int i=0; i<200; i++) {
        engine.calculate_force(&data);
    }

    ASSERT_NEAR(FFBEngineTestAccess::GetSmoothedTactileMult(engine), 1.0, 0.01);
}

TEST_CASE(test_soft_knee_compression_region, "Normalization") {
    FFBEngine engine;
    InitializeEngine(engine);

    FFBEngineTestAccess::SetStaticFrontLoad(engine, 4000.0);
    FFBEngineTestAccess::SetSmoothedTactileMult(engine, 1.0);

    TelemInfoV01 data = CreateBasicTestTelemetry();
    data.mDeltaTime = 0.0025;
    data.mWheel[0].mTireLoad = 10000.0;
    data.mWheel[1].mTireLoad = 10000.0; // avg_load = 10000

    // Ratio x = 2.5.
    // T = 1.5, R = 4.0, upper_bound = 1.75.
    // x > upper_bound, so compressed_load_factor = T + (x - T)/R = 1.5 + (2.5 - 1.5)/4.0 = 1.75.

    for(int i=0; i<400; i++) {
        engine.calculate_force(&data);
    }

    ASSERT_NEAR(FFBEngineTestAccess::GetSmoothedTactileMult(engine), 1.75, 0.01);
}

TEST_CASE(test_soft_knee_transition_region, "Normalization") {
    FFBEngine engine;
    InitializeEngine(engine);

    FFBEngineTestAccess::SetStaticFrontLoad(engine, 4000.0);
    FFBEngineTestAccess::SetSmoothedTactileMult(engine, 1.0);

    TelemInfoV01 data = CreateBasicTestTelemetry();
    data.mDeltaTime = 0.0025;
    data.mWheel[0].mTireLoad = 6000.0;
    data.mWheel[1].mTireLoad = 6000.0; // avg_load = 6000

    // Ratio x = 1.5.
    // T = 1.5, W = 0.5, R = 4.0.
    // lower_bound = 1.25, upper_bound = 1.75.
    // lower_bound < x < upper_bound.
    // diff = x - lower_bound = 1.5 - 1.25 = 0.25.
    // factor = x + (((1.0/R) - 1.0) * (diff*diff)) / (2.0 * W)
    // factor = 1.5 + ((0.25 - 1.0) * (0.0625)) / (1.0)
    // factor = 1.5 + (-0.75 * 0.0625) = 1.5 - 0.046875 = 1.453125.

    for(int i=0; i<400; i++) {
        engine.calculate_force(&data);
    }

    ASSERT_NEAR(FFBEngineTestAccess::GetSmoothedTactileMult(engine), 1.453125, 0.01);
}
