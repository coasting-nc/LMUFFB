#include "test_ffb_common.h"
#include "MathUtils.h"

using namespace LMUFFB::Utils;

namespace FFBEngineTests {

// Regression test for Issue #393: Holt-Winters 100Hz Sawtooth (The "Buzz" Bug)
TEST_CASE(test_math_utils_issue_393_holt_winters_continuity, "Math") {
    HoltWintersFilter filter;
    filter.Configure(0.5, 0.1, 0.01); // alpha=0.5, 100Hz game tick

    // Initial value
    filter.Process(10.0, 0.0025, true);

    // Next frame (10ms later) with a jump to 20.0
    // CORRECT BEHAVIOR: Returns smoothed value (around 15.0 if alpha=0.5)

    double out = filter.Process(20.0, 0.0025, true);

    ASSERT_LT(out, 19.9);
    ASSERT_GT(out, 10.1);
}

} // namespace FFBEngineTests
