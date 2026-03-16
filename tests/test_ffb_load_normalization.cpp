#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_class_seeding, "Physics") {
    std::cout << "\nTest: Load Normalization - Class Seeding (Enabled)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_auto_load_normalization_enabled = true; // Enable for testing seeding behavior

    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.01;
    data.mWheel[0].mRideHeight = 0.1;
    data.mWheel[1].mRideHeight = 0.1;
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;

    // 1. Test Default/Unknown (Now 4500)
    engine.calculate_force(&data, "UnknownClass", "UnknownCar");
    double peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 4500.0, 1.0);

    // 2. Test Hypercar (Case Insensitive)
    engine.calculate_force(&data, "hypercar", "Test");
    peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 9500.0, 1.0);

    // 3. Test GT3 (Case Insensitive)
    engine.calculate_force(&data, "gt3", "Test");
    peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 5000.0, 1.0);

    // 3b. Test LMGT3
    engine.calculate_force(&data, "lmgt3", "Test");
    peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 5000.0, 1.0);

    // 4. Test LMP2 (WEC) - Partial match (Issue #225: Now 7500N)
    engine.calculate_force(&data, "LMP2 2023", "Oreca 07");
    peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 7500.0, 1.0);

    // 5. Test LMP2 (ELMS) - Keyword match
    engine.calculate_force(&data, "LMP2", "Oreca 07 (derestricted)");
    peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 8500.0, 1.0);
}

TEST_CASE(test_fallback_seeding, "Physics") {
    std::cout << "\nTest: Load Normalization - Fallback Seeding (Enabled)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_auto_load_normalization_enabled = true;

    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.01;
    data.mWheel[0].mRideHeight = 0.1;
    data.mWheel[1].mRideHeight = 0.1;
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;

    // 1. Hypercar name fallback
    engine.calculate_force(&data, "Fallback_HC", "Ferrari 499P");
    double peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 9500.0, 1.0);

    // 2. LMP3 name fallback
    engine.calculate_force(&data, "Fallback_P3", "Ligier JS P320");
    peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 5800.0, 1.0);

    // 3. GTE name fallback
    engine.calculate_force(&data, "Fallback_GTE", "Porsche 911 RSR-19");
    peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 5500.0, 1.0);

    // 4. GT3 name fallback
    engine.calculate_force(&data, "Fallback_GT3", "BMW M4 GT3");
    peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 5000.0, 1.0);
}

TEST_CASE(test_peak_hold_adaptation, "Physics") {
    std::cout << "\nTest: Load Normalization - Peak Hold Adaptation (Enabled)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_auto_load_normalization_enabled = true;

    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.01;
    data.mWheel[0].mRideHeight = 0.1;
    data.mWheel[1].mRideHeight = 0.1;
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;

    // Seed as GT3 (4800N)
    engine.calculate_force(&data, "LMGT3");

    // Feed 6000N load
    data.mWheel[0].mTireLoad = 6000.0;
    data.mWheel[1].mTireLoad = 6000.0;

    engine.calculate_force(&data, "LMGT3");

    double peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 6000.0, 1.0);
}

TEST_CASE(test_peak_hold_adaptation_disabled, "Physics") {
    std::cout << "\nTest: Load Normalization - Peak Hold Adaptation (Disabled)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_auto_load_normalization_enabled = false;

    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.01;
    data.mWheel[0].mRideHeight = 0.1;
    data.mWheel[1].mRideHeight = 0.1;
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;

    // Seed as GT3 (4800N)
    engine.calculate_force(&data, "LMGT3");

    // Feed 6000N load
    data.mWheel[0].mTireLoad = 6000.0;
    data.mWheel[1].mTireLoad = 6000.0;

    engine.calculate_force(&data, "LMGT3");

    double peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    // Should stay at 5000.0
    ASSERT_NEAR(peak, 5000.0, 1.0);
}

TEST_CASE(test_peak_hold_decay, "Physics") {
    std::cout << "\nTest: Load Normalization - Peak Hold Decay (Enabled)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_auto_load_normalization_enabled = true;

    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.01;
    data.mWheel[0].mRideHeight = 0.1;
    data.mWheel[1].mRideHeight = 0.1;
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;

    // Set peak to 8000N
    engine.calculate_force(&data, "Hypercar"); // Seed high
    FFBEngineTestAccess::SetAutoPeakLoad(engine, 8000.0);

    // Feed 4000N load for 1 second (100 steps of 0.01s)
    data.mWheel[0].mTireLoad = 4000.0;
    data.mWheel[1].mTireLoad = 4000.0;

    for (int i = 0; i < 100; i++) {
        engine.calculate_force(&data, "Hypercar");
    }

    // Decay is ~100N/s. 8000 - 100 = 7900.
    double peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 7900.0, 5.0);
}

TEST_CASE(test_peak_hold_decay_disabled, "Physics") {
    std::cout << "\nTest: Load Normalization - Peak Hold Decay (Disabled)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_auto_load_normalization_enabled = false;

    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.01;
    data.mWheel[0].mRideHeight = 0.1;
    data.mWheel[1].mRideHeight = 0.1;
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;

    // Ensure seeded for Hypercar (9500N)
    engine.calculate_force(&data, "Hypercar");

    // Set peak to 8000N manually
    FFBEngineTestAccess::SetAutoPeakLoad(engine, 8000.0);

    // Feed 4000N load for 1 second (100 steps of 0.01s)
    data.mWheel[0].mTireLoad = 4000.0;
    data.mWheel[1].mTireLoad = 4000.0;

    for (int i = 0; i < 100; i++) {
        engine.calculate_force(&data, "Hypercar");
    }

    // Should stay at 8000.0 (no decay)
    double peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 8000.0, 1.0);
}

TEST_CASE(test_lmp2_restricted_load, "Physics") {
    std::cout << "\nTest: Load Normalization - LMP2 Restricted (Enabled)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_auto_load_normalization_enabled = true;

    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.01;
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;

    // Test LMP2 Restricted (Issue #225: Should be 7500N)
    engine.calculate_force(&data, "LMP2", "Generic ORECA");
    double peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 7500.0, 1.0);
}

TEST_CASE(test_hypercar_bottoming_threshold, "Physics") {
    std::cout << "\nTest: Hypercar Bottoming Threshold" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    FFBEngineTestAccess::SetAutoNormalizationEnabled(engine, false);
    engine.m_bottoming_enabled = true;
    engine.m_bottoming_gain = 1.0;
    engine.m_bottoming_method = 1; // Method B: Force Spike (which now includes safety trigger)

    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.005;
    data.mLocalVel.z = -20.0;
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;

    // Seed as Hypercar (9500N)
    engine.calculate_force(&data, "Hypercar");
    // Clear seeding for next call
    FFBEngineTestAccess::SetDerivativesSeeded(engine, true);

    // 1. Test 10000N load (Should be below threshold 9500 * 2.5 = 23750)
    data.mWheel[0].mTireLoad = 10000.0;
    data.mWheel[1].mTireLoad = 10000.0;
    data.mDeltaTime = 0.003; // Change dt to avoid phase cancellation

    engine.calculate_force(&data, "Hypercar");

    auto snaps = engine.GetDebugBatch();
    if (!snaps.empty()) {
        ASSERT_NEAR(snaps.back().texture_bottoming, 0.0, 0.001);
    }

    // 2. Test 30000N load (Should be ABOVE threshold 23750)
    data.mWheel[0].mTireLoad = 30000.0;
    data.mWheel[1].mTireLoad = 30000.0;
    data.mDeltaTime = 0.003; // Change dt to avoid phase cancellation

    engine.calculate_force(&data, "Hypercar");
    snaps = engine.GetDebugBatch();
    if (!snaps.empty()) {
        ASSERT_TRUE(std::abs(snaps.back().texture_bottoming) > 0.001);
    }
}

TEST_CASE(test_static_load_fallback_class_aware, "Physics") {
    std::cout << "\nTest: Static Load Fallback Class-Aware (Enabled)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_auto_load_normalization_enabled = true;

    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.01;
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;

    // Seed as Hypercar (9500N)
    engine.calculate_force(&data, "Hypercar");
    // Seeding call for metadata and static load
    data.mElapsedTime += 0.01;
    engine.calculate_force(&data, "Hypercar");

    // Static load reference should be 9500 * 0.5 = 4750.0
    double static_load = FFBEngineTestAccess::GetStaticFrontLoad(engine);
    ASSERT_NEAR(static_load, 4750.0, 1.0);
}

TEST_CASE(test_load_normalization_disabled_behavior, "Physics") {
    std::cout << "\nTest: Load Normalization Disabled Behavior (Disabled)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    // Default should be DISABLED
    ASSERT_FALSE(engine.m_auto_load_normalization_enabled);

    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.01;
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;

    // Seed as GT3 (5000N)
    engine.calculate_force(&data, "LMGT3");
    data.mElapsedTime += 0.01;
    engine.calculate_force(&data, "LMGT3");
    ASSERT_NEAR(FFBEngineTestAccess::GetAutoPeakLoad(engine), 5000.0, 1.0);

    // Feed massive load
    data.mWheel[0].mTireLoad = 10000.0;
    data.mWheel[1].mTireLoad = 10000.0;

    data.mElapsedTime += 0.01;
    engine.calculate_force(&data, "LMGT3");

    // Should NOT update peak
    ASSERT_NEAR(FFBEngineTestAccess::GetAutoPeakLoad(engine), 5000.0, 1.0);

    // Switch to Hypercar
    data.mElapsedTime += 0.01;
    engine.calculate_force(&data, "Hypercar");
    data.mElapsedTime += 0.01;
    engine.calculate_force(&data, "Hypercar");
    // Should update to Hypercar seed (9500N)
    ASSERT_NEAR(FFBEngineTestAccess::GetAutoPeakLoad(engine), 9500.0, 1.0);

    // Seed as GT3 (5000N)
    data.mElapsedTime += 0.01;
    engine.calculate_force(&data, "LMGT3");
    data.mElapsedTime += 0.01;
    engine.calculate_force(&data, "LMGT3");
    double sl = FFBEngineTestAccess::GetStaticFrontLoad(engine);
    ASSERT_NEAR(sl, 2500.0, 1.0);
}

TEST_CASE(test_static_load_fallback_class_aware_disabled, "Physics") {
    std::cout << "\nTest: Static Load Fallback Class-Aware (Disabled)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_auto_load_normalization_enabled = false;

    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.01;
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;

    // Seed as Hypercar (9500N)
    engine.calculate_force(&data, "Hypercar");
    data.mElapsedTime += 0.01;
    engine.calculate_force(&data, "Hypercar");

    // Static load reference should be 9500 * 0.5 = 4750.0
    double static_load = FFBEngineTestAccess::GetStaticFrontLoad(engine);
    ASSERT_NEAR(static_load, 4750.0, 1.0);

    // Seed as GT3 (5000N)
    data.mElapsedTime += 0.01;
    engine.calculate_force(&data, "LMGT3");
    data.mElapsedTime += 0.01;
    engine.calculate_force(&data, "LMGT3");
    static_load = FFBEngineTestAccess::GetStaticFrontLoad(engine);
    ASSERT_NEAR(static_load, 2500.0, 1.0);
}

} // namespace FFBEngineTests
