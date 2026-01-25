# FFB Effects & Customization Guide

This document details the Force Feedback effects implemented in LMUFFB, how they are derived from telemetry, and how to customize them.

## 1. Understeer (Front Grip Loss)
*   **Goal**: To communicate when the front tires are losing grip and sliding (pushing).
*   **Telemetry**: Derived from `mGripFract` (Grip Fraction) of the **Front Left (FL)** and **Front Right (FR)** tires.
*   **Mechanism**: Modulates the main steering force.
    *   `Output = GameForce * (1.0 - (1.0 - FrontGrip) * UndersteerGain)`
    *   As front grip drops, the wheel becomes lighter ("goes light"), simulating the loss of pneumatic trail and self-aligning torque.
*   **Customization**:
    *   **Understeer Effect (Slider)**: Controls the intensity of the lightening effect.

## 2. Oversteer (Rear Grip Loss)
*   **Goal**: To communicate when the rear tires are losing grip (loose/sliding), allowing the driver to catch a slide early.
*   **Current Multi-Effect Implementation (v0.6+)**:
    The oversteer system uses multiple distinct effects that work together:
    
    1. **Lateral G (SoP)**: Injects chassis lateral acceleration into wheel torque, providing "weight" cues during cornering. Calculated in `calculate_sop_lateral()`.
    
    2. **Lateral G Boost (Slide)**: Amplifies the SoP force when rear grip is lower than front grip (oversteer condition). Formula: `SoP *= (1.0 + (FrontGrip - RearGrip) * OversteerBoost * 2.0)`.
    
    3. **Rear Align Torque (SoP Self-Aligning)**: Calculates a synthetic aligning torque for the rear axle using slip angle and estimated load. Provides directional counter-steering pull during slides.
    
    4. **Yaw Kick**: Sharp momentary impulse at the onset of rotation. Derived from `mLocalRotAccel.y` with configurable activation threshold (`m_yaw_kick_threshold`, default 0.2 rad/s²) to filter road noise.

## 3. Braking Lockup (Progressive Scrub)
*   **Goal**: To signal when tires have stopped rotating during braking (flat-spotting risk), allowing the driver to find the threshold.
*   **Current Dynamic Implementation (v0.2.2+)**:
    *   **Progressive Vibration**: Signal is derived from `SlipRatio` deviation.
    *   **Range**: -0.1 (Peak Grip) to -0.5 (Locking).
    *   **Frequency**: Transitions from High Pitch (60Hz) at the limit to Low Pitch (10Hz) at full lock.
    *   **Amplitude**: Scales linearly with severity.
*   **Customization**:
    *   **Lockup Rumble (Toggle)**: Enable/Disable.
    *   **Lockup Gain (Slider)**: Intensity of the vibration.

## 4. Wheel Spin (Traction Loss)
*   **Goal**: To signal when the driven wheels are spinning under power.
*   **Current Dynamic Implementation (v0.2.2+)**:
    *   **Torque Reduction**: As rear wheel slip increases, the total FFB force is reduced (simulating "floating" rear end).
    *   **Vibration**: Frequency scales with wheel speed difference (Slip Ratio), giving a "revving up" sensation through the rim.
*   **Customization**:
    *   **Spin Traction Loss (Toggle)**: Enable/Disable.
    *   **Spin Gain (Slider)**: Intensity.

## 5. Road & Slide Texture
*   **Slide Texture**: Adds "scrubbing" vibration when any tire is sliding laterally (high Slip Angle).
*   **Road Texture**: Adds "bumps" based on suspension velocity changes (High-Pass Filter).

---

## Signal Interference & Clarity

A critical challenge in FFB design is managing the "Noise Floor". When multiple effects are active simultaneously, they can interfere with each other or mask the underlying physics.

### 1. Signal Masking
*   **The Issue**: High-frequency vibrations (like **Lockup Rumble** or **Road Texture**) can physically overpower subtle torque changes (like **Understeer Lightness** or **SoP**). If the wheel is vibrating violently due to a lockup, the driver might miss the feeling of the rear end stepping out (SoP).
*   **Mitigation**:
    *   **Priority System** *(Planned, Not Yet Implemented)*: A future version will implement "Side-chaining" or "Ducking" to dynamically reduce lower-priority effects when higher-priority signals need headroom. For example, if understeer occurs during curb contact, the system would reduce Road Texture to preserve the grip information signal.
    *   **Frequency Separation**: Ideally, "Information" (Grip/SoP) should be low-frequency (< 20Hz), while "Texture" (Lockup/Spin/Road) should be high-frequency (> 50Hz). This helps the human hand distinguish them.

### 2. Clipping
*   **The Issue**: Summing multiple effects (Game Torque + SoP + Rumble) can easily exceed the 100% force capability of the motor.
*   **Result**: The signal "clips" (flattens at max force). Information is lost. E.g., if you are cornering at 90% torque and a 20% SoP effect is added, you hit 100% and lose the detail of the SoP ramp-up.
*   **Mitigation**:
    *   **Master Limiter**: A soft-clip algorithm that compresses dynamic range rather than hard-clipping.
    *   **Tuning**: Users are advised to set "Master Gain" such that peak cornering forces hover around 70-80%, leaving headroom for dynamic effects.

### 3. Ambiguity (Texture Confusion)
*   **The Issue**: **Lockup** and **Wheel Spin** often use similar "Synthetic Rumble" effects. In the heat of battle, a driver might confuse one for the other if relying solely on the tactile cue without context (pedal position).
*   **Mitigation**:
    *   **Distinct Frequencies**: Future updates will tune Lockup to be "Sharper/Higher Pitch" (square wave) and Wheel Spin to be "Rougher/Lower Pitch" (sawtooth or randomized).
    *   **Context**: Since the driver knows if they are braking or accelerating, this ambiguity is usually resolved by context, but distinct tactile signatures help subconscious reaction times.


### 4. Interaction of Spin vs SoP Effects

*   The **Spin** effect reduces `total_force` (Torque Drop).
*   The **SoP** effect boosts force during oversteer.
*   **Result**: These two will fight slightly during a power slide. This is actually a good "natural" balance—the wheel tries to self-align (SoP), but the loss of traction makes it feel lighter/vaguer (Spin Drop). This should feel intuitive to the driver.

### 5. Robustness & Telemetry Health
LMUFFB includes a "Sanity Check" layer that protects effects against telemetry glitches (common in some game builds).
*   **Missing Load**: If the game reports 0 Load on tires, texture effects (Slide/Road/Lockup) will use a fallback value instead of going silent.
*   **Missing Grip**: If Grip data is missing, the Understeer effect defaults to "Full Grip" so you don't lose FFB entirely.
*   *Note:* If these fallbacks are triggered, a **Red Warning** will appear in the Telemetry Inspector GUI.

---

## Comparison of Implementation with iRFFB and Marvin's AIRA

| Effect | iRFFB (iRacing) | Marvin's AIRA (iRacing) | LMUFFB (LMU/rF2) |
| :--- | :--- | :--- | :--- |
| **Oversteer** | **SoP (Lateral G)** + Yaw logic | **Layered Effect**: Separate "Slip" channel. | **Rear Aligning Torque + SoP**: Synthetic rear-axle torque integration. |
| **Lockup** | Not explicit (part of "Understeer" feel in iRacing logic) | **Pedal Haptics** (often sent to pedals, but can be on wheel) | **Progressive Wheel Scrub**: Dynamic frequency/amplitude based on slip ratio. |
| **Wheel Spin** | Not explicit | **Pedal Haptics** / Wheel Rumble | **Torque Drop + Vibration**: Simulates traction loss + progressive rumble. |

---

## Legacy Implementation Notes (Pre-v0.2.2)

*   **Old Oversteer**: Relied solely on Grip Delta between Front/Rear to boost SoP.
*   **Old Lockup**: Binary rumble triggered when `SlipRatio < -0.2`.
*   **Old Wheel Spin**: Binary rumble triggered when `SlipRatio > 0.2`.
