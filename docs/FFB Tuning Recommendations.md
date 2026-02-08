# LMUFFB Tuning Guide: The Physics of Feel
> **Current Version:** v0.7.5

This guide provides a systematic approach to tuning Force Feedback in LMUFFB. Instead of randomly moving sliders, follow this sequence to build a cohesive and informative force feedback profile.

The philosophy is to build the signal in layers: **Calibration $\to$ Front Axle $\to$ Rear Axle $\to$ Textures $\to$ Refinement.**

---

## Phase 1: The Foundation (Calibration)

Before adding effects, we must ensure the signal strength is correct for your hardware.

### The Settings
*   **Max Torque Ref (Nm):** The "Calibration Scale." It tells the app how strong the game's physics are.
*   **Master Gain:** The "Volume Knob." It scales the final output sent to your wheel.

### Tuning Steps
1.  **Set Baseline:** Set `Master Gain` to **1.0** (100%).
2.  **Set Reference:** Set `Max Torque Ref` to around 80-100 Nm (lower values make the FFB to strong and cause clipping). This value represents the max forces that the car can generate (now just on the steering rack). This is a bit unintuitive because it is not a setting based on your wheel Nm output. This will be changed in a future version to make it more intuitive.

3.  **Drive & Adjust:** Drive a high-downforce car (e.g., Hypercar) through high-speed corners (e.g., Porsche Curves).
    *   *Goal:* The wheel should feel heavy and substantial, but **not** hit a "wall" of force where you lose detail.
    *   *Check:* Open the **Troubleshooting Graphs**. If the "Clipping" graph hits 1.0 frequently, **increase** `Max Torque Ref`.

---

## Phase 2: The Front Axle (Grip & Connection)

This layer communicates the connection between the front tires and the road.

### The Settings
*   **Steering Shaft Gain:** The raw aligning torque from the game physics.
*   **Understeer Effect:** A modifier that *reduces* force when front grip is lost.

### Tuning Steps
1.  **Isolate:** Temporarily set `SoP Lateral G` to 0.0.
2.  **Tune Weight:** Adjust `Steering Shaft Gain` until the car feels connected driving straight and turning slightly.
3.  **Tune the Drop:** Drive into a corner too fast and turn the wheel past the grip limit (scrub the front tires).
    *   **Adjust `Understeer Effect`:** Increase this slider until you feel the steering wheel go **light** or "hollow" the moment the car stops turning and starts sliding.
    *   *Criteria:* You want a clear drop in weight that prompts you to unwind the wheel, but not so much that the wheel goes completely limp.

    *   *Criteria:* You want a clear drop in weight that prompts you to unwind the wheel, but not so much that the wheel goes completely limp.

### Advanced: Slope Detection (v0.7.3)
For Direct Drive users who want more dynamic feedback, you can enable **Slope Detection**. This replaces the static "Understeer Effect" logic.
*   **What it does:** Monitors the tire's physics curve ($dG/d\alpha$). When the tire hits peak load and starts to scrub, it dynamically reduces force.
*   **Tuning the feel:**
    *   **Sensitivity:** Controls how abruptly the force drops. Lower (0.5) is smoother; Higher (1.0) is sharper.
    *   **Decay Rate:** Controls how fast the force returns to normal on straights. Default (5.0) is usually best.
    *   **Confidence Gate:** Keep this **ON**. It prevents false triggers when you aren't really pushing the car.
*   **Warning:** This automatically disables "Lateral G Boost" to prevent feedback loops.

---

## Phase 3: The Rear Axle (Oversteer & Balance)

This layer communicates what the rear of the car is doing. These effects interact to create the "Catch" sensation.

### The Settings
*   **SoP Lateral G:** Simulates chassis roll/weight transfer.
*   **Yaw Kick:** Predictive impulse at the *start* of rotation.
*   **Rear Align Torque:** Geometric counter-steering pull *during* the slide.
*   **Lateral G Boost (Slide):** Adds weight/inertia *during* the slide.

### Tuning Steps (The Sequence)
1.  **SoP Lateral G (The Body):** Drive a clean lap. Adjust this until you feel the "weight" of the car leaning into corners. It should add heaviness, not twitchiness.
2.  **Rear Align Torque (The Direction):** Induce a slide (power oversteer).
    *   Adjust until the wheel actively **spins** in the counter-steer direction.
    *   *Criteria:* The wheel should guide your hands to the correct angle to catch the slide.
3.  **Lateral G Boost (Slide) (The Momentum):**
    *   Increase this to add "heaviness" to the slide.
    *   *Interaction:* Combined with Rear Align, this creates a "Heavy Counter-Steer" feel. The wheel pulls correctly (Rear Align) but feels like it has mass behind it (Boost).
4.  **SoP Yaw Kick (The Warning):**
    *   Adjust this to feel a sharp "jolt" the exact millisecond the rear tires break traction.
    *   *Criteria:* This is your early warning system. It should be a quick impulse, not a sustained force.

---

## Phase 4: Textures & Immersion (The Surface)

These are high-frequency vibrations that sit "on top" of the forces.

### The Settings
*   **Road Texture:** Vertical bumps and curbs.
*   **Slide Texture:** Lateral scrubbing vibration (Sandpaper feel). **Note:** Works even at low speeds (exempt from Speed Gate).
*   **Scrub Drag:** Constant resistance (friction) when sliding.

### Tuning Steps
1.  **Road Texture:** Drive over curbs. Increase until you feel the impact, but ensure it doesn't rattle your teeth on straights.
    *   *Tip:* Use `Load Cap` (General Settings) if curbs are too violent in high-speed corners.
2.  **Slide Texture:** Induce understeer or oversteer.
    *   Adjust until you feel a gritty "grinding" sensation.
    *   *Criteria:* This confirms the tires are sliding. It should be distinct from road bumps.
3.  **Scrub Drag:** (Optional) Increase to add a "thick" resistance when sliding sideways. This mimics the friction of rubber dragging across asphalt.

---

## Phase 5: Haptics (Pedals on Wheel)

These simulate pedal feel through the wheel rim.

### The Settings
*   **Lockup Vibration:** Triggers when wheels stop rotating under braking.
*   **Spin Vibration:** Triggers when rear wheels spin up under power.

### Tuning Steps
1.  **Lockup:** Brake hard (without ABS). Adjust until the vibration scares you into releasing the brake.
2.  **Spin:** Mash the throttle in 1st gear. Adjust until you feel the "revving" vibration.

---

## Phase 6: Refinement (Signal Conditioning)

The final polish to match your specific hardware capabilities.

### The Settings
*   **SoP Smoothing:** Filters the Lateral G signal.
*   **Slip Angle Smoothing:** Filters the tire physics calculation.
*   **Gyroscopic Damping:** Adds resistance to rapid movements.

### Tuning Steps
1.  **Smoothing (Latency vs. Noise):**
    *   **Direct Drive:** Aim for **Low Latency** (Green text). Set SoP Smoothing to ~0.90 and Slip Smoothing to ~0.005.
    *   **Belt/Gear:** Aim for **Medium Latency**. Set SoP Smoothing to ~0.60 and Slip Smoothing to ~0.030.
    *   *Criteria:* Lower the smoothing until the wheel feels "grainy" or "robotic," then raise it just enough to make it smooth again.
    *   If the wheel oscillates (wobbles left/right) on straights or snaps too violently when catching a slide ("Tank Slapper"), **increase** Gyro Damping.
    *   *Criteria:* The wheel should feel "viscous" or fluid-like during rapid movements, not like a spring.
3.  **Speed Gate (Oscillation Prevention):**
    *   Limits forces at very low speeds (< 18 km/h) to prevent violent shaking when stopped.
    *   **Defaults:** 1.0 m/s (start) to 5.0 m/s (full).
    *   *Tip:* If you feel a "dead zone" when leaving the pits, try lowering the Upper Limit (e.g., to 3.0 m/s). If the wheel shakes when stopped, raise the Lower Limit.

---

## Summary Checklist

| Step | Goal | Primary Control | Success Criteria |
| :--- | :--- | :--- | :--- |
| **1** | **Calibrate** | `Max Torque Ref` | Strong forces without constant clipping. |
| **2** | **Front Feel** | `Understeer Effect` | Wheel goes light when pushing too hard. |
| **3** | **Body Roll** | `SoP Lateral G` | Wheel feels heavy in corners. |
| **4** | **Slide Catch** | `Rear Align Torque` | Wheel spins to counter-steer automatically. |
| **5** | **Slide Weight** | `Lat G Boost (Slide)` | Counter-steer feels heavy/substantial. |
| **6** | **Slide Onset** | `Yaw Kick` | Sharp jolt when traction breaks. |
| **7** | **Texture** | `Slide Texture` | Gritty vibration during slides. |
| **8** | **Stability** | `Gyro Damping` | No oscillation on straights or catches. |

---

## Rear Align Torque, Lateral G Boost (Slide), and Yaw Kick

The interaction between "Lateral G Boost (Slide)" (formerly Oversteer Weight) and "Rear Align Torque" is crucial for a natural and intuitive oversteer feel.

### 1. The Interaction: "Heavy Counter-Steer"

When both effects are present during a slide, they combine to create a sensation of **"Heavy Counter-Steer."**

*   **Lateral G Boost (Slide):** This effect makes the wheel feel **heavier** (more resistance) as the car's body swings out. It's like feeling the inertia of the chassis through the steering.
*   **Rear Align Torque:** This effect provides a **directional pull** in the counter-steering direction. It's the wheel actively trying to straighten itself or align with the direction of travel.

**When they combine:**
Instead of the wheel going light and vague (which happens in some sims during a slide), it becomes **heavy and pulls strongly** in the direction you need to counter-steer.

**Are they confusing or well-blended?**
If tuned correctly, they are **well-blended and complementary**. They provide two distinct but synergistic pieces of information:

1.  **"I am sliding, and the car has a lot of momentum."** (Lateral G Boost)
2.  **"Turn the wheel THIS WAY to catch it."** (Rear Align Torque)

This combination is highly informative and is often praised in sims like Assetto Corsa for making slides "catchable."

### 2. Criteria for a Natural and Intuitive Blend

To make this blend feel natural, we need to consider the **magnitude, timing, and frequency** of each component.

#### A. Magnitude Balance (The "Volume Knob")
*   **Problem:** If one effect is too strong, it can mask the other.
    *   Too much **Lateral G Boost**: The wheel feels like a brick, and you can't feel the subtle directional pull of the Rear Align Torque.
    *   Too much **Rear Align Torque**: The wheel snaps violently, but it feels "light" or "digital" because it lacks the inertia of the chassis.
*   **Criteria:**
    *   **Rear Align Torque** should be strong enough to provide a clear, active counter-steering cue.
    *   **Lateral G Boost** should add a layer of "weight" or "inertia" on top, making the counter-steer feel substantial, but not so much that it becomes a struggle to turn the wheel.
*   **Tuning Goal:** The driver should feel the *direction* of the counter-steer (Rear Align) and the *effort* required to hold it (Lateral G Boost).

#### B. Timing (The "Predictive Cue")
*   **Problem:** If both effects kick in at the exact same time, they might feel like one undifferentiated "blob" of force.
*   **Criteria:**
    *   **Yaw Kick (already implemented):** This is the *earliest* cue. It's a sharp, momentary impulse that signals the *onset* of rotation.
    *   **Rear Align Torque:** Should build up very quickly after the Yaw Kick, as the slip angle develops. This is the active "pull."
    *   **Lateral G Boost (Slide):** Should build up slightly more gradually, reflecting the inertia of the car's mass swinging out. It's a sustained force that tells you about the *magnitude* of the slide.
*   **Tuning Goal:** A sequence of cues: **Kick (onset) $\to$ Pull (direction) $\to$ Weight (momentum)**.

#### C. Frequency (The "Texture")
*   **Problem:** If both effects use similar frequencies, they can interfere.
*   **Criteria:**
    *   **Rear Align Torque:** This is a **low-frequency, sustained force**. It's a steady pull, not a vibration.
    *   **Lateral G Boost (Slide):** This is also a **low-frequency, sustained force**. It's a steady increase in resistance.
    *   **Slide Texture (separate effect):** This is a **high-frequency vibration** (the "sandpaper" feel). This is crucial for adding texture without interfering with the directional forces.
*   **Tuning Goal:** Keep the directional forces (Lateral G, Rear Align) smooth and distinct from the high-frequency textures.

### 3. Tuning Recommendations for the User

To achieve a natural blend, users should:

1.  **Start with "Rear Align Torque" first:** Tune this until the counter-steering pull feels clear and responsive.
2.  **Then add "Lateral G Boost (Slide)":** Increase this gradually to add the sensation of chassis momentum without making the wheel too heavy to turn.
3.  **Use "Yaw Kick" for early warning:** This should be a sharp, short impulse at the very start of the slide.
4.  **Monitor "Clipping":** If the total force is clipping, reduce `Master Gain` or increase `Max Torque Ref` to ensure all these distinct forces have headroom.

By understanding these individual roles and their combined effect, the user can tune a highly informative and intuitive oversteer experience.


### Tuning Tips for Rear Align Torque, Lateral G Boost (Slide), and Yaw Kick

To achieve a natural blend, users should:

1.  **Start with "Rear Align Torque" first:** Tune this until the counter-steering pull feels clear and responsive.
2.  **Then add "Lateral G Boost (Slide)":** Increase this gradually to add the sensation of chassis momentum without making the wheel too heavy to turn.
3.  **Use "Yaw Kick" for early warning:** This should be a sharp, short impulse at the very start of the slide.
4.  **Monitor "Clipping":** If the total force is clipping, reduce `Master Gain` or increase `Max Torque Ref` to ensure all these distinct forces have headroom.

By understanding these individual roles and their combined effect, the user can tune a highly informative and intuitive oversteer experience.
