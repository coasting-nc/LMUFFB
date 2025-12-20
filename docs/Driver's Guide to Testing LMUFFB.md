## **Driver's Guide to Testing LMUFFB**

### 🏁 Prerequisites

**Car/Track Choice:**
*   **Car:** **Porsche 911 GTE** is the best reference. The rear-engine layout acts like a pendulum, making oversteer very clear, while the light front end makes understeer distinct.
*   **Track:** **Paul Ricard** is ideal. It is perfectly flat (no elevation changes to confuse the FFB) and has massive asphalt run-off areas for safe spinning.
    *   *Tip:* Use the **"Mistral Straight"** for high-speed tests.
    *   *Tip:* Use the **last corner (Virage du Pont)** for low-speed traction tests.

**Global Setup:**
1.  **In-Game (LMU):** FFB Strength 0%, Smoothing 0.
2.  **Wheel Driver:** Set your physical wheel strength to **20-30%** (Safety first!).
3.  **LMUFFB:** Start with the **"Default (T300)"** preset, then modify as instructed below.

---

### 1. Understeer (Front Grip Loss)

**What is it?** The front tires are sliding. The car won't turn as much as you are turning the wheel.
**The Goal:** The steering should go **LIGHT** (lose weight) to tell you "Stop turning, you have no grip!"

**Quick Setup (Preset):**
*   Load Preset: **"Guide: Understeer (Front Grip)"**

**Extreme Car Setup:**
*   **Brake Bias:** **Max Forward (e.g., 70-80%)**. This ensures the front tires lock or overload immediately when you touch the brakes.
*   **Front Springs & ARB:** **Maximum Stiffness**. This reduces mechanical grip at the front.
*   **Rear Springs & ARB:** **Minimum Stiffness (Soft)**. This glues the rear to the road, forcing the car to push (plow) straight.
*   **Front Tire Pressure:** **Maximum**. Reduces the contact patch size.
*   **Rear Wing:** **Maximum**. Keeps the rear planted.

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

**Extreme Car Setup:**
*   **Traction Control (TC):** **OFF** (Crucial).
*   **Rear Springs & ARB:** **Maximum Stiffness**. This drastically reduces rear grip.
*   **Front Springs & ARB:** **Minimum Stiffness (Soft)**. This gives the front endless grip, ensuring the rear breaks first.
*   **Rear Ride Height:** **Maximum**. Raises the Center of Gravity, making the car unstable.
*   **Rear Wing:** **Minimum (P1)**. Removes aerodynamic grip.
*   **Differential Preload:** **Maximum**. Makes the rear wheels lock together, causing them to break traction easily in tight turns.

**The Test:**
1.  Take a slow 2nd gear corner.
2.  Mid-corner, **mash the throttle 100%**.
3.  The rear will kick out immediately.
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

**Extreme Car Setup:**
*   **Traction Control (TC):** **OFF**.
*   **Tire Pressures:** **Maximum** (Highest allowed). Hard, inflated tires slide easier and transmit vibration more sharply than soft tires.
*   **Differential Preload:** **Maximum**. A locked differential forces the tires to scrub and drag sideways during tight turns.
*   **Suspension:** **Stiffest (Max)** on both Front and Rear to reduce mechanical grip and induce sliding immediately.

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

**Extreme Car Setup:**
*   **ABS:** **OFF** (Crucial).
*   **Brake Bias:** **Extreme Forward (e.g., 75-80%)**. This guarantees the front wheels lock up long before the rears, making the test predictable.
*   **Front Tire Pressure:** **Maximum**. Less grip means easier locking.

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

**Extreme Car Setup:**
*   **TC:** **OFF**.
*   **Rear Tire Pressure:** **Maximum**. Turns the tires into hard plastic, making wheelspin effortless.
*   **Rear Springs:** **Maximum Stiffness**.
*   **Differential Preload:** **Maximum**. Ensures both rear wheels spin up together instantly.

**The Test:**
1.  Stop the car. Put it in 1st gear.
2.  Hold the brake and throttle (Launch Control style) or just mash throttle.
3.  **What to feel:**
    *   *The Cue:* The steering weight suddenly disappears (Torque Drop). It feels like the car is floating on ice.
    *   *The Vibe:* A high-pitched hum/whine that rises as the RPM/Wheel Speed rises.

---

### 6. SoP Yaw (The Kick)

**What is it?** A predictive impulse based on **Yaw Acceleration** (how fast the car *starts* to rotate). Unlike Lateral G (which is a sustained weight), this is a momentary "kick" or "jolt".
**The Goal:** To signal the exact moment the rear tires break traction.

**Quick Setup (Preset):**
*   Load Preset: **"Guide: SoP Yaw (Kick)"**

**Extreme Car Setup:**
*   **TC:** **OFF**.
*   **Brake Bias:** **Extreme Rearward (e.g., 40%)**. This makes the car incredibly unstable under braking.
*   **Rear Ride Height:** **Maximum**. Makes the car "tippy" and prone to snapping.
*   **Front Springs:** **Soft**. Allows the nose to dive, lightening the rear further.

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

**Quick Setup (Preset):**
*   Load Preset: **"Guide: Gyroscopic Damping"**

**Extreme Car Setup:**
*   **Aero:** **Minimum (Low Drag)**. To achieve the highest possible top speed on the straight.
*   **Caster:** **Maximum**. High caster creates strong self-aligning torque, which makes the need for damping more obvious to prevent oscillation.

**The Test:**
1.  **Stationary Test:** Sit in the pits (0 km/h). Wiggle the wheel left and right.
    *   *Result:* It should feel light/easy (No gyro effect).
2.  **Speed Test:** Drive down the straight at **250 km/h**.
3.  **The Wiggle:** Wiggle the wheel left and right quickly (small inputs).
    *   *The Cue:* The wheel should feel **thick**, **heavy**, or **viscous**. It resists your rapid movements.
    *   *Physics Check:* Turn the wheel *slowly*. It should feel lighter. Turn it *fast*. It should fight you. This velocity-dependent damping is what stabilizes the car.

---

### 8. Corner Entry (Weight Transfer & Loading)

**What is it?** The sensation of the steering wheel getting heavier as you brake and turn in, transferring the car's weight onto the front tires.
**The Goal:** To confirm that the steering rack force (Base Force) accurately communicates the increased load on the front axle.

**UI Settings (Isolation):**
*   **Master Gain:** `1.0`
*   **Steering Shaft Gain:** `1.0`
*   **Base Force Mode:** `Native`
*   **Understeer Effect:** `0.0`
*   **SoP / Textures:** `0.0`

**Extreme Car Setup:**
*   **Front Springs:** **Minimum Stiffness (Soft)**. Allows the nose to dive significantly under braking.
*   **Front Bump Dampers:** **Soft**. Allows fast weight transfer.
*   **Brake Bias:** **Forward**.

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
**The Goal:** To provide a tactile warning that you are at the limit of grip, allowing you to balance the car on the edge..

**UI Settings (Isolation):**
*   **Master Gain:** `1.0`
*   **Slide Rumble:** **Checked**
*   **Slide Gain:** `1.0`
*   **Understeer Effect:** `0.5`

**Extreme Car Setup:**
*   **Tire Pressures:** **High**. Makes the tire limit sharper and less forgiving.
*   **Aero:** **High**. Allows you to sustain high cornering speeds to hold the slip angle.

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

**Quick Setup (Preset):**
*   Load Preset: **"Guide: Braking Lockup"** (Note: This preset usually isolates lockup, but for this test, we want to feel the ABS pulse).

**Extreme Car Setup:**
*   **ABS:** **ON (Set to High / Max Intervention)**.
*   **Brake Pressure:** **100%**.
*   **Tire Pressures:** **Maximum**. Low grip ensures ABS triggers instantly.

**The Test:**
1.  Drive fast.
2.  Stomp the brake pedal 100%.
3.  **What to feel:**
    *   *The Cue:* A rapid, mechanical rattling or pulsing vibration.
    *   *Physics Check:* Since ABS prevents full lockup, the `Slip Ratio` will oscillate rapidly. The FFB should reflect this with a "Rattle" rather than a continuous "Screech."
    
---

### Effects currently missing in lmuFFB v0.4.25

Does LMUFFB produce all the effects described in this video `https://www.youtube.com/watch?v=XHSEAMQgN2c`?

**1. The "Brutal Counter-Steer" (SoP/Yaw): ✅ YES**
*   **Video:** Describes a force that "whips the hand off the wheel" the moment the rear steps out.
*   **LMUFFB:** We produce this via three combined effects:
    *   **SoP (Lateral G):** Provides the sustained weight.
    *   **Rear Aligning Torque:** Provides the geometric counter-steer force (which LMU 1.2 lacks natively).
    *   **Yaw Kick (`m_sop_yaw_gain`):** Provides the *derivative* "Kick" or "Whip" based on rotational acceleration. This specifically addresses the "immediacy" the author complains is missing in AC Evo.

**2. The "Throb" at the Limit (Texture): ✅ YES**
*   **Video:** Describes a vibration that indicates the limit before the slide.
*   **LMUFFB:** We produce this via **Slide Texture**.
    *   Our implementation uses `mLateralPatchVel` (Scrubbing Speed) and a **Sawtooth Wave**. This creates exactly the "grinding/sandpaper" feel described.
    *   *Nuance:* The author mentions feeling it *before* the slide. Our effect triggers based on `Slip Angle`. If our threshold (0.10 rad) is too high, it might trigger too late. (See `docs/dev_docs/grip_calculation_and_slip_angle_v0.4.12.md` for discussion on lowering this).

**3. The "ABS Rattle" (Pulsing): ⚠️ PARTIAL / UNCERTAIN**
*   **Video:** Describes a "pseudo feeling of the ABS pump working."
*   **LMUFFB:** We have a **Lockup Effect** (`m_lockup_enabled`).
    *   *Logic:* Triggers when `Slip Ratio < -0.1`.
    *   *The Gap:* If the car's ABS system is very good, it might keep the slip ratio *above* -0.1 (e.g., at -0.08). In that case, LMUFFB would be silent.
    *   *Missing Feature:* We do not have a specific "ABS Active" trigger. We rely on the physics result (Slip). If the ABS hides the slip, we hide the vibration. We might need to lower the threshold or read `mBrakePressure` oscillation to simulate the pump directly.

**4. Dynamic Weight Transfer (Longitudinal): ❌ MISSING**
*   **Video:** Praises AC1 for the feeling of the car getting heavy under braking and light under acceleration.
*   **LMUFFB:** We currently **Pass-Through** the game's steering torque.
    *   If LMU's physics engine (like AC Evo in the video) does not provide enough weight transfer in the steering column naturally, LMUFFB does not currently add it.
    *   *Missing Feature:* **Synthetic Longitudinal Weighting.** We calculate `Load Factor` for textures, but we do *not* use it to scale the `Base Force`.
    *   *Recommendation:* We should implement `Master_Gain_Dynamic = Master_Gain * (1.0 + (Longitudinal_G * Factor))` to artificially boost weight under braking if the game is too numb.


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
