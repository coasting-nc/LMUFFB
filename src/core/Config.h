#ifndef CONFIG_H
#define CONFIG_H

#include "FFBEngine.h"
#include "ffb/FFBConfig.h"
#include <string>
#include <vector>
#include <chrono>
#include <map>
#include <atomic>
#include "Version.h"

struct Preset {
    std::string name;
    bool is_builtin = false;
    std::string app_version = LMUFFB_VERSION; // NEW: Track if this is hardcoded or user-created
    
    // 1. SINGLE SOURCE OF TRUTH: Default Preset Values
    // These defaults are used by:
    // - FFBEngine constructor (via ApplyDefaultsToEngine)
    // - "Default" preset in LoadPresets()
    // - "Reset Defaults" button in GUI
    // - Test presets that don't explicitly set these values
    //
    // âš ï¸ IMPORTANT: When changing these defaults, you MUST also update:
    // 1. SetAdvancedBraking() default parameters below (abs_f, lockup_f)
    // 2. test_ffb_engine.cpp: expected_abs_freq and expected_lockup_freq_scale
    // 3. Any test presets in Config.cpp that rely on these defaults
    //
    // Current defaults match: GT3 DD 15 Nm (Simagic Alpha) - v0.6.35
    GeneralConfig general;
    FrontAxleConfig front_axle;
    RearAxleConfig rear_axle;
    float lateral_load = 0.0f; // New v0.7.121
    int lat_load_transform = 0; // New v0.7.154 (Issue #282)
    float slip_smoothing = 0.002f;
    float long_load_effect = 0.0f; // Renamed from dynamic_weight_gain (#301)
    float long_load_smoothing = 0.15f; // Renamed from dynamic_weight_smoothing (#301)
    int long_load_transform = 0; // New #301

    // FFB Safety (Issue #316)
    float safety_window_duration = FFBEngine::DEFAULT_SAFETY_WINDOW_DURATION;
    float safety_gain_reduction = FFBEngine::DEFAULT_SAFETY_GAIN_REDUCTION;
    float safety_smoothing_tau = FFBEngine::DEFAULT_SAFETY_SMOOTHING_TAU;
    float spike_detection_threshold = FFBEngine::DEFAULT_SPIKE_DETECTION_THRESHOLD;
    float immediate_spike_threshold = FFBEngine::DEFAULT_IMMEDIATE_SPIKE_THRESHOLD;
    float safety_slew_full_scale_time_s = FFBEngine::DEFAULT_SAFETY_SLEW_FULL_SCALE_TIME_S;
    bool stutter_safety_enabled = FFBEngine::DEFAULT_STUTTER_SAFETY_ENABLED;
    float stutter_threshold = FFBEngine::DEFAULT_STUTTER_THRESHOLD;

    float grip_smoothing_steady = 0.05f;    // v0.7.47
    float grip_smoothing_fast = 0.005f;     // v0.7.47
    float grip_smoothing_sensitivity = 0.1f; // v0.7.47
    
    bool lockup_enabled = true;
    float lockup_gain = 0.37479f;
    float lockup_start_pct = 1.0f;  // New v0.5.11
    float lockup_full_pct = 5.0f;  // New v0.5.11
    float lockup_rear_boost = 10.0f; // New v0.5.11
    float lockup_gamma = 0.1f;           // New v0.6.0
    float lockup_prediction_sens = 10.0f; // New v0.6.0
    float lockup_bump_reject = 0.1f;     // New v0.6.0
    float brake_load_cap = 2.0f;    // New v0.5.11
    float texture_load_cap = 1.5f;  // NEW v0.6.25
    
    bool abs_pulse_enabled = false;       // New v0.6.0
    float abs_gain = 2.0f;               // New v0.6.0
    float abs_freq = 25.5f;              // New v0.6.20
    
    bool spin_enabled = true;
    float spin_gain = 0.5f;
    float spin_freq_scale = 1.0f;        // New v0.6.20
    
    bool slide_enabled = false;
    float slide_gain = 0.226562f;
    float slide_freq = 1.0f;
    
    bool road_enabled = true;
    float road_gain = 0.0f;
    float vibration_gain = 1.0f; // New v0.7.110 (Issue #206)

    bool soft_lock_enabled = true;
    float soft_lock_stiffness = 20.0f;
    float soft_lock_damping = 0.5f;
    
    float lockup_freq_scale = 1.02f;      // New v0.6.20
    bool bottoming_enabled = true;
    float bottoming_gain = 1.0f;
    int bottoming_method = 0;
    float scrub_drag_gain = 0.0f;
    
    float gyro_gain = 0.0f;
    float stationary_damping = 1.0f; // New v0.7.206 (Issue #418)
    
    // NEW: Grip & Smoothing (v0.5.7)
    float optimal_slip_angle = 0.1f;
    float optimal_slip_ratio = 0.12f;
    
    // NEW: Advanced Smoothing (v0.5.8)
    float gyro_smoothing = 0.0f;
    float chassis_smoothing = 0.0f;

    // v0.6.23 New Settings with HIGHER DEFAULTS
    float speed_gate_lower = 1.0f; // 3.6 km/h
    float speed_gate_upper = 5.0f; // 18.0 km/h (Fixes idle shake)
    
    // Reserved for future implementation (v0.6.23+)
    float road_fallback_scale = 0.05f;      // Planned: Road texture fallback scaling
    bool understeer_affects_sop = false;     // Planned: Understeer modulation of SoP
    bool load_sensitivity_enabled = true;   // Issue #392

    // ===== SLOPE DETECTION (v0.7.0 â†’ v0.7.1 defaults) =====
    bool slope_detection_enabled = false;
    int slope_sg_window = 15;
    float slope_sensitivity = 0.5f;          // Reduced from 1.0 (less aggressive)

    float slope_smoothing_tau = 0.04f;       // Changed from 0.02 (smoother transitions)

    // v0.7.3: Slope detection stability fixes
    float slope_alpha_threshold = 0.02f;
    float slope_decay_rate = 5.0f;
    bool slope_confidence_enabled = true;

    // v0.7.11: Min/Max Threshold System
    float slope_min_threshold = -0.3f;
    float slope_max_threshold = -2.0f;

    // NEW v0.7.40: Advanced Slope Settings
    float slope_g_slew_limit = 50.0f;
    bool slope_use_torque = true;
    float slope_torque_sensitivity = 0.5f;
    float slope_confidence_max_rate = 0.10f;

    bool rest_api_enabled = false;
    int rest_api_port = 6397;

    // 2. Constructors
    Preset(std::string n, bool builtin = false) : name(n), is_builtin(builtin), app_version(LMUFFB_VERSION) {}
    Preset() : name("Unnamed"), is_builtin(false), app_version(LMUFFB_VERSION) {} // Default constructor for file loading

    // 3. Fluent Setters (The "Python Dictionary" feel)
    Preset& SetGain(float v) { general.gain = v; return *this; }
    Preset& SetUndersteer(float v) { front_axle.understeer_effect = v; return *this; }
    Preset& SetUndersteerGamma(float v) { front_axle.understeer_gamma = v; return *this; }
    Preset& SetSoP(float v) { rear_axle.sop_effect = v; return *this; }
    Preset& SetSoPScale(float v) { rear_axle.sop_scale = v; return *this; }
    Preset& SetSmoothing(float v) { rear_axle.sop_smoothing_factor = v; return *this; }
    Preset& SetMinForce(float v) { general.min_force = v; return *this; }
    Preset& SetOversteer(float v) { rear_axle.oversteer_boost = v; return *this; }
    Preset& SetLongitudinalLoad(float v) { long_load_effect = v; return *this; }
    Preset& SetLongitudinalLoadSmoothing(float v) { long_load_smoothing = v; return *this; }
    Preset& SetLongitudinalLoadTransform(int v) { long_load_transform = v; return *this; }
    Preset& SetGripSmoothing(float steady, float fast, float sens) {
        grip_smoothing_steady = steady;
        grip_smoothing_fast = fast;
        grip_smoothing_sensitivity = sens;
        return *this;
    }
    Preset& SetSlipSmoothing(float v) { slip_smoothing = v; return *this; }
    
    Preset& SetLockup(bool enabled, float g, float start = 5.0f, float full = 15.0f, float boost = 1.5f) { 
        lockup_enabled = enabled; 
        lockup_gain = g; 
        lockup_start_pct = start;
        lockup_full_pct = full;
        lockup_rear_boost = boost;
        return *this; 
    }
    Preset& SetBrakeCap(float v) { brake_load_cap = v; return *this; }
    Preset& SetSpin(bool enabled, float g, float scale = 1.0f) { 
        spin_enabled = enabled; 
        spin_gain = g; 
        spin_freq_scale = scale;
        return *this; 
    }
    Preset& SetSlide(bool enabled, float g, float f = 1.0f) { 
        slide_enabled = enabled; 
        slide_gain = g; 
        slide_freq = f; 
        return *this; 
    }
    Preset& SetRoad(bool enabled, float g) { road_enabled = enabled; road_gain = g; return *this; }
    Preset& SetVibrationGain(float v) { vibration_gain = v; return *this; }
    Preset& SetDynamicNormalization(bool enabled) { general.dynamic_normalization_enabled = enabled; return *this; }

    Preset& SetSoftLock(bool enabled, float stiffness, float damping) {
        soft_lock_enabled = enabled;
        soft_lock_stiffness = stiffness;
        soft_lock_damping = damping;
        return *this;
    }
    
    Preset& SetHardwareScaling(float wheelbase, float target) {
        general.wheelbase_max_nm = wheelbase;
        general.target_rim_nm = target;
        return *this;
    }
    
    Preset& SetBottoming(bool enabled, float gain, int method) {
        bottoming_enabled = enabled;
        bottoming_gain = gain;
        bottoming_method = method;
        return *this;
    }
    Preset& SetScrub(float v) { scrub_drag_gain = v; return *this; }
    Preset& SetRearAlign(float v) { rear_axle.rear_align_effect = v; return *this; }
    Preset& SetKerbStrikeRejection(float v) { rear_axle.kerb_strike_rejection = v; return *this; }
    Preset& SetSoPYaw(float v) { rear_axle.sop_yaw_gain = v; return *this; }
    Preset& SetGyro(float v) { gyro_gain = v; return *this; }
    Preset& SetStationaryDamping(float v) { stationary_damping = v; return *this; }
    
    Preset& SetShaftGain(float v) { front_axle.steering_shaft_gain = v; return *this; }
    Preset& SetInGameGain(float v) { front_axle.ingame_ffb_gain = v; return *this; }
    Preset& SetTorqueSource(int v, bool passthrough = false) { front_axle.torque_source = v; front_axle.torque_passthrough = passthrough; return *this; }
    Preset& SetSteering100HzReconstruction(int v) { front_axle.steering_100hz_reconstruction = v; return *this; }
    Preset& SetFlatspot(bool enabled, float strength = 1.0f, float q = 2.0f) { 
        front_axle.flatspot_suppression = enabled;
        front_axle.flatspot_strength = strength;
        front_axle.notch_q = q;
        return *this; 
    }
    
    Preset& SetStaticNotch(bool enabled, float freq, float width = 2.0f) {
        front_axle.static_notch_enabled = enabled;
        front_axle.static_notch_freq = freq;
        front_axle.static_notch_width = width;
        return *this;
    }
    Preset& SetYawKickThreshold(float v) { rear_axle.yaw_kick_threshold = v; return *this; }

    Preset& SetUnloadedYawKick(float gain, float threshold, float sens, float gamma, float punch) {
        rear_axle.unloaded_yaw_gain = gain;
        rear_axle.unloaded_yaw_threshold = threshold;
        rear_axle.unloaded_yaw_sens = sens;
        rear_axle.unloaded_yaw_gamma = gamma;
        rear_axle.unloaded_yaw_punch = punch;
        return *this;
    }

    Preset& SetPowerYawKick(float gain, float threshold, float slip, float gamma, float punch) {
        rear_axle.power_yaw_gain = gain;
        rear_axle.power_yaw_threshold = threshold;
        rear_axle.power_slip_threshold = slip;
        rear_axle.power_yaw_gamma = gamma;
        rear_axle.power_yaw_punch = punch;
        return *this;
    }

    Preset& SetSpeedGate(float lower, float upper) { speed_gate_lower = lower; speed_gate_upper = upper; return *this; }

    Preset& SetOptimalSlip(float angle, float ratio) {
        optimal_slip_angle = angle;
        optimal_slip_ratio = ratio;
        return *this;
    }
    Preset& SetShaftSmoothing(float v) { front_axle.steering_shaft_smoothing = v; return *this; }
    
    Preset& SetGyroSmoothing(float v) { gyro_smoothing = v; return *this; }
    Preset& SetYawSmoothing(float v) { rear_axle.yaw_accel_smoothing = v; return *this; }
    Preset& SetChassisSmoothing(float v) { chassis_smoothing = v; return *this; }
    
    Preset& SetSlopeDetection(bool enabled, int window = 15, float min_thresh = -0.3f, float max_thresh = -2.0f, float tau = 0.04f) {
        slope_detection_enabled = enabled;
        slope_sg_window = window;
        slope_min_threshold = min_thresh;
        slope_max_threshold = max_thresh;
        slope_smoothing_tau = tau;
        return *this;
    }

    Preset& SetSlopeStability(float alpha_thresh = 0.02f, float decay = 5.0f, bool conf = true) {
        slope_alpha_threshold = alpha_thresh;
        slope_decay_rate = decay;
        slope_confidence_enabled = conf;
        return *this;
    }

    Preset& SetSlopeAdvanced(float slew = 50.0f, bool use_torque = true, float torque_sens = 0.5f) {
        slope_g_slew_limit = slew;
        slope_use_torque = use_torque;
        slope_torque_sensitivity = torque_sens;
        return *this;
    }

    Preset& SetRestApiFallback(bool enabled, int port = 6397) {
        rest_api_enabled = enabled;
        rest_api_port = port;
        return *this;
    }

    Preset& SetSafety(float duration, float gain, float smoothing, float threshold, float immediate, float slew, bool stutter = false, float stutter_thresh = 1.5f) {
        safety_window_duration = duration;
        safety_gain_reduction = gain;
        safety_smoothing_tau = smoothing;
        spike_detection_threshold = threshold;
        immediate_spike_threshold = immediate;
        safety_slew_full_scale_time_s = slew;
        stutter_safety_enabled = stutter;
        stutter_threshold = stutter_thresh;
        return *this;
    }

    // Advanced Braking (v0.6.0)
    // âš ï¸ IMPORTANT: Default parameters (abs_f, lockup_f) must match Config.h defaults!
    // When changing Config.h defaults, update these values to match.
    // Current: abs_f=25.5, lockup_f=1.02 (GT3 DD 15 Nm defaults - v0.6.35)
    Preset& SetAdvancedBraking(float gamma, float sens, float bump, bool abs, float abs_g, float abs_f = 25.5f, float lockup_f = 1.02f) {
        lockup_gamma = gamma;
        lockup_prediction_sens = sens;
        lockup_bump_reject = bump;
        abs_pulse_enabled = abs;
        abs_gain = abs_g;
        abs_freq = abs_f;
        lockup_freq_scale = lockup_f;
        return *this;
    }

    // 4. Static method to apply defaults to FFBEngine (Single Source of Truth)
    // This is called by FFBEngine constructor to initialize with T300 defaults
    static void ApplyDefaultsToEngine(FFBEngine& engine) {
        Preset defaults; // Uses default member initializers (T300 values)
        defaults.Apply(engine);
    }

    // Apply this preset to an engine instance
    // v0.7.16: Added comprehensive safety clamping to prevent crashes/NaN from invalid config values
    void Apply(FFBEngine& engine) const {
        engine.m_general = this->general;
        engine.m_general.Validate();

        engine.m_front_axle = this->front_axle;
        engine.m_front_axle.Validate();

        engine.m_rear_axle = this->rear_axle;
        engine.m_rear_axle.Validate();

        engine.m_lat_load_effect = (std::max)(0.0f, (std::min)(2.0f, lateral_load));
        engine.m_lat_load_transform = static_cast<LoadTransform>(std::clamp(lat_load_transform, 0, 3));
        engine.m_slip_angle_smoothing = (std::max)(0.0001f, slip_smoothing);
        engine.m_long_load_effect = (std::max)(0.0f, (std::min)(10.0f, long_load_effect));
        engine.m_long_load_smoothing = (std::max)(0.0f, long_load_smoothing);
        engine.m_long_load_transform = static_cast<LoadTransform>(std::clamp(long_load_transform, 0, 3));
        engine.m_grip_smoothing_steady = (std::max)(0.0f, grip_smoothing_steady);
        engine.m_grip_smoothing_fast = (std::max)(0.0f, grip_smoothing_fast);
        engine.m_grip_smoothing_sensitivity = (std::max)(0.001f, grip_smoothing_sensitivity);

        // FFB Safety (Issue #316)
        engine.m_safety.m_safety_window_duration = (std::max)(0.0f, safety_window_duration);
        engine.m_safety.m_safety_gain_reduction = (std::max)(0.0f, (std::min)(1.0f, safety_gain_reduction));
        engine.m_safety.m_safety_smoothing_tau = (std::max)(0.001f, safety_smoothing_tau);
        engine.m_safety.m_spike_detection_threshold = (std::max)(1.0f, spike_detection_threshold);
        engine.m_safety.m_immediate_spike_threshold = (std::max)(1.0f, immediate_spike_threshold);
        engine.m_safety.m_safety_slew_full_scale_time_s = (std::max)(0.01f, safety_slew_full_scale_time_s);
        engine.m_safety.m_stutter_safety_enabled = stutter_safety_enabled;
        engine.m_safety.m_stutter_threshold = (std::max)(1.01f, stutter_threshold);

        engine.m_lockup_enabled = lockup_enabled;
        engine.m_lockup_gain = (std::max)(0.0f, lockup_gain);
        engine.m_lockup_start_pct = (std::max)(0.1f, lockup_start_pct);
        engine.m_lockup_full_pct = (std::max)(0.2f, lockup_full_pct);
        engine.m_lockup_rear_boost = (std::max)(0.0f, lockup_rear_boost);
        engine.m_lockup_gamma = (std::max)(0.1f, lockup_gamma); // Critical: prevent pow(0, negative) crash
        engine.m_lockup_prediction_sens = (std::max)(1.0f, lockup_prediction_sens);
        engine.m_lockup_bump_reject = (std::max)(0.01f, lockup_bump_reject);
        engine.m_brake_load_cap = (std::max)(1.0f, brake_load_cap);
        engine.m_texture_load_cap = (std::max)(1.0f, texture_load_cap);

        engine.m_abs_pulse_enabled = abs_pulse_enabled;
        engine.m_abs_gain = (std::max)(0.0f, abs_gain);

        engine.m_spin_enabled = spin_enabled;
        engine.m_spin_gain = (std::max)(0.0f, spin_gain);
        engine.m_slide_texture_enabled = slide_enabled;
        engine.m_slide_texture_gain = (std::max)(0.0f, slide_gain);
        engine.m_slide_freq_scale = (std::max)(0.1f, slide_freq);
        engine.m_road_texture_enabled = road_enabled;
        engine.m_road_texture_gain = (std::max)(0.0f, road_gain);
        engine.m_vibration_gain = (std::max)(0.0f, (std::min)(2.0f, vibration_gain));

        engine.m_soft_lock_enabled = soft_lock_enabled;
        engine.m_soft_lock_stiffness = (std::max)(0.0f, soft_lock_stiffness);
        engine.m_soft_lock_damping = (std::max)(0.0f, soft_lock_damping);

        engine.m_abs_freq_hz = (std::max)(1.0f, abs_freq);
        engine.m_lockup_freq_scale = (std::max)(0.1f, lockup_freq_scale);
        engine.m_spin_freq_scale = (std::max)(0.1f, spin_freq_scale);
        engine.m_bottoming_enabled = bottoming_enabled;
        engine.m_bottoming_gain = (std::max)(0.0f, (std::min)(2.0f, bottoming_gain));
        engine.m_bottoming_method = bottoming_method;
        engine.m_scrub_drag_gain = (std::max)(0.0f, scrub_drag_gain);
        engine.m_gyro_gain = (std::max)(0.0f, gyro_gain);
        engine.m_stationary_damping = (std::max)(0.0f, (std::min)(1.0f, stationary_damping));

        engine.m_speed_gate_lower = (std::max)(0.0f, speed_gate_lower);
        engine.m_speed_gate_upper = (std::max)(0.1f, speed_gate_upper);
        
        // NEW: Grip & Smoothing (v0.5.7/v0.5.8)
        engine.m_optimal_slip_angle = (std::max)(0.01f, optimal_slip_angle); // Critical for grip division
        engine.m_optimal_slip_ratio = (std::max)(0.01f, optimal_slip_ratio); // Critical for grip division
        engine.m_gyro_smoothing = (std::max)(0.0f, gyro_smoothing);
        engine.m_chassis_inertia_smoothing = (std::max)(0.0f, chassis_smoothing);
        engine.m_road_fallback_scale = (std::max)(0.0f, road_fallback_scale);
        engine.m_understeer_affects_sop = understeer_affects_sop;
        engine.m_load_sensitivity_enabled = load_sensitivity_enabled;
        
        // Slope Detection (v0.7.0)
        engine.m_slope_detection_enabled = slope_detection_enabled;
        engine.m_slope_sg_window = (std::max)(5, (std::min)(41, slope_sg_window));
        if (engine.m_slope_sg_window % 2 == 0) engine.m_slope_sg_window++; // Must be odd for SG
        engine.m_slope_sensitivity = (std::max)(0.1f, slope_sensitivity);

        engine.m_slope_smoothing_tau = (std::max)(0.001f, slope_smoothing_tau);

        // v0.7.3: Slope stability fixes
        engine.m_slope_alpha_threshold = (std::max)(0.001f, slope_alpha_threshold); // Critical for slope division
        engine.m_slope_decay_rate = (std::max)(0.1f, slope_decay_rate);
        engine.m_slope_confidence_enabled = slope_confidence_enabled;
        engine.m_slope_confidence_max_rate = (std::max)(engine.m_slope_alpha_threshold + 0.01f, slope_confidence_max_rate);

        // v0.7.11: Min/Max thresholds
        engine.m_slope_min_threshold = slope_min_threshold;
        engine.m_slope_max_threshold = slope_max_threshold;

        // NEW v0.7.40: Advanced Slope Settings
        engine.m_slope_g_slew_limit = (std::max)(1.0f, slope_g_slew_limit);
        engine.m_slope_use_torque = slope_use_torque;
        engine.m_slope_torque_sensitivity = (std::max)(0.01f, slope_torque_sensitivity);

        engine.m_rest_api_enabled = rest_api_enabled;
        engine.m_rest_api_port = (std::max)(1, rest_api_port);

        // Stage 1 & 2 Normalization (Issue #152 & #153)
        // Initialize session peak from target rim torque to provide a sane starting point.
        engine.m_session_peak_torque = (std::max)(1.0, (double)engine.m_general.target_rim_nm);
        engine.m_smoothed_structural_mult = 1.0 / engine.m_session_peak_torque;
    }

    // NEW: Ensure values are within safe ranges (v0.7.16)
    void Validate() {
        general.Validate();
        front_axle.Validate();
        rear_axle.Validate();
        lateral_load = (std::max)(0.0f, (std::min)(2.0f, lateral_load));
        lat_load_transform = std::clamp(lat_load_transform, 0, 3);
        slip_smoothing = (std::max)(0.0001f, slip_smoothing);
        long_load_effect = (std::max)(0.0f, (std::min)(10.0f, long_load_effect));
        long_load_smoothing = (std::max)(0.0f, long_load_smoothing);
        long_load_transform = std::clamp(long_load_transform, 0, 3);
        grip_smoothing_steady = (std::max)(0.0f, grip_smoothing_steady);
        grip_smoothing_fast = (std::max)(0.0f, grip_smoothing_fast);
        grip_smoothing_sensitivity = (std::max)(0.001f, grip_smoothing_sensitivity);
        lockup_gain = (std::max)(0.0f, lockup_gain);
        lockup_start_pct = (std::max)(0.1f, lockup_start_pct);
        lockup_full_pct = (std::max)(0.2f, lockup_full_pct);
        lockup_rear_boost = (std::max)(0.0f, lockup_rear_boost);
        lockup_gamma = (std::max)(0.1f, lockup_gamma);
        lockup_prediction_sens = (std::max)(1.0f, lockup_prediction_sens);
        lockup_bump_reject = (std::max)(0.01f, lockup_bump_reject);
        brake_load_cap = (std::max)(1.0f, brake_load_cap);
        texture_load_cap = (std::max)(1.0f, texture_load_cap);
        abs_gain = (std::max)(0.0f, abs_gain);
        bottoming_gain = (std::max)(0.0f, (std::min)(2.0f, bottoming_gain));
        spin_gain = (std::max)(0.0f, spin_gain);
        slide_gain = (std::max)(0.0f, slide_gain);
        slide_freq = (std::max)(0.1f, slide_freq);
        road_gain = (std::max)(0.0f, road_gain);
        vibration_gain = (std::max)(0.0f, (std::min)(2.0f, vibration_gain));
        soft_lock_stiffness = (std::max)(0.0f, soft_lock_stiffness);
        soft_lock_damping = (std::max)(0.0f, soft_lock_damping);
        abs_freq = (std::max)(1.0f, abs_freq);
        lockup_freq_scale = (std::max)(0.1f, lockup_freq_scale);
        spin_freq_scale = (std::max)(0.1f, spin_freq_scale);
        scrub_drag_gain = (std::max)(0.0f, scrub_drag_gain);
        gyro_gain = (std::max)(0.0f, gyro_gain);

        speed_gate_upper = (std::max)(0.1f, speed_gate_upper);
        optimal_slip_angle = (std::max)(0.01f, optimal_slip_angle);
        optimal_slip_ratio = (std::max)(0.01f, optimal_slip_ratio);
        gyro_smoothing = (std::max)(0.0f, gyro_smoothing);
        chassis_smoothing = (std::max)(0.0f, chassis_smoothing);
        road_fallback_scale = (std::max)(0.0f, road_fallback_scale);
        slope_sg_window = (std::max)(5, (std::min)(41, slope_sg_window));
        if (slope_sg_window % 2 == 0) slope_sg_window++;
        slope_sensitivity = (std::max)(0.1f, slope_sensitivity);
        slope_smoothing_tau = (std::max)(0.001f, slope_smoothing_tau);
        slope_alpha_threshold = (std::max)(0.001f, slope_alpha_threshold);
        slope_decay_rate = (std::max)(0.1f, slope_decay_rate);
        slope_g_slew_limit = (std::max)(1.0f, slope_g_slew_limit);
        slope_torque_sensitivity = (std::max)(0.01f, slope_torque_sensitivity);
        slope_confidence_max_rate = (std::max)(slope_alpha_threshold + 0.01f, slope_confidence_max_rate);
        rest_api_port = (std::max)(1, rest_api_port);

        // FFB Safety (Issue #316)
        safety_window_duration = (std::max)(0.0f, safety_window_duration);
        safety_gain_reduction = (std::max)(0.0f, (std::min)(1.0f, safety_gain_reduction));
        safety_smoothing_tau = (std::max)(0.001f, safety_smoothing_tau);
        spike_detection_threshold = (std::max)(1.0f, spike_detection_threshold);
        immediate_spike_threshold = (std::max)(1.0f, immediate_spike_threshold);
        safety_slew_full_scale_time_s = (std::max)(0.01f, safety_slew_full_scale_time_s);
        stutter_threshold = (std::max)(1.01f, stutter_threshold);
    }

    // NEW: Capture current engine state into this preset
    void UpdateFromEngine(const FFBEngine& engine) {
        general = engine.m_general;
        front_axle = engine.m_front_axle;
        rear_axle = engine.m_rear_axle;
        lateral_load = engine.m_lat_load_effect;
        lat_load_transform = static_cast<int>(engine.m_lat_load_transform);
        slip_smoothing = engine.m_slip_angle_smoothing;
        long_load_effect = engine.m_long_load_effect;
        long_load_smoothing = engine.m_long_load_smoothing;
        long_load_transform = static_cast<int>(engine.m_long_load_transform);
        grip_smoothing_steady = engine.m_grip_smoothing_steady;
        grip_smoothing_fast = engine.m_grip_smoothing_fast;
        grip_smoothing_sensitivity = engine.m_grip_smoothing_sensitivity;

        // FFB Safety (Issue #316)
        safety_window_duration = engine.m_safety.m_safety_window_duration;
        safety_gain_reduction = engine.m_safety.m_safety_gain_reduction;
        safety_smoothing_tau = engine.m_safety.m_safety_smoothing_tau;
        spike_detection_threshold = engine.m_safety.m_spike_detection_threshold;
        immediate_spike_threshold = engine.m_safety.m_immediate_spike_threshold;
        safety_slew_full_scale_time_s = engine.m_safety.m_safety_slew_full_scale_time_s;
        stutter_safety_enabled = engine.m_safety.m_stutter_safety_enabled;
        stutter_threshold = engine.m_safety.m_stutter_threshold;

        lockup_enabled = engine.m_lockup_enabled;
        lockup_gain = engine.m_lockup_gain;
        lockup_start_pct = engine.m_lockup_start_pct;
        lockup_full_pct = engine.m_lockup_full_pct;
        lockup_rear_boost = engine.m_lockup_rear_boost;
        lockup_gamma = engine.m_lockup_gamma;
        lockup_prediction_sens = engine.m_lockup_prediction_sens;
        lockup_bump_reject = engine.m_lockup_bump_reject;
        brake_load_cap = engine.m_brake_load_cap;
        texture_load_cap = engine.m_texture_load_cap;  // NEW v0.6.25
        abs_pulse_enabled = engine.m_abs_pulse_enabled;
        abs_gain = engine.m_abs_gain;
        
        spin_enabled = engine.m_spin_enabled;
        spin_gain = engine.m_spin_gain;
        slide_enabled = engine.m_slide_texture_enabled;
        slide_gain = engine.m_slide_texture_gain;
        slide_freq = engine.m_slide_freq_scale;
        road_enabled = engine.m_road_texture_enabled;
        road_gain = engine.m_road_texture_gain;
        vibration_gain = engine.m_vibration_gain;

        soft_lock_enabled = engine.m_soft_lock_enabled;
        soft_lock_stiffness = engine.m_soft_lock_stiffness;
        soft_lock_damping = engine.m_soft_lock_damping;

        abs_freq = engine.m_abs_freq_hz;
        lockup_freq_scale = engine.m_lockup_freq_scale;
        spin_freq_scale = engine.m_spin_freq_scale;
        bottoming_enabled = engine.m_bottoming_enabled;
        bottoming_gain = engine.m_bottoming_gain;
        bottoming_method = engine.m_bottoming_method;
        scrub_drag_gain = engine.m_scrub_drag_gain;
        gyro_gain = engine.m_gyro_gain;
        stationary_damping = engine.m_stationary_damping;

        speed_gate_lower = engine.m_speed_gate_lower;
        speed_gate_upper = engine.m_speed_gate_upper;

        // NEW: Grip & Smoothing (v0.5.7/v0.5.8)
        optimal_slip_angle = engine.m_optimal_slip_angle;
        optimal_slip_ratio = engine.m_optimal_slip_ratio;
        gyro_smoothing = engine.m_gyro_smoothing;
        chassis_smoothing = engine.m_chassis_inertia_smoothing;
        road_fallback_scale = engine.m_road_fallback_scale;
        understeer_affects_sop = engine.m_understeer_affects_sop;
        load_sensitivity_enabled = engine.m_load_sensitivity_enabled;

        // Slope Detection (v0.7.0)
        slope_detection_enabled = engine.m_slope_detection_enabled;
        slope_sg_window = engine.m_slope_sg_window;
        slope_sensitivity = engine.m_slope_sensitivity;

        slope_smoothing_tau = engine.m_slope_smoothing_tau;

        // v0.7.3: Slope stability fixes
        slope_alpha_threshold = engine.m_slope_alpha_threshold;
        slope_decay_rate = engine.m_slope_decay_rate;
        slope_confidence_enabled = engine.m_slope_confidence_enabled;
        slope_confidence_max_rate = engine.m_slope_confidence_max_rate;

        // v0.7.11: Min/Max thresholds
        slope_min_threshold = engine.m_slope_min_threshold;
        slope_max_threshold = engine.m_slope_max_threshold;

        // NEW v0.7.40: Advanced Slope Settings
        slope_g_slew_limit = engine.m_slope_g_slew_limit;
        slope_use_torque = engine.m_slope_use_torque;
        slope_torque_sensitivity = engine.m_slope_torque_sensitivity;

        rest_api_enabled = engine.m_rest_api_enabled;
        rest_api_port = engine.m_rest_api_port;

        app_version = LMUFFB_VERSION;
    }

    bool Equals(const Preset& p) const {
        const float eps = 0.0001f;
        auto is_near = [](float a, float b, float epsilon) { return std::abs(a - b) < epsilon; };

        if (!general.Equals(p.general, eps)) return false;
        if (!front_axle.Equals(p.front_axle, eps)) return false;
        if (!rear_axle.Equals(p.rear_axle, eps)) return false;
        if (!is_near(lateral_load, p.lateral_load, eps)) return false;
        if (lat_load_transform != p.lat_load_transform) return false;
        if (!is_near(slip_smoothing, p.slip_smoothing, eps)) return false;
        if (!is_near(long_load_effect, p.long_load_effect, eps)) return false;
        if (!is_near(long_load_smoothing, p.long_load_smoothing, eps)) return false;
        if (long_load_transform != p.long_load_transform) return false;
        if (!is_near(grip_smoothing_steady, p.grip_smoothing_steady, eps)) return false;
        if (!is_near(grip_smoothing_fast, p.grip_smoothing_fast, eps)) return false;
        if (!is_near(grip_smoothing_sensitivity, p.grip_smoothing_sensitivity, eps)) return false;

        // FFB Safety (Issue #316)
        if (!is_near(safety_window_duration, p.safety_window_duration, eps)) return false;
        if (!is_near(safety_gain_reduction, p.safety_gain_reduction, eps)) return false;
        if (!is_near(safety_smoothing_tau, p.safety_smoothing_tau, eps)) return false;
        if (!is_near(spike_detection_threshold, p.spike_detection_threshold, eps)) return false;
        if (!is_near(immediate_spike_threshold, p.immediate_spike_threshold, eps)) return false;
        if (!is_near(safety_slew_full_scale_time_s, p.safety_slew_full_scale_time_s, eps)) return false;
        if (stutter_safety_enabled != p.stutter_safety_enabled) return false;
        if (!is_near(stutter_threshold, p.stutter_threshold, eps)) return false;

        if (lockup_enabled != p.lockup_enabled) return false;
        if (!is_near(lockup_gain, p.lockup_gain, eps)) return false;
        if (!is_near(lockup_start_pct, p.lockup_start_pct, eps)) return false;
        if (!is_near(lockup_full_pct, p.lockup_full_pct, eps)) return false;
        if (!is_near(lockup_rear_boost, p.lockup_rear_boost, eps)) return false;
        if (!is_near(lockup_gamma, p.lockup_gamma, eps)) return false;
        if (!is_near(lockup_prediction_sens, p.lockup_prediction_sens, eps)) return false;
        if (!is_near(lockup_bump_reject, p.lockup_bump_reject, eps)) return false;
        if (!is_near(brake_load_cap, p.brake_load_cap, eps)) return false;
        if (!is_near(texture_load_cap, p.texture_load_cap, eps)) return false;

        if (abs_pulse_enabled != p.abs_pulse_enabled) return false;
        if (!is_near(abs_gain, p.abs_gain, eps)) return false;
        if (!is_near(abs_freq, p.abs_freq, eps)) return false;

        if (spin_enabled != p.spin_enabled) return false;
        if (!is_near(spin_gain, p.spin_gain, eps)) return false;
        if (!is_near(spin_freq_scale, p.spin_freq_scale, eps)) return false;

        if (slide_enabled != p.slide_enabled) return false;
        if (!is_near(slide_gain, p.slide_gain, eps)) return false;
        if (!is_near(slide_freq, p.slide_freq, eps)) return false;

        if (road_enabled != p.road_enabled) return false;
        if (!is_near(road_gain, p.road_gain, eps)) return false;
        if (!is_near(vibration_gain, p.vibration_gain, eps)) return false;

        if (soft_lock_enabled != p.soft_lock_enabled) return false;
        if (!is_near(soft_lock_stiffness, p.soft_lock_stiffness, eps)) return false;
        if (!is_near(soft_lock_damping, p.soft_lock_damping, eps)) return false;

        if (!is_near(lockup_freq_scale, p.lockup_freq_scale, eps)) return false;
        if (bottoming_enabled != p.bottoming_enabled) return false;
        if (!is_near(bottoming_gain, p.bottoming_gain, eps)) return false;
        if (bottoming_method != p.bottoming_method) return false;
        if (!is_near(scrub_drag_gain, p.scrub_drag_gain, eps)) return false;
        if (!is_near(gyro_gain, p.gyro_gain, eps)) return false;
        if (!is_near(stationary_damping, p.stationary_damping, eps)) return false;

        if (!is_near(optimal_slip_angle, p.optimal_slip_angle, eps)) return false;
        if (!is_near(optimal_slip_ratio, p.optimal_slip_ratio, eps)) return false;
        if (!is_near(gyro_smoothing, p.gyro_smoothing, eps)) return false;
        if (!is_near(chassis_smoothing, p.chassis_smoothing, eps)) return false;

        if (!is_near(speed_gate_lower, p.speed_gate_lower, eps)) return false;
        if (!is_near(speed_gate_upper, p.speed_gate_upper, eps)) return false;

        if (!is_near(road_fallback_scale, p.road_fallback_scale, eps)) return false;
        if (understeer_affects_sop != p.understeer_affects_sop) return false;
        if (load_sensitivity_enabled != p.load_sensitivity_enabled) return false;

        if (slope_detection_enabled != p.slope_detection_enabled) return false;
        if (slope_sg_window != p.slope_sg_window) return false;
        if (!is_near(slope_sensitivity, p.slope_sensitivity, eps)) return false;

        if (!is_near(slope_smoothing_tau, p.slope_smoothing_tau, eps)) return false;
        if (!is_near(slope_alpha_threshold, p.slope_alpha_threshold, eps)) return false;
        if (!is_near(slope_decay_rate, p.slope_decay_rate, eps)) return false;
        if (slope_confidence_enabled != p.slope_confidence_enabled) return false;
        if (!is_near(slope_min_threshold, p.slope_min_threshold, eps)) return false;
        if (!is_near(slope_max_threshold, p.slope_max_threshold, eps)) return false;

        if (!is_near(slope_g_slew_limit, p.slope_g_slew_limit, eps)) return false;
        if (slope_use_torque != p.slope_use_torque) return false;
        if (!is_near(slope_torque_sensitivity, p.slope_torque_sensitivity, eps)) return false;
        if (!is_near(slope_confidence_max_rate, p.slope_confidence_max_rate, eps)) return false;

        if (rest_api_enabled != p.rest_api_enabled) return false;
        if (rest_api_port != p.rest_api_port) return false;

        return true;
    }
};

class Config {
public:
    static std::string m_config_path; // Default: "config.ini"
    static void Save(const FFBEngine& engine, const std::string& filename = "");
    static void Load(FFBEngine& engine, const std::string& filename = "");
    
    // Preset Management
    static std::vector<Preset> presets;
    static std::string m_last_preset_name; // NEW (v0.7.14)
    static void LoadPresets(); // Populates presets vector
    static void ApplyPreset(int index, FFBEngine& engine);
    
    // NEW: Add a user preset
    static void AddUserPreset(const std::string& name, const FFBEngine& engine);

    // NEW: Delete and Duplicate (v0.7.14)
    static void DeletePreset(int index, const FFBEngine& engine);
    static void DuplicatePreset(int index, const FFBEngine& engine);
    static bool IsEngineDirtyRelativeToPreset(int index, const FFBEngine& engine);

    // NEW: Import/Export (v0.7.12)
    static void ExportPreset(int index, const std::string& filename);
    static bool ImportPreset(const std::string& filename, const FFBEngine& engine);

    // NEW: Persist selected device
    static std::string m_last_device_guid;

    // Global App Settings (not part of FFB Physics)
    static bool m_always_on_top;      // NEW: Keep window on top
    static bool m_auto_start_logging; // NEW: Auto-start logging
    static std::string m_log_path;    // NEW: Path to save logs

    // Window Geometry Persistence (v0.5.5)
    static int win_pos_x, win_pos_y;
    static int win_w_small, win_h_small; // Dimensions for Config Only
    static int win_w_large, win_h_large; // Dimensions for Config + Graphs
    static bool show_graphs;             // Remember if graphs were open

    // Persistent storage for vehicle static loads (v0.7.70)
    static std::map<std::string, double> m_saved_static_loads;
    static std::recursive_mutex m_static_loads_mutex;

    // Flag to request a save from the main thread (avoids File I/O on FFB thread)
    static std::atomic<bool> m_needs_save;

    // Thread-safe access to static loads map (v0.7.70)
    static void SetSavedStaticLoad(const std::string& vehicleName, double value);
    static bool GetSavedStaticLoad(const std::string& vehicleName, double& value);

private:
    // Helper for parsing preset lines (v0.7.12)
    static void ParsePresetLine(const std::string& line, Preset& p, std::string& version, bool& needs_save, bool& legacy_torque_hack, float& legacy_torque_val);
    static bool ParseSystemLine(const std::string& key, const std::string& value, Preset& p, std::string& version, bool& needs_save, bool& legacy_torque_hack, float& legacy_torque_val);
    static bool ParsePhysicsLine(const std::string& key, const std::string& value, Preset& p);
    static bool ParseBrakingLine(const std::string& key, const std::string& value, Preset& p);
    static bool ParseVibrationLine(const std::string& key, const std::string& value, Preset& p);
    static bool ParseSafetyLine(const std::string& key, const std::string& value, Preset& p);

    static bool SyncSystemLine(const std::string& key, const std::string& value, FFBEngine& engine, std::string& version, bool& legacy_torque_hack, float& legacy_torque_val, bool& needs_save);
    static bool SyncPhysicsLine(const std::string& key, const std::string& value, FFBEngine& engine, std::string& version, bool& needs_save);
    static bool SyncBrakingLine(const std::string& key, const std::string& value, FFBEngine& engine);
    static bool SyncVibrationLine(const std::string& key, const std::string& value, FFBEngine& engine);
    static bool SyncSafetyLine(const std::string& key, const std::string& value, FFBEngine& engine);

    // Helper for writing preset fields (v0.7.12)
    static void WritePresetFields(std::ofstream& file, const Preset& p);
};




#endif





