# Include wheel angular velocity

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