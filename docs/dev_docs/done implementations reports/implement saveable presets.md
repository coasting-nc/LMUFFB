## Question

We need a mechanism to save a preset with a custom name that the user can edit. The preset should be saved in one of the ini files.


## Answer


This is a great addition for usability. To implement this, we need to:

1.  **Update `Preset` struct**: Add a flag to distinguish between "Built-in" (read-only) and "User" (saveable) presets, and a method to capture the current engine state into a preset.
2.  **Update `Config::Save`**: Modify the save logic to iterate through the user presets and write them to the `.ini` file.
3.  **Update `GuiLayer`**: Add a text input and a "Save" button to the UI.

Here is the code.

### 1. Update `src/Config.h`

Add `is_builtin` and `UpdateFromEngine`.

```cpp
#ifndef CONFIG_H
#define CONFIG_H

#include "../FFBEngine.h"
#include <string>
#include <vector>

struct Preset {
    std::string name;
    bool is_builtin = false; // NEW: Track if this is hardcoded or user-created
    
    // ... [Existing variables: gain, understeer, etc.] ...
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
    float sop_yaw_gain = 0.0f;
    float gyro_gain = 0.0f;
    
    float steering_shaft_gain = 1.0f;
    int base_force_mode = 0;

    // Constructors
    Preset(std::string n, bool builtin = false) : name(n), is_builtin(builtin) {}
    Preset() : name("Unnamed"), is_builtin(false) {}

    // ... [Existing Fluent Setters] ...
    Preset& SetGain(float v) { gain = v; return *this; }
    // ... (Keep all existing setters) ...
    // Add these if missing from previous steps to be safe:
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

    // NEW: Capture current engine state into this preset
    void UpdateFromEngine(const FFBEngine& engine) {
        gain = engine.m_gain;
        understeer = engine.m_understeer_effect;
        sop = engine.m_sop_effect;
        sop_scale = engine.m_sop_scale;
        sop_smoothing = engine.m_sop_smoothing_factor;
        min_force = engine.m_min_force;
        oversteer_boost = engine.m_oversteer_boost;
        lockup_enabled = engine.m_lockup_enabled;
        lockup_gain = engine.m_lockup_gain;
        spin_enabled = engine.m_spin_enabled;
        spin_gain = engine.m_spin_gain;
        slide_enabled = engine.m_slide_texture_enabled;
        slide_gain = engine.m_slide_texture_gain;
        road_enabled = engine.m_road_texture_enabled;
        road_gain = engine.m_road_texture_gain;
        invert_force = engine.m_invert_force;
        max_torque_ref = engine.m_max_torque_ref;
        use_manual_slip = engine.m_use_manual_slip;
        bottoming_method = engine.m_bottoming_method;
        scrub_drag_gain = engine.m_scrub_drag_gain;
        rear_align_effect = engine.m_rear_align_effect;
        sop_yaw_gain = engine.m_sop_yaw_gain;
        gyro_gain = engine.m_gyro_gain;
        steering_shaft_gain = engine.m_steering_shaft_gain;
        base_force_mode = engine.m_base_force_mode;
    }
};

class Config {
public:
    static void Save(const FFBEngine& engine, const std::string& filename = "config.ini");
    static void Load(FFBEngine& engine, const std::string& filename = "config.ini");
    
    // Preset Management
    static std::vector<Preset> presets;
    static void LoadPresets(); 
    static void ApplyPreset(int index, FFBEngine& engine);
    
    // NEW: Add a user preset
    static void AddUserPreset(const std::string& name, const FFBEngine& engine);

    static bool m_ignore_vjoy_version_warning;
    static bool m_enable_vjoy;        
    static bool m_output_ffb_to_vjoy; 
};

#endif
```

### 2. Update `src/Config.cpp`

We need to update `LoadPresets` to mark built-ins, implement `AddUserPreset`, and update `Save` to write the presets to the file.

```cpp
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
    
    // --- Built-in Presets (Marked as true) ---
    presets.push_back(Preset("Default", true));

    presets.push_back(Preset("T300", true)
        .SetGain(1.0f).SetShaftGain(1.0f).SetMinForce(0.0f).SetMaxTorque(100.0f)
        .SetInvert(true).SetUndersteer(38.0f).SetSoP(0.0f).SetOversteer(0.0f)
        .SetRearAlign(0.0f).SetSoPYaw(0.0f).SetGyro(0.0f).SetLockup(false, 0.0f)
        .SetSpin(false, 0.0f).SetSlide(false, 0.0f).SetRoad(false, 0.0f)
        .SetScrub(0.0f).SetBaseMode(0)
    );
    
    presets.push_back(Preset("Test: Game Base FFB Only", true)
        .SetUndersteer(0.0f).SetSoP(0.0f).SetSoPScale(5.0f).SetSmoothing(0.0f)
        .SetSlide(false, 0.0f).SetRearAlign(0.0f)
    );

    presets.push_back(Preset("Test: SoP Only", true)
        .SetUndersteer(0.0f).SetSoP(1.0f).SetSoPScale(5.0f).SetSmoothing(0.0f)
        .SetSlide(false, 0.0f).SetRearAlign(0.0f).SetSoPYaw(0.0f).SetBaseMode(2)
    );

    presets.push_back(Preset("Test: Understeer Only", true)
        .SetUndersteer(1.0f).SetSoP(0.0f).SetSoPScale(0.0f).SetSmoothing(0.0f)
        .SetSlide(false, 0.0f).SetRearAlign(0.0f)
    );

    presets.push_back(Preset("Test: Textures Only", true)
        .SetUndersteer(0.0f).SetSoP(0.0f).SetSoPScale(0.0f).SetSmoothing(0.0f)
        .SetLockup(true, 1.0f).SetSpin(true, 1.0f).SetSlide(true, 1.0f)
        .SetRoad(true, 1.0f).SetRearAlign(0.0f).SetBaseMode(2)
    );

    presets.push_back(Preset("Test: Rear Align Torque Only", true)
        .SetGain(1.0f).SetUndersteer(0.0f).SetSoP(0.0f).SetSmoothing(0.0f)
        .SetSlide(false, 0.0f).SetRearAlign(1.0f).SetSoPYaw(0.0f)
    );

    presets.push_back(Preset("Test: SoP Base Only", true)
        .SetGain(1.0f).SetUndersteer(0.0f).SetSoP(1.0f).SetSmoothing(0.0f)
        .SetSlide(false, 0.0f).SetRearAlign(0.0f).SetSoPYaw(0.0f).SetBaseMode(2)
    );

    presets.push_back(Preset("Test: Slide Texture Only", true)
        .SetGain(1.0f).SetUndersteer(0.0f).SetSoP(0.0f).SetSmoothing(0.0f)
        .SetSlide(true, 1.0f).SetRearAlign(0.0f).SetBaseMode(2)
    );

    presets.push_back(Preset("Test: No Effects", true)
        .SetGain(1.0f).SetUndersteer(0.0f).SetSoP(0.0f).SetSmoothing(0.0f)
        .SetSlide(false, 0.0f).SetRearAlign(0.0f).SetBaseMode(2)
    );

    presets.push_back(Preset("Guide: Understeer (Front Grip)", true)
        .SetGain(1.0f).SetUndersteer(1.0f).SetSoP(0.0f).SetOversteer(0.0f)
        .SetRearAlign(0.0f).SetSoPYaw(0.0f).SetGyro(0.0f).SetLockup(false, 0.0f)
        .SetSpin(false, 0.0f).SetSlide(false, 0.0f).SetRoad(false, 0.0f)
        .SetScrub(0.0f).SetBaseMode(0)
    );

    presets.push_back(Preset("Guide: Oversteer (Rear Grip)", true)
        .SetGain(1.0f).SetUndersteer(0.0f).SetSoP(1.0f).SetSoPScale(20.0f)
        .SetRearAlign(1.0f).SetOversteer(1.0f).SetSoPYaw(0.0f).SetGyro(0.0f)
        .SetLockup(false, 0.0f).SetSpin(false, 0.0f).SetSlide(false, 0.0f)
        .SetRoad(false, 0.0f).SetScrub(0.0f).SetBaseMode(0)
    );

    presets.push_back(Preset("Guide: Slide Texture (Scrub)", true)
        .SetGain(1.0f).SetUndersteer(0.0f).SetSoP(0.0f).SetOversteer(0.0f)
        .SetRearAlign(0.0f).SetSlide(true, 1.0f).SetScrub(1.0f)
        .SetLockup(false, 0.0f).SetSpin(false, 0.0f).SetRoad(false, 0.0f).SetBaseMode(2)
    );

    presets.push_back(Preset("Guide: Braking Lockup", true)
        .SetGain(1.0f).SetUndersteer(0.0f).SetSoP(0.0f).SetOversteer(0.0f)
        .SetRearAlign(0.0f).SetLockup(true, 1.0f).SetSpin(false, 0.0f)
        .SetSlide(false, 0.0f).SetRoad(false, 0.0f).SetScrub(0.0f).SetBaseMode(2)
    );

    presets.push_back(Preset("Guide: Traction Loss (Spin)", true)
        .SetGain(1.0f).SetUndersteer(0.0f).SetSoP(0.0f).SetOversteer(0.0f)
        .SetRearAlign(0.0f).SetSpin(true, 1.0f).SetLockup(false, 0.0f)
        .SetSlide(false, 0.0f).SetRoad(false, 0.0f).SetScrub(0.0f).SetBaseMode(2)
    );

    presets.push_back(Preset("Guide: SoP Yaw (Kick)", true)
        .SetGain(1.0f).SetUndersteer(0.0f).SetSoP(0.0f).SetOversteer(0.0f)
        .SetRearAlign(0.0f).SetSoPYaw(2.0f).SetGyro(0.0f).SetLockup(false, 0.0f)
        .SetSpin(false, 0.0f).SetSlide(false, 0.0f).SetRoad(false, 0.0f)
        .SetScrub(0.0f).SetBaseMode(2)
    );

    presets.push_back(Preset("Guide: Gyroscopic Damping", true)
        .SetGain(1.0f).SetUndersteer(0.0f).SetSoP(0.0f).SetOversteer(0.0f)
        .SetRearAlign(0.0f).SetSoPYaw(0.0f).SetGyro(1.0f).SetLockup(false, 0.0f)
        .SetSpin(false, 0.0f).SetSlide(false, 0.0f).SetRoad(false, 0.0f)
        .SetScrub(0.0f).SetBaseMode(2)
    );

    // --- Parse User Presets from config.ini ---
    std::ifstream file("config.ini");
    if (!file.is_open()) return;

    std::string line;
    bool in_presets = false;
    
    std::string current_preset_name = "";
    Preset current_preset; 
    bool preset_pending = false;

    while (std::getline(file, line)) {
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

void Config::ApplyPreset(int index, FFBEngine& engine) {
    if (index >= 0 && index < presets.size()) {
        presets[index].Apply(engine);
        std::cout << "[Config] Applied preset: " << presets[index].name << std::endl;
    }
}

void Config::Save(const FFBEngine& engine, const std::string& filename) {
    std::ofstream file(filename);
    if (file.is_open()) {
        // 1. Global Settings
        file << "ignore_vjoy_version_warning=" << m_ignore_vjoy_version_warning << "\n";
        file << "enable_vjoy=" << m_enable_vjoy << "\n";
        file << "output_ffb_to_vjoy=" << m_output_ffb_to_vjoy << "\n";
        
        // 2. Current Engine State (Active Config)
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

// Load function remains the same
```

### 3. Update `src/GuiLayer.cpp`

Add the UI controls for saving a new preset.

```cpp
// ... inside DrawTuningWindow ...

    // --- PRESET LOGIC WITH DIRTY STATE ---
    static int selected_preset = 0;
    if (Config::presets.empty()) {
        Config::LoadPresets();
    }
    
    const char* preview_value = (selected_preset >= 0 && selected_preset < Config::presets.size()) 
                                ? Config::presets[selected_preset].name.c_str() 
                                : "Custom";
    
    if (ImGui::BeginCombo("Load Preset", preview_value)) {
        for (int i = 0; i < Config::presets.size(); i++) {
            bool is_selected = (selected_preset == i);
            // Optional: Color code built-in vs user presets?
            if (ImGui::Selectable(Config::presets[i].name.c_str(), is_selected)) {
                selected_preset = i;
                Config::ApplyPreset(i, engine);
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    
    // NEW: Save As Preset UI
    static char new_preset_name[64] = "";
    ImGui::InputText("##NewPresetName", new_preset_name, 64);
    ImGui::SameLine();
    if (ImGui::Button("Save as New Preset")) {
        if (strlen(new_preset_name) > 0) {
            Config::AddUserPreset(std::string(new_preset_name), engine);
            // Auto-select the new preset
            for (int i = 0; i < Config::presets.size(); i++) {
                if (Config::presets[i].name == std::string(new_preset_name)) {
                    selected_preset = i;
                    break;
                }
            }
            // Clear buffer
            new_preset_name[0] = '\0';
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save current settings as a new custom preset.");

    ImGui::Separator();
    
    // ... [Rest of the file] ...
```
