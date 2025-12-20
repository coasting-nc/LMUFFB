## Question

Is the  "100 Nm" Scaling Issue perhaps caused by the Sop Scale value at 20?

## Answer

**Yes, you have hit the nail on the head.**

The `SoP Scale` value of **20.0** is the primary reason your calculated forces are reaching such high levels (40–60 Nm), which forces you to set `Max Torque Ref` to **100 Nm** to prevent clipping.

Here is the math behind what is happening in your setup:

1.  **The SoP Calculation:**
    *   Formula: `Lateral G` × `SoP Effect` × `SoP Scale`.
    *   Typical Corner: **2.0 G** × **1.0** (Gain) × **20.0** (Scale) = **40.0 Nm**.
2.  **The Base Force:**
    *   LMU Physics: **~20.0 Nm** (typical for GTE cars).
3.  **The Total:**
    *   40 Nm (SoP) + 20 Nm (Base) = **60.0 Nm**.
4.  **The Scaling (Your Setting):**
    *   To fit this 60 Nm signal into your wheel without hitting the limit (clipping), you correctly set `Max Torque Ref` to **100 Nm**.
    *   Output: 60 / 100 = **0.60** (60% strength). This feels great on a T300.

### The "Side Effect" on Textures
This setup creates a problem for the **Slide Texture** (and other vibrations):
*   The Slide Texture formula generates a fixed force of about **3.0 Nm**.
*   When you divide that by your **100 Nm** reference: `3.0 / 100 = 0.03`.
*   You are sending **3% force** to the wheel. The T300 belt friction eats that alive.

### The Solution
You have two options to fix this imbalance:

**Option A: Rebalance the Macro Forces (Lower SoP)**
If you lower `SoP Scale` to **5.0** (the old default):
*   SoP becomes: 2.0 G × 5.0 = **10 Nm**.
*   Total becomes: 20 (Base) + 10 (SoP) = **30 Nm**.
*   You can now lower `Max Torque Ref` to **30-40 Nm**.
*   **Result:** The Slide Texture (3 Nm) is now `3 / 30 = 0.10` (**10% force**). You will feel it clearly.
*   *Trade-off:* The SoP effect will feel weaker relative to the car's natural steering weight.

**Option B: Boost the Micro Forces (Keep SoP High)**
If you like the heavy SoP feel, keep `Max Torque Ref` at 100 Nm, but we need to drastically boost the texture gain.
*   Currently, the slider maxes at **2.0**.
*   We need to allow it to go to **10.0** or **20.0** so the texture generates **30 Nm** (internal math) to result in a 30% output signal.

**Recommendation:**
For now, try **Option A** to verify the physics. Set `SoP Scale` to **5.0** and `Max Torque Ref` to **30.0**. This should make the Slide Texture immediately obvious.

## Question 2

So it seems the SoP scale should be lowered in order not to affect other FFB effects. If there is the trade off that the "SoP effect will feel weaker relative to the car's natural steering weight", we should adjust SoP specific sliders, instead of sliders that affect other effects. What should be a good default value, and a good range for the SoP scale slider? Right now, the slider goes up to 200.0, which seems disproportionate.


## Answer 2

You are absolutely correct. The current range (up to 200.0) is absurdly high for the new Newton-meter based math, and the default of 20.0 is aggressive enough that it forces the "100 Nm" scaling issue you discovered.

Here is the corrected calibration strategy.

### The Logic: Balancing the Mix
To fix the "Texture Visibility" issue without breaking the "SoP Weight," we need to bring the SoP forces down to a level comparable to the Base Game Physics (~15-25 Nm). This allows us to lower `Max Torque Ref`, which automatically "zooms in" on the textures.

**The Math:**
*   **Lateral G:** ~2.0 G (Cornering).
*   **Target SoP Force:** ~10.0 Nm (A strong but reasonable overlay).
*   **Formula:** $2.0 \text{ G} \times \text{Scale} = 10.0 \text{ Nm}$.
*   **Ideal Scale:** **5.0**.

### Recommended Values

*   **Default Value:** **5.0** (Was 20.0).
    *   *Result:* 2G turn = 10 Nm SoP. Added to ~20 Nm Base force = 30 Nm Total.
    *   *Benefit:* You can set `Max Torque Ref` to **30-40 Nm**. Textures (3 Nm) become **10%** of the signal (clearly felt).
*   **Slider Range:** **0.0 to 20.0** (Was 0.0 to 200.0).
    *   *Max (20.0):* 2G turn = 40 Nm SoP. This is massive (2x the car's actual steering force). Anything beyond this is just clipping.

### Code Changes Required

We need to update `src/Config.h` (Defaults) and `src/GuiLayer.cpp` (Slider Range).

#### 1. Update `src/Config.h`
Change the default `sop_scale` back to 5.0.

```cpp
// src/Config.h
struct Preset {
    // ...
    float sop = 1.0f;
    float sop_scale = 5.0f; // CHANGED: 20.0 -> 5.0 (Balanced for Nm)
    // ...
```

#### 2. Update `src/GuiLayer.cpp`
Clamp the slider range to a usable scale.

```cpp
// src/GuiLayer.cpp
// Inside DrawTuningWindow...

// Old
// FloatSetting("SoP Scale", &engine.m_sop_scale, 0.0f, 200.0f, "%.1f");

// New
FloatSetting("SoP Scale", &engine.m_sop_scale, 0.0f, 20.0f, "%.1f");
if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scales Lateral G to Nm.\n5.0 = Balanced (10Nm at 2G).\n20.0 = Heavy (40Nm at 2G).");
```

### How to compensate for the "Weaker Feel"
If you lower `SoP Scale` to 5.0 and the wheel feels too light in corners:
1.  **Do NOT** increase `SoP Scale` back to 20.
2.  **INSTEAD**, lower `Max Torque Ref` (e.g., from 100 down to 35 or 40).
    *   This restores the *strength* of the SoP.
    *   Crucially, it **also boosts the textures** by the same amount.

This maintains the **Ratio** between SoP and Textures, which is what was broken in your previous setup.
 