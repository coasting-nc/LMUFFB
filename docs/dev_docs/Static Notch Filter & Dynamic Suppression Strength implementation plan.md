# Technical Report: Implementation of Static Notch Filter & Dynamic Suppression Strength

**Target Version:** v0.4.42
**Context:** Enhancing the "Signal Filtering" capabilities to address both speed-dependent vibrations (flat spots) and constant frequency noise (engine/physics bugs), while giving users more control over the intensity of the filtering.

---

## 1. Analysis & Design

### A. Dynamic Notch: Suppression Strength
**Requirement:** Allow the user to blend between the filtered signal and the raw signal.
**Logic:** Linear Interpolation (Lerp).
$$ F_{final} = (F_{filtered} \times Strength) + (F_{raw} \times (1.0 - Strength)) $$
*   **Range:** 0.0 to 1.0 (0% to 100%).
*   **Default:** 1.0 (Full suppression).

### B. Static Notch Filter
**Requirement:** A filter that targets a fixed frequency regardless of car speed.
**Use Case:** Removing constant engine vibration, mains hum (50/60Hz), or specific physics resonance.
**Logic:** Identical Biquad implementation as the Dynamic filter, but `Center Frequency` is a user variable, not calculated from velocity.
**Parameters:**
*   **Frequency:** 10Hz to 100Hz slider.
*   **Q-Factor:** To keep the UI simple as requested ("slider to determine frequency"), we will default this to a **High Q (5.0)** internally. This ensures the filter is "surgical" and minimizes the loss of road detail, satisfying the warning requirement.

---

## 2. Implementation Steps

### Phase 1: Core Engine (`FFBEngine.h`)

We need to add state variables for the new settings and a second filter instance.

**1. Add Member Variables:**
```cpp
class FFBEngine {
public:
    // ... existing settings ...
    
    // Dynamic Filter Settings
    float m_flatspot_strength = 1.0f; // 0.0 - 1.0

    // Static Filter Settings
    bool m_static_notch_enabled = false;
    float m_static_notch_freq = 50.0f; // Default 50Hz
    // We use a fixed Q of 5.0 for static to be surgical, or reuse m_notch_q if desired. 
    // Let's hardcode 5.0 for now to keep UI simple as requested.

private:
    // Filter Instances
    BiquadNotch m_notch_filter;       // Existing (Dynamic)
    BiquadNotch m_static_notch_filter; // NEW (Static)
```

**2. Update `calculate_force` Logic:**

```cpp
// Inside calculate_force, replacing the existing filter block:

// --- 1. Dynamic Notch Filter (Speed Dependent) ---
if (m_flatspot_suppression) {
    // ... [Existing Frequency Calculation] ...
    
    if (wheel_freq > 1.0) {
        m_notch_filter.Update(wheel_freq, 400.0, m_notch_q);
        
        double raw_input = game_force;
        double filtered = m_notch_filter.Process(raw_input);
        
        // Apply Strength Blending
        game_force = (filtered * m_flatspot_strength) + (raw_input * (1.0f - m_flatspot_strength));
    } else {
        m_notch_filter.Reset();
    }
}

// --- 2. Static Notch Filter (Fixed Frequency) ---
if (m_static_notch_enabled) {
    // Fixed Q of 5.0 for surgical removal
    m_static_notch_filter.Update((double)m_static_notch_freq, 400.0, 5.0);
    game_force = m_static_notch_filter.Process(game_force);
}
```

---

### Phase 2: Configuration (`src/Config.h` & `.cpp`)

Persist the new settings.

**1. Update `Preset` Struct (`Config.h`):**
```cpp
struct Preset {
    // ... existing ...
    float flatspot_strength = 1.0f;
    bool static_notch_enabled = false;
    float static_notch_freq = 50.0f;

    // Update Setters
    Preset& SetFlatspot(bool enabled, float q = 2.0f, float strength = 1.0f) { 
        flatspot_suppression = enabled; 
        notch_q = q; 
        flatspot_strength = strength;
        return *this; 
    }
    
    Preset& SetStaticNotch(bool enabled, float freq) {
        static_notch_enabled = enabled;
        static_notch_freq = freq;
        return *this;
    }

    // Update Apply() and UpdateFromEngine() to map these new variables
};
```

**2. Update `Config::Save` and `Config::Load` (`Config.cpp`):**
*   Add keys: `flatspot_strength`, `static_notch_enabled`, `static_notch_freq`.

---

### Phase 3: User Interface (`src/GuiLayer.cpp`)

Update the "Signal Filtering" section in `DrawTuningWindow`.

```cpp
    if (ImGui::TreeNode("Signal Filtering")) {
        // --- Dynamic Section ---
        BoolSetting("Dynamic Flatspot Suppression", &engine.m_flatspot_suppression);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Removes vibrations linked to wheel speed (e.g. flat spots).");
        }

        if (engine.m_flatspot_suppression) {
            ImGui::Indent();
            FloatSetting("Notch Width (Q)", &engine.m_notch_q, 0.5f, 10.0f, "Q: %.1f");
            
            // NEW: Strength Slider
            FloatSetting("Suppression Strength", &engine.m_flatspot_strength, 0.0f, 1.0f, "%.0f%%");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Intensity of the filter.\n100% = Full removal.\n50% = Reduced vibration (Immersion).");
            
            // Frequency Readout (Existing)
            ImGui::TextColored(ImVec4(0,1,1,1), "Est. Freq: %.1f Hz | Theory: %.1f Hz", 
                (float)engine.m_debug_freq, (float)engine.m_theoretical_freq);
                
            ImGui::Unindent();
        }

        ImGui::Separator();

        // --- NEW: Static Section ---
        BoolSetting("Static Noise Filter", &engine.m_static_notch_enabled);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Removes a specific constant frequency (e.g. engine hum, physics bugs).\nWARNING: Removes road detail at this frequency!");
        }

        if (engine.m_static_notch_enabled) {
            ImGui::Indent();
            FloatSetting("Target Frequency", &engine.m_static_notch_freq, 10.0f, 100.0f, "%.0f Hz");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("The specific frequency to kill.");
            ImGui::Unindent();
        }
        
        ImGui::TreePop();
    }
```

---

## 3. Prompt for the Coding Agent
 
**Task: Implement Static Notch Filter & Dynamic Suppression Strength**

**Context:**
We are refining the "Signal Filtering" tools. Users need to be able to blend the Dynamic Notch filter (to keep some flatspot feel) and need a separate Static Notch filter to remove constant-frequency noise (like engine hum).

**Implementation Requirements:**

1.  **Core Physics (`FFBEngine.h`):**
    *   Add `m_flatspot_strength` (float, default 1.0) to `FFBEngine`.
    *   Add `m_static_notch_enabled` (bool) and `m_static_notch_freq` (float, default 50.0) to `FFBEngine`.
    *   Add a second `BiquadNotch` instance named `m_static_notch_filter`.
    *   **Update `calculate_force`:**
        *   Modify the Dynamic Notch logic to blend the output using `m_flatspot_strength` (Linear Interpolation).
        *   Add the Static Notch logic *after* the Dynamic Notch. It should use a fixed Q of 5.0 (Surgical) and the user-defined frequency.

2.  **Configuration (`src/Config.h` & `src/Config.cpp`):**
    *   Update `Preset` struct to include the 3 new variables.
    *   Update `Save` and `Load` to persist them to `config.ini`.
    *   Update `LoadPresets` to initialize them (Static: Off, Strength: 1.0).

3.  **User Interface (`src/GuiLayer.cpp`):**
    *   **Dynamic Section:** Add a slider for "Suppression Strength" (0-100%) under the Notch Width slider.
    *   **Static Section:** Add a new checkbox "Static Noise Filter" and a slider "Target Frequency" (10-100Hz).
    *   **Tooltips:** Add the specific warning for the Static filter: "WARNING: Removes road detail at this frequency!".

**Deliverables:**
*   Updated `FFBEngine.h`
*   Updated `src/Config.h` and `src/Config.cpp`
*   Updated `src/GuiLayer.cpp`

**Check-list for completion:**
- [ ] `m_flatspot_strength` implemented with blending logic.
- [ ] `m_static_notch_filter` implemented with fixed Q=5.0.
- [ ] Config system saves/loads new parameters.
- [ ] GUI displays "Suppression Strength" slider.
- [ ] GUI displays "Static Noise Filter" checkbox and Frequency slider.