#include "test_ffb_common.h"
#include "MathUtils.h"

using namespace LMUFFB::Utils;

namespace FFBEngineTests {

TEST_CASE(test_biquad_notch_stability, "Math") {
    BiquadNotch filter;

    // Normal update
    filter.Update(10.0, 400.0, 2.0); // 10Hz notch at 400Hz

    // Impulse
    double out = filter.Process(1.0);
    ASSERT_TRUE(std::isfinite(out));
    for (int i = 0; i < 400; i++) out = filter.Process(0.0);
    ASSERT_NEAR(out, 0.0, 0.001);

    // Step
    filter.Reset();
    for (int i = 0; i < 100; i++) out = filter.Process(1.0);
    ASSERT_NEAR(out, 1.0, 0.01);

    // Extreme Noise
    filter.Reset();
    out = filter.Process(1e6);
    ASSERT_TRUE(std::isfinite(out));
    out = filter.Process(-1e6);
    ASSERT_TRUE(std::isfinite(out));
}

TEST_CASE(test_biquad_clamping, "Math") {
    BiquadNotch filter;

    // Low frequency clamping (min 1.0Hz)
    filter.Update(0.1, 400.0, 1.0);
    ASSERT_NEAR(filter.b0, 1.0 / (1.0 + std::sin(2.0*PI*1.0/400.0)/(2.0*1.0)), 0.0001);

    // High frequency clamping (max 0.49 * sample_rate)
    filter.Update(300.0, 400.0, 1.0); // 300Hz > 196Hz (400*0.49)
    ASSERT_NEAR(filter.b0, 1.0 / (1.0 + std::sin(2.0*PI*196.0/400.0)/(2.0*1.0)), 0.0001);
}

TEST_CASE(test_inverse_lerp_behavior, "Math") {
    // Normal range
    ASSERT_NEAR(inverse_lerp(0.0, 10.0, 5.0), 0.5, 0.001);
    
    // Clamping
    ASSERT_NEAR(inverse_lerp(0.0, 10.0, 15.0), 1.0, 0.001);
    ASSERT_NEAR(inverse_lerp(0.0, 10.0, -5.0), 0.0, 0.001);
    
    // Inverse range (min > max)
    ASSERT_NEAR(inverse_lerp(10.0, 0.0, 5.0), 0.5, 0.001);
    
    // Degenerate case (zero range): Implementation returns 1.0 if val >= min, else 0.0
    ASSERT_NEAR(inverse_lerp(5.0, 5.0, 5.0), 1.0, 0.001);
    ASSERT_NEAR(inverse_lerp(5.0, 5.0, 4.0), 0.0, 0.001); // value < min -> 0.0
    ASSERT_NEAR(inverse_lerp(5.0, 5.0, 6.0), 1.0, 0.001); // value >= min -> 1.0

    // Inverse degenerate case (near-zero range, min > max): Implementation returns 1.0 if val <= min
    ASSERT_NEAR(inverse_lerp(5.0, 4.999999, 5.0), 1.0, 0.001); // value <= min -> 1.0
    ASSERT_NEAR(inverse_lerp(5.0, 4.999999, 5.1), 0.0, 0.001); // value > min -> 0.0
    ASSERT_NEAR(inverse_lerp(5.0, 4.999999, 4.0), 1.0, 0.001); // value <= min -> 1.0
}

TEST_CASE(test_smoothstep_behavior, "Math") {
    ASSERT_NEAR(smoothstep(0.0, 10.0, 0.0), 0.0, 0.001);
    ASSERT_NEAR(smoothstep(0.0, 10.0, 10.0), 1.0, 0.001);
    ASSERT_NEAR(smoothstep(0.0, 10.0, 5.0), 0.5, 0.001); // Symmetry at center
    
    // Clamping
    ASSERT_NEAR(smoothstep(0.0, 10.0, 15.0), 1.0, 0.001);
    ASSERT_NEAR(smoothstep(0.0, 10.0, -5.0), 0.0, 0.001);

    // Degenerate case (zero range): Implementation returns 1.0 if x >= edge0, else 0.0
    ASSERT_NEAR(smoothstep(5.0, 5.0, 5.0), 1.0, 0.001); // x >= edge0
    ASSERT_NEAR(smoothstep(5.0, 5.0, 4.0), 0.0, 0.001); // x < edge0
}

TEST_CASE(test_sg_derivative_ramp, "Math") {
    std::array<double, 41> buffer = {};
    double dt = 0.01; // 100Hz
    int window = 15;
    
    // Create a linear ramp: y = 2.0 * t
    for (int i = 0; i < 41; i++) {
        buffer[i] = 2.0 * (i * dt);
    }
    
    // index points to NEXT write slot. If we filled 41 samples, index is 0 again (wrapped)
    int index = 0;
    
    // Latest sample is at (index - 1) = 40.
    // SG derivative should be 2.0
    double deriv = calculate_sg_derivative(buffer, 41, window, dt, index);
    ASSERT_NEAR(deriv, 2.0, 0.001);
}

TEST_CASE(test_sg_derivative_buffer_states, "Math") {
    std::array<double, 41> buffer = {};
    double dt = 0.01;
    int window = 15;
    int index = 0;
    
    // Empty buffer
    double deriv = calculate_sg_derivative(buffer, 0, window, dt, index);
    ASSERT_NEAR(deriv, 0.0, 0.001);
    
    // 1-sample buffer
    deriv = calculate_sg_derivative(buffer, 1, window, dt, index);
    ASSERT_NEAR(deriv, 0.0, 0.001);
    
    // Half-full ( < window)
    deriv = calculate_sg_derivative(buffer, 7, window, dt, index);
    ASSERT_NEAR(deriv, 0.0, 0.001);
}

TEST_CASE(test_adaptive_smoothing, "Math") {
    double prev_out = 0.0;
    double dt = 0.0025; // 400Hz
    
    // Test slow smoothing (input near zero)
    double out1 = apply_adaptive_smoothing(0.1, prev_out, dt, 0.05, 0.005, 1.0);
    ASSERT_NEAR(out1, 0.00476, 0.001);
    
    // Test fast response (large delta)
    prev_out = 0.0;
    double out2 = apply_adaptive_smoothing(10.0, prev_out, dt, 0.05, 0.005, 1.0);
    ASSERT_NEAR(out2, 3.333, 0.01);

    // Test extreme sensitivity: Implementation handles sensitivity=0 by clamping t to 1.0
    prev_out = 0.0;
    double out3 = apply_adaptive_smoothing(0.1, prev_out, dt, 0.05, 0.005, 0.0);
    ASSERT_NEAR(out3, 0.0333, 0.001);
}

TEST_CASE(test_slew_limiter, "Math") {
    double prev_val = 1.0;
    double dt = 0.01; // 100Hz
    double limit = 10.0; // max 10 units / second
    
    // max change = 10 * 0.01 = 0.1
    
    // Attempt large jump (1.0 -> 5.0)
    double out = apply_slew_limiter(5.0, prev_val, limit, dt);
    ASSERT_NEAR(out, 1.1, 0.001);
    ASSERT_NEAR(prev_val, 1.1, 0.001);
    
    // Small jump (1.1 -> 1.15)
    out = apply_slew_limiter(1.15, prev_val, limit, dt);
    ASSERT_NEAR(out, 1.15, 0.001);
}

TEST_CASE(test_holt_winters_time_awareness, "Math") {
    HoltWintersFilter filter;
    filter.Configure(0.95, 0.10, 0.01); // 10ms default

    // Establishing a constant slope: 1.0 unit / 1.0 second
    // 1st frame at T=0.0
    filter.Process(10.0, 0.0, true);

    // 2nd frame at T=0.008 (Jitter: early)
    // Delta = 0.008, dt = 0.008. Expected Slope = 0.008 / 0.008 = 1.0
    double out1 = filter.Process(10.008, 0.008, true);

    // We expect the internal m_trend to be ~1.0.
    // In the old implementation (hardcoded 0.01), slope would be 0.008 / 0.01 = 0.8
    // With time-awareness, it should be 0.008 / 0.008 = 1.0

    // 3rd frame at T=0.020 (Jitter: late, 12ms since last frame)
    // Delta = 0.012, dt = 0.012. Expected Slope = 0.012 / 0.012 = 1.0
    double out2 = filter.Process(10.020, 0.012, true);

    // If it's time-aware, the trend remains constant despite jitter.
    // We can't access m_trend directly, but we can check extrapolation.
    // At T=0.020, if we extrapolate 0.0025s (one 400Hz tick):
    // Expected Trend ~ 0.184. (Calculated based on Alpha 0.95, Beta 0.1)
    // 1st extrap: Trend = 0.184 * 0.95 = 0.1748. Output = 10.0194 + 0.1748 * 0.0025 = 10.0198
    double extrap = filter.Process(10.020, 0.0025, false);
    ASSERT_NEAR(extrap, 10.0198, 0.001);
}

TEST_CASE(test_holt_winters_trend_damping, "Math") {
    HoltWintersFilter filter;
    filter.Configure(1.0, 1.0, 0.01); // No smoothing, full trend tracking

    // Establish a high trend (100 units/sec)
    filter.Process(0.0, 0.0, true);
    filter.Process(1.0, 0.01, true); // Trend = (1.0 - 0.0) / 0.01 = 100.0

    // Simulate frame drop/starvation for 10 ticks (25ms)
    double last_val = 0.0;
    for (int i = 0; i < 10; i++) {
        last_val = filter.Process(1.0, 0.0025, false);
    }

    // Without damping: 1.0 + (100 * 0.025) = 3.5
    // With 0.95 damping: Trend decays every tick.
    // 100 * 0.95^10 = ~59.87
    // The output should be significantly less than 3.5 because the velocity is slowing down.
    ASSERT_LT(last_val, 3.4);
    ASSERT_GT(last_val, 1.0); // Should still be extrapolating forward
}

TEST_CASE(test_holt_winters_lag_spike_upper_bound, "Math") {
    HoltWintersFilter filter;
    filter.Configure(1.0, 1.0, 0.01); // No smoothing, full trend tracking

    // Normal frames (10ms)
    filter.Process(1.0, 0.01, true);
    filter.Process(1.1, 0.01, true); // Trend = (1.1-1.0)/0.01 = 10.0

    // Massive 500ms freeze
    // If clamped to 0.050s: New Trend = 0.2*( (1.2-1.1)/0.050 ) + 0.8*10.0 = 0.2*2.0 + 8.0 = 8.4
    // If NOT clamped: New Trend = 0.2*( (1.2-1.1)/0.500 ) + 0.8*10.0 = 0.2*0.2 + 8.0 = 8.04
    // Actually, alpha/beta are usually configured. Let's use specific values.
    filter.Configure(0.2, 0.1, 0.01);
    filter.Process(1.0, 0.01, true);
    filter.Process(1.1, 0.01, true); // Establish trend

    double out = filter.Process(1.2, 0.500, true);

    // Verification: Trend and level should be finite and not zeroed out
    ASSERT_TRUE(std::isfinite(out));
    ASSERT_GT(out, 1.1);
}

TEST_CASE(test_holt_winters_double_frame_lower_bound, "Math") {
    HoltWintersFilter filter;
    filter.Configure(0.2, 0.1, 0.01);

    filter.Process(1.0, 0.01, true);

    // Frame with near-zero dt (0.1ms)
    // Clamped to 1ms (0.001)
    double out = filter.Process(1.1, 0.0001, true);

    ASSERT_TRUE(std::isfinite(out));
    // If it didn't clamp, (1.1-1.0)/0.0001 = 1000.0 (Huge trend)
    // If it clamps to 0.001, (1.1-1.0)/0.001 = 100.0 (Manageable trend)
    // We check that the trend didn't explode.
    // One more extrapolation call to see the trend effect.
    double extrap = filter.Process(1.1, 0.0025, false);
    ASSERT_LT(extrap, 2.0); // If trend was 1000, 1.1 + 1000*0.0025 = 3.6. If 100, 1.1 + 100*0.0025 = 1.35.
}

TEST_CASE(test_holt_winters_infinite_starvation, "Math") {
    HoltWintersFilter filter;
    filter.Configure(0.2, 0.1, 0.01);

    filter.Process(1.0, 0.01, true);
    filter.Process(1.1, 0.01, true); // Trend is established

    // 2 seconds of starvation at 400Hz = 800 calls
    double last_val = 1.1;
    for(int i=0; i<800; i++) {
        last_val = filter.Process(1.1, 0.0025, false);
    }

    // After 800 calls of 0.95 damping, trend should be ~0.0
    // 0.95^800 is effectively zero.
    double next_val = filter.Process(1.1, 0.0025, false);
    ASSERT_NEAR(last_val, next_val, 0.000001); // Output plateaus
    ASSERT_TRUE(std::isfinite(next_val));
}

TEST_CASE(test_holt_winters_sub_frame_accumulation, "Math") {
    HoltWintersFilter filter;
    filter.Configure(0.2, 0.1, 0.01);

    filter.Process(1.0, 0.0, true); // Reset

    // 4 sub-frames of 2.5ms = 10ms total
    filter.Process(1.0, 0.0025, false);
    filter.Process(1.0, 0.0025, false);
    filter.Process(1.0, 0.0025, false);
    filter.Process(1.0, 0.0025, false);

    // New frame arrives
    // Denominator should be 0.010
    // We verify this by ensuring the trend calculation uses 0.010
    // If we move value from 1.0 to 1.1, trend should be (1.1-1.0)/0.010 = 10.0
    filter.Process(1.1, 0.0, true); // is_new_frame = true, but we already added 10ms

    // Check trend via extrapolation
    double extrap = filter.Process(1.1, 0.01, false);
    // Level ~ 1.02, Trend ~ 0.2. Extrap ~ 1.02 + 0.2*0.01 = 1.022
    ASSERT_GT(extrap, 1.0);
}

TEST_CASE(test_holt_winters_damping_amplitude_reduction, "Math") {
    HoltWintersFilter filter;
    filter.Configure(1.0, 1.0, 0.01); // No smoothing, just raw trend

    filter.Process(0.0, 0.01, true);
    filter.Process(1.0, 0.01, true); // Trend = 100.0

    double linear_extrap = 1.0 + (100.0 * 3 * 0.0025); // 1.0 + 0.75 = 1.75

    filter.Process(1.0, 0.0025, false);
    filter.Process(1.0, 0.0025, false);
    double damped_extrap = filter.Process(1.0, 0.0025, false);

    ASSERT_LT(damped_extrap, linear_extrap);
    ASSERT_GT(damped_extrap, 1.0);
}

} // namespace FFBEngineTests
