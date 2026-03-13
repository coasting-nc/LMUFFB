#include "test_ffb_common.h"

using namespace FFBEngineTests;

TEST_CASE(test_issue_345_load_approximation, "Physics") {
    FFBEngine engine;
    InitializeEngine(engine);

    TelemWheelV01 w;
    w.mSuspForce = 2000.0; // Input pushrod force

    // 1. Hypercar (MR = 0.50, Front Offset = 400N, Rear Offset = 450N)
    FFBEngineTestAccess::CallInitializeLoadReference(engine, "Hypercar", "499P");

    double hyper_front = engine.approximate_load(w);
    double hyper_rear = engine.approximate_rear_load(w);

    // Expected Front: (2000 * 0.50) + 400 = 1400
    ASSERT_NEAR(hyper_front, 1400.0, 0.1);
    // Expected Rear: (2000 * 0.50) + 450 = 1450
    ASSERT_NEAR(hyper_rear, 1450.0, 0.1);

    // 2. GT3 (MR = 0.65, Front Offset = 500N, Rear Offset = 550N)
    FFBEngineTestAccess::CallInitializeLoadReference(engine, "GT3", "M4 GT3");

    double gt3_front = engine.approximate_load(w);
    double gt3_rear = engine.approximate_rear_load(w);

    // Expected Front: (2000 * 0.65) + 500 = 1800
    ASSERT_NEAR(gt3_front, 1800.0, 0.1);
    // Expected Rear: (2000 * 0.65) + 550 = 1850
    ASSERT_NEAR(gt3_rear, 1850.0, 0.1);

    // 3. Default/Unknown (MR = 0.55, Front Offset = 450N, Rear Offset = 500N)
    FFBEngineTestAccess::CallInitializeLoadReference(engine, "Unknown", "My Mod Car");

    double def_front = engine.approximate_load(w);
    double def_rear = engine.approximate_rear_load(w);

    // Expected Front: (2000 * 0.55) + 450 = 1550
    ASSERT_NEAR(def_front, 1550.0, 0.1);
    // Expected Rear: (2000 * 0.55) + 500 = 1600
    ASSERT_NEAR(def_rear, 1600.0, 0.1);

    // 4. Verification of Cadillac (Hypercar keywords)
    FFBEngineTestAccess::CallInitializeLoadReference(engine, "Unknown", "Cadillac V-Series.R");
    double cad_front = engine.approximate_load(w);
    ASSERT_NEAR(cad_front, 1400.0, 0.1); // Should match Hypercar MR/Offset
}
