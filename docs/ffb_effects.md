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
*   **Current Dynamic Implementation (v0.2.2+)**:
    *   **Aligning Torque Integration**: Calculates a synthetic "Aligning Torque" for the rear axle using `Rear Lateral Force`.
    *   **Mechanism**: This force is injected into the steering signal. If the rear tires generate large lateral forces (resisting a slide), the steering wheel will naturally counter-steer, providing a physical cue to catch the slide. This is modulated by the `Oversteer Boost` slider.
    *   **SoP (Seat of Pants)**: Also injects Lateral G-force into the wheel torque to provide "weight" cues.

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

## Legacy Implementation Notes (Pre-v0.2.2)

*   **Old Oversteer**: Relied solely on Grip Delta between Front/Rear to boost SoP.
*   **Old Lockup**: Binary rumble triggered when `SlipRatio < -0.2`.
*   **Old Wheel Spin**: Binary rumble triggered when `SlipRatio > 0.2`.

---

## Comparison of Implementation

| Effect | iRFFB (iRacing) | Marvin's AIRA (iRacing) | LMUFFB (LMU/rF2) |
| :--- | :--- | :--- | :--- |
| **Oversteer** | **SoP (Lateral G)** + Yaw logic | **Layered Effect**: Separate "Slip" channel. | **Rear Aligning Torque + SoP**: Synthetic rear-axle torque integration. |
| **Lockup** | Not explicit (part of "Understeer" feel in iRacing logic) | **Pedal Haptics** (often sent to pedals, but can be on wheel) | **Progressive Wheel Scrub**: Dynamic frequency/amplitude based on slip ratio. |
| **Wheel Spin** | Not explicit | **Pedal Haptics** / Wheel Rumble | **Torque Drop + Vibration**: Simulates traction loss + progressive rumble. |

---

## Signal Interference & Clarity

A critical challenge in FFB design is managing the "Noise Floor". When multiple effects are active simultaneously, they can interfere with each other or mask the underlying physics.

### 1. Signal Masking
*   **The Issue**: High-frequency vibrations (like **Lockup Rumble** or **Road Texture**) can physically overpower subtle torque changes (like **Understeer Lightness** or **SoP**). If the wheel is vibrating violently due to a lockup, the driver might miss the feeling of the rear end stepping out (SoP).
*   **Mitigation**:
    *   **Priority System**: Future versions should implement "Side-chaining" or "Ducking". For example, if a severe Lockup event occurs, reduce Road Texture gain to ensure the Lockup signal is clear.
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
