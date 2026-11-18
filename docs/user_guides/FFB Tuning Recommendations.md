# LMUFFB Tuning Guide: The Physics of Feel

> Updated November 2026 (app 0.8.007). For detailed values and every control, see the step-by-step tuning guide.

This guide gives an order for tuning LMUFFB. Instead of moving controls at random, build the feel in layers: **strength → front axle → rear axle → textures → refinement.**

New installations start on the **Classic Feel** profile, which is a good base. You only need to enter your wheelbase's maximum torque.

---

## Phase 1: Strength

Set the overall strength for your hardware before touching effects.

### The controls
*   **Wheelbase Max Torque (Nm):** your wheel's real maximum. Set it once, to your wheel's specification.
*   **Master Gain:** the main strength control. It scales everything together.
*   **Steering Scale:** how strong the steering and car-motion cues feel compared with vibrations.

### Steps
1.  Set **Wheelbase Max Torque** to your wheel's real value. Do not use it to make the steering lighter or heavier.
2.  Start with a comfortable **Master Gain**.
3.  Drive a high-downforce car (for example a Hypercar) through fast corners (for example the Porsche Curves).
    *   *Goal:* the wheel feels heavy and substantial, but never hits a "wall" where detail disappears.
    *   *If it does:* lower **Master Gain**.

---

## Phase 2: Front axle (grip and connection)

This layer tells you how the front tyres meet the road.

### The controls
*   **Steering Shaft Gain:** how much of the game's steering force you feel.
*   **Understeer Effect** and **Understeer Curve:** how much, how early and how sharply the steering goes light when the front grip runs out.

### Steps
1.  **Isolate:** temporarily set **Cornering Feel** to 0.
2.  **Weight:** adjust **Steering Shaft Gain** until the car feels connected on straights and gentle turns.
3.  **The drop:** enter a corner too fast and turn past the grip limit.
    *   Raise **Understeer Effect** until the steering goes light the moment the car stops turning and starts sliding.
    *   *Goal:* a clear drop that makes you unwind the wheel, without the wheel going completely limp.
    *   Use **Understeer Curve** to make the drop arrive earlier and more gently, or later and sharper.

---

## Phase 3: Rear axle (oversteer and balance)

This layer tells you what the rear of the car is doing. Together these effects create the "catch."

### The controls
*   **Cornering Feel:** the car's weight leaning into corners.
*   **Rotation Kick:** a short warning at the *start* of rotation.
*   **SoP Self-Aligning Torque:** the counter-steer pull *during* the slide.
*   **Cornering Feel Boost (Slide):** extra weight *during* the slide.

### Steps (in this order)
1.  **Cornering Feel (the body):** drive a clean lap. Adjust until you feel the car leaning into corners. It should add weight, not twitchiness.
2.  **SoP Self-Aligning Torque (the direction):** provoke a power slide. Adjust until the wheel turns into the counter-steer by itself and guides your hands to the right angle.
3.  **Cornering Feel Boost (Slide) (the momentum):** add it to make the slide feel substantial. With the pull, this gives a "heavy counter-steer": the wheel pulls the right way and feels like it has mass behind it.
4.  **Rotation Kick (the warning):** adjust until you feel a sharp jolt the moment the rear breaks away. It should be a quick impulse, not a lasting force. Raise **Rotation Kick Activation Threshold** if kerbs trigger it.

---

## Phase 4: Textures (the surface)

Vibrations that sit on top of the forces.

### The controls
*   **Road Texture:** bumps and kerbs.
*   **Slide Texture:** a sandpaper feel when the tyres slide sideways. It works at low speed too.
*   **Scrub Drag:** a steady resistance while sliding.

### Steps
1.  **Road Texture:** drive over kerbs. Raise it until you feel the impact, without rattling on straights.
2.  **Slide Texture:** provoke understeer or oversteer. Adjust until you feel a gritty grinding that is clearly different from road bumps.
3.  **Scrub Drag (optional):** raise it for a thicker resistance when sliding sideways.

---

## Phase 5: Pedal feel through the wheel

### The controls
*   **Lockup Vibration:** when the wheels stop turning under braking.
*   **ABS Pulse:** when ABS works.
*   **Spin Vibration:** when the driven wheels spin up under power.

### Steps
1.  **Lock-up:** brake hard with ABS off. Adjust until the vibration makes you release the brake.
2.  **Spin:** floor the throttle in 1st gear. Adjust until you feel the rising hum.

---

## Phase 6: Refinement

The final polish for your hardware.

### The controls
*   **Steering Smoothing:** softens a grainy steering feel. More smoothing makes the steering react later.
*   **Smart Hands-off Protection:** resistance against fast steering movements at speed.
*   **Stationary Damping:** weight when the car is stopped or slow.

### Steps
1.  **Smoothing:** keep it as low as possible. Lower it until the wheel feels grainy, then raise it just enough to make it smooth again. Direct-drive wheels usually need less than belt or gear wheels.
2.  **Stability:** if the wheel oscillates on straights, or snaps too violently when you catch a slide, raise **Smart Hands-off Protection**. The wheel should feel fluid during fast movements, not springy.
3.  **Standing still:** if the wheel shakes or feels loose in the pits, adjust **Stationary Damping**.

---

## Summary checklist

| Step | Goal | Main control | You got it right when |
| :--- | :--- | :--- | :--- |
| **1** | **Strength** | `Master Gain` | Strong forces without hitting a wall. |
| **2** | **Front feel** | `Understeer Effect` | The wheel goes light when you push too hard. |
| **3** | **Body** | `Cornering Feel` | The wheel feels heavy in corners. |
| **4** | **Slide catch** | `SoP Self-Aligning Torque` | The wheel turns into the counter-steer by itself. |
| **5** | **Slide weight** | `Cornering Feel Boost (Slide)` | The counter-steer feels substantial. |
| **6** | **Slide onset** | `Rotation Kick` | A sharp jolt when traction breaks. |
| **7** | **Texture** | `Slide Texture` | A gritty vibration during slides. |
| **8** | **Stability** | `Smart Hands-off Protection` | No oscillation on straights or catches. |

---

## Blending the slide cues

**SoP Self-Aligning Torque**, **Cornering Feel Boost (Slide)** and **Rotation Kick** work together during a slide.

### What each one tells you
1.  **Rotation Kick:** "the rear is starting to go." It comes first, as a short impulse.
2.  **SoP Self-Aligning Torque:** "turn the wheel *this* way to catch it." It follows quickly, as a pull.
3.  **Cornering Feel Boost (Slide):** "the car has a lot of momentum." It builds a little more slowly, as weight.

The aim is a clear sequence: **kick (onset) → pull (direction) → weight (momentum)**.

### Balancing them
*   **Too much Cornering Feel Boost (Slide):** the wheel feels like a brick and hides the pull.
*   **Too much SoP Self-Aligning Torque:** the wheel snaps violently but feels light and "digital."
*   **Keep textures separate:** the pull and the weight are smooth, steady forces; **Slide Texture** is the vibration on top. Keep the forces smooth so they stay distinct from the textures.

### Recommended order
1.  Tune **SoP Self-Aligning Torque** first, until the counter-steer pull is clear and responsive.
2.  Then add **Cornering Feel Boost (Slide)** gradually, without making the wheel too heavy to turn.
3.  Use **Rotation Kick** for the early warning: short and sharp, at the very start of the slide.
4.  If the strongest moments hit your wheel's maximum, lower **Master Gain** so every cue keeps room.
