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

    // 1. Test Default/Unknown
    engine.calculate_force(&data, "UnknownClass");
    double peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 4000.0, 1.0);

    // 2. Test Hypercar
    engine.calculate_force(&data, "Hypercar");
    peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 9500.0, 1.0);

    // 3. Test GT3
    engine.calculate_force(&data, "GT3");
    peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 4800.0, 1.0);

    // 4. Test LMP2 (WEC)
    engine.calculate_force(&data, "LMP2 (WEC)");
    peak = FFBEngineTestAccess::GetAutoPeakLoad(engine);
    ASSERT_NEAR(peak, 7500.0, 1.0);
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

} // namespace FFBEngineTests
