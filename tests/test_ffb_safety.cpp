#include "test_ffb_common.h"
#include "../src/FFBEngine.h"

namespace FFBEngineTests {

TEST_CASE(test_ffb_safety_allowed_logic, "Safety") {
    std::cout << "\nTest: FFB Safety - Allowed Logic (Issue #79)" << std::endl;
    FFBEngine engine;

    VehicleScoringInfoV01 scoring;
    std::memset(&scoring, 0, sizeof(scoring));

    // Default state: mIsPlayer=false, mControl=0, mFinishStatus=0
    scoring.mIsPlayer = true;
    scoring.mControl = 0;      // Local Player
    scoring.mFinishStatus = 0; // None

    ASSERT_TRUE(engine.IsFFBAllowed(scoring));
    std::cout << "  [PASS] FFB allowed for local player in race." << std::endl;

    // Case 1: Session Finished
    scoring.mFinishStatus = 1; // Finished
    ASSERT_TRUE(!engine.IsFFBAllowed(scoring));
    std::cout << "  [PASS] FFB muted when session finished." << std::endl;

    // Case 2: DNF
    scoring.mFinishStatus = 2; // DNF
    ASSERT_TRUE(!engine.IsFFBAllowed(scoring));
    std::cout << "  [PASS] FFB muted on DNF." << std::endl;

    // Case 3: AI Control
    scoring.mFinishStatus = 0;
    scoring.mControl = 1;      // AI
    ASSERT_TRUE(!engine.IsFFBAllowed(scoring));
    std::cout << "  [PASS] FFB muted under AI control." << std::endl;

    // Case 4: Remote Vehicle
    scoring.mControl = 2;      // Remote
    ASSERT_TRUE(!engine.IsFFBAllowed(scoring));
    std::cout << "  [PASS] FFB muted for remote vehicles." << std::endl;

    // Case 5: Not player vehicle
    scoring.mControl = 0;
    scoring.mIsPlayer = false;
    ASSERT_TRUE(!engine.IsFFBAllowed(scoring));
    std::cout << "  [PASS] FFB muted for non-player vehicles." << std::endl;

    g_tests_passed++; // Mark whole test case as passed if assertions succeeded
}

} // namespace FFBEngineTests
