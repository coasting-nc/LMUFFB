#ifndef DATASANITIZER_H
#define DATASANITIZER_H

#include <cmath>
#include <limits>
#include <algorithm>

class DataSanitizer {
public:
    struct ValidationResult {
        bool valid;
        double clamped_value;
        bool was_clamped;
        bool was_nan;
        bool was_inf;
    };

    static constexpr double MIN_TIRE_LOAD = 0.0;
    static constexpr double MAX_TIRE_LOAD = 15000.0;
    static constexpr double MIN_GRIP = 0.0;
    static constexpr double MAX_GRIP = 1.5;
    static constexpr double MIN_VELOCITY = 0.0;
    static constexpr double MAX_VELOCITY = 150.0;
    static constexpr double MIN_ACCEL = -50.0;
    static constexpr double MAX_ACCEL = 50.0;
    static constexpr double MIN_STEERING = -3.14159;
    static constexpr double MAX_STEERING = 3.14159;
    static constexpr double MIN_PRESSURE = 0.0;
    static constexpr double MAX_PRESSURE = 700.0;
    static constexpr double MIN_ROTATION = 0.0;
    static constexpr double MAX_ROTATION = 500.0;
    static constexpr double MIN_RADIUS = 0.1;
    static constexpr double MAX_RADIUS = 1.0;

    static bool IsNaN(double val) {
        return std::isnan(val);
    }

    static bool IsInf(double val) {
        return std::isinf(val);
    }

    static bool IsFinite(double val) {
        return std::isfinite(val);
    }

    static bool IsInRange(double val, double min_val, double max_val) {
        return val >= min_val && val <= max_val;
    }

    static ValidationResult SanitizeDouble(double input, double min_val, double max_val, double fallback = 0.0) {
        ValidationResult result = {};
        result.valid = true;
        result.clamped_value = fallback;
        result.was_clamped = false;
        result.was_nan = false;
        result.was_inf = false;

        if (IsNaN(input)) {
            result.valid = false;
            result.was_nan = true;
            result.clamped_value = fallback;
            return result;
        }

        if (IsInf(input)) {
            result.valid = false;
            result.was_inf = true;
            result.clamped_value = fallback;
            return result;
        }

        if (input < min_val) {
            result.clamped_value = min_val;
            result.was_clamped = true;
        } else if (input > max_val) {
            result.clamped_value = max_val;
            result.was_clamped = true;
        } else {
            result.clamped_value = input;
        }

        return result;
    }

    static double ClampDouble(double input, double min_val, double max_val) {
        return (std::max)(min_val, (std::min)(input, max_val));
    }

    static ValidationResult SanitizeTireLoad(double input, double fallback = 1000.0) {
        return SanitizeDouble(input, MIN_TIRE_LOAD, MAX_TIRE_LOAD, fallback);
    }

    static ValidationResult SanitizeGrip(double input, double fallback = 1.0) {
        return SanitizeDouble(input, MIN_GRIP, MAX_GRIP, fallback);
    }

    static ValidationResult SanitizeVelocity(double input, double fallback = 0.0) {
        return SanitizeDouble(input, MIN_VELOCITY, MAX_VELOCITY, fallback);
    }

    static ValidationResult SanitizeAcceleration(double input, double fallback = 0.0) {
        return SanitizeDouble(input, MIN_ACCEL, MAX_ACCEL, fallback);
    }

    static ValidationResult SanitizeSteering(double input, double fallback = 0.0) {
        return SanitizeDouble(input, MIN_STEERING, MAX_STEERING, fallback);
    }

    static ValidationResult SanitizeBrakePressure(double input, double fallback = 0.0) {
        return SanitizeDouble(input, MIN_PRESSURE, MAX_PRESSURE, fallback);
    }

    static ValidationResult SanitizeRotation(double input, double fallback = 0.0) {
        return SanitizeDouble(input, MIN_ROTATION, MAX_ROTATION, fallback);
    }

    static ValidationResult SanitizeRadius(double input, double fallback = 0.33) {
        return SanitizeDouble(input, MIN_RADIUS, MAX_RADIUS, fallback);
    }

    static double PercentInRange(double val, double min_val, double max_val) {
        if (max_val <= min_val) return 1.0;
        double range = max_val - min_val;
        double normalized = (val - min_val) / range;
        return (std::max)(0.0, (std::min)(1.0, normalized));
    }

    static double RemapRange(double val, double in_min, double in_max, double out_min, double out_max) {
        if (in_max <= in_min) return out_min;
        double normalized = (val - in_min) / (in_max - in_min);
        normalized = (std::max)(0.0, (std::min)(1.0, normalized));
        return out_min + normalized * (out_max - out_min);
    }

    static double Deadzone(double val, double threshold) {
        if (std::abs(val) < threshold) return 0.0;
        return val;
    }

    static double ApplyHysteresis(double current, double previous, double threshold, double target) {
        if (std::abs(current - previous) < threshold) {
            return previous;
        }
        if (current > previous + threshold) {
            return (std::min)(current, target);
        }
        return (std::max)(current, target);
    }

    static double MovingAverage3(const double* buffer) {
        return (buffer[0] + buffer[1] + buffer[2]) / 3.0;
    }

    static double MovingAverage5(const double* buffer) {
        return (buffer[0] + buffer[1] + buffer[2] + buffer[3] + buffer[4]) / 5.0;
    }

    static double Median3(double a, double b, double c) {
        double max_val = (std::max)((std::max)(a, b), c);
        double min_val = (std::min)((std::min)(a, b), c);
        if (a >= min_val && a <= max_val) return a;
        if (b >= min_val && b <= max_val) return b;
        return c;
    }

    static double WienerFilter(double signal, double noise_var, double signal_var) {
        if (signal_var <= 0.0) return signal;
        double snr = signal_var / noise_var;
        double weight = snr / (snr + 1.0);
        return signal * weight;
    }
};

#endif
