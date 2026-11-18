## **Driver's Guide to Testing LMUFFB**
> Updated November 2026 (app 0.8.007). The car setups and test drives date from version 0.7.5 and still apply.

### 🏁 Prerequisites

**Car and track:**
*   **Car:** the **Porsche 911 GT3/GTE** is the best reference. Its rear-engine layout swings like a pendulum, so oversteer is very clear, and its light front end makes understeer distinct.
*   **Track:** **Paul Ricard** is ideal. It is flat (no elevation changes to confuse the feel) and has huge asphalt run-off areas for safe spinning.
    *   *Tip:* use the **Mistral Straight** for high-speed tests.
    *   *Tip:* use the **last corner (Virage du Pont)** for low-speed traction tests.

**Global setup:**
1.  **In LMU:** force feedback can stay enabled (needed for the In-Game source); set smoothing to 0.
2.  **Wheel driver:** set your wheel's strength to **20-30%** (safety first).
3.  **LMUFFB:** start from the **"Default LMU"** profile, then change what each test asks.

---

### 1. Understeer (front grip loss)

**What is it?** The front tyres are sliding. The car turns less than you are turning the wheel.
**The goal:** the steering should go **light** to tell you "stop turning, you have no grip."

**Quick setup:** load the profile **"Guide: Understeer (Front Grip)"**.

**Extreme car setup:**
*   **Brake bias:** **max forward (70-80%)**, so the front tyres overload as soon as you brake.
*   **Front springs and anti-roll bar:** **stiffest**, to reduce front grip.
*   **Rear springs and anti-roll bar:** **softest**, to keep the rear planted and make the car push.
*   **Front tyre pressure:** **maximum**.
*   **Rear wing:** **maximum**.

**The test:**
1.  Drive at moderate speed (100 km/h).
2.  Turn into a medium corner (for example Turn 1).
3.  **Turn too much on purpose:** 90 degrees or more, past the point where the car still turns.
4.  **What to feel:**
    *   *Normal:* resistance builds as you turn.
    *   *The cue:* the resistance **stops increasing** or **drops**. The wheel feels hollow or disconnected.
    *   *Correct behaviour:* unwind slightly and the weight returns.

The weight should fall away progressively, like a real tyre giving up, rather than switching off. Adjust how early and how sharply with **Understeer Curve**.

---

### 2. Oversteer (rear grip loss)

**What is it?** The rear tyres are sliding. The back of the car is trying to overtake the front.
**The goal:** the wheel should **pull** against the turn (counter-steer), as if it wants to fix the slide for you.

**Quick setup:** load the profile **"Guide: Oversteer (Rear Grip)"**.

**Extreme car setup:**
*   **Traction control:** **off** (crucial).
*   **Rear springs and anti-roll bar:** **stiffest**.
*   **Front springs and anti-roll bar:** **softest**, so the rear breaks first.
*   **Rear ride height:** **maximum**.
*   **Rear wing:** **minimum**.
*   **Differential preload:** **maximum**.

**The test:**
1.  Take a slow 2nd-gear corner.
2.  Mid-corner, **floor the throttle**.
3.  The rear kicks out immediately.
4.  **What to feel:**
    *   *The cue:* the wheel snaps to the **opposite direction** of the turn. Turning left, the wheel pulls right.
    *   *Correct behaviour:* if you let go, the wheel turns to line up with the road.
    *   *Wrong behaviour:* if the wheel pulls *into* the turn (making you spin faster), report it on GitHub with a log.

---

### 3. Slide texture (scrubbing)

**What is it?** The tyres are dragging sideways across the asphalt.
**The goal:** a grinding, sandpaper-like vibration.

**Quick setup:** load the profile **"Guide: Slide Texture (Scrub)"**. Start with the slide texture strength at **100%**; raise it if too subtle, lower it if the wheel rattles.

**Extreme car setup:**
*   **Tyre pressures:** **maximum** front and rear.
*   **Springs and anti-roll bars:** **stiffest** front and rear.
*   **Differential preload:** **maximum**.
*   **Downforce:** **minimum**.

**The test:**
*Option A: the plough (recommended)*
1.  Drive at **80-100 km/h** in a run-off area.
2.  Turn the wheel **90 to 180 degrees** quickly and hold.
3.  The car refuses to turn and ploughs straight.
4.  **What to feel:** a gritty vibration in the rim. If it is a weak buzz, raise the strength; if it hammers, lower it.

*Option B: the donut*
1.  Stop. 1st gear.
2.  Full lock, full throttle.
3.  **What to feel:** a continuous rumble from the spinning, sliding rear tyres.

The texture should start only when the tyres actually slide, not in normal cornering.

---

### 4. Braking lock-up

**What is it?** You braked too hard. The wheel stopped turning while the car is still moving, burning a flat spot into the tyre.
**The goal:** a strong vibration that tells you to release the brake.

**Quick setup:** load the profile **"Guide: Braking Lockup"**.

**Extreme car setup:**
*   **ABS:** **off** (crucial).
*   **Brake bias:** **far forward (75-80%)**, so the fronts lock first and the test is predictable.
*   **Front tyre pressure:** **maximum**.

**The test:**
1.  Drive fast (200 km/h) down the Mistral Straight.
2.  Press the brake **100%**.
3.  **What to feel:**
    *   *The cue:* the wheel shakes strongly.
    *   *As the car slows:* the shaking gets **slower and heavier** (thump-thump-thump).

---

### 5. Traction loss (wheelspin)

**What is it?** You accelerated too hard. The rear wheels spin freely.
**The goal:** a hum that rises with the wheelspin.

**Quick setup:** load the profile **"Guide: Traction Loss (Spin)"**.

**Extreme car setup:**
*   **Traction control:** **off**.
*   **Rear tyre pressure:** **maximum**.
*   **Rear springs:** **stiffest**.
*   **Differential preload:** **maximum**.

**The test:**
1.  Stop the car, 1st gear.
2.  Hold brake and throttle (launch style), or simply floor the throttle.
3.  **What to feel:**
    *   *The vibration:* a high-pitched hum that rises as the wheels spin faster.
    *   *Steering weight:* it stays as it is, unless you turned on the optional **Wheelspin Force Reduction**, which makes the steering feel light and floaty.

---

### 6. Rotation Kick

**What is it?** A short impulse at the moment the car *starts* to rotate. Unlike cornering weight, which is sustained, this is a momentary kick.
**The goal:** to signal the exact moment the rear breaks away.

**Quick setup:** load the profile **"Guide: Rotation Kick"**.

**Extreme car setup:**
*   **Traction control:** **off**.
*   **Brake bias:** **far rearward (about 40%)**, to make the car unstable under braking.
*   **Rear ride height:** **maximum**.
*   **Front springs:** **soft**.

**The test:**
1.  Drive at moderate speed (3rd gear) on a straight.
2.  Do a **Scandinavian flick**: turn sharply left, then immediately whip the wheel right to unsettle the rear.
3.  **What to feel:**
    *   *The cue:* the moment the car starts to rotate, a sharp **jolt** in the counter-steer direction.
    *   *Check:* drive a steady circle. You should feel **no kick**. Now floor the throttle to spin: you should feel the **kick**. It reacts to the start of rotation, not to steady turning.

---

### 7. Smart Hands-off Protection (stability)

**What is it?** A resistance against fast steering movements that grows with speed.
**The goal:** to stop the wheel from oscillating on its own at speed (for example if you loosen your grip), and to give the steering a fluid feel at high speed.

**Quick setup:** load the profile **"Guide: Gyroscopic Damping"**.

**Extreme car setup:**
*   **Aero:** **minimum (low drag)**, for the highest top speed.
*   **Caster:** **maximum**, which makes the self-centring strong and the need for damping obvious.

**The test:**
1.  **Standing still:** in the pits, wiggle the wheel left and right. It should feel light and easy.
2.  **At speed:** drive down the straight at **250 km/h**.
3.  **The wiggle:** make small, quick left-right movements.
    *   *The cue:* the wheel feels **thick** or **viscous** and resists the quick movements.
    *   *Check:* turn slowly and it feels lighter; turn fast and it fights you.

---

### 8. Corner entry (weight transfer)

**What is it?** The steering getting heavier as you brake and turn in, moving the car's weight onto the front tyres.
**The goal:** to confirm that the steering communicates the extra load on the front.

**Quick setup:** load the profile **"Test: Game Base FFB Only"**, so only the game's own steering force is active.

**Extreme car setup:**
*   **Front springs:** **softest**, so the nose dives under braking.
*   **Front bump dampers:** **soft**.
*   **Brake bias:** **forward**.

**The test:**
1.  Drive fast on a straight.
2.  Brake hard and turn in smoothly (trail braking).
3.  **What to feel:**
    *   *The cue:* the steering gets clearly **heavier** as the nose dives and the car turns in. It feels planted.
    *   *If it feels light or flat:* add weight with **Cornering Feel** (load your usual profile again first).

---

### 9. Reaching the limit mid-corner

**What is it?** The moment the front tyres reach their peak grip, just before they start to slide.
**The goal:** a cue that you are at the limit, so you can balance the car on the edge.

**Quick setup:** use your own profile with **Slide Texture** on and **Understeer Effect** at about 50%.

**Extreme car setup:**
*   **Tyre pressures:** **high**, for a sharper limit.
*   **Aero:** **high**, to hold high cornering speeds.

**The test:**
1.  Take a long constant-radius corner.
2.  Gradually add steering until you hear the tyres start to scrub.
3.  **What to feel:**
    *   *The sequence:* grip (quiet) → limit (the steering starts to lighten, with some texture) → understeer (light and quiet).
    *   *If the cue comes too late:* make **Understeer Curve** earlier, or raise the slide texture strength.

---

### 10. ABS (the rattle)

**What is it?** A pulsing vibration that mimics the ABS releasing brake pressure when a wheel is about to lock.
**The goal:** to let you press the brake fully and feel exactly where the limit is, without looking at a display.

**Quick setup:** load **"Guide: Braking Lockup"** and make sure **ABS Pulse** is on.

**Extreme car setup:**
*   **ABS:** **on, maximum intervention**.
*   **Brake pressure:** **100%**.
*   **Tyre pressures:** **maximum**, so ABS works immediately.

**The test:**
1.  Drive fast.
2.  Press the brake 100%.
3.  **What to feel:** a quick mechanical rattle or pulse, rather than the continuous screech of a full lock-up.

---

### 🛠️ Troubleshooting cheat sheet

| Symptom | Likely cause | Fix |
| :--- | :--- | :--- |
| **Wheel feels dead or numb in corners** | Too little cornering weight, or Understeer Effect too strong. | Raise **Cornering Feel**. Lower **Understeer Effect**. |
| **Wheel oscillates left-right on straights** | Too much Min Force, or too little damping. | Lower **Min Force**. Raise **Smart Hands-off Protection**. |
| **Wheel pulls the wrong way in a slide** | Force direction inverted. | Check **Invert FFB Signal**; if that is correct, report it with a log. |
| **No road feel over kerbs** | Road texture too low. | Raise **Road Texture**. |
| **Effects feel on/off ("digital")** | The total force hits your wheel's maximum. | Lower **Master Gain**. |

### References

* `https://www.youtube.com/watch?v=XHSEAMQgN2c&t=655s`
* `https://www.youtube.com/watch?v=kj4AEsnX5Cs`
* `https://www.youtube.com/watch?v=3MLKewyTanc`
