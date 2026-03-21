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

    bool Equals(const GeneralConfig& o, float eps = 0.0001f) const {
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

struct FrontAxleConfig {
    float steering_shaft_gain = 1.0f;
    float ingame_ffb_gain = 1.0f;
    float steering_shaft_smoothing = 0.0f;
    float understeer_effect = 1.0f;
    float understeer_gamma = 1.0f;
    int torque_source = 0;
    int steering_100hz_reconstruction = 0;
    bool torque_passthrough = false;

    // Signal Filtering
    bool flatspot_suppression = false;
    float notch_q = 2.0f;
    float flatspot_strength = 1.0f;
    bool static_notch_enabled = false;
    float static_notch_freq = 11.0f;
    float static_notch_width = 2.0f;

    bool Equals(const FrontAxleConfig& o, float eps = 0.0001f) const {
        auto is_near = [eps](float a, float b) { return std::abs(a - b) < eps; };
        return is_near(steering_shaft_gain, o.steering_shaft_gain) &&
               is_near(ingame_ffb_gain, o.ingame_ffb_gain) &&
               is_near(steering_shaft_smoothing, o.steering_shaft_smoothing) &&
               is_near(understeer_effect, o.understeer_effect) &&
               is_near(understeer_gamma, o.understeer_gamma) &&
               torque_source == o.torque_source &&
               steering_100hz_reconstruction == o.steering_100hz_reconstruction &&
               torque_passthrough == o.torque_passthrough &&
               flatspot_suppression == o.flatspot_suppression &&
               is_near(notch_q, o.notch_q) &&
               is_near(flatspot_strength, o.flatspot_strength) &&
               static_notch_enabled == o.static_notch_enabled &&
               is_near(static_notch_freq, o.static_notch_freq) &&
               is_near(static_notch_width, o.static_notch_width);
    }

    void Validate() {
        steering_shaft_gain = (std::max)(0.0f, (std::min)(2.0f, steering_shaft_gain));
        ingame_ffb_gain = (std::max)(0.0f, (std::min)(2.0f, ingame_ffb_gain));
        steering_shaft_smoothing = (std::max)(0.0f, steering_shaft_smoothing);
        understeer_effect = (std::max)(0.0f, (std::min)(2.0f, understeer_effect));
        understeer_gamma = (std::max)(0.1f, (std::min)(4.0f, understeer_gamma));
        torque_source = (std::max)(0, (std::min)(1, torque_source));
        steering_100hz_reconstruction = (std::max)(0, (std::min)(1, steering_100hz_reconstruction));
        notch_q = (std::max)(0.1f, notch_q);
        flatspot_strength = (std::max)(0.0f, (std::min)(1.0f, flatspot_strength));
        static_notch_freq = (std::max)(1.0f, static_notch_freq);
        static_notch_width = (std::max)(0.1f, static_notch_width);
    }
};

struct RearAxleConfig {
    float oversteer_boost = 2.52101f;
    float sop_effect = 1.666f;
    float sop_scale = 1.0f;
    float sop_smoothing_factor = 0.0f;
    float rear_align_effect = 0.666f;
    float kerb_strike_rejection = 0.0f;

    // General Yaw Kick
    float sop_yaw_gain = 0.333f;
    float yaw_kick_threshold = 0.0f;
    float yaw_accel_smoothing = 0.001f;

    // Unloaded Yaw Kick (Braking/Lift-off)
    float unloaded_yaw_gain = 0.0f;
    float unloaded_yaw_threshold = 0.2f;
    float unloaded_yaw_sens = 1.0f;
    float unloaded_yaw_gamma = 0.5f;
    float unloaded_yaw_punch = 0.05f;

    // Power Yaw Kick (Acceleration)
    float power_yaw_gain = 0.0f;
    float power_yaw_threshold = 0.2f;
    float power_slip_threshold = 0.10f;
    float power_yaw_gamma = 0.5f;
    float power_yaw_punch = 0.05f;

    bool Equals(const RearAxleConfig& o, float eps = 0.0001f) const {
        auto is_near = [eps](float a, float b) { return std::abs(a - b) < eps; };
        return is_near(oversteer_boost, o.oversteer_boost) &&
               is_near(sop_effect, o.sop_effect) &&
               is_near(sop_scale, o.sop_scale) &&
               is_near(sop_smoothing_factor, o.sop_smoothing_factor) &&
               is_near(rear_align_effect, o.rear_align_effect) &&
               is_near(kerb_strike_rejection, o.kerb_strike_rejection) &&
               is_near(sop_yaw_gain, o.sop_yaw_gain) &&
               is_near(yaw_kick_threshold, o.yaw_kick_threshold) &&
               is_near(yaw_accel_smoothing, o.yaw_accel_smoothing) &&
               is_near(unloaded_yaw_gain, o.unloaded_yaw_gain) &&
               is_near(unloaded_yaw_threshold, o.unloaded_yaw_threshold) &&
               is_near(unloaded_yaw_sens, o.unloaded_yaw_sens) &&
               is_near(unloaded_yaw_gamma, o.unloaded_yaw_gamma) &&
               is_near(unloaded_yaw_punch, o.unloaded_yaw_punch) &&
               is_near(power_yaw_gain, o.power_yaw_gain) &&
               is_near(power_yaw_threshold, o.power_yaw_threshold) &&
               is_near(power_slip_threshold, o.power_slip_threshold) &&
               is_near(power_yaw_gamma, o.power_yaw_gamma) &&
               is_near(power_yaw_punch, o.power_yaw_punch);
    }

    void Validate() {
        oversteer_boost = (std::max)(0.0f, (std::min)(4.0f, oversteer_boost));
        sop_effect = (std::max)(0.0f, (std::min)(2.0f, sop_effect));
        sop_scale = (std::max)(0.01f, sop_scale);
        sop_smoothing_factor = (std::max)(0.0f, (std::min)(1.0f, sop_smoothing_factor));
        rear_align_effect = (std::max)(0.0f, (std::min)(2.0f, rear_align_effect));
        kerb_strike_rejection = (std::max)(0.0f, (std::min)(1.0f, kerb_strike_rejection));
        sop_yaw_gain = (std::max)(0.0f, (std::min)(1.0f, sop_yaw_gain));
        yaw_kick_threshold = (std::max)(0.0f, yaw_kick_threshold);
        yaw_accel_smoothing = (std::max)(0.0f, yaw_accel_smoothing);
        unloaded_yaw_gain = (std::max)(0.0f, (std::min)(1.0f, unloaded_yaw_gain));
        unloaded_yaw_threshold = (std::max)(0.0f, unloaded_yaw_threshold);
        unloaded_yaw_sens = (std::max)(0.1f, unloaded_yaw_sens);
        unloaded_yaw_gamma = (std::max)(0.1f, (std::min)(4.0f, unloaded_yaw_gamma));
        unloaded_yaw_punch = (std::max)(0.0f, (std::min)(1.0f, unloaded_yaw_punch));
        power_yaw_gain = (std::max)(0.0f, (std::min)(1.0f, power_yaw_gain));
        power_yaw_threshold = (std::max)(0.0f, power_yaw_threshold);
        power_slip_threshold = (std::max)(0.01f, (std::min)(1.0f, power_slip_threshold));
        power_yaw_gamma = (std::max)(0.1f, (std::min)(4.0f, power_yaw_gamma));
        power_yaw_punch = (std::max)(0.0f, (std::min)(1.0f, power_yaw_punch));
    }
};

#endif // FFBCONFIG_H
