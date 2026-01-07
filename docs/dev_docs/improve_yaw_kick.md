
## User question

Below is the full codebase and documentation of a FFB app I am developing. I need you to help me investigate and improve an issue. (Note: in your responses, never produce full code files, always produce only code snippets of the required changes).

The issue is with the effect called "Yaw Kick". As I understand it, this effect is only based on the yaw acceleration (plus some coefficients for things like normalization).

The effect was originally designed to give a feel (a "kick") when the rear of the car steps out, in order to help catch slides immediately.
At some point, a slider was added to tune this effect, establishing a threshold according to which below a certain acceleration, no yaw acceleration was transmitted to the FFB. However, some users later reported that the FFB signal for yaw at lower acceleration was not "noise", but it was actually giving useful information about "road details".

Your task is to help me determine how to improve the FFB effects / signal that we could get from yaw acceleration. Please give me first your own suggestions, independently from my proposals.

I have in particular the following ideas and questions:
  * should we do some form of "equalization" or "gamma" transformation to the yaw effect? With that I mean something like: for yaw acceleration below a certain threshold, transmit this signal with a certain force, while for those above the threshold, with another threshold. We could have an "equalizer like UI", that helps us increase or decrease the intensity of force at particular levels of yaw acceleration. We could have multiple "thresholds", establishing multiple "bands" of yaw acceleration that we want to adjust independently.
* can we really separate the yaw signal that means "road detail", from those that mean "slide starting"?

## Answer 

To address the issue with the "Yaw Kick" effect and improve the FFB signal for both road details and slide initiation, I have analyzed the physics and signal characteristics.

### Analysis & Recommendations

**1. Can we separate "Road Detail" from "Slide Starting"?**
Yes, primarily by **Amplitude (Magnitude)**.
*   **Road Details**: Typically manifest as **low-amplitude**, high-frequency fluctuations in yaw acceleration (e.g., 0.2 to 1.0 rad/s²).
*   **Slide Initiation**: Manifests as a **high-amplitude** impulse or spike (e.g., > 2.0 rad/s²).

**2. Should we use Equalization or Gamma?**
I recommend implementing a **Gamma (Response Curve)** control. This is superior to a multi-band equalizer because it provides a continuous, smooth transition between "Road Detail" and "Slide Kick" without creating artificial steps or cutoffs.

*   **Gamma < 1.0 (e.g., 0.5)**: Boosts low-amplitude signals (Road Details) while keeping high-amplitude signals (Slides) relatively normal. This makes the road texture "pop" without making the slide kicks dangerously strong.
*   **Gamma > 1.0 (e.g., 2.0)**: Suppresses low-amplitude signals (Noise/Road) and focuses purely on the high-amplitude kicks. This is ideal for users who want a "clean" wheel that only reacts to slides.

**3. The Role of the Threshold**
The existing `Activation Threshold` should be kept but treated strictly as a **Noise Gate** (Deadzone) to eliminate sensor jitter or engine vibration (very low amplitude noise, e.g., < 0.1 rad/s²). The Gamma curve will then operate on the signal *above* this threshold.

---

### Implementation Plan

I will implement a **"Kick Gamma"** slider for the Yaw effect.

#### 1. Update `src/FFBEngine.h`
Add the gamma variable and implement the power function logic in the force calculation.

```cpp
// In FFBEngine class public members
    // ... existing members ...
    float m_yaw_kick_threshold = 0.2f; 
    float m_yaw_kick_gamma = 1.0f; // NEW: Response Curve (1.0 = Linear)

// In calculate_force method (Yaw Acceleration section)
        // ...
        // v0.6.10: Configurable Noise Gate (Activation Threshold)
        else if (std::abs(raw_yaw_accel) < (double)m_yaw_kick_threshold) {
            raw_yaw_accel = 0.0;
        }
        else {
            // NEW: Apply Gamma Curve (Response Linearity)
            // We normalize against a reference value (e.g., 10.0 rad/s^2) to apply the curve correctly.
            // Gamma < 1.0 boosts low signals (Road Detail).
            // Gamma > 1.0 suppresses low signals (Focus on Slide).
            if (m_yaw_kick_gamma != 1.0f) {
                double ref_accel = 10.0; 
                double norm = std::abs(raw_yaw_accel) / ref_accel;
                norm = std::pow(norm, (double)m_yaw_kick_gamma);
                raw_yaw_accel = (raw_yaw_accel > 0.0 ? 1.0 : -1.0) * norm * ref_accel;
            }
        }
        
        // Apply Smoothing (Low Pass Filter)
        // ...
```

#### 2. Update `src/Config.h`
Add the new setting to the `Preset` structure.

```cpp
struct Preset {
    // ... existing members ...
    float yaw_kick_threshold = 0.0f;
    float yaw_kick_gamma = 1.0f; // NEW

    // ... existing setters ...
    Preset& SetYawKick(float gain, float threshold, float gamma = 1.0f) { 
        sop_yaw_gain = gain; 
        yaw_kick_threshold = threshold;
        yaw_kick_gamma = gamma;
        return *this; 
    }

    // Update Apply and UpdateFromEngine
    void Apply(FFBEngine& engine) const {
        // ...
        engine.m_yaw_kick_threshold = yaw_kick_threshold;
        engine.m_yaw_kick_gamma = yaw_kick_gamma; // NEW
        // ...
    }

    void UpdateFromEngine(const FFBEngine& engine) {
        // ...
        yaw_kick_threshold = engine.m_yaw_kick_threshold;
        yaw_kick_gamma = engine.m_yaw_kick_gamma; // NEW
        // ...
    }
};
```

#### 3. Update `src/Config.cpp`
Handle persistence (Save/Load) and default values.

```cpp
// In Config::LoadPresets()
    // Update Default Preset
    presets.push_back(Preset("Default", true)); // Will use default 1.0f

// In Config::Save()
    file << "yaw_kick_gamma=" << engine.m_yaw_kick_gamma << "\n";

// In Config::Load()
    else if (key == "yaw_kick_gamma") engine.m_yaw_kick_gamma = std::stof(value);

// In Config::Load() validation section
    if (engine.m_yaw_kick_gamma < 0.1f || engine.m_yaw_kick_gamma > 3.0f) {
        engine.m_yaw_kick_gamma = (std::max)(0.1f, (std::min)(3.0f, engine.m_yaw_kick_gamma));
    }
```

#### 4. Update `src/GuiLayer.cpp`
Add the slider to the GUI in the "Rear Axle" section.

```cpp
// In DrawTuningWindow, inside "Rear Axle (Oversteer)"

        FloatSetting("Yaw Kick", &engine.m_sop_yaw_gain, 0.0f, 1.0f, FormatDecoupled(engine.m_sop_yaw_gain, FFBEngine::BASE_NM_YAW_KICK), "This is the earliest cue for rear stepping out. It's a sharp, momentary impulse that signals the onset of rotation.\nBased on Yaw Acceleration.");
        
        // Indent advanced yaw settings
        ImGui::Indent();
        FloatSetting("Activation Threshold", &engine.m_yaw_kick_threshold, 0.0f, 5.0f, "%.2f rad/s²", "Minimum yaw acceleration required to trigger the kick.\nActs as a Noise Gate.\nIncrease to filter out engine vibration and small bumps.");
        
        FloatSetting("Response Curve (Gamma)", &engine.m_yaw_kick_gamma, 0.1f, 3.0f, "%.2f", 
            "Controls the linearity of the Yaw Kick.\n\n"
            "  1.0 = Linear (Standard)\n"
            "  <1.0 = Boost Low Range (Feel road details/micro-yaw)\n"
            "  >1.0 = Suppress Low Range (Focus only on big slide kicks)");
            
        FloatSetting("Kick Response (Smooth)", &engine.m_yaw_accel_smoothing, 0.000f, 0.050f, "%.3f s",
            "Low Pass Filter for the Yaw Kick signal.\nSmoothes out kick noise.\nLower = Sharper/Faster kick.\nHigher = Duller/Softer kick.",
            [&]() {
                int ms = (int)(engine.m_yaw_accel_smoothing * 1000.0f + 0.5f);
                ImVec4 color = (ms <= 15) ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
                ImGui::TextColored(color, "Latency: %d ms", ms);
            });
        ImGui::Unindent();
```

#### 5. Update `tests/test_ffb_engine.cpp`
Add a unit test to verify the Gamma curve behavior.

```cpp
static void test_yaw_kick_gamma() {
    std::cout << "\nTest: Yaw Kick Gamma Response" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    
    engine.m_sop_yaw_gain = 1.0f;
    engine.m_yaw_kick_threshold = 0.0f; // Disable gate
    engine.m_yaw_accel_smoothing = 0.0f; // Disable smoothing
    
    // Reference Input: 1.0 rad/s^2 (Small/Medium signal)
    data.mLocalRotAccel.y = 1.0;
    
    // Case 1: Linear (Gamma 1.0)
    engine.m_yaw_kick_gamma = 1.0f;
    double f_linear = std::abs(engine.calculate_force(&data));
    
    // Case 2: Boost Low End (Gamma 0.5)
    // Input 1.0 / Ref 10.0 = 0.1.
    // pow(0.1, 0.5) = 0.316.
    // Output should be ~3x stronger than linear.
    engine.m_yaw_kick_gamma = 0.5f;
    double f_boost = std::abs(engine.calculate_force(&data));
    
    // Case 3: Suppress Low End (Gamma 2.0)
    // pow(0.1, 2.0) = 0.01.
    // Output should be ~10x weaker than linear.
    engine.m_yaw_kick_gamma = 2.0f;
    double f_suppress = std::abs(engine.calculate_force(&data));
    
    if (f_boost > f_linear && f_linear > f_suppress) {
        std::cout << "[PASS] Yaw Gamma modifies signal correctly (Boost > Linear > Suppress)." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Yaw Gamma logic failed. B:" << f_boost << " L:" << f_linear << " S:" << f_suppress << std::endl;
        g_tests_failed++;
    }
}

// Add to Run()
// test_yaw_kick_gamma();
```