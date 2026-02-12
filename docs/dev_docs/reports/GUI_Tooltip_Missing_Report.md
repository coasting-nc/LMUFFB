# Investigation Report: Missing GUI Tooltips

**Date:** 2026-02-11
**Status:** Validated
**Affected Components:** GUI Widgets (Sliders, Checkboxes, ComboBoxes)

## 1. Issue Description

The user reported that the tooltips for many GUI widgets are "broken" or "not showing." Specifically:
*   Users can see the generic "Fine Tune: Arrow Keys | Exact: Ctrl+Click" instruction.
*   The descriptive text explaining **what the slider affects** (e.g., "Controls the intensity of...") is missing.
*   This creates a confusing user experience where the tooltip box appears but is mostly empty.

Example screenshot (Lateral G Boost, Sensitivity):
> *Visible*: "Fine Tune: Arrow Keys | Exact: Ctrl+Click" \
> *Missing*: (Description of the parameter)

## 2. Technical Investigation

Analysis of `src/GuiLayer_Common.cpp` and `src/GuiWidgets.h` reveals the root cause.

### 2.1. Tooltip Logic (`GuiWidgets::Float`)
The widget implementation in `GuiWidgets.h` handles tooltips as follows:

```cpp
// src/GuiWidgets.h : Line 64
if (!keyChanged && !ImGui::IsItemActive()) {
    ImGui::BeginTooltip();

    // Part A: Specific Description
    if (tooltip && strlen(tooltip) > 0) {
        ImGui::Text("%s", tooltip); // ONLY runs if 'tooltip' arg is provided
        ImGui::Separator();
    }

    // Part B: Generic Instructions (Always shown)
    ImGui::Text("Fine Tune: Arrow Keys | Exact: Ctrl+Click");

    ImGui::EndTooltip();
}
```

The "broken" behavior is actually the **correct execution of this logic** when the `tooltip` argument is `nullptr`.

### 2.2. Missing Arguments in Call Sites (`GuiLayer_Common.cpp`)
Most settings in the GUI are rendered via helper lambdas like `FloatSetting`, `BoolSetting`, etc.

**Correct Usage Example (General FFB):**
The "Master Gain" slider is implemented correctly:
```cpp
// Line 380
FloatSetting("Master Gain", &engine.m_gain, ..., "Global scale factor for all forces...");
```
*Result: Shows both description AND instructions.*

**Incorrect Usage Example (Slope Detection):**
The "Sensitivity" slider is implemented without the description argument:
```cpp
// Line 543
FloatSetting("  Sensitivity", &engine.m_slope_sensitivity, 0.1f, 5.0f, "%.1fx");
// The 'tooltip' argument defaults to nullptr
```
*Result: Skips the description, shows only "Fine Tune..." instructions.*

### 2.3. Historical Analysis (Regression Confirmed)
A review of previous commits confirms that these tooltips **were present** in earlier versions but were lost during the refactoring to `GuiLayer_Common.cpp`.

*   **Version 0.7.1 (Commit `8bff2ff`)**:
    *   File: `src/GuiLayer.cpp`
    *   Status: **Present**. "Lateral G Boost", "Yaw Kick", "SoP Smoothing", and "Slope Sensitivity" all had detailed tooltips.
*   **Version 0.6.31 (Commit `e66b074`)**:
    *   File: `src/GuiLayer.cpp`
    *   Status: **Present**. Core effects like "Lateral G Boost" had tooltips.

**Conclusion**: This is a regression caused by a recent refactoring where the tooltip arguments were omitted when moving code to the new file structure.

## 3. Scope of Affected Parameters

A review of `GuiLayer_Common.cpp` shows that **tooltips are missing for approximately 70-80% of the parameters**, particularly in newer sections.

### Front Axle (Mostly OK)
*   **OK**: Master Gain, Max Torque, Steering Shaft Gain/Smoothing, Understeer Effect.
*   **Missing**: Base Force Mode, Flatspot Suppression, Static Noise Filter.

### Rear Axle (All Missing)
*    Lateral G Boost (as reported)
*    Lateral G
*    SoP Self-Aligning Torque
*    Yaw Kick (and threshold)
*    Gyro Damping (and smoothing)
*    Advanced SoP (Smoothing, Scale)

### Grip & Slip (All Missing)
*    Slip Angle Smoothing
*    Chassis Inertia
*    Optimal Slip Angle/Ratio
*    **Slope Detection Settings** (Sensitivity, Threshold, etc. - as reported)

### Braking & Lockup (All Missing)
*    Lockup Strength, Brake Load Cap, Gamma, etc.
*    ABS Pulse

### Tactile Textures (All Missing)
*    Slide Rumble, Road Details, Spin Vibration.

## 4. Conclusion & Recommendation

The GUI code is functioning as written, but the content (tooltip strings) was never added for the majority of the advanced settings. This gives the impression of a broken UI.

**Action Plan:**
1.  **Immediate**: Restore the missing tooltip strings in `GuiLayer_Common.cpp` using the content from version 0.7.1 (`8bff2ff`) as the source of truth.
2.  **Validation**: Verify that the `Fine Tune` text is now accompanied by the parameter description.

No logic changes to `GuiWidgets.h` are required.
