# Investigation: FFB Effects Acceleration Analysis

## Objective
Following the resolution of the Yaw Kick "constant pull" issue (v0.7.144), this investigation evaluates whether other Force Feedback effects in `lmuFFB` are vulnerable to similar artifacts due to reliance on noisy, 100Hz game-provided acceleration channels (`mLocalAccel` and `mLocalRotAccel`).

## The Problem Pattern
The Yaw Kick bug was caused by a "Rectification" of high-frequency noise. Specifically:
1. The game outputs telemetry at 100Hz.
2. Stiff physics components (like dampers) create high-frequency chatter (>50Hz).
3. This chatter is aliased into the 100Hz sample rate, appearing as massive, non-physical spikes.
4. If an effect uses these spikes directly (or a derivative of them), it creates a DC offset in the FFB.

## Channel Usage Analysis

### 1. Vertical Acceleration (`mLocalAccel.y`)
**Used in**: `calculate_road_texture` (Fallback Mode for encrypted DLC).
*   **Current Implementation**: 
    ```cpp
    double delta_accel = vert_accel - m_prev_vert_accel;
    road_noise_val = delta_accel * ACCEL_ROAD_TEXTURE_SCALE;
    ```
*   **Risk**: **High**. This calculates "Jerk" (the derivative of acceleration). If the raw `mLocalAccel.y` has aliasing spikes, the "Jerk" will be magnified by orders of magnitude. This likely causes harsh, metallic "clacks" in the FFB when driving cars with encrypted telemetry on bumpy surfaces.
*   **Recommendation**: Derive vertical acceleration from `mLocalVel.y` before calculating the delta.

### 2. Longitudinal Acceleration (`mLocalAccel.z`)
**Used in**: `calculate_lockup_vibration` (Predictive Trigger).
*   **Current Implementation**: 
    ```cpp
    double car_dec_ang = -std::abs(data->mLocalAccel.z / radius);
    if (wheel_accel < car_dec_ang * threshold) { /* Trigger Lockup Pulse */ }
    ```
*   **Risk**: **Medium**. The `wheel_accel` is derived from rotation (velocity), making it very smooth. But the baseline it compares against (`car_dec_ang`) is raw and noisy. A 1-frame longitudinal spike from a curb can momentarily trick the engine into thinking the car has stopped decelerating, making the ABS/Lockup vibration feel "stuttery" or inconsistent.
*   **Recommendation**: Use a derived `mLocalAccel.z` from `mLocalVel.z`.

### 3. Lateral Acceleration (`mLocalAccel.x`)
**Used in**: `calculate_sop_lateral` (Seat of the Pants) and `calculate_kinematic_load`.
*   **Current Implementation**: Uses `mLocalAccel.x` with Exponential Moving Average (EMA) smoothing.
*   **Risk**: **Low**. SoP represents a bulk chassis state. The smoothing constants used (typically ~35ms to 100ms) are heavy enough to suppress the most violent aliasing. Additionally, SoP is a "Low Frequency" sensation where raw acceleration is a more natural fit than its derivative.
*   **Recommendation**: Maintain current implementation, but ensure the upsampled version is always used.

## Summary Table

| Effect | Channel | Derivative Order | Risk Level | Recommendation |
| :--- | :--- | :--- | :--- | :--- |
| **Yaw Kick** | `mLocalRotAccel.y` | 1st (Acceleration) | **FIXED** | Already using Derived Acceleration (v0.7.144). |
| **Road Texture** | `mLocalAccel.y` | 2nd (Jerk) | **HIGH** | Re-calculate Jab/Jerk from velocity-derived acceleration. |
| **Lockup Pred.** | `mLocalAccel.z` | 1st (Acceleration) | **MEDIUM** | Use velocity-derived long-accel for comparison baseline. |
| **SoP Lateral** | `mLocalAccel.x` | 1st (Acceleration) | **LOW** | Current EMA smoothing is sufficient. |

## Proposed Solution Strategy
We should introduce a centralized "Derived Acceleration" layer in the 400Hz loop. Instead of effects reaching for `data->mLocalAccel`, they should use a set of internal variables:
- `m_derived_accel_x` (from `mLocalVel.x`)
- `m_derived_accel_y` (from `mLocalVel.y`)
- `m_derived_accel_z` (from `mLocalVel.z`)

This ensures that the entire FFB pipeline is grounded in velocity-based physics, which are naturally integrated and "cleaner" than the game's raw industrial-sensor outputs.
