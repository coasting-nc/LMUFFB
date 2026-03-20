# Improve the Stationary Oscillations setting #430

**Opened by:** coasting-nc
**Date:** Mar 20, 2026

## Description

Improve the Stationary Oscillations setting

Current issues with the Stationary Oscillations setting:

* It should be enabled at 100% by default (currently it is disabled by default, since it is at 0%).
* The slider formatting should be more intuitive (change it to a standard percentage, without indicating Nm).
* The tooltip to say which is the speed at which it fades at 0%.
* It should be moved under the General FFB section, after "Min Force" and before "Soft Lock".

Here are the changes to implement this:

### 1. Change the Default to 100%

We need to update the default value in both the Engine and the Config Preset so that new users (and the "Reset Defaults" button) get the 100% value automatically.

**In src/ffb/FFBEngine.h:**
Find the variable and change its default to 1.0f:
```cpp
float m_gyro_gain;
float m_gyro_smoothing;
float m_stationary_damping = 1.0f; // <--- Changed to 1.0f (100%)
```

**In src/core/Config.h:**
Find the Preset struct and change its default as well:
```cpp
float gyro_gain = 0.0f;
float stationary_damping = 1.0f; // <--- Changed to 1.0f (100%)
```

### 2. Update the Base Tooltip (src/gui/Tooltips.h)

We will write the base explanation here, and we will append the dynamic speed to it in the UI code.

**Add this to Tooltips.h:**
```cpp
inline constexpr const char* STATIONARY_DAMPING =
    "Intensity of the friction/damping applied ONLY when the car is stationary.\n"
    "Resists fast wheel movements to prevent violent pit-box oscillations.\n"
    "Fades out completely to 0% as you accelerate to prevent masking FFB details.\n\n"
    "Note: This is a velocity multiplier. 100% = 1.0 Nm of resistance per rad/s of wheel speed.";
```

(Remember to add STATIONARY_DAMPING to the ALL list at the bottom of the file).

### 3. Move the Slider & Add Dynamic Tooltip (src/gui/GuiLayer_Common.cpp)

First, remove the Stationary Damping slider from the "Rear Axle (Oversteer)" section if you already added it there.

Next, find the General FFB section in DrawTuningWindow (around line 280). Look for the Min Force slider, and insert the new code right after it, just before the Soft Lock block:
```cpp
FloatSetting("Target Rim Torque", &engine.m_target_rim_nm, 1.0f, 50.0f, "%.1f Nm", Tooltips::TARGET_RIM_TORQUE);
FloatSetting("Min Force", &engine.m_min_force, 0.0f, 0.20f, "%.3f", Tooltips::MIN_FORCE);

// --- ADD STATIONARY DAMPING HERE ---
// Dynamically format the tooltip to show the exact fade-out speed in km/h
char stat_damp_tooltip[512];
StringUtils::SafeFormat(stat_damp_tooltip, sizeof(stat_damp_tooltip),
    "%s\n\nCurrently fades to 0%% at: %.1f km/h (See Advanced Settings -> Speed Gate).",
    Tooltips::STATIONARY_DAMPING, engine.m_speed_gate_upper * 3.6f);

FloatSetting("Stationary Damping", &engine.m_stationary_damping, 0.0f, 1.0f, FormatPct(engine.m_stationary_damping), stat_damp_tooltip);
// -----------------------------------

if (ImGui::TreeNodeEx("Soft Lock", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::NextColumn(); ImGui::NextColumn();
```
