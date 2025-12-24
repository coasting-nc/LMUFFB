# Implementation Plan: FFB Signal Gain Compensation ("Decoupling")

## 1. Introduction & Terminology

### The Problem: Signal Compression
Currently, the application calculates forces in absolute Newton-meters (Nm). These forces are then normalized (divided) by the user's `Max Torque Ref` setting to produce the final output signal (-1.0 to 1.0).

*   **Scenario A (Weak Wheel):** `Max Torque Ref` = 20 Nm. A 2 Nm texture effect results in **10%** signal output. Strong and clear.
*   **Scenario B (Strong Wheel):** `Max Torque Ref` = 100 Nm. The same 2 Nm texture effect results in **2%** signal output. Weak and imperceptible.

This forces users with high-torque settings to max out their effect sliders just to feel them, effectively confining them to a tiny usable range of the slider.

### The Solution: Gain Compensation
We will implement **Gain Compensation** (often referred to as "Decoupling"). This technique automatically scales the internal force of specific effects based on the `Max Torque Ref`.

*   **Logic:** If the user increases `Max Torque Ref` (zooming out the signal), the app automatically boosts the "Generator" effects (zooming them in) by the same ratio.
*   **Result:** A "50%" setting on a slider will feel like a 50% strength effect relative to the wheel's capability, regardless of whether the wheel is calibrated to 20 Nm or 100 Nm.

### Classification of Effects
To implement this correctly, we must classify every slider into one of two categories:

1.  **Generators (Apply Compensation):** These add new forces (Newtons) to the signal.
    *   *Examples:* SoP, Rear Align Torque, Slide Texture, Road Texture, Lockup.
2.  **Modifiers (Do NOT Apply Compensation):** These multiply or reduce existing forces. Scaling them would result in "Double Scaling" errors.
    *   *Examples:* Understeer Effect (Reduction ratio), Oversteer Boost (Multiplier).

---

## 2. Slider Configuration Table

This table defines the new behavior for every GUI control.

| Slider Name | Type | Compensation? | New GUI Range | Display Format |
| :--- | :--- | :--- | :--- | :--- |
| **Master Gain** | Scalar | **NO** | 0.0 - 2.0 | `0% - 200%` |
| **Max Torque Ref** | Reference | **NO** | 1.0 - 200.0 | `%.1f Nm` |
| **Steering Shaft Gain** | Attenuator | **NO** | 0.0 - 1.0 | `0% - 100%` |
| **Understeer Effect** | Modifier | **NO** | 0.0 - 50.0 | `0% - 100%` (Remapped) |
| **Oversteer Boost** | Modifier | **NO** | 0.0 - 20.0 | `0% - 200%` |
| **Rear Align Torque** | Generator | **YES** | 0.0 - 2.0 | `0% - 200% (~X Nm)` |
| **SoP Yaw (Kick)** | Generator | **YES** | 0.0 - 2.0 | `0% - 200% (~X Nm)` |
| **Gyro Damping** | Generator | **YES** | 0.0 - 1.0 | `0% - 100% (~X Nm)` |
| **Lateral G (SoP)** | Generator | **YES** | 0.0 - 2.0 | `0% - 200% (~X Nm)` |
| **Lockup Gain** | Generator | **YES** | 0.0 - 2.0 | `0% - 200% (~X Nm)` |
| **Spin Gain** | Generator | **YES** | 0.0 - 2.0 | `0% - 200% (~X Nm)` |
| **Slide Gain** | Generator | **YES** | 0.0 - 2.0 | `0% - 200% (~X Nm)` |
| **Road Gain** | Generator | **YES** | 0.0 - 2.0 | `0% - 200% (~X Nm)` |
| **Scrub Drag Gain** | Generator | **YES** | 0.0 - 1.0 | `0% - 100% (~X Nm)` |

---

## 3. Implementation: Physics Engine

We modify `FFBEngine.h` to calculate the scaling factor and apply it to all **Generators**.

**File:** `FFBEngine.h`

```cpp
// Inside calculate_force(const TelemInfoV01* data)

    // ... [Existing Setup] ...

    // --- 1. GAIN COMPENSATION (Decoupling) ---
    // Baseline: 20.0 Nm (The standard reference where 1.0 gain was tuned).
    // If MaxTorqueRef increases, we scale effects up to maintain relative intensity.
    double decoupling_scale = (double)m_max_torque_ref / 20.0;
    if (decoupling_scale < 0.1) decoupling_scale = 0.1; // Safety clamp

    // ... [Understeer Logic (Modifier - NO CHANGE)] ...

    // --- 2. SoP & Rear Align (Generators - DECOUPLED) ---
    
    // SoP Base
    // Old: ... * m_sop_scale;
    double sop_base_force = m_sop_lat_g_smoothed * m_sop_effect * (double)m_sop_scale * decoupling_scale;

    // Rear Align Torque
    // Old: ... * m_rear_align_effect;
    double rear_torque = -calc_rear_lat_force * REAR_ALIGN_TORQUE_COEFFICIENT * m_rear_align_effect * decoupling_scale;

    // Yaw Kick
    // Old: ... * 5.0;
    double yaw_force = -1.0 * m_yaw_accel_smoothed * m_sop_yaw_gain * 5.0 * decoupling_scale;

    // Gyro Damping
    // Old: ... * (car_speed / GYRO_SPEED_SCALE);
    double gyro_force = -1.0 * m_steering_velocity_smoothed * m_gyro_gain * (car_speed / GYRO_SPEED_SCALE) * decoupling_scale;

    // ... [Haptics (Generators - DECOUPLED)] ...

    // Lockup
    // Old: ... * 4.0;
    double amp_lock = severity * m_lockup_gain * 4.0 * decoupling_scale;

    // Spin
    // Old: ... * 2.5;
    double amp_spin = severity * m_spin_gain * 2.5 * decoupling_scale;

    // Slide Texture
    // Old: ... * 1.5 * ...;
    slide_noise = sawtooth * m_slide_texture_gain * 1.5 * load_factor * grip_scale * decoupling_scale;

    // Road Texture
    // Old: ... * 50.0 * ...;
    double road_noise = (delta_l + delta_r) * 50.0 * m_road_texture_gain * decoupling_scale;

    // Scrub Drag
    // Old: ... * 5.0 * ...;
    scrub_drag_force = drag_dir * m_scrub_drag_gain * 5.0 * fade * decoupling_scale;

    // Bottoming
    // Old: ... * 20.0;
    double bump_magnitude = intensity * m_bottoming_gain * 0.05 * 20.0 * decoupling_scale;

    // ... [Rest of function] ...
```

---

## 4. Implementation: GUI Layer

We modify `GuiLayer.cpp` to display the dynamic Newton-meter values and standardize the sliders.

**File:** `src/GuiLayer.cpp`

```cpp
// Inside DrawTuningWindow()

    // --- Helper: Format Decoupled Sliders ---
    // val: The current slider value (0.0 - 2.0)
    // base_nm: The physical force this effect produces at Gain 1.0 (Physics Constant)
    auto FormatDecoupled = [&](float val, float base_nm) {
        // Calculate the same scale used in FFBEngine
        float scale = (engine.m_max_torque_ref / 20.0f); 
        if (scale < 0.1f) scale = 0.1f;

        // Calculate estimated output Nm
        float estimated_nm = val * base_nm * scale;

        static char buf[64];
        // Display as Percentage + Dynamic Nm
        snprintf(buf, 64, "%.0f%% (~%.1f Nm)", val * 100.0f, estimated_nm); 
        return buf;
    };

    // --- Helper: Format Standard Percentages ---
    auto FormatPct = [&](float val) {
        static char buf[32];
        snprintf(buf, 32, "%.0f%%", val * 100.0f);
        return buf;
    };

    // ... [General Settings] ...
    
    // Example: Understeer (Modifier - Remapped 0-50 to 0-100%)
    // Note: We keep the internal variable range 0-50 for physics compatibility, 
    // but display it as 0-100% to the user.
    // Or better: Change the slider to 0.0-1.0 and multiply by 50 internally in engine.
    // For now, let's just format the existing 0-50 range.
    FloatSetting("Understeer Effect", &engine.m_understeer_effect, 0.0f, 50.0f, "%.1f"); 
    // (Ideally, refactor Understeer to be 0.0-1.0 internally in a future pass)

    // ... [SoP Section] ...

    // Rear Align (Generator - Base ~6.0 Nm at max load)
    FloatSetting("Rear Align Torque", &engine.m_rear_align_effect, 0.0f, 2.0f, 
                 FormatDecoupled(engine.m_rear_align_effect, 3.0f)); // 3.0 is approx base at gain 1.0

    // Yaw Kick (Generator - Base ~5.0 Nm)
    FloatSetting("SoP Yaw (Kick)", &engine.m_sop_yaw_gain, 0.0f, 2.0f, 
                 FormatDecoupled(engine.m_sop_yaw_gain, 5.0f));

    // ... [Textures Section] ...

    // Slide (Generator - Base ~1.5 Nm)
    FloatSetting("Slide Gain", &engine.m_slide_texture_gain, 0.0f, 2.0f, 
                 FormatDecoupled(engine.m_slide_texture_gain, 1.5f));

    // Road (Generator - Base ~2.5 Nm on avg curb)
    FloatSetting("Road Gain", &engine.m_road_texture_gain, 0.0f, 2.0f, 
                 FormatDecoupled(engine.m_road_texture_gain, 2.5f));
```

---

## 5. Configuration Updates

Since we are changing the effective strength of effects (by adding the scaling factor) and the slider ranges (e.g., Slide Gain max 5.0 -> 2.0), we must update the default presets to ensure a consistent experience.

**File:** `src/Config.cpp`

```cpp
void Config::LoadPresets() {
    presets.clear();
    
    // Update "Default (T300)"
    // Since T300 uses MaxTorqueRef ~100Nm (Scale = 5.0),
    // we need to lower the gains in the preset so the result isn't 5x stronger than before.
    // Old Slide Gain: 0.39. New Scale: 5.0. 
    // New Gain should be: 0.39 / 5.0 = ~0.08? 
    // WAIT: The goal is that 0.39 *should* feel like 39%.
    // If we decouple, setting it to 0.39 will result in (0.39 * 5.0) = 1.95x force.
    // This is actually what we WANT. The previous 0.39 was too weak on T300.
    // So we can likely keep the preset values, or tune them slightly down if they become too strong.
    
    presets.push_back(Preset("Default (T300)", true)
        .SetMaxTorque(98.3f)
        .SetSlide(true, 0.40f) // 40% strength
        .SetRoad(false, 0.50f) // 50% strength
        // ...
    );
}
```

---

## 6. Legacy Configuration Migration (Safety Strategy)

### The Risk
Previous versions of LMUFFB allowed users to set texture gains (Slide, Road, Lockup) up to **5.0** or even **20.0** to overcome the signal compression caused by high `Max Torque Ref` settings.

With the new **Gain Compensation** logic:
*   A user with `Max Torque Ref = 100 Nm` gets a **5x** internal boost automatically.
*   If they load a legacy config with `Slide Gain = 5.0`:
    *   **Result:** $5.0 \text{ (Gain)} \times 5.0 \text{ (Scale)} = 25.0 \text{ (Internal Force)}$.
    *   **Consequence:** Immediate hard clipping and potentially violent wheel oscillation on startup.

### The Fix: Load-Time Clamping
We must implement a safety layer in `Config::Load` to sanitize values coming from older `.ini` files.

**Logic:**
When parsing keys for **Generator** effects, we enforce the new maximums immediately.

```cpp
// Pseudo-code for Config::Load
if (key == "slide_gain") {
    float val = std::stof(value);
    // Clamp legacy 5.0 values to new safe max of 2.0
    engine.m_slide_texture_gain = (std::min)(2.0f, val); 
}
```

**Keys to Clamp to 2.0f:**
*   `rear_align_effect`
*   `sop_yaw_gain`
*   `sop_effect` (Lateral G)
*   `lockup_gain`
*   `spin_gain`
*   `slide_gain`
*   `road_gain`

**Keys to Clamp to 1.0f:**
*   `scrub_drag_gain`
*   `gyro_gain`

This ensures that the first run after the update is safe, even if the resulting FFB is still stronger than before (which is the intended improvement).

