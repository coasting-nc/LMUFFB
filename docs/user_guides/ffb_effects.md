# Force Feedback Effects Guide

> Updated November 2026 (app 0.8.007).

This guide describes what each LMUFFB effect makes you feel and which control adjusts it.

## 1. Understeer (front grip loss)
* **Goal:** tell you when the front tyres are sliding and the car is pushing wide.
* **What you feel:** the steering goes light ("hollow") as front grip runs out. Unwind the wheel and the weight returns.
* **Controls:** **Understeer Effect** (how much lighter) and **Understeer Curve** (how early and how sharply).

## 2. Oversteer (rear grip loss)
* **Goal:** tell you when the rear is stepping out, early enough to catch it.
* **Effects that work together:**
    1. **Cornering Feel:** the car's weight in your hands through the corner.
    2. **Cornering Feel Boost (Slide):** extra weight while the car slides.
    3. **SoP Self-Aligning Torque:** a pull in the counter-steer direction during the slide.
    4. **Rotation Kick:** a short kick at the moment the rear starts to rotate. Raise its **Activation Threshold** if kerbs trigger it.

## 3. Braking lock-up
* **Goal:** warn you when the wheels stop turning under braking, so you can find the limit and avoid flat spots.
* **What you feel:** a vibration that grows as the lock-up gets worse. Its pitch changes with speed: higher at high speed, lower and heavier as the car slows.
* **Controls:** **Lockup Vibration** on or off, **Lockup Strength**, and the lock-up pitch.

## 4. Wheelspin (traction loss)
* **Goal:** tell you when the driven wheels spin under power.
* **What you feel:** **Spin Vibration** adds a hum that rises in pitch as the wheels spin up. It adds rumble only; it does not lighten the steering.
* **Wheelspin Force Reduction** (optional, off by default) also lightens the steering during wheelspin. It can hide useful information while you catch a slide, so leave it at 0% unless you prefer that feel. Older personal profiles keep the amount they had; reapplying an updated built-in profile sets it back to 0%.

## 5. Road and slide textures
* **Slide Texture:** a scrubbing vibration when the tyres slide sideways.
* **Road Texture:** kerbs, bumps and the road surface.

---

## Keeping the signals clear

### 1. One effect hiding another
Strong vibrations (lock-up, road texture) can drown out subtle weight changes (understeer, cornering weight). If you miss the rear stepping out during a heavy lock-up or over kerbs, lower the vibration strengths before raising the forces.

### 2. Running out of force
When everything adds up to more than your wheel can deliver, the force flattens at its maximum and you lose detail. Lower **Master Gain** until the strongest corners no longer hit that ceiling, so the effects still have room.

### 3. Telling lock-up from wheelspin
Lock-up and wheelspin both vibrate. Context usually tells you which is which (braking or accelerating), and their different pitch helps you react without thinking.

### 4. When the game data drops out
If the game briefly stops sending usable data, the app keeps the effects sensible instead of letting them go silent or spike, and shows a warning in the troubleshooting window.

---

## Comparison with iRFFB and Marvin's AIRA

| Effect | iRFFB (iRacing) | Marvin's AIRA (iRacing) | LMUFFB (LMU) |
| :--- | :--- | :--- | :--- |
| **Oversteer** | Lateral weight plus rotation cues | A separate slip layer | A counter-steer pull plus cornering weight and a rotation kick |
| **Lock-up** | Not a separate effect | Often sent to pedal haptics | A vibration on the wheel that grows with the lock-up and follows speed |
| **Wheelspin** | Not a separate effect | Pedal haptics or wheel rumble | A vibration, plus an optional steering lightening (off by default) |
