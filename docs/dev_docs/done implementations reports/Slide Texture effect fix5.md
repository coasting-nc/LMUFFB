## Question

I did the test again after applying the frequency fix. Now I can feel the rumble in both scenarios (plow and donut). See the screenshot. The effect now is sensitive, I found an acceptable setting at around 0.8. Maybe the slider max value is now too high?


## Answer

Yes, now that the frequency is matched to your hardware's capability, the energy transfer is much more efficient, making the effect feel significantly stronger at lower gain settings.

If **0.88** feels good with a `Max Torque Ref` of **20.9 Nm**, then a slider range going up to **20.0** is indeed too high (it makes fine-tuning difficult because a small mouse movement changes the value too much).

However, we must remember the "100 Nm" scenario:
*   At **20 Nm Ref**, you like Gain **0.88**.
*   If you switch back to **100 Nm Ref**, you would need roughly **5x** that gain ($0.88 \times 5 = 4.4$) to feel the same force.

**Recommendation:**
Reduce the maximum slider value to **5.0**.
This provides enough headroom for high-torque settings (up to ~100 Nm Ref) while offering much better precision for your current setup.

### 1. Update `src/GuiLayer.cpp`

Change the max value from `20.0` to `5.0` for the texture effects.

```cpp
// src/GuiLayer.cpp

// ... inside DrawTuningWindow ...

    ImGui::Separator();
    ImGui::Text("Haptics (Dynamic)");
    BoolSetting("Progressive Lockup", &engine.m_lockup_enabled);
    if (engine.m_lockup_enabled) {
        // CHANGED: 20.0 -> 5.0
        ImGui::SameLine(); FloatSetting("##Lockup", &engine.m_lockup_gain, 0.0f, 5.0f, "Gain: %.2f");
    }
    
    BoolSetting("Spin Traction Loss", &engine.m_spin_enabled);
    if (engine.m_spin_enabled) {
        // CHANGED: 20.0 -> 5.0
        ImGui::SameLine(); FloatSetting("##Spin", &engine.m_spin_gain, 0.0f, 5.0f, "Gain: %.2f");
    }

    // ...

    ImGui::Separator();
    ImGui::Text("Textures");
    BoolSetting("Slide Rumble", &engine.m_slide_texture_enabled);
    if (engine.m_slide_texture_enabled) {
        ImGui::Indent();
        // CHANGED: 20.0 -> 5.0
        FloatSetting("Slide Gain", &engine.m_slide_texture_gain, 0.0f, 5.0f);
        ImGui::Unindent();
    }
    BoolSetting("Road Details", &engine.m_road_texture_enabled);
    if (engine.m_road_texture_enabled) {
        ImGui::Indent();
        // CHANGED: 20.0 -> 5.0
        FloatSetting("Road Gain", &engine.m_road_texture_gain, 0.0f, 5.0f);
        ImGui::Unindent();
    }
```

### 2. Updated Guide Instruction

Since "Max" is no longer necessary or safe, here is the refined instruction for the guide.

**Quick Setup (Preset):**
*   Load Preset: **"Guide: Slide Texture (Scrub)"**
*   *Note:* Start with **Slide Gain** at **1.0**. Increase if too subtle, decrease if the wheel rattles too violently.

**The Test:**
*Option A: The Plow (Recommended)*
1.  Drive at **80 - 100 km/h** in a runoff area.
2.  Turn the wheel **90 to 180 degrees** rapidly and hold.
3.  The car should refuse to turn and "plow" straight.
4.  **Feel:** A gritty vibration in the rim.
    *   *Tuning:* If it feels like a "buzz" but has no power, increase Gain. If it feels like a "hammer," decrease Gain.

*Option B: The Donut*
1.  Stop. 1st Gear.
2.  Turn fully to lock. Full Throttle.
3.  **Feel:** Continuous rumble from the rear tires spinning and sliding.