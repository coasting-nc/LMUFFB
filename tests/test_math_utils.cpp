#include "test_ffb_common.h"
#include "MathUtils.h"

namespace FFBEngineTests {

TEST_CASE(test_biquad_notch_stability, "Math") {
    ffb_math::BiquadNotch filter;

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
    ffb_math::BiquadNotch filter;

    // Low frequency clamping (min 1.0Hz)
    filter.Update(0.1, 400.0, 1.0);
    ASSERT_NEAR(filter.b0, 1.0 / (1.0 + std::sin(2.0*ffb_math::PI*1.0/400.0)/(2.0*1.0)), 0.0001);

    // High frequency clamping (max 0.49 * sample_rate)
    filter.Update(300.0, 400.0, 1.0); // 300Hz > 196Hz (400*0.49)
    ASSERT_NEAR(filter.b0, 1.0 / (1.0 + std::sin(2.0*ffb_math::PI*196.0/400.0)/(2.0*1.0)), 0.0001);
}

TEST_CASE(test_inverse_lerp_behavior, "Math") {
    // Normal range
    ASSERT_NEAR(ffb_math::inverse_lerp(0.0, 10.0, 5.0), 0.5, 0.001);
    
    // Clamping
    ASSERT_NEAR(ffb_math::inverse_lerp(0.0, 10.0, 15.0), 1.0, 0.001);
    ASSERT_NEAR(ffb_math::inverse_lerp(0.0, 10.0, -5.0), 0.0, 0.001);
    
    // Inverse range (min > max)
    ASSERT_NEAR(ffb_math::inverse_lerp(10.0, 0.0, 5.0), 0.5, 0.001);
    
    // Degenerate case (zero range): Implementation returns 1.0 if val >= min, else 0.0
    ASSERT_NEAR(ffb_math::inverse_lerp(5.0, 5.0, 5.0), 1.0, 0.001);
    ASSERT_NEAR(ffb_math::inverse_lerp(5.0, 5.0, 4.0), 0.0, 0.001); // value < min -> 0.0
    ASSERT_NEAR(ffb_math::inverse_lerp(5.0, 5.0, 6.0), 1.0, 0.001); // value >= min -> 1.0

    // Inverse degenerate case (near-zero range, min > max): Implementation returns 1.0 if val <= min
    ASSERT_NEAR(ffb_math::inverse_lerp(5.0, 4.999999, 5.0), 1.0, 0.001); // value <= min -> 1.0
    ASSERT_NEAR(ffb_math::inverse_lerp(5.0, 4.999999, 5.1), 0.0, 0.001); // value > min -> 0.0
    ASSERT_NEAR(ffb_math::inverse_lerp(5.0, 4.999999, 4.0), 1.0, 0.001); // value <= min -> 1.0
}

TEST_CASE(test_smoothstep_behavior, "Math") {
    ASSERT_NEAR(ffb_math::smoothstep(0.0, 10.0, 0.0), 0.0, 0.001);
    ASSERT_NEAR(ffb_math::smoothstep(0.0, 10.0, 10.0), 1.0, 0.001);
    ASSERT_NEAR(ffb_math::smoothstep(0.0, 10.0, 5.0), 0.5, 0.001); // Symmetry at center
    
    // Clamping
    ASSERT_NEAR(ffb_math::smoothstep(0.0, 10.0, 15.0), 1.0, 0.001);
    ASSERT_NEAR(ffb_math::smoothstep(0.0, 10.0, -5.0), 0.0, 0.001);

    // Degenerate case (zero range): Implementation returns 1.0 if x >= edge0, else 0.0
    ASSERT_NEAR(ffb_math::smoothstep(5.0, 5.0, 5.0), 1.0, 0.001); // x >= edge0
    ASSERT_NEAR(ffb_math::smoothstep(5.0, 5.0, 4.0), 0.0, 0.001); // x < edge0
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
    double deriv = ffb_math::calculate_sg_derivative(buffer, 41, window, dt, index);
    ASSERT_NEAR(deriv, 2.0, 0.001);
}

TEST_CASE(test_sg_derivative_buffer_states, "Math") {
    std::array<double, 41> buffer = {};
    double dt = 0.01;
    int window = 15;
    int index = 0;
    
    // Empty buffer
    double deriv = ffb_math::calculate_sg_derivative(buffer, 0, window, dt, index);
    ASSERT_NEAR(deriv, 0.0, 0.001);
    
    // 1-sample buffer
    deriv = ffb_math::calculate_sg_derivative(buffer, 1, window, dt, index);
    ASSERT_NEAR(deriv, 0.0, 0.001);
    
    // Half-full ( < window)
    deriv = ffb_math::calculate_sg_derivative(buffer, 7, window, dt, index);
    ASSERT_NEAR(deriv, 0.0, 0.001);
}

TEST_CASE(test_adaptive_smoothing, "Math") {
    double prev_out = 0.0;
    double dt = 0.0025; // 400Hz
    
    // Test slow smoothing (input near zero)
    double out1 = ffb_math::apply_adaptive_smoothing(0.1, prev_out, dt, 0.05, 0.005, 1.0);
    ASSERT_NEAR(out1, 0.00476, 0.001);
    
    // Test fast response (large delta)
    prev_out = 0.0;
    double out2 = ffb_math::apply_adaptive_smoothing(10.0, prev_out, dt, 0.05, 0.005, 1.0);
    ASSERT_NEAR(out2, 3.333, 0.01);

    // Test extreme sensitivity: Implementation handles sensitivity=0 by clamping t to 1.0
    prev_out = 0.0;
    double out3 = ffb_math::apply_adaptive_smoothing(0.1, prev_out, dt, 0.05, 0.005, 0.0);
    ASSERT_NEAR(out3, 0.0333, 0.001);
}

TEST_CASE(test_slew_limiter, "Math") {
    double prev_val = 1.0;
    double dt = 0.01; // 100Hz
    double limit = 10.0; // max 10 units / second
    
    // max change = 10 * 0.01 = 0.1
    
    // Attempt large jump (1.0 -> 5.0)
    double out = ffb_math::apply_slew_limiter(5.0, prev_val, limit, dt);
    ASSERT_NEAR(out, 1.1, 0.001);
    ASSERT_NEAR(prev_val, 1.1, 0.001);
    
    // Small jump (1.1 -> 1.15)
    out = ffb_math::apply_slew_limiter(1.15, prev_val, limit, dt);
    ASSERT_NEAR(out, 1.15, 0.001);
}

} // namespace FFBEngineTests
