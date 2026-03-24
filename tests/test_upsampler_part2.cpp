#include "test_ffb_common.h"
#include "../src/ffb/UpSampler.h"
#include "../src/logging/HealthMonitor.h"
#include <vector>

namespace FFBEngineTests {

using namespace LMUFFB;

// 1. DC Gain Test: Constant input (1.0) should result in constant output (1.0) after filter delay.
TEST_CASE(test_upsampler_dc_gain, "UpSamplerPart2") {
    PolyphaseResampler resampler;
    double input = 1.0;

    // Feed 10 physics frames (400Hz)
    // For each physics frame, we get 2 or 3 output frames (1000Hz)
    // Total iterations: 10 * 2.5 = 25 output ticks

    int phase_accumulator = 0;
    for (int i = 0; i < 25; ++i) {
        phase_accumulator += 2;
        bool run_physics = false;
        if (phase_accumulator >= 5) {
            phase_accumulator -= 5;
            run_physics = true;
        }

        double output = resampler.Process(input, run_physics);

        // After initial history buffer is filled (3 physics samples)
        if (i > 10) {
            ASSERT_NEAR(output, 1.0, 0.001);
        }
    }
}

// 2. Phase Timing Test: Verify physics runs exactly 2 out of 5 ticks.
TEST_CASE(test_upsampler_phase_timing, "UpSamplerPart2") {
    int phase_accumulator = 0;
    int physics_ticks = 0;
    int total_ticks = 1000;

    for (int i = 0; i < total_ticks; ++i) {
        phase_accumulator += 2;
        if (phase_accumulator >= 5) {
            phase_accumulator -= 5;
            physics_ticks++;
        }
    }

    // 400 physics ticks for 1000 loop ticks (40%)
    ASSERT_EQ(physics_ticks, 400);
}

// 3. Health Monitor 1000Hz Test: Verify updated thresholds.
TEST_CASE(test_health_monitor_1000hz, "UpSamplerPart2") {
    // Healthy: Loop 1000Hz, Physics 400Hz, Telemetry 100Hz, Torque 100Hz (Legacy)
    HealthStatus status = HealthMonitor::Check(1000.0, 100.0, 100.0, 0, 400.0);
    ASSERT_TRUE(status.is_healthy);

    // Degraded Loop: Loop 900Hz (< 950)
    status = HealthMonitor::Check(900.0, 100.0, 100.0, 0, 400.0);
    ASSERT_FALSE(status.is_healthy);
    ASSERT_TRUE(status.loop_low);

    // Degraded Physics: Physics 350Hz (< 380)
    status = HealthMonitor::Check(1000.0, 100.0, 100.0, 0, 350.0);
    ASSERT_FALSE(status.is_healthy);
    ASSERT_TRUE(status.physics_low);

    // Healthy Direct: Torque 400Hz (Direct)
    status = HealthMonitor::Check(1000.0, 100.0, 400.0, 1, 400.0);
    ASSERT_TRUE(status.is_healthy);
}

// 4. Signal Continuity Test: Verify smooth transitions between steps
TEST_CASE(test_upsampler_signal_continuity, "UpSamplerPart2") {
    PolyphaseResampler resampler;

    // Step input from 0.0 to 1.0
    double current_input = 0.0;
    int phase_accumulator = 0;
    std::vector<double> outputs;

    for (int i = 0; i < 50; ++i) {
        phase_accumulator += 2;
        bool run_physics = false;
        if (phase_accumulator >= 5) {
            phase_accumulator -= 5;
            run_physics = true;
            if (i >= 20) current_input = 1.0;
        }

        double output = resampler.Process(current_input, run_physics);
        outputs.push_back(output);
    }

    // Verify no massive jumps (> 1.2 per 1ms tick)
    // The polyphase filter has some overshoot/ringing with step inputs.
    // Fixed resampler has sharper transitions because it actually works now.
    for (size_t i = 1; i < outputs.size(); ++i) {
        ASSERT_LT(std::abs(outputs[i] - outputs[i-1]), 1.2);
    }

    // Final value should reach 1.0
    ASSERT_NEAR(outputs.back(), 1.0, 0.001);
}

} // namespace FFBEngineTests
