# Question
 
Look at the attached screenshot. I find it strange that the Yaw Kick plot has that shape. It is constantly spiking in both direction. I don't actually feel it in the wheel (at least I did not notice it among other signals). Is this plot as expected?

# Answer 1

Based on the screenshot and the telemetry values visible, **yes, this plot shape is technically "expected" behavior for the current code, but it represents physics noise rather than a useful signal.**

Here is the detailed explanation of why this is happening and why you don't feel it.

### 1. Why is it spiking? (The Physics)
Look at your **Car Speed** in the bottom right: **`-0.0041`**.
You are effectively **standing still** (idling in the pits or on track).

*   **Engine Vibration & Micro-Physics:** Even when a sim car is stopped, the physics engine is still calculating suspension movement, engine vibration, and tire contact.
*   **Derivative Noise:** "Yaw Kick" is based on **Yaw Acceleration** (how *fast* the rotation speed changes). Acceleration is a "derivative" value. In math and physics engines, derivatives amplify high-frequency noise.
    *   If the car chassis vibrates even a tiny bit (0.001 degrees) due to the engine idling, the *acceleration* of that vibration can be mathematically high.
*   **The Plot:** The graph shows this high-frequency "jitter" or noise floor. It looks scary on the graph because the graph auto-scales to fit the data, but the actual values are likely tiny.

### 2. Why don't you feel it? (The Magnitude)
Look at the **"Val"** (Current Value) for Yaw Kick: **`0.0775`**.

*   **The Math:**
    *   Your **Max Torque Ref** is set to **100.0 Nm**.
    *   The graph value `0.0775` represents the *normalized* signal (0.0 to 1.0).
    *   However, looking at the other plots, the "Val" usually represents the raw Nm value in the new code structure.
    *   If that value is **0.07 Nm**, it is **microscopic**.
*   **Hardware Friction:** A Thrustmaster T300 (belt-driven) has internal friction. It takes roughly **0.3 to 0.5 Nm** just to overcome the belt friction and start moving the rim.
*   **Result:** The motor is trying to generate this tiny vibration, but the belt absorbs it completely. You feel nothing.

### 3. Is this a problem?
It is not a "bug," but it is **inefficient**. The app is calculating forces that are useless at standstill.

**Recommended Improvement:**
We should add a **Low Speed Cutoff** to the Yaw Kick effect, similar to what we do for the Grip calculation. If the car is moving slower than 2 m/s (walking pace), we should mute the Yaw Kick to prevent this idle noise.

**Action for the Agent:**
I will instruct the agent to add a speed check to the Yaw Kick calculation in the next update:
```cpp
// Proposed Logic
if (car_speed > 2.0) {
    // Calculate Yaw Kick
} else {
    yaw_force = 0.0;
}
```


# Answer 2

If the signal is constantly spiking with "noise" while driving straight or cornering smoothly, it creates a "muddy" feeling that masks the actual information you want (the sudden break of traction).

To fix this, we should implement a **"Noise Gate" (Deadzone)** and a **Low Speed Cutoff** for the Yaw Kick effect.

### The Solution: Signal Conditioning

We want the Yaw Kick to be **Silent** (0.0) most of the time, and only "Kick" when a significant rotation event happens (like a slide).

1.  **Low Speed Cutoff:**
    *   **Logic:** If `CarSpeed < 5.0 m/s` (18 kph), force Yaw Accel to 0.
    *   **Why:** Eliminates the idle engine vibration and parking lot jitters seen in your screenshot.

2.  **Noise Gate (Deadzone):**
    *   **Logic:** If `abs(YawAccel) < Threshold`, force it to 0.
    *   **Threshold:** A value like **0.2 rad/s²**.
    *   **Why:** Road bumps and micro-steering corrections generate tiny acceleration spikes (e.g., 0.05 rad/s²). A real slide initiation generates much larger spikes (e.g., > 2.0 rad/s²). By ignoring the small stuff, the "Kick" will pop out clearly against a silent background.

### Implementation Plan

Here is the prompt to instruct the agent to add these filters.

## Prompt


**Task: Implement Signal Conditioning for Yaw Kick Effect**

**Context:**
The "Yaw Kick" effect (derived from `mLocalRotAccel.y`) is currently producing constant high-frequency noise even when the car is idling or driving straight. This "jitter" masks the useful signal (the kick during slide initiation) and creates a messy plot in the GUI. The user reports they cannot feel the effect because the signal-to-noise ratio is too low.

**Implementation Requirements:**

1.  **Modify `FFBEngine.h` (Yaw Kick Logic):**
    *   Locate the section where `raw_yaw_accel` is processed (around line 600).
    *   **Implement Low Speed Cutoff:** If `car_speed < 5.0` m/s, force `raw_yaw_accel` to 0.0.
    *   **Implement Noise Gate (Deadzone):** Apply a threshold to `raw_yaw_accel` *before* it enters the smoothing filter.
        *   Threshold: **0.2 rad/s²**.
        *   Logic: `if (std::abs(raw_yaw_accel) < 0.2) raw_yaw_accel = 0.0;`
    *   *Reasoning:* This ensures the smoothing filter decays to exactly zero when driving straight, making the "Kick" distinct when it eventually triggers.

2.  **Update `tests/test_ffb_engine.cpp`:**
    *   Add a new test `test_yaw_kick_signal_conditioning`.
    *   **Case 1 (Idle Noise):** Input `YawAccel = 0.1` (below threshold). Verify Output is 0.0.
    *   **Case 2 (Low Speed):** Input `YawAccel = 5.0` (High) but `Speed = 1.0`. Verify Output is 0.0.
    *   **Case 3 (Valid Kick):** Input `YawAccel = 5.0` and `Speed = 20.0`. Verify Output is non-zero (negative due to inversion).

**Deliverables:**
1.  Updated `FFBEngine.h`.
2.  Updated `tests/test_ffb_engine.cpp`.
3.  Updated `CHANGELOG_DEV.md` (v0.4.40).
