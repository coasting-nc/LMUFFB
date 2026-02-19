Based on the user report and the provided codebase, here is an analysis of the issue and the troubleshooting steps to assist the user.

### Analysis of the Issue

The user is reporting two distinct symptoms:
1.  **"Couldn't feel any effect":** The app is running, but the wheel isn't reacting to the app's commands.
2.  **"Jerking about randomly... if SOP slider > 0":** When a specific effect is enabled, the signal becomes unstable.

**Root Cause Diagnosis:**
The most probable cause is a **"Double FFB Conflict"**.

In the `README.md`, the architecture requires the game (Le Mans Ultimate) to send its signals to a **vJoy** (Virtual) device, while the **LMUFFB App** reads telemetry and sends the new FFB to the **Physical** wheel.

If the user has bound their **Moza R9 directly in the game settings** (instead of binding the vJoy device), two things happen:
1.  **The Game** is sending FFB commands to the Moza R9.
2.  **The App** is trying to send FFB commands to the Moza R9 (via the new DirectInput mode).

This results in a "Race Condition" at the driver level. The wheel receives a command from the Game, then milliseconds later a command from the App. This causes the "Jerking" (fighting signals) and the "No Effect" (the game overwriting the app's output).

The **SOP (Seat of Pants)** effect exacerbates this because it injects a constant lateral force during cornering. If the game is trying to center the wheel (aligning torque) and the App is trying to push it sideways (SOP), the rapid switching between these two contradictory commands causes violent oscillation.

---

### Troubleshooting Response for the User

Here is a draft you can send to the user.

***

**Subject:** Re: Moza R9 Setup / Jerking Issues

Hi, thanks for the report!

Based on your description ("No effect" initially, followed by "Jerking" when SOP is added), it sounds like you might be experiencing a **Double FFB Conflict**.

This happens when both **Le Mans Ultimate (The Game)** and **LMUFFB (The App)** are trying to control your steering wheel motor at the same time. They end up fighting each other, causing the wheel to go limp or jerk violently.

Please try this exact setup sequence to ensure the "Bridge" is working correctly:

**1. The "Dummy" Setup (In-Game)**
*   Ensure **vJoy** is installed.
*   In Le Mans Ultimate **Controls** menu, you must bind your **Steering Axis** to the **vJoy Device**, NOT your Moza R9.
    *   *Tip:* You may need to use the "vJoy Feeder" app (comes with vJoy) to move the virtual axis so the game detects it.
*   This ensures the game sends its Force Feedback to the "Dummy" (vJoy) and leaves your real wheel free for the App to control.

**2. The "Real" Setup (In App)**
*   Open LMUFFB.
*   In the **FFB Device** dropdown, select your **Moza R9**.
*   Now, the App will read the physics from the game and send the force to your Moza.

**3. Verify SOP Settings**
*   The **SOP (Seat of Pants)** effect adds Lateral G-force to the wheel. On a Direct Drive wheel like the R9, this can be very strong.
*   Once you have the vJoy binding sorted, start with **SOP Effect** at **0.0** and **Master Gain** at **0.5**.
*   Gradually increase SOP. If it still jerks on straights, let me know—it might indicate noise in the telemetry data which we can filter out in the next update.

**Summary Checklist:**
1.  [ ] Game Steering Axis = vJoy Device (Axis X)
2.  [ ] App FFB Device = Moza R9
3.  [ ] Game FFB Strength = 0 (or default, doesn't matter as long as it's sent to vJoy)

Let me know if that resolves the fighting sensation!

***

### Developer Notes (Internal)

If the user confirms they set it up correctly and the issue persists, there are two code-side areas to investigate in `FFBEngine.h` and `DirectInputFFB.cpp`:

1.  **SOP Noise Filtering (`FFBEngine.h`):**
    The current SOP implementation takes raw lateral acceleration:
    ```cpp
    double lat_g = data->mLocalAccel.x / 9.81;
    double sop_force = lat_g * m_sop_effect * 1000.0;
    ```
    If `mLocalAccel.x` is noisy (jittering around 0 on straights), this will translate directly to torque jitter.
    *Fix:* Implement a simple low-pass filter or a deadzone for `lat_g` in the next build.

2.  **DirectInput Update Safety (`DirectInputFFB.cpp`):**
    In `UpdateForce`, we check `if (magnitude == m_last_force) return;`.
    However, on high-torque DD wheels, rapid updates (400Hz) of the `ConstantForce` parameter without a `DIEP_START` flag is usually correct, but some drivers handle this poorly.
    *Verification:* If the user is still having issues, we might need to throttle the DirectInput update rate to 100Hz (while keeping physics at 400Hz) to see if the Moza driver prefers less frequent updates.