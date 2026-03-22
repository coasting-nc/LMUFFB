# Issue #466: Differentiate among the auxiliary channels: apply different setting for the zero latency reconstruction filter

**Status:** Open
**Opened by:** coasting-nc
**Date:** Mar 22, 2026

## Description
Differentiate among the auxiliary channels: apply different setting for the zero latency reconstruction filter

Follow the implementation plan defined here:
docs\dev_docs\implementation_plans\plan_issue_462_aux_channel_differentiation.md

## Context
Following the migration of all auxiliary telemetry channels to the HoltWintersFilter (Issue #461), this patch introduces signal-specific tuning. Not all telemetry signals share the same noise profile. Treating smooth driver inputs the same as violent chassis impacts leads to either sluggish responsiveness or dangerous mathematical spikes. This plan categorizes the 22 auxiliary channels into three distinct DSP groups (Driver Inputs, High-Frequency Texture, and Chassis Kinematics) and applies hardcoded alpha, beta, and ZeroLatency rules tailored to their physical characteristics.

## Design Rationale
Why differentiate?
In Digital Signal Processing (DSP), extrapolating a noisy signal is dangerous. The HoltWintersFilter in "Zero Latency" mode uses the signal's trend (derivative) to predict the future.
1. If we extrapolate a 50G kerb strike (Chassis Kinematics), the trend points to infinity, causing a massive artificial FFB jolt. These must be heavily smoothed and interpolated.
2. If we delay steering angle (Driver Inputs), derivative-based effects like Gyro Damping feel like thick mud. These must always be zero-latency.
3. High-frequency vibrations (Texture) are subjective. These should remain tied to the user's UI preference.

---

## Question
The coding agent is working on the patch, and it has written these suggestions for the future in the implementation notes:
* Monitor user feedback on "Zero Latency" stability. While mathematically superior, predictive filters can occasionally amplify telemetry noise if the alpha/beta tuning is too aggressive.
* Consider making Alpha/Beta parameters configurable per axle in a future update if some cars exhibit "jitter" in Zero Latency mode.

Given that, I am considering if we should differentiate among the auxiliary channels and have different setting among them, eg. according to which ones are more likely to be noisy. For some we could decide to expose alpha and beta parameters in the GUI if necessary. Here are the auxiliary channels we currently apply the "zero latency" reconstruction to:
```cpp
for (int i = 0; i < 4; i++) {
    m_upsample_lat_patch_vel[i].SetZeroLatency(aux_zero_latency);
    m_upsample_long_patch_vel[i].SetZeroLatency(aux_zero_latency);
    m_upsample_vert_deflection[i].SetZeroLatency(aux_zero_latency);
    m_upsample_susp_force[i].SetZeroLatency(aux_zero_latency);
    m_upsample_brake_pressure[i].SetZeroLatency(aux_zero_latency);
    m_upsample_rotation[i].SetZeroLatency(aux_zero_latency);
}
m_upsample_steering.SetZeroLatency(aux_zero_latency);
m_upsample_throttle.SetZeroLatency(aux_zero_latency);
m_upsample_brake.SetZeroLatency(aux_zero_latency);
m_upsample_local_accel_x.SetZeroLatency(aux_zero_latency);
m_upsample_local_accel_y.SetZeroLatency(aux_zero_latency);
m_upsample_local_accel_z.SetZeroLatency(aux_zero_latency);
m_upsample_local_rot_accel_y.SetZeroLatency(aux_zero_latency);
m_upsample_local_rot_y.SetZeroLatency(aux_zero_latency);
```

Determine me if we should do such differentiation, and how.

## Answer
This is an excellent observation by the coding agent, and it touches on a fundamental rule of Digital Signal Processing (DSP): You should not treat all signals equally.

To answer your question: Yes, you absolutely should differentiate among the auxiliary channels.

Here is why: The HoltWintersFilter in "Zero Latency" mode uses the signal's trend (derivative) to extrapolate into the future.
* If you extrapolate a smooth signal (like steering input), it works perfectly and eliminates lag.
* If you extrapolate a highly noisy signal (like vertical chassis acceleration hitting a kerb), the trend will momentarily point to infinity, causing the filter to wildly overshoot and create massive artificial FFB spikes.

You do not need to expose alpha and beta to the user in the GUI—that is usually too complex for a standard tuning menu. Instead, you should group the channels by their physical characteristics and hardcode sensible alpha/beta and ZeroLatency rules for each group.

### Group 1: Driver Inputs (Smooth & Latency-Sensitive)
Channels: m_upsample_steering, m_upsample_throttle, m_upsample_brake, m_upsample_brake_pressure
* Characteristics: These come from physical USB devices (potentiometers/encoders). They are inherently smooth and bounded.
* Strategy: ALWAYS Zero Latency. You never want to delay the driver's own inputs. Delaying steering is what caused the Gyro Damping to feel like mud.
* Tuning: High Alpha (trust the raw input), Low Beta (smooth the trend slightly). e.g., Configure(0.9, 0.1).

### Group 2: High-Frequency Texture (The "Feel" Channels)
Channels: m_upsample_vert_deflection, m_upsample_lat_patch_vel, m_upsample_long_patch_vel, m_upsample_rotation
* Characteristics: These contain the micro-vibrations of the road, tire slip, and ABS lockup.
* Strategy: Tie to the User UI Toggle. This is exactly where the Simagic user wants "Zero Latency" (to feel every pebble) and the Logitech user wants "Smooth" (to stop the plastic rattling).
* Tuning: Balanced. e.g., Configure(0.8, 0.2).

### Group 3: Chassis Kinematics & Impacts (Highly Noisy)
Channels: m_upsample_local_accel_x/y/z, m_upsample_local_rot_accel_y, m_upsample_local_rot_y, m_upsample_susp_force
* Characteristics: In a physics engine, rigid body accelerations and pushrod forces are incredibly violent and noisy. A single kerb strike will cause a massive 1-frame spike.
* Strategy: ALWAYS Smooth (Interpolation). If you extrapolate a 5G kerb strike, the Holt-Winters trend will multiply it, causing a math explosion that could violently snap the user's wheel. These channels are used for macro-forces (like Seat-of-Pants load transfer), which do not need 0ms latency.
* Tuning: Lower Alpha (heavy smoothing), Zero Beta (disable trend extrapolation entirely to prevent overshoot). e.g., Configure(0.5, 0.0).

---

## Recommended Tuning Guide for the Agent
Group 1: Driver Inputs (Steering, Throttle, Brake)
* Mode: ALWAYS Zero Latency (SetZeroLatency(true)).
* Tuning: Configure(0.95, 0.40).

Group 2: High-Frequency Texture (Deflection, Patch Velocities, Wheel Rotation)
* Mode: TIED TO UI TOGGLE.
* Tuning: Configure(0.80, 0.20).

Group 3: Chassis Kinematics & Impacts (Accelerations, Suspension Pushrod Force)
* Mode: ALWAYS Smooth (SetZeroLatency(false)).
* Tuning: Configure(0.50, 0.00).
