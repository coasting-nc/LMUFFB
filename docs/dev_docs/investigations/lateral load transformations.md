
# lateral load transformations report

Yes, applying a mathematical transformation to the shape of the curve is a highly effective and standard way to solve this exact problem in Force Feedback and joystick response tuning. 

### Why it feels "notchy"
Your current formula `(fl_load - fr_load) / (fl_load + fr_load)` produces a linear output from `-1.0` to `1.0`. The "notchiness" happens because as the car reaches maximum load transfer (e.g., the inside tire lifts off the ground and reaches `0` load), the formula hits a hard mathematical ceiling at `1.0` or `-1.0`. 

In FFB terms, hitting a linear ceiling feels like hitting a sudden wall. To fix this, you need an **S-curve (easing function)** that flattens out as it approaches the extremes, gently "leaning" into the maximum force rather than snapping to it.

### The Best Transformations

Here are the two best functions to apply to your normalized value. Both map `[-1, 1]` to `[-1, 1]`, but they reduce the derivative (slope) to `0` at the extremes.

#### 1. The Cubic Polynomial (Recommended)
This is computationally very cheap and perfectly rounds off the peaks.
**Formula:** $f(x) = 1.5x - 0.5x^3$

#### 2. The Sine Curve
This uses trigonometry to create a perfectly smooth roll-off. It behaves very similarly to the cubic polynomial but is slightly more expensive to compute.
**Formula:** $f(x) = \sin(x \cdot \frac{\pi}{2})$

---

### How to implement it in your codebase

If you look at `src/FFBEngine.cpp`, inside the `calculate_sop_lateral` function, you are currently calculating it like this:

```cpp
double total_load = fl_load + fr_load;
double lat_load_norm = (total_load > 1.0) ? (fl_load - fr_load) / total_load : 0.0;
```

You can apply the **Cubic Polynomial** transformation right after calculating `lat_load_norm` and *before* it gets smoothed. Here is the updated code:

```cpp
double total_load = fl_load + fr_load;
double lat_load_norm = (total_load > 1.0) ? (fl_load - fr_load) / total_load : 0.0;

// 1. Safety clamp to ensure we stay strictly within [-1, 1] 
// (in case game telemetry glitches and outputs negative tire loads during a crash)
lat_load_norm = std::clamp(lat_load_norm, -1.0, 1.0);

// 2. Apply Cubic S-Curve to soften the extremes and remove the "notch"
// f(x) = 1.5x - 0.5x^3
lat_load_norm = 1.5 * lat_load_norm - 0.5 * (lat_load_norm * lat_load_norm * lat_load_norm);

// (Existing smoothing logic follows...)
double smoothness = (double)m_sop_smoothing_factor;
```

### Why this is better than the other proposals you received:

1. **"Not being a proportional force" (i.e., just using `fl_load - fr_load`)**: If you remove the denominator, the force will scale with aerodynamic downforce. A Hypercar at 300 km/h will produce massively more lateral FFB than at 50 km/h, making the wheel impossibly heavy on fast corners. Keeping it proportional (normalized) is the right choice for consistent feel.
2. **"Replacing the denominator with static terms"**: If you divide by a static weight (e.g., `STATIC_FRONT_WEIGHT`), you run into the exact same aero problem mentioned above. The percentage-based load transfer you are currently using is much better for isolating the *chassis roll* sensation.

### What to expect from this change:
By applying the cubic transformation, the center of the wheel will become slightly more responsive (about 50% steeper slope at `x = 0`), which improves the feeling of connection when driving straight. As the car leans heavily into a corner and approaches 100% load transfer, the force will smoothly taper off, completely eliminating the sharp "notch" at the limit.



## Cubic parametric version

Yes, you can absolutely make this parametric! This is a very common technique in sim racing FFB tuning, often referred to as "Soft Clipping," "Rolloff," or "Gamma."

To do this efficiently without using expensive math functions like `pow()`, we can use **Linear Interpolation (Lerp)** to blend between the raw linear formula and the fully smoothed cubic formula. 

Let's introduce a parameter **$k$** (your GUI slider), which ranges from `0.0` to `1.0`:
* **$k = 0.0$**: Pure linear (sharp peak, notchy).
* **$k = 1.0$**: Full cubic smoothing (flat peak, very smooth).

### The Parametric Formula
If we blend $f(x) = x$ and $f(x) = 1.5x - 0.5x^3$ using parameter $k$, the simplified formula becomes:

$$f(x, k) = (1 + 0.5k)x - 0.5kx^3$$

This formula is brilliant for C++ FFB loops because:
1. It only requires basic multiplication (extremely fast).
2. No matter what $k$ is set to, an input of `1.0` will *always* output exactly `1.0`. It only changes the *shape* of the curve getting there.
3. The slope at the peak becomes exactly $1 - k$. So at $k=1$, the slope is $0$ (perfectly flat). At $k=0.5$, the slope is $0.5$ (half-rounded).



### How to implement this in your C++ Codebase

**1. Add the variable to `src/FFBEngine.h`**
Inside the `FFBEngine` class (around line 130 where your settings are):
```cpp
float m_lat_load_effect = 0.0f; 
float m_lat_load_rolloff = 1.0f; // NEW: 0.0 = Sharp, 1.0 = Smooth
```

**2. Update the physics in `src/FFBEngine.cpp`**
Inside `calculate_sop_lateral`:
```cpp
double total_load = fl_load + fr_load;
double lat_load_norm = (total_load > 1.0) ? (fl_load - fr_load) / total_load : 0.0;

// --- NEW: Parametric Rolloff ---
// Safety clamp
lat_load_norm = std::clamp(lat_load_norm, -1.0, 1.0);

// Apply parametric cubic smoothing: f(x) = (1 + 0.5k)x - 0.5k(x^3)
double k = (double)m_lat_load_rolloff;
lat_load_norm = (1.0 + 0.5 * k) * lat_load_norm - (0.5 * k) * (lat_load_norm * lat_load_norm * lat_load_norm);
// -------------------------------

// Smoothing: Map 0.0-1.0 slider to 0.1-0.0001s tau
```

**3. Add the GUI Slider in `src/GuiLayer_Common.cpp`**
Under the "Rear Axle (Oversteer)" section, right below where you draw the `m_lat_load_effect` slider:
```cpp
FloatSetting("Lateral Load", &engine.m_lat_load_effect, 0.0f, 2.0f, FormatDecoupled(engine.m_lat_load_effect, FFBEngine::BASE_NM_SOP_LATERAL), Tooltips::LATERAL_LOAD);

// NEW SLIDER
FloatSetting("  Load Rolloff (Smoothness)", &engine.m_lat_load_rolloff, 0.0f, 1.0f, "%.2f", "0.0 = Sharp/Notchy limit\n1.0 = Smooth/Rounded limit");
```

*(Note: You will also want to add `m_lat_load_rolloff` to `Config.h` and `Config.cpp` so it saves to the user's `.ini` file, just like you did for `m_lat_load_effect`)*.

This gives the user total control. If they like the raw, jagged, mechanical feel of the tire snapping, they can set it to `0.0`. If they want it to feel like a progressive, flexing rubber tire, they can set it to `1.0`.

## Cubic vs Quadratic


If you simply push **$k > 1.0$** in the previous Cubic formula, the math breaks. 

Because of how polynomials work, setting $k = 1.5$ would cause the formula to overshoot `1.0` (it would peak at around `1.08`) and then dip back down. In the steering wheel, this would feel like a weird, artificial "bump" of resistance right before the tires lose grip, which is not what you want.

To make the peak **even broader and softer** (bending earlier and spending more time flattening out), you are exactly right: **you need to change the exponent.**

### The Math: Lowering the Exponent
To make the dome wider, we actually need to *lower* the exponent from 3 (Cubic) to 2 (Quadratic). 

However, because lateral load goes from `-1.0` to `1.0`, a standard squared function ($x^2$) would destroy the negative values (turning left vs turning right). To fix this, we use a **Signed Quadratic** formula: $x \cdot |x|$.

The ultimate "broad dome" formula that peaks perfectly at `1.0` with a flat slope is:
$$f(x) = 2x - x|x|$$

### The New Parametric Formula
Just like before, we can blend between the raw Linear formula ($k=0$) and this new, ultra-smooth Quadratic formula ($k=1$). The math for this is incredibly elegant and fast for C++:

$$f(x, k) = (1 + k)x - k \cdot x \cdot |x|$$

* **$k = 0.0$**: Linear (Sharp peak, notchy)
* **$k = 0.5$**: Moderate smoothing
* **$k = 1.0$**: Full Quadratic (Massive, broad, soft dome)

*(Note: Because this formula uses `abs(x)` instead of `x^3`, it is actually slightly faster for the CPU to calculate!)*


### How to implement the Quadratic version in C++
If you prefer this broader feel, you can replace the cubic implementation in `src/FFBEngine.cpp` with this:

```cpp
double total_load = fl_load + fr_load;
double lat_load_norm = (total_load > 1.0) ? (fl_load - fr_load) / total_load : 0.0;

// Safety clamp
lat_load_norm = std::clamp(lat_load_norm, -1.0, 1.0);

// Apply Parametric Quadratic Rolloff: f(x) = (1 + k)x - k * x * |x|
double k = (double)m_lat_load_rolloff; // GUI Slider from 0.0 to 1.0
lat_load_norm = (1.0 + k) * lat_load_norm - k * lat_load_norm * std::abs(lat_load_norm);
```

### Summary
* **Can you just set $k > 1$ on the old formula?** No, it causes a mathematical overshoot.
* **Do you need a different formula?** Yes.
* **What is the formula?** You lower the exponent to 2, resulting in the Signed Quadratic formula: `(1 + k)x - k * x * |x|`. This gives you the widest, softest peak possible without using heavy trigonometry or square roots.

## Cubic Hermite Spline

To control the beginning of the curve (center steepness) and the end of the curve (peak broadness) completely independently, we use a mathematical concept called a **Cubic Hermite Spline**. 

This allows us to define exactly what the slope (steepness) should be at `x = 0` (driving straight) and what the slope should be at `x = 1` (the limit of grip), and the math smoothly connects them.

### The Two Parameters
We will introduce two new GUI sliders:
1. **`center_slope` ($S_0$)**: Controls the steepness from 0.0 to 0.5. 
   * `1.0` = Linear.
   * `0.5` = Gentle/Soft center (builds up slowly).
   * `1.5` = Aggressive center (heavy wheel immediately upon turning).
2. **`peak_slope` ($S_1$)**: Controls the broadness/notchiness at the top (0.7 to 1.0).
   * `1.0` = Sharp peak (Notchy, sudden drop-off).
   * `0.0` = Perfectly flat peak (Broad, wide dome).
   * `0.5` = A nice middle ground (Rounded, but not overly wide).

*(Fun fact: If you set $S_0 = 1.5$ and $S_1 = 0.0$, the math perfectly recreates the $1.5x - 0.5x^3$ formula from my first response!)*

### How to implement this in your C++ Codebase

Because this is just a cubic polynomial, it is incredibly fast to compute in C++ and perfectly safe for your 400Hz/1000Hz FFB loop.

**1. Add the variables to `src/FFBEngine.h`**
```cpp
float m_lat_load_effect = 0.0f; 
float m_lat_load_center_slope = 1.0f; // 0.5 = Gentle, 1.0 = Linear, 1.5 = Aggressive
float m_lat_load_peak_slope = 1.0f;   // 0.0 = Broad/Flat, 1.0 = Sharp/Notchy
```

**2. Update the physics in `src/FFBEngine.cpp`**
Inside `calculate_sop_lateral`:
```cpp
double total_load = fl_load + fr_load;
double lat_load_norm = (total_load > 1.0) ? (fl_load - fr_load) / total_load : 0.0;

// --- NEW: Independent Hermite Spline Shaping ---
// 1. Safety clamp
lat_load_norm = std::clamp(lat_load_norm, -1.0, 1.0);

// 2. Extract sign and absolute value (since the curve must mirror for left/right turns)
double abs_x = std::abs(lat_load_norm);
double sign_x = (lat_load_norm > 0.0) ? 1.0 : -1.0;

// 3. Get GUI Parameters
double S0 = (double)m_lat_load_center_slope;
double S1 = (double)m_lat_load_peak_slope;

// 4. Calculate Polynomial Coefficients
double a = S0 + S1 - 2.0;
double b = 3.0 - 2.0 * S0 - S1;
double c = S0;

// 5. Apply the curve: f(x) = ax^3 + bx^2 + cx
double shaped_abs_x = (a * abs_x * abs_x * abs_x) + (b * abs_x * abs_x) + (c * abs_x);

// 6. Restore the sign
lat_load_norm = shaped_abs_x * sign_x;
// -----------------------------------------------

// (Existing smoothing logic follows...)
double smoothness = (double)m_sop_smoothing_factor;
```

**3. Add the GUI Sliders in `src/GuiLayer_Common.cpp`**
```cpp
FloatSetting("Lateral Load", &engine.m_lat_load_effect, 0.0f, 2.0f, FormatDecoupled(engine.m_lat_load_effect, FFBEngine::BASE_NM_SOP_LATERAL), Tooltips::LATERAL_LOAD);

// NEW SLIDERS
FloatSetting("  Center Steepness", &engine.m_lat_load_center_slope, 0.1f, 2.0f, "%.2f", "0.5 = Gentle build-up\n1.0 = Linear\n1.5 = Aggressive build-up");
FloatSetting("  Peak Notchiness", &engine.m_lat_load_peak_slope, 0.0f, 1.0f, "%.2f", "0.0 = Broad, flat peak\n1.0 = Sharp, notchy peak");
```

*(Remember to add these two new variables to `Config.h` and `Config.cpp` so they save to the `.ini` file).*

### Summary
By using a **Cubic Hermite Spline**, you get exactly what you asked for: two independent parameters. You can make the initial turn-in as gentle as you want, while independently tuning the exact width and sharpness of the grip limit!


## Generalized Power Spline (Fractional Polynomial Spline)

By replacing the fixed $x^2$ and $x^3$ terms with a variable exponent $E$, we can create a single, unified formula that gives you **three completely independent controls**:
1. **Center Steepness ($S_0$)**: How fast the force builds up from the center.
2. **Peak Notchiness ($S_1$)**: The exact slope at the limit of grip (sharp vs. flat).
3. **Peak Broadness ($E$)**: How "fat" or "gentle" the curve is between the center and the peak. 

Setting $E=2.0$ perfectly recreates the Cubic Spline. Setting $E$ lower (e.g., $1.5$ or $1.2$) makes the curve **even more gentle and broader than a quadratic**, while perfectly respecting your chosen start and end slopes!

### The Generalized Formula
The math to achieve this while maintaining perfect boundary conditions at $x=0$ and $x=1$ is:

$$f(x) = S_0 x + B x^E + C x^{E+1}$$

Where the coefficients $B$ and $C$ are calculated dynamically based on your parameters:
* $B = E + 1 - (E \cdot S_0) - S_1$
* $C = S_1 - E + (E - 1) \cdot S_0$

*(Note: $E$ must be strictly greater than $1.0$. A safe range for the GUI slider is `1.1` to `3.0`)*.

### How to implement this in your C++ Codebase

Because we are using fractional exponents, we have to use `std::pow()`. While slightly slower than simple multiplication, modern CPUs can execute `std::pow` in nanoseconds. Running this once per wheel at 400Hz/1000Hz will have absolutely zero measurable impact on your performance budget.

**1. Add the variables to `src/FFBEngine.h`**
```cpp
float m_lat_load_effect = 0.0f; 
float m_lat_load_center_slope = 1.0f; // S0
float m_lat_load_peak_slope = 0.0f;   // S1
float m_lat_load_broadness = 2.0f;    // E (Exponent)
```

**2. Update the physics in `src/FFBEngine.cpp`**
Inside `calculate_sop_lateral`:
```cpp
double total_load = fl_load + fr_load;
double lat_load_norm = (total_load > 1.0) ? (fl_load - fr_load) / total_load : 0.0;

// --- NEW: Generalized Power Spline ---
// 1. Safety clamp input
lat_load_norm = std::clamp(lat_load_norm, -1.0, 1.0);

// 2. Extract sign and absolute value
double abs_x = std::abs(lat_load_norm);
double sign_x = (lat_load_norm > 0.0) ? 1.0 : -1.0;

// 3. Get GUI Parameters
double S0 = (double)m_lat_load_center_slope;
double S1 = (double)m_lat_load_peak_slope;
double E  = (double)m_lat_load_broadness;

// Safety clamp E to prevent math errors (must be > 1.0)
E = std::max(1.01, E);

// 4. Calculate Dynamic Coefficients
double B = E + 1.0 - (E * S0) - S1;
double C = S1 - E + (E - 1.0) * S0;

// 5. Apply the curve: f(x) = S0*x + B*x^E + C*x^(E+1)
double shaped_abs_x = (S0 * abs_x) + (B * std::pow(abs_x, E)) + (C * std::pow(abs_x, E + 1.0));

// 6. Safety clamp output (prevents overshoot if user sets crazy S0/S1 combos)
shaped_abs_x = std::clamp(shaped_abs_x, 0.0, 1.0);

// 7. Restore the sign
lat_load_norm = shaped_abs_x * sign_x;
// -------------------------------------

// (Existing smoothing logic follows...)
double smoothness = (double)m_sop_smoothing_factor;
```

**3. Add the GUI Sliders in `src/GuiLayer_Common.cpp`**
```cpp
FloatSetting("Lateral Load", &engine.m_lat_load_effect, 0.0f, 2.0f, FormatDecoupled(engine.m_lat_load_effect, FFBEngine::BASE_NM_SOP_LATERAL), Tooltips::LATERAL_LOAD);

// NEW SLIDERS
FloatSetting("  Center Steepness", &engine.m_lat_load_center_slope, 0.1f, 3.0f, "%.2f", "0.5 = Gentle\n1.0 = Linear\n2.0 = Aggressive");
FloatSetting("  Peak Notchiness", &engine.m_lat_load_peak_slope, 0.0f, 1.0f, "%.2f", "0.0 = Flat peak\n1.0 = Sharp peak");
FloatSetting("  Peak Broadness", &engine.m_lat_load_broadness, 1.1f, 4.0f, "%.2f", "1.1 = Ultra-broad/Gentle\n2.0 = Standard Cubic\n3.0 = Narrow/Late");
```

*(Don't forget to add the new variables to `Config.h` and `Config.cpp` for saving/loading).*

You now have a professional-grade, fully parametric FFB shaping curve that allows you to sculpt the exact physical sensation of the tire's limit!


##  The "Locked-Center" Hermite Spline

### The Best Transformation: The "Locked-Center" Hermite Spline

While those above are excellent options, the standard Parametric Cubic and Quadratic formulas have a hidden flaw: **if you flatten the peak, they artificially increase the slope at the center.** This means driving straight will suddenly feel heavier and more twitchy.

The **Cubic Hermite Spline** (Option 5 in your report) is the best choice, but exposing multiple sliders for slopes can confuse end-users. 

Instead, I highly recommend a **custom, optimized version of the Hermite Spline** that uses a single "Smoothness" slider ($k$). It locks the center slope to exactly `1.0` (keeping your current center-steering feel identical) and only applies the smoothing to the outer edges of the curve.

**The Formula:**
$f(x) = x \cdot (1 + k \cdot |x| - k \cdot x^2)$
* Where $x$ is your raw `lat_load_norm` (from -1.0 to 1.0).
* Where $k$ is the user slider from `0.0` (Sharp/Notchy) to `1.0` (Perfectly smooth/flat peak).

### How to implement it in your codebase

Here is the step-by-step implementation to integrate this seamlessly into your app.

#### 1. Add the parameter to `src/FFBEngine.h`
Find your settings variables (around line 130) and add the new rolloff parameter:
```cpp
    float m_sop_effect;
    float m_lat_load_effect = 0.0f; 
    float m_lat_load_rolloff = 1.0f; // NEW: 0.0 = Sharp/Linear, 1.0 = Smooth Peak
```

#### 2. Update the physics in `src/FFBEngine.cpp`
Locate the `calculate_sop_lateral` function. Update the `lat_load_norm` calculation to include the safety clamp and the new transformation:

```cpp
    // 2. Normalized Lateral Load Transfer (Issue #213)
    double fl_load = data->mWheel[0].mTireLoad;
    double fr_load = data->mWheel[1].mTireLoad;
    if (ctx.frame_warn_load) {
        fl_load = calculate_kinematic_load(data, 0);
        fr_load = calculate_kinematic_load(data, 1);
    }
    double total_load = fl_load + fr_load;
    double lat_load_norm = (total_load > 1.0) ? (fl_load - fr_load) / total_load : 0.0;
    
    // --- NEW: Anti-Notch Smoothing Transformation ---
    // 1. Safety clamp to strictly [-1.0, 1.0]
    lat_load_norm = std::clamp(lat_load_norm, -1.0, 1.0);
    
    // 2. Apply Locked-Center Hermite Spline: f(x) = x * (1 + k*|x| - k*x^2)
    double k = (double)m_lat_load_rolloff;
    double abs_x = std::abs(lat_load_norm);
    lat_load_norm = lat_load_norm * (1.0 + k * abs_x - k * (abs_x * abs_x));
    // ------------------------------------------------
    
    // Smoothing: Map 0.0-1.0 slider to 0.1-0.0001s tau
    double smoothness = (double)m_sop_smoothing_factor;
```

#### 3. Add the GUI Slider in `src/GuiLayer_Common.cpp`
Find the "Rear Axle (Oversteer)" section where `m_lat_load_effect` is drawn, and add the new slider right below it:

```cpp
        FloatSetting("Lateral Load", &engine.m_lat_load_effect, 0.0f, 2.0f, FormatDecoupled(engine.m_lat_load_effect, FFBEngine::BASE_NM_SOP_LATERAL), Tooltips::LATERAL_LOAD);
        
        // NEW SLIDER
        FloatSetting("  Load Rolloff", &engine.m_lat_load_rolloff, 0.0f, 1.0f, "%.2f", 
            "Softens the FFB at maximum load transfer to prevent a 'notchy' feeling.\n0.0 = Sharp/Linear limit\n1.0 = Smooth/Rounded limit");
```

#### 4. Save it to the Config (`src/Config.cpp` & `src/Config.h`)
To ensure the user's preference is saved, update the `Preset` struct and `Config` parser.

In `src/Config.h` inside `struct Preset`:
```cpp
    float lateral_load = 0.0f; 
    float lat_load_rolloff = 1.0f; // Add this
```
In `Preset::Apply()`:
```cpp
    engine.m_lat_load_effect = (std::max)(0.0f, (std::min)(2.0f, lateral_load));
    engine.m_lat_load_rolloff = (std::max)(0.0f, (std::min)(1.0f, lat_load_rolloff)); // Add this
```
In `Preset::UpdateFromEngine()`:
```cpp
    lateral_load = engine.m_lat_load_effect;
    lat_load_rolloff = engine.m_lat_load_rolloff; // Add this
```

In `src/Config.cpp` inside `ParsePresetLine`:
```cpp
    else if (key == "lateral_load_effect") current_preset.lateral_load = (std::min)(2.0f, std::stof(value));
    else if (key == "lat_load_rolloff") current_preset.lat_load_rolloff = (std::min)(1.0f, std::max(0.0f, std::stof(value))); // Add this
```
In `src/Config.cpp` inside `WritePresetFields` and `Save`:
```cpp
    file << "lateral_load_effect=" << p.lateral_load << "\n";
    file << "lat_load_rolloff=" << p.lat_load_rolloff << "\n"; // Add this
```

### Why this is the perfect fix:
1. **It directly solves the bug report:** At `k=1.0`, the slope of the force curve drops exactly to `0` right as the car hits 100% load transfer. The "notch" is completely erased, replaced by a feeling of the tire progressively leaning onto its sidewall.
2. **It preserves the center feel:** Unlike standard cubic/quadratic formulas, this specific math guarantees that small steering inputs around the center remain 100% linear and unchanged.
3. **It's computationally free:** It uses just two multiplications and one `std::abs()`, which takes less than a nanosecond in your 400Hz loop.