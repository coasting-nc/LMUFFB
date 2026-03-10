#include "Config.h"
#include "Version.h"
#include "Logger.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <mutex>

extern std::recursive_mutex g_engine_mutex;

bool Config::m_always_on_top = true;
std::string Config::m_last_device_guid = "";
std::string Config::m_last_preset_name = "Default";
std::string Config::m_config_path = "config.ini";
bool Config::m_auto_start_logging = false;
std::string Config::m_log_path = "logs/";

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

// Helper to compare semantic version strings (e.g., "0.7.66" <= "0.7.66")
static bool IsVersionLessEqual(const std::string& v1, const std::string& v2) {
    if (v1.empty()) return true; // Empty version treated as legacy
    if (v2.empty()) return false;

    std::stringstream ss1(v1), ss2(v2);
    std::string segment1, segment2;

    while (true) {
        bool has1 = (bool)std::getline(ss1, segment1, '.');
        bool has2 = (bool)std::getline(ss2, segment2, '.');

        if (!has1 && !has2) return true; // Exactly equal

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

bool Config::ParseSystemLine(const std::string& key, const std::string& value, Preset& current_preset, std::string& current_preset_version, bool& needs_save, bool& legacy_torque_hack, float& legacy_torque_val) {
    if (key == "app_version") { current_preset_version = value; return true; }
    if (key == "gain") { current_preset.gain = std::stof(value); return true; }
    if (key == "wheelbase_max_nm") { current_preset.wheelbase_max_nm = std::stof(value); return true; }
    if (key == "target_rim_nm") { current_preset.target_rim_nm = std::stof(value); return true; }
    if (key == "min_force") { current_preset.min_force = std::stof(value); return true; }
    if (key == "max_torque_ref") {
        float old_val = std::stof(value);
        if (old_val > 40.0f) {
            current_preset.wheelbase_max_nm = 15.0f;
            current_preset.target_rim_nm = 10.0f;
            legacy_torque_hack = true;
            legacy_torque_val = old_val;
        } else {
            current_preset.wheelbase_max_nm = old_val;
            current_preset.target_rim_nm = old_val;
        }
        needs_save = true;
        return true;
    }
    if (key == "rest_api_fallback_enabled") { current_preset.rest_api_enabled = (value == "1" || value == "true"); return true; }
    if (key == "rest_api_port") { current_preset.rest_api_port = std::stoi(value); return true; }
    return false;
}

bool Config::ParsePhysicsLine(const std::string& key, const std::string& value, Preset& current_preset) {
    if (key == "understeer") {
        float val = std::stof(value);
        if (val > 2.0f) val /= 100.0f;
        current_preset.understeer = (std::min)(2.0f, (std::max)(0.0f, val));
        return true;
    }
    if (key == "steering_shaft_gain") { current_preset.steering_shaft_gain = std::stof(value); return true; }
    if (key == "ingame_ffb_gain") { current_preset.ingame_ffb_gain = std::stof(value); return true; }
    if (key == "steering_shaft_smoothing") { current_preset.steering_shaft_smoothing = std::stof(value); return true; }
    if (key == "torque_source") { current_preset.torque_source = std::stoi(value); return true; }
    if (key == "torque_passthrough") { current_preset.torque_passthrough = (value == "1" || value == "true"); return true; }
    if (key == "sop") { current_preset.sop = std::stof(value); return true; }
    if (key == "lateral_load_effect") { current_preset.lateral_load = std::stof(value); return true; }
    if (key == "lat_load_transform") { current_preset.lat_load_transform = std::clamp(std::stoi(value), 0, 3); return true; }
    if (key == "sop_scale") { current_preset.sop_scale = std::stof(value); return true; }
    if (key == "sop_smoothing_factor") { current_preset.sop_smoothing = std::stof(value); return true; }
    if (key == "oversteer_boost") { current_preset.oversteer_boost = std::stof(value); return true; }
    if (key == "long_load_effect" || key == "dynamic_weight_gain") { current_preset.long_load_effect = std::stof(value); return true; }
    if (key == "long_load_smoothing" || key == "dynamic_weight_smoothing") { current_preset.long_load_smoothing = std::stof(value); return true; }
    if (key == "long_load_transform") { current_preset.long_load_transform = std::clamp(std::stoi(value), 0, 3); return true; }
    if (key == "grip_smoothing_steady") { current_preset.grip_smoothing_steady = std::stof(value); return true; }
    if (key == "grip_smoothing_fast") { current_preset.grip_smoothing_fast = std::stof(value); return true; }
    if (key == "grip_smoothing_sensitivity") { current_preset.grip_smoothing_sensitivity = std::stof(value); return true; }
    if (key == "rear_align_effect") { current_preset.rear_align_effect = std::stof(value); return true; }
    if (key == "sop_yaw_gain") { current_preset.sop_yaw_gain = std::stof(value); return true; }
    if (key == "yaw_kick_threshold") { current_preset.yaw_kick_threshold = std::stof(value); return true; }
    if (key == "unloaded_yaw_gain") { current_preset.unloaded_yaw_gain = std::stof(value); return true; }
    if (key == "unloaded_yaw_threshold") { current_preset.unloaded_yaw_threshold = std::stof(value); return true; }
    if (key == "unloaded_yaw_sens") { current_preset.unloaded_yaw_sens = std::stof(value); return true; }
    if (key == "unloaded_yaw_gamma") { current_preset.unloaded_yaw_gamma = std::stof(value); return true; }
    if (key == "unloaded_yaw_punch") { current_preset.unloaded_yaw_punch = std::stof(value); return true; }
    if (key == "power_yaw_gain") { current_preset.power_yaw_gain = std::stof(value); return true; }
    if (key == "power_yaw_threshold") { current_preset.power_yaw_threshold = std::stof(value); return true; }
    if (key == "power_slip_threshold") { current_preset.power_slip_threshold = std::stof(value); return true; }
    if (key == "power_yaw_gamma") { current_preset.power_yaw_gamma = std::stof(value); return true; }
    if (key == "power_yaw_punch") { current_preset.power_yaw_punch = std::stof(value); return true; }
    if (key == "yaw_accel_smoothing") { current_preset.yaw_smoothing = std::stof(value); return true; }
    if (key == "gyro_gain") { current_preset.gyro_gain = (std::min)(1.0f, std::stof(value)); return true; }
    if (key == "gyro_smoothing_factor") { current_preset.gyro_smoothing = std::stof(value); return true; }
    if (key == "optimal_slip_angle") { current_preset.optimal_slip_angle = std::stof(value); return true; }
    if (key == "optimal_slip_ratio") { current_preset.optimal_slip_ratio = std::stof(value); return true; }
    if (key == "slope_detection_enabled") { current_preset.slope_detection_enabled = (value == "1"); return true; }
    if (key == "slope_sg_window") { current_preset.slope_sg_window = std::stoi(value); return true; }
    if (key == "slope_sensitivity") { current_preset.slope_sensitivity = std::stof(value); return true; }
    if (key == "slope_negative_threshold" || key == "slope_min_threshold") { current_preset.slope_min_threshold = std::stof(value); return true; }
    if (key == "slope_smoothing_tau") { current_preset.slope_smoothing_tau = std::stof(value); return true; }
    if (key == "slope_max_threshold") { current_preset.slope_max_threshold = std::stof(value); return true; }
    if (key == "slope_alpha_threshold") { current_preset.slope_alpha_threshold = std::stof(value); return true; }
    if (key == "slope_decay_rate") { current_preset.slope_decay_rate = std::stof(value); return true; }
    if (key == "slope_confidence_enabled") { current_preset.slope_confidence_enabled = (value == "1"); return true; }
    if (key == "slope_g_slew_limit") { current_preset.slope_g_slew_limit = std::stof(value); return true; }
    if (key == "slope_use_torque") { current_preset.slope_use_torque = (value == "1"); return true; }
    if (key == "slope_torque_sensitivity") { current_preset.slope_torque_sensitivity = std::stof(value); return true; }
    if (key == "slope_confidence_max_rate") { current_preset.slope_confidence_max_rate = std::stof(value); return true; }
    if (key == "slip_angle_smoothing") { current_preset.slip_smoothing = std::stof(value); return true; }
    if (key == "chassis_inertia_smoothing") { current_preset.chassis_smoothing = std::stof(value); return true; }
    return false;
}

bool Config::ParseBrakingLine(const std::string& key, const std::string& value, Preset& current_preset) {
    if (key == "lockup_enabled") { current_preset.lockup_enabled = (value == "1" || value == "true"); return true; }
    if (key == "lockup_gain") { current_preset.lockup_gain = std::stof(value); return true; }
    if (key == "lockup_start_pct") { current_preset.lockup_start_pct = std::stof(value); return true; }
    if (key == "lockup_full_pct") { current_preset.lockup_full_pct = std::stof(value); return true; }
    if (key == "lockup_rear_boost") { current_preset.lockup_rear_boost = std::stof(value); return true; }
    if (key == "lockup_gamma") { current_preset.lockup_gamma = std::stof(value); return true; }
    if (key == "lockup_prediction_sens") { current_preset.lockup_prediction_sens = std::stof(value); return true; }
    if (key == "lockup_bump_reject") { current_preset.lockup_bump_reject = std::stof(value); return true; }
    if (key == "brake_load_cap") { current_preset.brake_load_cap = (std::min)(10.0f, std::stof(value)); return true; }
    if (key == "abs_pulse_enabled") { current_preset.abs_pulse_enabled = std::stoi(value); return true; }
    if (key == "abs_gain") { current_preset.abs_gain = std::stof(value); return true; }
    if (key == "abs_freq") { current_preset.abs_freq = std::stof(value); return true; }
    if (key == "lockup_freq_scale") { current_preset.lockup_freq_scale = std::stof(value); return true; }
    return false;
}

bool Config::ParseVibrationLine(const std::string& key, const std::string& value, Preset& current_preset) {
    if (key == "texture_load_cap" || key == "max_load_factor") { current_preset.texture_load_cap = std::stof(value); return true; }
    if (key == "spin_enabled") { current_preset.spin_enabled = (value == "1" || value == "true"); return true; }
    if (key == "spin_gain") { current_preset.spin_gain = std::stof(value); return true; }
    if (key == "spin_freq_scale") { current_preset.spin_freq_scale = std::stof(value); return true; }
    if (key == "slide_enabled") { current_preset.slide_enabled = (value == "1" || value == "true"); return true; }
    if (key == "slide_gain") { current_preset.slide_gain = std::stof(value); return true; }
    if (key == "slide_freq") { current_preset.slide_freq = std::stof(value); return true; }
    if (key == "road_enabled") { current_preset.road_enabled = (value == "1" || value == "true"); return true; }
    if (key == "road_gain") { current_preset.road_gain = std::stof(value); return true; }
    if (key == "vibration_gain" || key == "tactile_gain") { current_preset.vibration_gain = std::stof(value); return true; }
    if (key == "scrub_drag_gain") { current_preset.scrub_drag_gain = std::stof(value); return true; }
    if (key == "bottoming_method") { current_preset.bottoming_method = std::stoi(value); return true; }
    if (key == "dynamic_normalization_enabled") { current_preset.dynamic_normalization_enabled = (value == "1" || value == "true"); return true; }
    if (key == "auto_load_normalization_enabled") { current_preset.auto_load_normalization_enabled = (value == "1" || value == "true"); return true; }
    if (key == "soft_lock_enabled") { current_preset.soft_lock_enabled = (value == "1" || value == "true"); return true; }
    if (key == "soft_lock_stiffness") { current_preset.soft_lock_stiffness = std::stof(value); return true; }
    if (key == "soft_lock_damping") { current_preset.soft_lock_damping = std::stof(value); return true; }
    if (key == "flatspot_suppression") { current_preset.flatspot_suppression = (value == "1" || value == "true"); return true; }
    if (key == "notch_q") { current_preset.notch_q = std::stof(value); return true; }
    if (key == "flatspot_strength") { current_preset.flatspot_strength = std::stof(value); return true; }
    if (key == "static_notch_enabled") { current_preset.static_notch_enabled = (value == "1" || value == "true"); return true; }
    if (key == "static_notch_freq") { current_preset.static_notch_freq = std::stof(value); return true; }
    if (key == "static_notch_width") { current_preset.static_notch_width = std::stof(value); return true; }
    if (key == "speed_gate_lower") { current_preset.speed_gate_lower = std::stof(value); return true; }
    if (key == "speed_gate_upper") { current_preset.speed_gate_upper = std::stof(value); return true; }
    if (key == "road_fallback_scale") { current_preset.road_fallback_scale = std::stof(value); return true; }
    if (key == "understeer_affects_sop") { current_preset.understeer_affects_sop = std::stoi(value); return true; }
    return false;
}

bool Config::ParseSafetyLine(const std::string& key, const std::string& value, Preset& current_preset) {
    if (key == "safety_window_duration") { current_preset.safety_window_duration = std::stof(value); return true; }
    if (key == "safety_gain_reduction") { current_preset.safety_gain_reduction = std::stof(value); return true; }
    if (key == "safety_smoothing_tau") { current_preset.safety_smoothing_tau = std::stof(value); return true; }
    if (key == "spike_detection_threshold") { current_preset.spike_detection_threshold = std::stof(value); return true; }
    if (key == "immediate_spike_threshold") { current_preset.immediate_spike_threshold = std::stof(value); return true; }
    if (key == "safety_slew_full_scale_time_s") { current_preset.safety_slew_full_scale_time_s = std::stof(value); return true; }
    if (key == "stutter_safety_enabled") { current_preset.stutter_safety_enabled = (value == "1" || value == "true"); return true; }
    if (key == "stutter_threshold") { current_preset.stutter_threshold = std::stof(value); return true; }
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
    if (key == "invert_force") { engine.m_invert_force = std::stoi(value); return true; }
    if (key == "gain") { engine.m_gain = std::stof(value); return true; }
    if (key == "wheelbase_max_nm") { engine.m_wheelbase_max_nm = std::stof(value); return true; }
    if (key == "target_rim_nm") { engine.m_target_rim_nm = std::stof(value); return true; }
    if (key == "min_force") { engine.m_min_force = std::stof(value); return true; }
    if (key == "max_torque_ref") {
        float old_val = std::stof(value);
        if (old_val > 40.0f) {
            engine.m_wheelbase_max_nm = 15.0f;
            engine.m_target_rim_nm = 10.0f;
            legacy_torque_hack = true;
            legacy_torque_val = old_val;
        } else {
            engine.m_wheelbase_max_nm = old_val;
            engine.m_target_rim_nm = old_val;
        }
        needs_save = true;
        return true;
    }
    if (key == "rest_api_fallback_enabled") { engine.m_rest_api_enabled = (value == "1" || value == "true"); return true; }
    if (key == "rest_api_port") { engine.m_rest_api_port = std::stoi(value); return true; }
    return false;
}

bool Config::SyncPhysicsLine(const std::string& key, const std::string& value, FFBEngine& engine, std::string& config_version, bool& needs_save) {
    if (key == "understeer") {
        float val = std::stof(value);
        if (val > 2.0f) val /= 100.0f;
        engine.m_understeer_effect = (std::min)(2.0f, (std::max)(0.0f, val));
        return true;
    }
    if (key == "steering_shaft_gain") { engine.m_steering_shaft_gain = std::stof(value); return true; }
    if (key == "ingame_ffb_gain") { engine.m_ingame_ffb_gain = std::stof(value); return true; }
    if (key == "steering_shaft_smoothing") { engine.m_steering_shaft_smoothing = std::stof(value); return true; }
    if (key == "torque_source") { engine.m_torque_source = std::stoi(value); return true; }
    if (key == "torque_passthrough") { engine.m_torque_passthrough = (value == "1" || value == "true"); return true; }
    if (key == "sop") { engine.m_sop_effect = std::stof(value); return true; }
    if (key == "lateral_load_effect") { engine.m_lat_load_effect = std::stof(value); return true; }
    if (key == "lat_load_transform") { engine.m_lat_load_transform = static_cast<LoadTransform>(std::clamp(std::stoi(value), 0, 3)); return true; }
    if (key == "sop_scale") { engine.m_sop_scale = std::stof(value); return true; }
    if (key == "sop_smoothing_factor" || key == "smoothing") {
        float val = std::stof(value);
        if (IsVersionLessEqual(config_version, "0.7.146")) {
            val = 0.0f;
            needs_save = true;
        }
        engine.m_sop_smoothing_factor = val;
        return true;
    }
    if (key == "oversteer_boost") { engine.m_oversteer_boost = std::stof(value); return true; }
    if (key == "long_load_effect" || key == "dynamic_weight_gain") { engine.m_long_load_effect = std::stof(value); return true; }
    if (key == "long_load_smoothing" || key == "dynamic_weight_smoothing") { engine.m_long_load_smoothing = std::stof(value); return true; }
    if (key == "long_load_transform") { engine.m_long_load_transform = static_cast<LoadTransform>(std::clamp(std::stoi(value), 0, 3)); return true; }
    if (key == "grip_smoothing_steady") { engine.m_grip_smoothing_steady = std::stof(value); return true; }
    if (key == "grip_smoothing_fast") { engine.m_grip_smoothing_fast = std::stof(value); return true; }
    if (key == "grip_smoothing_sensitivity") { engine.m_grip_smoothing_sensitivity = std::stof(value); return true; }
    if (key == "rear_align_effect") { engine.m_rear_align_effect = std::stof(value); return true; }
    if (key == "sop_yaw_gain") { engine.m_sop_yaw_gain = std::stof(value); return true; }
    if (key == "yaw_kick_threshold") { engine.m_yaw_kick_threshold = std::stof(value); return true; }
    if (key == "unloaded_yaw_gain") { engine.m_unloaded_yaw_gain = std::stof(value); return true; }
    if (key == "unloaded_yaw_threshold") { engine.m_unloaded_yaw_threshold = std::stof(value); return true; }
    if (key == "unloaded_yaw_sens") { engine.m_unloaded_yaw_sens = std::stof(value); return true; }
    if (key == "unloaded_yaw_gamma") { engine.m_unloaded_yaw_gamma = std::stof(value); return true; }
    if (key == "unloaded_yaw_punch") { engine.m_unloaded_yaw_punch = std::stof(value); return true; }
    if (key == "power_yaw_gain") { engine.m_power_yaw_gain = std::stof(value); return true; }
    if (key == "power_yaw_threshold") { engine.m_power_yaw_threshold = std::stof(value); return true; }
    if (key == "power_slip_threshold") { engine.m_power_slip_threshold = std::stof(value); return true; }
    if (key == "power_yaw_gamma") { engine.m_power_yaw_gamma = std::stof(value); return true; }
    if (key == "power_yaw_punch") { engine.m_power_yaw_punch = std::stof(value); return true; }
    if (key == "yaw_accel_smoothing") { engine.m_yaw_accel_smoothing = std::stof(value); return true; }
    if (key == "gyro_gain") { engine.m_gyro_gain = (std::min)(1.0f, std::stof(value)); return true; }
    if (key == "gyro_smoothing_factor") { engine.m_gyro_smoothing = std::stof(value); return true; }
    if (key == "optimal_slip_angle") { engine.m_optimal_slip_angle = std::stof(value); return true; }
    if (key == "optimal_slip_ratio") { engine.m_optimal_slip_ratio = std::stof(value); return true; }
    if (key == "slope_detection_enabled") { engine.m_slope_detection_enabled = (value == "1"); return true; }
    if (key == "slope_sg_window") { engine.m_slope_sg_window = std::stoi(value); return true; }
    if (key == "slope_sensitivity") { engine.m_slope_sensitivity = std::stof(value); return true; }
    if (key == "slope_negative_threshold" || key == "slope_min_threshold") { engine.m_slope_min_threshold = std::stof(value); return true; }
    if (key == "slope_smoothing_tau") { engine.m_slope_smoothing_tau = std::stof(value); return true; }
    if (key == "slope_max_threshold") { engine.m_slope_max_threshold = std::stof(value); return true; }
    if (key == "slope_alpha_threshold") { engine.m_slope_alpha_threshold = std::stof(value); return true; }
    if (key == "slope_decay_rate") { engine.m_slope_decay_rate = std::stof(value); return true; }
    if (key == "slope_confidence_enabled") { engine.m_slope_confidence_enabled = (value == "1"); return true; }
    if (key == "slope_g_slew_limit") { engine.m_slope_g_slew_limit = std::stof(value); return true; }
    if (key == "slope_use_torque") { engine.m_slope_use_torque = (value == "1"); return true; }
    if (key == "slope_torque_sensitivity") { engine.m_slope_torque_sensitivity = std::stof(value); return true; }
    if (key == "slope_confidence_max_rate") { engine.m_slope_confidence_max_rate = std::stof(value); return true; }
    if (key == "slip_angle_smoothing") { engine.m_slip_angle_smoothing = std::stof(value); return true; }
    if (key == "chassis_inertia_smoothing") { engine.m_chassis_inertia_smoothing = std::stof(value); return true; }
    if (key == "speed_gate_lower") { engine.m_speed_gate_lower = std::stof(value); return true; }
    if (key == "speed_gate_upper") { engine.m_speed_gate_upper = std::stof(value); return true; }
    if (key == "road_fallback_scale") { engine.m_road_fallback_scale = std::stof(value); return true; }
    if (key == "understeer_affects_sop") { engine.m_understeer_affects_sop = std::stoi(value); return true; }
    return false;
}

bool Config::SyncBrakingLine(const std::string& key, const std::string& value, FFBEngine& engine) {
    if (key == "lockup_enabled") { engine.m_lockup_enabled = (value == "1" || value == "true"); return true; }
    if (key == "lockup_gain") { engine.m_lockup_gain = std::stof(value); return true; }
    if (key == "lockup_start_pct") { engine.m_lockup_start_pct = std::stof(value); return true; }
    if (key == "lockup_full_pct") { engine.m_lockup_full_pct = std::stof(value); return true; }
    if (key == "lockup_rear_boost") { engine.m_lockup_rear_boost = std::stof(value); return true; }
    if (key == "lockup_gamma") { engine.m_lockup_gamma = std::stof(value); return true; }
    if (key == "lockup_prediction_sens") { engine.m_lockup_prediction_sens = std::stof(value); return true; }
    if (key == "lockup_bump_reject") { engine.m_lockup_bump_reject = std::stof(value); return true; }
    if (key == "brake_load_cap") { engine.m_brake_load_cap = std::stof(value); return true; }
    if (key == "abs_pulse_enabled") { engine.m_abs_pulse_enabled = (value == "1" || value == "true"); return true; }
    if (key == "abs_gain") { engine.m_abs_gain = std::stof(value); return true; }
    if (key == "abs_freq") { engine.m_abs_freq_hz = std::stof(value); return true; }
    if (key == "lockup_freq_scale") { engine.m_lockup_freq_scale = std::stof(value); return true; }
    return false;
}

bool Config::SyncVibrationLine(const std::string& key, const std::string& value, FFBEngine& engine) {
    if (key == "texture_load_cap" || key == "max_load_factor") { engine.m_texture_load_cap = std::stof(value); return true; }
    if (key == "spin_enabled") { engine.m_spin_enabled = (value == "1" || value == "true"); return true; }
    if (key == "spin_gain") { engine.m_spin_gain = std::stof(value); return true; }
    if (key == "spin_freq_scale") { engine.m_spin_freq_scale = std::stof(value); return true; }
    if (key == "slide_enabled") { engine.m_slide_texture_enabled = (value == "1" || value == "true"); return true; }
    if (key == "slide_gain") { engine.m_slide_texture_gain = std::stof(value); return true; }
    if (key == "slide_freq") { engine.m_slide_freq_scale = std::stof(value); return true; }
    if (key == "road_enabled") { engine.m_road_texture_enabled = (value == "1" || value == "true"); return true; }
    if (key == "road_gain") { engine.m_road_texture_gain = std::stof(value); return true; }
    if (key == "vibration_gain" || key == "tactile_gain") { engine.m_vibration_gain = std::stof(value); return true; }
    if (key == "dynamic_normalization_enabled") { engine.m_dynamic_normalization_enabled = (value == "1" || value == "true"); return true; }
    if (key == "auto_load_normalization_enabled") { engine.m_auto_load_normalization_enabled = (value == "1" || value == "true"); return true; }
    if (key == "soft_lock_enabled") { engine.m_soft_lock_enabled = (value == "1" || value == "true"); return true; }
    if (key == "soft_lock_stiffness") { engine.m_soft_lock_stiffness = std::stof(value); return true; }
    if (key == "soft_lock_damping") { engine.m_soft_lock_damping = std::stof(value); return true; }
    if (key == "flatspot_suppression") { engine.m_flatspot_suppression = (value == "1" || value == "true"); return true; }
    if (key == "notch_q") { engine.m_notch_q = std::stof(value); return true; }
    if (key == "flatspot_strength") { engine.m_flatspot_strength = std::stof(value); return true; }
    if (key == "static_notch_enabled") { engine.m_static_notch_enabled = (value == "1" || value == "true"); return true; }
    if (key == "static_notch_freq") { engine.m_static_notch_freq = std::stof(value); return true; }
    if (key == "static_notch_width") { engine.m_static_notch_width = std::stof(value); return true; }
    if (key == "scrub_drag_gain") { engine.m_scrub_drag_gain = std::stof(value); return true; }
    if (key == "bottoming_method") { engine.m_bottoming_method = std::stoi(value); return true; }
    return false;
}

bool Config::SyncSafetyLine(const std::string& key, const std::string& value, FFBEngine& engine) {
    if (key == "safety_window_duration") { engine.m_safety_window_duration = std::stof(value); return true; }
    if (key == "safety_gain_reduction") { engine.m_safety_gain_reduction = std::stof(value); return true; }
    if (key == "safety_smoothing_tau") { engine.m_safety_smoothing_tau = std::stof(value); return true; }
    if (key == "spike_detection_threshold") { engine.m_spike_detection_threshold = std::stof(value); return true; }
    if (key == "immediate_spike_threshold") { engine.m_immediate_spike_threshold = std::stof(value); return true; }
    if (key == "safety_slew_full_scale_time_s") { engine.m_safety_slew_full_scale_time_s = std::stof(value); return true; }
    if (key == "stutter_safety_enabled") { engine.m_stutter_safety_enabled = (value == "1" || value == "true"); return true; }
    if (key == "stutter_threshold") { engine.m_stutter_threshold = std::stof(value); return true; }
    return false;
}

void Config::ParsePresetLine(const std::string& line, Preset& current_preset, std::string& current_preset_version, bool& needs_save, bool& legacy_torque_hack, float& legacy_torque_val) {
    std::istringstream is_line(line);
    std::string key;
    if (std::getline(is_line, key, '=')) {
        std::string value;
        if (std::getline(is_line, value)) {
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

void Config::LoadPresets() {
    presets.clear();
    
    // 1. Default - Uses Preset struct defaults from Config.h (Single Source of Truth)
    presets.push_back(Preset("Default", true));
    
    // 2. T300 (Custom optimized)
    {
        Preset p("T300", true);
        p.gain = 1.0f;
        p.wheelbase_max_nm = 4.0f;
        p.target_rim_nm = 4.0f;
        p.min_force = 0.01f;
        p.steering_shaft_gain = 1.0f;
        p.steering_shaft_smoothing = 0.0f;
        p.understeer = 0.5f;
        p.flatspot_suppression = false;
        p.notch_q = 2.0f;
        p.flatspot_strength = 1.0f;
        p.static_notch_enabled = false;
        p.static_notch_freq = 11.0f;
        p.static_notch_width = 2.0f;
        p.oversteer_boost = 2.40336f;
        p.sop = 0.425003f;
        p.rear_align_effect = 0.966383f;
        p.sop_yaw_gain = 0.386555f;
        p.yaw_kick_threshold = 1.68f;
        p.yaw_smoothing = 0.005f;
        p.gyro_gain = 0.0336134f;
        p.gyro_smoothing = 0.0f;
        p.sop_smoothing = 0.0f;
        p.sop_scale = 1.0f;
        p.understeer_affects_sop = false;
        p.slip_smoothing = 0.0f;
        p.chassis_smoothing = 0.0f;
        p.optimal_slip_angle = 0.10f;   // CHANGED from 0.06f
        p.optimal_slip_ratio = 0.12f;
        p.lockup_enabled = true;
        p.lockup_gain = 2.0f;
        p.brake_load_cap = 10.0f;
        p.lockup_freq_scale = 1.02f;
        p.lockup_gamma = 0.1f;
        p.lockup_start_pct = 1.0f;
        p.lockup_full_pct = 5.0f;
        p.lockup_prediction_sens = 10.0f;
        p.lockup_bump_reject = 0.1f;
        p.lockup_rear_boost = 10.0f;
        p.abs_pulse_enabled = true;
        p.abs_gain = 2.0f;
        p.abs_freq = 20.0f;
        p.texture_load_cap = 1.96f;
        p.slide_enabled = true;
        p.slide_gain = 0.235294f;
        p.slide_freq = 1.0f;
        p.road_enabled = true;
        p.road_gain = 2.0f;
        p.road_fallback_scale = 0.05f;
        p.spin_enabled = true;
        p.spin_gain = 0.5f;
        p.spin_freq_scale = 1.0f;
        p.scrub_drag_gain = 0.0462185f;
        p.bottoming_method = 0;
        p.speed_gate_lower = 0.0f;
        p.speed_gate_upper = 0.277778f;
        presets.push_back(p);
    }
    
    // 3. GT3 DD 15 Nm (Simagic Alpha)
    {
        Preset p("GT3 DD 15 Nm (Simagic Alpha)", true);
        p.gain = 1.0f;
        p.wheelbase_max_nm = 15.0f;
        p.target_rim_nm = 10.0f;
        p.min_force = 0.0f;
        p.steering_shaft_gain = 1.0f;
        p.steering_shaft_smoothing = 0.0f;
        p.understeer = 1.0f;
        p.flatspot_suppression = false;
        p.notch_q = 2.0f;
        p.flatspot_strength = 1.0f;
        p.static_notch_enabled = false;
        p.static_notch_freq = 11.0f;
        p.static_notch_width = 2.0f;
        p.oversteer_boost = 2.52101f;
        p.sop = 1.666f;
        p.rear_align_effect = 0.666f;
        p.sop_yaw_gain = 0.333f;
        p.yaw_kick_threshold = 0.0f;
        p.yaw_smoothing = 0.001f;
        p.gyro_gain = 0.0f;
        p.gyro_smoothing = 0.0f;
        p.sop_smoothing = 0.0f;
        p.sop_scale = 1.98f;
        p.understeer_affects_sop = false;
        p.slip_smoothing = 0.002f;
        p.chassis_smoothing = 0.012f;
        p.optimal_slip_angle = 0.1f;
        p.optimal_slip_ratio = 0.12f;
        p.lockup_enabled = true;
        p.lockup_gain = 0.37479f;
        p.brake_load_cap = 2.0f;
        p.lockup_freq_scale = 1.0f;
        p.lockup_gamma = 1.0f;
        p.lockup_start_pct = 1.0f;
        p.lockup_full_pct = 7.5f;
        p.lockup_prediction_sens = 10.0f;
        p.lockup_bump_reject = 0.1f;
        p.lockup_rear_boost = 1.0f;
        p.abs_pulse_enabled = false;
        p.abs_gain = 2.1f;
        p.abs_freq = 25.5f;
        p.texture_load_cap = 1.5f;
        p.slide_enabled = false;
        p.slide_gain = 0.226562f;
        p.slide_freq = 1.47f;
        p.road_enabled = true;
        p.road_gain = 0.0f;
        p.road_fallback_scale = 0.05f;
        p.spin_enabled = true;
        p.spin_gain = 0.462185f;
        p.spin_freq_scale = 1.8f;
        p.scrub_drag_gain = 0.333f;
        p.bottoming_method = 1;
        p.speed_gate_lower = 1.0f;
        p.speed_gate_upper = 5.0f;
        presets.push_back(p);
    }
    
    // 4. LMPx/HY DD 15 Nm (Simagic Alpha)
    {
        Preset p("LMPx/HY DD 15 Nm (Simagic Alpha)", true);
        p.gain = 1.0f;
        p.wheelbase_max_nm = 15.0f;
        p.target_rim_nm = 10.0f;
        p.min_force = 0.0f;
        p.steering_shaft_gain = 1.0f;
        p.steering_shaft_smoothing = 0.0f;
        p.understeer = 1.0f;
        p.flatspot_suppression = false;
        p.notch_q = 2.0f;
        p.flatspot_strength = 1.0f;
        p.static_notch_enabled = false;
        p.static_notch_freq = 11.0f;
        p.static_notch_width = 2.0f;
        p.oversteer_boost = 2.52101f;
        p.sop = 1.666f;
        p.rear_align_effect = 0.666f;
        p.sop_yaw_gain = 0.333f;
        p.yaw_kick_threshold = 0.0f;
        p.yaw_smoothing = 0.003f;
        p.gyro_gain = 0.0f;
        p.gyro_smoothing = 0.003f;
        p.sop_smoothing = 0.0f;
        p.sop_scale = 1.59f;
        p.understeer_affects_sop = false;
        p.slip_smoothing = 0.003f;
        p.chassis_smoothing = 0.019f;
        p.optimal_slip_angle = 0.12f;
        p.optimal_slip_ratio = 0.12f;
        p.lockup_enabled = true;
        p.lockup_gain = 0.37479f;
        p.brake_load_cap = 2.0f;
        p.lockup_freq_scale = 1.0f;
        p.lockup_gamma = 1.0f;
        p.lockup_start_pct = 1.0f;
        p.lockup_full_pct = 7.5f;
        p.lockup_prediction_sens = 10.0f;
        p.lockup_bump_reject = 0.1f;
        p.lockup_rear_boost = 1.0f;
        p.abs_pulse_enabled = false;
        p.abs_gain = 2.1f;
        p.abs_freq = 25.5f;
        p.texture_load_cap = 1.5f;
        p.slide_enabled = false;
        p.slide_gain = 0.226562f;
        p.slide_freq = 1.47f;
        p.road_enabled = true;
        p.road_gain = 0.0f;
        p.road_fallback_scale = 0.05f;
        p.spin_enabled = true;
        p.spin_gain = 0.462185f;
        p.spin_freq_scale = 1.8f;
        p.scrub_drag_gain = 0.333f;
        p.bottoming_method = 1;
        p.speed_gate_lower = 1.0f;
        p.speed_gate_upper = 5.0f;
        presets.push_back(p);
    }
    
    // 5. GM DD 21 Nm (Moza R21 Ultra)
    {
        Preset p("GM DD 21 Nm (Moza R21 Ultra)", true);
        p.gain = 1.454f;
        p.wheelbase_max_nm = 21.0f;
        p.target_rim_nm = 12.0f;
        p.min_force = 0.0f;
        p.steering_shaft_gain = 1.989f;
        p.steering_shaft_smoothing = 0.0f;
        p.understeer = 0.638f;
        p.flatspot_suppression = true;
        p.notch_q = 0.57f;
        p.flatspot_strength = 1.0f;
        p.static_notch_enabled = false;
        p.static_notch_freq = 11.0f;
        p.static_notch_width = 2.0f;
        p.oversteer_boost = 0.0f;
        p.sop = 0.0f;
        p.rear_align_effect = 0.29f;
        p.sop_yaw_gain = 0.0f;
        p.yaw_kick_threshold = 0.0f;
        p.yaw_smoothing = 0.015f;
        p.gyro_gain = 0.0f;
        p.gyro_smoothing = 0.0f;
        p.sop_smoothing = 0.0f;
        p.sop_scale = 0.89f;
        p.understeer_affects_sop = false;
        p.slip_smoothing = 0.002f;
        p.chassis_smoothing = 0.0f;
        p.optimal_slip_angle = 0.1f;
        p.optimal_slip_ratio = 0.12f;
        p.lockup_enabled = true;
        p.lockup_gain = 0.977f;
        p.brake_load_cap = 81.0f;
        p.lockup_freq_scale = 1.0f;
        p.lockup_gamma = 1.0f;
        p.lockup_start_pct = 1.0f;
        p.lockup_full_pct = 7.5f;
        p.lockup_prediction_sens = 10.0f;
        p.lockup_bump_reject = 0.1f;
        p.lockup_rear_boost = 1.0f;
        p.abs_pulse_enabled = false;
        p.abs_gain = 2.1f;
        p.abs_freq = 25.5f;
        p.texture_load_cap = 1.5f;
        p.slide_enabled = false;
        p.slide_gain = 0.0f;
        p.slide_freq = 1.47f;
        p.road_enabled = true;
        p.road_gain = 0.0f;
        p.road_fallback_scale = 0.05f;
        p.spin_enabled = true;
        p.spin_gain = 0.462185f;
        p.spin_freq_scale = 1.8f;
        p.scrub_drag_gain = 0.333f;
        p.bottoming_method = 1;
        p.speed_gate_lower = 1.0f;
        p.speed_gate_upper = 5.0f;
        presets.push_back(p);
    }
    
    // 6. GM + Yaw Kick DD 21 Nm (Moza R21 Ultra)
    {
        // Copy GM preset and add yaw kick
        Preset p("GM + Yaw Kick DD 21 Nm (Moza R21 Ultra)", true);
        p.gain = 1.454f;
        p.wheelbase_max_nm = 21.0f;
        p.target_rim_nm = 12.0f;
        p.min_force = 0.0f;
        p.steering_shaft_gain = 1.989f;
        p.steering_shaft_smoothing = 0.0f;
        p.understeer = 0.638f;
        p.flatspot_suppression = true;
        p.notch_q = 0.57f;
        p.flatspot_strength = 1.0f;
        p.static_notch_enabled = false;
        p.static_notch_freq = 11.0f;
        p.static_notch_width = 2.0f;
        p.oversteer_boost = 0.0f;
        p.sop = 0.0f;
        p.rear_align_effect = 0.29f;
        p.sop_yaw_gain = 0.333f;  // ONLY DIFFERENCE: Added yaw kick
        p.yaw_kick_threshold = 0.0f;
        p.yaw_smoothing = 0.003f;
        p.gyro_gain = 0.0f;
        p.gyro_smoothing = 0.0f;
        p.sop_smoothing = 0.0f;
        p.sop_scale = 0.89f;
        p.understeer_affects_sop = false;
        p.slip_smoothing = 0.002f;
        p.chassis_smoothing = 0.0f;
        p.optimal_slip_angle = 0.1f;
        p.optimal_slip_ratio = 0.12f;
        p.lockup_enabled = true;
        p.lockup_gain = 0.977f;
        p.brake_load_cap = 81.0f;
        p.lockup_freq_scale = 1.0f;
        p.lockup_gamma = 1.0f;
        p.lockup_start_pct = 1.0f;
        p.lockup_full_pct = 7.5f;
        p.lockup_prediction_sens = 10.0f;
        p.lockup_bump_reject = 0.1f;
        p.lockup_rear_boost = 1.0f;
        p.abs_pulse_enabled = false;
        p.abs_gain = 2.1f;
        p.abs_freq = 25.5f;
        p.texture_load_cap = 1.5f;
        p.slide_enabled = false;
        p.slide_gain = 0.0f;
        p.slide_freq = 1.47f;
        p.road_enabled = true;
        p.road_gain = 0.0f;
        p.road_fallback_scale = 0.05f;
        p.spin_enabled = true;
        p.spin_gain = 0.462185f;
        p.spin_freq_scale = 1.8f;
        p.scrub_drag_gain = 0.333f;
        p.bottoming_method = 1;
        p.speed_gate_lower = 1.0f;
        p.speed_gate_upper = 5.0f;
        presets.push_back(p);
    }
    
    // 8. Test: Game Base FFB Only
    presets.push_back(Preset("Test: Game Base FFB Only", true)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetSoPScale(1.0f)
        .SetSmoothing(0.0f)
        .SetSlipSmoothing(0.015f)
        .SetSlide(false, 0.0f)
        .SetRearAlign(0.0f)
    );

    // 9. Test: SoP Only
    presets.push_back(Preset("Test: SoP Only", true)
        .SetUndersteer(0.0f)
        .SetSoP(0.08f)
        .SetSoPScale(1.0f)
        .SetSmoothing(0.0f)
        .SetSlipSmoothing(0.015f)
        .SetSlide(false, 0.0f)
        .SetRearAlign(0.0f)
        .SetSoPYaw(0.0f)
    );

    // 10. Test: Understeer Only (Updated v0.6.31 for proper effect isolation)
    presets.push_back(Preset("Test: Understeer Only", true)
        // PRIMARY EFFECT
        .SetUndersteer(0.61f)
        
        // DISABLE ALL OTHER EFFECTS
        .SetSoP(0.0f)
        .SetSoPScale(1.0f)
        .SetOversteer(0.0f)          // Disable oversteer boost
        .SetRearAlign(0.0f)
        .SetSoPYaw(0.0f)             // Disable yaw kick
        .SetGyro(0.0f)               // Disable gyro damping
        
        // DISABLE ALL TEXTURES
        .SetSlide(false, 0.0f)
        .SetRoad(false, 0.0f)        // Disable road texture
        .SetSpin(false, 0.0f)        // Disable spin
        .SetLockup(false, 0.0f)      // Disable lockup vibration
        .SetAdvancedBraking(0.5f, 20.0f, 0.1f, false, 0.0f)  // Disable ABS pulse
        .SetScrub(0.0f)
        
        // SMOOTHING
        .SetSmoothing(0.0f)         // SoP smoothing (doesn't affect test since SoP=0)
        .SetSlipSmoothing(0.015f)    // Slip angle smoothing (important for grip calculation)
        
        // PHYSICS PARAMETERS (Explicit for clarity and future-proofing)
        .SetOptimalSlip(0.10f, 0.12f)  // Explicit optimal slip thresholds
        .SetSpeedGate(0.0f, 0.0f)      // Disable speed gate (0 = no gating)
    );

    // 11. Test: Yaw Kick Only
    presets.push_back(Preset("Test: Yaw Kick Only", true)
        // PRIMARY EFFECT
        .SetSoPYaw(0.386555f)        // Yaw kick at T300 level
        .SetYawKickThreshold(1.68f)  // T300 threshold
        .SetYawSmoothing(0.005f)     // T300 smoothing
        
        // DISABLE ALL OTHER EFFECTS
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetSoPScale(1.0f)
        .SetOversteer(0.0f)
        .SetRearAlign(0.0f)
        .SetGyro(0.0f)
        
        // DISABLE ALL TEXTURES
        .SetSlide(false, 0.0f)
        .SetRoad(false, 0.0f)
        .SetSpin(false, 0.0f)
        .SetLockup(false, 0.0f)
        .SetAdvancedBraking(0.5f, 20.0f, 0.1f, false, 0.0f)
        .SetScrub(0.0f)
        
        // SMOOTHING
        .SetSmoothing(0.0f)
        .SetSlipSmoothing(0.015f)
    );

    // 12. Test: Textures Only
    presets.push_back(Preset("Test: Textures Only", true)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetSoPScale(0.0f)
        .SetSmoothing(0.0f)
        .SetSlipSmoothing(0.015f)
        .SetLockup(true, 1.0f)
        .SetSpin(true, 1.0f)
        .SetSlide(true, 0.39f)
        .SetRoad(true, 1.0f)
        .SetRearAlign(0.0f)
    );

    // 13. Test: Rear Align Torque Only
    presets.push_back(Preset("Test: Rear Align Torque Only", true)
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetSmoothing(0.0f)
        .SetSlipSmoothing(0.015f)
        .SetSlide(false, 0.0f)
        .SetRearAlign(0.90f)
        .SetSoPYaw(0.0f)
    );

    // 14. Test: SoP Base Only
    presets.push_back(Preset("Test: SoP Base Only", true)
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(0.08f)
        .SetSmoothing(0.0f)
        .SetSlipSmoothing(0.015f)
        .SetSlide(false, 0.0f)
        .SetRearAlign(0.0f)
        .SetSoPYaw(0.0f)
    );

    // 15. Test: Slide Texture Only
    presets.push_back(Preset("Test: Slide Texture Only", true)
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetSmoothing(0.0f)
        .SetSlipSmoothing(0.015f)
        .SetSlide(true, 0.39f, 1.0f)
        .SetRearAlign(0.0f)
    );

    // 16. Test: No Effects
    presets.push_back(Preset("Test: No Effects", true)
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetSmoothing(0.0f)
        .SetSlipSmoothing(0.015f)
        .SetSlide(false, 0.0f)
        .SetRearAlign(0.0f)
    );

    // --- NEW GUIDE PRESETS (v0.4.24) ---

    // 17. Guide: Understeer (Front Grip Loss)
    presets.push_back(Preset("Guide: Understeer (Front Grip)", true)
        .SetGain(1.0f)
        .SetUndersteer(0.61f)
        .SetSoP(0.0f)
        .SetOversteer(0.0f)
        .SetRearAlign(0.0f)
        .SetSoPYaw(0.0f)
        .SetGyro(0.0f)
        .SetLockup(false, 0.0f)
        .SetSpin(false, 0.0f)
        .SetSlide(false, 0.0f)
        .SetRoad(false, 0.0f)
        .SetScrub(0.0f)
        .SetSmoothing(0.0f)
        .SetSlipSmoothing(0.015f)
    );

    // 18. Guide: Oversteer (Rear Grip Loss)
    presets.push_back(Preset("Guide: Oversteer (Rear Grip)", true)
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(0.08f)
        .SetSoPScale(1.0f)
        .SetRearAlign(0.90f)
        .SetOversteer(0.65f)
        .SetSoPYaw(0.0f)
        .SetGyro(0.0f)
        .SetLockup(false, 0.0f)
        .SetSpin(false, 0.0f)
        .SetSlide(false, 0.0f)
        .SetRoad(false, 0.0f)
        .SetScrub(0.0f)
        .SetSmoothing(0.0f)
        .SetSlipSmoothing(0.015f)
    );

    // 19. Guide: Slide Texture (Scrubbing)
    presets.push_back(Preset("Guide: Slide Texture (Scrub)", true)
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetOversteer(0.0f)
        .SetRearAlign(0.0f)
        .SetSlide(true, 0.39f, 1.0f) // Gain 0.39, Freq 1.0 (Rumble)
        .SetScrub(1.0f)
        .SetLockup(false, 0.0f)
        .SetSpin(false, 0.0f)
        .SetRoad(false, 0.0f)
        .SetSmoothing(0.0f)
        .SetSlipSmoothing(0.015f)
    );

    // 20. Guide: Braking Lockup
    presets.push_back(Preset("Guide: Braking Lockup", true)
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetOversteer(0.0f)
        .SetRearAlign(0.0f)
        .SetLockup(true, 1.0f)
        .SetSpin(false, 0.0f)
        .SetSlide(false, 0.0f)
        .SetRoad(false, 0.0f)
        .SetScrub(0.0f)
        .SetSmoothing(0.0f)
        .SetSlipSmoothing(0.015f)
    );

    // 21. Guide: Traction Loss (Wheel Spin)
    presets.push_back(Preset("Guide: Traction Loss (Spin)", true)
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetOversteer(0.0f)
        .SetRearAlign(0.0f)
        .SetSpin(true, 1.0f)
        .SetLockup(false, 0.0f)
        .SetSlide(false, 0.0f)
        .SetRoad(false, 0.0f)
        .SetScrub(0.0f)
        .SetSmoothing(0.0f)
        .SetSlipSmoothing(0.015f)
    );

     // 22. Guide: SoP Yaw (Kick)
    presets.push_back(Preset("Guide: SoP Yaw (Kick)", true)
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetOversteer(0.0f)
        .SetRearAlign(0.0f)
        .SetSoPYaw(5.0f) // Standard T300 level
        .SetGyro(0.0f)
        .SetLockup(false, 0.0f)
        .SetSpin(false, 0.0f)
        .SetSlide(false, 0.0f)
        .SetRoad(false, 0.0f)
        .SetScrub(0.0f)
        .SetSmoothing(0.0f)
        .SetSlipSmoothing(0.015f)
    );

    // 23. Guide: Gyroscopic Damping
    presets.push_back(Preset("Guide: Gyroscopic Damping", true)
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetOversteer(0.0f)
        .SetRearAlign(0.0f)
        .SetSoPYaw(0.0f)
        .SetGyro(1.0f) // Max damping
        .SetLockup(false, 0.0f)
        .SetSpin(false, 0.0f)
        .SetSlide(false, 0.0f)
        .SetRoad(false, 0.0f)
        .SetScrub(0.0f)
        .SetSmoothing(0.0f)
        .SetSlipSmoothing(0.015f)
    );

    // --- Parse User Presets from config.ini ---
    // (Keep the existing parsing logic below, it works fine for file I/O)
    std::ifstream file(m_config_path);
    if (!file.is_open()) return;

    std::string line;
    bool in_presets = false;
    bool needs_save = false;
    
    std::string current_preset_name = "";
    Preset current_preset; // Uses default constructor with default values
    std::string current_preset_version = "";
    bool preset_pending = false;
    bool legacy_torque_hack = false;
    float legacy_torque_val = 100.0f;

    while (std::getline(file, line)) {
        // Strip whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty() || line[0] == ';') continue;

        if (line[0] == '[') {
            if (preset_pending && !current_preset_name.empty()) {
                current_preset.name = current_preset_name;
                current_preset.is_builtin = false; // User preset
                
                // Issue #211: Legacy 100Nm hack scaling
                if (legacy_torque_hack && IsVersionLessEqual(current_preset_version, "0.7.66")) {
                    current_preset.gain *= (15.0f / legacy_torque_val);
                    Logger::Get().LogFile("[Config] Migrated legacy 100Nm hack for preset '%s'. Scaling gain.", current_preset_name.c_str());
                    needs_save = true;
                }

                // MIGRATION: If version is missing or old, update it
                if (current_preset_version.empty() || IsVersionLessEqual(current_preset_version, "0.7.146")) {
                    current_preset.sop_smoothing = 0.0f; // Issue #37: Reset to Raw for migration
                    needs_save = true;
                    if (current_preset_version.empty()) {
                        current_preset.app_version = LMUFFB_VERSION;
                        Logger::Get().LogFile("[Config] Migrated legacy preset '%s' to version %s", current_preset_name.c_str(), LMUFFB_VERSION);
                    } else {
                        Logger::Get().LogFile("[Config] Reset SoP Smoothing for migrated preset '%s' (v%s)", current_preset_name.c_str(), current_preset_version.c_str());
                        current_preset.app_version = LMUFFB_VERSION;
                    }
                } else {
                    current_preset.app_version = current_preset_version;
                }
                
                current_preset.Validate(); // v0.7.15: Validate before adding
                presets.push_back(current_preset);
                preset_pending = false;
            }
            
            if (line == "[Presets]") {
                in_presets = true;
            } else if (line.rfind("[Preset:", 0) == 0) { 
                in_presets = false; 
                size_t end_pos = line.find(']');
                if (end_pos != std::string::npos) {
                    current_preset_name = line.substr(8, end_pos - 8);
                    current_preset = Preset(current_preset_name, false); // Reset to defaults, not builtin
                    preset_pending = true;
                    current_preset_version = "";
                    legacy_torque_hack = false;
                    legacy_torque_val = 100.0f;
                }
            } else {
                in_presets = false;
            }
            continue;
        }

        if (preset_pending) {
            ParsePresetLine(line, current_preset, current_preset_version, needs_save, legacy_torque_hack, legacy_torque_val);
        }
    }
    
    if (preset_pending && !current_preset_name.empty()) {
        current_preset.name = current_preset_name;
        current_preset.is_builtin = false;
        
        // Issue #211: Legacy 100Nm hack scaling
        if (legacy_torque_hack && IsVersionLessEqual(current_preset_version, "0.7.66")) {
            current_preset.gain *= (15.0f / legacy_torque_val);
            Logger::Get().LogFile("[Config] Migrated legacy 100Nm hack for preset '%s'. Scaling gain.", current_preset_name.c_str());
            needs_save = true;
        }

        // MIGRATION: If version is missing or old, update it
        if (current_preset_version.empty() || IsVersionLessEqual(current_preset_version, "0.7.146")) {
            current_preset.sop_smoothing = 0.0f; // Issue #37: Reset to Raw for migration
            needs_save = true;
            if (current_preset_version.empty()) {
                current_preset.app_version = LMUFFB_VERSION;
                Logger::Get().LogFile("[Config] Migrated legacy preset '%s' to version %s", current_preset_name.c_str(), LMUFFB_VERSION);
            } else {
                Logger::Get().LogFile("[Config] Reset SoP Smoothing for migrated preset '%s' (v%s)", current_preset_name.c_str(), current_preset_version.c_str());
                current_preset.app_version = LMUFFB_VERSION;
            }
        } else {
            current_preset.app_version = current_preset_version;
        }
        
        current_preset.Validate(); // v0.7.15: Validate before adding
        presets.push_back(current_preset);
    }

    // Auto-save if migration occurred
    if (needs_save) {
        m_needs_save = true;
    }
}

void Config::ApplyPreset(int index, FFBEngine& engine) {
    if (index >= 0 && index < presets.size()) {
        std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
        presets[index].Apply(engine);
        m_last_preset_name = presets[index].name;
        Logger::Get().LogFile("[Config] Applied preset: %s", presets[index].name.c_str());
        Save(engine); // Integrated Auto-Save (v0.6.27)
    }
}

void Config::WritePresetFields(std::ofstream& file, const Preset& p) {
    file << "app_version=" << p.app_version << "\n";
    file << "gain=" << p.gain << "\n";
    file << "wheelbase_max_nm=" << p.wheelbase_max_nm << "\n";
    file << "target_rim_nm=" << p.target_rim_nm << "\n";
    file << "min_force=" << p.min_force << "\n";

    file << "steering_shaft_gain=" << p.steering_shaft_gain << "\n";
    file << "ingame_ffb_gain=" << p.ingame_ffb_gain << "\n";
    file << "steering_shaft_smoothing=" << p.steering_shaft_smoothing << "\n";
    file << "understeer=" << p.understeer << "\n";
    file << "torque_source=" << p.torque_source << "\n";
    file << "torque_passthrough=" << p.torque_passthrough << "\n";
    file << "flatspot_suppression=" << p.flatspot_suppression << "\n";
    file << "notch_q=" << p.notch_q << "\n";
    file << "flatspot_strength=" << p.flatspot_strength << "\n";
    file << "static_notch_enabled=" << p.static_notch_enabled << "\n";
    file << "static_notch_freq=" << p.static_notch_freq << "\n";
    file << "static_notch_width=" << p.static_notch_width << "\n";

    file << "oversteer_boost=" << p.oversteer_boost << "\n";
    file << "long_load_effect=" << p.long_load_effect << "\n";
    file << "long_load_smoothing=" << p.long_load_smoothing << "\n";
    file << "long_load_transform=" << p.long_load_transform << "\n";
    file << "grip_smoothing_steady=" << p.grip_smoothing_steady << "\n";
    file << "grip_smoothing_fast=" << p.grip_smoothing_fast << "\n";
    file << "grip_smoothing_sensitivity=" << p.grip_smoothing_sensitivity << "\n";
    file << "sop=" << p.sop << "\n";
    file << "lateral_load_effect=" << p.lateral_load << "\n";
    file << "lat_load_transform=" << p.lat_load_transform << "\n";
    file << "rear_align_effect=" << p.rear_align_effect << "\n";
    file << "sop_yaw_gain=" << p.sop_yaw_gain << "\n";
    file << "yaw_kick_threshold=" << p.yaw_kick_threshold << "\n";
    file << "unloaded_yaw_gain=" << p.unloaded_yaw_gain << "\n";
    file << "unloaded_yaw_threshold=" << p.unloaded_yaw_threshold << "\n";
    file << "unloaded_yaw_sens=" << p.unloaded_yaw_sens << "\n";
    file << "unloaded_yaw_gamma=" << p.unloaded_yaw_gamma << "\n";
    file << "unloaded_yaw_punch=" << p.unloaded_yaw_punch << "\n";
    file << "power_yaw_gain=" << p.power_yaw_gain << "\n";
    file << "power_yaw_threshold=" << p.power_yaw_threshold << "\n";
    file << "power_slip_threshold=" << p.power_slip_threshold << "\n";
    file << "power_yaw_gamma=" << p.power_yaw_gamma << "\n";
    file << "power_yaw_punch=" << p.power_yaw_punch << "\n";
    file << "yaw_accel_smoothing=" << p.yaw_smoothing << "\n";
    file << "gyro_gain=" << p.gyro_gain << "\n";
    file << "gyro_smoothing_factor=" << p.gyro_smoothing << "\n";
    file << "sop_smoothing_factor=" << p.sop_smoothing << "\n";
    file << "sop_scale=" << p.sop_scale << "\n";
    file << "understeer_affects_sop=" << p.understeer_affects_sop << "\n";
    file << "slope_detection_enabled=" << p.slope_detection_enabled << "\n";
    file << "slope_sg_window=" << p.slope_sg_window << "\n";
    file << "slope_sensitivity=" << p.slope_sensitivity << "\n";

    file << "slope_smoothing_tau=" << p.slope_smoothing_tau << "\n";
    file << "slope_min_threshold=" << p.slope_min_threshold << "\n";
    file << "slope_max_threshold=" << p.slope_max_threshold << "\n";
    file << "slope_alpha_threshold=" << p.slope_alpha_threshold << "\n";
    file << "slope_decay_rate=" << p.slope_decay_rate << "\n";
    file << "slope_confidence_enabled=" << p.slope_confidence_enabled << "\n";
    file << "slope_g_slew_limit=" << p.slope_g_slew_limit << "\n";
    file << "slope_use_torque=" << (p.slope_use_torque ? "1" : "0") << "\n";
    file << "slope_torque_sensitivity=" << p.slope_torque_sensitivity << "\n";
    file << "slope_confidence_max_rate=" << p.slope_confidence_max_rate << "\n";

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
    file << "vibration_gain=" << p.vibration_gain << "\n";
    file << "road_fallback_scale=" << p.road_fallback_scale << "\n";
    file << "dynamic_normalization_enabled=" << (p.dynamic_normalization_enabled ? "1" : "0") << "\n";
    file << "auto_load_normalization_enabled=" << (p.auto_load_normalization_enabled ? "1" : "0") << "\n";
    file << "soft_lock_enabled=" << (p.soft_lock_enabled ? "1" : "0") << "\n";
    file << "soft_lock_stiffness=" << p.soft_lock_stiffness << "\n";
    file << "soft_lock_damping=" << p.soft_lock_damping << "\n";
    file << "spin_enabled=" << (p.spin_enabled ? "1" : "0") << "\n";
    file << "spin_gain=" << p.spin_gain << "\n";
    file << "spin_freq_scale=" << p.spin_freq_scale << "\n";
    file << "scrub_drag_gain=" << p.scrub_drag_gain << "\n";
    file << "bottoming_method=" << p.bottoming_method << "\n";
    file << "rest_api_fallback_enabled=" << (p.rest_api_enabled ? "1" : "0") << "\n";
    file << "rest_api_port=" << p.rest_api_port << "\n";

    file << "safety_window_duration=" << p.safety_window_duration << "\n";
    file << "safety_gain_reduction=" << p.safety_gain_reduction << "\n";
    file << "safety_smoothing_tau=" << p.safety_smoothing_tau << "\n";
    file << "spike_detection_threshold=" << p.spike_detection_threshold << "\n";
    file << "immediate_spike_threshold=" << p.immediate_spike_threshold << "\n";
    file << "safety_slew_full_scale_time_s=" << p.safety_slew_full_scale_time_s << "\n";
    file << "stutter_safety_enabled=" << (p.stutter_safety_enabled ? "1" : "0") << "\n";
    file << "stutter_threshold=" << p.stutter_threshold << "\n";

    file << "speed_gate_lower=" << p.speed_gate_lower << "\n";
    file << "speed_gate_upper=" << p.speed_gate_upper << "\n";
}

void Config::ExportPreset(int index, const std::string& filename) {
    if (index < 0 || index >= presets.size()) return;

    const Preset& p = presets[index];
    std::ofstream file(filename);
    if (file.is_open()) {
        file << "[Preset:" << p.name << "]\n";
        WritePresetFields(file, p);
        file.close();
        Logger::Get().LogFile("[Config] Exported preset '%s' to %s", p.name.c_str(), filename.c_str());
    } else {
        Logger::Get().LogFile("[Config] Failed to export preset to %s", filename.c_str());
    }
}

bool Config::ImportPreset(const std::string& filename, const FFBEngine& engine) {
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
        // Strip whitespace
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
        current_preset.is_builtin = false;

        // Issue #211: Legacy 100Nm hack scaling
        if (legacy_torque_hack && IsVersionLessEqual(current_preset_version, "0.7.66")) {
            current_preset.gain *= (15.0f / legacy_torque_val);
            Logger::Get().LogFile("[Config] Migrated legacy 100Nm hack for imported preset '%s'. Scaling gain.", current_preset_name.c_str());
        }

        current_preset.app_version = current_preset_version.empty() ? LMUFFB_VERSION : current_preset_version;

        // Migration for imported preset (Issue #37)
        if (current_preset_version.empty() || IsVersionLessEqual(current_preset_version, "0.7.146")) {
            current_preset.sop_smoothing = 0.0f;
            current_preset.app_version = LMUFFB_VERSION;
            Logger::Get().LogFile("[Config] Reset SoP Smoothing for imported legacy preset '%s'", current_preset.name.c_str());
        }

        // Handle name collision
        std::string base_name = current_preset.name;
        int counter = 1;
        bool exists = true;
        while (exists) {
            exists = false;
            for (const auto& p : presets) {
                if (p.name == current_preset.name) {
                    current_preset.name = base_name + " (" + std::to_string(counter++) + ")";
                    exists = true;
                    break;
                }
            }
        }

        current_preset.Validate(); // v0.7.15: Validate before adding
        presets.push_back(current_preset);
        imported = true;
    }

    if (imported) {
        Save(engine);
        Logger::Get().LogFile("[Config] Imported preset '%s' from %s", current_preset.name.c_str(), filename.c_str());
        return true;
    }

    return false;
}

void Config::AddUserPreset(const std::string& name, const FFBEngine& engine) {
    // Check if name exists and overwrite, or add new
    bool found = false;
    for (auto& p : presets) {
        if (p.name == name && !p.is_builtin) {
            p.UpdateFromEngine(engine);
            found = true;
            break;
        }
    }
    
    if (!found) {
        Preset p(name, false);
        p.UpdateFromEngine(engine);
        presets.push_back(p);
    }
    
    m_last_preset_name = name;

    // Save immediately to persist
    Save(engine);
}

void Config::DeletePreset(int index, const FFBEngine& engine) {
    if (index < 0 || index >= (int)presets.size()) return;
    if (presets[index].is_builtin) return; // Cannot delete builtin presets

    std::string name = presets[index].name;
    presets.erase(presets.begin() + index);
    Logger::Get().LogFile("[Config] Deleted preset: %s", name.c_str());

    // If the deleted preset was the last used one, reset it
    if (m_last_preset_name == name) {
        m_last_preset_name = "Default";
    }

    Save(engine);
}

void Config::DuplicatePreset(int index, const FFBEngine& engine) {
    if (index < 0 || index >= (int)presets.size()) return;

    Preset p = presets[index];
    p.name = p.name + " (Copy)";
    p.is_builtin = false;
    p.app_version = LMUFFB_VERSION;

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
    Logger::Get().LogFile("[Config] Duplicated preset to: %s", p.name.c_str());
    Save(engine);
}

bool Config::IsEngineDirtyRelativeToPreset(int index, const FFBEngine& engine) {
    if (index < 0 || index >= (int)presets.size()) return false;

    Preset current_state;
    current_state.UpdateFromEngine(engine);

    return !presets[index].Equals(current_state);
}

void Config::SetSavedStaticLoad(const std::string& vehicleName, double value) {
    std::lock_guard<std::recursive_mutex> lock(m_static_loads_mutex);
    m_saved_static_loads[vehicleName] = value;
}

bool Config::GetSavedStaticLoad(const std::string& vehicleName, double& value) {
    std::lock_guard<std::recursive_mutex> lock(m_static_loads_mutex);
    auto it = m_saved_static_loads.find(vehicleName);
    if (it != m_saved_static_loads.end()) {
        value = it->second;
        return true;
    }
    return false;
}

void Config::Save(const FFBEngine& engine, const std::string& filename) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
    std::string final_path = filename.empty() ? m_config_path : filename;
    std::ofstream file(final_path);
    if (file.is_open()) {
        file << "; --- System & Window ---\n";
        // Config Version Tracking: The ini_version field serves dual purposes:
        // 1. Records the app version that last saved this config
        // 2. Acts as an implicit config format version for migration logic
        // NOTE: Currently migration is threshold-based (e.g., understeer > 2.0 = legacy).
        //       For more complex migrations, consider adding explicit config_format_version field.
        file << "ini_version=" << LMUFFB_VERSION << "\n";
        file << "always_on_top=" << m_always_on_top << "\n";
        file << "last_device_guid=" << m_last_device_guid << "\n";
        file << "last_preset_name=" << m_last_preset_name << "\n";
        file << "win_pos_x=" << win_pos_x << "\n";
        file << "win_pos_y=" << win_pos_y << "\n";
        file << "win_w_small=" << win_w_small << "\n";
        file << "win_h_small=" << win_h_small << "\n";
        file << "win_w_large=" << win_w_large << "\n";
        file << "win_h_large=" << win_h_large << "\n";
        file << "show_graphs=" << show_graphs << "\n";
        file << "auto_start_logging=" << m_auto_start_logging << "\n";
        file << "log_path=" << m_log_path << "\n";

        file << "\n; --- General FFB ---\n";
        file << "invert_force=" << engine.m_invert_force << "\n";
        file << "gain=" << engine.m_gain << "\n";
        file << "dynamic_normalization_enabled=" << engine.m_dynamic_normalization_enabled << "\n";
        file << "auto_load_normalization_enabled=" << engine.m_auto_load_normalization_enabled << "\n";
        file << "soft_lock_enabled=" << engine.m_soft_lock_enabled << "\n";
        file << "soft_lock_stiffness=" << engine.m_soft_lock_stiffness << "\n";
        file << "soft_lock_damping=" << engine.m_soft_lock_damping << "\n";
        file << "wheelbase_max_nm=" << engine.m_wheelbase_max_nm << "\n";
        file << "target_rim_nm=" << engine.m_target_rim_nm << "\n";
        file << "min_force=" << engine.m_min_force << "\n";

        file << "\n; --- Front Axle (Understeer) ---\n";
        file << "steering_shaft_gain=" << engine.m_steering_shaft_gain << "\n";
        file << "ingame_ffb_gain=" << engine.m_ingame_ffb_gain << "\n";
        file << "steering_shaft_smoothing=" << engine.m_steering_shaft_smoothing << "\n";
        file << "understeer=" << engine.m_understeer_effect << "\n";
        file << "torque_source=" << engine.m_torque_source << "\n";
        file << "torque_passthrough=" << engine.m_torque_passthrough << "\n";
        file << "flatspot_suppression=" << engine.m_flatspot_suppression << "\n";
        file << "notch_q=" << engine.m_notch_q << "\n";
        file << "flatspot_strength=" << engine.m_flatspot_strength << "\n";
        file << "static_notch_enabled=" << engine.m_static_notch_enabled << "\n";
        file << "static_notch_freq=" << engine.m_static_notch_freq << "\n";
        file << "static_notch_width=" << engine.m_static_notch_width << "\n";

        file << "\n; --- Rear Axle (Oversteer) ---\n";
        file << "oversteer_boost=" << engine.m_oversteer_boost << "\n";
        file << "long_load_effect=" << engine.m_long_load_effect << "\n";
        file << "long_load_smoothing=" << engine.m_long_load_smoothing << "\n";
        file << "long_load_transform=" << static_cast<int>(engine.m_long_load_transform) << "\n";
        file << "grip_smoothing_steady=" << engine.m_grip_smoothing_steady << "\n";
        file << "grip_smoothing_fast=" << engine.m_grip_smoothing_fast << "\n";
        file << "grip_smoothing_sensitivity=" << engine.m_grip_smoothing_sensitivity << "\n";
        file << "sop=" << engine.m_sop_effect << "\n";
        file << "lateral_load_effect=" << engine.m_lat_load_effect << "\n";
        file << "lat_load_transform=" << static_cast<int>(engine.m_lat_load_transform) << "\n";
        file << "rear_align_effect=" << engine.m_rear_align_effect << "\n";
        file << "sop_yaw_gain=" << engine.m_sop_yaw_gain << "\n";
        file << "yaw_kick_threshold=" << engine.m_yaw_kick_threshold << "\n";
        file << "unloaded_yaw_gain=" << engine.m_unloaded_yaw_gain << "\n";
        file << "unloaded_yaw_threshold=" << engine.m_unloaded_yaw_threshold << "\n";
        file << "unloaded_yaw_sens=" << engine.m_unloaded_yaw_sens << "\n";
        file << "unloaded_yaw_gamma=" << engine.m_unloaded_yaw_gamma << "\n";
        file << "unloaded_yaw_punch=" << engine.m_unloaded_yaw_punch << "\n";
        file << "power_yaw_gain=" << engine.m_power_yaw_gain << "\n";
        file << "power_yaw_threshold=" << engine.m_power_yaw_threshold << "\n";
        file << "power_slip_threshold=" << engine.m_power_slip_threshold << "\n";
        file << "power_yaw_gamma=" << engine.m_power_yaw_gamma << "\n";
        file << "power_yaw_punch=" << engine.m_power_yaw_punch << "\n";
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

        file << "slope_smoothing_tau=" << engine.m_slope_smoothing_tau << "\n";
        file << "slope_min_threshold=" << engine.m_slope_min_threshold << "\n";
        file << "slope_max_threshold=" << engine.m_slope_max_threshold << "\n";
        file << "slope_alpha_threshold=" << engine.m_slope_alpha_threshold << "\n";
        file << "slope_decay_rate=" << engine.m_slope_decay_rate << "\n";
        file << "slope_confidence_enabled=" << engine.m_slope_confidence_enabled << "\n";
        file << "slope_g_slew_limit=" << engine.m_slope_g_slew_limit << "\n";
        file << "slope_use_torque=" << (engine.m_slope_use_torque ? "1" : "0") << "\n";
        file << "slope_torque_sensitivity=" << engine.m_slope_torque_sensitivity << "\n";
        file << "slope_confidence_max_rate=" << engine.m_slope_confidence_max_rate << "\n";

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

        file << "\n; --- Vibration Effects ---\n";
        file << "texture_load_cap=" << engine.m_texture_load_cap << "\n";
        file << "slide_enabled=" << engine.m_slide_texture_enabled << "\n";
        file << "slide_gain=" << engine.m_slide_texture_gain << "\n";
        file << "slide_freq=" << engine.m_slide_freq_scale << "\n";
        file << "road_enabled=" << engine.m_road_texture_enabled << "\n";
        file << "road_gain=" << engine.m_road_texture_gain << "\n";
        file << "vibration_gain=" << engine.m_vibration_gain << "\n";
        file << "road_fallback_scale=" << engine.m_road_fallback_scale << "\n";
        file << "spin_enabled=" << engine.m_spin_enabled << "\n";
        file << "spin_gain=" << engine.m_spin_gain << "\n";
        file << "spin_freq_scale=" << engine.m_spin_freq_scale << "\n";
        file << "scrub_drag_gain=" << engine.m_scrub_drag_gain << "\n";
        file << "bottoming_method=" << engine.m_bottoming_method << "\n";
        file << "rest_api_fallback_enabled=" << engine.m_rest_api_enabled << "\n";
        file << "rest_api_port=" << engine.m_rest_api_port << "\n";

        file << "safety_window_duration=" << engine.m_safety_window_duration << "\n";
        file << "safety_gain_reduction=" << engine.m_safety_gain_reduction << "\n";
        file << "safety_smoothing_tau=" << engine.m_safety_smoothing_tau << "\n";
        file << "spike_detection_threshold=" << engine.m_spike_detection_threshold << "\n";
        file << "immediate_spike_threshold=" << engine.m_immediate_spike_threshold << "\n";
        file << "safety_slew_full_scale_time_s=" << engine.m_safety_slew_full_scale_time_s << "\n";
        file << "stutter_safety_enabled=" << engine.m_stutter_safety_enabled << "\n";
        file << "stutter_threshold=" << engine.m_stutter_threshold << "\n";

        file << "\n; --- Advanced Settings ---\n";
        file << "speed_gate_lower=" << engine.m_speed_gate_lower << "\n";
        file << "speed_gate_upper=" << engine.m_speed_gate_upper << "\n";

        file << "\n[StaticLoads]\n";
        {
            std::lock_guard<std::recursive_mutex> static_lock(m_static_loads_mutex);
            for (const auto& pair : m_saved_static_loads) {
                file << pair.first << "=" << pair.second << "\n";
            }
        }

        file << "\n[Presets]\n";
        for (const auto& p : presets) {
            if (!p.is_builtin) {
                file << "[Preset:" << p.name << "]\n";
                WritePresetFields(file, p);
                file << "\n";
            }
        }
        
        file.close();

    } else {
        Logger::Get().LogFile("[Config] Failed to save to %s", final_path.c_str());
    }
}

void Config::Load(FFBEngine& engine, const std::string& filename) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);
    std::string final_path = filename.empty() ? m_config_path : filename;
    std::ifstream file(final_path);
    if (!file.is_open()) {
        Logger::Get().LogFile("[Config] No config found, using defaults.");
        return;
    }

    std::string line;
    bool in_static_loads = false;
    bool in_presets = false;
    std::string config_version = "";
    bool legacy_torque_hack = false;
    float legacy_torque_val = 100.0f;

    while (std::getline(file, line)) {
        // Strip whitespace and check for section headers
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty() || line[0] == ';') continue;

        if (line[0] == '[') {
            if (line == "[StaticLoads]") {
                in_static_loads = true;
                in_presets = false;
            } else if (line == "[Presets]" || line.rfind("[Preset:", 0) == 0) {
                in_presets = true;
                in_static_loads = false;
            } else {
                in_static_loads = false;
                in_presets = false;
            }
            continue;
        }

        if (in_presets) continue;

        std::istringstream is_line(line);
        std::string key;
        if (std::getline(is_line, key, '=')) {
            std::string value;
            if (std::getline(is_line, value)) {
                try {
                    if (in_static_loads) {
                        SetSavedStaticLoad(key, std::stod(value));
                    }
                    else if (key == "ini_version") {
                        config_version = value;
                        Logger::Get().LogFile("[Config] Loading config version: %s", config_version.c_str());
                    }
                    bool ns = m_needs_save.load();
                    if (SyncSystemLine(key, value, engine, config_version, legacy_torque_hack, legacy_torque_val, ns)) { m_needs_save.store(ns); continue; }
                    else if (SyncPhysicsLine(key, value, engine, config_version, ns)) { m_needs_save.store(ns); continue; }
                    else if (SyncBrakingLine(key, value, engine)) continue;
                    else if (SyncVibrationLine(key, value, engine)) continue;
                    else if (SyncSafetyLine(key, value, engine)) continue;
                } catch (...) {
                    Logger::Get().Log("[Config] Error parsing line: %s", line.c_str());
                }
            }
        }
    }
    
    // v0.7.16: Comprehensive Safety Validation & Clamping
    // These checks ensure that even if config.ini is manually edited with invalid values,
    // the engine remains stable and doesn't crash or produce NaN.

    engine.m_gain = (std::max)(0.0f, engine.m_gain);
    engine.m_wheelbase_max_nm = (std::max)(1.0f, engine.m_wheelbase_max_nm);
    engine.m_target_rim_nm = (std::max)(1.0f, engine.m_target_rim_nm);
    engine.m_min_force = (std::max)(0.0f, engine.m_min_force);
    engine.m_sop_scale = (std::max)(0.01f, engine.m_sop_scale);
    engine.m_slip_angle_smoothing = (std::max)(0.0001f, engine.m_slip_angle_smoothing);
    engine.m_notch_q = (std::max)(0.1f, engine.m_notch_q);
    engine.m_static_notch_width = (std::max)(0.1f, engine.m_static_notch_width);
    engine.m_speed_gate_upper = (std::max)(0.1f, engine.m_speed_gate_upper);

    engine.m_torque_source = (std::max)(0, (std::min)(1, engine.m_torque_source));
    engine.m_gyro_gain = (std::max)(0.0f, (std::min)(1.0f, engine.m_gyro_gain));
    engine.m_scrub_drag_gain = (std::max)(0.0f, (std::min)(1.0f, engine.m_scrub_drag_gain));

    if (engine.m_optimal_slip_angle < 0.01f) {
        Logger::Get().Log("[Config] Invalid optimal_slip_angle (%.2f), resetting to default 0.10", engine.m_optimal_slip_angle);
        Logger::Get().Log("[Config] Invalid optimal_slip_angle (%.2f), resetting to default 0.10", engine.m_optimal_slip_angle);
        engine.m_optimal_slip_angle = 0.10f;
    }
    if (engine.m_optimal_slip_ratio < 0.01f) {
        Logger::Get().Log("[Config] Invalid optimal_slip_ratio (%.2f), resetting to default 0.12", engine.m_optimal_slip_ratio);
        Logger::Get().Log("[Config] Invalid optimal_slip_ratio (%.2f), resetting to default 0.12", engine.m_optimal_slip_ratio);
        engine.m_optimal_slip_ratio = 0.12f;
    }
    
    // Slope Detection Validation
    if (engine.m_slope_sg_window < 5) engine.m_slope_sg_window = 5;
    if (engine.m_slope_sg_window > 41) engine.m_slope_sg_window = 41;
    if (engine.m_slope_sg_window % 2 == 0) engine.m_slope_sg_window++; // Must be odd
    if (engine.m_slope_sensitivity < 0.1f) engine.m_slope_sensitivity = 0.1f;
    if (engine.m_slope_sensitivity > 10.0f) engine.m_slope_sensitivity = 10.0f;
    if (engine.m_slope_smoothing_tau < 0.001f) engine.m_slope_smoothing_tau = 0.04f;
    
    if (engine.m_slope_alpha_threshold < 0.001f || engine.m_slope_alpha_threshold > 0.1f) {
        Logger::Get().Log("[Config] Invalid slope_alpha_threshold (%.3f), resetting to 0.02f", engine.m_slope_alpha_threshold);
        Logger::Get().Log("[Config] Invalid slope_alpha_threshold (%.3f), resetting to 0.02f", engine.m_slope_alpha_threshold);
        engine.m_slope_alpha_threshold = 0.02f;
    }
    if (engine.m_slope_decay_rate < 0.1f || engine.m_slope_decay_rate > 20.0f) {
        Logger::Get().Log("[Config] Invalid slope_decay_rate (%.2f), resetting to 5.0f", engine.m_slope_decay_rate);
        Logger::Get().Log("[Config] Invalid slope_decay_rate (%.2f), resetting to 5.0f", engine.m_slope_decay_rate);
        engine.m_slope_decay_rate = 5.0f;
    }

    // Advanced Slope Validation (v0.7.40)
    engine.m_slope_g_slew_limit = (std::max)(1.0f, (std::min)(1000.0f, engine.m_slope_g_slew_limit));
    engine.m_slope_torque_sensitivity = (std::max)(0.01f, (std::min)(10.0f, engine.m_slope_torque_sensitivity));
    engine.m_slope_confidence_max_rate = (std::max)(engine.m_slope_alpha_threshold + 0.01f, (std::min)(1.0f, engine.m_slope_confidence_max_rate));

    // Migration: v0.7.x sensitivity â†’ v0.7.11 thresholds
    // If loading old config with sensitivity but at default thresholds
    if (engine.m_slope_min_threshold == -0.3f && 
        engine.m_slope_max_threshold == -2.0f &&
        engine.m_slope_sensitivity != 0.5f) {
        
        // Old formula: factor = 1 - (excess * 0.1 * sens)
        // At factor=0.2 (floor): excess * 0.1 * sens = 0.8
        // excess = 0.8 / (0.1 * sens) = 8 / sens
        // max = min - excess = -0.3 - (8/sens)
        double sens = (double)engine.m_slope_sensitivity;
        if (sens > 0.01) {
            engine.m_slope_max_threshold = (float)(engine.m_slope_min_threshold - (8.0 / sens));
            Logger::Get().Log("[Config] Migrated slope_sensitivity %.2f to max_threshold %.2f", sens, engine.m_slope_max_threshold);
            Logger::Get().Log("[Config] Migrated slope_sensitivity %.2f to max_threshold %.2f", sens, engine.m_slope_max_threshold);
        }
    }

    // Validation: max should be more negative than min
    if (engine.m_slope_max_threshold > engine.m_slope_min_threshold) {
        std::swap(engine.m_slope_min_threshold, engine.m_slope_max_threshold);
        Logger::Get().Log("[Config] Swapped slope thresholds (min should be > max)");
        Logger::Get().Log("[Config] Swapped slope thresholds (min should be > max)");
    }
    
    // v0.6.20: Safety Validation - Clamp Advanced Braking Parameters to Valid Ranges
    if (engine.m_lockup_gamma < 0.1f || engine.m_lockup_gamma > 4.0f) {
        Logger::Get().Log("[Config] Invalid lockup_gamma (%.2f), clamping to range [0.1, 4.0]", engine.m_lockup_gamma);
        Logger::Get().Log("[Config] Invalid lockup_gamma (%.2f), clamping to range [0.1, 4.0]", engine.m_lockup_gamma);
        engine.m_lockup_gamma = (std::max)(0.1f, (std::min)(4.0f, engine.m_lockup_gamma));
    }
    if (engine.m_lockup_prediction_sens < 10.0f || engine.m_lockup_prediction_sens > 100.0f) {
        Logger::Get().Log("[Config] Invalid lockup_prediction_sens (%.2f), clamping to range [10.0, 100.0]", engine.m_lockup_prediction_sens);
        Logger::Get().Log("[Config] Invalid lockup_prediction_sens (%.2f), clamping to range [10.0, 100.0]", engine.m_lockup_prediction_sens);
        engine.m_lockup_prediction_sens = (std::max)(10.0f, (std::min)(100.0f, engine.m_lockup_prediction_sens));
    }
    if (engine.m_lockup_bump_reject < 0.1f || engine.m_lockup_bump_reject > 5.0f) {
        Logger::Get().Log("[Config] Invalid lockup_bump_reject (%.2f), clamping to range [0.1, 5.0]", engine.m_lockup_bump_reject);
        Logger::Get().Log("[Config] Invalid lockup_bump_reject (%.2f), clamping to range [0.1, 5.0]", engine.m_lockup_bump_reject);
        engine.m_lockup_bump_reject = (std::max)(0.1f, (std::min)(5.0f, engine.m_lockup_bump_reject));
    }
    if (engine.m_abs_gain < 0.0f || engine.m_abs_gain > 10.0f) {
        Logger::Get().Log("[Config] Invalid abs_gain (%.2f), clamping to range [0.0, 10.0]", engine.m_abs_gain);
        Logger::Get().Log("[Config] Invalid abs_gain (%.2f), clamping to range [0.0, 10.0]", engine.m_abs_gain);
        engine.m_abs_gain = (std::max)(0.0f, (std::min)(10.0f, engine.m_abs_gain));
    }
    // Issue #211: Legacy 100Nm hack scaling
    if (legacy_torque_hack && IsVersionLessEqual(config_version, "0.7.66")) {
        engine.m_gain *= (15.0f / legacy_torque_val);
        Logger::Get().Log("[Config] Migrated legacy 100Nm hack for main config. Scaling gain.");
        Logger::Get().Log("[Config] Migrated legacy 100Nm hack for main config. Scaling gain.");
        m_needs_save = true;
    }

    // Legacy Migration: Convert 0-200 range to 0-2.0 range
    if (engine.m_understeer_effect > 2.0f) {
        float old_val = engine.m_understeer_effect;
        engine.m_understeer_effect = engine.m_understeer_effect / 100.0f;
        Logger::Get().Log("[Config] Migrated legacy understeer_effect: %.2f -> %.2f", old_val, engine.m_understeer_effect);
        Logger::Get().Log("[Config] Migrated legacy understeer_effect: %.2f -> %.2f", old_val, engine.m_understeer_effect);
    }
    // Clamp to new valid range [0.0, 2.0]
    if (engine.m_understeer_effect < 0.0f || engine.m_understeer_effect > 2.0f) {
        engine.m_understeer_effect = (std::max)(0.0f, (std::min)(2.0f, engine.m_understeer_effect));
    }
    if (engine.m_steering_shaft_gain < 0.0f || engine.m_steering_shaft_gain > 2.0f) {
        engine.m_steering_shaft_gain = (std::max)(0.0f, (std::min)(2.0f, engine.m_steering_shaft_gain));
    }
    if (engine.m_ingame_ffb_gain < 0.0f || engine.m_ingame_ffb_gain > 2.0f) {
        engine.m_ingame_ffb_gain = (std::max)(0.0f, (std::min)(2.0f, engine.m_ingame_ffb_gain));
    }
    if (engine.m_lockup_gain < 0.0f || engine.m_lockup_gain > 3.0f) {
        engine.m_lockup_gain = (std::max)(0.0f, (std::min)(3.0f, engine.m_lockup_gain));
    }
    if (engine.m_brake_load_cap < 1.0f || engine.m_brake_load_cap > 10.0f) {
        engine.m_brake_load_cap = (std::max)(1.0f, (std::min)(10.0f, engine.m_brake_load_cap));
    }
    if (engine.m_lockup_rear_boost < 1.0f || engine.m_lockup_rear_boost > 10.0f) {
        engine.m_lockup_rear_boost = (std::max)(1.0f, (std::min)(10.0f, engine.m_lockup_rear_boost));
    }
    if (engine.m_oversteer_boost < 0.0f || engine.m_oversteer_boost > 4.0f) {
        engine.m_oversteer_boost = (std::max)(0.0f, (std::min)(4.0f, engine.m_oversteer_boost));
    }
    if (engine.m_sop_yaw_gain < 0.0f || engine.m_sop_yaw_gain > 1.0f) {
         engine.m_sop_yaw_gain = (std::max)(0.0f, (std::min)(1.0f, engine.m_sop_yaw_gain));
    }
    if (engine.m_slide_texture_gain < 0.0f || engine.m_slide_texture_gain > 2.0f) {
        engine.m_slide_texture_gain = (std::max)(0.0f, (std::min)(2.0f, engine.m_slide_texture_gain));
    }
    if (engine.m_road_texture_gain < 0.0f || engine.m_road_texture_gain > 2.0f) {
        engine.m_road_texture_gain = (std::max)(0.0f, (std::min)(2.0f, engine.m_road_texture_gain));
    }
    if (engine.m_spin_gain < 0.0f || engine.m_spin_gain > 2.0f) {
        engine.m_spin_gain = (std::max)(0.0f, (std::min)(2.0f, engine.m_spin_gain));
    }
    if (engine.m_rear_align_effect < 0.0f || engine.m_rear_align_effect > 2.0f) {
        engine.m_rear_align_effect = (std::max)(0.0f, (std::min)(2.0f, engine.m_rear_align_effect));
    }
    if (engine.m_sop_effect < 0.0f || engine.m_sop_effect > 2.0f) {
        engine.m_sop_effect = (std::max)(0.0f, (std::min)(2.0f, engine.m_sop_effect));
    }
    engine.m_soft_lock_stiffness = (std::max)(0.0f, engine.m_soft_lock_stiffness);
    engine.m_soft_lock_damping = (std::max)(0.0f, engine.m_soft_lock_damping);
    engine.m_rest_api_port = (std::max)(1, engine.m_rest_api_port);

    // FFB Safety Validation
    engine.m_safety_window_duration = (std::max)(0.0f, engine.m_safety_window_duration);
    engine.m_safety_gain_reduction = (std::max)(0.0f, (std::min)(1.0f, engine.m_safety_gain_reduction));
    engine.m_safety_smoothing_tau = (std::max)(0.001f, engine.m_safety_smoothing_tau);
    engine.m_spike_detection_threshold = (std::max)(1.0f, engine.m_spike_detection_threshold);
    engine.m_immediate_spike_threshold = (std::max)(1.0f, engine.m_immediate_spike_threshold);
    engine.m_safety_slew_full_scale_time_s = (std::max)(0.01f, engine.m_safety_slew_full_scale_time_s);
    engine.m_stutter_threshold = (std::max)(1.01f, engine.m_stutter_threshold);

    Logger::Get().LogFile("[Config] Loaded from %s", final_path.c_str());
}









