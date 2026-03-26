#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <cmath>
#include <algorithm>
#include <array>

namespace LMUFFB {
namespace Utils {

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
        // NOTE: The ternary below is redundant but kept for safety.
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
        // NOTE: The ternary below is redundant but kept for safety.
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

/**
 * @brief Cubic S-Curve transformation for Load (Lateral or Longitudinal)
 * f(x) = 1.5x - 0.5x^3
 */
inline double apply_load_transform_cubic(double x) {
    return 1.5 * x - 0.5 * (x * x * x);
}

/**
 * @brief Quadratic (Signed) transformation for Load (Lateral or Longitudinal)
 * f(x) = 2x - x|x|
 */
inline double apply_load_transform_quadratic(double x) {
    return 2.0 * x - x * std::abs(x);
}

/**
 * @brief Locked-Center Hermite Spline transformation for Load (Lateral or Longitudinal)
 * f(x) = x * (1 + |x| - x^2)
 */
inline double apply_load_transform_hermite(double x) {
    double abs_x = std::abs(x);
    return x * (1.0 + abs_x - (abs_x * abs_x));
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
 * @brief Linear Interpolator (Inter-Frame Reconstruction)
 *
 * Upsamples a 100Hz signal to 400Hz+ by delaying the signal by 1 frame (10ms)
 * and smoothly interpolating. Eliminates sawtooth artifacts and derivative spikes.
 */
class LinearExtrapolator { // Kept name to avoid breaking other files
private:
    double m_prev_sample = 0.0;
    double m_target_sample = 0.0;
    double m_current_output = 0.0;
    double m_rate = 0.0;
    double m_time_since_update = 0.0;
    double m_game_tick = 0.01; // Default 100Hz
    bool m_initialized = false;

public:
    void Configure(double game_tick) {
        m_game_tick = (std::max)(0.0001, game_tick);
    }

    double Process(double raw_input, double dt, bool is_new_frame) {
        if (!m_initialized) {
            m_prev_sample = raw_input;
            m_target_sample = raw_input;
            m_current_output = raw_input;
            m_initialized = true;
            return raw_input;
        }

        if (is_new_frame) {
            // Shift the window: old target becomes the new start point
            m_prev_sample = m_target_sample;
            m_target_sample = raw_input;

            // Calculate rate to reach the new target over the next game tick
            m_rate = (m_target_sample - m_prev_sample) / m_game_tick;
            m_time_since_update = 0.0;

            // Output starts exactly at the previous sample (no snapping)
            m_current_output = m_prev_sample;
        } else {
            m_time_since_update += dt;
            if (m_time_since_update <= m_game_tick) {
                m_current_output = m_prev_sample + m_rate * m_time_since_update;
            } else {
                // If the game stutters/drops a frame, hold the target value safely
                m_current_output = m_target_sample;
            }
        }

        return m_current_output;
    }

    void Reset() {
        m_prev_sample = m_target_sample = m_current_output = m_rate = m_time_since_update = 0.0;
        m_initialized = false;
    }
};

/**
 * @brief Second-Order Holt-Winters (Double Exponential Smoothing)
 *
 * Tracks both the value and the trend (velocity) of a signal.
 * Acts as both an upsampler and a low-pass filter.
 */
class HoltWintersFilter {
private:
    double m_level = 0.0;      // Smoothed value (Target for interpolation)
    double m_prev_level = 0.0; // Previous smoothed value (Start for interpolation)
    double m_trend = 0.0;      // Smoothed trend/slope
    double m_time_since_update = 0.0;
    bool m_initialized = false;
    bool m_zero_latency = true; // Mode toggle

    // Time-Awareness & Safety State
    double m_accumulated_time = 0.01; // Tracks actual time between 100Hz frames
    double m_trend_damping = 0.95;    // Multiplier applied to trend during extrapolation

    // Tuning Parameters
    double m_alpha = 0.8; // Level weight (Higher = less lag)
    double m_beta = 0.2;  // Trend weight
    double m_game_tick = 0.01; // Default 100Hz

public:
    void Configure(double alpha, double beta, double game_tick = 0.01) {
        m_alpha = std::clamp(alpha, 0.01, 1.0);
        m_beta = std::clamp(beta, 0.0, 1.0);
        m_game_tick = (std::max)(0.0001, game_tick);
    }

    void SetZeroLatency(bool zero_latency) {
        m_zero_latency = zero_latency;
    }

    double GetAlpha() const { return m_alpha; }
    double GetBeta() const { return m_beta; }
    bool GetZeroLatency() const { return m_zero_latency; }

    double Process(double raw_input, double dt, bool is_new_frame) {
        if (!m_initialized) {
            m_level = raw_input;
            m_prev_level = raw_input;
            m_trend = 0.0;
            m_time_since_update = 0.0;
            m_accumulated_time = 0.0;
            m_initialized = true;
            return raw_input;
        }

        m_time_since_update += dt;
        m_accumulated_time += dt;

        if (is_new_frame) {
            double old_level = m_level;
            m_prev_level = m_level; // Save for interpolation start point

            // Time-Aware Update: Use actual measured interval for derivative logic
            // Divide by zero protection (dt should be > 0 on new frame)
            double interval = (m_accumulated_time > 1e-6) ? m_accumulated_time : m_game_tick;

            // CRITICAL SAFETY: Clamp interval to sensible bounds [1ms, 50ms]
            // Prevents massive game stutters from corrupting future derivative math
            interval = std::clamp(interval, 0.001, 0.050);

            // Update Level: Balance between the raw input and our previous prediction
            m_level = m_alpha * raw_input + (1.0 - m_alpha) * (m_level + m_trend * interval);

            // Update Trend: Balance between the new observed slope and the old trend
            m_trend = m_beta * ((m_level - old_level) / interval) + (1.0 - m_beta) * m_trend;

            // Dynamically update game tick for sub-frame interpolation accuracy
            m_game_tick = interval;

            m_time_since_update = 0.0;
            m_accumulated_time = 0.0;

            if (m_zero_latency) {
                return m_level;
            } else {
                return m_prev_level; // Start interpolation from previous level
            }
        } else {
            // Trend Damping: Safely arrest runaway extrapolation during dropped frames
            m_trend *= m_trend_damping;
        }

        if (m_zero_latency) {
            // Predict current state based on previous trend (Extrapolation)
            return m_level + m_trend * m_time_since_update;
        } else {
            // Smoothly blend between prev_level and level (Interpolation)
            if (m_time_since_update <= m_game_tick) {
                double t = m_time_since_update / m_game_tick;
                return m_prev_level + t * (m_level - m_prev_level);
            } else {
                return m_level; // Hold target if game stutters
            }
        }
    }

    void Reset() {
        m_level = m_prev_level = m_trend = m_time_since_update = 0.0;
        m_accumulated_time = 0.01;
        m_initialized = false;
    }
};

} // namespace Utils

// Temporary bridge for legacy code
using Utils::PI;
using Utils::TWO_PI;
using Utils::BiquadNotch;
using Utils::inverse_lerp;
using Utils::smoothstep;
using Utils::apply_slew_limiter;
using Utils::apply_adaptive_smoothing;
using Utils::apply_load_transform_cubic;
using Utils::apply_load_transform_quadratic;
using Utils::apply_load_transform_hermite;
using Utils::calculate_sg_derivative;
using Utils::LinearExtrapolator;
using Utils::HoltWintersFilter;

} // namespace LMUFFB

#endif // MATH_UTILS_H
