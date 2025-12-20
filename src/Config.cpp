#include "Config.h"
#include <fstream>
#include <sstream>
#include <iostream>

bool Config::m_ignore_vjoy_version_warning = false;
bool Config::m_enable_vjoy = false;
bool Config::m_output_ffb_to_vjoy = false;

std::vector<Preset> Config::presets;

void Config::LoadPresets() {
    presets.clear();
    
    // 1. Default (Uses the defaults defined in Config.h)
    presets.push_back(Preset("Default", true));
    
    // 2. T300 (User Tuned)
    // Tuned for Thrustmaster T300RS with 100Nm Reference.
    // Boosts effects by ~10x to compensate for the high reference.
    presets.push_back(Preset("T300", true)
        .SetGain(1.0f)
        .SetShaftGain(1.0f)
        .SetMinForce(0.0f)
        .SetMaxTorque(100.0f)    // High ref to prevent clipping
        .SetInvert(true)
        .SetUndersteer(38.0f)    // Grip Drop
        .SetSoP(1.0f)            // Lateral G (Weight)
        .SetRearAlign(5.0f)     // Counter-Steer Torque (The "Pull")
        .SetOversteer(1.0f)      // Boost when rear slips
        .SetSoPYaw(5.0f)         // Kick on rotation start
        .SetGyro(0.0f)
        .SetLockup(false, 0.0f)
        .SetSpin(false, 0.0f)
        .SetSlide(false, 0.0f)
        .SetRoad(false, 0.0f)
        .SetScrub(0.0f)
        .SetBaseMode(0)
    );
    
    // 3. Test: Game Base FFB Only
    presets.push_back(Preset("Test: Game Base FFB Only", true)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetSoPScale(5.0f)
        .SetSmoothing(0.0f)
        .SetSlide(false, 0.0f)
        .SetRearAlign(0.0f)
    );

    // 4. Test: SoP Only
    presets.push_back(Preset("Test: SoP Only", true)
        .SetUndersteer(0.0f)
        .SetSoP(1.0f)
        .SetSoPScale(5.0f)
        .SetSmoothing(0.0f)
        .SetSlide(false, 0.0f)
        .SetRearAlign(0.0f)
        .SetSoPYaw(0.0f)
        .SetBaseMode(2) // Muted
    );

    // 5. Test: Understeer Only
    presets.push_back(Preset("Test: Understeer Only", true)
        .SetUndersteer(1.0f)
        .SetSoP(0.0f)
        .SetSoPScale(0.0f)
        .SetSmoothing(0.0f)
        .SetSlide(false, 0.0f)
        .SetRearAlign(0.0f)
    );

    // 6. Test: Textures Only
    presets.push_back(Preset("Test: Textures Only", true)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetSoPScale(0.0f)
        .SetSmoothing(0.0f)
        .SetLockup(true, 1.0f)
        .SetSpin(true, 1.0f)
        .SetSlide(true, 1.0f)
        .SetRoad(true, 1.0f)
        .SetRearAlign(0.0f)
        .SetBaseMode(2) // Muted
    );

    // 7. Test: Rear Align Torque Only
    presets.push_back(Preset("Test: Rear Align Torque Only", true)
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetSmoothing(0.0f)
        .SetSlide(false, 0.0f)
        .SetRearAlign(1.0f)
        .SetSoPYaw(0.0f)
    );

    // 8. Test: SoP Base Only
    presets.push_back(Preset("Test: SoP Base Only", true)
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(1.0f)
        .SetSmoothing(0.0f)
        .SetSlide(false, 0.0f)
        .SetRearAlign(0.0f)
        .SetSoPYaw(0.0f)
        .SetBaseMode(2) // Muted
    );

    // 9. Test: Slide Texture Only
    presets.push_back(Preset("Test: Slide Texture Only", true)
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetSmoothing(0.0f)
        .SetSlide(true, 1.0f)
        .SetRearAlign(0.0f)
        .SetBaseMode(2) // Muted
    );

    // 10. Test: No Effects
    presets.push_back(Preset("Test: No Effects", true)
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetSmoothing(0.0f)
        .SetSlide(false, 0.0f)
        .SetRearAlign(0.0f)
        .SetBaseMode(2) // Muted
    );

    // --- NEW GUIDE PRESETS (v0.4.24) ---

    // 11. Guide: Understeer (Front Grip Loss)
    presets.push_back(Preset("Guide: Understeer (Front Grip)", true)
        .SetGain(1.0f)
        .SetUndersteer(1.0f)
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
        .SetBaseMode(0) // Native Physics needed to feel the drop
    );

    // 12. Guide: Oversteer (Rear Grip Loss)
    presets.push_back(Preset("Guide: Oversteer (Rear Grip)", true)
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(1.0f)
        .SetSoPScale(20.0f)
        .SetRearAlign(1.0f)
        .SetOversteer(1.0f)
        .SetSoPYaw(0.0f)
        .SetGyro(0.0f)
        .SetLockup(false, 0.0f)
        .SetSpin(false, 0.0f)
        .SetSlide(false, 0.0f)
        .SetRoad(false, 0.0f)
        .SetScrub(0.0f)
        .SetBaseMode(0) // Native Physics + Boost
    );

    // 13. Guide: Slide Texture (Scrubbing)
    presets.push_back(Preset("Guide: Slide Texture (Scrub)", true)
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetOversteer(0.0f)
        .SetRearAlign(0.0f)
        .SetSlide(true, 1.0f)
        .SetScrub(1.0f)
        .SetLockup(false, 0.0f)
        .SetSpin(false, 0.0f)
        .SetRoad(false, 0.0f)
        .SetBaseMode(2) // Muted for clear texture feel
    );

    // 14. Guide: Braking Lockup
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
        .SetBaseMode(2) // Muted
    );

    // 15. Guide: Traction Loss (Wheel Spin)
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
        .SetBaseMode(2) // Muted
    );

     // 16. Guide: SoP Yaw (Kick)
    presets.push_back(Preset("Guide: SoP Yaw (Kick)", true)
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetOversteer(0.0f)
        .SetRearAlign(0.0f)
        .SetSoPYaw(2.0f) // Max gain to make the impulse obvious
        .SetGyro(0.0f)
        .SetLockup(false, 0.0f)
        .SetSpin(false, 0.0f)
        .SetSlide(false, 0.0f)
        .SetRoad(false, 0.0f)
        .SetScrub(0.0f)
        .SetBaseMode(2) // Muted: Feel only the rotation impulse
    );

    // 17. Guide: Gyroscopic Damping
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
        .SetBaseMode(2) // Muted: Feel only the resistance to movement
    );

    // --- Parse User Presets from config.ini ---
    // (Keep the existing parsing logic below, it works fine for file I/O)
    std::ifstream file("config.ini");
    if (!file.is_open()) return;

    std::string line;
    bool in_presets = false;
    
    std::string current_preset_name = "";
    Preset current_preset; // Uses default constructor with default values
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
                }
            } else {
                in_presets = false;
            }
            continue;
        }

        if (preset_pending) {
            std::istringstream is_line(line);
            std::string key;
            if (std::getline(is_line, key, '=')) {
                std::string value;
                if (std::getline(is_line, value)) {
                    try {
                        // Map keys to struct members
                        if (key == "gain") current_preset.gain = std::stof(value);
                        else if (key == "understeer") current_preset.understeer = std::stof(value);
                        else if (key == "sop") current_preset.sop = std::stof(value);
                        else if (key == "sop_scale") current_preset.sop_scale = std::stof(value);
                        else if (key == "sop_smoothing_factor") current_preset.sop_smoothing = std::stof(value);
                        else if (key == "min_force") current_preset.min_force = std::stof(value);
                        else if (key == "oversteer_boost") current_preset.oversteer_boost = std::stof(value);
                        else if (key == "lockup_enabled") current_preset.lockup_enabled = std::stoi(value);
                        else if (key == "lockup_gain") current_preset.lockup_gain = std::stof(value);
                        else if (key == "spin_enabled") current_preset.spin_enabled = std::stoi(value);
                        else if (key == "spin_gain") current_preset.spin_gain = std::stof(value);
                        else if (key == "slide_enabled") current_preset.slide_enabled = std::stoi(value);
                        else if (key == "slide_gain") current_preset.slide_gain = std::stof(value);
                        else if (key == "road_enabled") current_preset.road_enabled = std::stoi(value);
                        else if (key == "road_gain") current_preset.road_gain = std::stof(value);
                        else if (key == "invert_force") current_preset.invert_force = std::stoi(value);
                        else if (key == "max_torque_ref") current_preset.max_torque_ref = std::stof(value);
                        else if (key == "use_manual_slip") current_preset.use_manual_slip = std::stoi(value);
                        else if (key == "bottoming_method") current_preset.bottoming_method = std::stoi(value);
                        else if (key == "scrub_drag_gain") current_preset.scrub_drag_gain = std::stof(value);
                        else if (key == "rear_align_effect") current_preset.rear_align_effect = std::stof(value);
                        else if (key == "sop_yaw_gain") current_preset.sop_yaw_gain = std::stof(value);
                        else if (key == "steering_shaft_gain") current_preset.steering_shaft_gain = std::stof(value);
                        else if (key == "base_force_mode") current_preset.base_force_mode = std::stoi(value);
                        else if (key == "gyro_gain") current_preset.gyro_gain = std::stof(value);
                    } catch (...) {}
                }
            }
        }
    }
    
    if (preset_pending && !current_preset_name.empty()) {
        current_preset.name = current_preset_name;
        current_preset.is_builtin = false;
        presets.push_back(current_preset);
    }
}

void Config::ApplyPreset(int index, FFBEngine& engine) {
    if (index >= 0 && index < presets.size()) {
        presets[index].Apply(engine);
        std::cout << "[Config] Applied preset: " << presets[index].name << std::endl;
    }
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
    
    // Save immediately to persist
    Save(engine);
}

void Config::Save(const FFBEngine& engine, const std::string& filename) {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << "ignore_vjoy_version_warning=" << m_ignore_vjoy_version_warning << "\n";
        file << "enable_vjoy=" << m_enable_vjoy << "\n";
        file << "output_ffb_to_vjoy=" << m_output_ffb_to_vjoy << "\n";
        file << "gain=" << engine.m_gain << "\n";
        file << "sop_smoothing_factor=" << engine.m_sop_smoothing_factor << "\n";
        file << "sop_scale=" << engine.m_sop_scale << "\n";
        file << "max_load_factor=" << engine.m_max_load_factor << "\n";
        file << "understeer=" << engine.m_understeer_effect << "\n";
        file << "sop=" << engine.m_sop_effect << "\n";
        file << "min_force=" << engine.m_min_force << "\n";
        file << "oversteer_boost=" << engine.m_oversteer_boost << "\n";
        file << "lockup_enabled=" << engine.m_lockup_enabled << "\n";
        file << "lockup_gain=" << engine.m_lockup_gain << "\n";
        file << "spin_enabled=" << engine.m_spin_enabled << "\n";
        file << "spin_gain=" << engine.m_spin_gain << "\n";
        file << "slide_enabled=" << engine.m_slide_texture_enabled << "\n";
        file << "slide_gain=" << engine.m_slide_texture_gain << "\n";
        file << "road_enabled=" << engine.m_road_texture_enabled << "\n";
        file << "road_gain=" << engine.m_road_texture_gain << "\n";
        file << "invert_force=" << engine.m_invert_force << "\n";
        file << "max_torque_ref=" << engine.m_max_torque_ref << "\n";
        file << "use_manual_slip=" << engine.m_use_manual_slip << "\n";
        file << "bottoming_method=" << engine.m_bottoming_method << "\n";
        file << "scrub_drag_gain=" << engine.m_scrub_drag_gain << "\n";
        file << "rear_align_effect=" << engine.m_rear_align_effect << "\n";
        file << "sop_yaw_gain=" << engine.m_sop_yaw_gain << "\n";
        file << "steering_shaft_gain=" << engine.m_steering_shaft_gain << "\n";
        file << "base_force_mode=" << engine.m_base_force_mode << "\n";
        file << "gyro_gain=" << engine.m_gyro_gain << "\n";
        
        // 3. User Presets
        file << "\n[Presets]\n";
        for (const auto& p : presets) {
            if (!p.is_builtin) {
                file << "[Preset:" << p.name << "]\n";
                file << "gain=" << p.gain << "\n";
                file << "understeer=" << p.understeer << "\n";
                file << "sop=" << p.sop << "\n";
                file << "sop_scale=" << p.sop_scale << "\n";
                file << "sop_smoothing_factor=" << p.sop_smoothing << "\n";
                file << "min_force=" << p.min_force << "\n";
                file << "oversteer_boost=" << p.oversteer_boost << "\n";
                file << "lockup_enabled=" << p.lockup_enabled << "\n";
                file << "lockup_gain=" << p.lockup_gain << "\n";
                file << "spin_enabled=" << p.spin_enabled << "\n";
                file << "spin_gain=" << p.spin_gain << "\n";
                file << "slide_enabled=" << p.slide_enabled << "\n";
                file << "slide_gain=" << p.slide_gain << "\n";
                file << "road_enabled=" << p.road_enabled << "\n";
                file << "road_gain=" << p.road_gain << "\n";
                file << "invert_force=" << p.invert_force << "\n";
                file << "max_torque_ref=" << p.max_torque_ref << "\n";
                file << "use_manual_slip=" << p.use_manual_slip << "\n";
                file << "bottoming_method=" << p.bottoming_method << "\n";
                file << "scrub_drag_gain=" << p.scrub_drag_gain << "\n";
                file << "rear_align_effect=" << p.rear_align_effect << "\n";
                file << "sop_yaw_gain=" << p.sop_yaw_gain << "\n";
                file << "steering_shaft_gain=" << p.steering_shaft_gain << "\n";
                file << "base_force_mode=" << p.base_force_mode << "\n";
                file << "gyro_gain=" << p.gyro_gain << "\n";
                file << "\n";
            }
        }
        
        file.close();
        std::cout << "[Config] Saved to " << filename << std::endl;
    } else {
        std::cerr << "[Config] Failed to save to " << filename << std::endl;
    }
}

void Config::Load(FFBEngine& engine, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "[Config] No config found, using defaults." << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream is_line(line);
        std::string key;
        if (std::getline(is_line, key, '=')) {
            std::string value;
            if (std::getline(is_line, value)) {
                try {
                    if (key == "ignore_vjoy_version_warning") m_ignore_vjoy_version_warning = std::stoi(value);
                    else if (key == "enable_vjoy") m_enable_vjoy = std::stoi(value);
                    else if (key == "output_ffb_to_vjoy") m_output_ffb_to_vjoy = std::stoi(value);
                    else if (key == "gain") engine.m_gain = std::stof(value);
                    else if (key == "sop_smoothing_factor") engine.m_sop_smoothing_factor = std::stof(value);
                    else if (key == "sop_scale") engine.m_sop_scale = std::stof(value);
                    else if (key == "max_load_factor") engine.m_max_load_factor = std::stof(value);
                    else if (key == "smoothing") engine.m_sop_smoothing_factor = std::stof(value); // Legacy support
                    else if (key == "understeer") engine.m_understeer_effect = std::stof(value);
                    else if (key == "sop") engine.m_sop_effect = std::stof(value);
                    else if (key == "min_force") engine.m_min_force = std::stof(value);
                    else if (key == "oversteer_boost") engine.m_oversteer_boost = std::stof(value);
                    else if (key == "lockup_enabled") engine.m_lockup_enabled = std::stoi(value);
                    else if (key == "lockup_gain") engine.m_lockup_gain = std::stof(value);
                    else if (key == "spin_enabled") engine.m_spin_enabled = std::stoi(value);
                    else if (key == "spin_gain") engine.m_spin_gain = std::stof(value);
                    else if (key == "oversteer_boost") engine.m_oversteer_boost = std::stof(value);
                    else if (key == "lockup_enabled") engine.m_lockup_enabled = std::stoi(value);
                    else if (key == "lockup_gain") engine.m_lockup_gain = std::stof(value);
                    else if (key == "spin_enabled") engine.m_spin_enabled = std::stoi(value);
                    else if (key == "spin_gain") engine.m_spin_gain = std::stof(value);
                    else if (key == "slide_enabled") engine.m_slide_texture_enabled = std::stoi(value);
                    else if (key == "slide_gain") engine.m_slide_texture_gain = std::stof(value);
                    else if (key == "road_enabled") engine.m_road_texture_enabled = std::stoi(value);
                    else if (key == "road_gain") engine.m_road_texture_gain = std::stof(value);
                    else if (key == "invert_force") engine.m_invert_force = std::stoi(value);
                    else if (key == "max_torque_ref") engine.m_max_torque_ref = std::stof(value);
                    else if (key == "use_manual_slip") engine.m_use_manual_slip = std::stoi(value);
                    else if (key == "bottoming_method") engine.m_bottoming_method = std::stoi(value);
                    else if (key == "scrub_drag_gain") engine.m_scrub_drag_gain = std::stof(value);
                    else if (key == "rear_align_effect") engine.m_rear_align_effect = std::stof(value);
                    else if (key == "sop_yaw_gain") engine.m_sop_yaw_gain = std::stof(value);
                    else if (key == "steering_shaft_gain") engine.m_steering_shaft_gain = std::stof(value);
                    else if (key == "base_force_mode") engine.m_base_force_mode = std::stoi(value);
                    else if (key == "gyro_gain") engine.m_gyro_gain = std::stof(value);
                } catch (...) {
                    std::cerr << "[Config] Error parsing line: " << line << std::endl;
                }
            }
        }
    }
    std::cout << "[Config] Loaded from " << filename << std::endl;
}
