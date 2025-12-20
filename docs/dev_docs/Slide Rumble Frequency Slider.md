## Question 

Should we have a slider in the GUI to customize the vibration frequency of this effect?


## Answer

Yes, absolutely. Adding a **Frequency Slider** is the best way to solve the "Belt vs. Direct Drive" dilemma.

### The Rationale
*   **Belt/Gear Wheels (T300, G29):** Physically cannot reproduce frequencies above ~80Hz. They need **Low Frequencies (10-60Hz)** to feel like "Rumble."
*   **Direct Drive Wheels (Simucube, Fanatec DD):** Can reproduce up to 400Hz. They benefit from **High Frequencies (100-250Hz)** which feel like realistic "Fine Texture" rather than a "Rumble."

By adding a slider, you allow the user to tune the pitch to their hardware's "sweet spot."

### Implementation Plan

We will add a new setting: **`Slide Frequency Scale`**.
*   **Range:** `0.5x` to `5.0x`.
*   **Default:** `1.0x` (Tuned for Belt/Rumble).
*   **Formula:** `(10Hz + Velocity * 5.0) * Scale`.

Here is the code to implement this feature.

#### 1. Update `FFBEngine.h`
Add the variable and update the formula.

```cpp
class FFBEngine {
public:
    // ... existing settings ...
    float m_slide_texture_gain = 2.0f; 
    float m_slide_freq_scale = 1.0f; // NEW: Frequency Multiplier

    // ... inside calculate_force ...
        if (m_slide_texture_enabled) {
            // ... [Slip Calculation] ...
            
            if (effective_slip_vel > 0.5) {
                // BASE FORMULA: 10Hz start, +5Hz per m/s. (Rumble optimized)
                double base_freq = 10.0 + (effective_slip_vel * 5.0);
                
                // APPLY USER SCALE
                double freq = base_freq * (double)m_slide_freq_scale;
                
                // Cap at 250Hz (Nyquist safety)
                if (freq > 250.0) freq = 250.0;

                // PHASE ACCUMULATION
                m_slide_phase += freq * dt * TWO_PI;
                m_slide_phase = std::fmod(m_slide_phase, TWO_PI); 

                // ... [Rest of logic] ...
            }
        }
```

#### 2. Update `src/Config.h`
Add the field to the Preset struct.

```cpp
struct Preset {
    // ...
    float slide_gain = 0.5f;
    float slide_freq = 1.0f; // NEW
    // ...

    // Setter
    Preset& SetSlide(bool enabled, float g, float f = 1.0f) { 
        slide_enabled = enabled; 
        slide_gain = g; 
        slide_freq = f; // Update this
        return *this; 
    }

    // Apply
    void Apply(FFBEngine& engine) const {
        // ...
        engine.m_slide_texture_gain = slide_gain;
        engine.m_slide_freq_scale = slide_freq; // Apply
        // ...
    }

    // UpdateFromEngine
    void UpdateFromEngine(const FFBEngine& engine) {
        // ...
        slide_gain = engine.m_slide_texture_gain;
        slide_freq = engine.m_slide_freq_scale; // Capture
        // ...
    }
};
```

#### 3. Update `src/Config.cpp`
Update the Save/Load logic and the Presets.

```cpp
// In LoadPresets(), update the "Guide: Slide Texture" preset to use 1.0 freq
presets.push_back(Preset("Guide: Slide Texture (Scrub)", true)
    // ...
    .SetSlide(true, 20.0f, 1.0f) // Gain 20, Freq 1.0
    // ...
);

// In Save()
file << "slide_freq=" << engine.m_slide_freq_scale << "\n";

// In Load()
else if (key == "slide_freq") engine.m_slide_freq_scale = std::stof(value);
```

#### 4. Update `src/GuiLayer.cpp`
Add the slider under the Slide Gain.

```cpp
    if (engine.m_slide_texture_enabled) {
        ImGui::Indent();
        FloatSetting("Slide Gain", &engine.m_slide_texture_gain, 0.0f, 20.0f);
        
        // NEW SLIDER
        FloatSetting("Slide Pitch (Freq)", &engine.m_slide_freq_scale, 0.5f, 5.0f, "%.1fx");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Adjusts the vibration pitch.\n1.0 = Low Rumble (Best for T300/G29)\n3.0+ = High Buzz (Best for Direct Drive)");
        
        ImGui::Unindent();
    }
```

### Result
*   **For your T300:** Leave it at **1.0**. You will feel the heavy rumble.
*   **For DD Users:** They can crank it to **3.0** or **4.0** to feel the fine grain of the asphalt.