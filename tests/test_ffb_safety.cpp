#include "test_ffb_common.h"
#include "../src/FFBEngine.h"

namespace FFBEngineTests {

TEST_CASE(test_ffb_safety_allowed_logic, "Safety") {
    std::cout << "\nTest: FFB Safety - Allowed Logic (Issue #126 / #79)" << std::endl;
    FFBEngine engine;

    VehicleScoringInfoV01 scoring;
    std::memset(&scoring, 0, sizeof(scoring));

    // Default state: mIsPlayer=true, mControl=0, mFinishStatus=0, gamePhase=5 (Green)
    scoring.mIsPlayer = true;
    scoring.mControl = 0;      // Local Player
    scoring.mFinishStatus = 0; // None
    unsigned char phase = 5;

    ASSERT_TRUE(engine.IsFFBAllowed(scoring, phase));
    std::cout << "  [PASS] FFB allowed for local player in race." << std::endl;

    // Case 1: Session Finished (Individual Status) - Now allowed for #126
    scoring.mFinishStatus = 1; // Finished
    ASSERT_TRUE(engine.IsFFBAllowed(scoring, phase));
    std::cout << "  [PASS] FFB allowed when individual finished but session active (#126)." << std::endl;

    // Case 2: DNF - Now allowed for coasting
    scoring.mFinishStatus = 2; // DNF
    ASSERT_TRUE(engine.IsFFBAllowed(scoring, phase));
    std::cout << "  [PASS] FFB allowed on DNF while session active." << std::endl;

    // Case 3: Disqualified - Still muted for safety/penalty
    scoring.mFinishStatus = 3; // DQ
    ASSERT_TRUE(!engine.IsFFBAllowed(scoring, phase));
    std::cout << "  [PASS] FFB muted on DQ." << std::endl;

    // Case 4: Session officially Over (Game Phase 8)
    scoring.mFinishStatus = 0;
    phase = 8;
    ASSERT_TRUE(!engine.IsFFBAllowed(scoring, phase));
    std::cout << "  [PASS] FFB muted when session officially over (Phase 8)." << std::endl;

    // Case 5: AI Control
    phase = 5;
    scoring.mControl = 1;      // AI
    ASSERT_TRUE(!engine.IsFFBAllowed(scoring, phase));
    std::cout << "  [PASS] FFB muted under AI control." << std::endl;

    // Case 6: Not player vehicle
    scoring.mControl = 0;
    scoring.mIsPlayer = false;
    ASSERT_TRUE(!engine.IsFFBAllowed(scoring, phase));
    std::cout << "  [PASS] FFB muted for non-player vehicles." << std::endl;

    g_tests_passed++;
}

TEST_CASE(test_ffb_safety_slew_limiter, "Safety") {
    std::cout << "\nTest: FFB Safety - Slew Rate Limiter (#79)" << std::endl;
    FFBEngine engine;
    double dt = 0.0025; // 400Hz

    // Normal Mode: 1000 units/s
    // 0 to 1.0 in 1 frame (dt=0.0025) would be 1.0/0.0025 = 400 units/s.
    // So 1000 units/s should allow a 0 to 1.0 jump in a single frame.
    double force1 = engine.ApplySafetySlew(1.0, dt, false);
    ASSERT_NEAR(force1, 1.0, 0.001);
    std::cout << "  [PASS] Normal mode allows rapid changes (up to 1000 u/s)." << std::endl;

    // Reset
    engine.ApplySafetySlew(0.0, 1.0, false); // Instant reset via large dt

    // Restricted Mode: 100 units/s
    // Max change per frame: 100 * 0.0025 = 0.25
    double force2 = engine.ApplySafetySlew(1.0, dt, true);
    ASSERT_NEAR(force2, 0.25, 0.001);
    std::cout << "  [PASS] Restricted mode clamps change to 100 u/s (0.25 per frame @ 400Hz)." << std::endl;

    double force3 = engine.ApplySafetySlew(1.0, dt, true);
    ASSERT_NEAR(force3, 0.50, 0.001);
    std::cout << "  [PASS] Second frame continues slew toward target." << std::endl;

    // NaN Safety
    double force_nan = engine.ApplySafetySlew(NAN, dt, false);
    ASSERT_NEAR(force_nan, 0.0, 0.001);
    std::cout << "  [PASS] NaN input results in 0.0 output." << std::endl;

    g_tests_passed++;
}

} // namespace FFBEngineTests
