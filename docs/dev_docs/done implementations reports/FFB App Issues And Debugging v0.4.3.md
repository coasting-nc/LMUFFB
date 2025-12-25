Here is the implementation plan to address the three issues you identified.

### Plan Overview

1.  **Inverted FFB**:
    *   **Logic**: Add a boolean flag `m_invert_force` to the Engine. Multiply the final output force by `-1.0` if true.
    *   **UI**: Add a checkbox in `GuiLayer`.
    *   **Persistence**: Update `Config.h` and `Config.cpp` to save/load this setting.
2.  **DirectInput Logging**:
    *   **Logic**: Modify `DirectInputFFB::SelectDevice` to track which Cooperative Level succeeded (Exclusive vs. Non-Exclusive) and print that specific string in the success message.
3.  **Telemetry Stats**:
    *   **Logic**: Refactor the `ChannelStats` struct in `FFBEngine.h`. Split the logic so `Min` and `Max` persist for the whole session, while `Sum` and `Count` (used for Average) are reset every second.
    *   **Output**: Update the console print to explicitly label "Session Min/Max" and "Interval Avg".
4.  **FFB Scaling (Intensity Fix)**:
    *   **Logic**: Replace the hardcoded `20.0` Nm normalization constant in `FFBEngine` with a configurable variable `m_max_torque_ref` (Defaulting to 40.0 Nm to correct the 2x signal strength).
    *   **UI**: Add a slider "Max Torque Ref (Nm)" in `GuiLayer` to allow users to calibrate 100% output to their specific car/wheel torque.
    *   **Persistence**: Update `Config` to save/load this reference value.

---

### Step 1: Inverted FFB Signal

**1.1 Update `FFBEngine.h`**
Add the boolean variable.

```cpp
class FFBEngine {
public:
    // ... existing settings ...
    bool m_invert_force = false; // New setting

    double calculate_force(const TelemInfoV01* data) {
        // ... [All existing calculations] ...

        // --- 5. Min Force & Output ---
        // ... [Existing normalization logic] ...
        
        // APPLY INVERSION HERE (Before clipping)
        if (m_invert_force) {
            norm_force *= -1.0;
        }

        return (std::max)(-1.0, (std::min)(1.0, norm_force));
    }
};
```

**1.2 Update `src/Config.h` & `src/Config.cpp`**
Ensure the setting is saved to `config.ini`.

*In `src/Config.h` (Preset struct):*
```cpp
struct Preset {
    // ... existing ...
    bool invert_force; // Add this

    void Apply(FFBEngine& engine) const {
        // ... existing ...
        engine.m_invert_force = invert_force;
    }
};
```

*In `src/Config.cpp` (LoadPresets):*
Update the built-in presets (default to `false`).
```cpp
presets.push_back({ "Default", 
    0.5f, 1.0f, 0.15f, 5.0f, 0.05f, 0.0f, 0.0f, 
    false, 0.5f, false, 0.5f, true, 0.5f, false, 0.5f,
    false // <--- Invert default
});
// Update other presets similarly...
```

*In `src/Config.cpp` (Save/Load):*
```cpp
void Config::Save(...) {
    // ...
    file << "invert_force=" << engine.m_invert_force << "\n";
    // ...
}

void Config::Load(...) {
    // ...
    else if (key == "invert_force") engine.m_invert_force = std::stoi(value);
    // ...
}
```

**1.3 Update `src/GuiLayer.cpp`**
Add the checkbox to the "Output" section.

```cpp
// Inside DrawTuningWindow
ImGui::Separator();
ImGui::Text("Output");

ImGui::Checkbox("Invert FFB Signal", &engine.m_invert_force); // <--- Add this
if (ImGui::IsItemHovered()) ImGui::SetTooltip("Check this if the wheel pulls away from center instead of aligning.");

// ... existing vJoy checkbox ...
```

---

### Step 2: DirectInput Logging Clarity

**Update `src/DirectInputFFB.cpp`**
Modify the `SelectDevice` function to capture the mode.

```cpp
bool DirectInputFFB::SelectDevice(const GUID& guid) {
    // ... [Creation logic] ...

    // Attempt 1: Exclusive/Background
    std::cout << "[DI] Attempting to set Cooperative Level..." << std::endl;
    HRESULT hr = m_pDevice->SetCooperativeLevel(m_hwnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND);
    
    std::string mode_str = "EXCLUSIVE | BACKGROUND"; // Default assumption

    // Fallback: Non-Exclusive
    if (FAILED(hr)) {
         std::cerr << "[DI] Exclusive mode failed (Error: " << std::hex << hr << std::dec << "). Retrying in Non-Exclusive mode..." << std::endl;
         hr = m_pDevice->SetCooperativeLevel(m_hwnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
         mode_str = "NON-EXCLUSIVE | BACKGROUND";
    }
    
    if (FAILED(hr)) {
        std::cerr << "[DI] Failed to set cooperative level." << std::endl;
        return false;
    }

    std::cout << "[DI] Acquiring device..." << std::endl;
    if (FAILED(m_pDevice->Acquire())) {
        std::cerr << "[DI] Failed to acquire device." << std::endl;
    } else {
        std::cout << "[DI] Device Acquired in " << mode_str << " mode." << std::endl; // <--- Explicit Log
    }

    // ... [Rest of function] ...
}
```

---

### Step 3: Telemetry Stats Clarification

**Update `FFBEngine.h`**
Refactor `ChannelStats` to separate session-long tracking from interval tracking.

```cpp
// FFBEngine.h

struct ChannelStats {
    // Session-wide stats (Persistent)
    double session_min = 1e9;
    double session_max = -1e9;
    
    // Interval stats (Reset every second)
    double interval_sum = 0.0;
    long interval_count = 0;
    
    void Update(double val) {
        // Update Session Min/Max
        if (val < session_min) session_min = val;
        if (val > session_max) session_max = val;
        
        // Update Interval Accumulator
        interval_sum += val;
        interval_count++;
    }
    
    void ResetInterval() {
        interval_sum = 0.0; 
        interval_count = 0;
        // Do NOT reset session_min/max here
    }
    
    double GetIntervalAvg() { 
        return interval_count > 0 ? interval_sum / interval_count : 0.0; 
    }
};

// Inside calculate_force logging block:
if (std::chrono::duration_cast<std::chrono::seconds>(now - last_log_time).count() >= 1) {
    std::cout << "--- TELEMETRY STATS (Last 1s Avg | Session Min/Max) ---" << std::endl;
    
    std::cout << "Torque (Nm): Avg=" << s_torque.GetIntervalAvg() 
              << " | Min=" << s_torque.session_min 
              << " Max=" << s_torque.session_max << std::endl;
              
    std::cout << "Load (N):    Avg=" << s_load.GetIntervalAvg()   
              << " | Min=" << s_load.session_min   
              << " Max=" << s_load.session_max << std::endl;
              
    // ... repeat for Grip and Lat G ...
    
    // Reset only the interval data
    s_torque.ResetInterval(); 
    s_load.ResetInterval(); 
    s_grip.ResetInterval(); 
    s_lat_g.ResetInterval();
    
    last_log_time = now;
}
```

---

### Step 4: Fix FFB Scaling (Intensity Mismatch)

**Investigation:**
The issue where "Gain 0.5" feels correct implies that the hardcoded reference torque (`20.0 Nm`) used for normalization is too low.
*   **Current Logic:** `Output = RawTorque / 20.0`.
*   **Observation:** If the game's internal FFB processing (which we bypass) uses a higher reference (e.g., 40Nm) or applies a default reduction factor (e.g., 0.5x Car Specific Multiplier), our raw output appears double the intensity.
*   **Fix:** Instead of hardcoding `20.0`, we should expose this as a configurable **"Max Torque Reference"**. Increasing this value will reduce the signal strength at a given Gain, allowing the user to keep Master Gain at 1.0 for a 1:1 feel.

**4.1 Update `FFBEngine.h`**
Replace the hardcoded constant with a configurable variable.

```cpp
class FFBEngine {
public:
    // ... existing settings ...
    float m_max_torque_ref = 40.0f; // New setting (Default 40.0 based on feedback)

    double calculate_force(const TelemInfoV01* data) {
        // ... [Calculations] ...

        // --- 6. Min Force (Deadzone Removal) ---
        // Use the configurable reference instead of hardcoded 20.0
        double max_force_ref = (double)m_max_torque_ref; 
        
        // Safety: Prevent divide by zero
        if (max_force_ref < 1.0) max_force_ref = 1.0;

        double norm_force = total_force / max_force_ref;
        
        // ... [Gain application] ...
    }
};
```

**4.2 Update `src/Config.h` & `src/Config.cpp`**
Persist the new setting.

*In `src/Config.h` (Preset struct):*
```cpp
struct Preset {
    // ... existing ...
    float max_torque_ref; // Add this

    void Apply(FFBEngine& engine) const {
        // ... existing ...
        engine.m_max_torque_ref = max_torque_ref;
    }
};
```

*In `src/Config.cpp` (LoadPresets):*
Update presets (Default to 40.0f).
```cpp
presets.push_back({ "Default", 
    // ... existing values ...
    false, // invert
    40.0f  // <--- Max Torque Ref
});
```

*In `src/Config.cpp` (Save/Load):*
```cpp
void Config::Save(...) {
    // ...
    file << "max_torque_ref=" << engine.m_max_torque_ref << "\n";
    // ...
}

void Config::Load(...) {
    // ...
    else if (key == "max_torque_ref") engine.m_max_torque_ref = std::stof(value);
    // ...
}
```

**4.3 Update `src/GuiLayer.cpp`**
Add the slider to the "Core Settings" or "Advanced Tuning" section.

```cpp
// Inside DrawTuningWindow, under "Core Settings" or "Advanced"
ImGui::SliderFloat("Max Torque Ref (Nm)", &engine.m_max_torque_ref, 10.0f, 100.0f, "%.1f Nm");
if (ImGui::IsItemHovered()) ImGui::SetTooltip("The torque value that equals 100% FFB output.\nIncrease this if FFB is too strong at Gain 1.0.\nTypical values: 20-40 Nm.");
```

**4.4 Update Documentation (`docs/dev_docs/FFB_formulas.md`)**
Update the formula reference.

```markdown
### 1. The Master Formula
$$ F_{final} = \text{Clamp}\left( \left( \frac{F_{total}}{T_{ref}} \times K_{gain} \right), -1.0, 1.0 \right) $$

*   $T_{ref}$: User setting `m_max_torque_ref` (Default 40.0 Nm).
```