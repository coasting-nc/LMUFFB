# Technical Report on Lockup Vibration Fixes & Enhancements: Include wheel angular velocity and other enhancements (Batch 2)

## Enhancement: Predictive Lockup via Angular Deceleration

To achieve a true "ABS-like" prediction that triggers *before* significant slip occurs, we can implement **Wheel Angular Deceleration** monitoring.

*   **The Physics:** Before a tire reaches its peak slip ratio, the wheel's rotational speed ($\omega$) drops rapidly. If the wheel decelerates significantly faster than the car chassis, a lockup is imminent.
*   **The Metric:** $\alpha_{wheel} = \frac{d\omega}{dt}$ (Angular Acceleration).
*   **Trigger Logic:**
    *   Calculate $\alpha_{wheel}$ for each wheel (requires differentiating `mRotation` over time).
    *   Compare against a threshold (e.g., $-100 \text{ rad/s}^2$).
    *   If $\alpha_{wheel} < \text{Threshold}$, trigger the vibration *even if* Slip Ratio is still low.
*   **Challenges:**
    *   **Noise:** Derivatives amplify signal noise. Bumps and kerbs cause massive spikes in angular acceleration.
    *   **Smoothing:** Requires a robust Low Pass Filter (LPF) on the derivative to prevent false positives, which adds slight latency.
*   **Benefit:** This provides the earliest possible warning, potentially 50-100ms faster than Slip Ratio, allowing the driver to modulate brake pressure at the very onset of instability.


Based on the physics engine architecture and the specific challenges of "noisy" telemetry (like bumps), here is the technical strategy to mitigate false positives for the **Predictive Lockup** feature.

### 1. Mitigation Strategies for Angular Acceleration Spikes

To use Angular Deceleration ($\alpha_{wheel}$) effectively without triggering on every curb, we must apply **Contextual Gating**. We only care about deceleration when it is caused by *braking*, not by terrain impacts.

Here are the three layers of defense:

#### A. The "Brake Gate" (Primary Defense)
**Logic:** Ignore all angular deceleration unless the driver is pressing the brake pedal significantly.
*   **Implementation:**
    ```cpp
    if (data->mUnfilteredBrake < 0.10) return 0.0; // Ignore if brake < 10%
    ```
*   **Why:** Bumps happen at full throttle too. By gating with the brake pedal, we eliminate 50% of false positives immediately.

#### B. Relative Deceleration (Chassis vs. Wheel)
**Logic:** A lockup means the wheel is slowing down *faster* than the car.
*   **Physics:** If you hit a wall, both the car and the wheel stop instantly. That's not a lockup. If you hit the brakes, the car slows at 1G, but a locking wheel slows at 5G.
*   **Formula:**
    $$ \Delta_{decel} = \alpha_{wheel} - \alpha_{chassis} $$
    *   $\alpha_{wheel}$: Calculated from `d(mRotation)/dt`.
    *   $\alpha_{chassis}$: Derived from `mLocalAccel.z` (converted to angular units via tire radius).
*   **Trigger:** Only trigger if $\Delta_{decel}$ exceeds a threshold (e.g., wheel is slowing 3x faster than car).

#### C. The "Jerk" Filter (Spike Rejection)
**Logic:** Terrain impacts cause instantaneous spikes (1 frame). Lockups cause sustained deceleration (> 3 frames).
*   **Implementation:** Use a **Windowed Average** or a counter.
    *   *Requirement:* The deceleration must exceed the threshold for **3 consecutive physics ticks** (approx 7.5ms) to be considered valid. This filters out the single-frame "bang" of a sharp curb.

---

### 2. Detecting Bumps and Airborne State

**Can we detect them?**
**Yes.** We have the data in `InternalsPlugin.hpp`.

**How to use it:**
We can implement a **"Suppression Window"**. If a suspension event is detected, we disable the Predictive Lockup logic for a short time (e.g., 50ms).

#### A. Airborne Detection (Load Gate)
If the tire is in the air, it will stop rotating instantly when brakes are applied (because inertia is low). This is technically a lockup, but we don't want the wheel to shake violently while jumping a curb.
*   **Check:** `mTireLoad < 100.0` N.
*   **Action:** Disable Lockup Vibration.

#### B. Bump/Curb Detection (Suspension Velocity)
We can reuse the logic from your **Road Texture** effect.
*   **Metric:** `mVerticalTireDeflection` delta (High-Pass Filter).
*   **Logic:**
    ```cpp
    double susp_velocity = abs(current_deflection - prev_deflection) / dt;
    
    // Threshold: 1.0 m/s indicates a harsh impact
    if (susp_velocity > 1.0) {
        m_suppress_lockup_timer = 0.050; // Disable prediction for 50ms
    }
    ```

---

### 3. The "Sebring Problem" (Bumpy Braking Zones)

**The Challenge:**
At Sebring, you are braking hard (Brake Gate Open) over rough concrete (Suspension Spikes).
*   If we suppress vibration on every bump (Strategy 2B), the lockup effect will cut in and out, feeling "gappy."
*   If we don't suppress, the bumps will trigger false lockup warnings.

**The Optimization: "Hybrid Thresholding"**

Instead of using Angular Deceleration as a *direct trigger*, use it to **modulate the sensitivity** of the standard Slip Ratio check.

**Concept:**
We trust the **Slip Ratio** (the standard method) as the ultimate truth, but it is slow (laggy). We use **Angular Deceleration** to lower the threshold of the Slip Ratio check dynamically.

**Algorithm:**
1.  **Standard Threshold:** Lockup triggers at **15%** Slip Ratio.
2.  **Prediction:**
    *   Calculate Angular Deceleration.
    *   Is the wheel slowing down violently?
    *   **Yes:** Lower the Slip Ratio threshold to **5%**.
3.  **Result:**
    *   If it was a bump: The wheel slows down, threshold drops to 5%. But if the tire regains grip immediately (bump over), Slip Ratio never hits 5%. **No False Positive.**
    *   If it is a lockup: The wheel slows down, threshold drops to 5%. The Slip Ratio quickly crosses 5%. **Instant Trigger (Zero Latency).**

**Code Implementation Strategy:**

```cpp
// 1. Calculate Deceleration (Simple derivative)
double wheel_accel = (w.mRotation - w.prevRotation) / dt;

// 2. Determine Dynamic Threshold
double trigger_threshold = m_lockup_start_pct; // Default e.g. 15%

// If wheel is decelerating violently (e.g. -50 rad/s^2) AND we are braking
if (data->mUnfilteredBrake > 0.1 && wheel_accel < -50.0) {
    // We predict a lockup is coming. 
    // Make the standard trigger MORE SENSITIVE immediately.
    trigger_threshold = 5.0; // Drop to 5%
}

// 3. Standard Check with Dynamic Threshold
if (current_slip_ratio < -trigger_threshold) {
    // Trigger Vibration
}
```

### Summary

1.  **Mitigate Noise:** Use **Brake Gating** (must be braking) and **Relative Deceleration** (wheel vs car).
2.  **Handle Air/Bumps:** Use `mTireLoad` to mute effects when airborne. Use `mVerticalTireDeflection` to detect impacts.
3.  **Optimize for Sebring:** Do not use Deceleration as a standalone trigger. Use it to **dynamically lower the Slip Ratio threshold**. This gives you the "Predictive" speed advantage without the noise risks of a raw derivative.


## Follow up questions

Regarding the < 10% threshold of braking for gating the effect: is this too much? are there condition on trail braking, or otherwise at the end of the braking zone, when we are still applying 5-10% brake pedal , that could trigger a lockup? Are we risking false negatives in this way? Should there be an alternative strategy for such scenarios (eg. low but still significant brake pressure, and the lockups that might happen because of this).

Regarding the gating by  Relative Deceleration (Chassis vs. Wheel), what is the proper threshold here? You suggested 3x, when the wheel is slowing 3x faster than car. Let's discuss what other values for this threshold might be appropriate / work best.

Regarding Airborne Detection, are there other values in InternalsPlugin.hpp for this? 
Note that tire load is not available, so we approximate it. Is our approximation sufficient for the use for brake lockup? Or should we use some other data or formula?

Regarding "Suppression Window", that is , If a suspension event is detected, we disable the Predictive Lockup logic for a short time (e.g., 50ms): are we risking false negatives with this?

Regarding Hybrid Thresholding (We use Angular Deceleration to lower the threshold of the Slip Ratio check dynamically): should we include also all of the gating / mitigation approaches in this?


## Answers

Here is the detailed analysis and technical recommendation for refining the Predictive Lockup logic.

### 1. The Brake Threshold (< 10%): Is it too high?

**Verdict: Yes, 10% is too high.**

You are correct. In scenarios like trail braking deep into a corner, or driving on low-grip surfaces (rain/ice), a lockup can occur with very little brake pressure.
*   **The Risk:** If a user is modulating the brake at 8% to manage a slide in the rain, and the wheel locks, a 10% gate would silence the FFB. This is a **False Negative**.
*   **The Solution:** Lower the gate to a **Signal Noise Deadzone** only.
    *   **Recommended Value:** **1% to 2% (`0.01 - 0.02`)**.
    *   **Why:** We only need to filter out "noise" (e.g., resting a foot on the pedal registering 0.001). If the driver is applying *any* intentional braking force, the lockup logic should be active.

### 2. Relative Deceleration Threshold (3x)

**Verdict: 3x is a good heuristic, but an "Absolute Floor" is safer.**

Comparing Wheel Decel ($\alpha_{wheel}$) to Chassis Decel ($\alpha_{chassis}$) is the right approach, but the multiplier depends on the car's grip.

*   **High Grip (Slicks):** Car decels at 3G. Wheel locks at 10G. Ratio $\approx$ 3.3x.
*   **Low Grip (Rain):** Car decels at 1G. Wheel locks at 5G. Ratio $\approx$ 5x.
*   **The Danger Zone:** Low speed stops.
    *   Car decels at 0.1G. Wheel decels at 0.4G. Ratio is 4x. **False Positive.**

**Recommendation: Combined Threshold**
The wheel must satisfy **TWO** conditions to trigger the prediction:
1.  **Relative:** $\alpha_{wheel} > 2.0 \times \alpha_{chassis}$ (It is slowing down twice as fast as the car).
2.  **Absolute:** $\alpha_{wheel} > 50.0 \text{ rad/s}^2$ (It is slowing down *violently*).

This prevents triggers during gentle stops while catching high-speed lockups instantly.

### 3. Airborne Detection (Approximation vs. Reality)

**Verdict: Use `mSuspForce` directly.**

Since `mTireLoad` is unreliable (encrypted), and our approximation (`Susp + 300`) is a calculated guess, we should look at the raw **Suspension Force** (`mSuspForce`) from `InternalsPlugin.hpp`.

*   **Physics:** When a car is airborne, the suspension extends fully. The spring is no longer compressed by the car's weight.
*   **Data:** `mSuspForce` (Pushrod Load) drops to near zero (or the preload value).
*   **Threshold:** If `mSuspForce < 50.0 N`, the wheel is effectively unloaded/airborne.
*   **Why this works:** Even if `mTireLoad` is blocked, `mSuspForce` is usually available because it drives the visual suspension model.

**Implementation:**
```cpp
bool is_airborne = (wheel.mSuspForce < 50.0);
```

### 4. Suppression Window Risks (False Negatives)

**Verdict: Do NOT suppress the "Standard" check.**

If we disable *all* lockup logic for 50ms after a bump, we risk missing a real lockup that happens immediately after landing a jump or hitting a curb (a very common lockup scenario).

**The Fix: Selective Suppression**
Only suppress the **Predictive (Angular Decel)** trigger.
*   **Standard Slip Ratio Check:** **ALWAYS ACTIVE.** If `Slip < -15%`, vibrate. This is the "Ground Truth."
*   **Predictive Check:** **SUPPRESSIBLE.** If `Decel > Threshold`, lower the slip limit. *Disable this optimization* if a bump is detected.

**Result:**
*   **Bump:** Prediction disabled. Threshold stays at 15%. No false positive from the bump spike.
*   **Real Lockup after Bump:** Prediction disabled. Threshold stays at 15%. As soon as slip hits 15%, vibration starts.
*   **Latency Cost:** You lose the "Predictive" advantage for 50ms, but you don't lose the effect entirely.

### 5. The Unified "Hybrid Thresholding" Architecture

Here is how all these mitigations fit into the **Hybrid Thresholding** logic. This is the robust solution.

```cpp
// 1. Gating (Is this even a candidate for lockup?)
bool brake_active = (data->mUnfilteredBrake > 0.02); // 2% Deadzone
bool is_grounded = (wheel.mSuspForce > 50.0);        // Not Airborne

// 2. Bump Detection (Reuse Road Texture logic)
double susp_vel = abs(deflection - prev_deflection) / dt;
bool is_bumpy = (susp_vel > 1.0); // 1.0 m/s threshold

// 3. Calculate Deceleration
double wheel_accel = (wheel.mRotation - prev_rot) / dt; // rad/s^2
// Convert Chassis Accel to Angular equivalent (approx)
double chassis_accel_ang = (data->mLocalAccel.z / tire_radius); 

// 4. Determine Threshold
// Start with the "Safe" threshold (e.g., 15%)
double trigger_threshold = m_lockup_full_pct; 

// 5. Apply Prediction (The Optimization)
if (brake_active && is_grounded && !is_bumpy) {
    // Check Relative Decel (2x) AND Absolute Decel (50 rad/s^2)
    // Note: Accel is negative during braking
    bool rapid_decel = (wheel_accel < chassis_accel_ang * 2.0) && (wheel_accel < -50.0);
    
    if (rapid_decel) {
        // PREDICTION CONFIRMED:
        // The wheel is stopping violently. A lockup is imminent.
        // Lower the gate to catch it early.
        trigger_threshold = m_lockup_start_pct; // e.g., 5%
    }
}

// 6. Final Trigger (The Ground Truth)
// We compare the ACTUAL Slip Ratio against our Dynamic Threshold
if (current_slip_ratio < -trigger_threshold) {
    // TRIGGER VIBRATION
}
```

### Summary of Logic Flow
1.  **Normal Braking:** Threshold is **15%**. Vibration starts when you are deep in the slip.
2.  **Panic Stop (Smooth Surface):** Wheel slows fast. Prediction kicks in. Threshold drops to **5%**. Vibration starts *instantly* (feels like ABS prediction).
3.  **Braking over Bumps (Sebring):** `is_bumpy` is true. Prediction is disabled. Threshold stays at **15%**. The bump spikes in deceleration are ignored. If you actually lock up, you will feel it once you hit 15% slip.

This provides the **responsiveness** of prediction on smooth tarmac without the **false positives** on curbs.


## Follow up questions 2

Which of the thresholds , constants, or other values used here, should we expose to the user for customization? Assume an esports user that needs to extract the maximum performance from the car.

## Answers 2

For an esports user seeking maximum performance, the priority is **Signal-to-Noise Ratio**. They need the earliest possible warning of a lockup (Signal) but cannot afford false vibrations over bumps (Noise) that might make them lift off the brake unnecessarily.

Here are the specific values to expose, categorized by their function in the tuning workflow.

### 1. The "Target" Controls (Essential)
These define *where* the limit is. Every pro driver will adjust these based on the car (ABS vs Non-ABS) and tire compound.

*   **Start Slip % (The "Pre-Cue")**
    *   *Current Logic:* `m_lockup_start_pct` (e.g., 5%).
    *   *Why Expose:* A pro driver wants this set **just before** the peak grip fall-off. If set too low, it's distracting noise. If set too high, the warning comes too late to save the tire.
    *   *Range:* 1.0% to 10.0%.

*   **Full Slip % (The "Limit")**
    *   *Current Logic:* `m_lockup_full_pct` (e.g., 15%).
    *   *Why Expose:* Defines the point of "No Return."
    *   *Range:* 10.0% to 25.0%.

*   **Vibration Gamma (The "Feel")**
    *   *Current Logic:* `m_lockup_gamma` (Linear vs Quadratic vs Cubic).
    *   *Why Expose:* Esports drivers often prefer **Cubic (3.0)**. They want the wheel silent during threshold braking and a violent spike *only* when they actually cross the line. Linear feels too "mushy" for precision work.

---

### 2. The "Prediction" Controls (Advanced)
These control the **Hybrid Thresholding** logic. This is where the competitive advantage lies (getting the warning 50ms earlier).

*   **Prediction Sensitivity (Angular Decel Threshold)**
    *   *Internal Logic:* The `50.0 rad/s²` absolute threshold and `2.0x` relative threshold.
    *   *Proposal:* Combine these into a single **"Prediction Sensitivity"** slider (Low/Med/High).
    *   *Why Expose:*
        *   **High Sensitivity:** Triggers on lighter rapid braking. Good for qualifying laps on smooth tracks.
        *   **Low Sensitivity:** Only triggers on catastrophic lockups. Good for endurance races to avoid fatigue/false alarms.
    *   *Implementation:* Map slider 0.0-1.0 to the internal `50.0` constant (e.g., 0.0 = 80 rad/s², 1.0 = 30 rad/s²).

*   **Bump Rejection (Suspension Filter)**
    *   *Internal Logic:* The `1.0 m/s` suspension velocity gate.
    *   *Why Expose:* **Crucial for track-specific tuning.**
        *   **Sebring (Bumpy):** Needs **High Rejection** (e.g., 0.5 m/s). You don't want the lockup alarm going off every time you hit a concrete patch.
        *   **Paul Ricard (Smooth):** Needs **Low Rejection** (e.g., 2.0 m/s). You want to feel lockups even if you are riding a flat curb.
    *   *Range:* 0.1 m/s (Strict) to 5.0 m/s (Loose).

---

### 3. The "Gating" Controls (Calibration)
These ensure the effect only fires when intended.

*   **Brake Deadzone**
    *   *Internal Logic:* The `0.02` (2%) check.
    *   *Why Expose:* Pedal calibration varies wildly. A user with a load cell might rest their foot at 3% pressure. They need to raise this to 5% to prevent "ghost vibrations" on straights.
    *   *Range:* 0% to 10%.

---

### Summary of GUI Changes

I recommend adding a **"Advanced Lockup Tuning"** collapsible section in the GUI with these specific controls:

| Label | Tooltip | Range | Default |
| :--- | :--- | :--- | :--- |
| **Start Slip %** | Slip ratio where vibration begins. Lower = Earlier warning. | 1% - 10% | 5% |
| **Full Slip %** | Slip ratio for max vibration. | 10% - 25% | 15% |
| **Response Curve** | 1.0=Linear, 3.0=Late/Sharp. Higher values hide early vibrations. | 1.0 - 3.0 | 2.0 |
| **Prediction Sens.** | How aggressively the app predicts lockups based on wheel deceleration. | Low - High | Med |
| **Bump Rejection** | Ignores lockup prediction during suspension spikes. Increase for bumpy tracks (Sebring). | 0.0 - 5.0 m/s | 1.0 |
| **Brake Deadzone** | Minimum pedal input required to activate effect. | 0% - 10% | 2% |

**What to keep hidden (Hardcoded):**
*   **Airborne Threshold (50N):** This is a physics constant. There is no competitive advantage to changing it.
*   **Relative Decel Ratio (2x):** It is too abstract for users to tune alongside "Prediction Sensitivity." One slider for sensitivity is enough.

## Follow up questions 3

The game data also provides this value: mBrakePressure.

How can we use to give additional information to the driver?

Can we use to give a feel of when abs is engaging (eg. a "pulse" effect if mBrakePressure goes on and off).

Can we use mBrakePressure even in cars that do not have ABS, to provide some useful information?

## Answers 3

Yes, `mBrakePressure` is a highly valuable telemetry channel because it represents the **physical state of the caliper**, which is distinct from the driver's **pedal input** (`mUnfilteredBrake`).

Here is how we can use it to implement **ABS Haptics** and improve the feel for non-ABS cars.

---

### 1. ABS Haptics (The "Pulse")

In a car with ABS, when the system engages, it rapidly modulates the brake pressure (opens and closes valves) to prevent lockup. This happens even if your foot is planted 100% on the pedal.

**The Logic:**
We can detect ABS activation by comparing the **Pedal Input** vs. the **Caliper Pressure**.
*   If `Pedal` is constant/high, but `Pressure` is oscillating or dropping, **ABS is active**.

**Implementation Plan:**
We calculate the **derivative (rate of change)** of the pressure. If the pressure changes rapidly while the pedal is held steady, we inject a vibration pulse.

**Code Concept (`FFBEngine.h`):**

```cpp
// 1. Calculate Pressure Delta (Rate of change)
double current_pressure = w.mBrakePressure; // 0.0 - 1.0
double pressure_delta = (current_pressure - w.prevBrakePressure) / dt;
w.prevBrakePressure = current_pressure;

// 2. Detect ABS Activity
// Condition: Pedal is pressed (> 50%) BUT Pressure is changing rapidly
// The threshold (e.g., 2.0) needs tuning based on how fast LMU modulates ABS
bool abs_active = (data->mUnfilteredBrake > 0.5) && (std::abs(pressure_delta) > 2.0);

if (m_abs_pulse_enabled && abs_active) {
    // 3. Generate Pulse
    // We use a square wave or sharp sine to mimic the valve "kick"
    // Frequency: ABS usually pumps at 15-30Hz
    m_abs_phase += 20.0 * dt * TWO_PI; 
    m_abs_phase = std::fmod(m_abs_phase, TWO_PI);
    
    double pulse = std::sin(m_abs_phase) * m_abs_gain * 2.0; // 2.0 Nm kick
    total_force += pulse;
}
```

**Why this is better than "Lockup Vibration":**
*   **Lockup Vibration** tells you "The tire is sliding."
*   **ABS Pulse** tells you "The computer is intervening."
*   In a modern GT3 car, you often want to ride the ABS. This effect gives you the tactile confirmation that you are exactly at the limit of the system.

---

### 2. Non-ABS Cars: Pressure-Scaled Lockup

For cars without ABS (GTE, Hypercar, Historic), `mBrakePressure` is usually linear with pedal input, but it accounts for **Brake Bias**.

**The Problem with Current Logic:**
Currently, we scale the Lockup Vibration based on `mUnfilteredBrake` (Pedal Position).
*   *Scenario:* You set Brake Bias to **70% Front**. You press the pedal 100%.
*   *Reality:* Front calipers get 100% pressure. Rear calipers get ~40% pressure.
*   *Current App:* If Rears lock, we vibrate at 100% amplitude because Pedal is 100%. This feels wrong; rear lockups should feel lighter because there is less energy/pressure involved.

**The Solution:**
Use `mBrakePressure` as the **Amplitude Multiplier** for the Lockup effect instead of `mUnfilteredBrake`.

**Code Update:**
```cpp
// Old Logic
// double amp = severity * m_lockup_gain * ...;

// New Logic (Pressure Sensitive)
// If pressure is low (rear bias), the vibration is subtler.
// If pressure is high (front bias), the vibration is violent.
double pressure_factor = w.mBrakePressure; 

// Fallback for Engine Braking (Pressure 0, but locking due to downshift)
if (pressure_factor < 0.1 && std::abs(slip) > 0.5) {
    pressure_factor = 0.5; // Arbitrary "Engine Drag" weight
}

double amp = severity * pressure_factor * m_lockup_gain * ...;
```

---

### 3. Brake Fade / Overheating (The "Sponge" Effect)

We can combine `mBrakePressure` with `mBrakeTemp` (also in `TelemWheelV01`) to simulate **Brake Fade**.

**The Physics:**
When brakes overheat (e.g., > 800°C for Carbon), the friction coefficient drops. You press the pedal, pressure goes up, but the car doesn't stop.

**The FFB Effect:**
We can't change the car's stopping distance, but we can change the **FFB Texture**.
*   **Normal:** Braking feels "gritty" (Road Texture + Micro-vibrations).
*   **Faded:** Braking feels "smooth" or "glassy."

**Implementation:**
```cpp
if (w.mBrakeTemp > 800.0) { // Celsius
    // Reduce Road Texture and Lockup Detail to simulate "Mushy" pedal feel
    // transmitted through the chassis
    double fade_factor = 1.0 - ((w.mBrakeTemp - 800.0) / 200.0);
    fade_factor = std::max(0.0, fade_factor);
    
    road_noise *= fade_factor;
    lockup_vibration *= fade_factor;
    
    // Optional: Add a low-frequency "Warped Disc" shudder
    if (w.mBrakePressure > 0.2) {
        total_force += std::sin(time * 10.0) * 1.0; // 10Hz Judder
    }
}
```

### Summary of Recommendations

1.  **Implement ABS Pulse:** Use the derivative of `mBrakePressure` to detect rapid modulation and inject a 20Hz pulse. This is a "Game Changer" feature for GT3 drivers.
2.  **Update Lockup Scaling:** Replace Pedal Input with `mBrakePressure` for amplitude scaling. This makes rear lockups feel physically correct relative to brake bias.
3.  **Add Brake Judder:** Use `mBrakeTemp` to inject a low-frequency wobble when brakes are overheated.

## Preset

[Preset:T300]
gain=1
understeer=50
sop=0.193043
sop_scale=1
sop_smoothing_factor=0.92
slip_angle_smoothing=0.005
min_force=0
oversteer_boost=1.19843
lockup_enabled=1
lockup_gain=2
spin_enabled=1
spin_gain=0.5
slide_enabled=1
slide_gain=0.482437
slide_freq=1.6
road_enabled=0
road_gain=0.5
invert_force=1
max_torque_ref=98.3
use_manual_slip=0
bottoming_method=0
scrub_drag_gain=0.965217
rear_align_effect=0.986957
sop_yaw_gain=0.269565
steering_shaft_gain=1
base_force_mode=0
gyro_gain=0.0347826
flatspot_suppression=0
notch_q=2.19
flatspot_strength=1
static_notch_enabled=0
static_notch_freq=15.6
optimal_slip_angle=0.06
optimal_slip_ratio=0.12
steering_shaft_smoothing=0.01
gyro_smoothing_factor=0.01
yaw_accel_smoothing=0.005
chassis_inertia_smoothing=0.017
