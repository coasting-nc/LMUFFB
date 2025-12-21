#ifndef CONFIG_H
#define CONFIG_H

#include "../FFBEngine.h"
#include <string>
#include <vector>

struct Preset {
    std::string name;
    bool is_builtin = false; // NEW: Track if this is hardcoded or user-created
    
    // 1. Define Defaults inline (Matches "Default" preset logic)
    float gain = 1.0f;
    float understeer = 0.61f; // Calibrated from Image
    float sop = 0.08f;        // Calibrated from Image
    float sop_scale = 1.0f;   // Calibrated from Image
    float sop_smoothing = 0.85f;
    float slip_smoothing = 0.015f;
    float min_force = 0.0f;
    float oversteer_boost = 0.65f; // Calibrated from Image
    
    bool lockup_enabled = false;
    float lockup_gain = 0.5f;
    
    bool spin_enabled = false;
    float spin_gain = 0.5f;
    
    bool slide_enabled = true; // Enabled in Image
    float slide_gain = 0.39f;  // Calibrated from Image
    float slide_freq = 1.0f;    // NEW: Frequency Multiplier (v0.4.36)
    
    bool road_enabled = false;
    float road_gain = 0.5f;
    
    bool invert_force = true;
    float max_torque_ref = 98.3f; // Calibrated from Image
    
    bool use_manual_slip = false;
    int bottoming_method = 0;
    float scrub_drag_gain = 0.0f;
    
    float rear_align_effect = 0.90f; // Calibrated from Image
    float sop_yaw_gain = 0.0f;       // Calibrated from Image
    float gyro_gain = 0.0f;
    
    float steering_shaft_gain = 1.0f;
    int base_force_mode = 0; // 0=Native

    // v0.4.41: Signal Filtering
    bool flatspot_suppression = false;
    float notch_q = 2.0f;
    float flatspot_strength = 1.0f;
    
    bool static_notch_enabled = false;
    float static_notch_freq = 50.0f;

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
    
    Preset& SetLockup(bool enabled, float g) { lockup_enabled = enabled; lockup_gain = g; return *this; }
    Preset& SetSpin(bool enabled, float g) { spin_enabled = enabled; spin_gain = g; return *this; }
    Preset& SetSlide(bool enabled, float g, float f = 1.0f) { 
        slide_enabled = enabled; 
        slide_gain = g; 
        slide_freq = f; 
        return *this; 
    }
    Preset& SetRoad(bool enabled, float g) { road_enabled = enabled; road_gain = g; return *this; }
    
    Preset& SetInvert(bool v) { invert_force = v; return *this; }
    Preset& SetMaxTorque(float v) { max_torque_ref = v; return *this; }
    
    Preset& SetManualSlip(bool v) { use_manual_slip = v; return *this; }
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
    
    Preset& SetStaticNotch(bool enabled, float freq) {
        static_notch_enabled = enabled;
        static_notch_freq = freq;
        return *this;
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
        engine.m_spin_enabled = spin_enabled;
        engine.m_spin_gain = spin_gain;
        engine.m_slide_texture_enabled = slide_enabled;
        engine.m_slide_texture_gain = slide_gain;
        engine.m_slide_freq_scale = slide_freq;
        engine.m_road_texture_enabled = road_enabled;
        engine.m_road_texture_gain = road_gain;
        engine.m_invert_force = invert_force;
        engine.m_max_torque_ref = max_torque_ref;
        engine.m_use_manual_slip = use_manual_slip;
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
        spin_enabled = engine.m_spin_enabled;
        spin_gain = engine.m_spin_gain;
        slide_enabled = engine.m_slide_texture_enabled;
        slide_gain = engine.m_slide_texture_gain;
        slide_freq = engine.m_slide_freq_scale;
        road_enabled = engine.m_road_texture_enabled;
        road_gain = engine.m_road_texture_gain;
        invert_force = engine.m_invert_force;
        max_torque_ref = engine.m_max_torque_ref;
        use_manual_slip = engine.m_use_manual_slip;
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
    }
};

class Config {
public:
    static void Save(const FFBEngine& engine, const std::string& filename = "config.ini");
    static void Load(FFBEngine& engine, const std::string& filename = "config.ini");
    
    // Preset Management
    static std::vector<Preset> presets;
    static void LoadPresets(); // Populates presets vector
    static void ApplyPreset(int index, FFBEngine& engine);
    
    // NEW: Add a user preset
    static void AddUserPreset(const std::string& name, const FFBEngine& engine);

    // Global App Settings (not part of FFB Physics)
    static bool m_ignore_vjoy_version_warning;
    static bool m_enable_vjoy;        // Acquire vJoy device (Driver Enabled)
    static bool m_output_ffb_to_vjoy; // Output FFB signal to vJoy Axis X (Monitor)
};

#endif
