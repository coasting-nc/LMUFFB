import os
import re

# 1. MathUtils.h
with open('src/utils/MathUtils.h', 'w') as f:
    f.write('''#ifndef MATH_UTILS_H
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
 */
struct BiquadNotch {
    double b0 = 0.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
    double x1 = 0.0, x2 = 0.0;
    double y1 = 0.0, y2 = 0.0;

    void Update(double center_freq, double sample_rate, double Q) {
        center_freq = (std::max)(1.0, (std::min)(center_freq, sample_rate * 0.49));
        double omega = 2.0 * PI * center_freq / sample_rate;
        double sn = std::sin(omega);
        double cs = std::cos(omega);
        double alpha = sn / (2.0 * Q);
        double a0 = 1.0 + alpha;
        b0 = 1.0 / a0; b1 = (-2.0 * cs) / a0; b2 = 1.0 / a0;
        a1 = (-2.0 * cs) / a0; a2 = (1.0 - alpha) / a0;
    }

    double Process(double in) {
        double out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = in; y2 = y1; y1 = out;
        return out;
    }
    void Reset() { x1 = x2 = y1 = y2 = 0.0; }
};

inline double inverse_lerp(double min_val, double max_val, double value) {
    double range = max_val - min_val;
    if (std::abs(range) >= 0.0001) {
        double t = (value - min_val) / range;
        return (std::max)(0.0, (std::min)(1.0, t));
    }
    if (max_val >= min_val) return (value >= min_val) ? 1.0 : 0.0;
    return (value <= min_val) ? 1.0 : 0.0;
}

inline double smoothstep(double edge0, double edge1, double x) {
    double range = edge1 - edge0;
    if (std::abs(range) >= 0.0001) {
        double t = (x - edge0) / range;
        t = (std::max)(0.0, (std::min)(1.0, t));
        return t * t * (3.0 - 2.0 * t);
    }
    return (x < edge0) ? 0.0 : 1.0;
}

inline double apply_slew_limiter(double input, double& prev_val, double limit, double dt) {
    double delta = input - prev_val;
    double max_change = limit * dt;
    delta = std::clamp(delta, -max_change, max_change);
    prev_val += delta;
    return prev_val;
}

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

inline double apply_load_transform_cubic(double x) { return 1.5 * x - 0.5 * (x * x * x); }
inline double apply_load_transform_quadratic(double x) { return 2.0 * x - x * std::abs(x); }
inline double apply_load_transform_hermite(double x) {
    double abs_x = std::abs(x);
    return x * (1.0 + abs_x - (abs_x * abs_x));
}

template <size_t BufferSize>
inline double calculate_sg_derivative(const std::array<double, BufferSize>& buffer,
                            const int& buffer_count, const int& window, double dt, const int& buffer_index) {
    if (buffer_count < window) return 0.0;
    int M = window / 2;
    double S2 = (double)M * (M + 1.0) * (2.0 * M + 1.0) / 3.0;
    int latest_idx = (buffer_index - 1 + BufferSize) % BufferSize;
    int center_idx = (latest_idx - M + BufferSize) % BufferSize;
    double sum = 0.0;
    for (int k = 1; k <= M; ++k) {
        int idx_pos = (center_idx + k + BufferSize) % BufferSize;
        int idx_neg = (center_idx - k + BufferSize) % BufferSize;
        sum += (double)k * (buffer[idx_pos] - buffer[idx_neg]);
    }
    return sum / (S2 * dt);
}

/**
 * @brief Linear Interpolator (Inter-Frame Reconstruction)
 */
class LinearExtrapolator {
private:
    double m_prev_sample = 0.0, m_target_sample = 0.0, m_current_output = 0.0;
    double m_rate = 0.0, m_time_since_update = 0.0, m_game_tick = 0.01;
    bool m_initialized = false;
public:
    void Configure(double game_tick) { m_game_tick = (std::max)(0.0001, game_tick); }
    double Process(double raw_input, double dt, bool is_new_frame) {
        if (!m_initialized) {
            m_prev_sample = m_target_sample = m_current_output = raw_input;
            m_initialized = true; return raw_input;
        }
        if (is_new_frame) {
            m_prev_sample = m_target_sample;
            m_target_sample = raw_input;
            m_rate = (m_target_sample - m_prev_sample) / m_game_tick;
            m_time_since_update = 0.0;
            m_current_output = m_prev_sample;
        } else {
            m_time_since_update += dt;
            if (m_time_since_update <= m_game_tick)
                m_current_output = m_prev_sample + m_rate * m_time_since_update;
            else m_current_output = m_target_sample;
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
 */
class HoltWintersFilter {
private:
    double m_level = 0.0, m_prev_level = 0.0, m_trend = 0.0, m_time_since_update = 0.0;
    bool m_initialized = false, m_zero_latency = true;
    double m_alpha = 0.8, m_beta = 0.2, m_game_tick = 0.01;
public:
    void Configure(double alpha, double beta, double game_tick = 0.01) {
        m_alpha = std::clamp(alpha, 0.01, 1.0);
        m_beta = std::clamp(beta, 0.01, 1.0);
        m_game_tick = (std::max)(0.0001, game_tick);
    }
    void SetZeroLatency(bool zero_latency) { m_zero_latency = zero_latency; }
    double Process(double raw_input, double dt, bool is_new_frame) {
        if (!m_initialized) {
            m_level = m_prev_level = raw_input;
            m_trend = 0.0; m_time_since_update = 0.0;
            m_initialized = true; return raw_input;
        }
        if (is_new_frame) {
            double old_level = m_level;
            m_prev_level = m_level;
            m_level = m_alpha * raw_input + (1.0 - m_alpha) * (m_level + m_trend * m_game_tick);
            m_trend = m_beta * ((m_level - old_level) / m_game_tick) + (1.0 - m_beta) * m_trend;
            m_time_since_update = 0.0;
            return m_zero_latency ? m_level : m_prev_level;
        } else {
            m_time_since_update += dt;
        }
        if (m_zero_latency) return m_level + m_trend * m_time_since_update;
        if (m_time_since_update <= m_game_tick) {
            double t = m_time_since_update / m_game_tick;
            return m_prev_level + t * (m_level - m_prev_level);
        }
        return m_level;
    }
    void Reset() { m_level = m_prev_level = m_trend = m_time_since_update = 0.0; m_initialized = false; }
};

} // namespace ffb_math

#endif // MATH_UTILS_H
''')

# 2. FFBEngine.h
with open('src/ffb/FFBEngine.h', 'r') as f:
    content = f.read()
if 'm_steering_100hz_reconstruction' not in content:
    content = content.replace('int m_torque_source = 0;', 'int m_torque_source = 0;\n    int m_steering_100hz_reconstruction = 0; // NEW: 0 = Zero Latency, 1 = Smooth')
with open('src/ffb/FFBEngine.h', 'w') as f:
    f.write(content)

# 3. FFBEngine.cpp
with open('src/ffb/FFBEngine.cpp', 'r') as f:
    content = f.read()
if 'm_upsample_shaft_torque.SetZeroLatency' not in content:
    content = re.sub(r'(double shaft_torque = m_upsample_shaft_torque\.Process\([^)]*\);)',
                     r'm_upsample_shaft_torque.SetZeroLatency(m_steering_100hz_reconstruction == 0);\n    \1', content)
with open('src/ffb/FFBEngine.cpp', 'w') as f:
    f.write(content)

# 4. Config.h
with open('src/core/Config.h', 'r') as f:
    content = f.read()
if 'steering_100hz_reconstruction' not in content:
    content = content.replace('int torque_source = 0;', 'int torque_source = 0;\n    int steering_100hz_reconstruction = 0;')
    content = content.replace('engine.m_torque_source = torque_source;', 'engine.m_torque_source = torque_source;\n        engine.m_steering_100hz_reconstruction = steering_100hz_reconstruction;')
    content = content.replace('torque_source = engine.m_torque_source;', 'torque_source = engine.m_torque_source;\n        steering_100hz_reconstruction = engine.m_steering_100hz_reconstruction;')
    content = content.replace('if (torque_source != p.torque_source) return false;', 'if (torque_source != p.torque_source) return false;\n        if (steering_100hz_reconstruction != p.steering_100hz_reconstruction) return false;')
with open('src/core/Config.h', 'w') as f:
    f.write(content)

# 5. Config.cpp
with open('src/core/Config.cpp', 'r') as f:
    content = f.read()
if 'steering_100hz_reconstruction' not in content:
    content = content.replace('if (key == "torque_source") { current_preset.torque_source = std::stoi(value); return true; }',
                              'if (key == "torque_source") { current_preset.torque_source = std::stoi(value); return true; }\n    if (key == "steering_100hz_reconstruction") { current_preset.steering_100hz_reconstruction = std::stoi(value); return true; }')
    content = content.replace('if (key == "torque_source") { engine.m_torque_source = std::stoi(value); return true; }',
                              'if (key == "torque_source") { engine.m_torque_source = std::stoi(value); return true; }\n    if (key == "steering_100hz_reconstruction") { engine.m_steering_100hz_reconstruction = std::stoi(value); return true; }')
    content = content.replace('file << "torque_source=" << p.torque_source << "\\n";',
                              'file << "torque_source=" << p.torque_source << "\\n";\n    file << "steering_100hz_reconstruction=" << p.steering_100hz_reconstruction << "\\n";')
    content = content.replace('file << "torque_source=" << engine.m_torque_source << "\\n";',
                              'file << "torque_source=" << engine.m_torque_source << "\\n";\n        file << "steering_100hz_reconstruction=" << engine.m_steering_100hz_reconstruction << "\\n";')
with open('src/core/Config.cpp', 'w') as f:
    f.write(content)

# 6. GuiLayer_Common.cpp
with open('src/gui/GuiLayer_Common.cpp', 'r') as f:
    content = f.read()
insertion = '''        IntSetting("Torque Source", &engine.m_torque_source, torque_sources, sizeof(torque_sources)/sizeof(torque_sources[0]),
            Tooltips::TORQUE_SOURCE);

        // NEW: Show reconstruction mode only if using the 100Hz source
        if (engine.m_torque_source == 0) {
            const char* recon_modes[] = { "Zero-Latency (Extrapolated)", "Smooth (Interpolated)" };
            IntSetting("  100Hz Reconstruction", &engine.m_steering_100hz_reconstruction, recon_modes, 2,
                "Zero-Latency: Best for response but can be grainy. Smooth: 10ms delay but eliminates all 100Hz buzz.");
        }'''
if '100Hz Reconstruction' not in content:
    content = re.sub(r'IntSetting\("Torque Source", &engine\.m_torque_source, torque_sources, sizeof\(torque_sources\)/sizeof\(torque_sources\[0\]\),\s+Tooltips::TORQUE_SOURCE\);', insertion, content)
with open('src/gui/GuiLayer_Common.cpp', 'w') as f:
    f.write(content)
