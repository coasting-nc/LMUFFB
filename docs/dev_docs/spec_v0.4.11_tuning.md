# Technical Specification: FFB Tuning & Expansion (v0.4.11)

**Target Version:** v0.4.11
**Priority:** High (Usability/Tuning)

## 1. Physics Tuning Requirements

We need to adjust the hardcoded scaling coefficients in `FFBEngine.h` to produce meaningful torque values in the Newton-meter domain.

### A. Rear Aligning Torque
*   **Current Logic:** `rear_torque = calc_rear_lat_force * 0.00025 * m_oversteer_boost`
*   **New Logic:** `rear_torque = calc_rear_lat_force * 0.001 * m_rear_align_effect`
*   **Coefficient Change:** `0.00025` -> **`0.001`**
    *   *Impact:* Max output increases from ~1.5 Nm to ~6.0 Nm.
*   **Variable Change:** Decoupled from `m_oversteer_boost`. Now controlled by `m_rear_align_effect`.

### B. Scrub Drag
*   **Current Logic:** `drag_force = ... * m_scrub_drag_gain * 2.0 * fade`
*   **New Logic:** `drag_force = ... * m_scrub_drag_gain * 5.0 * fade`
*   **Multiplier Change:** `2.0` -> **`5.0`**

### C. Road Texture
*   **Current Logic:** `road_noise = ... * 25.0 * m_road_texture_gain`
*   **New Logic:** `road_noise = ... * 50.0 * m_road_texture_gain`
*   **Multiplier Change:** `25.0` -> **`50.0`**

## 2. GUI Visualization Refinement

To make debugging easier, we will "zoom in" the Y-axis for texture-based plots in `GuiLayer.cpp`.

*   **Group A: Macro Forces (Keep ±20.0)**
    *   Base Torque, SoP, Oversteer Boost, Rear Align Torque, Scrub Drag, Understeer Cut.
*   **Group B: Micro Textures (Change to ±10.0)**
    *   Road Texture, Slide Texture, Lockup Vib, Spin Vib, Bottoming.

## 3. New Settings & Presets

### New Setting: Rear Align Effect
*   **Type:** `float`
*   **Default:** `1.0f`
*   **Range:** `0.0f` to `2.0f`
*   **Location:** `FFBEngine` class, `Config` struct, `GuiLayer` (Effects section).

### New Presets
Add to `Config::LoadPresets`:

1.  **"Test: Rear Align Torque Only"**
    *   Isolates the rear axle workaround force.
    *   `m_rear_align_effect = 1.0`, `m_sop_effect = 0.0`, `m_oversteer_boost = 0.0`.
2.  **"Test: SoP Base Only"**
    *   Isolates the lateral G force.
    *   `m_sop_effect = 1.0`, `m_rear_align_effect = 0.0`, `m_oversteer_boost = 0.0`.
3.  **"Test: Slide Texture Only"**
    *   Isolates the scrubbing vibration.
    *   `m_slide_texture_gain = 1.0`, all other gains 0.0.
