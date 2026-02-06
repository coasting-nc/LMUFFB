#include "PresetRegistry.h"
#include "Config.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>

void Preset::Apply(FFBEngine& engine) const {
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
    engine.m_texture_load_cap = texture_load_cap;
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
    engine.m_optimal_slip_angle = optimal_slip_angle;
    engine.m_optimal_slip_ratio = optimal_slip_ratio;
    engine.m_steering_shaft_smoothing = steering_shaft_smoothing;
    engine.m_gyro_smoothing = gyro_smoothing;
    engine.m_yaw_accel_smoothing = yaw_smoothing;
    engine.m_chassis_inertia_smoothing = chassis_smoothing;
    engine.m_road_fallback_scale = road_fallback_scale;
    engine.m_understeer_affects_sop = understeer_affects_sop;
    engine.m_slope_detection_enabled = slope_detection_enabled;
    engine.m_slope_sg_window = slope_sg_window;
    engine.m_slope_sensitivity = slope_sensitivity;
    engine.m_slope_negative_threshold = slope_negative_threshold;
    engine.m_slope_smoothing_tau = slope_smoothing_tau;
    engine.m_slope_alpha_threshold = slope_alpha_threshold;
    engine.m_slope_decay_rate = slope_decay_rate;
    engine.m_slope_confidence_enabled = slope_confidence_enabled;
    engine.m_slope_min_threshold = slope_min_threshold;
    engine.m_slope_max_threshold = slope_max_threshold;
}

void Preset::UpdateFromEngine(const FFBEngine& engine) {
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
    texture_load_cap = engine.m_texture_load_cap;
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
    optimal_slip_angle = engine.m_optimal_slip_angle;
    optimal_slip_ratio = engine.m_optimal_slip_ratio;
    steering_shaft_smoothing = engine.m_steering_shaft_smoothing;
    gyro_smoothing = engine.m_gyro_smoothing;
    yaw_smoothing = engine.m_yaw_accel_smoothing;
    chassis_smoothing = engine.m_chassis_inertia_smoothing;
    road_fallback_scale = engine.m_road_fallback_scale;
    understeer_affects_sop = engine.m_understeer_affects_sop;
    slope_detection_enabled = engine.m_slope_detection_enabled;
    slope_sg_window = engine.m_slope_sg_window;
    slope_sensitivity = engine.m_slope_sensitivity;
    slope_negative_threshold = engine.m_slope_negative_threshold;
    slope_smoothing_tau = engine.m_slope_smoothing_tau;
    slope_alpha_threshold = engine.m_slope_alpha_threshold;
    slope_decay_rate = engine.m_slope_decay_rate;
    slope_confidence_enabled = engine.m_slope_confidence_enabled;
    slope_min_threshold = engine.m_slope_min_threshold;
    slope_max_threshold = engine.m_slope_max_threshold;
    app_version = LMUFFB_VERSION;
}

void PresetRegistry::Load(const std::string& config_path) {
    m_presets.clear();
    m_presets.push_back(Preset("Default", true));
    std::ifstream file(config_path);
    if (file.is_open()) {
        std::string line; bool needs_save = false; std::string current_preset_name = "";
        Preset current_preset; std::string current_preset_version = ""; bool preset_pending = false;
        bool has_max_threshold = false;
        while (std::getline(file, line)) {
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            if (line.empty() || line[0] == ';') continue;
            if (line[0] == '[') {
                if (preset_pending && !current_preset_name.empty()) {
                    current_preset.name = current_preset_name; current_preset.is_builtin = false;
                    current_preset.app_version = current_preset_version.empty() ? LMUFFB_VERSION : current_preset_version;

                    // Legacy migration for slope_max_threshold in presets
                    if (!has_max_threshold && current_preset.slope_sensitivity > 0.1f) {
                        current_preset.slope_max_threshold = current_preset.slope_min_threshold - (8.0f / current_preset.slope_sensitivity);
                    }

                    m_presets.push_back(current_preset);
                    preset_pending = false;
                }
                if (line.rfind("[Preset:", 0) == 0) {
                    size_t end_pos = line.find(']');
                    if (end_pos != std::string::npos) {
                        current_preset_name = line.substr(8, end_pos - 8);
                        current_preset = Preset(current_preset_name, false);
                        preset_pending = true; current_preset_version = "";
                        has_max_threshold = false;
                    }
                }
                continue;
            }
            if (preset_pending) {
                if (line.find("slope_max_threshold=") == 0) has_max_threshold = true;
                ParsePresetLine(line, current_preset, current_preset_version, needs_save);
            }
        }
        if (preset_pending && !current_preset_name.empty()) {
            current_preset.name = current_preset_name; current_preset.is_builtin = false;
            current_preset.app_version = current_preset_version.empty() ? LMUFFB_VERSION : current_preset_version;

            if (!has_max_threshold && current_preset.slope_sensitivity > 0.1f) {
                current_preset.slope_max_threshold = current_preset.slope_min_threshold - (8.0f / current_preset.slope_sensitivity);
            }

            m_presets.push_back(current_preset);
        }
    }
    LoadBuiltins();
}

void PresetRegistry::LoadBuiltins() {
    m_presets.push_back(Preset("T300", true).SetInvert(true).SetGain(1.0f).SetMaxTorque(100.1f).SetMinForce(0.01f).SetUndersteer(0.5f).SetOversteer(2.40336f).SetSoP(0.425003f).SetRearAlign(0.966383f).SetSoPYaw(0.386555f).SetYawKickThreshold(1.68f).SetYawSmoothing(0.005f).SetSmoothing(1.0f).SetOptimalSlip(0.10f, 0.12f).SetLockup(true, 2.0f, 1.0f, 5.0f, 10.0f).SetBrakeCap(10.0f).SetSlide(true, 0.235294f).SetRoad(true, 2.0f).SetSpin(true, 0.5f).SetScrub(0.0462185f));
    m_presets.push_back(Preset("GT3 DD 15 Nm (Simagic Alpha)", true).SetGain(1.0f).SetMaxTorque(100.0f).SetOversteer(2.52101f).SetSoP(1.666f).SetRearAlign(0.666f).SetSoPYaw(0.333f).SetSmoothing(0.99f).SetSoPScale(1.98f).SetOptimalSlip(0.1f, 0.12f).SetSlipSmoothing(0.002f).SetChassisSmoothing(0.012f).SetLockup(true, 0.37479f, 1.0f, 7.5f, 1.0f).SetBrakeCap(2.0f).SetSpeedGate(1.0f, 5.0f).SetRoad(true, 0.0f).SetScrub(0.333f));
    m_presets.push_back(Preset("LMPx/HY DD 15 Nm (Simagic Alpha)", true).SetGain(1.0f).SetMaxTorque(100.0f).SetOversteer(2.52101f).SetSoP(1.666f).SetRearAlign(0.666f).SetOptimalSlip(0.12f, 0.12f).SetSmoothing(0.97f).SetSoPScale(1.59f).SetSlipSmoothing(0.003f).SetChassisSmoothing(0.019f).SetLockup(true, 0.37479f, 1.0f, 7.5f, 1.0f).SetBrakeCap(2.0f).SetSpeedGate(1.0f, 5.0f));
    m_presets.push_back(Preset("GM DD 21 Nm (Moza R21 Ultra)", true).SetGain(1.454f).SetMaxTorque(100.1f).SetShaftGain(1.989f).SetUndersteer(0.638f).SetFlatspot(true, 1.0f, 0.57f).SetOversteer(0.0f).SetSoP(0.0f).SetRearAlign(0.29f).SetSoPYaw(0.0f).SetSoPScale(0.89f).SetSlipSmoothing(0.002f).SetLockup(true, 0.977f, 1.0f, 7.5f, 1.0f).SetBrakeCap(81.0f).SetSpeedGate(1.0f, 5.0f));
    m_presets.push_back(Preset("GM + Yaw Kick DD 21 Nm (Moza R21 Ultra)", true).SetGain(1.454f).SetMaxTorque(100.1f).SetShaftGain(1.989f).SetUndersteer(0.638f).SetFlatspot(true, 1.0f, 0.57f).SetOversteer(0.0f).SetSoP(0.0f).SetRearAlign(0.29f).SetSoPYaw(0.333f).SetSoPScale(0.89f).SetSlipSmoothing(0.002f).SetLockup(true, 0.977f, 1.0f, 7.5f, 1.0f).SetBrakeCap(81.0f).SetSpeedGate(1.0f, 5.0f).SetYawSmoothing(0.003f));
    m_presets.push_back(Preset("Test: Game Base FFB Only", true).SetUndersteer(0.0f).SetSoP(0.0f).SetBaseMode(0));
    m_presets.push_back(Preset("Test: SoP Only", true).SetUndersteer(0.0f).SetSoP(0.08f).SetBaseMode(2));
    m_presets.push_back(Preset("Test: Understeer Only", true).SetUndersteer(0.61f).SetSoP(0.0f).SetBaseMode(0));
}

size_t PresetRegistry::GetUserInsertionPoint() const {
    auto it = std::find_if(m_presets.begin() + 1, m_presets.end(), [](const Preset& pr) { return pr.is_builtin; });
    return std::distance(m_presets.begin(), it);
}

void PresetRegistry::ApplyPreset(int index, FFBEngine& engine) {
    if (index >= 0 && index < (int)m_presets.size()) {
        m_presets[index].Apply(engine);
        m_last_preset_name = m_presets[index].name;
        Config::Save(engine);
    }
}

void PresetRegistry::AddUserPreset(const std::string& name, const FFBEngine& engine) {
    bool found = false;
    for (auto& p : m_presets) {
        if (p.name == name && !p.is_builtin) { p.UpdateFromEngine(engine); found = true; break; }
    }
    if (!found) {
        Preset p(name, false); p.UpdateFromEngine(engine);
        m_presets.insert(m_presets.begin() + GetUserInsertionPoint(), p);
    }
    m_last_preset_name = name; Config::Save(engine);
}

void PresetRegistry::DeletePreset(int index, const FFBEngine& engine) {
    if (index < 0 || index >= (int)m_presets.size() || m_presets[index].is_builtin) return;
    std::string name = m_presets[index].name;
    m_presets.erase(m_presets.begin() + index);
    if (m_last_preset_name == name) m_last_preset_name = "Default";
    Config::Save(engine);
}

void PresetRegistry::DuplicatePreset(int index, const FFBEngine& engine) {
    if (index < 0 || index >= (int)m_presets.size()) return;
    Preset p = m_presets[index]; p.name = p.name + " (Copy)"; p.is_builtin = false;
    p.app_version = LMUFFB_VERSION;
    std::string base_name = p.name; int counter = 1; bool exists = true;
    while (exists) {
        exists = false;
        for (const auto& existing : m_presets) {
            if (existing.name == p.name) { p.name = base_name + " " + std::to_string(counter++); exists = true; break; }
        }
    }
    m_presets.insert(m_presets.begin() + GetUserInsertionPoint(), p);
    m_last_preset_name = p.name; Config::Save(engine);
}

bool PresetRegistry::IsDirty(int index, const FFBEngine& engine) const {
    if (index < 0 || index >= (int)m_presets.size()) return false;
    const Preset& p = m_presets[index]; const float eps = 0.0001f;
    auto is_near = [](float a, float b, float epsilon) { return std::abs(a - b) < epsilon; };
    if (!is_near(p.gain, engine.m_gain, eps)) return true;
    if (!is_near(p.understeer, engine.m_understeer_effect, eps)) return true;
    if (!is_near(p.sop, engine.m_sop_effect, eps)) return true;
    if (!is_near(p.sop_scale, engine.m_sop_scale, eps)) return true;
    if (!is_near(p.sop_smoothing, engine.m_sop_smoothing_factor, eps)) return true;
    if (!is_near(p.slip_smoothing, engine.m_slip_angle_smoothing, eps)) return true;
    if (!is_near(p.min_force, engine.m_min_force, eps)) return true;
    if (!is_near(p.oversteer_boost, engine.m_oversteer_boost, eps)) return true;
    if (p.lockup_enabled != engine.m_lockup_enabled) return true;
    if (!is_near(p.lockup_gain, engine.m_lockup_gain, eps)) return true;
    if (!is_near(p.lockup_start_pct, engine.m_lockup_start_pct, eps)) return true;
    if (!is_near(p.lockup_full_pct, engine.m_lockup_full_pct, eps)) return true;
    if (!is_near(p.lockup_rear_boost, engine.m_lockup_rear_boost, eps)) return true;
    if (!is_near(p.lockup_gamma, engine.m_lockup_gamma, eps)) return true;
    if (!is_near(p.lockup_prediction_sens, engine.m_lockup_prediction_sens, eps)) return true;
    if (!is_near(p.lockup_bump_reject, engine.m_lockup_bump_reject, eps)) return true;
    if (!is_near(p.brake_load_cap, engine.m_brake_load_cap, eps)) return true;
    if (!is_near(p.texture_load_cap, engine.m_texture_load_cap, eps)) return true;
    if (p.abs_pulse_enabled != engine.m_abs_pulse_enabled) return true;
    if (!is_near(p.abs_gain, engine.m_abs_gain, eps)) return true;
    if (!is_near(p.abs_freq, engine.m_abs_freq_hz, eps)) return true;
    if (p.spin_enabled != engine.m_spin_enabled) return true;
    if (!is_near(p.spin_gain, engine.m_spin_gain, eps)) return true;
    if (!is_near(p.spin_freq_scale, engine.m_spin_freq_scale, eps)) return true;
    if (p.slide_enabled != engine.m_slide_texture_enabled) return true;
    if (!is_near(p.slide_gain, engine.m_slide_texture_gain, eps)) return true;
    if (!is_near(p.slide_freq, engine.m_slide_freq_scale, eps)) return true;
    if (p.road_enabled != engine.m_road_texture_enabled) return true;
    if (!is_near(p.road_gain, engine.m_road_texture_gain, eps)) return true;
    if (p.invert_force != engine.m_invert_force) return true;
    if (!is_near(p.max_torque_ref, engine.m_max_torque_ref, eps)) return true;
    if (!is_near(p.lockup_freq_scale, engine.m_lockup_freq_scale, eps)) return true;
    if (p.bottoming_method != engine.m_bottoming_method) return true;
    if (!is_near(p.scrub_drag_gain, engine.m_scrub_drag_gain, eps)) return true;
    if (!is_near(p.rear_align_effect, engine.m_rear_align_effect, eps)) return true;
    if (!is_near(p.sop_yaw_gain, engine.m_sop_yaw_gain, eps)) return true;
    if (!is_near(p.gyro_gain, engine.m_gyro_gain, eps)) return true;
    if (!is_near(p.steering_shaft_gain, engine.m_steering_shaft_gain, eps)) return true;
    if (p.base_force_mode != engine.m_base_force_mode) return true;
    if (!is_near(p.optimal_slip_angle, engine.m_optimal_slip_angle, eps)) return true;
    if (!is_near(p.optimal_slip_ratio, engine.m_optimal_slip_ratio, eps)) return true;
    if (!is_near(p.steering_shaft_smoothing, engine.m_steering_shaft_smoothing, eps)) return true;
    if (!is_near(p.gyro_smoothing, engine.m_gyro_smoothing, eps)) return true;
    if (!is_near(p.yaw_smoothing, engine.m_yaw_accel_smoothing, eps)) return true;
    if (!is_near(p.chassis_smoothing, engine.m_chassis_inertia_smoothing, eps)) return true;
    if (p.flatspot_suppression != engine.m_flatspot_suppression) return true;
    if (!is_near(p.notch_q, engine.m_notch_q, eps)) return true;
    if (!is_near(p.flatspot_strength, engine.m_flatspot_strength, eps)) return true;
    if (p.static_notch_enabled != engine.m_static_notch_enabled) return true;
    if (!is_near(p.static_notch_freq, engine.m_static_notch_freq, eps)) return true;
    if (!is_near(p.static_notch_width, engine.m_static_notch_width, eps)) return true;
    if (!is_near(p.yaw_kick_threshold, engine.m_yaw_kick_threshold, eps)) return true;
    if (!is_near(p.speed_gate_lower, engine.m_speed_gate_lower, eps)) return true;
    if (!is_near(p.speed_gate_upper, engine.m_speed_gate_upper, eps)) return true;
    if (!is_near(p.road_fallback_scale, engine.m_road_fallback_scale, eps)) return true;
    if (p.understeer_affects_sop != engine.m_understeer_affects_sop) return true;
    if (p.slope_detection_enabled != engine.m_slope_detection_enabled) return true;
    if (p.slope_sg_window != engine.m_slope_sg_window) return true;
    if (!is_near(p.slope_sensitivity, engine.m_slope_sensitivity, eps)) return true;
    if (!is_near(p.slope_negative_threshold, (float)engine.m_slope_negative_threshold, eps)) return true;
    if (!is_near(p.slope_smoothing_tau, engine.m_slope_smoothing_tau, eps)) return true;
    if (!is_near(p.slope_alpha_threshold, engine.m_slope_alpha_threshold, eps)) return true;
    if (!is_near(p.slope_decay_rate, engine.m_slope_decay_rate, eps)) return true;
    if (p.slope_confidence_enabled != engine.m_slope_confidence_enabled) return true;
    if (!is_near(p.slope_min_threshold, engine.m_slope_min_threshold, eps)) return true;
    if (!is_near(p.slope_max_threshold, engine.m_slope_max_threshold, eps)) return true;
    return false;
}

void PresetRegistry::ParsePresetLine(const std::string& line, Preset& p, std::string& version, bool& needs_save) {
    std::istringstream is_line(line); std::string key;
    if (std::getline(is_line, key, '=')) {
        std::string value; if (std::getline(is_line, value)) {
            try {
                if (key == "app_version") version = value;
                else if (key == "gain") p.gain = std::stof(value);
                else if (key == "understeer") { float val = std::stof(value); if (val > 2.0f) { val /= 100.0f; needs_save = true; } p.understeer = (std::min)(2.0f, (std::max)(0.0f, val)); }
                else if (key == "sop") p.sop = std::stof(value);
                else if (key == "sop_scale") p.sop_scale = std::stof(value);
                else if (key == "sop_smoothing_factor") p.sop_smoothing = std::stof(value);
                else if (key == "min_force") p.min_force = std::stof(value);
                else if (key == "oversteer_boost") p.oversteer_boost = std::stof(value);
                else if (key == "lockup_enabled") p.lockup_enabled = std::stoi(value);
                else if (key == "lockup_gain") {
                    float val = std::stof(value);
                    p.lockup_gain = (std::min)(3.0f, (std::max)(0.0f, val));
                }
                else if (key == "lockup_start_pct") p.lockup_start_pct = std::stof(value);
                else if (key == "lockup_full_pct") p.lockup_full_pct = std::stof(value);
                else if (key == "lockup_rear_boost") p.lockup_rear_boost = std::stof(value);
                else if (key == "lockup_gamma") p.lockup_gamma = std::stof(value);
                else if (key == "lockup_prediction_sens") p.lockup_prediction_sens = std::stof(value);
                else if (key == "lockup_bump_reject") p.lockup_bump_reject = std::stof(value);
                else if (key == "brake_load_cap") {
                    float val = std::stof(value);
                    p.brake_load_cap = (std::min)(10.0f, (std::max)(1.0f, val));
                }
                else if (key == "texture_load_cap") p.texture_load_cap = std::stof(value);
                else if (key == "abs_pulse_enabled") p.abs_pulse_enabled = std::stoi(value);
                else if (key == "abs_gain") p.abs_gain = std::stof(value);
                else if (key == "abs_freq") p.abs_freq = std::stof(value);
                else if (key == "spin_enabled") p.spin_enabled = std::stoi(value);
                else if (key == "spin_gain") p.spin_gain = std::stof(value);
                else if (key == "slide_enabled") p.slide_enabled = std::stoi(value);
                else if (key == "slide_gain") p.slide_gain = std::stof(value);
                else if (key == "slide_freq") p.slide_freq = std::stof(value);
                else if (key == "road_enabled") p.road_enabled = std::stoi(value);
                else if (key == "road_gain") p.road_gain = std::stof(value);
                else if (key == "invert_force") p.invert_force = std::stoi(value);
                else if (key == "max_torque_ref") p.max_torque_ref = std::stof(value);
                else if (key == "abs_freq") p.abs_freq = std::stof(value);
                else if (key == "lockup_freq_scale") p.lockup_freq_scale = std::stof(value);
                else if (key == "spin_freq_scale") p.spin_freq_scale = std::stof(value);
                else if (key == "bottoming_method") p.bottoming_method = std::stoi(value);
                else if (key == "scrub_drag_gain") p.scrub_drag_gain = std::stof(value);
                else if (key == "rear_align_effect") p.rear_align_effect = std::stof(value);
                else if (key == "sop_yaw_gain") p.sop_yaw_gain = std::stof(value);
                else if (key == "steering_shaft_gain") p.steering_shaft_gain = std::stof(value);
                else if (key == "slip_angle_smoothing") p.slip_smoothing = std::stof(value);
                else if (key == "base_force_mode") p.base_force_mode = std::stoi(value);
                else if (key == "gyro_gain") p.gyro_gain = std::stof(value);
                else if (key == "flatspot_suppression") p.flatspot_suppression = std::stoi(value);
                else if (key == "notch_q") p.notch_q = std::stof(value);
                else if (key == "flatspot_strength") p.flatspot_strength = std::stof(value);
                else if (key == "static_notch_enabled") p.static_notch_enabled = std::stoi(value);
                else if (key == "static_notch_freq") p.static_notch_freq = std::stof(value);
                else if (key == "static_notch_width") p.static_notch_width = std::stof(value);
                else if (key == "yaw_kick_threshold") p.yaw_kick_threshold = std::stof(value);
                else if (key == "optimal_slip_angle") p.optimal_slip_angle = std::stof(value);
                else if (key == "optimal_slip_ratio") p.optimal_slip_ratio = std::stof(value);
                else if (key == "slope_detection_enabled") p.slope_detection_enabled = (value == "1");
                else if (key == "slope_sg_window") p.slope_sg_window = std::stoi(value);
                else if (key == "slope_sensitivity") p.slope_sensitivity = std::stof(value);
                else if (key == "slope_negative_threshold") p.slope_negative_threshold = std::stof(value);
                else if (key == "slope_smoothing_tau") p.slope_smoothing_tau = std::stof(value);
                else if (key == "slope_min_threshold") p.slope_min_threshold = std::stof(value);
                else if (key == "slope_max_threshold") p.slope_max_threshold = std::stof(value);
                else if (key == "slope_alpha_threshold") {
                    float val = std::stof(value);
                    if (val < 0.001f || val > 0.1f) val = 0.02f;
                    p.slope_alpha_threshold = val;
                }
                else if (key == "slope_decay_rate") p.slope_decay_rate = std::stof(value);
                else if (key == "slope_confidence_enabled") p.slope_confidence_enabled = (value == "1");
                else if (key == "steering_shaft_smoothing") p.steering_shaft_smoothing = std::stof(value);
                else if (key == "gyro_smoothing_factor") p.gyro_smoothing = std::stof(value);
                else if (key == "yaw_accel_smoothing") p.yaw_smoothing = std::stof(value);
                else if (key == "chassis_inertia_smoothing") p.chassis_smoothing = std::stof(value);
                else if (key == "speed_gate_lower") p.speed_gate_lower = std::stof(value);
                else if (key == "speed_gate_upper") p.speed_gate_upper = std::stof(value);
                else if (key == "road_fallback_scale") p.road_fallback_scale = std::stof(value);
                else if (key == "understeer_affects_sop") p.understeer_affects_sop = std::stoi(value);
            } catch (...) {}
        }
    }
}

void PresetRegistry::WritePresetFields(std::ofstream& file, const Preset& p) {
    file << "app_version=" << p.app_version << "\n";
    file << "invert_force=" << (p.invert_force ? "1" : "0") << "\n";
    file << "gain=" << p.gain << "\n";
    file << "max_torque_ref=" << p.max_torque_ref << "\n";
    file << "min_force=" << p.min_force << "\n";
    file << "steering_shaft_gain=" << p.steering_shaft_gain << "\n";
    file << "steering_shaft_smoothing=" << p.steering_shaft_smoothing << "\n";
    file << "understeer=" << p.understeer << "\n";
    file << "base_force_mode=" << p.base_force_mode << "\n";
    file << "flatspot_suppression=" << p.flatspot_suppression << "\n";
    file << "notch_q=" << p.notch_q << "\n";
    file << "flatspot_strength=" << p.flatspot_strength << "\n";
    file << "static_notch_enabled=" << p.static_notch_enabled << "\n";
    file << "static_notch_freq=" << p.static_notch_freq << "\n";
    file << "static_notch_width=" << p.static_notch_width << "\n";
    file << "oversteer_boost=" << p.oversteer_boost << "\n";
    file << "sop=" << p.sop << "\n";
    file << "rear_align_effect=" << p.rear_align_effect << "\n";
    file << "sop_yaw_gain=" << p.sop_yaw_gain << "\n";
    file << "yaw_kick_threshold=" << p.yaw_kick_threshold << "\n";
    file << "yaw_accel_smoothing=" << p.yaw_smoothing << "\n";
    file << "gyro_gain=" << p.gyro_gain << "\n";
    file << "gyro_smoothing_factor=" << p.gyro_smoothing << "\n";
    file << "sop_smoothing_factor=" << p.sop_smoothing << "\n";
    file << "sop_scale=" << p.sop_scale << "\n";
    file << "understeer_affects_sop=" << p.understeer_affects_sop << "\n";
    file << "slope_detection_enabled=" << p.slope_detection_enabled << "\n";
    file << "slope_sg_window=" << p.slope_sg_window << "\n";
    file << "slope_sensitivity=" << p.slope_sensitivity << "\n";
    file << "slope_negative_threshold=" << p.slope_negative_threshold << "\n";
    file << "slope_smoothing_tau=" << p.slope_smoothing_tau << "\n";
    file << "slope_min_threshold=" << p.slope_min_threshold << "\n";
    file << "slope_max_threshold=" << p.slope_max_threshold << "\n";
    file << "slope_alpha_threshold=" << p.slope_alpha_threshold << "\n";
    file << "slope_decay_rate=" << p.slope_decay_rate << "\n";
    file << "slope_confidence_enabled=" << p.slope_confidence_enabled << "\n";
    file << "slip_angle_smoothing=" << p.slip_smoothing << "\n";
    file << "chassis_inertia_smoothing=" << p.chassis_smoothing << "\n";
    file << "optimal_slip_angle=" << p.optimal_slip_angle << "\n";
    file << "optimal_slip_ratio=" << p.optimal_slip_ratio << "\n";
    file << "lockup_enabled=" << (p.lockup_enabled ? "1" : "0") << "\n";
    file << "lockup_gain=" << p.lockup_gain << "\n";
    file << "brake_load_cap=" << p.brake_load_cap << "\n";
    file << "lockup_freq_scale=" << p.lockup_freq_scale << "\n";
    file << "lockup_gamma=" << p.lockup_gamma << "\n";
    file << "lockup_start_pct=" << p.lockup_start_pct << "\n";
    file << "lockup_full_pct=" << p.lockup_full_pct << "\n";
    file << "lockup_prediction_sens=" << p.lockup_prediction_sens << "\n";
    file << "lockup_bump_reject=" << p.lockup_bump_reject << "\n";
    file << "lockup_rear_boost=" << p.lockup_rear_boost << "\n";
    file << "abs_pulse_enabled=" << (p.abs_pulse_enabled ? "1" : "0") << "\n";
    file << "abs_gain=" << p.abs_gain << "\n";
    file << "abs_freq=" << p.abs_freq << "\n";
    file << "texture_load_cap=" << p.texture_load_cap << "\n";
    file << "slide_enabled=" << (p.slide_enabled ? "1" : "0") << "\n";
    file << "slide_gain=" << p.slide_gain << "\n";
    file << "slide_freq=" << p.slide_freq << "\n";
    file << "road_enabled=" << (p.road_enabled ? "1" : "0") << "\n";
    file << "road_gain=" << p.road_gain << "\n";
    file << "road_fallback_scale=" << p.road_fallback_scale << "\n";
    file << "spin_enabled=" << (p.spin_enabled ? "1" : "0") << "\n";
    file << "spin_gain=" << p.spin_gain << "\n";
    file << "spin_freq_scale=" << p.spin_freq_scale << "\n";
    file << "scrub_drag_gain=" << p.scrub_drag_gain << "\n";
    file << "bottoming_method=" << p.bottoming_method << "\n";
    file << "speed_gate_lower=" << p.speed_gate_lower << "\n";
    file << "speed_gate_upper=" << p.speed_gate_upper << "\n";
}

void PresetRegistry::WritePresets(std::ofstream& file) {
    file << "\n[Presets]\n";
    for (const auto& p : m_presets) {
        if (!p.is_builtin) {
            file << "[Preset:" << p.name << "]\n";
            WritePresetFields(file, p);
            file << "\n";
        }
    }
}

void PresetRegistry::Save(const FFBEngine& engine, const std::string& path) {
    std::ofstream file(path);
    if (file.is_open()) {
        file << "; --- System & Window ---\n";
        file << "ini_version=" << LMUFFB_VERSION << "\n";
        file << "ignore_vjoy_version_warning=" << Config::m_ignore_vjoy_version_warning << "\n";
        file << "enable_vjoy=" << Config::m_enable_vjoy << "\n";
        file << "output_ffb_to_vjoy=" << Config::m_output_ffb_to_vjoy << "\n";
        file << "always_on_top=" << Config::m_always_on_top << "\n";
        file << "last_device_guid=" << Config::m_last_device_guid << "\n";
        file << "last_preset_name=" << m_last_preset_name << "\n";
        file << "win_pos_x=" << Config::win_pos_x << "\n";
        file << "win_pos_y=" << Config::win_pos_y << "\n";
        file << "win_w_small=" << Config::win_w_small << "\n";
        file << "win_h_small=" << Config::win_h_small << "\n";
        file << "win_w_large=" << Config::win_w_large << "\n";
        file << "win_h_large=" << Config::win_h_large << "\n";
        file << "show_graphs=" << Config::show_graphs << "\n";
        file << "auto_start_logging=" << Config::m_auto_start_logging << "\n";
        file << "log_path=" << Config::m_log_path << "\n";
        file << "\n; --- General FFB ---\n";
        file << "invert_force=" << engine.m_invert_force << "\n";
        file << "gain=" << engine.m_gain << "\n";
        file << "max_torque_ref=" << engine.m_max_torque_ref << "\n";
        file << "min_force=" << engine.m_min_force << "\n";
        file << "\n; --- Front Axle (Understeer) ---\n";
        file << "steering_shaft_gain=" << engine.m_steering_shaft_gain << "\n";
        file << "steering_shaft_smoothing=" << engine.m_steering_shaft_smoothing << "\n";
        file << "understeer=" << engine.m_understeer_effect << "\n";
        file << "base_force_mode=" << engine.m_base_force_mode << "\n";
        file << "flatspot_suppression=" << engine.m_flatspot_suppression << "\n";
        file << "notch_q=" << engine.m_notch_q << "\n";
        file << "flatspot_strength=" << engine.m_flatspot_strength << "\n";
        file << "static_notch_enabled=" << engine.m_static_notch_enabled << "\n";
        file << "static_notch_freq=" << engine.m_static_notch_freq << "\n";
        file << "static_notch_width=" << engine.m_static_notch_width << "\n";
        file << "\n; --- Rear Axle (Oversteer) ---\n";
        file << "oversteer_boost=" << engine.m_oversteer_boost << "\n";
        file << "sop=" << engine.m_sop_effect << "\n";
        file << "rear_align_effect=" << engine.m_rear_align_effect << "\n";
        file << "sop_yaw_gain=" << engine.m_sop_yaw_gain << "\n";
        file << "yaw_kick_threshold=" << engine.m_yaw_kick_threshold << "\n";
        file << "yaw_accel_smoothing=" << engine.m_yaw_accel_smoothing << "\n";
        file << "gyro_gain=" << engine.m_gyro_gain << "\n";
        file << "gyro_smoothing_factor=" << engine.m_gyro_smoothing << "\n";
        file << "sop_smoothing_factor=" << engine.m_sop_smoothing_factor << "\n";
        file << "sop_scale=" << engine.m_sop_scale << "\n";
        file << "understeer_affects_sop=" << engine.m_understeer_affects_sop << "\n";
        file << "\n; --- Physics (Grip & Slip Angle) ---\n";
        file << "slip_angle_smoothing=" << engine.m_slip_angle_smoothing << "\n";
        file << "chassis_inertia_smoothing=" << engine.m_chassis_inertia_smoothing << "\n";
        file << "optimal_slip_angle=" << engine.m_optimal_slip_angle << "\n";
        file << "optimal_slip_ratio=" << engine.m_optimal_slip_ratio << "\n";
        file << "slope_detection_enabled=" << engine.m_slope_detection_enabled << "\n";
        file << "slope_sg_window=" << engine.m_slope_sg_window << "\n";
        file << "slope_sensitivity=" << engine.m_slope_sensitivity << "\n";
        file << "slope_negative_threshold=" << engine.m_slope_negative_threshold << "\n";
        file << "slope_smoothing_tau=" << engine.m_slope_smoothing_tau << "\n";
        file << "slope_min_threshold=" << engine.m_slope_min_threshold << "\n";
        file << "slope_max_threshold=" << engine.m_slope_max_threshold << "\n";
        file << "slope_alpha_threshold=" << engine.m_slope_alpha_threshold << "\n";
        file << "slope_decay_rate=" << engine.m_slope_decay_rate << "\n";
        file << "slope_confidence_enabled=" << engine.m_slope_confidence_enabled << "\n";
        file << "\n; --- Braking & Lockup ---\n";
        file << "lockup_enabled=" << engine.m_lockup_enabled << "\n";
        file << "lockup_gain=" << engine.m_lockup_gain << "\n";
        file << "brake_load_cap=" << engine.m_brake_load_cap << "\n";
        file << "lockup_freq_scale=" << engine.m_lockup_freq_scale << "\n";
        file << "lockup_gamma=" << engine.m_lockup_gamma << "\n";
        file << "lockup_start_pct=" << engine.m_lockup_start_pct << "\n";
        file << "lockup_full_pct=" << engine.m_lockup_full_pct << "\n";
        file << "lockup_prediction_sens=" << engine.m_lockup_prediction_sens << "\n";
        file << "lockup_bump_reject=" << engine.m_lockup_bump_reject << "\n";
        file << "lockup_rear_boost=" << engine.m_lockup_rear_boost << "\n";
        file << "abs_pulse_enabled=" << engine.m_abs_pulse_enabled << "\n";
        file << "abs_gain=" << engine.m_abs_gain << "\n";
        file << "abs_freq=" << engine.m_abs_freq_hz << "\n";
        file << "\n; --- Tactile Textures ---\n";
        file << "texture_load_cap=" << engine.m_texture_load_cap << "\n";
        file << "slide_enabled=" << engine.m_slide_texture_enabled << "\n";
        file << "slide_gain=" << engine.m_slide_texture_gain << "\n";
        file << "slide_freq=" << engine.m_slide_freq_scale << "\n";
        file << "road_enabled=" << engine.m_road_texture_enabled << "\n";
        file << "road_gain=" << engine.m_road_texture_gain << "\n";
        file << "road_fallback_scale=" << engine.m_road_fallback_scale << "\n";
        file << "spin_enabled=" << engine.m_spin_enabled << "\n";
        file << "spin_gain=" << engine.m_spin_gain << "\n";
        file << "spin_freq_scale=" << engine.m_spin_freq_scale << "\n";
        file << "scrub_drag_gain=" << engine.m_scrub_drag_gain << "\n";
        file << "bottoming_method=" << engine.m_bottoming_method << "\n";
        file << "\n; --- Advanced Settings ---\n";
        file << "speed_gate_lower=" << engine.m_speed_gate_lower << "\n";
        file << "speed_gate_upper=" << engine.m_speed_gate_upper << "\n";
        WritePresets(file);
        file.close();
    }
}

void PresetRegistry::ExportPreset(int index, const std::string& filename) {
    if (index < 0 || index >= (int)m_presets.size()) return;
    const Preset& p = m_presets[index];
    std::ofstream file(filename);
    if (file.is_open()) {
        file << "[Preset:" << p.name << "]\n";
        WritePresetFields(file, p);
    }
}

bool PresetRegistry::ImportPreset(const std::string& filename, const FFBEngine& engine) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    std::string line, current_preset_name = "";
    Preset current_preset; std::string current_preset_version = "";
    bool preset_pending = false, needs_save = false;
    bool has_max_threshold = false;

    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty() || line[0] == ';') continue;
        if (line[0] == '[') {
            if (line.rfind("[Preset:", 0) == 0) {
                size_t end_pos = line.find(']');
                if (end_pos != std::string::npos) {
                    current_preset_name = line.substr(8, end_pos - 8);
                    current_preset = Preset(current_preset_name, false);
                    preset_pending = true; current_preset_version = "";
                    has_max_threshold = false;
                }
            }
            continue;
        }
        if (preset_pending) {
            if (line.find("slope_max_threshold=") == 0) has_max_threshold = true;
            ParsePresetLine(line, current_preset, current_preset_version, needs_save);
        }
    }
    if (preset_pending && !current_preset_name.empty()) {
        current_preset.name = current_preset_name; current_preset.is_builtin = false;
        current_preset.app_version = current_preset_version.empty() ? LMUFFB_VERSION : current_preset_version;

        if (!has_max_threshold && current_preset.slope_sensitivity > 0.1f) {
            current_preset.slope_max_threshold = current_preset.slope_min_threshold - (8.0f / current_preset.slope_sensitivity);
        }

        std::string base_name = current_preset.name; int counter = 1; bool exists = true;
        while (exists) {
            exists = false;
            for (const auto& p : m_presets) {
                if (p.name == current_preset.name) {
                    current_preset.name = base_name + " (" + std::to_string(counter++) + ")";
                    exists = true; break;
                }
            }
        }
        m_presets.insert(m_presets.begin() + GetUserInsertionPoint(), current_preset);
        Config::Save(engine);
        return true;
    }
    return false;
}
