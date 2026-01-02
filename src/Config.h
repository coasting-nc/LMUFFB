#ifndef CONFIG_H
#define CONFIG_H

#include "FFBEngine.h"
#include <string>
#include <vector>

struct Preset {
    std::string name;
    bool is_builtin = false; // NEW: Track if this is hardcoded or user-created
    
    // 1. SINGLE SOURCE OF TRUTH: T300 Default Values
    // These defaults are used by:
    // - FFBEngine constructor (via ApplyDefaultsToEngine)
    // - "Default (T300)" preset in LoadPresets()
    // - "Reset Defaults" button in GUI
   float gain = 1.0f;
    float understeer = 1.0f;  // New scale: 0.0-2.0, where 1.0 = proportional
    float sop = 1.5f;
    float sop_scale = 1.0f;
    float sop_smoothing = 1.0f;
    float slip_smoothing = 0.002f;
    float min_force = 0.0f;
    float oversteer_boost = 2.0f;
    
    bool lockup_enabled = true;
    float lockup_gain = 2.0f;
    float lockup_start_pct = 1.0f;  // New v0.5.11
    float lockup_full_pct = 5.0f;  // New v0.5.11
    float lockup_rear_boost = 3.0f; // New v0.5.11
    float lockup_gamma = 0.5f;           // New v0.6.0
    float lockup_prediction_sens = 20.0f; // New v0.6.0
    float lockup_bump_reject = 0.1f;     // New v0.6.0
    float brake_load_cap = 3.0f;    // New v0.5.11
    float texture_load_cap = 1.5f;  // NEW v0.6.25
    
    bool abs_pulse_enabled = true;       // New v0.6.0
    float abs_gain = 2.0f;               // New v0.6.0
    float abs_freq = 20.0f;              // New v0.6.20
    
    bool spin_enabled = false;
    float spin_gain = 0.5f;
    float spin_freq_scale = 1.0f;        // New v0.6.20
    
    bool slide_enabled = true;
    float slide_gain = 0.39f;
    float slide_freq = 1.0f;
    
    bool road_enabled = true;
    float road_gain = 0.5f;
    
    bool invert_force = true;
    float max_torque_ref = 100.0f; // T300 Calibrated
    
    float lockup_freq_scale = 1.0f;      // New v0.6.20
    int bottoming_method = 0;
    float scrub_drag_gain = 0.0f;
    
    float rear_align_effect = 1.0084f;
    float sop_yaw_gain = 0.0504202f;
    float gyro_gain = 0.0336134f;
    
    float steering_shaft_gain = 1.0f;
    int base_force_mode = 0; // 0=Native
    
    // NEW: Grip & Smoothing (v0.5.7)
    float optimal_slip_angle = 0.1f;
    float optimal_slip_ratio = 0.12f;
    float steering_shaft_smoothing = 0.0f;
    
    // NEW: Advanced Smoothing (v0.5.8)
    float gyro_smoothing = 0.0f;
    float yaw_smoothing = 0.015f;
    float chassis_smoothing = 0.0f;

    // v0.4.41: Signal Filtering
    bool flatspot_suppression = false;
    float notch_q = 2.0f;
    float flatspot_strength = 1.0f;
    
    bool static_notch_enabled = false;
    float static_notch_freq = 11.0f;
    float static_notch_width = 2.0f; // New v0.6.10
    float yaw_kick_threshold = 0.2f; // New v0.6.10

    // v0.6.23 New Settings with HIGHER DEFAULTS
    float speed_gate_lower = 1.0f; // 3.6 km/h
    float speed_gate_upper = 5.0f; // 18.0 km/h (Fixes idle shake)
    
    // Reserved for future implementation (v0.6.23+)
    float road_fallback_scale = 0.05f;      // Planned: Road texture fallback scaling
    bool understeer_affects_sop = false;     // Planned: Understeer modulation of SoP

    // 2. Constructors
    Preset(std::string n, bool builtin = false) : name(n), is_builtin(builtin) {}
    Preset() : name("Unnamed"), is_builtin(false) {} // Default constructor for file loading

    // 3. Fluent Setters (The "Python Dictionary" feel)
    Preset& SetGain(float v) { gain = v; return *this; }
    Preset& SetUndersteer(float v) { understeer = v; return *this; }
    Preset& SetSoP(float v) { sop = v; return *this; }
    Preset& SetSoPScale(float v) { sop_scale = v; return *this; }
    Preset& SetSmoothing(float v) { sop_smoothing = v; return *this; }
    Preset& SetMinForce(float v) { min_force = v; return *this; }
    Preset& SetOversteer(float v) { oversteer_boost = v; return *this; }
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
    
    Preset& SetInvert(bool v) { invert_force = v; return *this; }
    Preset& SetMaxTorque(float v) { max_torque_ref = v; return *this; }
    
    Preset& SetBottoming(int method) { bottoming_method = method; return *this; }
    Preset& SetScrub(float v) { scrub_drag_gain = v; return *this; }
    Preset& SetRearAlign(float v) { rear_align_effect = v; return *this; }
    Preset& SetSoPYaw(float v) { sop_yaw_gain = v; return *this; }
    Preset& SetGyro(float v) { gyro_gain = v; return *this; }
    
    Preset& SetShaftGain(float v) { steering_shaft_gain = v; return *this; }
    Preset& SetBaseMode(int v) { base_force_mode = v; return *this; }
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
    
    // Advanced Braking (v0.6.0)
    Preset& SetAdvancedBraking(float gamma, float sens, float bump, bool abs, float abs_g, float abs_f = 20.0f, float lockup_f = 1.0f) {
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
    void Apply(FFBEngine& engine) const {
        engine.m_gain = gain;
        engine.m_understeer_effect = understeer;
        engine.m_sop_effect = sop;
        engine.m_sop_scale = sop_scale;
        engine.m_sop_smoothing_factor = sop_smoothing;
        engine.m_slip_angle_smoothing = slip_smoothing;
        engine.m_min_force = min_force;
        engine.m_oversteer_boost = oversteer_boost;
        engine.m_lockup_enabled = lockup_enabled;
        engine.m_lockup_gain = lockup_gain;
        engine.m_lockup_start_pct = lockup_start_pct;
        engine.m_lockup_full_pct = lockup_full_pct;
        engine.m_lockup_rear_boost = lockup_rear_boost;
        engine.m_lockup_gamma = lockup_gamma;
        engine.m_lockup_prediction_sens = lockup_prediction_sens;
        engine.m_lockup_bump_reject = lockup_bump_reject;
        engine.m_brake_load_cap = brake_load_cap;
        engine.m_texture_load_cap = texture_load_cap;  // NEW v0.6.25
        engine.m_abs_pulse_enabled = abs_pulse_enabled;
        engine.m_abs_gain = abs_gain;

        engine.m_spin_enabled = spin_enabled;
        engine.m_spin_gain = spin_gain;
        engine.m_slide_texture_enabled = slide_enabled;
        engine.m_slide_texture_gain = slide_gain;
        engine.m_slide_freq_scale = slide_freq;
        engine.m_road_texture_enabled = road_enabled;
        engine.m_road_texture_gain = road_gain;
        engine.m_invert_force = invert_force;
        engine.m_max_torque_ref = max_torque_ref;
        engine.m_abs_freq_hz = abs_freq;
        engine.m_lockup_freq_scale = lockup_freq_scale;
        engine.m_spin_freq_scale = spin_freq_scale;
        engine.m_bottoming_method = bottoming_method;
        engine.m_scrub_drag_gain = scrub_drag_gain;
        engine.m_rear_align_effect = rear_align_effect;
        engine.m_sop_yaw_gain = sop_yaw_gain;
        engine.m_gyro_gain = gyro_gain;
        engine.m_steering_shaft_gain = steering_shaft_gain;
        engine.m_base_force_mode = base_force_mode;
        engine.m_flatspot_suppression = flatspot_suppression;
        engine.m_notch_q = notch_q;
        engine.m_flatspot_strength = flatspot_strength;
        engine.m_static_notch_enabled = static_notch_enabled;
        engine.m_static_notch_freq = static_notch_freq;
        engine.m_static_notch_width = static_notch_width;
        engine.m_yaw_kick_threshold = yaw_kick_threshold;
        engine.m_speed_gate_lower = speed_gate_lower;
        engine.m_speed_gate_upper = speed_gate_upper;
        engine.m_road_fallback_scale = road_fallback_scale;
        engine.m_understeer_affects_sop = understeer_affects_sop;
        engine.m_optimal_slip_angle = optimal_slip_angle;
        engine.m_optimal_slip_ratio = optimal_slip_ratio;
        engine.m_steering_shaft_smoothing = steering_shaft_smoothing;
        engine.m_gyro_smoothing = gyro_smoothing;
        engine.m_yaw_accel_smoothing = yaw_smoothing;
        engine.m_chassis_inertia_smoothing = chassis_smoothing;
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
        flatspot_suppression = engine.m_flatspot_suppression;
        notch_q = engine.m_notch_q;
        flatspot_strength = engine.m_flatspot_strength;
        static_notch_enabled = engine.m_static_notch_enabled;
        static_notch_freq = engine.m_static_notch_freq;
        static_notch_width = engine.m_static_notch_width;
        yaw_kick_threshold = engine.m_yaw_kick_threshold;
        speed_gate_lower = engine.m_speed_gate_lower;
        speed_gate_upper = engine.m_speed_gate_upper;
        road_fallback_scale = engine.m_road_fallback_scale;
        understeer_affects_sop = engine.m_understeer_affects_sop;
        optimal_slip_angle = engine.m_optimal_slip_angle;
        optimal_slip_ratio = engine.m_optimal_slip_ratio;
        steering_shaft_smoothing = engine.m_steering_shaft_smoothing;
        gyro_smoothing = engine.m_gyro_smoothing;
        yaw_smoothing = engine.m_yaw_accel_smoothing;
        chassis_smoothing = engine.m_chassis_inertia_smoothing;
    }
};

class Config {
public:
    static std::string m_config_path; // Default: "config.ini"
    static void Save(const FFBEngine& engine, const std::string& filename = "");
    static void Load(FFBEngine& engine, const std::string& filename = "");
    
    // Preset Management
    static std::vector<Preset> presets;
    static void LoadPresets(); // Populates presets vector
    static void ApplyPreset(int index, FFBEngine& engine);
    
    // NEW: Add a user preset
    static void AddUserPreset(const std::string& name, const FFBEngine& engine);

    // NEW: Persist selected device
    static std::string m_last_device_guid;

    // Global App Settings (not part of FFB Physics)
    static bool m_ignore_vjoy_version_warning;
    static bool m_enable_vjoy;        // Acquire vJoy device (Driver Enabled)
    static bool m_output_ffb_to_vjoy; // Output FFB signal to vJoy Axis X (Monitor)
    static bool m_always_on_top;      // NEW: Keep window on top

    // Window Geometry Persistence (v0.5.5)
    static int win_pos_x, win_pos_y;
    static int win_w_small, win_h_small; // Dimensions for Config Only
    static int win_w_large, win_h_large; // Dimensions for Config + Graphs
    static bool show_graphs;             // Remember if graphs were open
};

#endif
