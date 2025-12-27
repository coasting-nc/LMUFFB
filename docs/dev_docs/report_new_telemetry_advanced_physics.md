# Report: New Telemetry Effects & Advanced Physics

## 1. Introduction and Context
The "Troubleshooting 25" list and following notes suggest a desire to expand the physical model of the FFB engine. Current effects are primarily lateral (SoP) and vibrational. The goal is to incorporate Longitudinal physics (Dive/Squat), Chassis Rotation (Pitch/Roll), and specialized Surface effects (Wet/Grass).

**Features Requested:**
*   **Chassis Movement**: Use `mLocalRot`, `mLocalRotAccel` to feel the car's body roll and pitch.
*   **Deceleration Cues**: Use `mLocalAccel.z` for "Brake Dive" (weight transfer feeling on steering) and "Acceleration Squat".
*   **"Rubbery" Lockup**: A feeling of "change in deceleration" constant force rather than just vibration when locking up.
*   **True Bottoming**: Use `mSuspensionDeflection` (if available) to detect hitting bump stops, rather than just Force spikes.
*   **Wet/Surface Effects**: Use `mSurfaceType`, `mRaining`, `mTemperature` to modulate grip and friction dynamically.

TODO: mLocalRotAccel is not used in any of the proposed solutions.
TODO: split this report in two: have a separate report only for **Chassis Movement**, **Deceleration Cues**, **"Rubbery" Lockup** (Chassis Body Effects, Advanced Lockup - Longitudinal Force). Also include a more fleshed out description of each effect, and what feeling  from real life driving are we trying to recreate. Expand on the notes in docs\dev_docs\TODO.md

## 2. Proposed Solution

### 2.1. Chassis Body Effects
*   **Weight Transfer Force**: Map `mLocalAccel.z` (Longitudinal) to a constant steering offset (centering or lightening).
    *   *Concept*: Under heavy braking, weight transfers forward -> Front tires load up -> Steering gets heavier (Self Aligning Torque naturally handles this via `mTireLoad`, but we can add a specific "Dive" cue if SAT is clipped or insufficient).
*   **Roll Cues**: Use `mLocalRot.z` (Roll Rate) to add a subtly distinct frequency or force layer during rapid direction changes (chicane).

### 2.2. Advanced Lockup (Longitudinal Force)
*   **Theory**: When a tire locks, the longitudinal braking force drops (or plateaus) and becomes erratic. The driver feels a loss of deceleration "G-force".
*   **Implementation**: Calculate the derivative of `mLocalAccel.z` (Jerk). If `Jerk` is negative (losing deceleration) AND `BrakePressure` is constant/increasing, it indicates a Lockup Slide.
*   **Effect**: Reduce the `Master Gain` momentarily or inject a "Counter-Force" to simulate the loss of resistance.

TODO: reconsider; this seems to only trigger when we have already lockup, so it's not predictive, just reactive; however, since this is not a vibration (which is a lower class of effects), but rather a force/load effect, this could be an improvement over what we already have among the reactive effects to lockups, and could be enabled independently of the lockup vibration effect.

### 2.3. Surface & Weather
*   **Wet Mod**: If `mSurfaceType == 1` (Wet) or `mRaining > 0.1`:
    *   Scale `m_optimal_slip_angle` by 0.8 (Peak grip happens earlier).
    *   Scale `m_global_friction` by 0.7 (Overall forces lower).
    *   Enhance "Slide Texture" (easier to slide).

### 2.4. Bottoming Method C
*   **Logic**: If `mSuspensionDeflection` > `limit` (e.g. 95% of travel):
    *   Trigger "Hard Bump" (Single impulse).

## 3. Implementation Plan

### 3.1. `src/FFBEngine.h`
1.  **Add Effect Variables**:
    ```cpp
    float m_brake_dive_gain = 0.0f;
    float m_body_roll_gain = 0.0f;
    ```
2.  **Update `calculate_force`**:
    *   **Brake Dive**:
        ```cpp
        // Add weight based on long accel
        double dive_force = data->mLocalAccel.z * m_brake_dive_gain * 0.1; // Scale factor
        total_force += dive_force;
        ```
    *   **Logic for Surface**:
        ```cpp
        double surf_friction = 1.0;
        if (data->mSurfaceType == 1) surf_friction = 0.7;
        total_force *= surf_friction;
        ```

### 3.2. `src/GuiLayer.cpp`
1.  **New "Body & Chassis" Group**:
    *   Add sliders for `Brake Dive Gain`, `Body Roll Gain`.

## 4. Testing Plan

### 4.1. Wet Track Test
*   **Setup**: Load a rainy session in LMU.
*   **Action**: Drive the same car/setup as dry.
*   **Verification**: FFB should feel uniformly lighter. Slides should initiate at lower steering angles (due to `optimal_slip` scaling).

### 4.2. Brake Dive
*   **Setup**: Maximize "Brake Dive Gain".
*   **Action**: Drive straight, slam brakes.
*   **Verification**: The wheel should get significantly heavier (or lighter, depending on sign) during the braking phase, independent of the cornering force.
