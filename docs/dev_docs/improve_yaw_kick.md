# Yaw Kick Improvement: Gamma Response Curve

**Version**: v0.6.36  
**Date**: 2026-01-07  
**Status**: Ready for Implementation

---

## User Question

Below is the full codebase and documentation of a FFB app I am developing. I need you to help me investigate and improve an issue. (Note: in your responses, never produce full code files, always produce only code snippets of the required changes).

The issue is with the effect called "Yaw Kick". As I understand it, this effect is only based on the yaw acceleration (plus some coefficients for things like normalization).

The effect was originally designed to give a feel (a "kick") when the rear of the car steps out, in order to help catch slides immediately.
At some point, a slider was added to tune this effect, establishing a threshold according to which below a certain acceleration, no yaw acceleration was transmitted to the FFB. However, some users later reported that the FFB signal for yaw at lower acceleration was not "noise", but it was actually giving useful information about "road details".

Your task is to help me determine how to improve the FFB effects / signal that we could get from yaw acceleration. Please give me first your own suggestions, independently from my proposals.

I have in particular the following ideas and questions:
  * should we do some form of "equalization" or "gamma" transformation to the yaw effect? With that I mean something like: for yaw acceleration below a certain threshold, transmit this signal with a certain force, while for those above the threshold, with another threshold. We could have an "equalizer like UI", that helps us increase or decrease the intensity of force at particular levels of yaw acceleration. We could have multiple "thresholds", establishing multiple "bands" of yaw acceleration that we want to adjust independently.
* can we really separate the yaw signal that means "road detail", from those that mean "slide starting"?

---

## Analysis & Recommendations

**1. Can we separate "Road Detail" from "Slide Starting"?**

Yes, primarily by **Amplitude (Magnitude)**.

*   **Road Details**: Typically manifest as **low-amplitude**, high-frequency fluctuations in yaw acceleration.
    *   *Estimated Range*: 0.2 to 1.0 rad/s² (pending telemetry validation)
*   **Slide Initiation**: Manifests as a **high-amplitude** impulse or spike.
    *   *Estimated Range*: > 2.0 rad/s² (pending telemetry validation)

> ⚠️ **Note**: These amplitude ranges are estimated based on physics intuition. Actual values should be validated via in-game telemetry logging before finalizing defaults.

**2. Should we use Equalization or Gamma?**

I recommend implementing a **Gamma (Response Curve)** control. This is superior to a multi-band equalizer because it provides a continuous, smooth transition between "Road Detail" and "Slide Kick" without creating artificial steps or cutoffs.

*   **Gamma < 1.0 (e.g., 0.5)**: Boosts low-amplitude signals (Road Details) while keeping high-amplitude signals (Slides) relatively normal. This makes the road texture "pop" without making the slide kicks dangerously strong.
*   **Gamma > 1.0 (e.g., 2.0)**: Suppresses low-amplitude signals (Noise/Road) and focuses purely on the high-amplitude kicks. This is ideal for users who want a "clean" wheel that only reacts to slides.

**3. The Role of the Threshold**

The existing `Activation Threshold` should be kept but treated strictly as a **Noise Gate** (Deadzone) to eliminate sensor jitter or engine vibration (very low amplitude noise, e.g., < 0.1 rad/s²). The Gamma curve will then operate on the signal *above* this threshold.

---

## Implementation Plan

I will implement a **"Kick Gamma"** slider for the Yaw effect.

### 1. Update `src/FFBEngine.h`

Add the gamma variable and implement the power function logic in the force calculation.

```cpp
// In FFBEngine class public members (around line 254)
    // ... existing members ...
    float m_yaw_kick_threshold = 0.2f; // Existing v0.6.10
    float m_yaw_kick_gamma = 1.0f;     // NEW v0.6.36: Response Curve (1.0 = Linear)

// NEW: Physics constant for gamma normalization (add near other constants around line 175)
    // Reference yaw acceleration for gamma curve normalization.
    // 10.0 rad/s² represents a typical maximum yaw acceleration for a GT3 car
    // at the limit of adhesion (spinning out). This ensures the gamma curve
    // has a meaningful range where 0.1 normalized = road detail, 1.0 = full spin.
    static constexpr double YAW_ACCEL_REFERENCE = 10.0; // rad/s²

// In calculate_force method (Yaw Acceleration section, around line 1226)
        // ...
        // v0.6.10: Configurable Noise Gate (Activation Threshold)
        else if (std::abs(raw_yaw_accel) < (double)m_yaw_kick_threshold) {
            raw_yaw_accel = 0.0;
        }
        else {
            // NEW v0.6.36: Apply Gamma Curve (Response Linearity)
            // Gamma < 1.0 boosts low signals (Road Detail).
            // Gamma > 1.0 suppresses low signals (Focus on Slide).
            if (m_yaw_kick_gamma != 1.0f) {
                double norm = std::abs(raw_yaw_accel) / YAW_ACCEL_REFERENCE;
                norm = std::pow(norm, (double)m_yaw_kick_gamma);
                raw_yaw_accel = (raw_yaw_accel > 0.0 ? 1.0 : -1.0) * norm * YAW_ACCEL_REFERENCE;
            }
        }
        
        // Apply Smoothing (Low Pass Filter)
        // ...
```

### 2. Update `src/Config.h`

Add the new setting to the `Preset` structure in **three places**:

#### 2a. Field Declaration (around line 92)

```cpp
    float yaw_kick_threshold = 0.0f; // Existing v0.6.10
    float yaw_kick_gamma = 1.0f;     // NEW v0.6.36: Response Curve
```

#### 2b. Fluent Setter (around line 163)

```cpp
    Preset& SetYawKickThreshold(float v) { yaw_kick_threshold = v; return *this; } // Existing
    Preset& SetYawKickGamma(float v) { yaw_kick_gamma = v; return *this; }         // NEW v0.6.36
```

#### 2c. Apply() Method (around line 247)

```cpp
        engine.m_yaw_kick_threshold = yaw_kick_threshold;
        engine.m_yaw_kick_gamma = yaw_kick_gamma; // NEW v0.6.36
```

#### 2d. UpdateFromEngine() Method (around line 308)

```cpp
        yaw_kick_threshold = engine.m_yaw_kick_threshold;
        yaw_kick_gamma = engine.m_yaw_kick_gamma; // NEW v0.6.36
```

### 3. Update `src/Config.cpp`

Handle persistence (Save/Load) for both main config and user presets.

#### 3a. Main Config Save (around line 825)

```cpp
        // --- Rear Axle ---
        file << "yaw_kick_threshold=" << engine.m_yaw_kick_threshold << "\n";
        file << "yaw_kick_gamma=" << engine.m_yaw_kick_gamma << "\n"; // NEW v0.6.36
```

#### 3b. Main Config Load (around line 1042)

```cpp
                    else if (key == "yaw_kick_threshold") engine.m_yaw_kick_threshold = std::stof(value);
                    else if (key == "yaw_kick_gamma") engine.m_yaw_kick_gamma = std::stof(value); // NEW v0.6.36
```

#### 3c. Main Config Load Validation (add after other validations)

```cpp
    // Validate yaw kick gamma (v0.6.36)
    if (engine.m_yaw_kick_gamma < 0.1f || engine.m_yaw_kick_gamma > 3.0f) {
        engine.m_yaw_kick_gamma = (std::max)(0.1f, (std::min)(3.0f, engine.m_yaw_kick_gamma));
    }
```

#### 3d. User Preset Save (around line 896)

```cpp
                file << "yaw_kick_threshold=" << p.yaw_kick_threshold << "\n";
                file << "yaw_kick_gamma=" << p.yaw_kick_gamma << "\n"; // NEW v0.6.36
```

#### 3e. User Preset Load (around line 725)

```cpp
                        else if (key == "yaw_kick_threshold") current_preset.yaw_kick_threshold = std::stof(value);
                        else if (key == "yaw_kick_gamma") current_preset.yaw_kick_gamma = std::stof(value); // NEW v0.6.36
```

### 4. Update `src/GuiLayer.cpp`

Add the slider to the GUI in the "Rear Axle" section.

```cpp
// In DrawTuningWindow, inside "Rear Axle (Oversteer)" section (around line 1013)

        FloatSetting("Yaw Kick", &engine.m_sop_yaw_gain, 0.0f, 1.0f, FormatDecoupled(engine.m_sop_yaw_gain, FFBEngine::BASE_NM_YAW_KICK), "This is the earliest cue for rear stepping out. It's a sharp, momentary impulse that signals the onset of rotation.\nBased on Yaw Acceleration.");
        
        // Indent advanced yaw settings for visual hierarchy
        ImGui::Indent();
        
        FloatSetting("Activation Threshold", &engine.m_yaw_kick_threshold, 0.0f, 10.0f, "%.2f rad/s²", 
            "Minimum yaw acceleration required to trigger the kick.\nActs as a Noise Gate.\nIncrease to filter out engine vibration and small bumps.");
        
        // NEW v0.6.36: Gamma Response Curve
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

### 5. Update `docs/dev_docs/FFB_formulas.md`

Add gamma curve documentation to the Yaw Acceleration section (around line 103).

```markdown
3.  **Yaw Acceleration ("The Kick")**:
    *   **Input**: `mLocalRotAccel.y` (rad/s²). **Note**: Inverted (-1.0) to comply with SDK requirement to negate rotation data.
    *   **Conditioning**:
        *   **Low Speed Cutoff**: 0.0 if Speed < 5.0 m/s.
        *   **Activation Threshold**: 0.0 if $|Accel| < m_{\text{yaw-kick-threshold}}$ rad/s². 
            *   *Default*: 0.2 rad/s². Configurable to filter road noise.
        *   **Response Curve (Gamma)** *(NEW v0.6.36)*:
            *   Applied after threshold gating to shape the amplitude response.
            *   $\text{YawAccel}_{\text{gamma}} = \text{Sign}(\text{Accel}) \times \left(\frac{|\text{Accel}|}{10.0}\right)^{\gamma} \times 10.0$
            *   *Default*: 1.0 (Linear). Range: 0.1 - 3.0.
            *   Gamma < 1.0: Boosts low-amplitude signals (road detail feel).
            *   Gamma > 1.0: Suppresses low-amplitude signals (slide focus).
    *   **Rationale**: Requires heavy smoothing (LPF) to separate true chassis rotation from "Slide Texture" vibration noise, preventing a feedback loop where vibration is mistaken for rotation.
    *   **Formula**: $-\text{YawAccel}_{\text{smooth}} \times K_{\text{yaw}} \times 5.0 \text{Nm} \times K_{\text{decouple}}$.
    *   **Max Clamp**: 1.0 (Updated v0.6.20).
    *   **Note**: Negative sign provides counter-steering torque.
```

### 6. Update `tests/test_ffb_engine.cpp`

Add a unit test to verify the Gamma curve behavior.

```cpp
static void test_yaw_kick_gamma() {
    std::cout << "\nTest: Yaw Kick Gamma Response" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(20.0);
    
    engine.m_sop_yaw_gain = 1.0f;
    engine.m_yaw_kick_threshold = 0.0f; // Disable gate
    engine.m_yaw_accel_smoothing = 0.0f; // Disable smoothing for deterministic test
    
    // Reference Input: 1.0 rad/s^2 (Small/Medium signal)
    // Normalized: 1.0 / 10.0 = 0.1
    data.mLocalRotAccel.y = 1.0;
    
    // Helper: Run multiple frames to let any remaining LPF settle
    auto settle_and_measure = [&]() -> double {
        engine.m_yaw_accel_smoothed = 0.0; // Reset LPF state
        for (int i = 0; i < 10; i++) {
            engine.calculate_force(&data);
        }
        return std::abs(engine.calculate_force(&data));
    };
    
    // Case 1: Linear (Gamma 1.0)
    // pow(0.1, 1.0) = 0.1
    engine.m_yaw_kick_gamma = 1.0f;
    double f_linear = settle_and_measure();
    
    // Case 2: Boost Low End (Gamma 0.5)
    // pow(0.1, 0.5) = 0.316 (~3x boost)
    engine.m_yaw_kick_gamma = 0.5f;
    double f_boost = settle_and_measure();
    
    // Case 3: Suppress Low End (Gamma 2.0)
    // pow(0.1, 2.0) = 0.01 (~10x suppression)
    engine.m_yaw_kick_gamma = 2.0f;
    double f_suppress = settle_and_measure();
    
    std::cout << "  Boost (γ=0.5): " << f_boost << std::endl;
    std::cout << "  Linear (γ=1.0): " << f_linear << std::endl;
    std::cout << "  Suppress (γ=2.0): " << f_suppress << std::endl;
    
    if (f_boost > f_linear && f_linear > f_suppress) {
        std::cout << "[PASS] Yaw Gamma modifies signal correctly (Boost > Linear > Suppress)." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Yaw Gamma logic failed. Expected: Boost > Linear > Suppress" << std::endl;
        g_tests_failed++;
    }
}

// Add to Run() function:
// test_yaw_kick_gamma();
```

---

## Changelog Entry

Add to `CHANGELOG.md`:

```markdown
### v0.6.36 - Yaw Kick Gamma Response

**New Features:**
- **Yaw Kick Response Curve (Gamma)**: New slider to control the amplitude response of the Yaw Kick effect.
  - Gamma < 1.0: Boosts low-amplitude yaw signals, making road details and micro-corrections more perceptible.
  - Gamma = 1.0: Linear response (default, unchanged behavior).
  - Gamma > 1.0: Suppresses low-amplitude signals, focusing the effect on large slide kicks only.
- Added physics constant `YAW_ACCEL_REFERENCE` (10.0 rad/s²) for gamma normalization.

**Technical:**
- New setting: `yaw_kick_gamma` (default 1.0, range 0.1-3.0).
- Updated FFB_formulas.md with gamma curve documentation.
- Added regression test `test_yaw_kick_gamma()`.
```

---

## Summary of Changes

| File | Change |
|------|--------|
| `src/FFBEngine.h` | Add `m_yaw_kick_gamma`, `YAW_ACCEL_REFERENCE`, gamma logic in `calculate_force()` |
| `src/Config.h` | Add `yaw_kick_gamma` field, `SetYawKickGamma()` setter, update `Apply()` and `UpdateFromEngine()` |
| `src/Config.cpp` | Add save/load for main config and user presets |
| `src/GuiLayer.cpp` | Add "Response Curve (Gamma)" slider with indentation |
| `docs/dev_docs/FFB_formulas.md` | Add gamma curve formula and documentation |
| `tests/test_ffb_engine.cpp` | Add `test_yaw_kick_gamma()` regression test |
| `CHANGELOG.md` | Add v0.6.36 entry |

---

## Verification Steps

1. **Build**: Compile the project and verify no errors.
2. **Test**: Run `test_ffb_engine.exe` and verify `test_yaw_kick_gamma` passes.
3. **GUI**: Launch the app and verify the new slider appears under "Rear Axle (Oversteer)".
4. **Persistence**: 
   - Set gamma to 0.5, close and reopen the app, verify it persists.
   - Create a user preset, verify gamma is saved and loaded correctly.
5. **In-Game**: Test with LMU:
   - Gamma 0.5: Road bumps should feel more pronounced in yaw.
   - Gamma 2.0: Only big slides should trigger noticeable yaw kicks.

---

## Appendix: Research Query

The following query can be used with a deep research tool to validate the physics assumptions and gather additional insights:

---

### Deep Research Query: Yaw Acceleration in Racing Simulation FFB

**Context:**
I am developing a Force Feedback (FFB) application for racing simulators (specifically Le Mans Ultimate / rFactor 2). The application processes telemetry data including yaw acceleration (`mLocalRotAccel.y` in rad/s²) to provide a "Yaw Kick" effect through the steering wheel. This effect is designed to alert drivers when the rear of the car begins to slide (oversteer onset).

**Research Objectives:**

1. **Validate Amplitude Ranges:**
   - What are typical yaw acceleration magnitudes experienced by a GT3 or prototype race car during:
     - Normal cornering on a smooth track surface?
     - Driving over road imperfections (curbs, bumps, surface texture variations)?
     - The onset of oversteer / rear slide initiation (before full spin)?
     - A full spin or loss of control?
   - I have estimated:
     - **Road Details / Micro-corrections**: 0.2 to 1.0 rad/s²
     - **Slide Initiation**: > 2.0 rad/s²
   - Are these estimates reasonable? What ranges would you suggest based on vehicle dynamics literature or telemetry data?

2. **Signal Characteristics:**
   - How can yaw acceleration signals from "road texture" (high-frequency, low-amplitude) be distinguished from "slide onset" (impulsive, high-amplitude) in real-time signal processing?
   - Are there frequency-domain characteristics that differentiate these? (e.g., road texture at 20-100Hz vs. slide onset at 1-5Hz?)
   - Would a gamma/power-law transformation be an effective method to separate these signals, or are there better approaches (e.g., band-pass filtering, envelope detection, derivative analysis)?

3. **Best Practices for Yaw-to-FFB Mapping:**
   - In professional racing simulators or commercial FFB systems (Simucube, Fanatec, Logitech TrueForce), how is yaw information typically used in the FFB signal?
   - Are there published papers or patents on yaw-based FFB effects in driving simulators?
   - What human perception thresholds exist for rotational acceleration cues through a steering wheel? (i.e., what is the minimum perceptible yaw-induced torque?)

4. **Reference Values:**
   - What is a reasonable "maximum reference" yaw acceleration for normalizing the signal? I am using 10.0 rad/s² as the maximum expected value for a car at the limit. Is this appropriate for GT3/LMP cars?
   - Are there SAE, ISO, or academic standards for yaw rate/acceleration in vehicle dynamics that could inform these thresholds?

5. **Related Concepts:**
   - How does yaw acceleration relate to other oversteer indicators (sideslip angle, yaw rate vs. steering angle, rear tire slip angle)?
   - Could combining yaw acceleration with rear slip angle provide a more robust "slide detection" algorithm?

**Desired Output:**
- Specific numerical ranges for yaw acceleration in different driving conditions (with sources if available).
- Recommendations for real-time signal processing techniques to separate road texture from slide events.
- Any academic papers, industry whitepapers, or simulator documentation discussing yaw-based FFB effects.
- Suggested improvements to my gamma-curve approach, or alternative algorithms.

---

*Query created: 2026-01-07*


# Adaptive smoothing for Yaw

## User question

Should we have a smoothing that differentiates between different values of yaw (eg. with a threshold)? What I mean is, having something that smooths out lower acceleration value, and does not smooth out higher yaw values (or viceversa). This is in the case in which different types of yaw signal require different levels of smoothing (eg. road noise might require more, while slides less).

## Answer 

This is an excellent idea. What you are describing is known in signal processing as **Adaptive Smoothing** or **Magnitude-Dependent Filtering**.

It is superior to a simple "Threshold/Gate" because:
1.  **Hard Threshold (Current):** Below 0.2, the signal is **0.0** (Dead). You feel nothing.
2.  **Adaptive Smoothing (Proposed):** Below 0.2, the signal is **heavily smoothed** (e.g., 50ms latency). You feel a subtle, smooth "weight" or "texture" (Road Detail) without the jagged noise.
3.  **The Kick:** Above 0.2, the smoothing drops to **0ms** (or whatever the user set). You feel the slide instantly.

### The Logic: "The Harder the Kick, the Faster the Filter"

We can implement a dynamic Time Constant ($\tau$) that changes based on the input magnitude.

*   **Input:** `Yaw Acceleration`
*   **User Setting:** `Kick Response` (Let's say 0ms / Raw).
*   **Logic:**
    *   **Low Input (< 1.0 rad/s²):** We force a high smoothing (e.g., **50ms**). This turns "jittery noise" into "smooth road information."
    *   **High Input (> 5.0 rad/s²):** We use the **User Setting** (0ms). This ensures zero latency when catching a slide.
    *   **Transition:** Linearly blend between them.

### Implementation Plan

We can modify the existing `calculate_force` logic to make the smoothing dynamic. We don't even need a new slider; we can just use a sensible "Noise Smoothing" constant (like 50ms) and blend to the user's "Performance Smoothing" setting.

#### 1. Update `src/FFBEngine.h`

We will modify the Yaw Kick section. We will effectively replace the "Hard Cut" threshold logic with this "Soft Blend" logic, or use the threshold to define the blend range.

```cpp
// In calculate_force...

// ... [Yaw Kick Logic] ...

double raw_yaw_accel = data->mLocalRotAccel.y;

// 1. Low Speed Cutoff (Keep this, we don't want idle vibration)
if (car_v_long < 5.0) {
    raw_yaw_accel = 0.0;
} 
else {
    // 2. ADAPTIVE SMOOTHING (The Fix)
    
    // Define the range of acceleration where we transition from "Noise" to "Signal"
    // We can use the existing m_yaw_kick_threshold as the "Start of the Kick"
    // Default threshold is usually 0.2 to 1.0.
    double noise_floor = (double)m_yaw_kick_threshold; 
    if (noise_floor < 0.5) noise_floor = 0.5; // Ensure a reasonable blend window
    
    double magnitude = std::abs(raw_yaw_accel);
    
    // Calculate Blend Factor (0.0 = Noise, 1.0 = Kick)
    // If mag < noise_floor, we are in noise territory.
    // If mag > noise_floor * 4, we are in full kick territory.
    double blend = (magnitude) / (noise_floor * 4.0);
    blend = (std::max)(0.0, (std::min)(1.0, blend));
    
    // Define Smoothing Targets
    double slow_tau = 0.050; // 50ms (Smooths out road noise/jitter)
    double fast_tau = (double)m_yaw_accel_smoothing; // User setting (e.g. 0ms)
    
    // Interpolate Tau
    // Low Signal -> Slow Tau. High Signal -> Fast Tau.
    double dynamic_tau = slow_tau + (fast_tau - slow_tau) * blend;
    
    // Apply Smoothing with Dynamic Tau
    if (dynamic_tau < 0.0001) dynamic_tau = 0.0001;
    double alpha_yaw = dt / (dynamic_tau + dt);
    
    m_yaw_accel_smoothed += alpha_yaw * (raw_yaw_accel - m_yaw_accel_smoothed);
    
    // Apply Gamma (Optional, if you still want it)
    // ...
}
```

### Benefits of this approach

1.  **No Latency on Slides:** When a real slide happens (high acceleration), the filter opens up instantly.
2.  **Road Feel Restored:** Instead of cutting small signals to zero (deadzone), we smooth them. You will feel the chassis "breathing" and reacting to the road, but it won't feel like digital noise or spikes.
3.  **User Simplicity:** The user still only adjusts "Kick Response" (their desired performance). The "Noise Smoothing" happens automatically in the background.

### Recommendation

I recommend implementing this **Adaptive Smoothing** instead of the hard "Activation Threshold" cut.

*   **Current Behavior:** Signal < Threshold = 0. (Dead).
*   **New Behavior:** Signal < Threshold = Smoothed. (Detailed but calm).

Do you want me to generate the code changes to replace the hard threshold with this adaptive logic?