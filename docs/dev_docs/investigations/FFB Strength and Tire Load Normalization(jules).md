# Deep Research Report: FFB Strength and Tire Load Normalization

## Executive Summary
Normalization of Force Feedback (FFB) in telemetry-based applications aims to solve two distinct problems:
1.  **Steering Weight Consistency**: Ensuring different car classes (GT3 vs. Hypercar) feel similarly "heavy" or "light" relative to their physical limits without requiring manual gain adjustments.
2.  **Haptic Detail Scaling**: Ensuring tactile effects (Road Texture, Brake Vibration) maintain appropriate physical intensity as aerodynamic load and vehicle mass change.

## 1. FFB Strength Normalization (Overall Weight)

### Current Industry Standards
- **iRacing "Auto-Strength"**: Monitors the peak steering rack torque over a period of driving (usually a few corners) and sets the `Max Force` setting to that peak. This maximizes dynamic range for every car/track combo.
- **rFactor 2 "Torque Capability"**: Allows users to specify their physical wheel's Nm. If the car's physics torque exceeds this, it clips. If it's below, it scales.
- **irFFB**: Implements a "Max Force" setting per car. When "Auto" is pressed, it captures the session peak.

### Comparison of Methods

| Method | Description | Pros | Cons |
| :--- | :--- | :--- | :--- |
| **Fixed Class Mapping** | Use lookup tables (GT3=25Nm, HC=45Nm) to scale input torque. | Instant effect; no "learning" lap required. | Inaccurate for specific car setups or unusual aero. |
| **Dynamic Session Peak** | Monitor `mSteeringShaftTorque` and set 100% output to the seen peak. | Perfectly tuned to car/track/setup. | Requires a "warm-up" period; strength changes as you drive harder. |
| **Static Weight Scaling** | Scale FFB based on vehicle mass/static corner weight. | Physically grounded. | Does not account for aerodynamic effects on steering rack. |

### Recommendation for LMUFFB
The most robust approach is **Session-Learned Peak Normalization** (similar to iRacing's Auto-Gain). 
- The app should maintain a `session_peak_torque` variable (fast-attack, very-slow-decay).
- The normalization factor should be `m_max_torque_ref / session_peak_torque`.
- This ensures that if a user sets `Max Torque Ref` to 20Nm, *any* car's peak effort will physically feel like 20Nm on their wheel.

---

## 2. Tire Load Normalization (Haptics)

### Technical Problem
Scaling haptic effects (Road Texture, Bumps) by tire load is intended to make the wheel feel "alive" at speed. 
- **The "Chasing Peak" Bug**: If normalized by session peak load, the effect strength at 300 km/h eventually drops back to 1.0x as the peak is reached and held.
- **Goal**: High-speed textures should be significantly stronger than low-speed textures to reflect the increased forces on the tires.

### Comparison of Methods

| Method | Description | Pros | Cons |
| :--- | :--- | :--- | :--- |
| **Global Constant** | Normalize by a fixed 4000N. | Simple. | Feels too weak in Hypercars (High peak) or too strong in Karts (Low peak). |
| **Session Peak** | Normalize by max load seen. | Matches car's range. | **Effect Neutralization** (as discussed). |
| **Static Load Anchoring** | Normalize by corner weight at rest. | High speed (aero) naturally produces factors > 1.0x (e.g. 2.5x). | Initial seeds must be accurate per class. |

### Recommendation for LMUFFB
**Static Load Anchoring** is the superior method for haptics.
- `HapticFactor = CurrentLoad / StaticLoad`.
- `StaticLoad` is learned at low speeds (2-15 m/s) where aero is negligible.
- This results in a natural "force ramp" as the car accelerates, without the risk of the "chasing peak" neutralizing the feel over time.

---

## 3. Implementation Framework for LMUFFB

### Strength (#95)
1.  Add `m_learned_peak_torque` to `FFBEngine`.
2.  In `calculate_force`, update `m_learned_peak_torque` with `max(m_learned_peak_torque, raw_torque)`.
3.  Implement a "Slow Decay" (~0.01 Nm/s) to allow for session restarts or car changes without a full reset if desired, OR reset on car class change.
4.  Apply `StrengthMult = m_max_torque_ref / m_learned_peak_torque` to the `BaseInput`.

### Load (#120)
1.  Already moving to `Static Load` anchoring in the proposed plan.
2.  Refine `m_static_front_load` learning to be more aggressive during the "learning window" (speed 2-15 m/s).

## Conclusion
The proposed plan for #120 (Static Load) is correct. For #95 (Strength), **Dynamic Session Peak** is preferred over a **Fixed Lookup Table** as it handles car setups and variations more accurately, following the iRacing/irFFB industry standard.
