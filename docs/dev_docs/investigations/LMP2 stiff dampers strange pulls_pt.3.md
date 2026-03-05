

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


# Answer 1

Yes, we can absolutely identify what is wrong in the code. You have hit a classic digital signal processing bug known as **Signal Rectification via Hard Thresholding**. 

The issue perfectly explains why you are getting a "constant pull" (a DC offset) when using stiff dampers (which introduce high-frequency chassis chatter).

### 1. The Root Cause in the Code
Let's look at the Yaw Kick logic in `FFBEngine::calculate_sop_lateral`:

```cpp
    // 4. Yaw Kick (Inertial Oversteer)
    double raw_yaw_accel = data->mLocalRotAccel.y;
    
    // v0.4.16: Reject yaw at low speeds and below threshold
    if (ctx.car_speed < MIN_YAW_KICK_SPEED_MS || std::abs(raw_yaw_accel) < (double)m_yaw_kick_threshold) {
        raw_yaw_accel = 0.0; // <--- THE BUG IS HERE
    }
    
    // Alpha Smoothing (v0.4.16)
    double tau_yaw = (double)m_yaw_accel_smoothing;
    if (tau_yaw < MIN_TAU_S) tau_yaw = MIN_TAU_S;
    double alpha_yaw = ctx.dt / (tau_yaw + ctx.dt);
    m_yaw_accel_smoothed += alpha_yaw * (raw_yaw_accel - m_yaw_accel_smoothed);
```

**Why this causes a constant pull:**
You are applying a **hard threshold** *before* the **low-pass filter** (`m_yaw_accel_smoothed`). 

Imagine you are driving straight, but the track has a slight camber causing a tiny, imperceptible true yaw acceleration of `0.1 rad/s²`. Because your LMP2 dampers are very stiff, the chassis chatters over the bumps, adding a high-frequency vibration of `±1.0 rad/s²`.
* Your raw signal oscillates rapidly between **`+1.1`** and **`-0.9`**.
* If your `m_yaw_kick_threshold` is set to `1.0`, look at what the `if` statement does:
  * `+1.1` passes the threshold $\rightarrow$ stays `1.1`
  * `-0.9` fails the threshold $\rightarrow$ becomes `0.0`
* The low-pass filter then averages these values: `(1.1 + 0.0) / 2 = 0.55 rad/s²`.

The code has accidentally turned a tiny `0.1` drift into a massive, sustained `0.55` force in one direction! The threshold acted as a diode (rectifier), and the smoothing filter turned that rectified noise into a constant DC pull.

### 2. The Code Fix
To fix this, you must apply the smoothing filter **first** (to extract the true macroscopic yaw of the car and kill the chassis chatter), and then apply a **continuous deadzone** (so forces don't suddenly jump from 0 to 1.0 when crossing the threshold).

Update that section of `FFBEngine.cpp` to this:

```cpp
    // 4. Yaw Kick (Inertial Oversteer)
    double raw_yaw_accel = data->mLocalRotAccel.y;
    
    // 1. Apply Alpha Smoothing FIRST to remove high-frequency chassis chatter
    double tau_yaw = (double)m_yaw_accel_smoothing;
    if (tau_yaw < MIN_TAU_S) tau_yaw = MIN_TAU_S;
    double alpha_yaw = ctx.dt / (tau_yaw + ctx.dt);
    m_yaw_accel_smoothed += alpha_yaw * (raw_yaw_accel - m_yaw_accel_smoothed);
    
    // 2. Apply a continuous deadzone (instead of a hard threshold)
    double processed_yaw = 0.0;
    if (ctx.car_speed >= MIN_YAW_KICK_SPEED_MS) {
        if (std::abs(m_yaw_accel_smoothed) > (double)m_yaw_kick_threshold) {
            // Subtract the threshold to prevent a sudden jump when crossing it
            processed_yaw = m_yaw_accel_smoothed - std::copysign((double)m_yaw_kick_threshold, m_yaw_accel_smoothed);
        }
    }
    
    ctx.yaw_force = -1.0 * processed_yaw * m_sop_yaw_gain * (double)BASE_NM_YAW_KICK;
```

### 3. Immediate App Settings to Verify
You can prove this theory right now without recompiling the code:
1. **Set `Yaw Kick Threshold` to `0.0`**: This completely disables the `if` statement. The noise will remain symmetric (zero-mean), the low-pass filter will average it to `0.0`, and the constant pull should instantly disappear (it might turn into a slight harmless vibration).
2. **Set `Yaw Kick` gain to `0.0`**: This will completely mute the effect, confirming 100% that this specific calculation is the culprit.

### 4. Car Setup Investigations (LMP2)
If you want to investigate the physics side of why the LMP2 is chattering so much:
* **Fast Bump / Fast Rebound**: Soften the high-speed damper circuits. Stiff fast-dampers cause the tire to skip over micro-bumps rather than absorbing them, sending violent rotational spikes into the chassis.
* **Packers / Ride Height**: The LMP2 relies heavily on underbody aero. If your ride height is too low or packers are too thick, the car is bottoming out on the track surface. When the plank hits the ground, suspension stiffness effectively becomes infinite, causing massive `mLocalRotAccel.y` spikes.

### 5. CSV Logging & Motec Diagnostics
Currently, your `LogFrame` struct in `AsyncLogger.h` is missing the exact channels needed to debug this! 

**Add these to `AsyncLogger.h`:**
```cpp
// In LogFrame struct:
float raw_yaw_accel;
float ffb_yaw_kick;

// In WriteHeader():
// Add "RawYawAccel,FFBYawKick," to the CSV header string

// In WriteFrame():
<< frame.raw_yaw_accel << "," << frame.ffb_yaw_kick << ","

// In FFBEngine.cpp (around line 450 where you populate the LogFrame):
frame.raw_yaw_accel = (float)data->mLocalRotAccel.y;
frame.ffb_yaw_kick = (float)ctx.yaw_force;
```

**What to look for in your Python Analyzer / Motec:**
1. Plot `RawYawAccel` (`mLocalRotAccel.y`). You will likely see a thick, fuzzy band of high-frequency noise (chatter) when driving with the stiff setup.
2. Plot `FFBYawKick`. With the old code, you will see this line drift away from zero and stay there (the constant pull) whenever the chatter gets loud. With the fixed code, it will stay cleanly at zero until the car actually oversteers.