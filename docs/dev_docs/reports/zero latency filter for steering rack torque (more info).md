# Question

Regarding the Main FFB Torque (Steering Shaft Torque), should I also add the option to use a HoltWintersFilter (configured for Zero Latency)? Should I keep 3 options for this (zero latency, smooth, and HoltWintersFilter )?

Should I add the same options for the "In-Game FFB", when it is selected to be used instead of the Steering Shaft Torque channel?

# Answer




Here is the breakdown of how you should handle both the **Steering Shaft Torque** and the **In-Game FFB**, and why you actually already have the right pieces in place!

### 1. Regarding the Main FFB Torque (Steering Shaft Torque)

You actually **already are** using the `HoltWintersFilter` for this! 

If you look at your `FFBEngine.cpp` code, you have this line:
```cpp
m_upsample_shaft_torque.SetZeroLatency(m_front_axle.steering_100hz_reconstruction == 0);
double shaft_torque = m_upsample_shaft_torque.Process(m_working_info.mSteeringShaftTorque, ffb_dt, is_new_frame);
```
And in your changelog for v0.7.199, you wrote: *"Upgraded `HoltWintersFilter` in `MathUtils.h` to support both Zero Latency (Extrapolation) and Smooth (Interpolation) modes."*

**Recommendation: Keep exactly 2 options.**
You do not need a third option. The two options you currently have in the UI are perfect:
1. **Zero Latency (Holt-Winters Extrapolation):** This is the best possible "raw" mode. It predicts the next value without adding a 10ms delay, but because it uses double-exponential smoothing on the *trend*, it is much less grainy than naive linear extrapolation. High-end DD users will use this.
2. **Smooth (Holt-Winters Interpolation):** This adds a 10ms delay to draw a perfectly smooth line between 100Hz ticks. Belt-driven and gear-driven users will use this to eliminate buzzing.

### 2. Regarding the "In-Game FFB" Channel

**Do NOT add these options to the "In-Game FFB" channel.**

Here is why: The reconstruction filters (`HoltWintersFilter` and `LinearExtrapolator`) exist specifically to solve a sample-rate mismatch. They take a **100Hz** signal and guess what happens in between those ticks to feed your **400Hz** physics loop.

The "In-Game FFB" (`genFFBTorque`) is natively streamed from the game at **400Hz**. 
Because it perfectly matches your physics loop rate (`ffb_dt = 0.0025`), it does not need to be upsampled, extrapolated, or interpolated. If you were to pass the 400Hz `genFFBTorque` through a filter designed for 100Hz data, you would artificially throttle the signal, destroying the high-frequency detail the game engine is trying to send you.

In your code, you are already handling this perfectly:
```cpp
double raw_torque_input = (m_front_axle.torque_source == 1) ? (double)genFFBTorque * (double)m_general.wheelbase_max_nm : shaft_torque;
```
Leave this exactly as it is. "In-Game FFB" should remain a pure, unfiltered passthrough.

---

### 3. How to Fix the User's Bug (Issue #459)

If the user is using "In-Game FFB" (which is unfiltered) or "Steering Shaft Torque" set to Zero Latency, **why are they still feeling heavy damping?**

The damping is coming from the **Auxiliary Telemetry**. 

Look at this block in your `FFBEngine.cpp`:
```cpp
// Upsample other derivative sources
m_working_info.mUnfilteredSteering = m_upsample_steering.Process(m_working_info.mUnfilteredSteering, ffb_dt, is_new_frame);
m_working_info.mLocalAccel.x = m_upsample_local_accel_x.Process(m_working_info.mLocalAccel.x, ffb_dt, is_new_frame);
// ... and the wheel channels (deflection, rotation, etc.)
```

In v0.7.198, you changed the class behind `m_upsample_steering` (the `LinearExtrapolator`) to act as a **10ms-delayed interpolator**. 

Because `mUnfilteredSteering` is now delayed and smoothed:
1. When the user turns the wheel, the software sees the steering angle changing sluggishly.
2. The **Gyro Damping** effect calculates steering velocity (`steer_vel = (steer_angle - m_prev_steering_angle) / ctx.dt`).
3. Because the angle is smoothed, the velocity is artificially widened and flattened.
4. The Gyro Damping applies a thick, viscous resistance to the user's physical steering wheel.

**The Fix:**
You need to replace the `LinearExtrapolator` class used for `m_upsample_steering`, `m_upsample_vert_deflection`, etc., with the `HoltWintersFilter` class. 

Then, give the user a single global toggle in the "Advanced" UI tab:
**"Telemetry Reconstruction Mode"**
*   **Raw / Zero Latency:** Calls `SetZeroLatency(true)` on all auxiliary Holt-Winters filters. (The Simagic user will select this, restoring all the sharp road texture and removing the Gyro mud).
*   **Smooth:** Calls `SetZeroLatency(false)` on all auxiliary Holt-Winters filters. (Entry-level users will select this to stop their wheels from rattling).

