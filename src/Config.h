#ifndef CONFIG_H
#define CONFIG_H

#include "../FFBEngine.h"
#include <string>
#include <vector>

struct Preset {
    std::string name;
    
    // 1. Define Defaults inline (Matches "Default" preset logic)
    float gain = 0.5f;
    float understeer = 1.0f;
    float sop = 0.15f;
    float sop_scale = 20.0f;
    float sop_smoothing = 0.05f;
    float min_force = 0.0f;
    float oversteer_boost = 0.0f;
    
    bool lockup_enabled = false;
    float lockup_gain = 0.5f;
    
    bool spin_enabled = false;
    float spin_gain = 0.5f;
    
    bool slide_enabled = true;
    float slide_gain = 0.5f;
    
    bool road_enabled = false;
    float road_gain = 0.5f;
    
    bool invert_force = false;
    float max_torque_ref = 40.0f;
    
    bool use_manual_slip = false;
    int bottoming_method = 0;
    float scrub_drag_gain = 0.0f;
    
    float rear_align_effect = 1.0f;
    float sop_yaw_gain = 0.0f; // New v0.4.15
    float gyro_gain = 0.0f; // New v0.4.17
    
    float steering_shaft_gain = 1.0f;
    int base_force_mode = 0; // 0=Native

    // 2. Constructor
    Preset(std::string n) : name(n) {}
    Preset() : name("Unnamed") {} // Default constructor for file loading

    // 3. Fluent Setters (The "Python Dictionary" feel)
    Preset& SetGain(float v) { gain = v; return *this; }
    Preset& SetUndersteer(float v) { understeer = v; return *this; }
    Preset& SetSoP(float v) { sop = v; return *this; }
    Preset& SetSoPScale(float v) { sop_scale = v; return *this; }
    Preset& SetSmoothing(float v) { sop_smoothing = v; return *this; }
    Preset& SetMinForce(float v) { min_force = v; return *this; }
    Preset& SetOversteer(float v) { oversteer_boost = v; return *this; }
    
    Preset& SetLockup(bool enabled, float g) { lockup_enabled = enabled; lockup_gain = g; return *this; }
    Preset& SetSpin(bool enabled, float g) { spin_enabled = enabled; spin_gain = g; return *this; }
    Preset& SetSlide(bool enabled, float g) { slide_enabled = enabled; slide_gain = g; return *this; }
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

    // Apply this preset to an engine instance
    void Apply(FFBEngine& engine) const {
        engine.m_gain = gain;
        engine.m_understeer_effect = understeer;
        engine.m_sop_effect = sop;
        engine.m_sop_scale = sop_scale;
        engine.m_sop_smoothing_factor = sop_smoothing;
        engine.m_min_force = min_force;
        engine.m_oversteer_boost = oversteer_boost;
        engine.m_lockup_enabled = lockup_enabled;
        engine.m_lockup_gain = lockup_gain;
        engine.m_spin_enabled = spin_enabled;
        engine.m_spin_gain = spin_gain;
        engine.m_slide_texture_enabled = slide_enabled;
        engine.m_slide_texture_gain = slide_gain;
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

    // Global App Settings (not part of FFB Physics)
    static bool m_ignore_vjoy_version_warning;
    static bool m_enable_vjoy;        // Acquire vJoy device (Driver Enabled)
    static bool m_output_ffb_to_vjoy; // Output FFB signal to vJoy Axis X (Monitor)
};

#endif
