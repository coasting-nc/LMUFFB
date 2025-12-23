### 1. Terminology: "Gain Compensation"

You are looking for a term that describes adjusting specific signal components to maintain their perceived intensity relative to a changing master scale.

In signal processing and audio engineering, the most precise terms are:

1.  **Gain Compensation (or Make-up Gain):** This is the standard term used in compressors/limiters. When you compress a signal (reduce its dynamic range), you apply "Make-up Gain" to bring the peak level back up. Here, when you increase `Max Torque Ref` (which compresses the signal ratio), we apply gain to the effects to restore their perceived level.
2.  **Ratio-metric Scaling:** This describes maintaining the *ratio* between the "Micro" forces (textures) and the "Macro" forces (steering torque) regardless of the absolute output scale.
3.  **Perceptual Loudness Matching:** In audio (ISO 226), this ensures bass/treble are boosted at low volumes so the human ear perceives the same balance. Similarly, we are boosting "tactile treble" (vibrations) so the human hand perceives the same detail even when the "volume" (Torque Ref) changes.

**Recommendation:** **"Gain Compensation"** is the most technically accurate engineering term for what we are doing.

---

### 2. Re-evaluation of Understeer & Oversteer Boost

You asked to reconsider decoupling these. Let's look at the math in `FFBEngine.h`.

#### A. Understeer Effect (Grip Modulation)
*   **Formula:** `Factor = 1.0 - ((1.0 - Grip) * UndersteerGain)`
*   **Type:** **Modifier (Multiplicative)**. It does not generate force; it reduces existing force.
*   **Scenario:**
    *   Grip drops by 20% (0.8).
    *   Gain is 1.0.
    *   Result: Force is reduced by 20%.
*   **If we Decouple (Scale) it:**
    *   If `Max Torque Ref` is 100Nm, Scale is 5.0.
    *   Gain becomes 5.0.
    *   Result: Force is reduced by $0.2 \times 5.0 = 100\%$.
*   **The Problem:** Scaling this slider changes the **Physics Sensitivity**, not just the intensity. It would make the drop-off extremely abrupt (binary) on high-torque settings.
*   **Conclusion:** **DO NOT Decouple.** A 20% drop in force is physically meaningful regardless of whether the max force is 20Nm or 100Nm.
*   **UI Proposal:** However, you are right about the **Slider Presentation**. Currently 0-50 is arbitrary. We should map this to **0% - 100%** (or 0.0 - 1.0 internally) to make it intuitive.

#### B. Oversteer Boost
*   **Formula:** `SoP_Total *= (1.0 + (GripDelta * BoostGain))`
*   **Type:** **Modifier (Multiplicative)**.
*   **The Logic:** This multiplies the `SoP Force`.
*   **Inheritance:** We have already decided to Decouple (Scale) the `SoP Force`.
    *   If `SoP Force` is scaled up by 5x, and we multiply it by `Boost`, the `Boost` effect is **automatically scaled up by 5x** because the base number is larger.
*   **If we Decouple Boost too:** We would multiply a 5x larger force by a 5x larger boost. Result = 25x force. This is a "Double Scaling" error.
*   **Conclusion:** **DO NOT Decouple.** It inherits the scaling from the SoP effect it modifies.

---

### 3. Comprehensive Slider Table

Here is the plan for every slider in the GUI.

*   **Generator:** Adds Newtons (Needs Decoupling/Gain Compensation).
*   **Modifier:** Multiplies/Scales existing Newtons (Do NOT Decouple).
*   **Reference:** Sets the scale (The Source).

| Section | Slider Name | Type | Current Range | **Decouple?** | **Proposed UI Range / Format** |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **General** | **Master Gain** | Global Scalar | 0.0 - 2.0 | **NO** | 0% - 200% |
| **General** | **Max Torque Ref** | Reference | 1.0 - 200.0 | **NO** | 1.0 - 200.0 Nm (Source of Scaling) |
| **General** | **Min Force** | Post-Process | 0.0 - 0.20 | **NO** | 0.0 - 20.0% (Keep as is) |
| **General** | **Load Cap** | Limiter | 1.0 - 3.0 | **NO** | 1.0x - 3.0x |
| **Understeer** | **Steering Shaft Gain** | Attenuator | 0.0 - 1.0 | **NO** | 0% - 100% |
| **Understeer** | **Understeer Effect** | Modifier | 0.0 - 50.0 | **NO** | **0% - 100%** (Remap internal 0-50 to 0-100 UI) |
| **Oversteer** | **Oversteer Boost** | Modifier | 0.0 - 20.0 | **NO** | **0% - 200%** (Remap for clarity) |
| **SoP** | **Rear Align Torque** | Generator | 0.0 - 20.0 | **YES** | 0% - 200% (Show dynamic Nm in text) |
| **SoP** | **SoP Yaw (Kick)** | Generator | 0.0 - 20.0 | **YES** | 0% - 200% (Show dynamic Nm in text) |
| **SoP** | **Gyro Damping** | Generator | 0.0 - 1.0 | **YES** | 0% - 100% (Show dynamic Nm in text) |
| **SoP** | **Lateral G (SoP)** | Generator | 0.0 - 20.0 | **YES** | 0% - 200% (Show dynamic Nm in text) |
| **Haptics** | **Lockup Gain** | Generator | 0.0 - 5.0 | **YES** | 0% - 100% (Show dynamic Nm in text) |
| **Haptics** | **Spin Gain** | Generator | 0.0 - 5.0 | **YES** | 0% - 100% (Show dynamic Nm in text) |
| **Textures** | **Slide Gain** | Generator | 0.0 - 5.0 | **YES** | 0% - 100% (Show dynamic Nm in text) |
| **Textures** | **Road Gain** | Generator | 0.0 - 5.0 | **YES** | 0% - 100% (Show dynamic Nm in text) |
| **Textures** | **Scrub Drag Gain** | Generator | 0.0 - 1.0 | **YES** | 0% - 100% (Show dynamic Nm in text) |

### 4. Implementation Details for "Dynamic Nm Display"

For the sliders marked **YES**, we will use the `FormatNm` helper in the GUI. This satisfies your request to "show the final value in Newtons depending on the current position".

**Example Logic for `GuiLayer.cpp`:**

```cpp
// Helper to format slider text with Dynamic Nm calculation
// gain: The variable being edited
// base_nm: The approximate Nm this effect produces at Gain 1.0 (Physics Constant)
auto FormatDecoupled = [&](float val, float base_nm) {
    // 1. Calculate the Decoupling Scale (Same as FFBEngine)
    float scale = (engine.m_max_torque_ref / 20.0f); 
    if (scale < 0.1f) scale = 0.1f;

    // 2. Calculate estimated output Nm
    float estimated_nm = val * base_nm * scale;

    // 3. Format string: "50% (~3.5 Nm)"
    static char buf[64];
    // Assuming slider is 0.0-1.0 (mapped to 0-100%)
    snprintf(buf, 64, "%.0f%% (~%.1f Nm)", val * 100.0f, estimated_nm); 
    return buf;
};
```

### 5. Summary of Changes

1.  **FFBEngine.h**: Apply `decoupling_scale` to all **Generators** (SoP, Rear Align, Yaw, Gyro, Textures). Do **not** apply to Modifiers (Understeer, Boost).
2.  **GuiLayer.cpp**:
    *   Update slider ranges to be consistent (mostly 0.0 to 1.0 or 2.0).
    *   Use the `FormatDecoupled` helper to show the user the *actual* force they are dialing in.
    *   Rename "Understeer Effect" range to 0-100% (internally mapping to the physics constant).

This approach ensures that:
1.  **Consistency:** Users see "50%" on a slider and know it's half strength.
2.  **Transparency:** Users see "(~5.0 Nm)" and know exactly how much force that 50% represents on *their* specific wheel setup.
3.  **Physics Integrity:** We don't break the sensitivity of Understeer/Oversteer modifiers.