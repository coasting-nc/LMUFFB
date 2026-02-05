#include "Config.h"
#include "Version.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

bool Config::m_ignore_vjoy_version_warning = false;
bool Config::m_enable_vjoy = false;
bool Config::m_output_ffb_to_vjoy = false;
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

std::vector<Preset> Config::presets;

void Config::ParsePresetLine(const std::string& line, Preset& current_preset, std::string& current_preset_version, bool& needs_save) {
    std::istringstream is_line(line);
    std::string key;
    if (std::getline(is_line, key, '=')) {
        std::string value;
        if (std::getline(is_line, value)) {
            try {
                // Map keys to struct members
                if (key == "app_version") current_preset_version = value;
                else if (key == "gain") current_preset.gain = std::stof(value);
                else if (key == "understeer") {
                    float val = std::stof(value);
                    if (val > 2.0f) {
                        float old_val = val;
                        val = val / 100.0f; // Migrating 0-200 range to 0-2
                        std::cout << "[Preset] Migrated legacy understeer: " << old_val
                                    << " -> " << val << std::endl;
                        needs_save = true;
                    }
                    current_preset.understeer = (std::min)(2.0f, (std::max)(0.0f, val));
                }
                else if (key == "sop") current_preset.sop = (std::min)(2.0f, std::stof(value));
                else if (key == "sop_scale") current_preset.sop_scale = std::stof(value);
                else if (key == "sop_smoothing_factor") current_preset.sop_smoothing = std::stof(value);
                else if (key == "min_force") current_preset.min_force = std::stof(value);
                else if (key == "oversteer_boost") current_preset.oversteer_boost = std::stof(value);
                else if (key == "lockup_enabled") current_preset.lockup_enabled = std::stoi(value);
                else if (key == "lockup_gain") current_preset.lockup_gain = (std::min)(3.0f, std::stof(value));
                else if (key == "lockup_start_pct") current_preset.lockup_start_pct = std::stof(value);
                else if (key == "lockup_full_pct") current_preset.lockup_full_pct = std::stof(value);
                else if (key == "lockup_rear_boost") current_preset.lockup_rear_boost = std::stof(value);
                else if (key == "lockup_gamma") current_preset.lockup_gamma = std::stof(value);
                else if (key == "lockup_prediction_sens") current_preset.lockup_prediction_sens = std::stof(value);
                else if (key == "lockup_bump_reject") current_preset.lockup_bump_reject = std::stof(value);
                else if (key == "brake_load_cap") current_preset.brake_load_cap = (std::min)(10.0f, std::stof(value));
                else if (key == "texture_load_cap") current_preset.texture_load_cap = std::stof(value); // NEW v0.6.25
                else if (key == "max_load_factor") current_preset.texture_load_cap = std::stof(value); // Legacy Backward Compatibility
                else if (key == "abs_pulse_enabled") current_preset.abs_pulse_enabled = std::stoi(value);
                else if (key == "abs_gain") current_preset.abs_gain = std::stof(value);
                else if (key == "spin_enabled") current_preset.spin_enabled = std::stoi(value);
                else if (key == "spin_gain") current_preset.spin_gain = (std::min)(2.0f, std::stof(value));
                else if (key == "slide_enabled") current_preset.slide_enabled = std::stoi(value);
                else if (key == "slide_gain") current_preset.slide_gain = (std::min)(2.0f, std::stof(value));
                else if (key == "slide_freq") current_preset.slide_freq = std::stof(value);
                else if (key == "road_enabled") current_preset.road_enabled = std::stoi(value);
                else if (key == "road_gain") current_preset.road_gain = (std::min)(2.0f, std::stof(value));
                else if (key == "invert_force") current_preset.invert_force = std::stoi(value);
                else if (key == "max_torque_ref") current_preset.max_torque_ref = std::stof(value);
                else if (key == "abs_freq") current_preset.abs_freq = std::stof(value);
                else if (key == "lockup_freq_scale") current_preset.lockup_freq_scale = std::stof(value);
                else if (key == "spin_freq_scale") current_preset.spin_freq_scale = std::stof(value);
                else if (key == "bottoming_method") current_preset.bottoming_method = std::stoi(value);
                else if (key == "scrub_drag_gain") current_preset.scrub_drag_gain = (std::min)(1.0f, std::stof(value));
                else if (key == "rear_align_effect") current_preset.rear_align_effect = (std::min)(2.0f, std::stof(value));
                else if (key == "sop_yaw_gain") current_preset.sop_yaw_gain = (std::min)(2.0f, std::stof(value));
                else if (key == "steering_shaft_gain") current_preset.steering_shaft_gain = std::stof(value);
                else if (key == "slip_angle_smoothing") current_preset.slip_smoothing = std::stof(value);
                else if (key == "base_force_mode") current_preset.base_force_mode = std::stoi(value);
                else if (key == "gyro_gain") current_preset.gyro_gain = (std::min)(1.0f, std::stof(value));
                else if (key == "flatspot_suppression") current_preset.flatspot_suppression = std::stoi(value);
                else if (key == "notch_q") current_preset.notch_q = std::stof(value);
                else if (key == "flatspot_strength") current_preset.flatspot_strength = std::stof(value);
                else if (key == "static_notch_enabled") current_preset.static_notch_enabled = std::stoi(value);
                else if (key == "static_notch_freq") current_preset.static_notch_freq = std::stof(value);
                else if (key == "static_notch_width") current_preset.static_notch_width = std::stof(value);
                else if (key == "yaw_kick_threshold") current_preset.yaw_kick_threshold = std::stof(value);
                else if (key == "optimal_slip_angle") current_preset.optimal_slip_angle = std::stof(value);
                else if (key == "optimal_slip_ratio") current_preset.optimal_slip_ratio = std::stof(value);
                else if (key == "slope_detection_enabled") current_preset.slope_detection_enabled = (value == "1");
                else if (key == "slope_sg_window") current_preset.slope_sg_window = std::stoi(value);
                else if (key == "slope_sensitivity") current_preset.slope_sensitivity = std::stof(value);
                else if (key == "slope_negative_threshold") current_preset.slope_negative_threshold = std::stof(value);
                else if (key == "slope_smoothing_tau") current_preset.slope_smoothing_tau = std::stof(value);
                else if (key == "slope_min_threshold") current_preset.slope_min_threshold = std::stof(value);
                else if (key == "slope_max_threshold") current_preset.slope_max_threshold = std::stof(value);
                else if (key == "slope_alpha_threshold") current_preset.slope_alpha_threshold = std::stof(value);
                else if (key == "slope_decay_rate") current_preset.slope_decay_rate = std::stof(value);
                else if (key == "slope_confidence_enabled") current_preset.slope_confidence_enabled = (value == "1");
                else if (key == "steering_shaft_smoothing") current_preset.steering_shaft_smoothing = std::stof(value);
                else if (key == "gyro_smoothing_factor") current_preset.gyro_smoothing = std::stof(value);
                else if (key == "yaw_accel_smoothing") current_preset.yaw_smoothing = std::stof(value);
                else if (key == "chassis_inertia_smoothing") current_preset.chassis_smoothing = std::stof(value);
                else if (key == "speed_gate_lower") current_preset.speed_gate_lower = std::stof(value); // NEW v0.6.25
                else if (key == "speed_gate_upper") current_preset.speed_gate_upper = std::stof(value); // NEW v0.6.25
                else if (key == "road_fallback_scale") current_preset.road_fallback_scale = std::stof(value); // NEW v0.6.25
                else if (key == "understeer_affects_sop") current_preset.understeer_affects_sop = std::stoi(value); // NEW v0.6.25
            } catch (...) {}
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
        p.invert_force = true;
        p.gain = 1.0f;
        p.max_torque_ref = 100.1f;
        p.min_force = 0.01f;
        p.steering_shaft_gain = 1.0f;
        p.steering_shaft_smoothing = 0.0f;
        p.understeer = 0.5f;
        p.base_force_mode = 0;
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
        p.sop_smoothing = 1.0f;
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
        p.max_torque_ref = 100.0f;
        p.min_force = 0.0f;
        p.steering_shaft_gain = 1.0f;
        p.steering_shaft_smoothing = 0.0f;
        p.understeer = 1.0f;
        p.base_force_mode = 0;
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
        p.sop_smoothing = 0.99f;
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
        p.max_torque_ref = 100.0f;
        p.min_force = 0.0f;
        p.steering_shaft_gain = 1.0f;
        p.steering_shaft_smoothing = 0.0f;
        p.understeer = 1.0f;
        p.base_force_mode = 0;
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
        p.sop_smoothing = 0.97f;
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
        p.max_torque_ref = 100.1f;
        p.min_force = 0.0f;
        p.steering_shaft_gain = 1.989f;
        p.steering_shaft_smoothing = 0.0f;
        p.understeer = 0.638f;
        p.base_force_mode = 0;
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
        p.max_torque_ref = 100.1f;
        p.min_force = 0.0f;
        p.steering_shaft_gain = 1.989f;
        p.steering_shaft_smoothing = 0.0f;
        p.understeer = 0.638f;
        p.base_force_mode = 0;
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
        .SetSmoothing(0.85f)
        .SetSlipSmoothing(0.015f)
        .SetSlide(false, 0.0f)
        .SetRearAlign(0.0f)
    );

    // 9. Test: SoP Only
    presets.push_back(Preset("Test: SoP Only", true)
        .SetUndersteer(0.0f)
        .SetSoP(0.08f)
        .SetSoPScale(1.0f)
        .SetSmoothing(0.85f)
        .SetSlipSmoothing(0.015f)
        .SetSlide(false, 0.0f)
        .SetRearAlign(0.0f)
        .SetSoPYaw(0.0f)
        .SetBaseMode(2) // Muted
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
        .SetSmoothing(0.85f)         // SoP smoothing (doesn't affect test since SoP=0)
        .SetSlipSmoothing(0.015f)    // Slip angle smoothing (important for grip calculation)
        
        // PHYSICS PARAMETERS (Explicit for clarity and future-proofing)
        .SetOptimalSlip(0.10f, 0.12f)  // Explicit optimal slip thresholds
        .SetBaseMode(0)                 // Native physics mode (required for understeer)
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
        .SetSmoothing(0.85f)
        .SetSlipSmoothing(0.015f)
        
        // BASE MODE
        .SetBaseMode(2)  // Muted: Feel only the yaw kick impulse
    );

    // 12. Test: Textures Only
    presets.push_back(Preset("Test: Textures Only", true)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetSoPScale(0.0f)
        .SetSmoothing(0.85f)
        .SetSlipSmoothing(0.015f)
        .SetLockup(true, 1.0f)
        .SetSpin(true, 1.0f)
        .SetSlide(true, 0.39f)
        .SetRoad(true, 1.0f)
        .SetRearAlign(0.0f)
        .SetBaseMode(2) // Muted
    );

    // 13. Test: Rear Align Torque Only
    presets.push_back(Preset("Test: Rear Align Torque Only", true)
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetSmoothing(0.85f)
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
        .SetSmoothing(0.85f)
        .SetSlipSmoothing(0.015f)
        .SetSlide(false, 0.0f)
        .SetRearAlign(0.0f)
        .SetSoPYaw(0.0f)
        .SetBaseMode(2) // Muted
    );

    // 15. Test: Slide Texture Only
    presets.push_back(Preset("Test: Slide Texture Only", true)
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetSmoothing(0.85f)
        .SetSlipSmoothing(0.015f)
        .SetSlide(true, 0.39f, 1.0f)
        .SetRearAlign(0.0f)
        .SetBaseMode(2) // Muted
    );

    // 16. Test: No Effects
    presets.push_back(Preset("Test: No Effects", true)
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetSmoothing(0.85f)
        .SetSlipSmoothing(0.015f)
        .SetSlide(false, 0.0f)
        .SetRearAlign(0.0f)
        .SetBaseMode(2) // Muted
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
        .SetSmoothing(0.85f)
        .SetSlipSmoothing(0.015f)
        .SetBaseMode(0) // Native Physics needed to feel the drop
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
        .SetSmoothing(0.85f)
        .SetSlipSmoothing(0.015f)
        .SetBaseMode(0) // Native Physics + Boost
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
        .SetSmoothing(0.85f)
        .SetSlipSmoothing(0.015f)
        .SetBaseMode(2) // Muted for clear texture feel
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
        .SetSmoothing(0.85f)
        .SetSlipSmoothing(0.015f)
        .SetBaseMode(2) // Muted
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
        .SetSmoothing(0.85f)
        .SetSlipSmoothing(0.015f)
        .SetBaseMode(2) // Muted
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
        .SetSmoothing(0.85f)
        .SetSlipSmoothing(0.015f)
        .SetBaseMode(2) // Muted: Feel only the rotation impulse
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
        .SetSmoothing(0.85f)
        .SetSlipSmoothing(0.015f)
        .SetBaseMode(2) // Muted: Feel only the resistance to movement
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

    while (std::getline(file, line)) {
        // Strip whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty() || line[0] == ';') continue;

        if (line[0] == '[') {
            if (preset_pending && !current_preset_name.empty()) {
                current_preset.name = current_preset_name;
                current_preset.is_builtin = false; // User preset
                
                // MIGRATION: If version is missing or old, update it
                if (current_preset_version.empty()) {
                    current_preset.app_version = LMUFFB_VERSION;
                    needs_save = true;
                    std::cout << "[Config] Migrated legacy preset '" << current_preset_name << "' to version " << LMUFFB_VERSION << std::endl;
                } else {
                    current_preset.app_version = current_preset_version;
                }
                
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
                }
            } else {
                in_presets = false;
            }
            continue;
        }

        if (preset_pending) {
            ParsePresetLine(line, current_preset, current_preset_version, needs_save);
        }
    }
    
    if (preset_pending && !current_preset_name.empty()) {
        current_preset.name = current_preset_name;
        current_preset.is_builtin = false;
        
        // MIGRATION: If version is missing or old, update it
        if (current_preset_version.empty()) {
            current_preset.app_version = LMUFFB_VERSION;
            needs_save = true;
            std::cout << "[Config] Migrated legacy preset '" << current_preset_name << "' to version " << LMUFFB_VERSION << std::endl;
        } else {
            current_preset.app_version = current_preset_version;
        }
        
        presets.push_back(current_preset);
    }

    // Auto-save if migration occurred
    if (needs_save) {
        FFBEngine temp_engine; // Just to satisfy the Save signature
        // We might want a version of Save that doesn't overwrite current engine settings
        // but for now, the plan says "call Config::SaveManualPresetsOnly() (or similar)".
        // Looking at Save(), it saves everything. 
        // If we just loaded presets, we haven't applied them to any engine yet.
        // But Config::Save takes an engine.
        // Actually, if we just want to update the presets on disk, we should call Save.
        Save(temp_engine);
    }
}

void Config::ApplyPreset(int index, FFBEngine& engine) {
    if (index >= 0 && index < presets.size()) {
        presets[index].Apply(engine);
        m_last_preset_name = presets[index].name;
        std::cout << "[Config] Applied preset: " << presets[index].name << std::endl;
        Save(engine); // Integrated Auto-Save (v0.6.27)
    }
}

void Config::WritePresetFields(std::ofstream& file, const Preset& p) {
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

void Config::ExportPreset(int index, const std::string& filename) {
    if (index < 0 || index >= presets.size()) return;

    const Preset& p = presets[index];
    std::ofstream file(filename);
    if (file.is_open()) {
        file << "[Preset:" << p.name << "]\n";
        WritePresetFields(file, p);
        file.close();
        std::cout << "[Config] Exported preset '" << p.name << "' to " << filename << std::endl;
    } else {
        std::cerr << "[Config] Failed to export preset to " << filename << std::endl;
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
                }
            }
            continue;
        }

        if (preset_pending) {
            bool dummy_needs_save = false;
            ParsePresetLine(line, current_preset, current_preset_version, dummy_needs_save);
        }
    }

    if (preset_pending && !current_preset_name.empty()) {
        current_preset.name = current_preset_name;
        current_preset.is_builtin = false;
        current_preset.app_version = current_preset_version.empty() ? LMUFFB_VERSION : current_preset_version;

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

        presets.push_back(current_preset);
        imported = true;
    }

    if (imported) {
        Save(engine);
        std::cout << "[Config] Imported preset '" << current_preset.name << "' from " << filename << std::endl;
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
    std::cout << "[Config] Deleted preset: " << name << std::endl;

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
    std::cout << "[Config] Duplicated preset to: " << p.name << std::endl;
    Save(engine);
}

bool Config::IsEngineDirtyRelativeToPreset(int index, const FFBEngine& engine) {
    // ⚠ IMPORTANT MAINTENANCE WARNING:
    // When adding new FFB parameters to the FFBEngine class or Preset struct,
    // you MUST add a comparison check here to ensure the "dirty" flag (*)
    // appears correctly in the GUI.

    if (index < 0 || index >= (int)presets.size()) return false;

    const Preset& p = presets[index];
    const float eps = 0.0001f;

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

void Config::Save(const FFBEngine& engine, const std::string& filename) {
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
        file << "ignore_vjoy_version_warning=" << m_ignore_vjoy_version_warning << "\n";
        file << "enable_vjoy=" << m_enable_vjoy << "\n";
        file << "output_ffb_to_vjoy=" << m_output_ffb_to_vjoy << "\n";
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
        std::cerr << "[Config] Failed to save to " << final_path << std::endl;
    }
}

void Config::Load(FFBEngine& engine, const std::string& filename) {
    std::string final_path = filename.empty() ? m_config_path : filename;
    std::ifstream file(final_path);
    if (!file.is_open()) {
        std::cout << "[Config] No config found, using defaults." << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Strip whitespace and check for section headers
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        if (line.empty() || line[0] == ';') continue;
        if (line[0] == '[') break; // Top-level settings end here (e.g. [Presets])

        std::istringstream is_line(line);
        std::string key;
        if (std::getline(is_line, key, '=')) {
            std::string value;
            if (std::getline(is_line, value)) {
                try {
                    if (key == "ini_version") {
                        // Config Version Tracking: This field records the app version that last saved the config.
                        // It serves as an implicit config format version for migration decisions.
                        // Current approach: Threshold-based detection (e.g., understeer > 2.0 = legacy format).
                        // Future improvement: Add explicit config_format_version field if migrations become
                        // more complex (e.g., structural changes, removed fields, renamed keys).
                        std::string config_version = value;
                        std::cout << "[Config] Loading config version: " << config_version << std::endl;
                    }
                    else if (key == "ignore_vjoy_version_warning") m_ignore_vjoy_version_warning = std::stoi(value);
                    else if (key == "enable_vjoy") m_enable_vjoy = std::stoi(value);
                    else if (key == "output_ffb_to_vjoy") m_output_ffb_to_vjoy = std::stoi(value);
                    else if (key == "always_on_top") m_always_on_top = std::stoi(value);
                    else if (key == "last_device_guid") m_last_device_guid = value;
                    else if (key == "last_preset_name") m_last_preset_name = value;
                    // Window Geometry (v0.5.5)
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
                    else if (key == "max_load_factor") engine.m_texture_load_cap = std::stof(value); // Legacy Backward Compatibility
                    else if (key == "brake_load_cap") engine.m_brake_load_cap = std::stof(value);
                    else if (key == "smoothing") engine.m_sop_smoothing_factor = std::stof(value); // Legacy support
                    else if (key == "understeer") engine.m_understeer_effect = std::stof(value);
                    else if (key == "sop") engine.m_sop_effect = std::stof(value);
                    else if (key == "min_force") engine.m_min_force = std::stof(value);
                    else if (key == "oversteer_boost") engine.m_oversteer_boost = std::stof(value);
                    // v0.4.50: SAFETY CLAMPING for Generator Effects (Gain Compensation Migration)
                    // Legacy configs may have high gains (e.g., 5.0) to compensate for lack of auto-scaling.
                    // With new decoupling, these would cause 25x force explosions. Clamp to safe maximums.
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
                    else if (key == "scrub_drag_gain") engine.m_scrub_drag_gain = (std::min)(1.0f, std::stof(value));
                    else if (key == "rear_align_effect") engine.m_rear_align_effect = std::stof(value);
                    else if (key == "sop_yaw_gain") engine.m_sop_yaw_gain = std::stof(value);
                    else if (key == "steering_shaft_gain") engine.m_steering_shaft_gain = std::stof(value);
                    else if (key == "base_force_mode") engine.m_base_force_mode = std::stoi(value);
                    else if (key == "gyro_gain") engine.m_gyro_gain = (std::min)(1.0f, std::stof(value));
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
                    else if (key == "slope_sensitivity") engine.m_slope_sensitivity = std::stof(value);
                    else if (key == "slope_negative_threshold") engine.m_slope_negative_threshold = std::stof(value);
                    else if (key == "slope_smoothing_tau") engine.m_slope_smoothing_tau = std::stof(value);
                    else if (key == "slope_min_threshold") engine.m_slope_min_threshold = std::stof(value);
                    else if (key == "slope_max_threshold") engine.m_slope_max_threshold = std::stof(value);
                    else if (key == "slope_alpha_threshold") engine.m_slope_alpha_threshold = std::stof(value);
                    else if (key == "slope_decay_rate") engine.m_slope_decay_rate = std::stof(value);
                    else if (key == "slope_confidence_enabled") engine.m_slope_confidence_enabled = (value == "1");
                    else if (key == "steering_shaft_smoothing") engine.m_steering_shaft_smoothing = std::stof(value);
                    else if (key == "gyro_smoothing_factor") engine.m_gyro_smoothing = std::stof(value);
                    else if (key == "yaw_accel_smoothing") engine.m_yaw_accel_smoothing = std::stof(value);
                    else if (key == "chassis_inertia_smoothing") engine.m_chassis_inertia_smoothing = std::stof(value);
                    else if (key == "speed_gate_lower") engine.m_speed_gate_lower = std::stof(value); // NEW v0.6.25
                    else if (key == "speed_gate_upper") engine.m_speed_gate_upper = std::stof(value); // NEW v0.6.25
                    else if (key == "road_fallback_scale") engine.m_road_fallback_scale = std::stof(value); // NEW v0.6.25
                    else if (key == "understeer_affects_sop") engine.m_understeer_affects_sop = std::stoi(value); // NEW v0.6.25
                } catch (...) {
                    std::cerr << "[Config] Error parsing line: " << line << std::endl;
                }
            }
        }
    }
    
    // v0.5.7: Safety Validation - Prevent Division by Zero in Grip Calculation
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
    
    // Slope Detection Validation (v0.7.0)
    if (engine.m_slope_sg_window < 5) engine.m_slope_sg_window = 5;
    if (engine.m_slope_sg_window > 41) engine.m_slope_sg_window = 41;
    if (engine.m_slope_sg_window % 2 == 0) engine.m_slope_sg_window++; // Must be odd
    if (engine.m_slope_sensitivity < 0.1f) engine.m_slope_sensitivity = 0.1f;
    if (engine.m_slope_sensitivity > 10.0f) engine.m_slope_sensitivity = 10.0f;
    if (engine.m_slope_smoothing_tau < 0.001f) engine.m_slope_smoothing_tau = 0.04f;
    
    // v0.7.3: Slope stability parameter validation
    if (engine.m_slope_alpha_threshold < 0.001f || engine.m_slope_alpha_threshold > 0.1f) {
        std::cerr << "[Config] Invalid slope_alpha_threshold (" << engine.m_slope_alpha_threshold 
                  << "), resetting to 0.02f" << std::endl;
        engine.m_slope_alpha_threshold = 0.02f; // Safe default
    }
    if (engine.m_slope_decay_rate < 0.5f || engine.m_slope_decay_rate > 20.0f) {
        std::cerr << "[Config] Invalid slope_decay_rate (" << engine.m_slope_decay_rate 
                  << "), resetting to 5.0f" << std::endl;
        engine.m_slope_decay_rate = 5.0f; // Safe default
    }

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
            std::cout << "[Config] Migrated slope_sensitivity " << sens 
                      << " to max_threshold " << engine.m_slope_max_threshold << std::endl;
        }
    }

    // Validation: max should be more negative than min
    if (engine.m_slope_max_threshold > engine.m_slope_min_threshold) {
        std::swap(engine.m_slope_min_threshold, engine.m_slope_max_threshold);
        std::cout << "[Config] Swapped slope thresholds (min should be > max)" << std::endl;
    }
    
    // v0.6.20: Safety Validation - Clamp Advanced Braking Parameters to Valid Ranges (Expanded)
    if (engine.m_lockup_gamma < 0.1f || engine.m_lockup_gamma > 3.0f) {
        std::cerr << "[Config] Invalid lockup_gamma (" << engine.m_lockup_gamma 
                  << "), clamping to range [0.1, 3.0]" << std::endl;
        engine.m_lockup_gamma = (std::max)(0.1f, (std::min)(3.0f, engine.m_lockup_gamma));
    }
    if (engine.m_lockup_prediction_sens < 10.0f || engine.m_lockup_prediction_sens > 100.0f) {
        std::cerr << "[Config] Invalid lockup_prediction_sens (" << engine.m_lockup_prediction_sens 
                  << "), clamping to range [10.0, 100.0]" << std::endl;
        engine.m_lockup_prediction_sens = (std::max)(10.0f, (std::min)(100.0f, engine.m_lockup_prediction_sens));
    }
    if (engine.m_lockup_bump_reject < 0.1f || engine.m_lockup_bump_reject > 5.0f) {
        std::cerr << "[Config] Invalid lockup_bump_reject (" << engine.m_lockup_bump_reject 
                  << "), clamping to range [0.1, 5.0]" << std::endl;
        engine.m_lockup_bump_reject = (std::max)(0.1f, (std::min)(5.0f, engine.m_lockup_bump_reject));
    }
    if (engine.m_abs_gain < 0.0f || engine.m_abs_gain > 10.0f) {
        std::cerr << "[Config] Invalid abs_gain (" << engine.m_abs_gain 
                  << "), clamping to range [0.0, 10.0]" << std::endl;
        engine.m_abs_gain = (std::max)(0.0f, (std::min)(10.0f, engine.m_abs_gain));
    }
    // Legacy Migration: Convert 0-200 range to 0-2.0 range
    if (engine.m_understeer_effect > 2.0f) {
        float old_val = engine.m_understeer_effect;
        engine.m_understeer_effect = engine.m_understeer_effect / 100.0f;
        std::cout << "[Config] Migrated legacy understeer_effect: " << old_val 
                  << " -> " << engine.m_understeer_effect << std::endl;
    }
    // Clamp to new valid range [0.0, 2.0]
    if (engine.m_understeer_effect < 0.0f || engine.m_understeer_effect > 2.0f) {
        engine.m_understeer_effect = (std::max)(0.0f, (std::min)(2.0f, engine.m_understeer_effect));
    }
    if (engine.m_steering_shaft_gain < 0.0f || engine.m_steering_shaft_gain > 2.0f) {
        engine.m_steering_shaft_gain = (std::max)(0.0f, (std::min)(2.0f, engine.m_steering_shaft_gain));
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
    std::cout << "[Config] Loaded from " << filename << std::endl;
}









