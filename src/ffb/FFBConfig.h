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

struct LoadForcesConfig {
    float lat_load_effect = 0.0f;
    int lat_load_transform = 0; // Cast to LoadTransform enum in engine
    float long_load_effect = 0.0f;
    float long_load_smoothing = 0.15f;
    int long_load_transform = 0; // Cast to LoadTransform enum in engine

    bool Equals(const LoadForcesConfig& o, float eps = 0.0001f) const {
        auto is_near = [eps](float a, float b) { return std::abs(a - b) < eps; };
        return is_near(lat_load_effect, o.lat_load_effect) &&
               lat_load_transform == o.lat_load_transform &&
               is_near(long_load_effect, o.long_load_effect) &&
               is_near(long_load_smoothing, o.long_load_smoothing) &&
               long_load_transform == o.long_load_transform;
    }

    void Validate() {
        lat_load_effect = (std::max)(0.0f, (std::min)(2.0f, lat_load_effect));
        lat_load_transform = (std::max)(0, (std::min)(3, lat_load_transform));
        long_load_effect = (std::max)(0.0f, (std::min)(10.0f, long_load_effect));
        long_load_smoothing = (std::max)(0.0f, long_load_smoothing);
        long_load_transform = (std::max)(0, (std::min)(3, long_load_transform));
    }
};

struct GripEstimationConfig {
    float optimal_slip_angle = 0.1f;
    float optimal_slip_ratio = 0.12f;
    float slip_angle_smoothing = 0.002f;
    float chassis_inertia_smoothing = 0.0f;
    bool load_sensitivity_enabled = true;

    float grip_smoothing_steady = 0.05f;
    float grip_smoothing_fast = 0.005f;
    float grip_smoothing_sensitivity = 0.1f;

    bool Equals(const GripEstimationConfig& o, float eps = 0.0001f) const {
        auto is_near = [eps](float a, float b) { return std::abs(a - b) < eps; };
        return is_near(optimal_slip_angle, o.optimal_slip_angle) &&
               is_near(optimal_slip_ratio, o.optimal_slip_ratio) &&
               is_near(slip_angle_smoothing, o.slip_angle_smoothing) &&
               is_near(chassis_inertia_smoothing, o.chassis_inertia_smoothing) &&
               load_sensitivity_enabled == o.load_sensitivity_enabled &&
               is_near(grip_smoothing_steady, o.grip_smoothing_steady) &&
               is_near(grip_smoothing_fast, o.grip_smoothing_fast) &&
               is_near(grip_smoothing_sensitivity, o.grip_smoothing_sensitivity);
    }

    void Validate() {
        optimal_slip_angle = (std::max)(0.01f, optimal_slip_angle);
        optimal_slip_ratio = (std::max)(0.01f, optimal_slip_ratio);
        slip_angle_smoothing = (std::max)(0.0001f, slip_angle_smoothing);
        chassis_inertia_smoothing = (std::max)(0.0f, chassis_inertia_smoothing);
        grip_smoothing_steady = (std::max)(0.0f, grip_smoothing_steady);
        grip_smoothing_fast = (std::max)(0.0f, grip_smoothing_fast);
        grip_smoothing_sensitivity = (std::max)(0.001f, grip_smoothing_sensitivity);
    }
};

struct SlopeDetectionConfig {
    bool enabled = false;
    int sg_window = 15;
    float sensitivity = 0.5f;
    float smoothing_tau = 0.04f;
    float alpha_threshold = 0.02f;
    float decay_rate = 5.0f;
    bool confidence_enabled = true;
    float confidence_max_rate = 0.10f;
    float min_threshold = -0.3f;
    float max_threshold = -2.0f;
    float g_slew_limit = 50.0f;
    bool use_torque = true;
    float torque_sensitivity = 0.5f;

    bool Equals(const SlopeDetectionConfig& o, float eps = 0.0001f) const {
        auto is_near = [eps](float a, float b) { return std::abs(a - b) < eps; };
        return enabled == o.enabled &&
               sg_window == o.sg_window &&
               is_near(sensitivity, o.sensitivity) &&
               is_near(smoothing_tau, o.smoothing_tau) &&
               is_near(alpha_threshold, o.alpha_threshold) &&
               is_near(decay_rate, o.decay_rate) &&
               confidence_enabled == o.confidence_enabled &&
               is_near(confidence_max_rate, o.confidence_max_rate) &&
               is_near(min_threshold, o.min_threshold) &&
               is_near(max_threshold, o.max_threshold) &&
               is_near(g_slew_limit, o.g_slew_limit) &&
               use_torque == o.use_torque &&
               is_near(torque_sensitivity, o.torque_sensitivity);
    }

    void Validate() {
        sg_window = (std::max)(5, (std::min)(41, sg_window));
        if (sg_window % 2 == 0) sg_window++; // Must be odd for SG
        sensitivity = (std::max)(0.1f, sensitivity);
        smoothing_tau = (std::max)(0.001f, smoothing_tau);

        // Use soft limits with reset-to-default for alpha and decay (preserving legacy behavior)
        if (alpha_threshold < 0.001f || alpha_threshold > 0.1f) {
            alpha_threshold = 0.02f;
        }
        if (decay_rate < 0.1f || decay_rate > 20.0f) {
            decay_rate = 5.0f;
        }

        confidence_max_rate = (std::max)(alpha_threshold + 0.01f, confidence_max_rate);
        g_slew_limit = (std::max)(1.0f, g_slew_limit);
        torque_sensitivity = (std::max)(0.01f, torque_sensitivity);
    }
};

struct BrakingConfig {
    bool lockup_enabled = true;
    float lockup_gain = 0.37479f;
    float lockup_start_pct = 1.0f;
    float lockup_full_pct = 5.0f;
    float lockup_rear_boost = 10.0f;
    float lockup_gamma = 0.1f;
    float lockup_prediction_sens = 10.0f;
    float lockup_bump_reject = 0.1f;
    float brake_load_cap = 2.0f;
    float lockup_freq_scale = 1.02f;

    bool abs_pulse_enabled = false;
    float abs_gain = 2.0f;
    float abs_freq = 25.5f;

    bool Equals(const BrakingConfig& o, float eps = 0.0001f) const {
        auto is_near = [eps](float a, float b) { return std::abs(a - b) < eps; };
        return lockup_enabled == o.lockup_enabled &&
               is_near(lockup_gain, o.lockup_gain) &&
               is_near(lockup_start_pct, o.lockup_start_pct) &&
               is_near(lockup_full_pct, o.lockup_full_pct) &&
               is_near(lockup_rear_boost, o.lockup_rear_boost) &&
               is_near(lockup_gamma, o.lockup_gamma) &&
               is_near(lockup_prediction_sens, o.lockup_prediction_sens) &&
               is_near(lockup_bump_reject, o.lockup_bump_reject) &&
               is_near(brake_load_cap, o.brake_load_cap) &&
               is_near(lockup_freq_scale, o.lockup_freq_scale) &&
               abs_pulse_enabled == o.abs_pulse_enabled &&
               is_near(abs_gain, o.abs_gain) &&
               is_near(abs_freq, o.abs_freq);
    }

    void Validate() {
        lockup_gain = (std::max)(0.0f, lockup_gain);
        lockup_start_pct = (std::max)(0.1f, lockup_start_pct);
        lockup_full_pct = (std::max)(0.2f, lockup_full_pct);
        lockup_rear_boost = (std::max)(0.0f, lockup_rear_boost);
        lockup_gamma = (std::max)(0.1f, (std::min)(4.0f, lockup_gamma));
        lockup_prediction_sens = (std::max)(10.0f, (std::min)(100.0f, lockup_prediction_sens));
        lockup_bump_reject = (std::max)(0.01f, (std::min)(5.0f, lockup_bump_reject));
        brake_load_cap = (std::max)(1.0f, (std::min)(10.0f, brake_load_cap));
        lockup_freq_scale = (std::max)(0.1f, lockup_freq_scale);
        abs_gain = (std::max)(0.0f, (std::min)(10.0f, abs_gain));
        abs_freq = (std::max)(1.0f, abs_freq);
    }
};

struct VibrationConfig {
    float vibration_gain = 1.0f;
    float texture_load_cap = 1.5f;

    bool slide_enabled = false;
    float slide_gain = 0.226562f;
    float slide_freq = 1.0f;

    bool road_enabled = true;
    float road_gain = 0.0f;

    bool spin_enabled = true;
    float spin_gain = 0.5f;
    float spin_freq_scale = 1.0f;

    float scrub_drag_gain = 0.0f;

    bool bottoming_enabled = true;
    float bottoming_gain = 1.0f;
    int bottoming_method = 0;

    bool Equals(const VibrationConfig& o, float eps = 0.0001f) const {
        auto is_near = [eps](float a, float b) { return std::abs(a - b) < eps; };
        return is_near(vibration_gain, o.vibration_gain) &&
               is_near(texture_load_cap, o.texture_load_cap) &&
               slide_enabled == o.slide_enabled &&
               is_near(slide_gain, o.slide_gain) &&
               is_near(slide_freq, o.slide_freq) &&
               road_enabled == o.road_enabled &&
               is_near(road_gain, o.road_gain) &&
               spin_enabled == o.spin_enabled &&
               is_near(spin_gain, o.spin_gain) &&
               is_near(spin_freq_scale, o.spin_freq_scale) &&
               is_near(scrub_drag_gain, o.scrub_drag_gain) &&
               bottoming_enabled == o.bottoming_enabled &&
               is_near(bottoming_gain, o.bottoming_gain) &&
               bottoming_method == o.bottoming_method;
    }

    void Validate() {
        vibration_gain = (std::max)(0.0f, (std::min)(2.0f, vibration_gain));
        texture_load_cap = (std::max)(1.0f, (std::min)(10.0f, texture_load_cap));
        slide_gain = (std::max)(0.0f, (std::min)(2.0f, slide_gain));
        slide_freq = (std::max)(0.1f, (std::min)(5.0f, slide_freq));
        road_gain = (std::max)(0.0f, (std::min)(2.0f, road_gain));
        spin_gain = (std::max)(0.0f, (std::min)(2.0f, spin_gain));
        spin_freq_scale = (std::max)(0.1f, (std::min)(5.0f, spin_freq_scale));
        scrub_drag_gain = (std::max)(0.0f, (std::min)(1.0f, scrub_drag_gain));
        bottoming_gain = (std::max)(0.0f, (std::min)(2.0f, bottoming_gain));
        bottoming_method = (std::max)(0, (std::min)(1, bottoming_method));
    }
};

struct AdvancedConfig {
    float gyro_gain = 0.0f;
    float gyro_smoothing = 0.0f;
    float stationary_damping = 1.0f;
    bool soft_lock_enabled = true;
    float soft_lock_stiffness = 20.0f;
    float soft_lock_damping = 0.5f;
    float speed_gate_lower = 1.0f;
    float speed_gate_upper = 5.0f;
    bool rest_api_enabled = false;
    int rest_api_port = 6397;
    float road_fallback_scale = 0.05f;
    bool understeer_affects_sop = false;

    bool Equals(const AdvancedConfig& o, float eps = 0.0001f) const {
        auto is_near = [eps](float a, float b) { return std::abs(a - b) < eps; };
        return is_near(gyro_gain, o.gyro_gain) &&
               is_near(gyro_smoothing, o.gyro_smoothing) &&
               is_near(stationary_damping, o.stationary_damping) &&
               soft_lock_enabled == o.soft_lock_enabled &&
               is_near(soft_lock_stiffness, o.soft_lock_stiffness) &&
               is_near(soft_lock_damping, o.soft_lock_damping) &&
               is_near(speed_gate_lower, o.speed_gate_lower) &&
               is_near(speed_gate_upper, o.speed_gate_upper) &&
               rest_api_enabled == o.rest_api_enabled &&
               rest_api_port == o.rest_api_port &&
               is_near(road_fallback_scale, o.road_fallback_scale) &&
               understeer_affects_sop == o.understeer_affects_sop;
    }

    void Validate() {
        gyro_gain = (std::max)(0.0f, (std::min)(1.0f, gyro_gain));
        gyro_smoothing = (std::max)(0.0f, gyro_smoothing);
        stationary_damping = (std::max)(0.0f, (std::min)(1.0f, stationary_damping));
        soft_lock_stiffness = (std::max)(0.0f, soft_lock_stiffness);
        soft_lock_damping = (std::max)(0.0f, soft_lock_damping);
        speed_gate_lower = (std::max)(0.0f, speed_gate_lower);
        speed_gate_upper = (std::max)(0.1f, speed_gate_upper);
        rest_api_port = (std::max)(1, rest_api_port);
        road_fallback_scale = (std::max)(0.0f, road_fallback_scale);
    }
};

struct SafetyConfig {
    float window_duration = 0.0f;
    float gain_reduction = 0.3f;
    float smoothing_tau = 0.2f;
    float spike_detection_threshold = 500.0f;
    float immediate_spike_threshold = 1500.0f;
    float slew_full_scale_time_s = 1.0f;
    bool stutter_safety_enabled = false;
    float stutter_threshold = 1.5f;

    bool Equals(const SafetyConfig& o, float eps = 0.0001f) const {
        auto is_near = [eps](float a, float b) { return std::abs(a - b) < eps; };
        return is_near(window_duration, o.window_duration) &&
               is_near(gain_reduction, o.gain_reduction) &&
               is_near(smoothing_tau, o.smoothing_tau) &&
               is_near(spike_detection_threshold, o.spike_detection_threshold) &&
               is_near(immediate_spike_threshold, o.immediate_spike_threshold) &&
               is_near(slew_full_scale_time_s, o.slew_full_scale_time_s) &&
               stutter_safety_enabled == o.stutter_safety_enabled &&
               is_near(stutter_threshold, o.stutter_threshold);
    }

    void Validate() {
        window_duration = (std::max)(0.0f, window_duration);
        gain_reduction = (std::max)(0.0f, (std::min)(1.0f, gain_reduction));
        smoothing_tau = (std::max)(0.001f, smoothing_tau);
        spike_detection_threshold = (std::max)(1.0f, spike_detection_threshold);
        immediate_spike_threshold = (std::max)(1.0f, immediate_spike_threshold);
        slew_full_scale_time_s = (std::max)(0.01f, slew_full_scale_time_s);
        stutter_threshold = (std::max)(1.01f, stutter_threshold);
    }
};

#endif // FFBCONFIG_H
