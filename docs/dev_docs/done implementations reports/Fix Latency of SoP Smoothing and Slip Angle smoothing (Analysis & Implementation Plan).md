# Latency Analysis & Implementation Plan: SoP and Slip Angle Smoothing

**Date:** December 21, 2025
**Status:** Ready for Implementation
**Target Version:** v0.4.40

---

## 1. Executive Summary

Users with high-end Direct Drive wheels reported a perceptible delay in the Force Feedback signal compared to the base game. Investigation confirmed that the default smoothing settings in the FFB Engine introduce approximately **95ms of latency** to the Seat of Pants (SoP) effect and **22.5ms** to the Slip Angle calculation.

This report outlines the necessary changes to:
1.  **Reduce Default Latency:** Shift defaults to target **15ms**, which is responsive enough for Direct Drive wheels while remaining stable for Belt/Gear driven wheels (T300/G29).
2.  **Expose Hidden Parameters:** Make the internal Slip Angle smoothing time constant user-configurable.
3.  **Improve GUI Feedback:** Explicitly display calculated latency in milliseconds with color-coded warnings (Red/Green) to educate users about the trade-off between smoothness and lag.

---

## 2. Problem Analysis

### A. The Primary Cause: SoP Smoothing
The "Seat of Pants" (Lateral G) effect uses a Low Pass Filter (LPF) controlled by `m_sop_smoothing_factor`.
*   **Formula:** $\tau = (1.0 - \text{factor}) \times 0.1 \text{ seconds}$.
*   **Current Default:** `0.05`.
*   **Resulting Latency:** $(1.0 - 0.05) \times 100\text{ms} = \mathbf{95\text{ms}}$.
*   **Impact:** A nearly 0.1-second delay between the car's physical movement and the FFB weight transfer is highly noticeable on responsive hardware.

### B. The Secondary Cause: Slip Angle Smoothing
In v0.4.37, a "Time-Corrected" smoothing filter was added to the Slip Angle calculation to prevent noise.
*   **Current Implementation:** Hardcoded constant `const double tau = 0.0225;`.
*   **Resulting Latency:** **22.5ms**.
*   **Impact:** Delays the onset of Understeer (Grip Loss) and Rear Aligning Torque effects.

### C. Hardware Suitability Analysis (15ms Target)
We have determined that **15ms** is the optimal baseline target.
*   **Direct Drive:** 15ms is fast enough to feel "connected" and "raw".
*   **Belt/Gear (T300/G29):** These wheels have inherent mechanical damping and friction. A 15ms electronic filter is sufficient to remove high-frequency digital noise (>60Hz) without making the wheel feel "lazy" or disconnected. The previous 95ms default was excessive for these wheels.

---

## 3. Mathematical Derivation for Defaults

To achieve the **15ms** target, the configuration defaults must be updated as follows:

### A. SoP Smoothing Factor
*   **Target:** 15ms.
*   **Equation:** $15 = (1.0 - \text{Factor}) \times 100$.
*   **Solution:** $0.15 = 1.0 - \text{Factor} \rightarrow \text{Factor} = \mathbf{0.85}$.

### B. Slip Angle Smoothing (Tau)
*   **Target:** 15ms.
*   **Equation:** Direct time constant in seconds.
*   **Solution:** $\mathbf{0.015\text{s}}$.

---

## 4. Implementation Specification

### Component A: Physics Engine (`FFBEngine.h`)

1.  **Promote Constant to Variable:**
    *   Remove the local `const double tau = 0.0225;` from `calculate_slip_angle`.
    *   Add a public member variable: `float m_slip_angle_smoothing = 0.015f;`.
2.  **Update Logic:**
    *   Use the member variable in the LPF calculation.
    *   Add a safety clamp to prevent division by zero: `if (tau < 0.0001) tau = 0.0001;`.

### Component B: Configuration (`src/Config.h` & `src/Config.cpp`)

1.  **Update Defaults (`Config.h`):**
    *   Change `sop_smoothing` default from `0.05f` to **`0.85f`**.
    *   Add `float slip_smoothing = 0.015f;` to the `Preset` struct.
2.  **Update Methods:**
    *   Implement `SetSlipSmoothing`, `Apply`, and `UpdateFromEngine` to handle the new variable.
3.  **Update Presets (`Config.cpp`):**
    *   Update "Default (T300)" and "T300" presets to explicitly set `.SetSmoothing(0.85f)` and `.SetSlipSmoothing(0.015f)`.
4.  **Persistence:**
    *   Update `Save` and `Load` to handle the key `slip_angle_smoothing`.

### Component C: GUI Layer (`src/GuiLayer.cpp`)

The GUI must be updated to provide "Event Driven" visual feedback using Immediate Mode logic.

#### 1. Visual Design Requirements
*   **Latency Label:** A text line *above* the slider explicitly stating the latency.
*   **Color Coding:**
    *   **Red:** If Latency > 20ms (Warning: High Lag).
    *   **Green:** If Latency <= 20ms (OK).
*   **Slider Text:** The slider bar must display the value AND the lag (e.g., `0.85 (15ms lag)`).

#### 2. SoP Smoothing Implementation Logic
```cpp
// Calculate Latency
int lat_ms = (int)((1.0f - engine.m_sop_smoothing_factor) * 100.0f);

// Draw Label
ImGui::Text("SoP Smoothing");
ImGui::SameLine();
if (lat_ms > 20) 
    ImGui::TextColored(Red, "(SIGNAL LATENCY: %d ms)", lat_ms);
else 
    ImGui::TextColored(Green, "(Latency: %d ms - OK)", lat_ms);

// Draw Slider (Hidden Label ##)
char fmt[64];
snprintf(fmt, sizeof(fmt), "%.2f (%dms lag)", lat_ms);
FloatSetting("##SoPSmoothing", &engine.m_sop_smoothing_factor, 0.0f, 1.0f, fmt);
```

#### 3. Slip Angle Smoothing Implementation Logic
*   **Location:** "Advanced Tuning" section.
*   **Range:** `0.000` to `0.100` seconds.
*   **Logic:** Same Red/Green logic as SoP, but calculating `ms = value * 1000`.

---

## 5. Implementation Checklist

### 1. Physics Engine (`FFBEngine.h`)
- [ ] **Promote Constant to Variable**:
    - Remove `const double tau = 0.0225;` inside `calculate_slip_angle`.
    - Add public member variable: `float m_slip_angle_smoothing = 0.015f;` (Default 15ms).
- [ ] **Update Logic**:
    - In `calculate_slip_angle`, set `double tau = (double)m_slip_angle_smoothing;`.
    - Add safety clamp: `if (tau < 0.0001) tau = 0.0001;`.

### 2. Configuration Structure (`src/Config.h`)
- [ ] **Update Defaults**:
    - Change `float sop_smoothing` default from `0.05f` to **`0.85f`** (15ms).
    - Add `float slip_smoothing = 0.015f;` (15ms).
- [ ] **Update Methods**:
    - Add `Preset& SetSlipSmoothing(float v)`.
    - Update `Apply(FFBEngine& engine)` to copy `slip_smoothing` to `engine.m_slip_angle_smoothing`.
    - Update `UpdateFromEngine` to read `engine.m_slip_angle_smoothing`.

### 3. Persistence & Presets (`src/Config.cpp`)
- [ ] **Update `LoadPresets`**:
    - Ensure "Default (T300)" and "T300" presets explicitly set `.SetSmoothing(0.85f)` and `.SetSlipSmoothing(0.015f)`.
- [ ] **Update `Save`**:
    - Write `file << "slip_angle_smoothing=" << engine.m_slip_angle_smoothing << "\n";`.
- [ ] **Update `Load`**:
    - Add parsing logic for key `"slip_angle_smoothing"`.

### 4. GUI Layer (`src/GuiLayer.cpp`)
- [ ] **Refactor SoP Smoothing Slider** (inside `DrawTuningWindow` -> "Advanced Tuning"):
    - Calculate latency: `int lat_ms = (int)((1.0f - engine.m_sop_smoothing_factor) * 100.0f);`.
    - Add colored text label above slider:
        - **Red** if `lat_ms > 20`.
        - **Green** if `lat_ms <= 20`.
    - Create dynamic format string: `snprintf(buf, ..., "%.2f (%dms lag)", lat_ms)`.
    - Update `FloatSetting` to use the dynamic format string and `##HiddenLabel`.
    - Update Tooltip to explain the trade-off.

- [ ] **Add Slip Angle Smoothing Slider**:
    - Calculate latency: `int slip_ms = (int)(engine.m_slip_angle_smoothing * 1000.0f);`.
    - Add colored text label (Red > 20ms / Green <= 20ms).
    - Create dynamic format string: `snprintf(buf, ..., "%.3fs (%dms lag)", slip_ms)`.
    - Add `FloatSetting` for `engine.m_slip_angle_smoothing` (Range 0.000 to 0.100).
    - Add Tooltip explaining "Physics Response Time".

### 5. Documentation & Build
- [ ] **Update `CHANGELOG.md`**: Document the new defaults (15ms) and the new "Physics Response" slider.
- [ ] **Update `VERSION`**: Increment version number.
- [ ] **Verify**: Compile and run tests.
