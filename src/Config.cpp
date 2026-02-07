#include "Config.h"
#include "PresetRegistry.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <chrono>

// Global App Settings
bool Config::m_ignore_vjoy_version_warning = false;
bool Config::m_enable_vjoy = true;
bool Config::m_output_ffb_to_vjoy = true;
bool Config::m_always_on_top = true;
bool Config::m_auto_start_logging = false;
std::string Config::m_log_path = "logs";
std::string Config::m_config_path = "config.ini";
std::string Config::m_last_device_guid = "";

// Window Geometry
int Config::win_pos_x = 100;
int Config::win_pos_y = 100;
int Config::win_w_small = 500;
int Config::win_h_small = 800;
int Config::win_w_large = 1400;
int Config::win_h_large = 800;
bool Config::show_graphs = false;

void Config::Save(const FFBEngine& engine, const std::string& filename) {
    std::string final_path = filename.empty() ? m_config_path : filename;
    std::ofstream file(final_path);
    if (!file.is_open()) return;

    file << "; --- System & Window ---\n";
    file << "ini_version=" << LMUFFB_VERSION << "\n";
    file << "ignore_vjoy_version_warning=" << m_ignore_vjoy_version_warning << "\n";
    file << "enable_vjoy=" << m_enable_vjoy << "\n";
    file << "output_ffb_to_vjoy=" << m_output_ffb_to_vjoy << "\n";
    file << "always_on_top=" << m_always_on_top << "\n";
    file << "last_device_guid=" << m_last_device_guid << "\n";
    file << "last_preset_name=" << PresetRegistry::Get().GetLastPresetName() << "\n";
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

    PresetRegistry::Get().WritePresets(file);
    file.close();
}

void Config::Load(FFBEngine& engine, const std::string& filename) {
    std::string final_path = filename.empty() ? m_config_path : filename;
    std::ifstream file(final_path);
    if (!file.is_open()) {
        PresetRegistry::Get().Load(final_path);
        return;
    }

    bool has_max_threshold = false;
    float sensitivity = engine.m_slope_sensitivity;

    std::string line;
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        if (line.empty() || line[0] == ';') continue;
        if (line[0] == '[') break;

        std::istringstream is_line(line);
        std::string key;
        if (std::getline(is_line, key, '=')) {
            std::string value;
            if (std::getline(is_line, value)) {
                try {
                    if (key == "ignore_vjoy_version_warning") m_ignore_vjoy_version_warning = std::stoi(value);
                    else if (key == "enable_vjoy") m_enable_vjoy = std::stoi(value);
                    else if (key == "output_ffb_to_vjoy") m_output_ffb_to_vjoy = std::stoi(value);
                    else if (key == "always_on_top") m_always_on_top = std::stoi(value);
                    else if (key == "last_device_guid") m_last_device_guid = value;
                    else if (key == "last_preset_name") PresetRegistry::Get().SetLastPresetName(value);
                    else if (key == "win_pos_x") win_pos_x = std::stoi(value);
                    else if (key == "win_pos_y") win_pos_y = std::stoi(value);
                    else if (key == "win_w_small") win_w_small = std::stoi(value);
                    else if (key == "win_h_small") win_h_small = std::stoi(value);
                    else if (key == "win_w_large") win_w_large = std::stoi(value);
                    else if (key == "win_h_large") win_h_large = std::stoi(value);
                    else if (key == "show_graphs") show_graphs = std::stoi(value);
                    else if (key == "auto_start_logging") m_auto_start_logging = std::stoi(value);
                    else if (key == "log_path") m_log_path = value;
                    else if (key == "gain") engine.m_gain = std::stof(value);
                    else if (key == "sop_smoothing_factor") engine.m_sop_smoothing_factor = std::stof(value);
                    else if (key == "sop_scale") engine.m_sop_scale = std::stof(value);
                    else if (key == "slip_angle_smoothing") engine.m_slip_angle_smoothing = std::stof(value);
                    else if (key == "texture_load_cap") engine.m_texture_load_cap = std::stof(value);
                    else if (key == "max_load_factor") engine.m_texture_load_cap = std::stof(value);
                    else if (key == "brake_load_cap") {
                        float val = std::stof(value);
                        engine.m_brake_load_cap = (std::min)(10.0f, (std::max)(1.0f, val));
                    }
                    else if (key == "smoothing") engine.m_sop_smoothing_factor = std::stof(value);
                    else if (key == "understeer") {
                        float val = std::stof(value);
                        if (val > 2.0f) val /= 100.0f; // Legacy % support
                        engine.m_understeer_effect = val;
                    }
                    else if (key == "sop") engine.m_sop_effect = std::stof(value);
                    else if (key == "min_force") engine.m_min_force = std::stof(value);
                    else if (key == "oversteer_boost") engine.m_oversteer_boost = std::stof(value);
                    else if (key == "lockup_enabled") engine.m_lockup_enabled = std::stoi(value);
                    else if (key == "lockup_gain") engine.m_lockup_gain = std::stof(value);
                    else if (key == "lockup_start_pct") engine.m_lockup_start_pct = std::stof(value);
                    else if (key == "lockup_full_pct") engine.m_lockup_full_pct = std::stof(value);
                    else if (key == "lockup_rear_boost") engine.m_lockup_rear_boost = std::stof(value);
                    else if (key == "lockup_gamma") engine.m_lockup_gamma = std::stof(value);
                    else if (key == "lockup_prediction_sens") engine.m_lockup_prediction_sens = std::stof(value);
                    else if (key == "lockup_bump_reject") engine.m_lockup_bump_reject = std::stof(value);
                    else if (key == "abs_pulse_enabled") engine.m_abs_pulse_enabled = std::stoi(value);
                    else if (key == "abs_gain") engine.m_abs_gain = std::stof(value);
                    else if (key == "spin_enabled") engine.m_spin_enabled = std::stoi(value);
                    else if (key == "spin_gain") engine.m_spin_gain = std::stof(value);
                    else if (key == "slide_enabled") engine.m_slide_texture_enabled = std::stoi(value);
                    else if (key == "slide_gain") engine.m_slide_texture_gain = std::stof(value);
                    else if (key == "slide_freq") engine.m_slide_freq_scale = std::stof(value);
                    else if (key == "road_enabled") engine.m_road_texture_enabled = std::stoi(value);
                    else if (key == "road_gain") engine.m_road_texture_gain = std::stof(value);
                    else if (key == "invert_force") engine.m_invert_force = std::stoi(value);
                    else if (key == "max_torque_ref") engine.m_max_torque_ref = std::stof(value);
                    else if (key == "abs_freq") engine.m_abs_freq_hz = std::stof(value);
                    else if (key == "lockup_freq_scale") engine.m_lockup_freq_scale = std::stof(value);
                    else if (key == "spin_freq_scale") engine.m_spin_freq_scale = std::stof(value);
                    else if (key == "bottoming_method") engine.m_bottoming_method = std::stoi(value);
                    else if (key == "scrub_drag_gain") engine.m_scrub_drag_gain = std::stof(value);
                    else if (key == "rear_align_effect") engine.m_rear_align_effect = std::stof(value);
                    else if (key == "sop_yaw_gain") engine.m_sop_yaw_gain = std::stof(value);
                    else if (key == "steering_shaft_gain") engine.m_steering_shaft_gain = std::stof(value);
                    else if (key == "base_force_mode") engine.m_base_force_mode = std::stoi(value);
                    else if (key == "gyro_gain") engine.m_gyro_gain = std::stof(value);
                    else if (key == "flatspot_suppression") engine.m_flatspot_suppression = std::stoi(value);
                    else if (key == "notch_q") engine.m_notch_q = std::stof(value);
                    else if (key == "flatspot_strength") engine.m_flatspot_strength = std::stof(value);
                    else if (key == "static_notch_enabled") engine.m_static_notch_enabled = std::stoi(value);
                    else if (key == "static_notch_freq") engine.m_static_notch_freq = std::stof(value);
                    else if (key == "static_notch_width") engine.m_static_notch_width = std::stof(value);
                    else if (key == "yaw_kick_threshold") engine.m_yaw_kick_threshold = std::stof(value);
                    else if (key == "optimal_slip_angle") engine.m_optimal_slip_angle = std::stof(value);
                    else if (key == "optimal_slip_ratio") engine.m_optimal_slip_ratio = std::stof(value);
                    else if (key == "slope_detection_enabled") engine.m_slope_detection_enabled = (value == "1");
                    else if (key == "slope_sg_window") engine.m_slope_sg_window = std::stoi(value);
                    else if (key == "slope_sensitivity") {
                        sensitivity = std::stof(value);
                        engine.m_slope_sensitivity = sensitivity;
                    }
                    else if (key == "slope_negative_threshold") engine.m_slope_negative_threshold = std::stof(value);
                    else if (key == "slope_smoothing_tau") engine.m_slope_smoothing_tau = std::stof(value);
                    else if (key == "slope_min_threshold") engine.m_slope_min_threshold = std::stof(value);
                    else if (key == "slope_max_threshold") {
                        engine.m_slope_max_threshold = std::stof(value);
                        has_max_threshold = true;
                    }
                    else if (key == "slope_alpha_threshold") engine.m_slope_alpha_threshold = std::stof(value);
                    else if (key == "slope_decay_rate") engine.m_slope_decay_rate = std::stof(value);
                    else if (key == "slope_confidence_enabled") engine.m_slope_confidence_enabled = (value == "1");
                    else if (key == "steering_shaft_smoothing") engine.m_steering_shaft_smoothing = std::stof(value);
                    else if (key == "gyro_smoothing_factor") engine.m_gyro_smoothing = std::stof(value);
                    else if (key == "yaw_accel_smoothing") engine.m_yaw_accel_smoothing = std::stof(value);
                    else if (key == "chassis_inertia_smoothing") engine.m_chassis_inertia_smoothing = std::stof(value);
                    else if (key == "speed_gate_lower") engine.m_speed_gate_lower = std::stof(value);
                    else if (key == "speed_gate_upper") engine.m_speed_gate_upper = std::stof(value);
                    else if (key == "road_fallback_scale") engine.m_road_fallback_scale = std::stof(value);
                    else if (key == "understeer_affects_sop") engine.m_understeer_affects_sop = std::stoi(value);
                } catch (...) {}
            }
        }
    }

    // v0.7.11 Migration: Calculate max_threshold if missing
    if (!has_max_threshold && sensitivity > 0.1f) {
        engine.m_slope_max_threshold = engine.m_slope_min_threshold - (8.0f / sensitivity);
    }

    // v0.7.16: Comprehensive Safety Validation & Clamping
    engine.m_gain = (std::max)(0.0f, engine.m_gain);
    engine.m_max_torque_ref = (std::max)(1.0f, engine.m_max_torque_ref);
    engine.m_min_force = (std::max)(0.0f, engine.m_min_force);
    engine.m_sop_scale = (std::max)(0.01f, engine.m_sop_scale);
    engine.m_slip_angle_smoothing = (std::max)(0.0001f, engine.m_slip_angle_smoothing);
    engine.m_notch_q = (std::max)(0.1f, engine.m_notch_q);
    engine.m_static_notch_width = (std::max)(0.1f, engine.m_static_notch_width);
    engine.m_speed_gate_upper = (std::max)(0.1f, engine.m_speed_gate_upper);
    engine.m_lockup_gain = (std::max)(0.0f, (std::min)(3.0f, engine.m_lockup_gain));
    engine.m_lockup_gamma = (std::max)(0.1f, engine.m_lockup_gamma);

    // v0.7.16: Safety Clamping (v0.4.50 parity)
    engine.m_slide_texture_gain = (std::min)(2.0f, engine.m_slide_texture_gain);
    engine.m_road_texture_gain = (std::min)(2.0f, engine.m_road_texture_gain);
    engine.m_spin_gain = (std::min)(2.0f, engine.m_spin_gain);
    engine.m_rear_align_effect = (std::min)(2.0f, engine.m_rear_align_effect);
    engine.m_sop_yaw_gain = (std::min)(1.0f, engine.m_sop_yaw_gain);
    engine.m_sop_effect = (std::min)(2.0f, engine.m_sop_effect);
    engine.m_scrub_drag_gain = (std::min)(1.0f, engine.m_scrub_drag_gain);
    engine.m_gyro_gain = (std::min)(1.0f, engine.m_gyro_gain);

    if (engine.m_optimal_slip_angle < 0.01f) {
        std::cerr << "[Config] Invalid optimal_slip_angle (" << engine.m_optimal_slip_angle 
                  << "), resetting to default 0.10" << std::endl;
        engine.m_optimal_slip_angle = 0.10f;
    }
    if (engine.m_optimal_slip_ratio < 0.01f) {
        std::cerr << "[Config] Invalid optimal_slip_ratio (" << engine.m_optimal_slip_ratio 
                  << "), resetting to default 0.12" << std::endl;
        engine.m_optimal_slip_ratio = 0.12f;
    }
    
    // Slope Detection Validation
    if (engine.m_slope_sg_window < 5) engine.m_slope_sg_window = 5;
    if (engine.m_slope_sg_window > 41) engine.m_slope_sg_window = 41;
    if (engine.m_slope_sg_window % 2 == 0) engine.m_slope_sg_window++;
    if (engine.m_slope_sensitivity < 0.1f) engine.m_slope_sensitivity = 0.1f;
    if (engine.m_slope_sensitivity > 10.0f) engine.m_slope_sensitivity = 10.0f;
    if (engine.m_slope_smoothing_tau < 0.001f) engine.m_slope_smoothing_tau = 0.04f;
    
    if (engine.m_slope_alpha_threshold < 0.001f || engine.m_slope_alpha_threshold > 0.1f) {
        engine.m_slope_alpha_threshold = 0.02f;
    }

    PresetRegistry::Get().Load(final_path);
}
