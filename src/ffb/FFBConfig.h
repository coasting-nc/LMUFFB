#ifndef FFBCONFIG_H
#define FFBCONFIG_H

#include <algorithm>
#include <cmath>

struct GeneralConfig {
    float gain = 1.0f;
    float min_force = 0.0f;
    float wheelbase_max_nm = 15.0f;
    float target_rim_nm = 10.0f;
    bool dynamic_normalization_enabled = false;
    bool auto_load_normalization_enabled = false;

    bool Equals(const GeneralConfig& o) const {
        const float eps = 0.0001f;
        auto is_near = [eps](float a, float b) { return std::abs(a - b) < eps; };
        return is_near(gain, o.gain) &&
               is_near(min_force, o.min_force) &&
               is_near(wheelbase_max_nm, o.wheelbase_max_nm) &&
               is_near(target_rim_nm, o.target_rim_nm) &&
               dynamic_normalization_enabled == o.dynamic_normalization_enabled &&
               auto_load_normalization_enabled == o.auto_load_normalization_enabled;
    }

    void Validate() {
        gain = (std::max)(0.0f, gain);
        wheelbase_max_nm = (std::max)(1.0f, wheelbase_max_nm);
        target_rim_nm = (std::max)(1.0f, target_rim_nm);
        min_force = (std::max)(0.0f, min_force);
    }
};

#endif // FFBCONFIG_H
