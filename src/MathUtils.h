#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <cmath>
#include <algorithm>
#include <array>

namespace ffb_math {

// Mathematical Constants
static constexpr double PI = 3.14159265358979323846;
static constexpr double TWO_PI = 2.0 * PI;

/**
 * @brief Bi-quad Filter (Direct Form II)
 * 
 * Used for filtering oscillations (e.g., steering wheel "death wobbles") 
 * and smoothing out high-frequency road noise.
 */
struct BiquadNotch {
    
    // Coefficients
    double b0 = 0.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
    // State history (Inputs x, Outputs y)
    double x1 = 0.0, x2 = 0.0;
    double y1 = 0.0, y2 = 0.0;

    // Update coefficients based on dynamic frequency
    void Update(double center_freq, double sample_rate, double Q) {
        // Safety: Clamp frequency to Nyquist (sample_rate / 2) and min 1Hz
        center_freq = (std::max)(1.0, (std::min)(center_freq, sample_rate * 0.49));
        
        double omega = 2.0 * PI * center_freq / sample_rate;
        double sn = std::sin(omega);
        double cs = std::cos(omega);
        double alpha = sn / (2.0 * Q);

        double a0 = 1.0 + alpha;
        
        // Calculate and Normalize
        b0 = 1.0 / a0;
        b1 = (-2.0 * cs) / a0;
        b2 = 1.0 / a0;
        a1 = (-2.0 * cs) / a0;
        a2 = (1.0 - alpha) / a0;
    }

    // Apply filter to single sample
    double Process(double in) {
        double out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        
        // Shift history
        x2 = x1; x1 = in;
        y2 = y1; y1 = out;
        
        return out;
    }
    
    void Reset() {
        x1 = x2 = y1 = y2 = 0.0;
    }
};

// Helper: Inverse linear interpolation
// Returns normalized position of value between min and max
// Returns 0 if value >= min, 1 if value <= max (for negative threshold use)
// Clamped to [0, 1] range
inline double inverse_lerp(double min_val, double max_val, double value) {
    double range = max_val - min_val;
    if (std::abs(range) >= 0.0001) {
        double t = (value - min_val) / (std::abs(range) >= 0.0001 ? range : 1.0);
        return (std::max)(0.0, (std::min)(1.0, t));
    }
    
    // Degenerate case when range is zero or near-zero
    if (max_val >= min_val) return (value >= min_val) ? 1.0 : 0.0;
    return (value <= min_val) ? 1.0 : 0.0;
}

// Helper: Smoothstep interpolation
// Returns smooth S-curve interpolation from 0 to 1
// Uses Hermite polynomial: t² × (3 - 2t)
// Zero derivative at both endpoints for seamless transitions
inline double smoothstep(double edge0, double edge1, double x) {
    double range = edge1 - edge0;
    if (std::abs(range) >= 0.0001) {
        double t = (x - edge0) / (std::abs(range) >= 0.0001 ? range : 1.0);
        t = (std::max)(0.0, (std::min)(1.0, t));
        return t * t * (3.0 - 2.0 * t);
    }
    return (x < edge0) ? 0.0 : 1.0;
}

// Helper: Apply Slew Rate Limiter
// Clamps the rate of change of a signal.
inline double apply_slew_limiter(double input, double& prev_val, double limit, double dt) {
    double delta = input - prev_val;
    double max_change = limit * dt;
    delta = std::clamp(delta, -max_change, max_change);
    prev_val += delta;
    return prev_val;
}

// Helper: Adaptive Non-Linear Smoothing
// t=0 (Steady) uses slow_tau, t=1 (Transient) uses fast_tau
inline double apply_adaptive_smoothing(double input, double& prev_out, double dt,
                                double slow_tau, double fast_tau, double sensitivity) {
    double delta = std::abs(input - prev_out);
    double t = delta / (sensitivity + 0.000001);
    t = (std::min)(1.0, t);

    double tau = slow_tau + t * (fast_tau - slow_tau);
    double alpha = dt / (tau + dt + 1e-9);
    alpha = (std::max)(0.0, (std::min)(1.0, alpha));

    prev_out = prev_out + alpha * (input - prev_out);
    return prev_out;
}

// Helper: Calculate Savitzky-Golay First Derivative
// Uses closed-form coefficient generation for quadratic polynomial fit.
template <size_t BufferSize>
inline double calculate_sg_derivative(const std::array<double, BufferSize>& buffer, 
                            const int& buffer_count, const int& window, double dt, const int& buffer_index) {
    // Note: buffer_index passed from caller should be the current write index (next slot)
    
    // Ensure we have enough samples
    if (buffer_count < window) return 0.0;
    
    int M = window / 2;  // Half-width (e.g., window=15 -> M=7)
    
    // Calculate S_2 = M(M+1)(2M+1)/3
    double S2 = (double)M * (M + 1.0) * (2.0 * M + 1.0) / 3.0;
    
    // Correct Indexing (v0.7.0 Fix)
    // m_slope_buffer_index points to the next slot to write.
    // Latest sample is at (index - 1). Center is at (index - 1 - M).
    // buffer_index MUST be the same as m_slope_buffer_index
    int latest_idx = (buffer_index - 1 + BufferSize) % BufferSize;
    int center_idx = (latest_idx - M + BufferSize) % BufferSize;
    
    double sum = 0.0;
    for (int k = 1; k <= M; ++k) {
        int idx_pos = (center_idx + k + BufferSize) % BufferSize;
        int idx_neg = (center_idx - k + BufferSize) % BufferSize;
        
        // Weights for d=1 are simply k
        sum += (double)k * (buffer[idx_pos] - buffer[idx_neg]);
    }
    
    // Divide by dt to get derivative in units/second
    return sum / (S2 * dt);
}

/**
 * @brief Apply Soft-Knee Compression (Soft Limiter)
 *
 * Gradually reduces gain as signal approaches 1.0 to prevent hard clipping
 * and force rectification. Uses tanh-based asymptotic compression.
 *
 * @param input Raw normalized force (-infinity to +infinity)
 * @param knee The point [0.1, 1.0] where compression starts
 * @return Compressed force, asymptotically approaching 1.0
 */
inline double apply_soft_limiter(double input, double knee) {
    double abs_input = std::abs(input);
    if (abs_input <= knee) return input;

    // Soft-knee compression using tanh
    // Approaches 1.0 asymptotically
    double range = 1.0 - knee;
    if (range < 0.001) return std::clamp(input, -1.0, 1.0); // Fallback to hard clamp

    double compressed = knee + range * std::tanh((abs_input - knee) / range);
    return (input > 0) ? compressed : -compressed;
}

} // namespace ffb_math

#endif // MATH_UTILS_H
