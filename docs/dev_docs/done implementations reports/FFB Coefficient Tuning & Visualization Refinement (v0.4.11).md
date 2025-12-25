# Technical Specification: FFB Coefficient Tuning & Visualization Refinement (v0.4.11)

**Target Version:** v0.4.11
**Date:** December 13, 2025
**Priority:** High (Usability/Tuning)

## 1. Problem Statement

Following the implementation of physics workarounds in v0.4.10, the FFB engine is now correctly calculating forces for Rear Aligning Torque and Scrub Drag. However, the **magnitude** of these forces is numerically too small to be effective.

*   **Rear Align Torque:** Currently peaks at ~0.75 Nm. On a 20 Nm wheel, this is barely perceptible.
*   **Scrub Drag:** Peaks at ~0.5 Nm. Invisible on graphs and undetectable by the driver.
*   **Visualization:** The Troubleshooting Graphs use a uniform ±20 Nm scale. While appropriate for the main Steering Torque, it hides the detail of subtle texture effects (Road, Slide, Vibrations) which typically operate in the 0-5 Nm range.

## 2. Physics Tuning Requirements (`FFBEngine.h`)

We need to adjust the hardcoded scaling coefficients to produce meaningful torque values in the Newton-meter domain.

### A. Rear Aligning Torque
*   **Current Logic:** `rear_torque = calc_rear_lat_force * 0.00025 * m_oversteer_boost`
*   **Analysis:** Max lateral force is clamped at 6000 N.
    *   $6000 \times 0.00025 = 1.5 \text{ Nm}$.
    *   With default boost (0.5), output is $0.75 \text{ Nm}$.
*   **New Coefficient:** **`0.001`**
    *   $6000 \times 0.001 = 6.0 \text{ Nm}$.
    *   With default boost (0.5), output is $3.0 \text{ Nm}$. This provides a distinct, feelable counter-steering cue.

### B. Scrub Drag
*   **Current Logic:** `drag_force = ... * m_scrub_drag_gain * 2.0 * fade`
*   **Analysis:** At max gain (1.0), output is 2.0 Nm.
*   **New Multiplier:** **`5.0`**
    *   Max output becomes $5.0 \text{ Nm}$. This allows the user to dial in a very heavy resistance if desired, or keep it subtle at lower gain settings.

### C. Road Texture
*   **Current Logic:** `road_noise = (delta_l + delta_r) * 25.0 * ...`
*   **Analysis:** Suspension deltas are tiny (e.g., 0.002m). $0.004 \times 25 = 0.1 \text{ Nm}$.
*   **New Multiplier:** **`50.0`**
    *   Boosts the signal to ensure bumps are felt on direct drive wheels.

---

## 3. GUI Visualization Refinement (`GuiLayer.cpp`)

To make debugging easier, we will "zoom in" the Y-axis for texture-based plots.

### Plot Scaling Logic
The `ImGui::PlotLines` function accepts `scale_min` and `scale_max`.

*   **Group A: Macro Forces (Keep ±20.0)**
    *   Base Torque
    *   SoP (Base Chassis G)
    *   Lateral G Boost (Slide)
    *   Rear Align Torque
    *   Scrub Drag Force
    *   Understeer Cut

*   **Group B: Micro Textures (Change to ±10.0)**
    *   Road Texture
    *   Slide Texture
    *   Lockup Vib
    *   Spin Vib
    *   Bottoming

**Implementation Example:**
```cpp
// Old
ImGui::PlotLines("##Road", ..., -20.0f, 20.0f, ...);

// New
ImGui::PlotLines("##Road", ..., -10.0f, 10.0f, ...);
```

## 4. Summary of Changes

| Component | Variable/Function | Old Value | New Value |
| :--- | :--- | :--- | :--- |
| `FFBEngine.h` | Rear Torque Coeff | `0.00025` | **`0.001`** |
| `FFBEngine.h` | Scrub Drag Multiplier | `2.0` | **`5.0`** |
| `FFBEngine.h` | Road Texture Multiplier | `25.0` | **`50.0`** |
| `GuiLayer.cpp` | Texture Plot Scales | `±20.0f` | **`±10.0f`** |