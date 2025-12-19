## **Driver's Guide to Testing LMUFFB**

### 🏁 Prerequisites

**Car/Track Choice:**
*   **Car:** **Porsche 911 GTE/GT3** is excellent. Because the engine is in the back, the front is light (easy to understeer) and the rear acts like a pendulum (clear oversteer).
*   **Track:** **Paul Ricard** is perfect because it is flat (no elevation changes to confuse you) and has massive run-off areas so you can spin safely.
    *   *Tip:* Use the **"Mistral Straight"** (the long one) for high-speed tests.
    *   *Tip:* Use the **last corner (Virage du Pont)** for low-speed traction tests.

**Global Setup:**
1.  **In-Game (LMU):** FFB Strength 0%, Smoothing 0.
2.  **Wheel Driver:** Set your physical wheel strength to **20-30%** (Safety first!).
3.  **LMUFFB:** Start with the **"Default"** preset, then modify as instructed below.

---

### 1. Understeer (Front Grip Loss)

**What is it?** The front tires are sliding. The car won't turn as much as you are turning the wheel.
**The Goal:** The steering should go **LIGHT** (lose weight) to tell you "Stop turning, you have no grip!"

**Quick Setup (Preset):**
*   Load Preset: **"Guide: Understeer (Front Grip)"**

**Car Setup:**
*   **Brake Bias:** Move forward (e.g., 60%) to overload front tires.
*   **Front ARB (Anti-Roll Bar):** Stiff (Max).

**The Test:**
1.  Drive at moderate speed (100 km/h).
2.  Turn into a medium corner (e.g., Turn 1).
3.  **Intentionally turn too much.** Turn the wheel 90 degrees or more, past the point where the car actually turns.
4.  **What to feel:**
    *   *Normal:* Resistance builds up as you turn.
    *   *The Cue:* Suddenly, the resistance **stops increasing** or even **drops**. The wheel feels "hollow" or "disconnected."
    *   *Correct Behavior:* If you unwind the wheel (straighten slightly), the weight returns.

---

### 2. Oversteer (Rear Grip Loss)

**What is it?** The rear tires are sliding. The back of the car is trying to overtake the front.
**The Goal:** The wheel should **PULL** against the turn (Counter-Steer). It wants to fix the slide for you.

**Quick Setup (Preset):**
*   Load Preset: **"Guide: Oversteer (Rear Grip)"**

**Car Setup:**
*   **Traction Control (TC):** **OFF** (Crucial).
*   **Rear ARB:** Stiff.

**The Test:**
1.  Take a slow 2nd gear corner.
2.  Mid-corner, **mash the throttle 100%**.
3.  The rear will kick out.
4.  **What to feel:**
    *   *The Cue:* The steering wheel violently snaps in the **opposite direction** of the turn. If you are turning Left, the wheel rips to the Right.
    *   *Correct Behavior:* If you let go of the wheel, it should spin to align with the road (self-correcting).
    *   *Bug Check:* If the wheel pulls *into* the turn (making you spin faster), the "Inverted Force" bug is present.

---

### 3. Slide Texture (Scrubbing)

**What is it?** The tires are dragging sideways across the asphalt.
**The Goal:** A "grinding" or "sandpaper" vibration.

**Quick Setup (Preset):**
*   Load Preset: **"Guide: Slide Texture (Scrub)"**

**The Test:**
1.  Go to a wide runoff area.
2.  Turn the wheel fully to one side and accelerate to do a "donut" or a heavy understeer plow.
3.  **What to feel:**
    *   *The Cue:* A distinct, gritty vibration.
    *   *Physics Check:* The vibration pitch (frequency) should get **higher** as you slide faster.
        *   Slow slide = "Grrr-grrr" (Low rumble).
        *   Fast slide = "Zzzzzzz" (High buzz).

---

### 4. Braking Lockup

**What is it?** You braked too hard. The wheel stopped spinning, but the car is moving. You are burning a flat spot on the tire.
**The Goal:** A violent, jarring vibration to tell you to release the brake.

**Quick Setup (Preset):**
*   Load Preset: **"Guide: Braking Lockup"**

**Car Setup:**
*   **ABS:** **OFF** (Crucial).

**The Test:**
1.  Drive fast (200 km/h) down the Mistral Straight.
2.  Stomp the brake pedal **100%**.
3.  **What to feel:**
    *   *The Cue:* The wheel shakes violently.
    *   *Physics Check:* As the car slows down, the shaking should get **slower and heavier** (thump-thump-thump) because the "scrubbing speed" is decreasing.

---

### 5. Traction Loss (Wheel Spin)

**What is it?** You accelerated too hard. The rear wheels are spinning freely (burnout).
**The Goal:** The steering feels "light" and "floaty" combined with a high-frequency engine-like vibe.

**Quick Setup (Preset):**
*   Load Preset: **"Guide: Traction Loss (Spin)"**

**Car Setup:**
*   **TC:** **OFF**.

**The Test:**
1.  Stop the car. Put it in 1st gear.
2.  Hold the brake and throttle (Launch Control style) or just mash throttle.
3.  **What to feel:**
    *   *The Cue:* The steering weight suddenly disappears (Torque Drop). It feels like the car is floating on ice.
    *   *The Vibe:* A high-pitched hum/whine that rises as the RPM/Wheel Speed rises.


---

### 6. SoP Yaw (The Kick)

**What is it?** A predictive impulse based on **Yaw Acceleration** (how fast the car *starts* to rotate). Unlike Lateral G (which is a sustained weight), this is a momentary "kick" or "jolt".
**The Goal:** To signal the exact moment the rear tires break traction, often before the visual slide is apparent.

**UI Settings (Isolation):**
*   **Preset:** `Guide: SoP Yaw (Kick)`
*   **Master Gain:** `1.0`
*   **SoP Yaw (Kick):** `2.0` (Max)
*   **Base Force Mode:** `Muted` (Crucial to feel the isolated kick)

**The Test:**
1.  Drive at moderate speed (3rd gear) on a straight.
2.  Perform a **Scandinavian Flick**: Turn sharply Left, then immediately whip the wheel Right to destabilize the rear.
3.  **What to feel:**
    *   *The Cue:* At the exact moment the car starts to rotate (yaw), you should feel a sharp **jolt** or **tug** in the steering wheel towards the counter-steer direction.
    *   *Physics Check:* Drive in a steady circle (constant yaw rate). You should feel **nothing**. Now stomp the gas to spin. You should feel the **kick**. (Acceleration vs Velocity).

---

### 7. Gyroscopic Damping (Stability)

**What is it?** A resistance force that opposes rapid steering movements. It simulates the gyroscopic inertia of the spinning front wheels.
**The Goal:** To prevent "Tank Slappers" (oscillation) and give the steering a sensation of weight/viscosity that scales with speed.

**UI Settings (Isolation):**
*   **Preset:** `Guide: Gyroscopic Damping`
*   **Master Gain:** `1.0`
*   **Gyroscopic Damping:** `1.0` (Max)
*   **Base Force Mode:** `Muted` (Crucial: The wheel will have no self-aligning torque, only resistance).

**The Test:**
1.  **Stationary Test:** Sit in the pits (0 km/h). Wiggle the wheel left and right.
    *   *Result:* It should feel light/easy (No gyro effect).
2.  **Speed Test:** Drive down the straight at **250 km/h**.
3.  **The Wiggle:** Wiggle the wheel left and right quickly (small inputs).
    *   *The Cue:* The wheel should feel **thick**, **heavy**, or **viscous**. It resists your rapid movements.
    *   *Physics Check:* Turn the wheel *slowly*. It should feel lighter. Turn it *fast*. It should fight you. This velocity-dependent damping is what stabilizes the car.

---

---

### 8. Corner Entry (Weight Transfer & Loading)

**What is it?** The sensation of the steering wheel getting heavier as you brake and turn in, transferring the car's weight onto the front tires.
**The Goal:** To confirm that the steering rack force (Base Force) accurately communicates the increased load on the front axle before the limit is reached.

**UI Settings (Isolation):**
*   **Master Gain:** `1.0`
*   **Steering Shaft Gain:** `1.0`
*   **Base Force Mode:** `Native` (Crucial: We need the game's physics alignment torque).
*   **Understeer Effect:** `0.0` (Disable to feel the raw build-up).
*   **SoP / Textures:** `0.0`

**The Test:**
1.  Drive at high speed on a straight.
2.  Brake hard and turn in smoothly (Trail Braking).
3.  **What to feel:**
    *   *The Cue:* The steering weight should **increase** significantly as the nose dives and the car rotates. It should feel "planted" and heavy.
    *   *Diagnosis:* If the wheel feels light or static during turn-in, the game's base physics might be numb.
    *   *Fix:* If the game is numb, we currently rely on `SoP (Lateral G)` to add this weight artificially. Try increasing `SoP Effect`.

---

### 9. Mid-Corner Limit (The "Throb")

**What is it?** A specific vibration texture that appears *exactly* when the front tires reach their peak slip angle, just before they start to slide/understeer.
**The Goal:** To provide a tactile warning that you are at the limit of grip, allowing you to balance the car on the edge.

**UI Settings (Isolation):**
*   **Master Gain:** `1.0`
*   **Slide Rumble:** **Checked**
*   **Slide Gain:** `1.0`
*   **Understeer Effect:** `0.5` (To feel the weight drop *after* the throb).

**The Test:**
1.  Take a long, constant-radius corner (e.g., a carousel).
2.  Gradually increase steering angle until you hear the tires just starting to scrub.
3.  **What to feel:**
    *   *The Cue:* A distinct, rhythmic vibration ("Throb" or "Grinding") should start.
    *   *The Sequence:* Grip (Silent) -> Limit (Throb/Vibration) -> Understeer (Lightness/Silence).
    *   *Tuning:* If the vibration starts too late (after you are already sliding), lower the `Optimal Slip Angle` threshold in the code (currently fixed at 0.10 rad) or increase `Slide Gain`.

---

### 10. ABS Threshold (The "Rattle")

**What is it?** A pulsing vibration that mimics the ABS pump releasing brake pressure when the wheel is about to lock.
**The Goal:** To allow the driver to mash the brake pedal and feel exactly where the threshold is without looking at a HUD.

**UI Settings (Isolation):**
*   **Master Gain:** `1.0`
*   **Progressive Lockup:** **Checked**
*   **Lockup Gain:** `1.0`
*   **Base Force Mode:** `Muted` (To isolate the vibration).

**Car Setup:**
*   **ABS:** **ON** (Set to a high intervention level).

**The Test:**
1.  Drive fast.
2.  Stomp the brake pedal 100%.
3.  **What to feel:**
    *   *The Cue:* A rapid, mechanical rattling or pulsing vibration.
    *   *Physics Check:* Since ABS prevents full lockup, the `Slip Ratio` will oscillate rapidly. The FFB should reflect this with a "Rattle" rather than a continuous "Screech."

---

### 🛠️ Troubleshooting Cheat Sheet

| Symptom | Diagnosis | Fix |
| :--- | :--- | :--- |
| **Wheel feels dead/numb in corners** | SoP is too low or Understeer is too aggressive. | Increase `SoP (Lateral G)`. Decrease `Understeer`. |
| **Wheel oscillates (shakes L/R) on straights** | Latency or too much Min Force. | Increase `SoP Smoothing`. Decrease `Min Force`. |
| **Wheel pulls the wrong way in a slide** | Inverted Physics. | Check `Invert FFB Signal` or report bug (Yaw/Scrub inversion). |
| **No Road Texture over curbs** | Suspension frequency mismatch. | Increase `Road Gain`. Ensure `Load Cap` isn't too low. |
| **Effects feel "Digital" (On/Off)** | Clipping. | Check the "Clipping" bar in Debug window. Reduce `Master Gain` or increase `Max Torque Ref`. |

### References

* `https://www.youtube.com/watch?v=XHSEAMQgN2c&t=655s`
