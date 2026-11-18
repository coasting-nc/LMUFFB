# Step-by-Step Interactive FFB Tuning Guide

This guide is designed to help you set up and tune the lmuFFB application from scratch. Because the app offers a highly sophisticated, physics-driven force feedback (FFB) engine, tuning too many sliders at once can quickly lead to a muddy, clipped, or confusing wheel feel. 

By adjusting **one setting at a time** while on track, you can isolate exactly what each slider does, understand how interdependent settings interact, and dial in a premium, communicative force feedback signature tailored to your specific wheelbase.

---

## Preparation: Starting with a "Flat Slate"

Before adjusting any sliders, you must clear out all pre-configured effects so you can feel each component in isolation.

1. **Open the lmuFFB GUI** and navigate to the **Presets & General** tab.
2. We recommend creating a temporary preset named **`Tuning_Baseline`**. 
3. Set all settings to their baseline "Flat Slate" values:
   * **All Vibrations & Textures** (Road, Slide, Spin, Scrub, Bottoming, ABS, Lockups): `0%` or `Disabled`.
   * **Car-motion cues** (Cornering Feel, Cornering Weight, SoP Self-Aligning Torque, Rotation Kick, Braking Kick, Power Kick): `0%` or `0.00`.
   * **Steering Smoothing**: `0.000 s`.
   * **Smart Hands-off Protection**: `0%` or `0.00`.
   * **Stationary Damping**: `0%` or `0.00`.
4. In **Le Mans Ultimate (Game Settings)**:
   * Ensure game FFB strength is set to **`100%`** (1.0). This ensures the game output is not pre-attenuated before reaching lmuFFB.
   * Verify your wheel driver (e.g., Simucube, Fanatec, Moza) is set to **`100%`** force and has its own smoothing and damping at a minimum.

---

## Step 1: The Wheelbase Foundation (Hardware Limits)

Before doing any physics tuning, you must tell lmuFFB the exact limits of your physical hardware. 

### Wheelbase Max Torque
* **Where to find it:** `Presets & General` -> `Wheelbase Configuration`
* **What it does:** Represents the absolute peak physical torque your motor can produce in Newton-meters (Nm).
* **Tuning action:** 
  * Set this slider to match your wheelbase's manufacturer specifications (e.g., `4.3` for a Thrustmaster T300, `8.0` for a Fanatec CSL DD, `15.0` for a Simagic Alpha, `25.0` for a Simucube 2 Pro).
  * **CRITICAL:** Do **not** inflate or deflate this value to adjust overall steering weight. Keep it physically accurate. 

### Invert FFB Signal
* **Where to find it:** `Presets & General` -> `Wheelbase Configuration`
* **What it does:** Inverts the direction of the force feedback signal.
* **Tuning action:** Start driving slowly. If the steering wheel pulls aggressively *away* from center when you turn slightly, check this box. If it naturally resists steering inputs and returns to center, leave it unchecked.

---

## Step 2: Overall strength (Steering Scale & Strength Margin)

### Overall strength and calibration

- **Master Gain** is the main overall strength control. It scales steering, added cues, vibrations and damping together. Start comfortably low, especially on a direct-drive base, then raise it gradually. There is no universal 85% recommendation.
- **Steering Scale** changes steering and car-motion cues relative to vibrations and damping. It is not a guaranteed physical torque limit.
- **Strength Margin** lowers steering, car-motion cues and vibrations relative to damping. It does not reserve torque for vibrations, and it does not make clipping impossible. Its default is 100%.

With the **In-Game** source, the game's steering force is capped before Master Gain. Strength Margin and In-Game Gain decide where that cap is reached. Beyond it, more Steering Scale only raises the car-motion cues, so do not keep raising it to seek heavier base steering. With **Shaft Torque**, Scale still directly sets the steering level. The final combined force can clip with either source.

Use the correct Wheelbase Max Torque and LMU's FFB Strength at 100%, and turn off the game's own wheel output separately. Set a comfortable Master Gain first. Change one calibration setting at a time on the same car and corners, watching clipping and the balance of cues. Added effects may need reducing on small wheels even when base steering is well calibrated.

For examples and the source-dependent ceiling, see the [Steering Scale guide](Cornering_Steering_Weight_Guide.md).

---

## Step 3: Center Feel

If you are using a belt-driven or gear-driven wheel, or an older direct-drive unit, you may experience a "dead zone" or a loose, floating feeling when the steering wheel is near 0 degrees (dead center).

### Center Feel
* **Where to find it:** `Presets & General` -> `Center Feel`
* **What it does:** Adds a little weight when the wheel is near center.
* **Tuning action:** 
  * Drive down a long straight. If the wheel feels disconnected or loose around center (a dead zone of 1-3 degrees), increase **Center Feel** in increments of `0.2` (up to a maximum of `2.0`).
  * **Too High:** You will feel a distinct, artificial "notch" or "spring" at the center point. The wheel may start to oscillate (wagging left and right) rapidly on straights when you let go.
  * **Too Low:** The wheel feels loose, and you cannot sense small tracking adjustments on straightaways.

### Fade Start, Fade End and Range %
* **What they do:** Control how quickly the Center Feel effect fades out as steering angle and cornering forces build.
* **Tuning action:** 
  * Set **Range %** to `2.5%` (it follows your car's steering lock).
  * Set **Fade Start** to `0.5 Nm` and **Fade End** to `2.0 Nm`.
  * Make sure that, a few degrees into a corner, Center Feel has faded out completely and you feel only the game's steering.

---

## Step 4: Cornering Feedback & Understeer (Front Axle)

This step calibrates how you feel the front tires losing grip when you enter a corner too fast.

### 1. Steer Torque Source
* **Where to find it:** `Front Axle` -> `Steer Torque Source`
* **What it does:** Chooses which of the game's steering forces you feel.
* **Tuning action:** 
  * Set this to **`In-Game FFB (LMU 1.2+)`**, the game's own force feedback and the most detailed choice.

### 2. Understeer Effect & Understeer Curve
* **Where to find it:** `Front Axle` -> `Understeer Feeling`
* **What it does:** Scales how much the steering wheel "goes light" when the front tires slide (understeer).
* **Tuning action:**
  * Drive onto a slow corner, turn the wheel past its useful limit to purposely force the car to slide straight (understeer).
  * Increase **Understeer Effect** until you feel a distinct drop-off in steering weight when the front tires wash out.
  * Adjust **Understeer Curve**: 
    * Set to `1.0` for an even transition.
    * Set **below 1.0** (e.g., `0.8`) if you want the wheel to go light *very early* as grip begins to fade.
    * Set **above 1.0** (e.g., `1.5`) if you want a safety buffer, where the wheel remains heavy until a deep slide occurs.
  * **Too High:** The steering wheel becomes completely limp and disconnected the moment you turn slightly too hard, making it difficult to find the grip limit.
  * **Too Low:** You cannot feel when you have turned the front wheels too far, leading to excessive tire wear and slow lap times.

---

## Step 5: Cornering and slide catching

> [!IMPORTANT]
> **Interdependent Settings Warning**
> **Cornering Feel**, **SoP Self-Aligning Torque** and **Cornering Feel Boost (Slide)** work together to tell you when the rear of the car is sliding. Understanding their relationship is crucial for catching spins.

### How They Interact:
* **Cornering Feel** builds weight as you load the car into a corner, giving you the sensation of the car leaning on its tyres.
* **SoP Self-Aligning Torque** is the active "pull" that rotates the wheel *in the direction of the slide* (counter-steer) the moment the rear tires slip.
* **Cornering Feel Boost (Slide)** increases the weight of the steering wheel *during* the slide. It provides the sustained resistance needed to hold a slide, reflecting the inertia of the car's weight swinging out.

```
Rear Axle Slide
 1. Rear Steps Out  ──>  2. Self-Aligning Torque  ──>  3. Cornering Feel Boost (Slide)
 (Rotation Kick Impulse)      (Active Counter-Steer Pull)   (Sustained Weight of the Slide)
```

### 1. Cornering Feel
* **What to feel for:** The steady, smooth build-up of weight as you enter a sweeping corner.
* **Tuning action:**
  * Start with **Cornering Feel** at `1.0`.
  * **Too High:** The wheel feels like it is resisting your input too smoothly, masking the tactile "snappiness" of the steering rack.
  * **Too Low:** You lose the sense of the car's weight, making it feel floaty and detached from the road.

### 2. SoP Self-Aligning Torque
* **What to feel for:** The wheel actively pulling itself to counter-steer when the rear slides.
* **Tuning action:**
  * Initiate a small slide on corner exit.
  * Increase **SoP Self-Aligning Torque** in increments of `0.1` until the wheel naturally "catches" itself in your hands without you having to manually throw it into a counter-steer.
  * **Too High:** The wheel snaps violently out of your hands at the slightest hint of rotation, making the car twitchy and hard to drive smoothly.
  * **Too Low:** You have to manually steer the car into a counter-steer during a slide, resulting in delayed reactions and spins.

### 3. Cornering Feel Boost (Slide)
* **What to feel for:** The physical effort required to hold the wheel steady *during* a slide.
* **Tuning action:**
  * While holding a slide, notice how heavy the wheel feels. Increase **Cornering Feel Boost (Slide)** to add weight and stability during the slide.
  * **Too High:** The wheel becomes incredibly heavy and difficult to turn when drifting or catching a slide, locking your steering inputs.
  * **Too Low:** The wheel feels light and limp once the slide has started, making it hard to judge how much steering angle is needed to recover.

### 4. Rotation Kick
* **Where to find it:** `Rear Axle` -> `Oversteer Response`
* **What it does:** A sharp, short kick the moment the rear steps out. This is your earliest warning.
* **Tuning action:**
  * Adjust the **Rotation Kick** strength until you feel a clear, sharp "thump" or "snap" through the wheel the moment the car starts to rotate.
  * If it fires too often over kerbs, raise the **Activation Threshold** (e.g., to `1.0` or `1.5`) so small bumps do not trigger it.
  * **Too High:** Every curb strike or sudden lane change causes a harsh, metallic kick on the wheel.
  * **Too Low:** You miss the initial moment of rear-tire breakaway, receiving the warning too late to react.

---

## Step 6: Advanced oversteer cues (Power Kick & Braking Kick)

Once the basic slide catching is calibrated, you can fine-tune the system to differentiate between oversteer under acceleration and oversteer under braking.

### 1. Power Kick
* **Where to find it:** `Power Oversteer` or `Quick Access`
* **What it does:** Triggers a rotation cue specifically when the rear tires spin under throttle (wheel spin oversteer).
* **Tuning action:**
  * Set **TC Slip Target** to your preference (e.g., `10%`).
  * Accelerate hard out of a slow corner to spin the rear tires. Increase **Power Kick Gain** until you feel an immediate, sharp warning snap.
  * Adjust **Curve**: lower values (e.g., `0.5`) make it stronger on small, early slides; higher values (e.g., `1.5`) make it act only on big power slides.

### 2. Braking Kick
* **Where to find it:** `Braking` or `Quick Access`
* **What it does:** Triggers a rotation cue when the rear axle becomes unloaded and slides under heavy braking or lift-off (lift-off oversteer).
* **Tuning action:**
  * Brake heavily while turning into a corner (trail braking) to make the rear step out.
  * Increase **Braking Kick Gain** until you feel a clear counter-steering torque pull during trail-braking slides.

---

## Step 7: Braking Weight & ABS Tactility

This step ensures you can feel the deceleration forces and tire status under heavy braking.

### 1. Brake Feel
* **Where to find it:** `Braking` or `Quick Access`
* **What it does:** Makes the steering wheel physically heavier under braking. This simulates weight transferring onto the front tires without being affected by aerodynamics.
* **Tuning action:**
  * Brake in a straight line from high speed. Increase **Brake Feel** until the steering wheel feels significantly heavier during maximum braking deceleration.
  * Adjust **Brake Feel Smoothing** (recommended: `0.100 s` to `0.200 s`) to simulate the suspension settling under weight transfer.
  * **Too High:** Turning the wheel under braking becomes exhausting and masks all road details.
  * **Too Low:** The steering weight feels identical whether you are cruising or threshold braking.

### 2. Lockup Vibration
* **Where to find it:** `Braking` -> `Lockup Vibration`
* **What it does:** Vibrates the wheel to indicate locked front or rear tires.
* **Tuning action:**
  * Stomp on the brakes to intentionally lock the tires.
  * Set **Lockup Strength** until the vibration is strong enough to feel, but not violent.
  * Adjust **Vibration Pitch**: Match this to your wheelbase's physical resonance. (Direct drive wheels feel best with higher pitch, belt-driven with lower pitch).
  * **Too High:** Lockups cause the wheel to shake violently, potentially clipping the motor and breaking your grip on the wheel.
  * **Too Low:** You cannot distinguish a locked tire from standard road texture.

### 3. ABS Pulse
* **Where to find it:** `Braking` -> `ABS & Hardware`
* **What it does:** Simulates a mechanical ABS pulsing through the steering column when driving cars equipped with ABS.
* **Tuning action:** Check **ABS Pulse** and set **Pulse Gain** to a comfortable level. You will feel a steady, rhythmic pulse (e.g., `15 Hz`) ONLY while ABS is working.

---

## Step 8: Tactile Textures & Surface Vibrations

Now that the core physics, weight, and rotation cues are set up, you can overlay the fine-grained road and tire textures.

### 1. Global Vibrations Strength
* **Where to find it:** `Vibrations` -> `Global Vibrations Setting`
* **What it does:** A master volume control for all tactile vibration effects.
* **Tuning action:** Set this to `95%` or `100%` for direct-drive wheels, or lower (`50%-75%`) for gear-driven wheels to prevent excessive rattle.

### 2. Road Details
* **What to feel for:** The fine grain of the asphalt, expansion joints, and curbs.
* **Tuning action:**
  * Drive over a rough section of track (like the sebring concrete or curbs).
  * Increase **Road Gain** until you feel the texture of the track surface.
  * Adjust **Road Smoothing**: If the textures feel "sharp," "harsh," or "clicky," increase smoothing (`0.005 s` to `0.015 s`) to simulate tire rubber absorbing the edges.
  * **Too High:** The wheel constantly buzzes and shakes on straights, masking steering weight forces.
  * **Too Low:** The track feels artificially smooth, like driving on glass.

### Spin Vibration and optional wheelspin lightening (0.7.914+)

**Spin Vibration** adds a rumble when the driven wheels spin. Adjust **Spin Strength** for that rumble. In LMU, **Wheelspin Force Reduction** is a separate amount in the same panel (also in Quick Access). Start it at **0%** to retain steering information during a slide. Global Vibrations Strength does not control this reduction. Legacy profiles retain their previous amount; setting it to zero, resetting or reapplying an updated built-in can make wheelspin force stronger, so recheck Master Gain and clipping. Saving a copy of an existing profile retains its current settings.

### 3. Slide Rumble (Tire Scrub)
* **What to feel for:** A grinding, low-frequency scrub feeling when the tires slide laterally.
* **Tuning action:**
  * Slide the car laterally through a corner.
  * Increase **Slide Gain** and adjust **Slide Damping** to shape the texture.
  * **Too High:** Long slides create a massive, exhausting vibration that obscures all other feedback.

---

## Step 9: Safety & Pit Box Stability

The final step is to apply safety damping so the wheel is comfortable to use and does not shake violently when you let go.

### 1. Smart Hands-off Protection (Gyro Damping)
* **Where to find it:** `Presets & General` -> `Oscillation Prevention & Safety`
* **What it does:** Smartly dampens the wheel when you let go on a straightaway, preventing violent tank-slappers. It automatically fades out during active cornering.
* **Tuning action:**
  * Drive down a long straight at high speed, then momentarily let go of the wheel.
  * If the wheel begins to wobble and oscillate violently, increase **Smart Hands-off Protection** in increments of `5%` until the wheel immediately self-stabilizes when released.
  * **Too High:** The steering feels muddy, sluggish, and artificially heavy, masking fine road textures during cornering.
  * **Too Low:** Letting go of the wheel on a straightaway or during a crash will cause it to spin violently back and forth.

### 2. Stationary Damping
* **What it does:** Dampens the wheel *only* when the car is stopped.
* **Tuning action:**
  * While sitting in the garage or pit box, turn the wheel.
  * Increase **Stationary Damping** until the wheel feels solid and does not oscillate or vibrate when stationary.
  * As you drive out of the pits, notice that the damping fades away completely above `10 km/h`.

---

## Summary: Recommended Tuning Workflow

For the best results, always follow this order:

```
  ┌──────────────────────────────────────────────────────────┐
  │ 1. Set Wheelbase Max Torque (Manufacturer spec)         │
  └────────────────────────────┬─────────────────────────────┘
                               ▼
  ┌──────────────────────────────────────────────────────────┐
  │ 2. Dial in Steering Scale & Strength Margin      │
  └────────────────────────────┬─────────────────────────────┘
                               ▼
  ┌──────────────────────────────────────────────────────────┐
  │ 3. Adjust Understeer & Slide Alignment (Catching spins)  │
  └────────────────────────────┬─────────────────────────────┘
                               ▼
  ┌──────────────────────────────────────────────────────────┐
  │ 4. Layer on ABS, Lockups, & Road Details (Tactile)      │
  └────────────────────────────┬─────────────────────────────┘
                               ▼
  ┌──────────────────────────────────────────────────────────┐
  │ 5. Apply Gyro Damping (Safety & Stability)               │
  └──────────────────────────────────────────────────────────┘
```

---

## Document Revision History

| Date | App Version | Event / Summary of Changes |
| :--- | :--- | :--- |
| **2026-06-21** | `0.7.412` (released with `0.7.413`) | **Initial Creation**: Created the comprehensive step-by-step interactive FFB tuning guide covering baseline flat-slate setup, wheelbase hardware torque limits, overall strength, center feel, understeer, cornering and slide catching, power and braking kicks, braking feel and ABS/lockup vibrations, road textures, and safety damping. |
| **2026-11-18** | `0.8.001` | **Renamed settings:** uses the new setting names (Cornering Feel, Rotation Kick, Strength Margin and others) and describes feel and controls only. |
