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

namespace LMUFFB {

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
    FFB::GeneralConfig general;
    FFB::FrontAxleConfig front_axle;
    FFB::RearAxleConfig rear_axle;
    FFB::LoadForcesConfig load_forces;
    FFB::GripEstimationConfig grip_estimation;
    FFB::SlopeDetectionConfig slope_detection;
    FFB::BrakingConfig braking;
    FFB::VibrationConfig vibration;
    FFB::AdvancedConfig advanced;
    FFB::SafetyConfig safety;

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
    Preset& SetLongitudinalLoad(float v) { load_forces.long_load_effect = v; return *this; }
    Preset& SetLongitudinalLoadSmoothing(float v) { load_forces.long_load_smoothing = v; return *this; }
    Preset& SetLongitudinalLoadTransform(int v) { load_forces.long_load_transform = v; return *this; }
    Preset& SetGripSmoothing(float steady, float fast, float sens) {
        grip_estimation.grip_smoothing_steady = steady;
        grip_estimation.grip_smoothing_fast = fast;
        grip_estimation.grip_smoothing_sensitivity = sens;
        return *this;
    }
    Preset& SetSlipSmoothing(float v) { grip_estimation.slip_angle_smoothing = v; return *this; }
    
    Preset& SetLockup(bool enabled, float g, float start = 5.0f, float full = 15.0f, float boost = 1.5f) { 
        braking.lockup_enabled = enabled;
        braking.lockup_gain = g;
        braking.lockup_start_pct = start;
        braking.lockup_full_pct = full;
        braking.lockup_rear_boost = boost;
        return *this; 
    }
    Preset& SetBrakeCap(float v) { braking.brake_load_cap = v; return *this; }
    Preset& SetSpin(bool enabled, float g, float scale = 1.0f) { 
        vibration.spin_enabled = enabled;
        vibration.spin_gain = g;
        vibration.spin_freq_scale = scale;
        return *this; 
    }
    Preset& SetSlide(bool enabled, float g, float f = 1.0f) { 
        vibration.slide_enabled = enabled;
        vibration.slide_gain = g;
        vibration.slide_freq = f;
        return *this; 
    }
    Preset& SetRoad(bool enabled, float g) { vibration.road_enabled = enabled; vibration.road_gain = g; return *this; }
    Preset& SetVibrationGain(float v) { vibration.vibration_gain = v; return *this; }
    Preset& SetDynamicNormalization(bool enabled) { general.dynamic_normalization_enabled = enabled; return *this; }

    Preset& SetSoftLock(bool enabled, float stiffness, float damping) {
        advanced.soft_lock_enabled = enabled;
        advanced.soft_lock_stiffness = stiffness;
        advanced.soft_lock_damping = damping;
        return *this;
    }
    
    Preset& SetHardwareScaling(float wheelbase, float target) {
        general.wheelbase_max_nm = wheelbase;
        general.target_rim_nm = target;
        return *this;
    }
    
    Preset& SetBottoming(bool enabled, float gain, int method) {
        vibration.bottoming_enabled = enabled;
        vibration.bottoming_gain = gain;
        vibration.bottoming_method = method;
        return *this;
    }
    Preset& SetScrub(float v) { vibration.scrub_drag_gain = v; return *this; }
    Preset& SetRearAlign(float v) { rear_axle.rear_align_effect = v; return *this; }
    Preset& SetKerbStrikeRejection(float v) { rear_axle.kerb_strike_rejection = v; return *this; }
    Preset& SetSoPYaw(float v) { rear_axle.sop_yaw_gain = v; return *this; }
    Preset& SetGyro(float v) { advanced.gyro_gain = v; return *this; }
    Preset& SetStationaryDamping(float v) { advanced.stationary_damping = v; return *this; }
    
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

    Preset& SetSpeedGate(float lower, float upper) { advanced.speed_gate_lower = lower; advanced.speed_gate_upper = upper; return *this; }

    Preset& SetOptimalSlip(float angle, float ratio) {
        grip_estimation.optimal_slip_angle = angle;
        grip_estimation.optimal_slip_ratio = ratio;
        return *this;
    }
    Preset& SetAuxTelemetryReconstruction(int v) { advanced.aux_telemetry_reconstruction = v; return *this; }
    Preset& SetShaftSmoothing(float v) { front_axle.steering_shaft_smoothing = v; return *this; }
    
    Preset& SetGyroSmoothing(float v) { advanced.gyro_smoothing = v; return *this; }
    Preset& SetYawSmoothing(float v) { rear_axle.yaw_accel_smoothing = v; return *this; }
    Preset& SetChassisSmoothing(float v) { grip_estimation.chassis_inertia_smoothing = v; return *this; }
    
    Preset& SetSlopeDetection(bool enabled, int window = 15, float min_thresh = -0.3f, float max_thresh = -2.0f, float tau = 0.04f) {
        slope_detection.enabled = enabled;
        slope_detection.sg_window = window;
        slope_detection.min_threshold = min_thresh;
        slope_detection.max_threshold = max_thresh;
        slope_detection.smoothing_tau = tau;
        return *this;
    }

    Preset& SetSlopeStability(float alpha_thresh = 0.02f, float decay = 5.0f, bool conf = true) {
        slope_detection.alpha_threshold = alpha_thresh;
        slope_detection.decay_rate = decay;
        slope_detection.confidence_enabled = conf;
        return *this;
    }

    Preset& SetSlopeAdvanced(float slew = 50.0f, bool use_torque = true, float torque_sens = 0.5f) {
        slope_detection.g_slew_limit = slew;
        slope_detection.use_torque = use_torque;
        slope_detection.torque_sensitivity = torque_sens;
        return *this;
    }

    Preset& SetRestApiFallback(bool enabled, int port = 6397) {
        advanced.rest_api_enabled = enabled;
        advanced.rest_api_port = port;
        return *this;
    }

    Preset& SetSafety(float duration, float gain, float smoothing, float threshold, float immediate, float slew, bool stutter = false, float stutter_thresh = 1.5f) {
        safety.window_duration = duration;
        safety.gain_reduction = gain;
        safety.smoothing_tau = smoothing;
        safety.spike_detection_threshold = threshold;
        safety.immediate_spike_threshold = immediate;
        safety.slew_full_scale_time_s = slew;
        safety.stutter_safety_enabled = stutter;
        safety.stutter_threshold = stutter_thresh;
        return *this;
    }

    // Advanced Braking (v0.6.0)
    // âš ï¸ IMPORTANT: Default parameters (abs_f, lockup_f) must match Config.h defaults!
    // When changing Config.h defaults, update these values to match.
    // Current: abs_f=25.5, lockup_f=1.02 (GT3 DD 15 Nm defaults - v0.6.35)
    Preset& SetAdvancedBraking(float gamma, float sens, float bump, bool abs, float abs_g, float abs_f = 25.5f, float lockup_f = 1.02f) {
        braking.lockup_gamma = gamma;
        braking.lockup_prediction_sens = sens;
        braking.lockup_bump_reject = bump;
        braking.abs_pulse_enabled = abs;
        braking.abs_gain = abs_g;
        braking.abs_freq = abs_f;
        braking.lockup_freq_scale = lockup_f;
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

        engine.m_load_forces = this->load_forces;
        engine.m_load_forces.Validate();

        engine.m_grip_estimation = this->grip_estimation;
        engine.m_grip_estimation.Validate();

        engine.m_slope_detection = this->slope_detection;
        engine.m_slope_detection.Validate();

        engine.m_braking = this->braking;
        engine.m_braking.Validate();

        engine.m_vibration = this->vibration;
        engine.m_vibration.Validate();

        engine.m_advanced = this->advanced;
        engine.m_advanced.Validate();

        engine.m_safety.m_config = this->safety;
        engine.m_safety.m_config.Validate();

        // Update upsamplers after config move
        engine.UpdateUpsamplerModes();

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
        load_forces.Validate();
        grip_estimation.Validate();
        slope_detection.Validate();
        braking.Validate();
        vibration.Validate();
        advanced.Validate();
        safety.Validate();
    }

    // NEW: Capture current engine state into this preset
    void UpdateFromEngine(const FFBEngine& engine) {
        general = engine.m_general;
        front_axle = engine.m_front_axle;
        rear_axle = engine.m_rear_axle;
        load_forces = engine.m_load_forces;
        grip_estimation = engine.m_grip_estimation;
        slope_detection = engine.m_slope_detection;
        braking = engine.m_braking;
        vibration = engine.m_vibration;
        advanced = engine.m_advanced;
        safety = engine.m_safety.m_config;

        app_version = LMUFFB_VERSION;
    }

    bool Equals(const Preset& p) const {
        const float eps = 0.0001f;
        auto is_near = [](float a, float b, float epsilon) { return std::abs(a - b) < epsilon; };

        if (!general.Equals(p.general, eps)) return false;
        if (!front_axle.Equals(p.front_axle, eps)) return false;
        if (!rear_axle.Equals(p.rear_axle, eps)) return false;
        if (!load_forces.Equals(p.load_forces, eps)) return false;
        if (!grip_estimation.Equals(p.grip_estimation, eps)) return false;
        if (!slope_detection.Equals(p.slope_detection, eps)) return false;
        if (!braking.Equals(p.braking, eps)) return false;
        if (!vibration.Equals(p.vibration, eps)) return false;
        if (!advanced.Equals(p.advanced, eps)) return false;
        if (!safety.Equals(p.safety, eps)) return false;

        return true;
    }
};

class Config {
public:
    static std::string m_config_path; // Default: "config.toml"
    static std::string m_user_presets_path; // Default: "user_presets"
    static void Save(const FFBEngine& engine, const std::string& filename = "");
    static void Load(FFBEngine& engine, const std::string& filename = "");

    // Phase 2: Legacy Migration
    static void MigrateFromLegacyIni(FFBEngine& engine, const std::string& filename);
    
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

} // namespace LMUFFB

#endif





