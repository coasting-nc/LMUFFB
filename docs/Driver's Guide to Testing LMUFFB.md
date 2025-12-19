## **Driver's Guide to Testing LMUFFB**

### 🏁 Prerequisites

**Car/Track Choice:**
*   **Car:** **Porsche 911 GTE/GT3** is excellent. Because the engine is in the back, the front is light (easy to understeer) and the rear acts like a pendulum (clear oversteer).
*   **Track:** **Paul Ricard** is perfect because it is flat (no elevation changes to confuse you) and has massive run-off areas so you can spin safely.
    *   *Tip:* Use the **"Mistral Straight"** (the long one) for high-speed tests.
    *   *Tip:* Use the **last corner (Virage du Pont)** for low-speed traction tests.

**Global Setup:**
1.  **In-Game (LMU):** FFB Strength 0%, Smoothing 0.
2.  **Wheel Driver:** Set your physical wheel strength to **20-30%** (Safety first!).
3.  **LMUFFB:** Start with the **"Default"** preset, then modify as instructed below.

---

### 1. Understeer (Front Grip Loss)

**What is it?** The front tires are sliding. The car won't turn as much as you are turning the wheel.
**The Goal:** The steering should go **LIGHT** (lose weight) to tell you "Stop turning, you have no grip!"

**UI Settings (Isolation):**
*   **Master Gain:** `1.0`
*   **Understeer (Grip):** **`1.0`** (Max)
*   **SoP / Oversteer / Textures:** `0.0` (Turn OFF to feel the pure weight drop)

**Car Setup:**
*   **Brake Bias:** Move forward (e.g., 60%) to overload front tires.
*   **Front ARB (Anti-Roll Bar):** Stiff (Max).

**The Test:**
1.  Drive at moderate speed (100 km/h).
2.  Turn into a medium corner (e.g., Turn 1).
3.  **Intentionally turn too much.** Turn the wheel 90 degrees or more, past the point where the car actually turns.
4.  **What to feel:**
    *   *Normal:* Resistance builds up as you turn.
    *   *The Cue:* Suddenly, the resistance **stops increasing** or even **drops**. The wheel feels "hollow" or "disconnected."
    *   *Correct Behavior:* If you unwind the wheel (straighten slightly), the weight returns.

---

### 2. Oversteer (Rear Grip Loss)

**What is it?** The rear tires are sliding. The back of the car is trying to overtake the front.
**The Goal:** The wheel should **PULL** against the turn (Counter-Steer). It wants to fix the slide for you.

**UI Settings (Isolation):**
*   **Master Gain:** `1.0`
*   **SoP (Lateral G):** `1.0`
*   **Rear Align Torque:** `1.0`
*   **Oversteer Boost:** `1.0`
*   **Understeer / Textures:** `0.0`

**Car Setup:**
*   **Traction Control (TC):** **OFF** (Crucial).
*   **Rear ARB:** Stiff.

**The Test:**
1.  Take a slow 2nd gear corner.
2.  Mid-corner, **mash the throttle 100%**.
3.  The rear will kick out.
4.  **What to feel:**
    *   *The Cue:* The steering wheel violently snaps in the **opposite direction** of the turn. If you are turning Left, the wheel rips to the Right.
    *   *Correct Behavior:* If you let go of the wheel, it should spin to align with the road (self-correcting).
    *   *Bug Check:* If the wheel pulls *into* the turn (making you spin faster), the "Inverted Force" bug is present.

---

### 3. Slide Texture (Scrubbing)

**What is it?** The tires are dragging sideways across the asphalt.
**The Goal:** A "grinding" or "sandpaper" vibration.

**UI Settings (Isolation):**
*   **Master Gain:** `1.0`
*   **Slide Rumble:** **Checked**
*   **Slide Gain:** `1.0`
*   **Scrub Drag Gain:** `1.0`
*   **All other effects:** `0.0`

**The Test:**
1.  Go to a wide runoff area.
2.  Turn the wheel fully to one side and accelerate to do a "donut" or a heavy understeer plow.
3.  **What to feel:**
    *   *The Cue:* A distinct, gritty vibration.
    *   *Physics Check:* The vibration pitch (frequency) should get **higher** as you slide faster.
        *   Slow slide = "Grrr-grrr" (Low rumble).
        *   Fast slide = "Zzzzzzz" (High buzz).

---

### 4. Braking Lockup

**What is it?** You braked too hard. The wheel stopped spinning, but the car is moving. You are burning a flat spot on the tire.
**The Goal:** A violent, jarring vibration to tell you to release the brake.

**UI Settings (Isolation):**
*   **Master Gain:** `1.0`
*   **Progressive Lockup:** **Checked**
*   **Lockup Gain:** `1.0`
*   **All other effects:** `0.0`

**Car Setup:**
*   **ABS:** **OFF** (Crucial).

**The Test:**
1.  Drive fast (200 km/h) down the Mistral Straight.
2.  Stomp the brake pedal **100%**.
3.  **What to feel:**
    *   *The Cue:* The wheel shakes violently.
    *   *Physics Check:* As the car slows down, the shaking should get **slower and heavier** (thump-thump-thump) because the "scrubbing speed" is decreasing.

---

### 5. Traction Loss (Wheel Spin)

**What is it?** You accelerated too hard. The rear wheels are spinning freely (burnout).
**The Goal:** The steering feels "light" and "floaty" combined with a high-frequency engine-like vibe.

**UI Settings (Isolation):**
*   **Master Gain:** `1.0`
*   **Spin Traction Loss:** **Checked**
*   **Spin Gain:** `1.0`
*   **All other effects:** `0.0`

**Car Setup:**
*   **TC:** **OFF**.

**The Test:**
1.  Stop the car. Put it in 1st gear.
2.  Hold the brake and throttle (Launch Control style) or just mash throttle.
3.  **What to feel:**
    *   *The Cue:* The steering weight suddenly disappears (Torque Drop). It feels like the car is floating on ice.
    *   *The Vibe:* A high-pitched hum/whine that rises as the RPM/Wheel Speed rises.

---

### 🛠️ Troubleshooting Cheat Sheet

| Symptom | Diagnosis | Fix |
| :--- | :--- | :--- |
| **Wheel feels dead/numb in corners** | SoP is too low or Understeer is too aggressive. | Increase `SoP (Lateral G)`. Decrease `Understeer`. |
| **Wheel oscillates (shakes L/R) on straights** | Latency or too much Min Force. | Increase `SoP Smoothing`. Decrease `Min Force`. |
| **Wheel pulls the wrong way in a slide** | Inverted Physics. | Check `Invert FFB Signal` or report bug (Yaw/Scrub inversion). |
| **No Road Texture over curbs** | Suspension frequency mismatch. | Increase `Road Gain`. Ensure `Load Cap` isn't too low. |
| **Effects feel "Digital" (On/Off)** | Clipping. | Check the "Clipping" bar in Debug window. Reduce `Master Gain` or increase `Max Torque Ref`. |



### Additional presets

I will add the 5 new presets corresponding to the "Driver's Guide to Testing LMUFFB" to `src/Config.cpp`. I will also update the `CHANGELOG.md` to reflect this addition.

### 1. Update `src/Config.cpp`

I will append the new "Guide:" presets to the `LoadPresets` function.

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
    
    // 1. Default (Uses the defaults defined in Config.h)
    presets.push_back(Preset("Default"));
    
    // 2. Test: Game Base FFB Only
    presets.push_back(Preset("Test: Game Base FFB Only")
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetSoPScale(5.0f)
        .SetSmoothing(0.0f)
        .SetSlide(false, 0.0f)
        .SetRearAlign(0.0f)
    );

    // 3. Test: SoP Only
    presets.push_back(Preset("Test: SoP Only")
        .SetUndersteer(0.0f)
        .SetSoP(1.0f)
        .SetSoPScale(5.0f)
        .SetSmoothing(0.0f)
        .SetSlide(false, 0.0f)
        .SetRearAlign(0.0f)
        .SetSoPYaw(0.0f)
        .SetBaseMode(2) // Muted
    );

    // 4. Test: Understeer Only
    presets.push_back(Preset("Test: Understeer Only")
        .SetUndersteer(1.0f)
        .SetSoP(0.0f)
        .SetSoPScale(0.0f)
        .SetSmoothing(0.0f)
        .SetSlide(false, 0.0f)
        .SetRearAlign(0.0f)
    );

    // 5. Test: Textures Only
    presets.push_back(Preset("Test: Textures Only")
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

    // 6. Test: Rear Align Torque Only
    presets.push_back(Preset("Test: Rear Align Torque Only")
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetSmoothing(0.0f)
        .SetSlide(false, 0.0f)
        .SetRearAlign(1.0f)
        .SetSoPYaw(0.0f)
    );

    // 7. Test: SoP Base Only
    presets.push_back(Preset("Test: SoP Base Only")
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(1.0f)
        .SetSmoothing(0.0f)
        .SetSlide(false, 0.0f)
        .SetRearAlign(0.0f)
        .SetSoPYaw(0.0f)
        .SetBaseMode(2) // Muted
    );

    // 8. Test: Slide Texture Only
    presets.push_back(Preset("Test: Slide Texture Only")
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetSmoothing(0.0f)
        .SetSlide(true, 1.0f)
        .SetRearAlign(0.0f)
        .SetBaseMode(2) // Muted
    );

    // 9. Test: No Effects
    presets.push_back(Preset("Test: No Effects")
        .SetGain(1.0f)
        .SetUndersteer(0.0f)
        .SetSoP(0.0f)
        .SetSmoothing(0.0f)
        .SetSlide(false, 0.0f)
        .SetRearAlign(0.0f)
        .SetBaseMode(2) // Muted
    );

    // --- NEW GUIDE PRESETS (v0.4.23) ---

    // 10. Guide: Understeer (Front Grip Loss)
    presets.push_back(Preset("Guide: Understeer (Front Grip)")
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

    // 11. Guide: Oversteer (Rear Grip Loss)
    presets.push_back(Preset("Guide: Oversteer (Rear Grip)")
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

    // 12. Guide: Slide Texture (Scrubbing)
    presets.push_back(Preset("Guide: Slide Texture (Scrub)")
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

    // 13. Guide: Braking Lockup
    presets.push_back(Preset("Guide: Braking Lockup")
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

    // 14. Guide: Traction Loss (Wheel Spin)
    presets.push_back(Preset("Guide: Traction Loss (Spin)")
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
                    current_preset = Preset(current_preset_name); // Reset to defaults
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
        file << "sop_yaw_gain=" << engine.m_sop_yaw_gain << "\n";
        file << "steering_shaft_gain=" << engine.m_steering_shaft_gain << "\n";
        file << "base_force_mode=" << engine.m_base_force_mode << "\n";
        file << "gyro_gain=" << engine.m_gyro_gain << "\n";
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
```

### 2. Update `CHANGELOG.md`

```markdown
## [0.4.24] - 2025-12-19
### Added
- **Guide Presets**: Added 5 new built-in presets corresponding to the "Driver's Guide to Testing LMUFFB".
    - **Guide: Understeer (Front Grip)**: Isolates the grip modulation effect.
    - **Guide: Oversteer (Rear Grip)**: Isolates SoP and Rear Aligning Torque.
    - **Guide: Slide Texture (Scrub)**: Isolates the scrubbing vibration (mutes base force).
    - **Guide: Braking Lockup**: Isolates the lockup vibration (mutes base force).
    - **Guide: Traction Loss (Spin)**: Isolates the wheel spin vibration (mutes base force).
    - These presets allow users to quickly configure the app for the specific test scenarios described in the documentation.
```

### 3. Update `VERSION`

```
0.4.24
```