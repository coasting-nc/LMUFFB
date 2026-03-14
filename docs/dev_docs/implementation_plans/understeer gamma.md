The "cliff" you are feeling is the mathematical nature of a squared function ($x^2$). It stays very flat near zero (holding the force high) and then suddenly ramps up steeply (dropping the force off a cliff). 

Your suggestion to add a **Gamma** slider is the perfect solution. It will allow you to sculpt the exact shape of the grip falloff curve:
*   **Gamma = 1.0 (Linear):** A smooth, constant drop in force as grip is lost.
*   **Gamma > 1.0 (e.g., 2.0):** Holds the force strong near the center, then drops off a cliff (what you just experienced).
*   **Gamma < 1.0 (e.g., 0.5):** Drops the force immediately as soon as you turn the wheel, which will **eliminate that initial turn-in resistance** you mentioned.

Here are the exact code changes to add the **Understeer Gamma** slider to the UI and physics engine.

### 1. `src/FFBEngine.h`
Add the new variable to the `FFBEngine` class (around line 100, near `m_understeer_effect`):
```cpp
    float m_gain;
    float m_understeer_effect;
    float m_understeer_gamma = 1.0f; // NEW: Gamma curve for understeer
    float m_sop_effect;
```

### 2. `src/FFBEngine.cpp`
Update the math in `calculate_force` to use the new gamma variable instead of the hardcoded square.
```cpp
    // Base Steering Force
    double base_input = game_force_proc;
    
    // --- REWRITTEN: Gamma-Shaped Grip Modulation ---
    double raw_loss = std::clamp(1.0 - ctx.avg_front_grip, 0.0, 1.0);
    
    // Apply Gamma curve (pow)
    double shaped_loss = std::pow(raw_loss, (double)m_understeer_gamma); 
    
    double grip_loss = shaped_loss * m_understeer_effect;
    ctx.grip_factor = (std::max)(0.0, 1.0 - grip_loss);

    // v0.7.63: Passthrough Logic for Direct Torque (TIC mode)
    double grip_factor_applied = m_torque_passthrough ? 1.0 : ctx.grip_factor;
```

### 3. `src/Config.h`
We need to add this to the `Preset` struct so it saves and loads correctly.

**A. Add to the variables (around line 20):**
```cpp
    float gain = 1.0f;
    float understeer = 1.0f;  
    float understeer_gamma = 1.0f; // NEW
    float sop = 1.666f;
```

**B. Add a fluent setter (around line 130):**
```cpp
    Preset& SetUndersteer(float v) { understeer = v; return *this; }
    Preset& SetUndersteerGamma(float v) { understeer_gamma = v; return *this; } // NEW
```

**C. Add to `Apply()` (around line 240):**
```cpp
        engine.m_gain = (std::max)(0.0f, gain);
        engine.m_understeer_effect = (std::max)(0.0f, (std::min)(2.0f, understeer));
        engine.m_understeer_gamma = (std::max)(0.1f, (std::min)(4.0f, understeer_gamma)); // NEW
```

**D. Add to `Validate()` (around line 330):**
```cpp
        gain = (std::max)(0.0f, gain);
        understeer = (std::max)(0.0f, (std::min)(2.0f, understeer));
        understeer_gamma = (std::max)(0.1f, (std::min)(4.0f, understeer_gamma)); // NEW
```

**E. Add to `UpdateFromEngine()` (around line 380):**
```cpp
        gain = engine.m_gain;
        understeer = engine.m_understeer_effect;
        understeer_gamma = engine.m_understeer_gamma; // NEW
```

**F. Add to `Equals()` (around line 480):**
```cpp
        if (!is_near(gain, p.gain, eps)) return false;
        if (!is_near(understeer, p.understeer, eps)) return false;
        if (!is_near(understeer_gamma, p.understeer_gamma, eps)) return false; // NEW
```

### 4. `src/Config.cpp`
Add the parsing and saving logic.

**A. In `ParsePhysicsLine` (around line 60):**
```cpp
    if (key == "understeer") {
        float val = std::stof(value);
        if (val > 2.0f) val /= 100.0f;
        current_preset.understeer = (std::min)(2.0f, (std::max)(0.0f, val));
        return true;
    }
    if (key == "understeer_gamma") { current_preset.understeer_gamma = std::stof(value); return true; } // NEW
```

**B. In `SyncPhysicsLine` (around line 160):**
```cpp
    if (key == "understeer") {
        float val = std::stof(value);
        if (val > 2.0f) val /= 100.0f;
        engine.m_understeer_effect = (std::min)(2.0f, (std::max)(0.0f, val));
        return true;
    }
    if (key == "understeer_gamma") { engine.m_understeer_gamma = std::stof(value); return true; } // NEW
```

**C. In `WritePresetFields` (around line 580):**
```cpp
    file << "steering_shaft_smoothing=" << p.steering_shaft_smoothing << "\n";
    file << "understeer=" << p.understeer << "\n";
    file << "understeer_gamma=" << p.understeer_gamma << "\n"; // NEW
```

**D. In `Save` (around line 850):**
```cpp
        file << "steering_shaft_smoothing=" << engine.m_steering_shaft_smoothing << "\n";
        file << "understeer=" << engine.m_understeer_effect << "\n";
        file << "understeer_gamma=" << engine.m_understeer_gamma << "\n"; // NEW
```

### 5. `src/Tooltips.h`
Add the tooltip text (around line 40):
```cpp
    inline constexpr const char* STEERING_SHAFT_SMOOTHING = "Low Pass Filter applied ONLY to the raw game force.";
    inline constexpr const char* UNDERSTEER_EFFECT = "Scales how much front grip loss reduces steering force.";
    inline constexpr const char* UNDERSTEER_GAMMA = "Shapes the grip loss curve.\n1.0 = Linear (Constant drop).\n> 1.0 = Holds force longer, then drops sharply (Cliff).\n< 1.0 = Drops force immediately off-center (Softer initial turn-in)."; // NEW
```
*(Don't forget to add `UNDERSTEER_GAMMA` to the `ALL` array at the bottom of `Tooltips.h`)*

### 6. `src/GuiLayer_Common.cpp`
Finally, add the slider to the UI (around line 350, under the Understeer Effect slider):
```cpp
        FloatSetting("Understeer Effect", &engine.m_understeer_effect, 0.0f, 2.0f, FormatPct(engine.m_understeer_effect),
            Tooltips::UNDERSTEER_EFFECT);
            
        // NEW: Gamma Slider
        FloatSetting("  Response Curve (Gamma)", &engine.m_understeer_gamma, 0.1f, 4.0f, "%.2f", 
            Tooltips::UNDERSTEER_GAMMA);
```

### How to Tune It Now:
Once compiled, you will have a new **Response Curve (Gamma)** slider right below the Understeer Effect.
1. Set it to **`1.0`** to feel the original linear drop.
2. Set it to **`2.0`** to feel the "cliff" you just experienced.
3. Set it to **`0.5`** to eliminate that initial turn-in resistance (the wheel will start getting lighter the moment the tires begin to slip, making the center feel much softer and more progressive).