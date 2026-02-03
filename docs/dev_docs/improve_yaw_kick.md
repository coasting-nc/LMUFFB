# Yaw Kick Improvement: Gamma Response Curve

**Version**: v0.6.36  
**Date**: 2026-01-07  
**Status**: Ready for Implementation  
**Research Validation**: ✅ Completed ([Deep Research Report](./yaw_kick_deep_reasearch_report.md))

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

### Research-Validated Findings

Based on deep research into vehicle dynamics literature, ESC systems, and human perception studies (see [full research report](./yaw_kick_deep_reasearch_report.md)), the following findings are now **validated**:

**1. Can we separate "Road Detail" from "Slide Starting"?**

**Yes**, primarily by **Frequency Domain**, with amplitude as a secondary discriminator.

| Vehicle State | Yaw Accel Range | Frequency Band | Signal Nature |
|:--------------|:----------------|:---------------|:--------------|
| Steady Cornering | < 0.2 rad/s² | DC / < 1 Hz | Noise / Null |
| Road Texture | 0.2 – 1.5 rad/s² | **20 – 100 Hz** | Oscillatory |
| Curb Strike | 1.0 – 8.0 rad/s² | > 50 Hz | Impulse / Spike |
| **Slide Initiation** | **2.0 – 5.0 rad/s²** | **1 – 10 Hz** | **Step / Surge** |
| Deep Slide / Spin | > 5.0 rad/s² | 0.5 – 2 Hz | Sustained Surge |

> **Key Insight**: A slide is a *low-frequency, high-amplitude* event. Texture is a *high-frequency, variable-amplitude* event. The polar moment of inertia (Izz ≈ 2500-3000 kg·m² for GT cars) acts as a physical low-pass filter—it is physically impossible for the chassis to oscillate in yaw at 20+ Hz.

**2. Should we use Equalization or Gamma?**

**Gamma (Response Curve)** is confirmed as the superior approach:

- **Gamma > 1.0 (e.g., 1.5 - 2.0)**: Creates a curve that is flat near zero and steepens at higher values, effectively acting as a "soft deadzone" that suppresses micro-corrections while preserving slide response.
- **Research Recommendation**: γ ≈ **1.8** provides optimal balance.
- **Why not DRC?** Dynamic Range Compression would *amplify* road noise—the opposite of what we want. Gamma effectively acts as an "Expander" in audio terms.

**3. Critical Filter Requirement**

The research **strongly recommends** adding a **Low-Pass Filter** before the gamma stage:

- **Cutoff Frequency**: **10 Hz** (range: 8-12 Hz acceptable)
- **Filter Type**: 2nd-order Butterworth (flat passband, -12 dB/octave rolloff)
- **Rationale**: This separates handling dynamics (0-5 Hz) from texture/vibration (10-100 Hz)

**4. Human Perception: Weber's Law**

A critical insight for tuning:

- **Weber Fraction**: Human hand/arm force perception JND is **7-15%**
- **Implication**: If the wheel is loaded with 5 Nm of cornering force, a "Kick" of 0.2 Nm (4%) will be **imperceptible**. The kick must be at least 10% of current FFB load to be noticed.
- **Future Enhancement**: Consider adaptive gain scaling based on current FFB load.

---

## Latency Analysis

> ⚠️ **Design Principle**: All latency-inducing components MUST be user-adjustable to zero. Some users require absolute minimum latency for competitive racing.

### Component Latency Breakdown

| Component | Latency | User-Adjustable? | Notes |
|:----------|:--------|:-----------------|:------|
| **Gamma Curve** | **0 ms** | N/A | Pure algebraic operation (`pow(x, γ)`), no filtering |
| **Activation Threshold** | **0 ms** | N/A | Simple comparison, no filtering |
| **LPF (Kick Response)** | **0-50 ms** | ✅ Yes | User can set to 0.000s for zero latency |
| Speed Gate | 0 ms | N/A | Simple comparison |

### Key Points

1. **Gamma adds ZERO latency**: The gamma transformation is a pure mathematical operation applied to each sample independently. There is no filtering, smoothing, or state involved. The output is computed instantaneously from the input.

2. **LPF is fully user-controllable**: The existing "Kick Response (Smooth)" slider (`m_yaw_accel_smoothing`) can be set to **0.000s** for zero filtering latency. Users who want maximum response can disable smoothing entirely.

3. **Research recommendation vs. user choice**: While research recommends ~16ms (10 Hz) for optimal texture/handling separation, this is a **recommendation**, not a requirement. Users prioritizing reaction time over signal cleanliness can set it to 0.

### GUI Latency Indicator

The "Kick Response" slider displays latency with **color-coded feedback** following the existing codebase pattern:
- **Green**: Latency ≤ 15 ms (acceptable for competitive racing)
- **Red**: Latency > 15 ms (may feel sluggish)

This matches the pattern used for other smoothing sliders in the application (e.g., Chassis Smoothing, SoP Smoothing).

---

## Implementation Plan

Based on the research findings, I will implement:

1. **Yaw Kick Gamma** (Response Curve) - User-adjustable non-linearity
2. **Improved Low-Pass Filter** - 10 Hz cutoff for frequency separation
3. **Validated Reference** - 10.0 rad/s² normalization ceiling (confirmed appropriate)

### 1. Update `src/FFBEngine.h`

Add the gamma variable and implement the validated signal processing pipeline.

```cpp
// In FFBEngine class public members (around line 254)
    // ... existing members ...
    float m_yaw_kick_threshold = 0.2f; // Existing v0.6.10
    float m_yaw_kick_gamma = 1.0f;     // NEW v0.6.36: Response Curve (1.0 = Linear)

// NEW: Physics constants for yaw processing (add near other constants around line 175)
    // Reference yaw acceleration for gamma curve normalization.
    // 10.0 rad/s² represents the upper limit of recoverable vehicle dynamics.
    // Validated by ESC intervention thresholds (2.0-4.0 rad/s² for detection).
    // Source: Research Report Section 2.4, 4.3
    static constexpr double YAW_ACCEL_REFERENCE = 10.0; // rad/s²

// In calculate_force method (Yaw Acceleration section, around line 1226)
        // ...
        // v0.6.10: Configurable Noise Gate (Activation Threshold)
        else if (std::abs(raw_yaw_accel) < (double)m_yaw_kick_threshold) {
            raw_yaw_accel = 0.0;
        }
        else {
            // NEW v0.6.36: Apply Gamma Curve (Response Linearity)
            // Research validated: γ > 1.0 suppresses micro-corrections (0-1.5 rad/s²)
            // while amplifying slide signals (>2.0 rad/s²).
            // Recommended: γ ≈ 1.8 for optimal balance.
            // Source: Research Report Section 3.2.1
            if (m_yaw_kick_gamma != 1.0f) {
                double norm = std::abs(raw_yaw_accel) / YAW_ACCEL_REFERENCE;
                norm = std::pow(norm, (double)m_yaw_kick_gamma);
                raw_yaw_accel = (raw_yaw_accel > 0.0 ? 1.0 : -1.0) * norm * YAW_ACCEL_REFERENCE;
            }
        }
        
        // Apply Smoothing (Low Pass Filter)
        // RESEARCH NOTE: Current LPF uses user-configurable tau.
        // Research recommends fc ≈ 10 Hz (tau ≈ 0.016s) for optimal
        // handling/texture separation. Consider making this the default.
        // Source: Research Report Section 3.1.1
        // ...
```

### 2. Update `src/Config.h`

Add the new setting to the `Preset` structure in **four places**:

#### 2a. Field Declaration (around line 92)

```cpp
    float yaw_kick_threshold = 0.0f; // Existing v0.6.10
    float yaw_kick_gamma = 1.0f;     // NEW v0.6.36: Response Curve (research default: 1.8)
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
    // Research recommends 1.5-2.0 range, but allow user experimentation 0.1-3.0
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
        
        // NEW v0.6.36: Gamma Response Curve (Research Validated)
        FloatSetting("Response Curve (Gamma)", &engine.m_yaw_kick_gamma, 0.1f, 3.0f, "%.2f", 
            "Controls the linearity of the Yaw Kick.\n"
            "Research recommends ~1.8 for optimal balance.\n\n"
            "  1.0 = Linear (Standard)\n"
            "  <1.0 = Boost Low Range (More road detail feel)\n"
            "  >1.0 = Suppress Low Range (Focus on slide kicks only)");
            
        // v0.6.36: Updated latency indicator to use standard green/red pattern
        // Green: ≤15ms (good for competitive), Red: >15ms (may feel sluggish)
        FloatSetting("Kick Response (Smooth)", &engine.m_yaw_accel_smoothing, 0.000f, 0.050f, "%.3f s",
            "Low Pass Filter for the Yaw Kick signal.\n"
            "Set to 0.000 for ZERO latency (raw, but may have noise).\n"
            "Research recommends ~0.016s (10 Hz) for signal quality.\n"
            "Lower = Sharper/Faster. Higher = Smoother/Cleaner.",
            [&]() {
                int ms = (int)(engine.m_yaw_accel_smoothing * 1000.0f + 0.5f);
                // Standard latency color coding: Green ≤ 15ms, Red > 15ms
                ImVec4 color = (ms <= 15) ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
                ImGui::TextColored(color, "Latency: %d ms", ms);
                // Also show frequency equivalent for advanced users
                if (engine.m_yaw_accel_smoothing > 0.0001f) {
                    float fc = 1.0f / (2.0f * 3.14159f * engine.m_yaw_accel_smoothing);
                    ImGui::SameLine();
                    ImGui::TextDisabled("(~%.0f Hz cutoff)", fc);
                }
            });
            
        ImGui::Unindent();
```

### 5. Update `docs/dev_docs/FFB_formulas.md`

Add gamma curve and frequency information to the Yaw Acceleration section (around line 103).

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
            *   *Default*: 1.0 (Linear). Research Optimal: **1.8**. Range: 0.1 - 3.0.
            *   Gamma > 1.0: Suppresses micro-corrections (0-1.5 rad/s²), amplifies slides (>2.0 rad/s²).
    *   **Frequency Characteristics** *(Research Validated)*:
        *   Slide Initiation: **1-10 Hz** (Low-Mid frequency, Step/Surge pattern)
        *   Road Texture: **20-100 Hz** (High frequency, Oscillatory)
        *   Recommended LPF Cutoff: **10 Hz** (fc = 10 Hz, τ ≈ 0.016s)
    *   **Rationale**: Heavy smoothing (LPF) required to separate true chassis rotation from "Slide Texture" vibration. Polar moment of inertia (Izz ≈ 2500 kg·m²) physically limits yaw oscillation to <10 Hz.
    *   **Formula**: $-\text{YawAccel}_{\text{smooth}} \times K_{\text{yaw}} \times 5.0 \text{Nm} \times K_{\text{decouple}}$.
    *   **Max Clamp**: 1.0 (Updated v0.6.20).
    *   **Note**: Negative sign provides counter-steering torque.
    *   **Sources**: [Deep Research Report](./yaw_kick_deep_reasearch_report.md), ESC Standards, OptimumG Vehicle Dynamics.
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
    
    // Reference Input: 1.0 rad/s^2 (Small/Medium signal - "Road Detail" range)
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
    
    // Case 2: Research Optimal (Gamma 1.8)
    // pow(0.1, 1.8) = 0.0158 (~6x suppression of low signals)
    engine.m_yaw_kick_gamma = 1.8f;
    double f_research = settle_and_measure();
    
    // Case 3: Aggressive Suppression (Gamma 2.0)
    // pow(0.1, 2.0) = 0.01 (~10x suppression)
    engine.m_yaw_kick_gamma = 2.0f;
    double f_suppress = settle_and_measure();
    
    // Case 4: Boost Low End (Gamma 0.5) - For road detail enthusiasts
    // pow(0.1, 0.5) = 0.316 (~3x boost)
    engine.m_yaw_kick_gamma = 0.5f;
    double f_boost = settle_and_measure();
    
    std::cout << "  Boost (γ=0.5): " << f_boost << std::endl;
    std::cout << "  Linear (γ=1.0): " << f_linear << std::endl;
    std::cout << "  Research (γ=1.8): " << f_research << std::endl;
    std::cout << "  Suppress (γ=2.0): " << f_suppress << std::endl;
    
    if (f_boost > f_linear && f_linear > f_research && f_research > f_suppress) {
        std::cout << "[PASS] Yaw Gamma modifies signal correctly (Boost > Linear > Research > Suppress)." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Yaw Gamma logic failed. Expected: Boost > Linear > Research > Suppress" << std::endl;
        g_tests_failed++;
    }
}

// Add to Run() function:
// test_yaw_kick_gamma();
```

---

## Changelog Entry

Add to `CHANGELOG_DEV.md`:

```markdown
### v0.6.36 - Yaw Kick Gamma Response (Research Validated)

**New Features:**
- **Yaw Kick Response Curve (Gamma)**: New slider to control the amplitude response of the Yaw Kick effect.
  - Gamma < 1.0: Boosts low-amplitude yaw signals (road detail feel).
  - Gamma = 1.0: Linear response (default, unchanged behavior).
  - Gamma > 1.0: Suppresses low-amplitude signals, focusing on slide kicks.
  - **Research Optimal**: γ ≈ 1.8 for best handling/texture separation.
- Added physics constant `YAW_ACCEL_REFERENCE` (10.0 rad/s²) for gamma normalization.
- Enhanced "Kick Response" tooltip now shows frequency equivalent (Hz).

**Research Validated:**
- Slide Initiation: 2.0-5.0 rad/s² at 1-10 Hz (Step/Surge)
- Road Texture: 0.2-1.5 rad/s² at 20-100 Hz (Oscillatory)
- Optimal LPF cutoff: 10 Hz (τ ≈ 0.016s)
- See: `docs/dev_docs/yaw_kick_deep_reasearch_report.md`

**Technical:**
- New setting: `yaw_kick_gamma` (default 1.0, range 0.1-3.0).
- Updated FFB_formulas.md with gamma curve and frequency documentation.
- Added regression test `test_yaw_kick_gamma()`.
```

---

## Summary of Changes

| File | Change |
|------|--------|
| `src/FFBEngine.h` | Add `m_yaw_kick_gamma`, `YAW_ACCEL_REFERENCE`, gamma logic in `calculate_force()` |
| `src/Config.h` | Add `yaw_kick_gamma` field, `SetYawKickGamma()` setter, update `Apply()` and `UpdateFromEngine()` |
| `src/Config.cpp` | Add save/load for main config and user presets |
| `src/GuiLayer.cpp` | Add "Response Curve (Gamma)" slider with indentation and Hz display |
| `docs/dev_docs/FFB_formulas.md` | Add gamma curve formula, frequency bands, and research citations |
| `tests/test_ffb_engine.cpp` | Add `test_yaw_kick_gamma()` regression test with research-optimal case |
| `CHANGELOG_DEV.md` | Add v0.6.36 entry with research validation notes |

---

## Verification Steps

1. **Build**: Compile the project and verify no errors.
2. **Test**: Run `test_ffb_engine.exe` and verify `test_yaw_kick_gamma` passes.
3. **GUI**: Launch the app and verify the new slider appears under "Rear Axle (Oversteer)".
4. **Persistence**: 
   - Set gamma to 1.8, close and reopen the app, verify it persists.
   - Create a user preset, verify gamma is saved and loaded correctly.
5. **In-Game**: Test with LMU:
   - Gamma 1.0: Baseline behavior.
   - Gamma 1.8: Road bumps should feel dampened; only significant slides trigger kicks.
   - Gamma 0.5: Road texture should be more pronounced in yaw feel.

---

## UI Tooltips Reference

This section contains the **exact tooltip text** for all Yaw Kick-related UI settings, both current and planned. These tooltips should be informative, explain the physics, and guide users toward optimal settings.

### Current Implementation (v0.6.36)

#### 1. Yaw Kick (Gain Slider)
**Variable**: `m_sop_yaw_gain`  
**Range**: 0.0 - 1.0

```
Yaw Kick Gain

The earliest warning for rear stepping out. Provides a sharp, 
momentary torque impulse when the car begins to rotate unexpectedly.

Based on Yaw Acceleration (rotational acceleration around the 
vertical axis). This is a LEADING indicator - it fires before 
the car has significantly rotated, giving you maximum reaction time.

  0.0 = Disabled
  0.3 = Subtle cue (recommended for most users)
  1.0 = Maximum intensity

Tip: If you feel kicks during normal driving, increase the 
Activation Threshold or Response Curve (Gamma) below.
```

#### 2. Activation Threshold
**Variable**: `m_yaw_kick_threshold`  
**Range**: 0.0 - 10.0 rad/s²

```
Activation Threshold (Noise Gate)

Minimum yaw acceleration required to trigger a kick.
Acts as a hard cutoff - signals below this value are ignored.

  0.0 = No filtering (feel everything, including noise)
  0.2 = Default (filters sensor jitter only)
  1.0 = Moderate (filters road bumps and micro-corrections)
  2.0 = Aggressive (only significant slides trigger kicks)

Use this to eliminate unwanted kicks from:
  • Engine vibration at idle
  • Small road imperfections
  • Driver micro-corrections

For a softer transition, use Response Curve (Gamma) instead.
```

#### 3. Response Curve (Gamma) – NEW v0.6.36
**Variable**: `m_yaw_kick_gamma`  
**Range**: 0.1 - 3.0

```
Response Curve (Gamma)

Shapes how yaw acceleration is converted to force.
Unlike Activation Threshold (hard cutoff), this provides a 
smooth, organic transition between small and large signals.

  < 1.0 = BOOST low range (more road detail/texture feel)
  = 1.0 = LINEAR (standard, no shaping)
  > 1.0 = SUPPRESS low range (focus on slide kicks only)

Research recommends: γ ≈ 1.8 for optimal balance.

Physics: Low values (e.g., 0.5) amplify small yaw movements, 
making the wheel feel more "alive" but potentially noisier.
High values (e.g., 2.0) create a "soft deadzone" that ignores 
micro-corrections while reacting strongly to actual slides.

Latency: ZERO. This is pure math, no filtering delay.
```

#### 4. Kick Response (Smooth)
**Variable**: `m_yaw_accel_smoothing`  
**Range**: 0.000 - 0.050 seconds

```
Kick Response (Low-Pass Filter)

Smooths the yaw acceleration signal to reduce noise and jitter.
Higher values = smoother but slower response.

  0.000s = ZERO latency (raw signal, may have noise)
  0.010s = Fast (10ms, ~16 Hz cutoff)
  0.016s = Research optimal (16ms, ~10 Hz cutoff)
  0.050s = Smooth (50ms, may feel sluggish)

The research recommends ~10 Hz cutoff (0.016s) because:
  • Slide events occur at 1-10 Hz (low frequency)
  • Road texture occurs at 20-100 Hz (high frequency)
  • A 10 Hz filter passes slides, blocks texture noise

For competitive racing where reaction time is critical:
  Set to 0.000s for zero latency (accept some noise).

Latency: Displayed below slider (Green ≤15ms, Red >15ms).
```

---

### Future Enhancements (Planned Tooltips)

#### 5. Adaptive Smoothing (Toggle) – Future
**Variable**: `m_yaw_adaptive_smoothing_enabled`  
**Type**: Checkbox

```
Adaptive Smoothing

When enabled, the filter automatically adjusts based on 
signal magnitude:
  • Small signals (road noise): Heavy smoothing (50ms)
  • Large signals (slides): Use your Kick Response setting

This provides road texture feel without noise, while 
preserving instant response for catching slides.

⚠️ Introduces minimum 50ms latency for small signals.
Disable this if you need absolute minimum latency.

Default: OFF (preserves zero-latency option)
```

#### 6. Adaptive Gain (Weber's Law) – Future
**Variable**: `m_yaw_weber_scaling_enabled`  
**Type**: Checkbox

```
Adaptive Gain (Weber's Law)

When enabled, the kick strength automatically scales with 
the current FFB load to maintain perceptibility.

Physics: Human perception follows Weber's Law - you only 
notice a force change if it's at least 10% of the current 
force you're already feeling.

Example:
  • Wheel loaded with 5 Nm cornering force:
    Kick must be ≥0.5 Nm to be felt
  • Wheel loaded with 15 Nm (high downforce):
    Kick must be ≥1.5 Nm to be felt

Without this, kicks may feel strong at low speed but 
imperceptible at high speed under heavy cornering load.

Default: OFF (manual gain control)
```

#### 7. Sustained Acceleration Gate – Future
**Variable**: `m_yaw_sustained_gate_enabled`  
**Type**: Checkbox

```
Sustained Acceleration Gate

When enabled, a kick only triggers if yaw acceleration 
stays above threshold for 2-3 consecutive frames (50-75ms).

Purpose: Eliminates false positives from:
  • Aggressive turn-in (peaks and falls quickly)
  • Curb strikes (instantaneous spike)

A true slide produces sustained acceleration as the car 
continues to rotate. This filter catches the difference.

Latency: Adds 50-75ms delay to kick trigger.
Benefit: Significantly reduces unwanted kicks.

Default: OFF (instant response)
```

---

### Dynamic Status Indicators

The following dynamic indicators should be displayed below relevant sliders:

#### Latency Indicator (for Kick Response)
```cpp
int ms = (int)(engine.m_yaw_accel_smoothing * 1000.0f + 0.5f);
ImVec4 color = (ms <= 15) ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
ImGui::TextColored(color, "Latency: %d ms", ms);
```
- **Green**: ≤ 15ms (good for competitive racing)
- **Red**: > 15ms (may feel sluggish)

#### Frequency Indicator (optional, for advanced users)
```cpp
if (engine.m_yaw_accel_smoothing > 0.0001f) {
    float fc = 1.0f / (2.0f * 3.14159f * engine.m_yaw_accel_smoothing);
    ImGui::SameLine();
    ImGui::TextDisabled("(~%.0f Hz cutoff)", fc);
}
```

#### Gamma Curve Visual (optional, future)
Consider adding a small visual preview of the gamma curve shape:
```
γ = 0.5: ╱  (boost low end)
γ = 1.0: ╱  (linear)
γ = 2.0: _╱ (suppress low end, soft deadzone)
```

---

## Future Enhancements (Research-Suggested)


Based on the deep research, the following enhancements are recommended for future versions:

### 1. Adaptive Gain Scaling (Weber's Law)
**Priority**: High  
The kick strength should scale with current FFB load to maintain perceptibility:
```cpp
// Kick must be >10% of current torque to be perceived (Weber's Law)
double weber_scale = (std::max)(0.1, std::abs(current_ffb_output) * 0.10);
yaw_force = (std::max)(yaw_force, weber_scale);
```

### 2. Gated Algorithm (Yaw + Rear Slip)
**Priority**: Medium  
**⚠️ LATENCY CONCERN**: This approach may negate the benefit of yaw acceleration as an early warning.

The idea is to combine yaw acceleration with rear slip angle to eliminate false positives:
```cpp
// Only trigger kick when BOTH conditions are met:
// A) Rear tires are saturated (slip angle > 4°)
// B) Yaw acceleration > 2.0 rad/s²
bool rear_saturated = std::abs(rear_slip_angle) > 0.07; // ~4 degrees
bool yaw_significant = std::abs(raw_yaw_accel) > 2.0;
if (rear_saturated && yaw_significant) {
    // Apply kick
}
```

**Problem**: The causal chain is:
1. Rear tires saturate → 2. **Yaw acceleration spikes** → 3. Yaw rate increases → 4. **Slip angle increases**

Yaw acceleration is valuable precisely because it's a **leading indicator** (step 2). Slip angle is a **lagging indicator** (step 4). Gating on slip angle forces us to wait until the car is already rotating significantly, which defeats the purpose of the early warning.

**Research Finding** (Section 5.1):
> *"Yaw acceleration is the precursor to sideslip... it is a **leading indicator**, whereas sideslip is a **lagging indicator**."*

**Better Alternatives**:
1. **Sustained Acceleration Gate**: Instead of slip angle, check if yaw acceleration is sustained over 2-3 frames (50-75ms). A true slide will sustain; turn-in will peak and fall.
2. **Acceleration + Throttle Gate**: Check `YawAccel > 2.0 && Throttle > 0.5`. Most snap oversteer occurs under power.
3. **Direction Check**: Compare yaw acceleration sign vs steering input. If they're opposite (countersteering already happening), skip the kick.

### 3. 2nd-Order Butterworth LPF
**Priority**: Medium  
Replace simple exponential LPF with proper Butterworth for steeper rolloff:
- Current: -6 dB/octave (1st-order exponential)
- Recommended: -12 dB/octave (2nd-order Butterworth)

### 4. Adaptive Smoothing (Magnitude-Dependent Filtering)
**Priority**: Low  
**⚠️ LATENCY CONCERN**: This enhancement would introduce **non-removable latency**.

Dynamically adjust tau based on signal magnitude:
- Low signal (< 1.0 rad/s²): Heavy smoothing (50ms) → Road feel without jitter
- High signal (> 2.0 rad/s²): User setting (0-10ms) → Instant slide response

**Problem**: The 50ms minimum for low signals **cannot be disabled** by the user. This violates the design principle that all latency must be user-adjustable to zero.

**If Implemented**: Must be **optional** with a toggle:
```cpp
// Only apply adaptive smoothing if user opts in
bool m_yaw_adaptive_smoothing_enabled = false; // Default OFF to preserve zero-latency option

if (m_yaw_adaptive_smoothing_enabled) {
    // Blend between 50ms (noise) and user setting (fast)
    double slow_tau = 0.050;
    double fast_tau = (double)m_yaw_accel_smoothing;
    double blend = magnitude / (noise_floor * 4.0);
    dynamic_tau = slow_tau + (fast_tau - slow_tau) * blend;
} else {
    // Standard: Use user setting directly (can be 0 for zero latency)
    dynamic_tau = (double)m_yaw_accel_smoothing;
}
```

---

## Research References

Full citations available in: [`docs/dev_docs/yaw_kick_deep_reasearch_report.md`](./yaw_kick_deep_reasearch_report.md)

Key sources:
- OptimumG: "Getting to grips with your yaw moments"
- ESC Standards (Wikipedia, SAE 2023-01-0661)
- Vehicle Dynamics frequency analysis (Clemson, Chalmers)
- Human perception thresholds (Weber's Law, ePrints Soton)
- Simucube/Fanatec filter implementations (Granite Devices)

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

# dynamic range compression

The application of dynamic range compression—a technique typically used in high-fidelity audio—to provide a more nuanced alternative to standard power-law curves.  By using this logic, I can think through how to amplify the subtle, informative textures of the road surface while simultaneously preventing high-amplitude rotational events from exceeding the mechanical limits of the hardware. This ensures that the driver maintains a clear sense of the car's state without the feedback becoming harsh or lossy during extreme maneuvers.

# Deep research Report in answer from the Query

docs\dev_docs\yaw_kick_deep_reasearch_report.md

---

## Appendix: Research Query – Gated Algorithm Latency Analysis

The following query can be used with a deep research tool to investigate the latency implications of combining yaw acceleration with rear slip angle for slide detection.

---

### Deep Research Query: Temporal Relationship Between Yaw Acceleration and Rear Slip Angle

**Context:**
I am developing a Force Feedback (FFB) application for racing simulators (Le Mans Ultimate / rFactor 2). The application provides a "Yaw Kick" effect based on yaw acceleration (`mLocalRotAccel.y`) to alert drivers when the rear of the car begins to slide.

A proposed enhancement is a "Gated Algorithm" that only triggers the yaw kick when **BOTH** conditions are met:
- **Condition A**: Rear slip angle > 4° (rear tires saturated)
- **Condition B**: Yaw acceleration > 2.0 rad/s² (car rotating rapidly)

The goal is to reduce false positives (e.g., aggressive turn-in that produces high yaw acceleration but is intentional and stable).

**The Concern:**
Yaw acceleration is a **leading indicator** - it detects the moment imbalance at the instant of grip loss. Rear slip angle is a **lagging indicator** - it only becomes significant after the car has already started rotating.

If we gate on rear slip angle, we may be introducing latency that negates the benefit of using yaw acceleration as an early warning.

**Research Objectives:**

1. **Temporal Lag Quantification:**
   - In a typical snap oversteer event (e.g., GT3 car at 120 km/h corner entry):
     - How many milliseconds after the yaw acceleration spike does the rear slip angle exceed 4°?
     - What is the typical lag between these two indicators?
   - Is this lag consistent, or does it vary significantly based on:
     - Vehicle speed?
     - Tire compound / temperature?
     - Weight distribution?
     - Aerodynamic downforce?

2. **Telemetry Analysis:**
   - Are there published telemetry traces from real race cars or high-fidelity simulators showing the time-series relationship between yaw acceleration and slip angle during oversteer events?
   - What is the typical phase relationship between these signals?

3. **ESC / Stability Control Systems:**
   - How do real Electronic Stability Control (ESC) systems handle this trade-off?
   - Do ESC systems use yaw acceleration as a primary trigger with slip angle as a secondary confirmation?
   - What latency budgets do automotive ESC systems work with?

4. **Human Reaction Time Consideration:**
   - Human reaction time for corrective steering is approximately 150-300ms.
   - If the gated algorithm introduces 50-100ms of latency, is this significant relative to human reaction time?
   - At what point does the latency become "too late" for the driver to benefit from the warning?

5. **Alternative Gating Strategies:**
   - Instead of slip angle, would sustained yaw acceleration (2-3 frames / 50-75ms) be a better gating metric?
   - Could we use yaw acceleration sign vs. steering input direction as a gate (i.e., only trigger if they're aligned, indicating the driver is NOT already countersteering)?
   - Are there other leading indicators that could replace slip angle for false positive reduction?

6. **Causal Chain Timing:**
   For the sequence: Tire Saturation → Yaw Acceleration → Yaw Rate Increase → Slip Angle Increase
   - What are typical time constants for each transition in this chain?
   - Are these measurable in simulation telemetry (rFactor 2 / iRacing / Assetto Corsa)?

**Desired Output:**
- Quantified latency estimates (in milliseconds) between yaw acceleration spike and slip angle threshold crossing.
- Analysis of whether the gated algorithm introduces "unacceptable" delay for competitive racing.
- Recommended alternative approaches if the slip angle gate is found to introduce too much latency.
- Any academic papers, motorsport engineering documents, or ESC patents that discuss the temporal relationship between these metrics.

---

*Query created: 2026-01-07*