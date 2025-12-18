# **Driver's Guide to Testing LMUFFB**.

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

**UI Settings (Isolation):**
*   **Master Gain:** `1.0`
*   **Understeer (Grip):** **`1.0`** (Max)
*   **SoP / Oversteer / Textures:** `0.0` (Turn OFF to feel the pure weight drop)

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

**UI Settings (Isolation):**
*   **Master Gain:** `1.0`
*   **SoP (Lateral G):** `1.0`
*   **Rear Align Torque:** `1.0`
*   **Oversteer Boost:** `1.0`
*   **Understeer / Textures:** `0.0`

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

**UI Settings (Isolation):**
*   **Master Gain:** `1.0`
*   **Slide Rumble:** **Checked**
*   **Slide Gain:** `1.0`
*   **Scrub Drag Gain:** `1.0`
*   **All other effects:** `0.0`

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

**UI Settings (Isolation):**
*   **Master Gain:** `1.0`
*   **Progressive Lockup:** **Checked**
*   **Lockup Gain:** `1.0`
*   **All other effects:** `0.0`

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

**UI Settings (Isolation):**
*   **Master Gain:** `1.0`
*   **Spin Traction Loss:** **Checked**
*   **Spin Gain:** `1.0`
*   **All other effects:** `0.0`

**Car Setup:**
*   **TC:** **OFF**.

**The Test:**
1.  Stop the car. Put it in 1st gear.
2.  Hold the brake and throttle (Launch Control style) or just mash throttle.
3.  **What to feel:**
    *   *The Cue:* The steering weight suddenly disappears (Torque Drop). It feels like the car is floating on ice.
    *   *The Vibe:* A high-pitched hum/whine that rises as the RPM/Wheel Speed rises.

---

### 🛠️ Troubleshooting Cheat Sheet

| Symptom | Diagnosis | Fix |
| :--- | :--- | :--- |
| **Wheel feels dead/numb in corners** | SoP is too low or Understeer is too aggressive. | Increase `SoP (Lateral G)`. Decrease `Understeer`. |
| **Wheel oscillates (shakes L/R) on straights** | Latency or too much Min Force. | Increase `SoP Smoothing`. Decrease `Min Force`. |
| **Wheel pulls the wrong way in a slide** | Inverted Physics. | Check `Invert FFB Signal` or report bug (Yaw/Scrub inversion). |
| **No Road Texture over curbs** | Suspension frequency mismatch. | Increase `Road Gain`. Ensure `Load Cap` isn't too low. |
| **Effects feel "Digital" (On/Off)** | Clipping. | Check the "Clipping" bar in Debug window. Reduce `Master Gain` or increase `Max Torque Ref`. |
