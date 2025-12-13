#include "Config.h"
#include <fstream>
#include <sstream>
#include <iostream>

bool Config::m_ignore_vjoy_version_warning = false;
bool Config::m_enable_vjoy = false;        // Disabled by default (Replaces vJoyActive logic)
bool Config::m_output_ffb_to_vjoy = false; // Disabled by default (Safety)

std::vector<Preset> Config::presets;

void Config::LoadPresets() {
    presets.clear();
    
    // Built-in Presets
    presets.push_back({ "Default", 
        0.5f, 1.0f, 0.15f, 20.0f, 0.05f, 0.0f, 0.0f, // gain, under, sop, scale, smooth, min, over
        false, 0.5f, false, 0.5f, true, 0.5f, false, 0.5f, // lockup, spin, slide, road
        false, 40.0f, // invert, max_torque_ref (Default 40Nm for 1.0 Gain)
        false, 0, 0.0f, // use_manual_slip, bottoming_method, scrub_drag_gain (v0.4.5)
        1.0f // rear_align_effect (v0.4.11)
    });
    
    presets.push_back({ "Test: Game Base FFB Only", 
        0.5f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f,
        false, 0.0f, false, 0.0f, false, 0.0f, false, 0.0f,
        false, 40.0f,
        false, 0, 0.0f, // v0.4.5
        0.0f // rear_align_effect
    });

    presets.push_back({ "Test: SoP Only", 
        0.5f, 0.0f, 1.0f, 5.0f, 0.0f, 0.0f, 0.0f,
        false, 0.0f, false, 0.0f, false, 0.0f, false, 0.0f,
        false, 40.0f,
        false, 0, 0.0f, // v0.4.5
        0.0f // rear_align_effect (Matched old behavior where boost=0 -> rear=0)
    });

    presets.push_back({ "Test: Understeer Only", 
        0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        false, 0.0f, false, 0.0f, false, 0.0f, false, 0.0f,
        false, 40.0f,
        false, 0, 0.0f, // v0.4.5
        0.0f // rear_align_effect
    });

    presets.push_back({ "Test: Textures Only", 
        0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        true, 1.0f, false, 1.0f, true, 1.0f, true, 1.0f,
        false, 40.0f,
        false, 0, 0.0f, // v0.4.5
        0.0f // rear_align_effect
    });

    presets.push_back({ "Test: Rear Align Torque Only", 
        1.0f, 0.0f, 0.0f, 20.0f, 0.0f, 0.0f, 0.0f, // gain, under, sop(0), scale, smooth, min, over(0)
        false, 0.0f, false, 0.0f, false, 0.0f, false, 0.0f, // No textures
        false, 40.0f,
        false, 0, 0.0f, // v0.4.5
        1.0f // rear_align_effect=1.0
    });

    presets.push_back({ "Test: SoP Base Only", 
        1.0f, 0.0f, 1.0f, 20.0f, 0.0f, 0.0f, 0.0f, // sop=1.0, over=0
        false, 0.0f, false, 0.0f, false, 0.0f, false, 0.0f,
        false, 40.0f,
        false, 0, 0.0f,
        0.0f // rear_align_effect=0
    });

    presets.push_back({ "Test: Slide Texture Only", 
        1.0f, 0.0f, 0.0f, 20.0f, 0.0f, 0.0f, 0.0f,
        false, 0.0f, false, 0.0f, true, 1.0f, false, 0.0f, // Slide enabled, gain 1.0
        false, 40.0f,
        false, 0, 0.0f,
        0.0f
    });

    presets.push_back({ "Test: No Effects", 
        1.0f, 0.0f, 0.0f, 20.0f, 0.0f, 0.0f, 0.0f, // gain, under, sop, scale, smooth, min, over
        false, 0.0f, false, 0.0f, false, 0.0f, false, 0.0f, // lockup, spin, slide, road
        false, 40.0f, // invert, max_torque_ref
        false, 0, 0.0f, // use_manual_slip, bottoming_method, scrub_drag_gain
        0.0f // rear_align_effect
    });

    // Parse User Presets from config.ini [Presets] section
    std::ifstream file("config.ini");
    if (!file.is_open()) return;

    std::string line;
    bool in_presets = false;
    
    // Preset parsing state
    std::string current_preset_name = "";
    Preset current_preset;
    bool preset_pending = false;

    while (std::getline(file, line)) {
        // Strip whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty() || line[0] == ';') continue;

        if (line[0] == '[') {
            if (preset_pending && !current_preset_name.empty()) {
                current_preset.name = current_preset_name;
                presets.push_back(current_preset);
                preset_pending = false;
            }
            
            if (line == "[Presets]") {
                in_presets = true;
            } else if (line.rfind("[Preset:", 0) == 0) { // e.g. [Preset:My Custom]
                // Start a new preset
                in_presets = false; // We are in a specific preset block now
                size_t end_pos = line.find(']');
                if (end_pos != std::string::npos) {
                    current_preset_name = line.substr(8, end_pos - 8);
                    // Initialize with default values to be safe
                    current_preset = presets[0]; 
                    current_preset.name = current_preset_name;
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
                        // New Params (v0.4.5)
                        else if (key == "use_manual_slip") current_preset.use_manual_slip = std::stoi(value);
                        else if (key == "bottoming_method") current_preset.bottoming_method = std::stoi(value);
                        else if (key == "scrub_drag_gain") current_preset.scrub_drag_gain = std::stof(value);
                        else if (key == "rear_align_effect") current_preset.rear_align_effect = std::stof(value);
                    } catch (...) {}
                }
            }
        }
    }
    
    // Push the last one
    if (preset_pending && !current_preset_name.empty()) {
        current_preset.name = current_preset_name;
        presets.push_back(current_preset);
    }
}

void Config::ApplyPreset(int index, FFBEngine& engine) {
    if (index >= 0 && index < presets.size()) {
        presets[index].Apply(engine);
        std::cout << "[Config] Applied preset: " << presets[index].name << std::endl;
    }
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
                } catch (...) {
                    std::cerr << "[Config] Error parsing line: " << line << std::endl;
                }
            }
        }
    }
    std::cout << "[Config] Loaded from " << filename << std::endl;
}
