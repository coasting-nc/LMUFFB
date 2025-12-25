## Question 1

Here is the full codebase and documentation of a  force feedback (FFB) app. Your task is to investigate an issue. 

An user tested the app and reported that there was a constant vibration that he want to eliminate or mask. This vibration might be coming from the Steering Shaft Torque given by the game Le Mans Ultimate (LMU). This vibration might be due to tyre flat-spots. The user identified the frequency of the vibration as being between 10 and 60 Hz.

The user disabled many settings / sliders in our LMUFFB app to determine what was causing the vibration, and it seems it was caused by the Steering Shaft Torque signal itself, and not by any effect our app produces.

The user tried to mask the vibration by various means (eg. FFB signal equalizer in the Moza device driver, or adding smoothing), but he said that the things he tried also masked other useful details from the force feedback in addition to the vibration. Therefore, we want to find out if we can improve the LMUFFB app to more effectively mask out or eliminate this vibration without reducing, compromising or affecting any other detail of the force feedback.

Your task is to investigate the following:

* verify whether  we might still be causing some vibration from our app, even when we have only the Steering Torque Shaft force enabled and everything else disabled.
* if we are not causing this vibration, and the game signal itself is the cause, determine ways in which we can "mask" or eliminate this vibration. What are the possible solutions for this?

	* Can we add a "counter" frequency signal that cancels out that vibration? This is assuming that that vibration is at a constant frequency.

	* Can we add some tools to "troubleshoot" and identify the exact frequency of this vibration in the steering shaft torque? Like some spectrum analyzer plot, or something else?

	* Can we add some form of damping or smoothing only to the steering shaft torque? Can we do this without adding any significant latency (that is, less than 15 milliseconds)?

	* Are there other recommended solutions?

----

## Answer 1

Based on the codebase and the user's description, here is the investigation report and the recommended technical solutions.

### 1. Verification: Is the App Causing the Vibration?

**Analysis:**
I have reviewed `FFBEngine.h` and `calculate_force`.
If the user has disabled all effects (SoP, Textures, etc.), the app acts as a pass-through for `mSteeringShaftTorque`. However, there is **one setting** in our app that can artificially amplify noise into a constant vibration: **`m_min_force`**.

*   **The Mechanism:**
    ```cpp
    // FFBEngine.h ~line 830
    if (std::abs(norm_force) > 0.0001 && std::abs(norm_force) < m_min_force) {
        double sign = (norm_force > 0.0) ? 1.0 : -1.0;
        norm_force = sign * m_min_force;
    }
    ```
*   **The Risk:** If the game signal is noisy around zero (e.g., oscillating between -0.001 and +0.001 Nm due to the flat spot), and `m_min_force` is set to e.g., 0.05 (5%), the app will amplify this tiny noise into a **Square Wave** of +/- 5% force. This feels like a harsh, constant rattle.

**Verification Step:**
Ask the user to ensure **Min Force is set to 0.0**. If the vibration persists with Min Force at 0, then the app is innocent, and we must filter the game signal.

---

### 2. Solution: The "Surgical" Fix (Dynamic Notch Filter)

The user identified the frequency as **10-60 Hz**. This range perfectly matches the rotational frequency of a race car tire:
*   **100 km/h** $\approx$ 14 Hz
*   **300 km/h** $\approx$ 42 Hz

This confirms the vibration is a **Flat Spot** or **Polygon/Mesh** issue linked to wheel speed.
Standard smoothing (Low Pass Filter) fails because to kill 14 Hz, you need a heavy filter that destroys steering latency.

**The Solution:** Implement a **Speed-Tracking Notch Filter**.
A Notch Filter removes *only* a specific frequency band. By linking the center frequency to the car's speed, we can surgically remove the "flat spot" thumping frequency while leaving the rest of the FFB (road texture, curb strikes, weight transfer) completely untouched.

#### Implementation Plan

**A. Add Biquad Filter Class**
Add this struct to `FFBEngine.h`. It implements a standard Biquad filter that can be updated in real-time.

```cpp
// Add to FFBEngine.h

struct BiquadNotch {
    double b0, b1, b2, a0, a1, a2;
    double x1 = 0, x2 = 0, y1 = 0, y2 = 0; // State history

    void Update(double center_freq, double sample_rate, double Q) {
        // Prevent explosion at low freq
        if (center_freq < 1.0) center_freq = 1.0;
        
        double omega = 2.0 * 3.14159265358979323846 * center_freq / sample_rate;
        double sn = std::sin(omega);
        double cs = std::cos(omega);
        double alpha = sn / (2.0 * Q);

        b0 = 1.0;
        b1 = -2.0 * cs;
        b2 = 1.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cs;
        a2 = 1.0 - alpha;

        // Normalize
        b0 /= a0; b1 /= a0; b2 /= a0;
        a1 /= a0; a2 /= a0;
    }

    double Process(double in) {
        double out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = in;
        y2 = y1; y1 = out;
        return out;
    }
};
```

**B. Update `FFBEngine` Class**
Add the filter instance and a new setting.

```cpp
class FFBEngine {
public:
    // ... existing settings ...
    bool m_flatspot_suppression = false; // New Toggle
    BiquadNotch m_notch_filter;
    
    // ... inside calculate_force ...
    
    // 1. Calculate Wheel Frequency
    // F = Velocity / Circumference
    double car_speed = std::abs(data->mLocalVel.z);
    
    // Get radius (convert cm to m)
    double radius = (double)fl.mStaticUndeflectedRadius / 100.0;
    if (radius < 0.1) radius = 0.33; // Safety fallback
    
    double circumference = 2.0 * 3.14159 * radius;
    double wheel_freq = car_speed / circumference;

    // 2. Apply Filter to Base Force
    double game_force = data->mSteeringShaftTorque;

    if (m_flatspot_suppression && wheel_freq > 1.0) {
        // Q factor determines width. 2.0 is a good balance.
        // 400.0 is our physics loop rate
        m_notch_filter.Update(wheel_freq, 400.0, 2.0);
        game_force = m_notch_filter.Process(game_force);
    }
    
    // ... continue with game_force ...
```

**Why this works:**
*   **Zero Latency:** Outside the target frequency (e.g., steering inputs at 1-2Hz), the filter has near-zero phase shift.
*   **Precision:** It tracks the car speed perfectly. As the user accelerates, the filter moves up the spectrum, constantly deleting the vibration.

---

### 3. Solution: Troubleshooting Tool (Spectrum Analyzer)

To help the user confirm the frequency, we can add a simple **Frequency Estimator** to the Debug Window.

**Implementation in `FFBEngine.h`:**
We don't need a full FFT. We can just measure the time between "Zero Crossings" of the AC component of the torque.

```cpp
// Add to FFBEngine class
double m_debug_freq = 0.0;
double m_last_crossing_time = 0.0;
double m_torque_ac_accum = 0.0; // High-pass filter state

// Inside calculate_force
// 1. Isolate AC component (Vibration) using simple High Pass
double ac_torque = game_force - m_torque_smoothed; // Assuming you have a smoothed var
// 2. Detect Zero Crossing
if ((m_prev_ac_torque > 0 && ac_torque <= 0) || (m_prev_ac_torque < 0 && ac_torque >= 0)) {
    double now = data->mElapsedTime;
    double period = now - m_last_crossing_time;
    m_last_crossing_time = now;
    if (period > 0.001) {
        m_debug_freq = 1.0 / period; // Hz
    }
}
m_prev_ac_torque = ac_torque;
```

**GUI Update:**
Display `m_debug_freq` in the "Raw Telemetry" window. If it matches the `Wheel Frequency` calculated above, the Notch Filter is guaranteed to work.

---

### 4. Alternative: Slew Rate Limiter (The "Damping" Approach)

If the Notch Filter is too complex or the vibration is irregular, a **Slew Rate Limiter** is the best "Masking" technique that preserves latency better than smoothing.

*   **Concept:** Limit the maximum change in torque per millisecond.
*   **Effect:** It "chops off" the sharp spikes of the vibration without delaying the slower steering movements.

```cpp
// Inside calculate_force
double delta = game_force - m_prev_game_force;
double max_delta = 50.0 * dt; // Allow max 50 Nm/sec change (Tunable)

if (delta > max_delta) game_force = m_prev_game_force + max_delta;
if (delta < -max_delta) game_force = m_prev_game_force - max_delta;

m_prev_game_force = game_force;
```

### Recommendation Summary

1.  **Immediate Check:** Verify `Min Force` is 0.0.
2.  **Best Solution:** Implement the **Speed-Tracking Notch Filter**. It is the only way to eliminate a variable-frequency vibration without compromising FFB detail or latency.
3.  **Backup Solution:** Implement the **Slew Rate Limiter** as a "Softening" slider.
