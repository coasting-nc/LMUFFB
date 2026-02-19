#ifndef CONFIG_H
#define CONFIG_H

#include "FFBEngine.h"
#include <string>
#include <vector>
#include <chrono>
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
   float gain = 1.0f;
    float understeer = 1.0f;  // New scale: 0.0-2.0, where 1.0 = proportional
    float sop = 1.666f;
    float sop_scale = 1.0f;
    float sop_smoothing = 1.0f;
    float slip_smoothing = 0.002f;
    float min_force = 0.0f;
    float oversteer_boost = 2.52101f;
    float dynamic_weight_gain = 0.0f; // NEW v0.7.46
    float dynamic_weight_smoothing = 0.15f; // v0.7.47
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

    bool soft_lock_enabled = true;
    float soft_lock_stiffness = 20.0f;
    float soft_lock_damping = 0.5f;
    
    bool invert_force = true;
    float max_torque_ref = 100.0f; // T300 Calibrated
    
    float lockup_freq_scale = 1.02f;      // New v0.6.20
    int bottoming_method = 0;
    float scrub_drag_gain = 0.0f;
    
    float rear_align_effect = 0.666f;
    float sop_yaw_gain = 0.333f;
    float gyro_gain = 0.0f;
    
    float steering_shaft_gain = 1.0f;
    int base_force_mode = 0; // 0=Native
    int torque_source = 0;   // 0=Shaft, 1=Direct
    
    // NEW: Grip & Smoothing (v0.5.7)
    float optimal_slip_angle = 0.1f;
    float optimal_slip_ratio = 0.12f;
    float steering_shaft_smoothing = 0.0f;
    
    // NEW: Advanced Smoothing (v0.5.8)
    float gyro_smoothing = 0.0f;
    float yaw_smoothing = 0.001f;
    float chassis_smoothing = 0.0f;

    // v0.4.41: Signal Filtering
    bool flatspot_suppression = false;
    float notch_q = 2.0f;
    float flatspot_strength = 1.0f;
    
    bool static_notch_enabled = false;
    float static_notch_freq = 11.0f;
    float static_notch_width = 2.0f; // New v0.6.10
    float yaw_kick_threshold = 0.0f; // New v0.6.10

    // v0.6.23 New Settings with HIGHER DEFAULTS
    float speed_gate_lower = 1.0f; // 3.6 km/h
    float speed_gate_upper = 5.0f; // 18.0 km/h (Fixes idle shake)
    
    // Reserved for future implementation (v0.6.23+)
    float road_fallback_scale = 0.05f;      // Planned: Road texture fallback scaling
    bool understeer_affects_sop = false;     // Planned: Understeer modulation of SoP

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

    // 2. Constructors
    Preset(std::string n, bool builtin = false) : name(n), is_builtin(builtin), app_version(LMUFFB_VERSION) {}
    Preset() : name("Unnamed"), is_builtin(false), app_version(LMUFFB_VERSION) {} // Default constructor for file loading

    // 3. Fluent Setters (The "Python Dictionary" feel)
    Preset& SetGain(float v) { gain = v; return *this; }
    Preset& SetUndersteer(float v) { understeer = v; return *this; }
    Preset& SetSoP(float v) { sop = v; return *this; }
    Preset& SetSoPScale(float v) { sop_scale = v; return *this; }
    Preset& SetSmoothing(float v) { sop_smoothing = v; return *this; }
    Preset& SetMinForce(float v) { min_force = v; return *this; }
    Preset& SetOversteer(float v) { oversteer_boost = v; return *this; }
    Preset& SetDynamicWeight(float v) { dynamic_weight_gain = v; return *this; }
    Preset& SetDynamicWeightSmoothing(float v) { dynamic_weight_smoothing = v; return *this; }
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

    Preset& SetSoftLock(bool enabled, float stiffness, float damping) {
        soft_lock_enabled = enabled;
        soft_lock_stiffness = stiffness;
        soft_lock_damping = damping;
        return *this;
    }
    
    Preset& SetInvert(bool v) { invert_force = v; return *this; }
    Preset& SetMaxTorque(float v) { max_torque_ref = v; return *this; }
    
    Preset& SetBottoming(int method) { bottoming_method = method; return *this; }
    Preset& SetScrub(float v) { scrub_drag_gain = v; return *this; }
    Preset& SetRearAlign(float v) { rear_align_effect = v; return *this; }
    Preset& SetSoPYaw(float v) { sop_yaw_gain = v; return *this; }
    Preset& SetGyro(float v) { gyro_gain = v; return *this; }
    
    Preset& SetShaftGain(float v) { steering_shaft_gain = v; return *this; }
    Preset& SetBaseMode(int v) { base_force_mode = v; return *this; }
    Preset& SetTorqueSource(int v) { torque_source = v; return *this; }
    Preset& SetFlatspot(bool enabled, float strength = 1.0f, float q = 2.0f) { 
        flatspot_suppression = enabled; 
        flatspot_strength = strength;
        notch_q = q; 
        return *this; 
    }
    
    Preset& SetStaticNotch(bool enabled, float freq, float width = 2.0f) {
        static_notch_enabled = enabled;
        static_notch_freq = freq;
        static_notch_width = width;
        return *this;
    }
    Preset& SetYawKickThreshold(float v) { yaw_kick_threshold = v; return *this; }
    Preset& SetSpeedGate(float lower, float upper) { speed_gate_lower = lower; speed_gate_upper = upper; return *this; }

    Preset& SetOptimalSlip(float angle, float ratio) {
        optimal_slip_angle = angle;
        optimal_slip_ratio = ratio;
        return *this;
    }
    Preset& SetShaftSmoothing(float v) { steering_shaft_smoothing = v; return *this; }
    
    Preset& SetGyroSmoothing(float v) { gyro_smoothing = v; return *this; }
    Preset& SetYawSmoothing(float v) { yaw_smoothing = v; return *this; }
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
        engine.m_gain = (std::max)(0.0f, gain);
        engine.m_understeer_effect = (std::max)(0.0f, (std::min)(2.0f, understeer));
        engine.m_sop_effect = (std::max)(0.0f, (std::min)(2.0f, sop));
        engine.m_sop_scale = (std::max)(0.01f, sop_scale);
        engine.m_sop_smoothing_factor = (std::max)(0.0f, (std::min)(1.0f, sop_smoothing));
        engine.m_slip_angle_smoothing = (std::max)(0.0001f, slip_smoothing);
        engine.m_min_force = (std::max)(0.0f, min_force);
        engine.m_oversteer_boost = (std::max)(0.0f, oversteer_boost);
        engine.m_dynamic_weight_gain = (std::max)(0.0f, (std::min)(2.0f, dynamic_weight_gain));
        engine.m_dynamic_weight_smoothing = (std::max)(0.0f, dynamic_weight_smoothing);
        engine.m_grip_smoothing_steady = (std::max)(0.0f, grip_smoothing_steady);
        engine.m_grip_smoothing_fast = (std::max)(0.0f, grip_smoothing_fast);
        engine.m_grip_smoothing_sensitivity = (std::max)(0.001f, grip_smoothing_sensitivity);

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

        engine.m_soft_lock_enabled = soft_lock_enabled;
        engine.m_soft_lock_stiffness = (std::max)(0.0f, soft_lock_stiffness);
        engine.m_soft_lock_damping = (std::max)(0.0f, soft_lock_damping);

        engine.m_invert_force = invert_force;
        engine.m_max_torque_ref = (std::max)(1.0f, max_torque_ref); // Critical for normalization division
        engine.m_abs_freq_hz = (std::max)(1.0f, abs_freq);
        engine.m_lockup_freq_scale = (std::max)(0.1f, lockup_freq_scale);
        engine.m_spin_freq_scale = (std::max)(0.1f, spin_freq_scale);
        engine.m_bottoming_method = bottoming_method;
        engine.m_scrub_drag_gain = (std::max)(0.0f, scrub_drag_gain);
        engine.m_rear_align_effect = (std::max)(0.0f, rear_align_effect);
        engine.m_sop_yaw_gain = (std::max)(0.0f, sop_yaw_gain);
        engine.m_gyro_gain = (std::max)(0.0f, gyro_gain);
        engine.m_steering_shaft_gain = (std::max)(0.0f, steering_shaft_gain);
        engine.m_base_force_mode = base_force_mode;
        engine.m_torque_source = torque_source;
        engine.m_flatspot_suppression = flatspot_suppression;
        engine.m_notch_q = (std::max)(0.1f, notch_q); // Critical for biquad division
        engine.m_flatspot_strength = (std::max)(0.0f, (std::min)(1.0f, flatspot_strength));
        engine.m_static_notch_enabled = static_notch_enabled;
        engine.m_static_notch_freq = (std::max)(1.0f, static_notch_freq);
        engine.m_static_notch_width = (std::max)(0.1f, static_notch_width);
        engine.m_yaw_kick_threshold = (std::max)(0.0f, yaw_kick_threshold);
        engine.m_speed_gate_lower = (std::max)(0.0f, speed_gate_lower);
        engine.m_speed_gate_upper = (std::max)(0.1f, speed_gate_upper);
        
        // NEW: Grip & Smoothing (v0.5.7/v0.5.8)
        engine.m_optimal_slip_angle = (std::max)(0.01f, optimal_slip_angle); // Critical for grip division
        engine.m_optimal_slip_ratio = (std::max)(0.01f, optimal_slip_ratio); // Critical for grip division
        engine.m_steering_shaft_smoothing = (std::max)(0.0f, steering_shaft_smoothing);
        engine.m_gyro_smoothing = (std::max)(0.0f, gyro_smoothing);
        engine.m_yaw_accel_smoothing = (std::max)(0.0f, yaw_smoothing);
        engine.m_chassis_inertia_smoothing = (std::max)(0.0f, chassis_smoothing);
        engine.m_road_fallback_scale = (std::max)(0.0f, road_fallback_scale);
        engine.m_understeer_affects_sop = understeer_affects_sop;
        
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
    }

    // NEW: Ensure values are within safe ranges (v0.7.16)
    void Validate() {
        gain = (std::max)(0.0f, gain);
        understeer = (std::max)(0.0f, (std::min)(2.0f, understeer));
        sop = (std::max)(0.0f, (std::min)(2.0f, sop));
        sop_scale = (std::max)(0.01f, sop_scale);
        sop_smoothing = (std::max)(0.0f, (std::min)(1.0f, sop_smoothing));
        slip_smoothing = (std::max)(0.0001f, slip_smoothing);
        min_force = (std::max)(0.0f, min_force);
        oversteer_boost = (std::max)(0.0f, oversteer_boost);
        dynamic_weight_gain = (std::max)(0.0f, (std::min)(2.0f, dynamic_weight_gain));
        dynamic_weight_smoothing = (std::max)(0.0f, dynamic_weight_smoothing);
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
        spin_gain = (std::max)(0.0f, spin_gain);
        slide_gain = (std::max)(0.0f, slide_gain);
        slide_freq = (std::max)(0.1f, slide_freq);
        road_gain = (std::max)(0.0f, road_gain);
        soft_lock_stiffness = (std::max)(0.0f, soft_lock_stiffness);
        soft_lock_damping = (std::max)(0.0f, soft_lock_damping);
        max_torque_ref = (std::max)(1.0f, max_torque_ref);
        abs_freq = (std::max)(1.0f, abs_freq);
        lockup_freq_scale = (std::max)(0.1f, lockup_freq_scale);
        spin_freq_scale = (std::max)(0.1f, spin_freq_scale);
        scrub_drag_gain = (std::max)(0.0f, scrub_drag_gain);
        rear_align_effect = (std::max)(0.0f, rear_align_effect);
        sop_yaw_gain = (std::max)(0.0f, sop_yaw_gain);
        gyro_gain = (std::max)(0.0f, gyro_gain);
        steering_shaft_gain = (std::max)(0.0f, steering_shaft_gain);
        torque_source = (std::max)(0, (std::min)(1, torque_source));
        notch_q = (std::max)(0.1f, notch_q);
        flatspot_strength = (std::max)(0.0f, (std::min)(1.0f, flatspot_strength));
        static_notch_freq = (std::max)(1.0f, static_notch_freq);
        static_notch_width = (std::max)(0.1f, static_notch_width);
        speed_gate_upper = (std::max)(0.1f, speed_gate_upper);
        optimal_slip_angle = (std::max)(0.01f, optimal_slip_angle);
        optimal_slip_ratio = (std::max)(0.01f, optimal_slip_ratio);
        steering_shaft_smoothing = (std::max)(0.0f, steering_shaft_smoothing);
        gyro_smoothing = (std::max)(0.0f, gyro_smoothing);
        yaw_smoothing = (std::max)(0.0f, yaw_smoothing);
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
    }

    // NEW: Capture current engine state into this preset
    void UpdateFromEngine(const FFBEngine& engine) {
        gain = engine.m_gain;
        understeer = engine.m_understeer_effect;
        sop = engine.m_sop_effect;
        sop_scale = engine.m_sop_scale;
        sop_smoothing = engine.m_sop_smoothing_factor;
        slip_smoothing = engine.m_slip_angle_smoothing;
        min_force = engine.m_min_force;
        oversteer_boost = engine.m_oversteer_boost;
        dynamic_weight_gain = engine.m_dynamic_weight_gain;
        dynamic_weight_smoothing = engine.m_dynamic_weight_smoothing;
        grip_smoothing_steady = engine.m_grip_smoothing_steady;
        grip_smoothing_fast = engine.m_grip_smoothing_fast;
        grip_smoothing_sensitivity = engine.m_grip_smoothing_sensitivity;
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

        soft_lock_enabled = engine.m_soft_lock_enabled;
        soft_lock_stiffness = engine.m_soft_lock_stiffness;
        soft_lock_damping = engine.m_soft_lock_damping;

        invert_force = engine.m_invert_force;
        max_torque_ref = engine.m_max_torque_ref;
        abs_freq = engine.m_abs_freq_hz;
        lockup_freq_scale = engine.m_lockup_freq_scale;
        spin_freq_scale = engine.m_spin_freq_scale;
        bottoming_method = engine.m_bottoming_method;
        scrub_drag_gain = engine.m_scrub_drag_gain;
        rear_align_effect = engine.m_rear_align_effect;
        sop_yaw_gain = engine.m_sop_yaw_gain;
        gyro_gain = engine.m_gyro_gain;
        steering_shaft_gain = engine.m_steering_shaft_gain;
        base_force_mode = engine.m_base_force_mode;
        torque_source = engine.m_torque_source;
        flatspot_suppression = engine.m_flatspot_suppression;
        notch_q = engine.m_notch_q;
        flatspot_strength = engine.m_flatspot_strength;
        static_notch_enabled = engine.m_static_notch_enabled;
        static_notch_freq = engine.m_static_notch_freq;
        static_notch_width = engine.m_static_notch_width;
        yaw_kick_threshold = engine.m_yaw_kick_threshold;
        speed_gate_lower = engine.m_speed_gate_lower;
        speed_gate_upper = engine.m_speed_gate_upper;

        // NEW: Grip & Smoothing (v0.5.7/v0.5.8)
        optimal_slip_angle = engine.m_optimal_slip_angle;
        optimal_slip_ratio = engine.m_optimal_slip_ratio;
        steering_shaft_smoothing = engine.m_steering_shaft_smoothing;
        gyro_smoothing = engine.m_gyro_smoothing;
        yaw_smoothing = engine.m_yaw_accel_smoothing;
        chassis_smoothing = engine.m_chassis_inertia_smoothing;
        road_fallback_scale = engine.m_road_fallback_scale;
        understeer_affects_sop = engine.m_understeer_affects_sop;

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
        app_version = LMUFFB_VERSION;
    }

    bool Equals(const Preset& p) const {
        const float eps = 0.0001f;
        auto is_near = [](float a, float b, float epsilon) { return std::abs(a - b) < epsilon; };

        if (!is_near(gain, p.gain, eps)) return false;
        if (!is_near(understeer, p.understeer, eps)) return false;
        if (!is_near(sop, p.sop, eps)) return false;
        if (!is_near(sop_scale, p.sop_scale, eps)) return false;
        if (!is_near(sop_smoothing, p.sop_smoothing, eps)) return false;
        if (!is_near(slip_smoothing, p.slip_smoothing, eps)) return false;
        if (!is_near(min_force, p.min_force, eps)) return false;
        if (!is_near(oversteer_boost, p.oversteer_boost, eps)) return false;
        if (!is_near(dynamic_weight_gain, p.dynamic_weight_gain, eps)) return false;
        if (!is_near(dynamic_weight_smoothing, p.dynamic_weight_smoothing, eps)) return false;
        if (!is_near(grip_smoothing_steady, p.grip_smoothing_steady, eps)) return false;
        if (!is_near(grip_smoothing_fast, p.grip_smoothing_fast, eps)) return false;
        if (!is_near(grip_smoothing_sensitivity, p.grip_smoothing_sensitivity, eps)) return false;

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

        if (soft_lock_enabled != p.soft_lock_enabled) return false;
        if (!is_near(soft_lock_stiffness, p.soft_lock_stiffness, eps)) return false;
        if (!is_near(soft_lock_damping, p.soft_lock_damping, eps)) return false;

        if (invert_force != p.invert_force) return false;
        if (!is_near(max_torque_ref, p.max_torque_ref, eps)) return false;
        if (!is_near(lockup_freq_scale, p.lockup_freq_scale, eps)) return false;
        if (bottoming_method != p.bottoming_method) return false;
        if (!is_near(scrub_drag_gain, p.scrub_drag_gain, eps)) return false;
        if (!is_near(rear_align_effect, p.rear_align_effect, eps)) return false;
        if (!is_near(sop_yaw_gain, p.sop_yaw_gain, eps)) return false;
        if (!is_near(gyro_gain, p.gyro_gain, eps)) return false;
        if (!is_near(steering_shaft_gain, p.steering_shaft_gain, eps)) return false;
        if (base_force_mode != p.base_force_mode) return false;
        if (torque_source != p.torque_source) return false;

        if (!is_near(optimal_slip_angle, p.optimal_slip_angle, eps)) return false;
        if (!is_near(optimal_slip_ratio, p.optimal_slip_ratio, eps)) return false;
        if (!is_near(steering_shaft_smoothing, p.steering_shaft_smoothing, eps)) return false;
        if (!is_near(gyro_smoothing, p.gyro_smoothing, eps)) return false;
        if (!is_near(yaw_smoothing, p.yaw_smoothing, eps)) return false;
        if (!is_near(chassis_smoothing, p.chassis_smoothing, eps)) return false;

        if (flatspot_suppression != p.flatspot_suppression) return false;
        if (!is_near(notch_q, p.notch_q, eps)) return false;
        if (!is_near(flatspot_strength, p.flatspot_strength, eps)) return false;

        if (static_notch_enabled != p.static_notch_enabled) return false;
        if (!is_near(static_notch_freq, p.static_notch_freq, eps)) return false;
        if (!is_near(static_notch_width, p.static_notch_width, eps)) return false;
        if (!is_near(yaw_kick_threshold, p.yaw_kick_threshold, eps)) return false;

        if (!is_near(speed_gate_lower, p.speed_gate_lower, eps)) return false;
        if (!is_near(speed_gate_upper, p.speed_gate_upper, eps)) return false;

        if (!is_near(road_fallback_scale, p.road_fallback_scale, eps)) return false;
        if (understeer_affects_sop != p.understeer_affects_sop) return false;

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

private:
    // Helper for parsing preset lines (v0.7.12)
    static void ParsePresetLine(const std::string& line, Preset& p, std::string& version, bool& needs_save);
    // Helper for writing preset fields (v0.7.12)
    static void WritePresetFields(std::ofstream& file, const Preset& p);
};




#endif





