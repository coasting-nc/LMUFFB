#include "Config.h"
#include "Version.h"
#ifdef _WIN32
#include <windows.h>
#include <string_view>
#include <optional>
#include "../gui/resource.h"
#else
#include "GeneratedBuiltinPresets.h"
#endif
#include "logging/Logger.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <mutex>
#include <toml++/toml.hpp>
#include <filesystem>

namespace LMUFFB {
extern std::recursive_mutex g_engine_mutex;

bool Config::m_always_on_top = true;
std::string Config::m_last_device_guid = "";
std::string Config::m_last_preset_name = "Default";
std::string Config::m_config_path = "config.toml";
bool Config::m_auto_start_logging = false;
std::string Config::m_log_path = "logs/";
std::string Config::m_user_presets_path = "user_presets";

// Window Geometry Defaults (v0.5.5)
int Config::win_pos_x = 100;
int Config::win_pos_y = 100;
int Config::win_w_small = 500;   // Narrow (Config Only)
int Config::win_h_small = 800;
int Config::win_w_large = 1400;  // Wide (Config + Graphs)
int Config::win_h_large = 800;
bool Config::show_graphs = false;

std::map<std::string, double> Config::m_saved_static_loads;
std::recursive_mutex Config::m_static_loads_mutex;
std::atomic<bool> Config::m_needs_save{ false };

std::vector<Preset> Config::presets;
    
#ifdef _WIN32
// Helper to safely load raw binary/text data from a Windows PE resource
static std::optional<std::string_view> LoadTextResource(int resourceId) {
    HMODULE hModule = GetModuleHandle(nullptr);
    
    // Find the resource by ID and type (RT_RCDATA)
    HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hResource) return std::nullopt;

    // Load it into memory
    HGLOBAL hMemory = LoadResource(hModule, hResource);
    if (!hMemory) return std::nullopt;

    // Get the exact size in bytes
    DWORD dwSize = SizeofResource(hModule, hResource);
    
    // Get the pointer to the first byte
    void* pData = LockResource(hMemory);
    if (!pData || dwSize == 0) return std::nullopt;

    // Return a string_view (Pointer + Size). No null-terminator required!
    return std::string_view(static_cast<const char*>(pData), dwSize);
}
#endif

// Helper to compare semantic version strings
static bool IsVersionLessEqual(const std::string& v1, const std::string& v2) {
    if (v1.empty()) return true;
    if (v2.empty()) return false;

    std::stringstream ss1(v1), ss2(v2);
    std::string segment1, segment2;

    while (true) {
        bool has1 = (bool)std::getline(ss1, segment1, '.');
        bool has2 = (bool)std::getline(ss2, segment2, '.');

        if (!has1 && !has2) return true;

        int val1 = 0;
        try { if (has1) val1 = std::stoi(segment1); } catch (...) {}
        int val2 = 0;
        try { if (has2) val2 = std::stoi(segment2); } catch (...) {}

        if (val1 < val2) return true;
        if (val1 > val2) return false;

        if (!has1 || !has2) break;
    }
    return true;
}

// --- TOML Serialization Helpers ---

static toml::table PresetToToml(const Preset& p) {
    toml::table tbl;

    tbl.insert("name", p.name);
    tbl.insert("app_version", p.app_version);

    tbl.insert("General", toml::table{
        {"gain", p.general.gain},
        {"wheelbase_max_nm", p.general.wheelbase_max_nm},
        {"target_rim_nm", p.general.target_rim_nm},
        {"min_force", p.general.min_force},
        {"dynamic_normalization_enabled", p.general.dynamic_normalization_enabled},
        {"auto_load_normalization_enabled", p.general.auto_load_normalization_enabled}
    });

    tbl.insert("FrontAxle", toml::table{
        {"understeer_effect", p.front_axle.understeer_effect},
        {"understeer_gamma", p.front_axle.understeer_gamma},
        {"steering_shaft_gain", p.front_axle.steering_shaft_gain},
        {"ingame_ffb_gain", p.front_axle.ingame_ffb_gain},
        {"steering_shaft_smoothing", p.front_axle.steering_shaft_smoothing},
        {"torque_source", p.front_axle.torque_source},
        {"steering_100hz_reconstruction", p.front_axle.steering_100hz_reconstruction},
        {"torque_passthrough", p.front_axle.torque_passthrough},
        {"flatspot_suppression", p.front_axle.flatspot_suppression},
        {"notch_q", p.front_axle.notch_q},
        {"flatspot_strength", p.front_axle.flatspot_strength},
        {"static_notch_enabled", p.front_axle.static_notch_enabled},
        {"static_notch_freq", p.front_axle.static_notch_freq},
        {"static_notch_width", p.front_axle.static_notch_width}
    });

    tbl.insert("RearAxle", toml::table{
        {"sop_effect", p.rear_axle.sop_effect},
        {"sop_scale", p.rear_axle.sop_scale},
        {"sop_smoothing_factor", p.rear_axle.sop_smoothing_factor},
        {"oversteer_boost", p.rear_axle.oversteer_boost},
        {"rear_align_effect", p.rear_axle.rear_align_effect},
        {"kerb_strike_rejection", p.rear_axle.kerb_strike_rejection},
        {"sop_yaw_gain", p.rear_axle.sop_yaw_gain},
        {"yaw_kick_threshold", p.rear_axle.yaw_kick_threshold},
        {"unloaded_yaw_gain", p.rear_axle.unloaded_yaw_gain},
        {"unloaded_yaw_threshold", p.rear_axle.unloaded_yaw_threshold},
        {"unloaded_yaw_sens", p.rear_axle.unloaded_yaw_sens},
        {"unloaded_yaw_gamma", p.rear_axle.unloaded_yaw_gamma},
        {"unloaded_yaw_punch", p.rear_axle.unloaded_yaw_punch},
        {"power_yaw_gain", p.rear_axle.power_yaw_gain},
        {"power_yaw_threshold", p.rear_axle.power_yaw_threshold},
        {"power_slip_threshold", p.rear_axle.power_slip_threshold},
        {"power_yaw_gamma", p.rear_axle.power_yaw_gamma},
        {"power_yaw_punch", p.rear_axle.power_yaw_punch},
        {"yaw_accel_smoothing", p.rear_axle.yaw_accel_smoothing}
    });

    tbl.insert("LoadForces", toml::table{
        {"lat_load_effect", p.load_forces.lat_load_effect},
        {"lat_load_transform", p.load_forces.lat_load_transform},
        {"long_load_effect", p.load_forces.long_load_effect},
        {"long_load_smoothing", p.load_forces.long_load_smoothing},
        {"long_load_transform", p.load_forces.long_load_transform}
    });

    tbl.insert("GripEstimation", toml::table{
        {"grip_smoothing_steady", p.grip_estimation.grip_smoothing_steady},
        {"grip_smoothing_fast", p.grip_estimation.grip_smoothing_fast},
        {"grip_smoothing_sensitivity", p.grip_estimation.grip_smoothing_sensitivity},
        {"optimal_slip_angle", p.grip_estimation.optimal_slip_angle},
        {"optimal_slip_ratio", p.grip_estimation.optimal_slip_ratio},
        {"slip_angle_smoothing", p.grip_estimation.slip_angle_smoothing},
        {"chassis_inertia_smoothing", p.grip_estimation.chassis_inertia_smoothing},
        {"load_sensitivity_enabled", p.grip_estimation.load_sensitivity_enabled}
    });

    tbl.insert("SlopeDetection", toml::table{
        {"enabled", p.slope_detection.enabled},
        {"sg_window", p.slope_detection.sg_window},
        {"sensitivity", p.slope_detection.sensitivity},
        {"smoothing_tau", p.slope_detection.smoothing_tau},
        {"alpha_threshold", p.slope_detection.alpha_threshold},
        {"decay_rate", p.slope_detection.decay_rate},
        {"confidence_enabled", p.slope_detection.confidence_enabled},
        {"confidence_max_rate", p.slope_detection.confidence_max_rate},
        {"min_threshold", p.slope_detection.min_threshold},
        {"max_threshold", p.slope_detection.max_threshold},
        {"g_slew_limit", p.slope_detection.g_slew_limit},
        {"use_torque", p.slope_detection.use_torque},
        {"torque_sensitivity", p.slope_detection.torque_sensitivity}
    });

    tbl.insert("Braking", toml::table{
        {"lockup_enabled", p.braking.lockup_enabled},
        {"lockup_gain", p.braking.lockup_gain},
        {"lockup_start_pct", p.braking.lockup_start_pct},
        {"lockup_full_pct", p.braking.lockup_full_pct},
        {"lockup_rear_boost", p.braking.lockup_rear_boost},
        {"lockup_gamma", p.braking.lockup_gamma},
        {"lockup_prediction_sens", p.braking.lockup_prediction_sens},
        {"lockup_bump_reject", p.braking.lockup_bump_reject},
        {"brake_load_cap", p.braking.brake_load_cap},
        {"lockup_freq_scale", p.braking.lockup_freq_scale},
        {"abs_pulse_enabled", p.braking.abs_pulse_enabled},
        {"abs_gain", p.braking.abs_gain},
        {"abs_freq", p.braking.abs_freq}
    });

    tbl.insert("Vibration", toml::table{
        {"vibration_gain", p.vibration.vibration_gain},
        {"texture_load_cap", p.vibration.texture_load_cap},
        {"slide_enabled", p.vibration.slide_enabled},
        {"slide_gain", p.vibration.slide_gain},
        {"slide_freq", p.vibration.slide_freq},
        {"road_enabled", p.vibration.road_enabled},
        {"road_gain", p.vibration.road_gain},
        {"spin_enabled", p.vibration.spin_enabled},
        {"spin_gain", p.vibration.spin_gain},
        {"spin_freq_scale", p.vibration.spin_freq_scale},
        {"scrub_drag_gain", p.vibration.scrub_drag_gain},
        {"bottoming_enabled", p.vibration.bottoming_enabled},
        {"bottoming_gain", p.vibration.bottoming_gain},
        {"bottoming_method", p.vibration.bottoming_method}
    });

    tbl.insert("Advanced", toml::table{
        {"gyro_gain", p.advanced.gyro_gain},
        {"gyro_smoothing", p.advanced.gyro_smoothing},
        {"stationary_damping", p.advanced.stationary_damping},
        {"soft_lock_enabled", p.advanced.soft_lock_enabled},
        {"soft_lock_stiffness", p.advanced.soft_lock_stiffness},
        {"soft_lock_damping", p.advanced.soft_lock_damping},
        {"speed_gate_lower", p.advanced.speed_gate_lower},
        {"speed_gate_upper", p.advanced.speed_gate_upper},
        {"rest_api_enabled", p.advanced.rest_api_enabled},
        {"rest_api_port", p.advanced.rest_api_port},
        {"road_fallback_scale", p.advanced.road_fallback_scale},
        {"understeer_affects_sop", p.advanced.understeer_affects_sop},
        {"aux_telemetry_reconstruction", p.advanced.aux_telemetry_reconstruction}
    });

    tbl.insert("Safety", toml::table{
        {"window_duration", p.safety.window_duration},
        {"gain_reduction", p.safety.gain_reduction},
        {"smoothing_tau", p.safety.smoothing_tau},
        {"spike_detection_threshold", p.safety.spike_detection_threshold},
        {"immediate_spike_threshold", p.safety.immediate_spike_threshold},
        {"slew_full_scale_time_s", p.safety.slew_full_scale_time_s},
        {"stutter_safety_enabled", p.safety.stutter_safety_enabled},
        {"stutter_threshold", p.safety.stutter_threshold}
    });

    return tbl;
}

static void TomlToPreset(const toml::table& tbl, Preset& p) {
    auto baseline = p; // Keep defaults for missing keys

    if (auto val = tbl["name"].as_string()) p.name = val->get();
    if (auto val = tbl["app_version"].as_string()) p.app_version = val->get();

    if (auto gen = tbl["General"].as_table()) {
        p.general.gain = (float)(*gen)["gain"].value_or((double)baseline.general.gain);
        p.general.wheelbase_max_nm = (float)(*gen)["wheelbase_max_nm"].value_or((double)baseline.general.wheelbase_max_nm);
        p.general.target_rim_nm = (float)(*gen)["target_rim_nm"].value_or((double)baseline.general.target_rim_nm);
        p.general.min_force = (float)(*gen)["min_force"].value_or((double)baseline.general.min_force);
        p.general.dynamic_normalization_enabled = (*gen)["dynamic_normalization_enabled"].value_or(baseline.general.dynamic_normalization_enabled);
        p.general.auto_load_normalization_enabled = (*gen)["auto_load_normalization_enabled"].value_or(baseline.general.auto_load_normalization_enabled);
    }

    if (auto fr = tbl["FrontAxle"].as_table()) {
        p.front_axle.understeer_effect = (float)(*fr)["understeer_effect"].value_or((double)baseline.front_axle.understeer_effect);
        // Special case: old understeer key
        if (auto u = (*fr)["understeer"].as_floating_point()) {
             p.front_axle.understeer_effect = (float)u->get();
        }
        p.front_axle.understeer_gamma = (float)(*fr)["understeer_gamma"].value_or((double)baseline.front_axle.understeer_gamma);
        p.front_axle.steering_shaft_gain = (float)(*fr)["steering_shaft_gain"].value_or((double)baseline.front_axle.steering_shaft_gain);
        p.front_axle.ingame_ffb_gain = (float)(*fr)["ingame_ffb_gain"].value_or((double)baseline.front_axle.ingame_ffb_gain);
        p.front_axle.steering_shaft_smoothing = (float)(*fr)["steering_shaft_smoothing"].value_or((double)baseline.front_axle.steering_shaft_smoothing);
        p.front_axle.torque_source = (int)(*fr)["torque_source"].value_or((int64_t)baseline.front_axle.torque_source);
        p.front_axle.steering_100hz_reconstruction = (int)(*fr)["steering_100hz_reconstruction"].value_or((int64_t)baseline.front_axle.steering_100hz_reconstruction);
        p.front_axle.torque_passthrough = (*fr)["torque_passthrough"].value_or(baseline.front_axle.torque_passthrough);
        p.front_axle.flatspot_suppression = (*fr)["flatspot_suppression"].value_or(baseline.front_axle.flatspot_suppression);
        p.front_axle.notch_q = (float)(*fr)["notch_q"].value_or((double)baseline.front_axle.notch_q);
        p.front_axle.flatspot_strength = (float)(*fr)["flatspot_strength"].value_or((double)baseline.front_axle.flatspot_strength);
        p.front_axle.static_notch_enabled = (*fr)["static_notch_enabled"].value_or(baseline.front_axle.static_notch_enabled);
        p.front_axle.static_notch_freq = (float)(*fr)["static_notch_freq"].value_or((double)baseline.front_axle.static_notch_freq);
        p.front_axle.static_notch_width = (float)(*fr)["static_notch_width"].value_or((double)baseline.front_axle.static_notch_width);
    }

    if (auto rr = tbl["RearAxle"].as_table()) {
        p.rear_axle.sop_effect = (float)(*rr)["sop_effect"].value_or((double)baseline.rear_axle.sop_effect);
        p.rear_axle.sop_scale = (float)(*rr)["sop_scale"].value_or((double)baseline.rear_axle.sop_scale);
        p.rear_axle.sop_smoothing_factor = (float)(*rr)["sop_smoothing_factor"].value_or((double)baseline.rear_axle.sop_smoothing_factor);
        p.rear_axle.oversteer_boost = (float)(*rr)["oversteer_boost"].value_or((double)baseline.rear_axle.oversteer_boost);
        p.rear_axle.rear_align_effect = (float)(*rr)["rear_align_effect"].value_or((double)baseline.rear_axle.rear_align_effect);
        p.rear_axle.kerb_strike_rejection = (float)(*rr)["kerb_strike_rejection"].value_or((double)baseline.rear_axle.kerb_strike_rejection);
        p.rear_axle.sop_yaw_gain = (float)(*rr)["sop_yaw_gain"].value_or((double)baseline.rear_axle.sop_yaw_gain);
        p.rear_axle.yaw_kick_threshold = (float)(*rr)["yaw_kick_threshold"].value_or((double)baseline.rear_axle.yaw_kick_threshold);
        p.rear_axle.unloaded_yaw_gain = (float)(*rr)["unloaded_yaw_gain"].value_or((double)baseline.rear_axle.unloaded_yaw_gain);
        p.rear_axle.unloaded_yaw_threshold = (float)(*rr)["unloaded_yaw_threshold"].value_or((double)baseline.rear_axle.unloaded_yaw_threshold);
        p.rear_axle.unloaded_yaw_sens = (float)(*rr)["unloaded_yaw_sens"].value_or((double)baseline.rear_axle.unloaded_yaw_sens);
        p.rear_axle.unloaded_yaw_gamma = (float)(*rr)["unloaded_yaw_gamma"].value_or((double)baseline.rear_axle.unloaded_yaw_gamma);
        p.rear_axle.unloaded_yaw_punch = (float)(*rr)["unloaded_yaw_punch"].value_or((double)baseline.rear_axle.unloaded_yaw_punch);
        p.rear_axle.power_yaw_gain = (float)(*rr)["power_yaw_gain"].value_or((double)baseline.rear_axle.power_yaw_gain);
        p.rear_axle.power_yaw_threshold = (float)(*rr)["power_yaw_threshold"].value_or((double)baseline.rear_axle.power_yaw_threshold);
        p.rear_axle.power_slip_threshold = (float)(*rr)["power_slip_threshold"].value_or((double)baseline.rear_axle.power_slip_threshold);
        p.rear_axle.power_yaw_gamma = (float)(*rr)["power_yaw_gamma"].value_or((double)baseline.rear_axle.power_yaw_gamma);
        p.rear_axle.power_yaw_punch = (float)(*rr)["power_yaw_punch"].value_or((double)baseline.rear_axle.power_yaw_punch);
        p.rear_axle.yaw_accel_smoothing = (float)(*rr)["yaw_accel_smoothing"].value_or((double)baseline.rear_axle.yaw_accel_smoothing);
    }

    if (auto lf = tbl["LoadForces"].as_table()) {
        p.load_forces.lat_load_effect = (float)(*lf)["lat_load_effect"].value_or((double)baseline.load_forces.lat_load_effect);
        p.load_forces.lat_load_transform = (int)(*lf)["lat_load_transform"].value_or((int64_t)baseline.load_forces.lat_load_transform);
        p.load_forces.long_load_effect = (float)(*lf)["long_load_effect"].value_or((double)baseline.load_forces.long_load_effect);
        p.load_forces.long_load_smoothing = (float)(*lf)["long_load_smoothing"].value_or((double)baseline.load_forces.long_load_smoothing);
        p.load_forces.long_load_transform = (int)(*lf)["long_load_transform"].value_or((int64_t)baseline.load_forces.long_load_transform);
    }

    if (auto ge = tbl["GripEstimation"].as_table()) {
        p.grip_estimation.grip_smoothing_steady = (float)(*ge)["grip_smoothing_steady"].value_or((double)baseline.grip_estimation.grip_smoothing_steady);
        p.grip_estimation.grip_smoothing_fast = (float)(*ge)["grip_smoothing_fast"].value_or((double)baseline.grip_estimation.grip_smoothing_fast);
        p.grip_estimation.grip_smoothing_sensitivity = (float)(*ge)["grip_smoothing_sensitivity"].value_or((double)baseline.grip_estimation.grip_smoothing_sensitivity);
        p.grip_estimation.optimal_slip_angle = (float)(*ge)["optimal_slip_angle"].value_or((double)baseline.grip_estimation.optimal_slip_angle);
        p.grip_estimation.optimal_slip_ratio = (float)(*ge)["optimal_slip_ratio"].value_or((double)baseline.grip_estimation.optimal_slip_ratio);
        p.grip_estimation.slip_angle_smoothing = (float)(*ge)["slip_angle_smoothing"].value_or((double)baseline.grip_estimation.slip_angle_smoothing);
        p.grip_estimation.chassis_inertia_smoothing = (float)(*ge)["chassis_inertia_smoothing"].value_or((double)baseline.grip_estimation.chassis_inertia_smoothing);
        p.grip_estimation.load_sensitivity_enabled = (*ge)["load_sensitivity_enabled"].value_or(baseline.grip_estimation.load_sensitivity_enabled);
    }

    if (auto sd = tbl["SlopeDetection"].as_table()) {
        p.slope_detection.enabled = (*sd)["enabled"].value_or(baseline.slope_detection.enabled);
        p.slope_detection.sg_window = (int)(*sd)["sg_window"].value_or((int64_t)baseline.slope_detection.sg_window);
        p.slope_detection.sensitivity = (float)(*sd)["sensitivity"].value_or((double)baseline.slope_detection.sensitivity);
        p.slope_detection.smoothing_tau = (float)(*sd)["smoothing_tau"].value_or((double)baseline.slope_detection.smoothing_tau);
        p.slope_detection.alpha_threshold = (float)(*sd)["alpha_threshold"].value_or((double)baseline.slope_detection.alpha_threshold);
        p.slope_detection.decay_rate = (float)(*sd)["decay_rate"].value_or((double)baseline.slope_detection.decay_rate);
        p.slope_detection.confidence_enabled = (*sd)["confidence_enabled"].value_or(baseline.slope_detection.confidence_enabled);
        p.slope_detection.confidence_max_rate = (float)(*sd)["confidence_max_rate"].value_or((double)baseline.slope_detection.confidence_max_rate);
        p.slope_detection.min_threshold = (float)(*sd)["min_threshold"].value_or((double)baseline.slope_detection.min_threshold);
        p.slope_detection.max_threshold = (float)(*sd)["max_threshold"].value_or((double)baseline.slope_detection.max_threshold);
        p.slope_detection.g_slew_limit = (float)(*sd)["g_slew_limit"].value_or((double)baseline.slope_detection.g_slew_limit);
        p.slope_detection.use_torque = (*sd)["use_torque"].value_or(baseline.slope_detection.use_torque);
        p.slope_detection.torque_sensitivity = (float)(*sd)["torque_sensitivity"].value_or((double)baseline.slope_detection.torque_sensitivity);
    }

    if (auto br = tbl["Braking"].as_table()) {
        p.braking.lockup_enabled = (*br)["lockup_enabled"].value_or(baseline.braking.lockup_enabled);
        p.braking.lockup_gain = (float)(*br)["lockup_gain"].value_or((double)baseline.braking.lockup_gain);
        p.braking.lockup_start_pct = (float)(*br)["lockup_start_pct"].value_or((double)baseline.braking.lockup_start_pct);
        p.braking.lockup_full_pct = (float)(*br)["lockup_full_pct"].value_or((double)baseline.braking.lockup_full_pct);
        p.braking.lockup_rear_boost = (float)(*br)["lockup_rear_boost"].value_or((double)baseline.braking.lockup_rear_boost);
        p.braking.lockup_gamma = (float)(*br)["lockup_gamma"].value_or((double)baseline.braking.lockup_gamma);
        p.braking.lockup_prediction_sens = (float)(*br)["lockup_prediction_sens"].value_or((double)baseline.braking.lockup_prediction_sens);
        p.braking.lockup_bump_reject = (float)(*br)["lockup_bump_reject"].value_or((double)baseline.braking.lockup_bump_reject);
        p.braking.brake_load_cap = (float)(*br)["brake_load_cap"].value_or((double)baseline.braking.brake_load_cap);
        p.braking.lockup_freq_scale = (float)(*br)["lockup_freq_scale"].value_or((double)baseline.braking.lockup_freq_scale);
        p.braking.abs_pulse_enabled = (*br)["abs_pulse_enabled"].value_or(baseline.braking.abs_pulse_enabled);
        p.braking.abs_gain = (float)(*br)["abs_gain"].value_or((double)baseline.braking.abs_gain);
        p.braking.abs_freq = (float)(*br)["abs_freq"].value_or((double)baseline.braking.abs_freq);
    }

    if (auto vi = tbl["Vibration"].as_table()) {
        p.vibration.vibration_gain = (float)(*vi)["vibration_gain"].value_or((double)baseline.vibration.vibration_gain);
        p.vibration.texture_load_cap = (float)(*vi)["texture_load_cap"].value_or((double)baseline.vibration.texture_load_cap);
        // Special case: old texture_load_cap key was max_load_factor
        if (auto mlf = (*vi)["max_load_factor"].as_floating_point()) {
             p.vibration.texture_load_cap = (float)mlf->get();
        }
        p.vibration.slide_enabled = (*vi)["slide_enabled"].value_or(baseline.vibration.slide_enabled);
        p.vibration.slide_gain = (float)(*vi)["slide_gain"].value_or((double)baseline.vibration.slide_gain);
        p.vibration.slide_freq = (float)(*vi)["slide_freq"].value_or((double)baseline.vibration.slide_freq);
        p.vibration.road_enabled = (*vi)["road_enabled"].value_or(baseline.vibration.road_enabled);
        p.vibration.road_gain = (float)(*vi)["road_gain"].value_or((double)baseline.vibration.road_gain);
        p.vibration.spin_enabled = (*vi)["spin_enabled"].value_or(baseline.vibration.spin_enabled);
        p.vibration.spin_gain = (float)(*vi)["spin_gain"].value_or((double)baseline.vibration.spin_gain);
        p.vibration.spin_freq_scale = (float)(*vi)["spin_freq_scale"].value_or((double)baseline.vibration.spin_freq_scale);
        p.vibration.scrub_drag_gain = (float)(*vi)["scrub_drag_gain"].value_or((double)baseline.vibration.scrub_drag_gain);
        p.vibration.bottoming_enabled = (*vi)["bottoming_enabled"].value_or(baseline.vibration.bottoming_enabled);
        p.vibration.bottoming_gain = (float)(*vi)["bottoming_gain"].value_or((double)baseline.vibration.bottoming_gain);
        p.vibration.bottoming_method = (int)(*vi)["bottoming_method"].value_or((int64_t)baseline.vibration.bottoming_method);
    }

    if (auto ad = tbl["Advanced"].as_table()) {
        p.advanced.gyro_gain = (float)(*ad)["gyro_gain"].value_or((double)baseline.advanced.gyro_gain);
        p.advanced.gyro_smoothing = (float)(*ad)["gyro_smoothing"].value_or((double)baseline.advanced.gyro_smoothing);
        p.advanced.stationary_damping = (float)(*ad)["stationary_damping"].value_or((double)baseline.advanced.stationary_damping);
        p.advanced.soft_lock_enabled = (*ad)["soft_lock_enabled"].value_or(baseline.advanced.soft_lock_enabled);
        p.advanced.soft_lock_stiffness = (float)(*ad)["soft_lock_stiffness"].value_or((double)baseline.advanced.soft_lock_stiffness);
        p.advanced.soft_lock_damping = (float)(*ad)["soft_lock_damping"].value_or((double)baseline.advanced.soft_lock_damping);
        p.advanced.speed_gate_lower = (float)(*ad)["speed_gate_lower"].value_or((double)baseline.advanced.speed_gate_lower);
        p.advanced.speed_gate_upper = (float)(*ad)["speed_gate_upper"].value_or((double)baseline.advanced.speed_gate_upper);
        p.advanced.rest_api_enabled = (*ad)["rest_api_enabled"].value_or(baseline.advanced.rest_api_enabled);
        p.advanced.rest_api_port = (int)(*ad)["rest_api_port"].value_or((int64_t)baseline.advanced.rest_api_port);
        p.advanced.road_fallback_scale = (float)(*ad)["road_fallback_scale"].value_or((double)baseline.advanced.road_fallback_scale);
        p.advanced.understeer_affects_sop = (*ad)["understeer_affects_sop"].value_or(baseline.advanced.understeer_affects_sop);
        p.advanced.aux_telemetry_reconstruction = (int)(*ad)["aux_telemetry_reconstruction"].value_or((int64_t)baseline.advanced.aux_telemetry_reconstruction);
    }

    if (auto sa = tbl["Safety"].as_table()) {
        p.safety.window_duration = (float)(*sa)["window_duration"].value_or((double)baseline.safety.window_duration);
        p.safety.gain_reduction = (float)(*sa)["gain_reduction"].value_or((double)baseline.safety.gain_reduction);
        p.safety.smoothing_tau = (float)(*sa)["smoothing_tau"].value_or((double)baseline.safety.smoothing_tau);
        p.safety.spike_detection_threshold = (float)(*sa)["spike_detection_threshold"].value_or((double)baseline.safety.spike_detection_threshold);
        p.safety.immediate_spike_threshold = (float)(*sa)["immediate_spike_threshold"].value_or((double)baseline.safety.immediate_spike_threshold);
        p.safety.slew_full_scale_time_s = (float)(*sa)["slew_full_scale_time_s"].value_or((double)baseline.safety.slew_full_scale_time_s);
        p.safety.stutter_safety_enabled = (*sa)["stutter_safety_enabled"].value_or(baseline.safety.stutter_safety_enabled);
        p.safety.stutter_threshold = (float)(*sa)["stutter_threshold"].value_or((double)baseline.safety.stutter_threshold);
    }
}

static std::string SanitizeFilename(std::string name) {
    std::string out = name;
    // Replace spaces with underscores
    std::replace(out.begin(), out.end(), ' ', '_');
    // Remove illegal Windows filename characters (\ / : * ? " < > |)
    const std::string illegal = "\\/:*?\"<>|";
    out.erase(std::remove_if(out.begin(), out.end(), [&](char c) {
        return illegal.find(c) != std::string::npos;
    }), out.end());
    return out;
}

static void SaveUserPresetFile(const Preset& p) {
    std::string filename = Config::m_user_presets_path + "/" + SanitizeFilename(p.name) + ".toml";
    std::filesystem::create_directories(Config::m_user_presets_path);
    std::ofstream file(filename);
    if (file.is_open()) {
        file << PresetToToml(p);
    }
}

// --- End TOML Serialization Helpers ---

// --- Legacy INI Parsing Helpers (Phase 2 Migration Path) ---

bool Config::ParseSystemLine(const std::string& key, const std::string& value, Preset& current_preset, std::string& current_preset_version, bool& needs_save, bool& legacy_torque_hack, float& legacy_torque_val) {
    if (key == "app_version" || key == "ini_version") { current_preset_version = value; return true; }
    if (key == "gain") { current_preset.general.gain = std::stof(value); return true; }
    if (key == "wheelbase_max_nm") { current_preset.general.wheelbase_max_nm = std::stof(value); return true; }
    if (key == "target_rim_nm") { current_preset.general.target_rim_nm = std::stof(value); return true; }
    if (key == "min_force") { current_preset.general.min_force = std::stof(value); return true; }
    if (key == "max_torque_ref") {
        float old_val = std::stof(value);
        if (old_val > 40.0f) {
            current_preset.general.wheelbase_max_nm = 15.0f;
            current_preset.general.target_rim_nm = 10.0f;
            legacy_torque_hack = true;
            legacy_torque_val = old_val;
        } else {
            current_preset.general.wheelbase_max_nm = old_val;
            current_preset.general.target_rim_nm = old_val;
        }
        needs_save = true;
        return true;
    }
    if (key == "rest_api_fallback_enabled") { current_preset.advanced.rest_api_enabled = (value == "1" || value == "true"); return true; }
    if (key == "rest_api_port") { current_preset.advanced.rest_api_port = std::stoi(value); return true; }
    return false;
}

bool Config::ParsePhysicsLine(const std::string& key, const std::string& value, Preset& current_preset) {
    if (key == "understeer" || key == "understeer_effect") {
        float val = std::stof(value);
        if (val > 2.0f) val /= 100.0f;
        current_preset.front_axle.understeer_effect = (std::min)(2.0f, (std::max)(0.0f, val));
        return true;
    }
    if (key == "understeer_gamma") { current_preset.front_axle.understeer_gamma = std::stof(value); return true; }
    if (key == "steering_shaft_gain") { current_preset.front_axle.steering_shaft_gain = std::stof(value); return true; }
    if (key == "ingame_ffb_gain") { current_preset.front_axle.ingame_ffb_gain = std::stof(value); return true; }
    if (key == "steering_shaft_smoothing") { current_preset.front_axle.steering_shaft_smoothing = std::stof(value); return true; }
    if (key == "torque_source") { current_preset.front_axle.torque_source = std::stoi(value); return true; }
    if (key == "steering_100hz_reconstruction") { current_preset.front_axle.steering_100hz_reconstruction = std::stoi(value); return true; }
    if (key == "torque_passthrough") { current_preset.front_axle.torque_passthrough = (value == "1" || value == "true"); return true; }
    if (key == "sop") { current_preset.rear_axle.sop_effect = std::stof(value); return true; }
    if (key == "lateral_load_effect") { current_preset.load_forces.lat_load_effect = std::stof(value); return true; }
    if (key == "lat_load_transform") { current_preset.load_forces.lat_load_transform = std::clamp(std::stoi(value), 0, 3); return true; }
    if (key == "sop_scale") { current_preset.rear_axle.sop_scale = std::stof(value); return true; }
    if (key == "sop_smoothing_factor" || key == "smoothing") { current_preset.rear_axle.sop_smoothing_factor = std::stof(value); return true; }
    if (key == "oversteer_boost") { current_preset.rear_axle.oversteer_boost = std::stof(value); return true; }
    if (key == "long_load_effect" || key == "dynamic_weight_gain") { current_preset.load_forces.long_load_effect = std::stof(value); return true; }
    if (key == "long_load_smoothing" || key == "dynamic_weight_smoothing") { current_preset.load_forces.long_load_smoothing = std::stof(value); return true; }
    if (key == "long_load_transform") { current_preset.load_forces.long_load_transform = std::clamp(std::stoi(value), 0, 3); return true; }
    if (key == "grip_smoothing_steady") { current_preset.grip_estimation.grip_smoothing_steady = std::stof(value); return true; }
    if (key == "grip_smoothing_fast") { current_preset.grip_estimation.grip_smoothing_fast = std::stof(value); return true; }
    if (key == "grip_smoothing_sensitivity") { current_preset.grip_estimation.grip_smoothing_sensitivity = std::stof(value); return true; }
    if (key == "rear_align_effect") { current_preset.rear_axle.rear_align_effect = std::stof(value); return true; }
    if (key == "kerb_strike_rejection") { current_preset.rear_axle.kerb_strike_rejection = std::stof(value); return true; }
    if (key == "sop_yaw_gain") { current_preset.rear_axle.sop_yaw_gain = std::stof(value); return true; }
    if (key == "yaw_kick_threshold") { current_preset.rear_axle.yaw_kick_threshold = std::stof(value); return true; }
    if (key == "unloaded_yaw_gain") { current_preset.rear_axle.unloaded_yaw_gain = std::stof(value); return true; }
    if (key == "unloaded_yaw_threshold") { current_preset.rear_axle.unloaded_yaw_threshold = std::stof(value); return true; }
    if (key == "unloaded_yaw_sens") { current_preset.rear_axle.unloaded_yaw_sens = std::stof(value); return true; }
    if (key == "unloaded_yaw_gamma") { current_preset.rear_axle.unloaded_yaw_gamma = std::stof(value); return true; }
    if (key == "unloaded_yaw_punch") { current_preset.rear_axle.unloaded_yaw_punch = std::stof(value); return true; }
    if (key == "power_yaw_gain") { current_preset.rear_axle.power_yaw_gain = std::stof(value); return true; }
    if (key == "power_yaw_threshold") { current_preset.rear_axle.power_yaw_threshold = std::stof(value); return true; }
    if (key == "power_slip_threshold") { current_preset.rear_axle.power_slip_threshold = std::stof(value); return true; }
    if (key == "power_yaw_gamma") { current_preset.rear_axle.power_yaw_gamma = std::stof(value); return true; }
    if (key == "power_yaw_punch") { current_preset.rear_axle.power_yaw_punch = std::stof(value); return true; }
    if (key == "yaw_accel_smoothing") { current_preset.rear_axle.yaw_accel_smoothing = std::stof(value); return true; }
    if (key == "gyro_gain") { current_preset.advanced.gyro_gain = (std::min)(1.0f, std::stof(value)); return true; }
    if (key == "stationary_damping") { current_preset.advanced.stationary_damping = (std::min)(1.0f, std::stof(value)); return true; }
    if (key == "gyro_smoothing_factor") { current_preset.advanced.gyro_smoothing = std::stof(value); return true; }
    if (key == "optimal_slip_angle") { current_preset.grip_estimation.optimal_slip_angle = std::stof(value); return true; }
    if (key == "optimal_slip_ratio") { current_preset.grip_estimation.optimal_slip_ratio = std::stof(value); return true; }
    if (key == "slope_detection_enabled") { current_preset.slope_detection.enabled = (value == "1"); return true; }
    if (key == "slope_sg_window") { current_preset.slope_detection.sg_window = std::stoi(value); return true; }
    if (key == "slope_sensitivity") {
        float sens = std::stof(value);
        current_preset.slope_detection.sensitivity = sens;
        current_preset.slope_detection.max_threshold = current_preset.slope_detection.min_threshold - (8.0f / (std::max)(0.1f, sens));
        return true;
    }
    if (key == "slope_negative_threshold" || key == "slope_min_threshold") { current_preset.slope_detection.min_threshold = std::stof(value); return true; }
    if (key == "slope_smoothing_tau") { current_preset.slope_detection.smoothing_tau = std::stof(value); return true; }
    if (key == "slope_max_threshold") { current_preset.slope_detection.max_threshold = std::stof(value); return true; }
    if (key == "slope_alpha_threshold") {
        float v = std::stof(value);
        if (v < 0.001f || v > 0.1f) v = 0.02f;
        current_preset.slope_detection.alpha_threshold = v;
        return true;
    }
    if (key == "slope_decay_rate") {
        float v = std::stof(value);
        if (v < 0.1f || v > 20.0f) v = 5.0f;
        current_preset.slope_detection.decay_rate = v;
        return true;
    }
    if (key == "slope_confidence_enabled") { current_preset.slope_detection.confidence_enabled = (value == "1"); return true; }
    if (key == "slope_g_slew_limit") { current_preset.slope_detection.g_slew_limit = std::stof(value); return true; }
    if (key == "slope_use_torque") { current_preset.slope_detection.use_torque = (value == "1"); return true; }
    if (key == "slope_torque_sensitivity") { current_preset.slope_detection.torque_sensitivity = std::stof(value); return true; }
    if (key == "slope_confidence_max_rate") { current_preset.slope_detection.confidence_max_rate = std::stof(value); return true; }
    if (key == "slip_angle_smoothing") { current_preset.grip_estimation.slip_angle_smoothing = std::stof(value); return true; }
    if (key == "chassis_inertia_smoothing") { current_preset.grip_estimation.chassis_inertia_smoothing = std::stof(value); return true; }
    if (key == "load_sensitivity_enabled") { current_preset.grip_estimation.load_sensitivity_enabled = (value == "1" || value == "true"); return true; }
    return false;
}

bool Config::ParseBrakingLine(const std::string& key, const std::string& value, Preset& current_preset) {
    if (key == "lockup_enabled") { current_preset.braking.lockup_enabled = (value == "1" || value == "true"); return true; }
    if (key == "lockup_gain") { current_preset.braking.lockup_gain = std::stof(value); return true; }
    if (key == "lockup_start_pct") { current_preset.braking.lockup_start_pct = std::stof(value); return true; }
    if (key == "lockup_full_pct") { current_preset.braking.lockup_full_pct = std::stof(value); return true; }
    if (key == "lockup_rear_boost") { current_preset.braking.lockup_rear_boost = std::stof(value); return true; }
    if (key == "lockup_gamma") { current_preset.braking.lockup_gamma = std::stof(value); return true; }
    if (key == "lockup_prediction_sens") { current_preset.braking.lockup_prediction_sens = std::stof(value); return true; }
    if (key == "lockup_bump_reject") { current_preset.braking.lockup_bump_reject = std::stof(value); return true; }
    if (key == "brake_load_cap") { current_preset.braking.brake_load_cap = (std::min)(10.0f, std::stof(value)); return true; }
    if (key == "abs_pulse_enabled") { current_preset.braking.abs_pulse_enabled = std::stoi(value); return true; }
    if (key == "abs_gain") { current_preset.braking.abs_gain = std::stof(value); return true; }
    if (key == "abs_freq") { current_preset.braking.abs_freq = std::stof(value); return true; }
    if (key == "lockup_freq_scale") { current_preset.braking.lockup_freq_scale = std::stof(value); return true; }
    return false;
}

bool Config::ParseVibrationLine(const std::string& key, const std::string& value, Preset& current_preset) {
    if (key == "flatspot_suppression") { current_preset.front_axle.flatspot_suppression = (value == "1" || value == "true"); return true; }
    if (key == "notch_q") { current_preset.front_axle.notch_q = std::stof(value); return true; }
    if (key == "flatspot_strength") { current_preset.front_axle.flatspot_strength = std::stof(value); return true; }
    if (key == "static_notch_enabled") { current_preset.front_axle.static_notch_enabled = (value == "1" || value == "true"); return true; }
    if (key == "static_notch_freq") { current_preset.front_axle.static_notch_freq = std::stof(value); return true; }
    if (key == "static_notch_width") { current_preset.front_axle.static_notch_width = std::stof(value); return true; }
    if (key == "texture_load_cap" || key == "max_load_factor") { current_preset.vibration.texture_load_cap = std::stof(value); return true; }
    if (key == "spin_enabled") { current_preset.vibration.spin_enabled = (value == "1" || value == "true"); return true; }
    if (key == "spin_gain") { current_preset.vibration.spin_gain = std::stof(value); return true; }
    if (key == "spin_freq_scale") { current_preset.vibration.spin_freq_scale = std::stof(value); return true; }
    if (key == "slide_enabled") { current_preset.vibration.slide_enabled = (value == "1" || value == "true"); return true; }
    if (key == "slide_gain") { current_preset.vibration.slide_gain = std::stof(value); return true; }
    if (key == "slide_freq") { current_preset.vibration.slide_freq = std::stof(value); return true; }
    if (key == "road_enabled") { current_preset.vibration.road_enabled = (value == "1" || value == "true"); return true; }
    if (key == "road_gain") { current_preset.vibration.road_gain = std::stof(value); return true; }
    if (key == "vibration_gain" || key == "tactile_gain") { current_preset.vibration.vibration_gain = std::stof(value); return true; }
    if (key == "scrub_drag_gain") { current_preset.vibration.scrub_drag_gain = std::stof(value); return true; }
    if (key == "bottoming_enabled") { current_preset.vibration.bottoming_enabled = (value == "1" || value == "true"); return true; }
    if (key == "bottoming_gain") { current_preset.vibration.bottoming_gain = std::stof(value); return true; }
    if (key == "bottoming_method") { current_preset.vibration.bottoming_method = std::stoi(value); return true; }
    if (key == "dynamic_normalization_enabled") { current_preset.general.dynamic_normalization_enabled = (value == "1" || value == "true"); return true; }
    if (key == "auto_load_normalization_enabled") { current_preset.general.auto_load_normalization_enabled = (value == "1" || value == "true"); return true; }
    if (key == "soft_lock_enabled") { current_preset.advanced.soft_lock_enabled = (value == "1" || value == "true"); return true; }
    if (key == "soft_lock_stiffness") { current_preset.advanced.soft_lock_stiffness = std::stof(value); return true; }
    if (key == "soft_lock_damping") { current_preset.advanced.soft_lock_damping = std::stof(value); return true; }
    if (key == "speed_gate_lower") { current_preset.advanced.speed_gate_lower = std::stof(value); return true; }
    if (key == "speed_gate_upper") { current_preset.advanced.speed_gate_upper = std::stof(value); return true; }
    if (key == "road_fallback_scale") { current_preset.advanced.road_fallback_scale = std::stof(value); return true; }
    if (key == "understeer_affects_sop") { current_preset.advanced.understeer_affects_sop = (value == "1" || value == "true"); return true; }
    if (key == "aux_telemetry_reconstruction") { current_preset.advanced.aux_telemetry_reconstruction = std::stoi(value); return true; }
    return false;
}

bool Config::ParseSafetyLine(const std::string& key, const std::string& value, Preset& current_preset) {
    if (key == "safety_window_duration") { current_preset.safety.window_duration = std::stof(value); return true; }
    if (key == "safety_gain_reduction") { current_preset.safety.gain_reduction = std::stof(value); return true; }
    if (key == "safety_smoothing_tau") { current_preset.safety.smoothing_tau = std::stof(value); return true; }
    if (key == "spike_detection_threshold") { current_preset.safety.spike_detection_threshold = std::stof(value); return true; }
    if (key == "immediate_spike_threshold") { current_preset.safety.immediate_spike_threshold = std::stof(value); return true; }
    if (key == "safety_slew_full_scale_time_s") { current_preset.safety.slew_full_scale_time_s = std::stof(value); return true; }
    if (key == "stutter_safety_enabled") { current_preset.safety.stutter_safety_enabled = (value == "1" || value == "true"); return true; }
    if (key == "stutter_threshold") { current_preset.safety.stutter_threshold = std::stof(value); return true; }
    return false;
}

bool Config::SyncSystemLine(const std::string& key, const std::string& value, FFBEngine& engine, std::string& config_version, bool& legacy_torque_hack, float& legacy_torque_val, bool& needs_save) {
    if (key == "always_on_top") { m_always_on_top = (value == "1" || value == "true"); return true; }
    if (key == "last_device_guid") { m_last_device_guid = value; return true; }
    if (key == "last_preset_name") { m_last_preset_name = value; return true; }
    if (key == "win_pos_x") { win_pos_x = std::stoi(value); return true; }
    if (key == "win_pos_y") { win_pos_y = std::stoi(value); return true; }
    if (key == "win_w_small") { win_w_small = std::stoi(value); return true; }
    if (key == "win_h_small") { win_h_small = std::stoi(value); return true; }
    if (key == "win_w_large") { win_w_large = std::stoi(value); return true; }
    if (key == "win_h_large") { win_h_large = std::stoi(value); return true; }
    if (key == "show_graphs") { show_graphs = (value == "1" || value == "true"); return true; }
    if (key == "auto_start_logging") { m_auto_start_logging = (value == "1" || value == "true"); return true; }
    if (key == "log_path") { m_log_path = value; return true; }
    if (key == "invert_force") { engine.m_invert_force = (value == "1" || value == "true"); return true; }
    if (key == "gain") { engine.m_general.gain = std::stof(value); return true; }
    if (key == "wheelbase_max_nm") { engine.m_general.wheelbase_max_nm = std::stof(value); return true; }
    if (key == "target_rim_nm") { engine.m_general.target_rim_nm = std::stof(value); return true; }
    if (key == "min_force") { engine.m_general.min_force = std::stof(value); return true; }
    if (key == "max_torque_ref") {
        float old_val = std::stof(value);
        if (old_val > 40.0f) {
            engine.m_general.wheelbase_max_nm = 15.0f;
            engine.m_general.target_rim_nm = 10.0f;
            legacy_torque_hack = true;
            legacy_torque_val = old_val;
        } else {
            engine.m_general.wheelbase_max_nm = old_val;
            engine.m_general.target_rim_nm = old_val;
        }
        needs_save = true;
        return true;
    }
    if (key == "rest_api_fallback_enabled") { engine.m_advanced.rest_api_enabled = (value == "1" || value == "true"); return true; }
    if (key == "rest_api_port") { engine.m_advanced.rest_api_port = std::stoi(value); return true; }
    return false;
}

bool Config::SyncPhysicsLine(const std::string& key, const std::string& value, FFBEngine& engine, std::string& config_version, bool& needs_save) {
    if (key == "understeer" || key == "understeer_effect") {
        float val = std::stof(value);
        if (val > 2.0f) val /= 100.0f;
        engine.m_front_axle.understeer_effect = (std::min)(2.0f, (std::max)(0.0f, val));
        return true;
    }
    if (key == "understeer_gamma") { engine.m_front_axle.understeer_gamma = std::stof(value); return true; }
    if (key == "steering_shaft_gain") { engine.m_front_axle.steering_shaft_gain = std::stof(value); return true; }
    if (key == "ingame_ffb_gain") { engine.m_front_axle.ingame_ffb_gain = std::stof(value); return true; }
    if (key == "steering_shaft_smoothing") { engine.m_front_axle.steering_shaft_smoothing = std::stof(value); return true; }
    if (key == "torque_source") { engine.m_front_axle.torque_source = std::stoi(value); return true; }
    if (key == "steering_100hz_reconstruction") { engine.m_front_axle.steering_100hz_reconstruction = std::stoi(value); return true; }
    if (key == "torque_passthrough") { engine.m_front_axle.torque_passthrough = (value == "1" || value == "true"); return true; }
    if (key == "sop") { engine.m_rear_axle.sop_effect = std::stof(value); return true; }
    if (key == "lateral_load_effect") { engine.m_load_forces.lat_load_effect = std::stof(value); return true; }
    if (key == "lat_load_transform") { engine.m_load_forces.lat_load_transform = std::clamp(std::stoi(value), 0, 3); return true; }
    if (key == "sop_scale") { engine.m_rear_axle.sop_scale = std::stof(value); return true; }
    if (key == "sop_smoothing_factor" || key == "smoothing") {
        float val = std::stof(value);
        if (!config_version.empty() && IsVersionLessEqual(config_version, "0.7.146")) {
            val = 0.0f;
            needs_save = true;
        }
        engine.m_rear_axle.sop_smoothing_factor = val;
        return true;
    }
    if (key == "oversteer_boost") { engine.m_rear_axle.oversteer_boost = std::stof(value); return true; }
    if (key == "long_load_effect" || key == "dynamic_weight_gain") { engine.m_load_forces.long_load_effect = std::stof(value); return true; }
    if (key == "long_load_smoothing" || key == "dynamic_weight_smoothing") { engine.m_load_forces.long_load_smoothing = std::stof(value); return true; }
    if (key == "long_load_transform") { engine.m_load_forces.long_load_transform = std::clamp(std::stoi(value), 0, 3); return true; }
    if (key == "grip_smoothing_steady") { engine.m_grip_estimation.grip_smoothing_steady = std::stof(value); return true; }
    if (key == "grip_smoothing_fast") { engine.m_grip_estimation.grip_smoothing_fast = std::stof(value); return true; }
    if (key == "grip_smoothing_sensitivity") { engine.m_grip_estimation.grip_smoothing_sensitivity = std::stof(value); return true; }
    if (key == "rear_align_effect") { engine.m_rear_axle.rear_align_effect = std::stof(value); return true; }
    if (key == "kerb_strike_rejection") { engine.m_rear_axle.kerb_strike_rejection = std::stof(value); return true; }
    if (key == "sop_yaw_gain") { engine.m_rear_axle.sop_yaw_gain = std::stof(value); return true; }
    if (key == "yaw_kick_threshold") { engine.m_rear_axle.yaw_kick_threshold = std::stof(value); return true; }
    if (key == "unloaded_yaw_gain") { engine.m_rear_axle.unloaded_yaw_gain = std::stof(value); return true; }
    if (key == "unloaded_yaw_threshold") { engine.m_rear_axle.unloaded_yaw_threshold = std::stof(value); return true; }
    if (key == "unloaded_yaw_sens") { engine.m_rear_axle.unloaded_yaw_sens = std::stof(value); return true; }
    if (key == "unloaded_yaw_gamma") { engine.m_rear_axle.unloaded_yaw_gamma = std::stof(value); return true; }
    if (key == "unloaded_yaw_punch") { engine.m_rear_axle.unloaded_yaw_punch = std::stof(value); return true; }
    if (key == "power_yaw_gain") { engine.m_rear_axle.power_yaw_gain = std::stof(value); return true; }
    if (key == "power_yaw_threshold") { engine.m_rear_axle.power_yaw_threshold = std::stof(value); return true; }
    if (key == "power_slip_threshold") { engine.m_rear_axle.power_slip_threshold = std::stof(value); return true; }
    if (key == "power_yaw_gamma") { engine.m_rear_axle.power_yaw_gamma = std::stof(value); return true; }
    if (key == "power_yaw_punch") { engine.m_rear_axle.power_yaw_punch = std::stof(value); return true; }
    if (key == "yaw_accel_smoothing") { engine.m_rear_axle.yaw_accel_smoothing = std::stof(value); return true; }
    if (key == "gyro_gain") { engine.m_advanced.gyro_gain = (std::min)(1.0f, std::stof(value)); return true; }
    if (key == "stationary_damping") { engine.m_advanced.stationary_damping = (std::min)(1.0f, std::stof(value)); return true; }
    if (key == "gyro_smoothing_factor") { engine.m_advanced.gyro_smoothing = std::stof(value); return true; }
    if (key == "optimal_slip_angle") {
        float v = std::stof(value);
        if (v < 0.01f) v = 0.10f;
        engine.m_grip_estimation.optimal_slip_angle = v;
        return true;
    }
    if (key == "optimal_slip_ratio") {
        float v = std::stof(value);
        if (v < 0.01f) v = 0.12f;
        engine.m_grip_estimation.optimal_slip_ratio = v;
        return true;
    }
    if (key == "slope_detection_enabled") { engine.m_slope_detection.enabled = (value == "1"); return true; }
    if (key == "slope_sg_window") { engine.m_slope_detection.sg_window = std::stoi(value); return true; }
    if (key == "slope_sensitivity") {
        float sens = std::stof(value);
        engine.m_slope_detection.sensitivity = sens;
        // Migration logic: update max_threshold from legacy sensitivity
        engine.m_slope_detection.max_threshold = engine.m_slope_detection.min_threshold - (8.0f / (std::max)(0.1f, sens));
        return true;
    }
    if (key == "slope_negative_threshold" || key == "slope_min_threshold") { engine.m_slope_detection.min_threshold = std::stof(value); return true; }
    if (key == "slope_smoothing_tau") { engine.m_slope_detection.smoothing_tau = std::stof(value); return true; }
    if (key == "slope_max_threshold") { engine.m_slope_detection.max_threshold = std::stof(value); return true; }
    if (key == "slope_alpha_threshold") {
        float v = std::stof(value);
        if (v < 0.001f || v > 0.1f) v = 0.02f;
        engine.m_slope_detection.alpha_threshold = v;
        return true;
    }
    if (key == "slope_decay_rate") {
        float v = std::stof(value);
        if (v < 0.1f || v > 20.0f) v = 5.0f;
        engine.m_slope_detection.decay_rate = v;
        return true;
    }
    if (key == "slope_confidence_enabled") { engine.m_slope_detection.confidence_enabled = (value == "1"); return true; }
    if (key == "slope_g_slew_limit") { engine.m_slope_detection.g_slew_limit = std::stof(value); return true; }
    if (key == "slope_use_torque") { engine.m_slope_detection.use_torque = (value == "1"); return true; }
    if (key == "slope_torque_sensitivity") { engine.m_slope_detection.torque_sensitivity = std::stof(value); return true; }
    if (key == "slope_confidence_max_rate") { engine.m_slope_detection.confidence_max_rate = std::stof(value); return true; }
    if (key == "slip_angle_smoothing") { engine.m_grip_estimation.slip_angle_smoothing = std::stof(value); return true; }
    if (key == "chassis_inertia_smoothing") { engine.m_grip_estimation.chassis_inertia_smoothing = std::stof(value); return true; }
    if (key == "load_sensitivity_enabled") { engine.m_grip_estimation.load_sensitivity_enabled = (value == "1" || value == "true"); return true; }
    if (key == "speed_gate_lower") { engine.m_advanced.speed_gate_lower = std::stof(value); return true; }
    if (key == "speed_gate_upper") { engine.m_advanced.speed_gate_upper = std::stof(value); return true; }
    if (key == "road_fallback_scale") { engine.m_advanced.road_fallback_scale = std::stof(value); return true; }
    if (key == "understeer_affects_sop") { engine.m_advanced.understeer_affects_sop = (value == "1" || value == "true"); return true; }
    if (key == "aux_telemetry_reconstruction") {
        engine.m_advanced.aux_telemetry_reconstruction = std::stoi(value);
        engine.UpdateUpsamplerModes();
        return true;
    }
    return false;
}

bool Config::SyncBrakingLine(const std::string& key, const std::string& value, FFBEngine& engine) {
    if (key == "lockup_enabled") { engine.m_braking.lockup_enabled = (value == "1" || value == "true"); return true; }
    if (key == "lockup_gain") { engine.m_braking.lockup_gain = std::stof(value); return true; }
    if (key == "lockup_start_pct") { engine.m_braking.lockup_start_pct = std::stof(value); return true; }
    if (key == "lockup_full_pct") { engine.m_braking.lockup_full_pct = std::stof(value); return true; }
    if (key == "lockup_rear_boost") { engine.m_braking.lockup_rear_boost = std::stof(value); return true; }
    if (key == "lockup_gamma") { engine.m_braking.lockup_gamma = std::stof(value); return true; }
    if (key == "lockup_prediction_sens") { engine.m_braking.lockup_prediction_sens = std::stof(value); return true; }
    if (key == "lockup_bump_reject") { engine.m_braking.lockup_bump_reject = std::stof(value); return true; }
    if (key == "brake_load_cap") { engine.m_braking.brake_load_cap = (std::min)(10.0f, std::stof(value)); return true; }
    if (key == "abs_pulse_enabled") { engine.m_braking.abs_pulse_enabled = (value == "1" || value == "true"); return true; }
    if (key == "abs_gain") { engine.m_braking.abs_gain = std::stof(value); return true; }
    if (key == "abs_freq") { engine.m_braking.abs_freq = std::stof(value); return true; }
    if (key == "lockup_freq_scale") { engine.m_braking.lockup_freq_scale = std::stof(value); return true; }
    return false;
}

bool Config::SyncVibrationLine(const std::string& key, const std::string& value, FFBEngine& engine) {
    if (key == "flatspot_suppression") { engine.m_front_axle.flatspot_suppression = (value == "1" || value == "true"); return true; }
    if (key == "notch_q") { engine.m_front_axle.notch_q = std::stof(value); return true; }
    if (key == "flatspot_strength") { engine.m_front_axle.flatspot_strength = std::stof(value); return true; }
    if (key == "static_notch_enabled") { engine.m_front_axle.static_notch_enabled = (value == "1" || value == "true"); return true; }
    if (key == "static_notch_freq") { engine.m_front_axle.static_notch_freq = std::stof(value); return true; }
    if (key == "static_notch_width") { engine.m_front_axle.static_notch_width = std::stof(value); return true; }
    if (key == "texture_load_cap" || key == "max_load_factor") { engine.m_vibration.texture_load_cap = std::stof(value); return true; }
    if (key == "spin_enabled") { engine.m_vibration.spin_enabled = (value == "1" || value == "true"); return true; }
    if (key == "spin_gain") { engine.m_vibration.spin_gain = std::stof(value); return true; }
    if (key == "spin_freq_scale") { engine.m_vibration.spin_freq_scale = std::stof(value); return true; }
    if (key == "slide_enabled") { engine.m_vibration.slide_enabled = (value == "1" || value == "true"); return true; }
    if (key == "slide_gain") { engine.m_vibration.slide_gain = std::stof(value); return true; }
    if (key == "slide_freq") { engine.m_vibration.slide_freq = std::stof(value); return true; }
    if (key == "road_enabled") { engine.m_vibration.road_enabled = (value == "1" || value == "true"); return true; }
    if (key == "road_gain") { engine.m_vibration.road_gain = std::stof(value); return true; }
    if (key == "vibration_gain" || key == "tactile_gain") { engine.m_vibration.vibration_gain = std::stof(value); return true; }
    if (key == "bottoming_enabled") { engine.m_vibration.bottoming_enabled = (value == "1" || value == "true"); return true; }
    if (key == "bottoming_gain") { engine.m_vibration.bottoming_gain = std::stof(value); return true; }
    if (key == "dynamic_normalization_enabled") { engine.m_general.dynamic_normalization_enabled = (value == "1" || value == "true"); return true; }
    if (key == "auto_load_normalization_enabled") { engine.m_general.auto_load_normalization_enabled = (value == "1" || value == "true"); return true; }
    if (key == "soft_lock_enabled") { engine.m_advanced.soft_lock_enabled = (value == "1" || value == "true"); return true; }
    if (key == "soft_lock_stiffness") { engine.m_advanced.soft_lock_stiffness = std::stof(value); return true; }
    if (key == "soft_lock_damping") { engine.m_advanced.soft_lock_damping = std::stof(value); return true; }
    if (key == "scrub_drag_gain") { engine.m_vibration.scrub_drag_gain = std::stof(value); return true; }
    if (key == "bottoming_method") { engine.m_vibration.bottoming_method = std::stoi(value); return true; }
    return false;
}

bool Config::SyncSafetyLine(const std::string& key, const std::string& value, FFBEngine& engine) {
    if (key == "safety_window_duration") { engine.m_safety.m_config.window_duration = std::stof(value); return true; }
    if (key == "safety_gain_reduction") { engine.m_safety.m_config.gain_reduction = std::stof(value); return true; }
    if (key == "safety_smoothing_tau") { engine.m_safety.m_config.smoothing_tau = std::stof(value); return true; }
    if (key == "spike_detection_threshold") { engine.m_safety.m_config.spike_detection_threshold = std::stof(value); return true; }
    if (key == "immediate_spike_threshold") { engine.m_safety.m_config.immediate_spike_threshold = std::stof(value); return true; }
    if (key == "safety_slew_full_scale_time_s") { engine.m_safety.m_config.slew_full_scale_time_s = std::stof(value); return true; }
    if (key == "stutter_safety_enabled") { engine.m_safety.m_config.stutter_safety_enabled = (value == "1" || value == "true"); return true; }
    if (key == "stutter_threshold") { engine.m_safety.m_config.stutter_threshold = std::stof(value); return true; }
    return false;
}

void Config::ParsePresetLine(const std::string& line, Preset& current_preset, std::string& current_preset_version, bool& needs_save, bool& legacy_torque_hack, float& legacy_torque_val) {
    std::istringstream is_line(line);
    std::string key;
    if (std::getline(is_line, key, '=')) {
        std::string value;
        if (std::getline(is_line, value)) {
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);

            try {
                if (ParseSystemLine(key, value, current_preset, current_preset_version, needs_save, legacy_torque_hack, legacy_torque_val)) return;
                if (ParsePhysicsLine(key, value, current_preset)) return;
                if (ParseBrakingLine(key, value, current_preset)) return;
                if (ParseVibrationLine(key, value, current_preset)) return;
                if (ParseSafetyLine(key, value, current_preset)) return;
            } catch (...) { Logger::Get().Log("[Config] ParsePresetLine Error."); }
        }
    }
}

// --- End Legacy INI Helpers ---

static void FinalizePreset(Preset& p, const std::string& name, const std::string& version, bool hack, float hack_val) {
    p.name = name;
    if (!version.empty()) p.app_version = version;
    
    if (hack && IsVersionLessEqual(version, "0.7.66")) {
        p.general.gain *= (15.0f / hack_val);
    }
    // Apply Issue #37 reset if necessary
    if (IsVersionLessEqual(version, "0.7.146")) {
        p.rear_axle.sop_smoothing_factor = 0.0f;
    }
    p.Validate();
    // Add or Update
    bool found = false;
    for (auto& existing : Config::presets) {
        if (existing.name == name && !existing.is_builtin) {
            existing = p;
            found = true;
            break;
        }
    }
    if (!found) Config::presets.push_back(p);
}

void Config::Load(FFBEngine& engine, const std::string& filename) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);

    std::string final_path = filename.empty() ? m_config_path : filename;
    if (!std::filesystem::exists(final_path)) {
        // Auto-migration check (one-time)
        if (final_path.size() > 5 && final_path.substr(final_path.size() - 5) == ".toml") {
            std::string ini_path = final_path.substr(0, final_path.size() - 5) + ".ini";
            if (std::filesystem::exists(ini_path)) {
                Logger::Get().Log("[Config] TOML not found, migrating legacy INI: %s", ini_path.c_str());
                MigrateFromLegacyIni(engine, ini_path);
                Save(engine, final_path);
                // v0.7.223: Disable automatic renaming to reduce ransomware-like behavioral flags
                // try { std::filesystem::rename(ini_path, ini_path + ".bak"); } catch (...) {}
                return;
            }
        }
        LoadPresets();
        return;
    }

    bool toml_loaded = false;
    try {
        toml::table tbl = toml::parse_file(final_path);
        if (tbl.contains("System") || tbl.contains("General") || tbl.contains("Presets") || tbl.contains("FrontAxle")) {
            toml_loaded = true;

            // Phase 3 One-Time Migration: Extract [Presets] table from config.toml
            if (tbl.contains("Presets")) {
                if (auto ps = tbl["Presets"].as_table()) {
                    Logger::Get().Log("[Config] Migrating [Presets] from %s to individual files...", final_path.c_str());
                    for (auto& [key, value] : *ps) {
                        if (auto p_tbl = value.as_table()) {
                            Preset pr(std::string(key.str()), false);
                            TomlToPreset(*p_tbl, pr);
                            SaveUserPresetFile(pr);
                        }
                    }
                }
                tbl.erase("Presets");

                // Overwrite config.toml without Presets table
                std::ofstream file(final_path);
                if (file.is_open()) {
                    file << tbl;
                }
            }

            LoadPresets();

            if (auto sys = tbl["System"].as_table()) {
                if (auto val = (*sys)["always_on_top"].value<bool>()) m_always_on_top = *val;
                if (auto val = (*sys)["last_device_guid"].as_string()) m_last_device_guid = val->get();
                if (auto val = (*sys)["last_preset_name"].as_string()) m_last_preset_name = val->get();
                if (auto val = (*sys)["log_path"].as_string()) m_log_path = val->get();
                if (auto val = (*sys)["win_pos_x"].value<int64_t>()) win_pos_x = (int)*val;
                if (auto val = (*sys)["win_pos_y"].value<int64_t>()) win_pos_y = (int)*val;
                if (auto val = (*sys)["win_w_small"].value<int64_t>()) win_w_small = (int)*val;
                if (auto val = (*sys)["win_h_small"].value<int64_t>()) win_h_small = (int)*val;
                if (auto val = (*sys)["win_w_large"].value<int64_t>()) win_w_large = (int)*val;
                if (auto val = (*sys)["win_h_large"].value<int64_t>()) win_h_large = (int)*val;
                if (auto val = (*sys)["show_graphs"].value<bool>()) show_graphs = *val;
                if (auto val = (*sys)["auto_start_logging"].value<bool>()) m_auto_start_logging = *val;
                if (auto val = (*sys)["invert_force"].value<bool>()) engine.m_invert_force = *val;
                if (auto val = (*sys)["session_peak_torque"].value<double>()) {
                    if (std::isfinite(*val) && *val > 0.01) engine.m_session_peak_torque = *val;
                }
                if (auto val = (*sys)["auto_peak_front_load"].value<double>()) {
                    if (std::isfinite(*val) && *val > 10.0) engine.m_auto_peak_front_load = *val;
                }
            }

            Preset p("Temporary", false);
            p.UpdateFromEngine(engine);
            TomlToPreset(tbl, p);

            if (p.grip_estimation.optimal_slip_angle < 0.01f) p.grip_estimation.optimal_slip_angle = 0.10f;
            if (p.grip_estimation.optimal_slip_ratio < 0.01f) p.grip_estimation.optimal_slip_ratio = 0.12f;

            p.Apply(engine);

            if (auto sl = tbl["StaticLoads"].as_table()) {
                std::lock_guard<std::recursive_mutex> sl_lock(m_static_loads_mutex);
                for (auto& [key, value] : *sl) {
                    if (auto val = value.value<double>()) {
                        m_saved_static_loads[std::string(key.str())] = *val;
                    }
                }
            }
        }
    } catch (...) {}

    if (!toml_loaded) {
        MigrateFromLegacyIni(engine, final_path);
    }

    // Individual validations and legacy resets (redundant but safe)
    if (engine.m_grip_estimation.optimal_slip_angle < 0.01f) engine.m_grip_estimation.optimal_slip_angle = 0.10f;
    if (engine.m_grip_estimation.optimal_slip_ratio < 0.01f) engine.m_grip_estimation.optimal_slip_ratio = 0.12f;
    

    if (engine.m_braking.lockup_gain > 3.0f) engine.m_braking.lockup_gain = 3.0f;

    engine.m_general.Validate();
    engine.m_front_axle.Validate();
    engine.m_rear_axle.Validate();
    engine.m_load_forces.Validate();
    engine.m_grip_estimation.Validate();
    engine.m_slope_detection.Validate();
    engine.m_braking.Validate();
    engine.m_vibration.Validate();
    engine.m_advanced.Validate();
    engine.m_safety.m_config.Validate();
}

void Config::Save(const FFBEngine& engine, const std::string& filename) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
    std::string final_path = filename.empty() ? m_config_path : filename;

    toml::table tbl;

    tbl.insert("System", toml::table{
        {"app_version", LMUFFB_VERSION},
        {"always_on_top", m_always_on_top},
        {"last_device_guid", m_last_device_guid},
        {"last_preset_name", m_last_preset_name},
        {"win_pos_x", win_pos_x},
        {"win_pos_y", win_pos_y},
        {"win_w_small", win_w_small},
        {"win_h_small", win_h_small},
        {"win_w_large", win_w_large},
        {"win_h_large", win_h_large},
        {"show_graphs", show_graphs},
        {"auto_start_logging", m_auto_start_logging},
        {"log_path", m_log_path},
        {"invert_force", engine.m_invert_force},
        {"session_peak_torque", engine.m_session_peak_torque},
        {"auto_peak_front_load", engine.m_auto_peak_front_load}
    });

    // Merge categories from engine config into the main table
    Preset p("Engine", false);
    p.UpdateFromEngine(engine);
    toml::table config_tbl = PresetToToml(p);
    for (auto& [key, value] : config_tbl) {
        tbl.insert(key, std::move(value));
    }

    {
        std::lock_guard<std::recursive_mutex> sl_lock(m_static_loads_mutex);
        if (!m_saved_static_loads.empty()) {
            toml::table sl_tbl;
            for (auto const& [key, val] : m_saved_static_loads) {
                sl_tbl.insert(key, val);
            }
            tbl.insert("StaticLoads", sl_tbl);
        }
    }

    std::ofstream file(final_path);
    if (file.is_open()) {
        file << tbl;
        m_needs_save = false;
    }
}

void Config::LoadPresets() {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
    presets.clear();

#ifdef _WIN32
    // 1. Load Built-in Presets from Windows Resources
    static const std::vector<std::pair<int, std::string>> resource_presets = {
        { IDR_PRESET_DEFAULT, "Default" },
        { IDR_PRESET_FANATEC_CSL_DD__GT_DD_PRO, "Fanatec CSL DD / GT DD Pro" },
        { IDR_PRESET_FANATEC_PODIUM_DD1DD2, "Fanatec Podium DD1/DD2" },
        { IDR_PRESET_GM___YAW_KICK_DD_21_NM__MOZA_R21_ULTRA_, "GM + Yaw Kick DD 21 Nm (Moza R21 Ultra)" },
        { IDR_PRESET_GM_DD_21_NM__MOZA_R21_ULTRA_, "GM DD 21 Nm (Moza R21 Ultra)" },
        { IDR_PRESET_GT3_DD_15_NM__SIMAGIC_ALPHA_, "GT3 DD 15 Nm (Simagic Alpha)" },
        { IDR_PRESET_GUIDE_BRAKING_LOCKUP, "Guide: Braking & Lockup" },
        { IDR_PRESET_GUIDE_GYROSCOPIC_DAMPING, "Guide: Gyroscopic Damping" },
        { IDR_PRESET_GUIDE_OVERSTEER__REAR_GRIP_, "Guide: Oversteer (Rear Grip)" },
        { IDR_PRESET_GUIDE_SLIDE_TEXTURE__SCRUB_, "Guide: Slide & Texture (Scrub)" },
        { IDR_PRESET_GUIDE_SOP_YAW__KICK_, "Guide: SoP & Yaw (Kick)" },
        { IDR_PRESET_GUIDE_TRACTION_LOSS__SPIN_, "Guide: Traction Loss (Spin)" },
        { IDR_PRESET_GUIDE_UNDERSTEER__FRONT_GRIP_, "Guide: Understeer (Front Grip)" },
        { IDR_PRESET_LMPXHY_DD_15_NM__SIMAGIC_ALPHA_, "LMPx/HY DD 15 Nm (Simagic Alpha)" },
        { IDR_PRESET_LOGITECH_G25G27G29G920, "Logitech G25/G27/G29/G920" },
        { IDR_PRESET_MOZA_R5R9R16R21, "Moza R5 / R9 / R16 / R21" },
        { IDR_PRESET_SIMAGIC_ALPHAALPHA_MINIALPHA_U, "Simagic Alpha / Alpha Mini / Alpha U" },
        { IDR_PRESET_SIMUCUBE_2_SPORTPROULTIMATE, "Simucube 2 Sport / Pro / Ultimate" },
        { IDR_PRESET_T300_V0_7_164, "Thrustmaster T300 (v0.7.164 Optimized)" },
        { IDR_PRESET_TEST_GAME_BASE_FFB_ONLY, "Test: Game Base FFB Only" },
        { IDR_PRESET_TEST_NO_EFFECTS, "Test: No Effects" },
        { IDR_PRESET_TEST_REAR_ALIGN_TORQUE_ONLY, "Test: Rear Align Torque Only" },
        { IDR_PRESET_TEST_SLIDE_TEXTURE_ONLY, "Test: Slide & Texture Only" },
        { IDR_PRESET_TEST_SOP_BASE_ONLY, "Test: SoP Base Only" },
        { IDR_PRESET_TEST_SOP_ONLY, "Test: SoP Only" },
        { IDR_PRESET_TEST_TEXTURES_ONLY, "Test: Textures Only" },
        { IDR_PRESET_TEST_UNDERSTEER_ONLY, "Test: Understeer Only" },
        { IDR_PRESET_TEST_YAW_KICK_ONLY, "Test: Yaw Kick Only" },
        { IDR_PRESET_THRUSTMASTER_T_GTT_GT_II, "Thrustmaster T-GT / T-GT II" },
        { IDR_PRESET_THRUSTMASTER_T300TX, "Thrustmaster T300 / TX" },
        { IDR_PRESET_THRUSTMASTER_TS_PCTS_XW, "Thrustmaster TS-PC / TS-XW" }
    };

    for (const auto& [resId, displayName] : resource_presets) {
        auto content = LoadTextResource(resId);
        if (content) {
            try {
                toml::table tbl = toml::parse(*content);
                Preset p("Unnamed", true);
                TomlToPreset(tbl, p);
                if (p.name == "Unnamed") {
                    p.name = displayName;
                }
                p.is_builtin = true;
                p.Validate();
                presets.push_back(p);
            } catch (const std::exception& e) {
                Logger::Get().Log("[Config] Error parsing built-in preset resource %d: %s", resId, e.what());
            }
        }
    }
#else
    // 1. Load Built-in Presets from embedded BUILTIN_PRESETS map (Linux)
    for (auto const& [name, content] : BUILTIN_PRESETS) {
        try {
            toml::table tbl = toml::parse(content);
            Preset p("Unnamed", true);
            TomlToPreset(tbl, p);
            if (p.name == "Unnamed") {
                std::string n(name);
                std::replace(n.begin(), n.end(), '_', ' ');
                p.name = n;
            }
            p.is_builtin = true;
            p.Validate();
            presets.push_back(p);
        } catch (const std::exception& e) {
            Logger::Get().Log("[Config] Error parsing embedded built-in preset %s: %s", std::string(name).c_str(), e.what());
        }
    }
#endif

    // 2. Load User Presets from m_user_presets_path directory
    std::filesystem::create_directories(m_user_presets_path);
    try {
        for (const auto& entry : std::filesystem::directory_iterator(m_user_presets_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".toml") {
                try {
                    toml::table tbl = toml::parse_file(entry.path().string());
                    // The filename is sanitized, but the Preset name should ideally be inside the TOML
                    // or we use the filename if not present. v0.7.218: user presets are individual tables.

                    // UX: The name in the UI should match the filename or an internal 'name' key.
                    // Deliverable 2.2 says: set is_builtin = false and add it.

                    Preset p("Unnamed", false);
                    TomlToPreset(tbl, p);

                    // If TomlToPreset didn't find a name (it doesn't currently look for one in the table root),
                    // use the filename without extension.
                    if (p.name == "Unnamed") {
                        p.name = entry.path().stem().string();
                        // De-sanitize for UI (replace _ with space is risky, but let's see)
                        // Actually, Deliverable 2.4 says SanitizeFilename replaces spaces with underscores.
                    }

                    p.is_builtin = false;
                    p.Validate();
                    presets.push_back(p);
                } catch (const std::exception& e) {
                    Logger::Get().Log("[Config] Error loading user preset %s: %s", entry.path().string().c_str(), e.what());
                }
            }
        }
    } catch (...) {}

    // 3. Deterministic Sorting: Built-ins first, then "Default" first within that group, then alphabetical.
    std::sort(presets.begin(), presets.end(), [](const Preset& a, const Preset& b) {
        if (a.is_builtin != b.is_builtin) return a.is_builtin; // Built-ins first
        if (a.name == "Default") return true;
        if (b.name == "Default") return false;
        return a.name < b.name;
    });
}


void Config::AddUserPreset(const std::string& name, const FFBEngine& engine) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
    bool found = false;
    for (auto& p : presets) {
        if (p.name == name && !p.is_builtin) {
            p.UpdateFromEngine(engine);
            SaveUserPresetFile(p);
            found = true;
            break;
        }
    }
    if (!found) {
        Preset p(name, false);
        p.UpdateFromEngine(engine);
        presets.push_back(p);
        SaveUserPresetFile(p);
    }
    m_last_preset_name = name;
    Save(engine); // Still save main config to persist last_preset_name
}

void Config::DeletePreset(int index, const FFBEngine& engine) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
    if (index < 0 || index >= (int)presets.size()) return;
    if (presets[index].is_builtin) return;

    std::string name = presets[index].name;
    std::string filename = m_user_presets_path + "/" + SanitizeFilename(name) + ".toml";
    if (std::filesystem::exists(filename)) {
        std::filesystem::remove(filename);
    }

    presets.erase(presets.begin() + index);
    if (m_last_preset_name == name) m_last_preset_name = "Default";
    Save(engine);
}

void Config::DuplicatePreset(int index, const FFBEngine& engine) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
    if (index < 0 || index >= (int)presets.size()) return;
    Preset p = presets[index];
    p.name = p.name + " (Copy)";
    p.is_builtin = false;

    // Ensure unique name
    std::string base_name = p.name;
    int counter = 1;
    bool exists = true;
    while (exists) {
        exists = false;
        for (const auto& existing : presets) {
            if (existing.name == p.name) {
                p.name = base_name + " " + std::to_string(counter++);
                exists = true;
                break;
            }
        }
    }

    presets.push_back(p);
    m_last_preset_name = p.name;
    Save(engine);
}

bool Config::IsEngineDirtyRelativeToPreset(int index, const FFBEngine& engine) {
    if (index < 0 || index >= (int)presets.size()) return false;
    Preset current_state;
    current_state.UpdateFromEngine(engine);
    return !presets[index].Equals(current_state);
}

bool Config::GetSavedStaticLoad(const std::string& vehicle_name, double& value) {
    std::lock_guard<std::recursive_mutex> lock(m_static_loads_mutex);
    auto it = m_saved_static_loads.find(vehicle_name);
    if (it != m_saved_static_loads.end()) {
        value = it->second;
        return true;
    }
    return false;
}

void Config::SetSavedStaticLoad(const std::string& vehicle_name, double load) {
    std::lock_guard<std::recursive_mutex> lock(m_static_loads_mutex);
    m_saved_static_loads[vehicle_name] = load;
    m_needs_save = true;
}

void Config::MigrateFromLegacyIni(FFBEngine& engine, const std::string& filename) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
    std::ifstream file(filename);
    if (!file.is_open()) return;

    std::string line;
    bool in_static_loads = false;
    std::string current_preset_name = "";
    Preset current_preset;
    std::string config_version = "";
    bool legacy_torque_hack = false;
    float legacy_torque_val = 100.0f;
    bool preset_pending = false;

    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty() || line[0] == ';') continue;

        if (line[0] == '[') {
            if (preset_pending && !current_preset_name.empty()) {
                FinalizePreset(current_preset, current_preset_name, config_version, legacy_torque_hack, legacy_torque_val);
                preset_pending = false;
                legacy_torque_hack = false;
            }

            if (line == "[StaticLoads]") { in_static_loads = true; current_preset_name = ""; preset_pending = false; }
            else if (line == "[Presets]") { in_static_loads = false; current_preset_name = ""; preset_pending = false; }
            else if (line.rfind("[Preset:", 0) == 0) {
                in_static_loads = false;
                size_t end_pos = line.find(']');
                if (end_pos != std::string::npos) {
                    current_preset_name = line.substr(8, end_pos - 8);
                    // Trim name
                    current_preset_name.erase(0, current_preset_name.find_first_not_of(" \t\r\n"));
                    current_preset_name.erase(current_preset_name.find_last_not_of(" \t\r\n") + 1);

                    current_preset = Preset(current_preset_name, false);
                    preset_pending = true;
                    legacy_torque_hack = false;
                }
            } else {
                in_static_loads = false;
                current_preset_name = "";
                preset_pending = false;
            }
            continue;
        }

        size_t eq_pos = line.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = line.substr(0, eq_pos);
            std::string value = line.substr(eq_pos + 1);

            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);

            if (preset_pending) {
                bool dummy_needs_save = false;
                ParsePresetLine(key + "=" + value, current_preset, config_version, dummy_needs_save, legacy_torque_hack, legacy_torque_val);
            } else if (in_static_loads) {
                try { SetSavedStaticLoad(key, std::stod(value)); } catch (...) {}
            } else {
                if (key == "ini_version") { config_version = value; }
                bool ns = m_needs_save.load();
                if (SyncSystemLine(key, value, engine, config_version, legacy_torque_hack, legacy_torque_val, ns)) {
                    m_needs_save.store(ns);
                } else if (SyncPhysicsLine(key, value, engine, config_version, ns)) {
                    m_needs_save.store(ns);
                } else if (SyncBrakingLine(key, value, engine)) {}
                else if (SyncVibrationLine(key, value, engine)) {}
                else if (SyncSafetyLine(key, value, engine)) {}
            }
        }
    }
    if (preset_pending && !current_preset_name.empty()) {
        FinalizePreset(current_preset, current_preset_name, config_version, legacy_torque_hack, legacy_torque_val);
    }

    // Apply final torque hack for global gain if it was pending
    if (legacy_torque_hack && IsVersionLessEqual(config_version, "0.7.66")) {
        engine.m_general.gain *= (15.0f / legacy_torque_val);
    }
}

void Config::ExportPreset(int index, const std::string& filename) {
    if (index < 0 || index >= (int)presets.size()) return;

    if (presets[index].is_builtin) {
        // For built-ins, we serialize from memory
        std::ofstream file(filename);
        if (file.is_open()) {
            file << PresetToToml(presets[index]);
        }
    } else {
        // For user presets, just copy the file
        std::string source = "user_presets/" + SanitizeFilename(presets[index].name) + ".toml";
        if (std::filesystem::exists(source)) {
            try {
                std::filesystem::copy_file(source, filename, std::filesystem::copy_options::overwrite_existing);
            } catch (...) {
                // Fallback to serialization if copy fails
                std::ofstream file(filename);
                if (file.is_open()) {
                    file << PresetToToml(presets[index]);
                }
            }
        } else {
            // Should not happen if out-of-sync, but fallback to serialization
            std::ofstream file(filename);
            if (file.is_open()) {
                file << PresetToToml(presets[index]);
            }
        }
    }
}

bool Config::ImportPreset(const std::string& filename, const FFBEngine& engine) {
    std::string ext = std::filesystem::path(filename).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".toml") {
        try {
            toml::table tbl = toml::parse_file(filename);
            Preset p("Imported", false);
            TomlToPreset(tbl, p);
            // If the TOML was exported with the old "Preset" wrapper (from Phase 2), handle it
            if (tbl.contains("Preset")) {
                if (auto p_tbl = tbl["Preset"].as_table()) {
                    TomlToPreset(*p_tbl, p);
                }
            }
            if (p.name == "Imported" || p.name == "Unnamed") {
                p.name = std::filesystem::path(filename).stem().string();
            }
            p.Validate();
            SaveUserPresetFile(p);
            presets.push_back(p);
            return true;
        } catch (...) {}
    }

    // Fallback to legacy INI import
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    std::string line;
    std::string current_preset_name = "";
    Preset current_preset;
    std::string current_preset_version = "";
    bool preset_pending = false;
    bool imported = false;
    bool legacy_torque_hack = false;
    float legacy_torque_val = 100.0f;

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
                    preset_pending = true;
                    current_preset_version = "";
                    legacy_torque_hack = false;
                    legacy_torque_val = 100.0f;
                }
            }
            continue;
        }
        if (preset_pending) {
            bool dummy_needs_save = false;
            ParsePresetLine(line, current_preset, current_preset_version, dummy_needs_save, legacy_torque_hack, legacy_torque_val);
        }
    }
    if (preset_pending && !current_preset_name.empty()) {
        current_preset.name = current_preset_name;
        if (legacy_torque_hack && IsVersionLessEqual(current_preset_version, "0.7.66")) {
            current_preset.general.gain *= (15.0f / legacy_torque_val);
        }
        current_preset.Validate();
        SaveUserPresetFile(current_preset);
        presets.push_back(current_preset);
        imported = true;
    }
    return imported;
}

void Config::ApplyPreset(int index, FFBEngine& engine) {
    if (index >= 0 && index < (int)presets.size()) {
        std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
        presets[index].Apply(engine);
        m_last_preset_name = presets[index].name;
        Logger::Get().LogFile("[Config] Applied preset: %s", presets[index].name.c_str());
        Save(engine);
    }
}

} // namespace LMUFFB
