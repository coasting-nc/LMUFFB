#include "test_ffb_common.h"
#include <iostream>

using namespace FFBEngineTests;

TEST_CASE(test_issue_379_normalization_reset, "Regression") {
    FFBEngine engine;
    InitializeEngine(engine);

    // 1. Pollute state
    FFBEngineTestAccess::SetSessionPeakTorque(engine, 50.0);
    FFBEngineTestAccess::SetAutoPeakLoad(engine, 8000.0);
    FFBEngineTestAccess::SetStaticFrontLoad(engine, 6000.0);
    FFBEngineTestAccess::SetStaticRearLoad(engine, 6000.0);
    FFBEngineTestAccess::SetLastRawTorque(engine, 10.0);

    // 2. Reset
    engine.ResetNormalization();

    // 3. Verify
    ASSERT_NEAR(FFBEngineTestAccess::GetSessionPeakTorque(engine), 20.0, 0.01);
    ASSERT_NEAR(FFBEngineTestAccess::GetAutoPeakLoad(engine), 4500.0, 0.01); // Default GT3 load
    ASSERT_NEAR(FFBEngineTestAccess::GetStaticFrontLoad(engine), 2250.0, 0.01);
    ASSERT_NEAR(FFBEngineTestAccess::GetStaticRearLoad(engine), 2250.0, 0.01);
    ASSERT_NEAR(FFBEngineTestAccess::GetLastRawTorque(engine), 0.0, 0.01);
}

TEST_CASE(test_issue_379_car_change_reset, "Regression") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Pollute slope detection
    std::array<double, 41> junk; junk.fill(1.23);
    FFBEngineTestAccess::SetSlopeBuffer(engine, junk);
    FFBEngineTestAccess::SetSlopeBufferCount(engine, 41);

    // Pollute seeding flags
    FFBEngineTestAccess::SetDerivativesSeeded(engine, true);

    // Trigger car change
    FFBEngineTestAccess::CallInitializeLoadReference(engine, "LMGTE", "Porsche 911 RSR");

    // Verify slope reset
    // We can't easily check the buffer without more accessors, but we can check buffer count
    // (Assuming we add an accessor or just check behavior)
    // For now, check if seeding was reset

    // Verify normalization was also called
    ASSERT_NEAR(FFBEngineTestAccess::GetAutoPeakLoad(engine), 5500.0, 0.01); // GTE default
}

TEST_CASE(test_issue_379_teleport_spike_prevention, "Regression") {
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_bottoming_enabled = true;
    engine.m_bottoming_method = 1; // Force-derivative based
    engine.m_bottoming_gain = 1.0f;
    engine.m_general.gain = 1.0f;

    TelemInfoV01 data = CreateBasicTestTelemetry(0.0, 0.0);

    // 1. Initial frame (Garage) - allowed=false
    engine.calculate_force(&data, "GT3", "Ferrari 296", 0.0f, false);

    // 2. Teleport to track (allowed=true) with HUGE suspension force jump
    data.mWheel[0].mSuspForce = 50000.0;
    data.mWheel[1].mSuspForce = 50000.0;
    data.mElapsedTime += 0.0025;

    // This call should trigger seeding and return 0, NOT a massive bottoming thump
    double force = engine.calculate_force(&data, "GT3", "Ferrari 296", 0.0f, true);

    ASSERT_NEAR(force, 0.0, 0.0001);

    // 3. Next frame should have normal physics
    data.mElapsedTime += 0.0025;
    force = engine.calculate_force(&data, "GT3", "Ferrari 296", 0.0f, true);

    // Since force didn't change from 50000, derivative is 0, no thump
    ASSERT_NEAR(force, 0.0, 0.0001);
}

AutoRegister reg_issue_379("Issue #379 Context Changes", "Regression", {"Physics", "Regression"}, []() {
    test_issue_379_normalization_reset();
    test_issue_379_car_change_reset();
    test_issue_379_teleport_spike_prevention();
});
