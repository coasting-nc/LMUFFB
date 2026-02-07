#ifndef PRESET_REGISTRY_H
#define PRESET_REGISTRY_H

#include "FFBEngine.h"
#include "Version.h"
#include <string>
#include <vector>
#include <chrono>
#include <fstream>

struct Preset {
    std::string name;
    bool is_builtin = false;
    std::string app_version = LMUFFB_VERSION;

    float gain = 1.0f;
    float understeer = 1.0f;
    float sop = 1.666f;
    float sop_scale = 1.0f;
    float sop_smoothing = 1.0f;
    float slip_smoothing = 0.002f;
    float min_force = 0.0f;
    float oversteer_boost = 2.52101f;

    bool lockup_enabled = true;
    float lockup_gain = 0.37479f;
    float lockup_start_pct = 1.0f;
    float lockup_full_pct = 5.0f;
    float lockup_rear_boost = 10.0f;
    float lockup_gamma = 0.1f;
    float lockup_prediction_sens = 10.0f;
    float lockup_bump_reject = 0.1f;
    float brake_load_cap = 2.0f;
    float texture_load_cap = 1.5f;

    bool abs_pulse_enabled = false;
    float abs_gain = 2.0f;
    float abs_freq = 25.5f;

    bool spin_enabled = true;
    float spin_gain = 0.5f;
    float spin_freq_scale = 1.0f;

    bool slide_enabled = false;
    float slide_gain = 0.226562f;
    float slide_freq = 1.0f;

    bool road_enabled = true;
    float road_gain = 0.0f;

    bool invert_force = true;
    float max_torque_ref = 100.0f;

    float lockup_freq_scale = 1.02f;
    int bottoming_method = 0;
    float scrub_drag_gain = 0.0f;

    float rear_align_effect = 0.666f;
    float sop_yaw_gain = 0.333f;
    float gyro_gain = 0.0f;

    float steering_shaft_gain = 1.0f;
    int base_force_mode = 0;

    float optimal_slip_angle = 0.1f;
    float optimal_slip_ratio = 0.12f;
    float steering_shaft_smoothing = 0.0f;

    float gyro_smoothing = 0.0f;
    float yaw_smoothing = 0.001f;
    float chassis_smoothing = 0.0f;

    bool flatspot_suppression = false;
    float notch_q = 2.0f;
    float flatspot_strength = 1.0f;

    bool static_notch_enabled = false;
    float static_notch_freq = 11.0f;
    float static_notch_width = 2.0f;
    float yaw_kick_threshold = 0.0f;

    float speed_gate_lower = 1.0f;
    float speed_gate_upper = 5.0f;

    float road_fallback_scale = 0.05f;
    bool understeer_affects_sop = false;

    bool slope_detection_enabled = false;
    int slope_sg_window = 15;
    float slope_sensitivity = 0.5f;
    float slope_negative_threshold = -0.3f;
    float slope_smoothing_tau = 0.04f;

    float slope_alpha_threshold = 0.02f;
    float slope_decay_rate = 5.0f;
    bool slope_confidence_enabled = true;

    float slope_min_threshold = -0.3f;
    float slope_max_threshold = -2.0f;

    Preset(std::string n, bool builtin = false) : name(n), is_builtin(builtin), app_version(LMUFFB_VERSION) {}
    Preset() : name("Unnamed"), is_builtin(false), app_version(LMUFFB_VERSION) {}

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

    static void ApplyDefaultsToEngine(FFBEngine& engine) {
        Preset defaults;
        defaults.Apply(engine);
    }

    void Apply(FFBEngine& engine) const;
    void UpdateFromEngine(const FFBEngine& engine);
    void Validate();
};

class PresetRegistry {
public:
    static PresetRegistry& Get() { static PresetRegistry instance; return instance; }

    void Load(const std::string& config_path);
    void Save(const FFBEngine& engine, const std::string& path);
    void WritePresets(std::ofstream& file);

    const std::vector<Preset>& GetPresets() const { return m_presets; }
    void ApplyPreset(int index, FFBEngine& engine);

    void AddUserPreset(const std::string& name, const FFBEngine& engine);
    void DeletePreset(int index, const FFBEngine& engine);
    void DuplicatePreset(int index, const FFBEngine& engine);

    bool ImportPreset(const std::string& filename, const FFBEngine& engine);
    void ExportPreset(int index, const std::string& filename);

    bool IsDirty(int index, const FFBEngine& engine) const;

    std::string GetLastPresetName() const { return m_last_preset_name; }
    void SetLastPresetName(const std::string& name) { m_last_preset_name = name; }

private:
    std::vector<Preset> m_presets;
    std::string m_last_preset_name = "Default";

    void LoadBuiltins();
    size_t GetUserInsertionPoint() const;

    void ParsePresetLine(const std::string& line, Preset& p, std::string& version, bool& needs_save);
    void WritePresetFields(std::ofstream& file, const Preset& p);
};

#endif
