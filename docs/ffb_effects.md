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
*   **Telemetry**: Derived from `mGripFract` of the **Rear Left (RL)** and **Rear Right (RR)** tires, and `mLocalAccel.x` (Lateral G).
*   **Mechanism**:
    1.  **Seat of Pants (SoP)**: Injects Lateral G-force into the wheel torque. This provides a "weight" cue that correlates with the car's yaw/cornering force.
    2.  **Rear Traction Loss**: Monitors the rear tires. If rear grip drops significantly while the front has grip, it boosts the SoP effect or adds a specific vibration/cue to signal rotation.
*   **Customization**:
    *   **SoP Effect (Slider)**: Controls the strength of the Lateral G injection.
    *   **Oversteer Boost (Slider)**: Controls how much the SoP effect is amplified when rear grip is lost.

## 3. Braking Lockup
*   **Goal**: To signal when tires have stopped rotating during braking (flat-spotting risk).
*   **Telemetry**:
    *   `mUnfilteredBrake` > 0.
    *   `mSlipRatio` or `mGripFract`: If the tire rotation speed deviates massively from the road speed (Slip Ratio -> -1.0 means locked).
*   **Mechanism**: Injects a high-frequency, harsh vibration ("Rumble") when locking is detected.
*   **Customization**:
    *   **Lockup Rumble (Toggle)**: Enable/Disable.
    *   **Lockup Gain (Slider)**: Intensity of the vibration.

## 4. Wheel Spin (Acceleration Slip)
*   **Goal**: To signal when the driven wheels are spinning under power.
*   **Telemetry**:
    *   `mUnfilteredThrottle` > 0.
    *   `mSlipRatio`: If positive and high (wheel spinning faster than road speed).
*   **Mechanism**: Injects a medium-frequency vibration ("Scrub") to indicate power-slide or burnout.
*   **Customization**:
    *   **Wheel Spin Rumble (Toggle)**: Enable/Disable.
    *   **Wheel Spin Gain (Slider)**: Intensity.

## 5. Road & Slide Texture
*   **Slide Texture**: Adds "scrubbing" vibration when any tire is sliding laterally (high Slip Angle).
*   **Road Texture**: Adds "bumps" based on suspension velocity changes (High-Pass Filter).

---

## Comparison of Implementation

| Effect | iRFFB (iRacing) | Marvin's AIRA (iRacing) | LMUFFB (LMU/rF2) |
| :--- | :--- | :--- | :--- |
| **Oversteer** | **SoP (Lateral G)** + Yaw logic | **Layered Effect**: Separate "Slip" channel. | **SoP + Rear Grip Logic**: Uses direct rear tire grip telemetry to boost SoP. |
| **Lockup** | Not explicit (part of "Understeer" feel in iRacing logic) | **Pedal Haptics** (often sent to pedals, but can be on wheel) | **Wheel Rumble**: Direct rumble effect on the steering wheel when `SlipRatio` indicates locking. |
| **Wheel Spin** | Not explicit | **Pedal Haptics** / Wheel Rumble | **Wheel Rumble**: Direct rumble effect when `SlipRatio` indicates positive spin. |

---

## Future: Dynamic vs Synthetic Effects

Currently, LMUFFB (and many sim apps) use "Canned" or "Synthetic" effects for certain events—generating a predefined vibration wave when a threshold is crossed.
The goal for future versions is to replace these with **Dynamic Telemetry-Based Signals** that evolve organically with the physics.

### 1. Oversteer (Rear Grip Loss)
*   **Current State**: **Dynamic**. We use the delta between Front and Rear grip (`GripFract`) to mathematically boost the SoP effect. This is already a physics-derived method, not a canned rumble.
*   **Future Improvement**:
    *   **Aligning Torque Integration**: Instead of just boosting SoP, we could calculate the *Self Aligning Torque (SAT)* of the rear tires explicitly and mix it into the steering signal. This would provide a "counter-steer" force that pulls the wheel into the slide naturally, rather than just adding "weight".

### 2. Braking Lockup (Pre-Lockup Feel)
*   **Current State**: **Synthetic (Rumble)**. When `SlipRatio < -0.2`, a square wave vibration is triggered.
*   **The Problem**: It acts as a binary alarm ("You are locking up!"). It doesn't help you find the limit *before* locking.
*   **Future Solution**: **Progressive Scrub**.
    *   **Logic**: Tires generate maximum braking force at a specific slip ratio (usually around -0.10 to -0.15). Beyond this peak, grip falls off.
    *   **Implementation**: Create a vibration signal where **Frequency** and **Amplitude** are functions of `SlipRatio`.
        *   `SlipRatio 0.0 to -0.1`: No effect (Pure grip).
        *   `SlipRatio -0.1 to -0.2`: **High Frequency / Low Amplitude** "Micro-scrub". This tells the driver they are at the limit (Peak braking).
        *   `SlipRatio < -0.2`: **Low Frequency / High Amplitude** "Judder". This indicates the wheel is beginning to lock/hop.
    *   **Comparison**:
        *   **Marvin's AIRA**: Uses "Tactile" channels (often sent to bass shakers) that allow defining frequency curves mapped to telemetry. This is effectively the "Dynamic" approach but implemented via sound card output.
        *   **iRFFB**: Does not explicitly add lockup vibrations. It relies on the game physics engine to reduce the steering torque naturally as the front load changes during braking.

### 3. Wheel Spin (Traction Loss)
*   **Current State**: **Synthetic (Rumble)**. Triggered when `SlipRatio > 0.2`.
*   **The Problem**: Similar to lockup, it's a late warning.
*   **Future Solution**: **Torque Drop-off Simulation**.
    *   **Logic**: When rear tires spin, the car's yaw acceleration often spikes, and the steering torque should change.
    *   **Implementation**:
        1.  **Vibration**: Scale vibration intensity linearly with `SlipRatio`. `Intensity = (SlipRatio - Threshold) * Gain`. This allows feeling the "spin up".
        2.  **Steering Lightness**: When rear wheels spin, the car often yaws. We can *reduce* the SoP effect slightly or modulate it with high-frequency noise to simulate the rear end "floating" on the power.

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
