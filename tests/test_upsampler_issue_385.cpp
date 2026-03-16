#include "test_ffb_common.h"
#include "../src/ffb/UpSampler.h"
#include <vector>

namespace FFBEngineTests {

using namespace ffb_math;

// Regression test for Issue #385: Reversed convolution and incorrect phase advancement
TEST_CASE(test_upsampler_issue_385_regression, "UpSamplerPart2") {
    PolyphaseResampler resampler;
    resampler.Reset();

    std::vector<double> outputs;
    double p0 = 1.0;

    // Tick 0: New Physics sample P0 = 1.0
    // History [0, 0, 1.0]. Phase 0.
    // Correct output = c[0]*1.0 + c[1]*0 + c[2]*0 = -0.01855
    outputs.push_back(resampler.Process(p0, true));

    // Tick 1: No new physics. Phase 2.
    // Correct output = c[2][0]*1.0 + c[2][1]*0 + c[2][2]*0 = 0.0 * 1.0 = 0.0
    outputs.push_back(resampler.Process(p0, false));

    // Tick 2: No new physics. Phase 4.
    // Correct output = c[4][0]*1.0 + c[4][1]*0 + c[4][2]*0 = 0.34753 * 1.0 = 0.34753
    outputs.push_back(resampler.Process(p0, false));

    ASSERT_NEAR(outputs[0], -0.01855, 0.0001);
    ASSERT_NEAR(outputs[1], 0.00000, 0.0001);
    ASSERT_NEAR(outputs[2], 0.34753, 0.0001);
}

TEST_CASE(test_upsampler_phase_sequence_regression, "UpSamplerPart2") {
    PolyphaseResampler resampler;
    resampler.Reset();

    // 0 -> 2 -> 4 -> 1 -> 3 -> 0

    // Call 1: Phase 0. History [0, 0, 1]. phase -> 2.
    resampler.Process(1.0, true);
    // Call 2: Phase 2. History [0, 0, 1]. phase -> 4.
    double out1 = resampler.Process(1.0, false);
    // Phase 2 is identity on middle tap. History [0, 0, 1]. Middle is 0.
    ASSERT_NEAR(out1, 0.0, 0.0001);

    // Call 3: Phase 4. phase -> 1.
    resampler.Process(1.0, false);
    // Call 4: Phase 1. phase -> 3.
    resampler.Process(1.0, false);
    // Call 5: Phase 3. phase -> 0.
    resampler.Process(1.0, false);
    // Call 6: Phase 0. phase -> 2.
    double out6 = resampler.Process(1.0, false);
    ASSERT_NEAR(out6, -0.01855, 0.0001);
}

} // namespace FFBEngineTests
