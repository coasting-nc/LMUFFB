# Question 1

Here is the full codebase of a force feedback (FFB) app. Your task is to help me investigate an issue.
The issue is that when driving the LMP2 car, and using a car setup with stiff dampers, strange pulls in the FFB happen, which seems coming from the Yaw Kick effect (from yaw acceleration).
I need to investigate why this is happening. Is there an issue in our formula for yaw kick, or there might be an issue with the game physics when the dampers are stiff?
Can we already identify if something is wrong in our code in relation for this issue?
In case the code looks ok, can you propose a series of diagnostics / investigations I could do to investigate the issue?
For instance, I could look at the telemetry of the car physics, to inspect all that is happening in the car physics-wise (I would use Motec after exporting the telemetry); could you suggest which plot of the Motec telemetry I could look at to investigate the issue?
Could you suggest particular setup changes that I could try on the LMP2 car to further investigate the issue? For instance, variations of the setup that causes the issue (stiff damper), setups that do not cause is (soft dampers), and combinations of other setting that might help in more precisely identify when the issue is triggered.

Note that my FFB app also can exports some csv logs of the telemetry. However, currently some data might not be exported. And we also have a python log analyser that analyses the csv logs, calculates stats and produces plots.
Would you suggest exporting additional data channels to csv to investigate this issue, and to perform particular analysis on those (stats, plots, etc.)?

If it is of any help, note that the pulls are "constant" (not vibrations or frequent oscillations), like forces pulling strong in one direction that seems off, in part similar to when the car has suspension damage. It also has some similarity to the feeling when FFB is inverted (wrong minus sign on the whole formula).

A recent patch tried to fix the issue by smoothing the yaw acceleration before applying the threshold, and introducing Continuous Deadzone (see notes changelog below). 

However this did not seem to fix the issue. Here is what a user reported after the last patch:
" just done a few laps of Silverstone and issue is still present, though , it seems a bit less frequent."
Suggestion to the user: "Try setting the yaw kick threshold to 0. Does it make any difference? "
Response from the user: "Threshold was already on 0 as just used default scaled for my wheel. will try again in 20mins with same tricks from yesterday and get back to you."

### Changelod notes from the patch
- **Yaw Kick Signal Rectification (Issue #241)**:
  - Fixed a mathematical flaw where a hard threshold was applied before smoothing in the Yaw Kick effect. This "rectified" high-frequency symmetric noise (from stiff dampers) into a DC offset, perceived as a constant directional pull.
  - Implemented **Smoothing-Before-Thresholding**: The 400Hz low-pass filter is now applied to the raw yaw acceleration first, correctly averaging out chassis chatter before any gating occurs.
  - Introduced **Continuous Deadzone**: Replaced the hard binary gate with a smooth subtraction deadzone. Force now ramps up naturally from zero once the threshold is exceeded, eliminating the "step function" feel of the legacy implementation.


# Answer 1




Based on the codebase provided and the symptoms described, we can definitely identify a few potential culprits. The fact that the pull is "constant" (like a DC offset or suspension damage) and happens specifically with **stiff dampers** on a high-downforce car (LMP2) points to a classic signal processing issue: **High-frequency noise being amplified and aliased into a low-frequency offset.**

Here is a breakdown of what might be wrong in the code, followed by the diagnostics and investigations you can perform.

### 1. Potential Flaws in the Code (The "Extrapolator Blowup")
While your recent patch fixed the thresholding logic, there is a deeper issue in how the raw telemetry is processed before it even reaches the Yaw Kick formula.

In `FFBEngine::calculate_force`, you are upsampling the 100Hz telemetry to 400Hz using a `LinearExtrapolator`:
```cpp
m_working_info.mLocalRotAccel.y = m_upsample_local_rot_accel_y.Process(data->mLocalRotAccel.y, ffb_dt, is_new_frame);
```
**The Problem:** `LinearExtrapolator` calculates the rate of change (derivative) between the last two 100Hz frames and projects it forward. 
*   `mLocalRotAccel.y` is *Yaw Acceleration*. Its derivative is *Yaw Jerk*.
*   When you run **stiff dampers**, the chassis chatters and vibrates at high frequencies over bumps. 
*   If the telemetry oscillates rapidly (e.g., +50 rad/s² then -50 rad/s² on the next frame), the `LinearExtrapolator` calculates a massive rate of change and shoots off to extreme values during the 2.5ms inter-frame periods.
*   If the car hits a kerb asymmetrically, this extrapolation will create a **massive asymmetric spike**.
*   When this spiked signal is fed into your `m_yaw_accel_smoothed` low-pass filter, the filter "catches" the massive spike and slowly decays it. To the user, this feels exactly like a **constant, strong pull in one direction** that lasts for a second or two.

**Code Fix Suggestion:** Do not use `LinearExtrapolator` for acceleration channels. Accelerations are too noisy. Switch `m_upsample_local_rot_accel_y` (and `m_upsample_local_accel_x/y/z`) to a simple Zero-Order Hold (just return the raw value without extrapolating) or use the `HoltWintersFilter` with a very low trend weight.

---

### 2. Diagnostics: Isolating the Issue
Before diving into Motec, let's prove 100% that Yaw Kick is the culprit. The user *suspects* it, but it could be something else (like Rear Aligning Torque or Lateral Load).

**Ask the user to do this test:**
1. Load their normal LMP2 setup that causes the issue.
2. In the lmuFFB UI, set **Yaw Kick to 0.0**. Drive a few laps. Does the pull completely disappear?
3. If it still pulls, set **SoP Self-Aligning Torque (Rear Align) to 0.0**. (Rear align uses tire load, which also spikes violently with stiff dampers).
4. If it *still* pulls, set **Lateral Load to 0.0**.

---

### 3. Motec Telemetry Investigations
If you export the raw game telemetry to Motec, here is exactly what you should plot to see the physics engine freaking out:

1. **The Aliasing Plot:**
   * Plot `mLocalRotAccel.y` (Yaw Acceleration) and `mLocalAccel.x` (Lateral Acceleration).
   * **What to look for:** Zoom in on a straight or a smooth corner. With stiff dampers, you might see the signal oscillating wildly frame-by-frame (Nyquist frequency at 100Hz). Look for a "wave" enveloping the noise—this is aliasing. If the average of the noise isn't zero, the game physics is generating a DC offset.
2. **The Asymmetric Load Plot:**
   * Plot `mWheel[0].mSuspForce` (FL) vs `mWheel[1].mSuspForce` (FR).
   * **What to look for:** Stiff dampers can cause the car to "skip" across the track. If the inside wheel lifts off the ground (SuspForce hits 0) while the outside wheel takes 100% of the load, your `lat_load_norm` and `Rear Aligning Torque` calculations will generate massive directional pulls.
3. **Ride Height / Bottoming:**
   * Plot `mFrontRideHeight` and `mRearRideHeight`.
   * **What to look for:** Is the car bottoming out (hitting 0.0)? When the chassis hits the ground, infinite stiffness is applied, causing violent acceleration spikes in the physics engine.

---

### 4. Car Setup Variations to Test
To figure out exactly what mechanical trait triggers the math bug, ask the user to test these specific setup changes one by one:

*   **Fast Bump / Fast Rebound Dampers:** Soften these significantly. Fast dampers control high-frequency kerb strikes and track chatter. If softening these fixes the pull, it confirms the issue is high-frequency noise blowing up the extrapolator.
*   **Packers / Bump Stops:** Increase the gap to the bump stops (or raise the ride height). If the car is riding on the bump stops, the effective spring rate becomes infinite, causing the physics engine to output garbage acceleration data.
*   **Anti-Roll Bars (ARB):** Disconnect or set the front ARB to the softest setting. A stiff ARB transfers bumps from one side of the car to the other, creating violent lateral/yaw oscillations.

---

### 5. CSV Logging Enhancements
Your current `LogFrame` in `AsyncLogger.h` is missing a few crucial channels that would make your Python analyzer instantly spot this bug. 

I highly recommend adding the following to `LogFrame` and `WriteFrame`:

1. **Individual FFB Components:**
   Currently, you log `ffb_yaw_kick` and `ffb_sop`. You should add:
   * `float ffb_rear_torque;` (To see if Rear Aligning Torque is causing the pull)
   * `float ffb_gyro;`
   * `float ffb_lat_load;` (The isolated lateral load component)
2. **Raw vs Extrapolated Signals:**
   * Log the *raw* `data->mLocalRotAccel.y` alongside the *extrapolated/smoothed* `m_yaw_accel_smoothed`. If the Python script shows the raw data hovering around 0, but the smoothed data sitting at +5.0, you have absolute proof that the `LinearExtrapolator` is breaking the math.
3. **Suspension Forces:**
   * Add `float susp_force_fl, susp_force_fr, susp_force_rl, susp_force_rr;`
   * This will allow your Python script to detect if the car is two-wheeling or bottoming out asymmetrically.

**Python Analysis Suggestion:**
Once you have this CSV data, write a Python script to calculate the **Rolling Mean (window=100)** of `ffb_yaw_kick` and `ffb_rear_torque`. If the rolling mean deviates significantly from 0 on a straightaway, you have found your "constant pull".