#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_class_seeding, "Physics") {
    std::cout << "\nTest: Load Normalization - Class Seeding" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

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
    engine.calculate_force(&data, "lmgt3", "Test");
    peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 4800.0, 1.0);

    // 4. Test LMP2 (WEC) - Partial match
    engine.calculate_force(&data, "LMP2 2023", "Oreca 07");
    peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 8000.0, 1.0);

    // 5. Test LMP2 (ELMS) - Keyword match
    engine.calculate_force(&data, "LMP2", "Oreca 07 (derestricted)");
    peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 8500.0, 1.0);
}

TEST_CASE(test_fallback_seeding, "Physics") {
    std::cout << "\nTest: Load Normalization - Fallback Seeding (Name Keyword)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

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
    ASSERT_NEAR(peak, 4800.0, 1.0);
}

TEST_CASE(test_peak_hold_adaptation, "Physics") {
    std::cout << "\nTest: Load Normalization - Peak Hold Adaptation (Fast Attack)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.01;
    data.mWheel[0].mRideHeight = 0.1;
    data.mWheel[1].mRideHeight = 0.1;
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;

    // Seed as GT3 (4800N)
    engine.calculate_force(&data, "GT3");

    // Feed 6000N load
    data.mWheel[0].mTireLoad = 6000.0;
    data.mWheel[1].mTireLoad = 6000.0;

    engine.calculate_force(&data, "GT3");

    double peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 6000.0, 1.0);
}

TEST_CASE(test_peak_hold_decay, "Physics") {
    std::cout << "\nTest: Load Normalization - Peak Hold Decay (Slow Decay)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

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

TEST_CASE(test_lmp2_unspecified_load, "Physics") {
    std::cout << "\nTest: Load Normalization - LMP2 Unspecified" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.01;
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;

    // Test LMP2 Unspecified (Should be 8000N now)
    engine.calculate_force(&data, "LMP2", "Generic ORECA");
    double peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 8000.0, 1.0);
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

    // 1. Test 10000N load (Should be below threshold 9500 * 1.6 = 15200)
    data.mWheel[0].mTireLoad = 10000.0;
    data.mWheel[1].mTireLoad = 10000.0;
    data.mDeltaTime = 0.003; // Change dt to avoid phase cancellation

    engine.calculate_force(&data, "Hypercar");

    auto snaps = engine.GetDebugBatch();
    if (!snaps.empty()) {
        ASSERT_NEAR(snaps.back().texture_bottoming, 0.0, 0.001);
    }

    // 2. Test 16000N load (Should be ABOVE threshold 15200)
    data.mWheel[0].mTireLoad = 16000.0;
    data.mWheel[1].mTireLoad = 16000.0;
    data.mDeltaTime = 0.003; // Change dt to avoid phase cancellation

    engine.calculate_force(&data, "Hypercar");
    snaps = engine.GetDebugBatch();
    if (!snaps.empty()) {
        ASSERT_TRUE(std::abs(snaps.back().texture_bottoming) > 0.001);
    }
}

TEST_CASE(test_static_load_fallback_class_aware, "Physics") {
    std::cout << "\nTest: Static Load Fallback Class-Aware" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);

    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    data.mDeltaTime = 0.01;
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;

    // Seed as Hypercar (9500N)
    engine.calculate_force(&data, "Hypercar");

    // Static load reference should be 9500 * 0.5 = 4750.0
    double static_load = FFBEngineTestAccess::GetStaticFrontLoad(engine);
    ASSERT_NEAR(static_load, 4750.0, 1.0);

    // Seed as GT3 (4800N)
    engine.calculate_force(&data, "GT3");
    static_load = FFBEngineTestAccess::GetStaticFrontLoad(engine);
    ASSERT_NEAR(static_load, 2400.0, 1.0);
}

} // namespace FFBEngineTests
