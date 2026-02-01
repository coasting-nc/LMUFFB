# Frequency Tuning Guide

**Version:** 0.6.20+  
**Last Updated:** 2025-12-27

---

## Introduction

Starting with version 0.6.20, lmuFFB allows you to tune the **vibrational pitch** of FFB effects to match your wheel's hardware characteristics. This guide explains how to use these new controls to get the best tactile feedback from your rig.

### What is Frequency Tuning?

Every force feedback wheel has a **resonant frequency** - a specific vibration rate where the wheel feels most responsive and natural. This depends on your wheel's mechanical design:

- **Belt-driven wheels** (e.g., Thrustmaster T300, Fanatec CSL DD) respond best to **lower frequencies** (10-25 Hz)
- **Gear-driven wheels** (e.g., Logitech G29/G920) feel sharper at **mid frequencies** (20-35 Hz)
- **Direct-drive wheels** (e.g., Simucube, VRS, Moza) can reproduce **high frequencies** accurately (30-50 Hz)

**The Problem:** Using the wrong frequency can make effects feel:
- Too "buzzy" or "toy-like" (frequency too high for your hardware)
- Too "mushy" or "vague" (frequency too low)
- Weak or barely noticeable (frequency doesn't match resonance)

**The Solution:** Tune the frequency to match your wheel's sweet spot.

---

## The Three Frequency Controls

lmuFFB provides three independent frequency tuning parameters:

### 1. ABS Pulse Frequency
**Location:** Tuning → Braking & Lockup → ABS Pulse → Pulse Frequency  
**Range:** 10 - 50 Hz  
**Default:** 20 Hz

**What it does:** Sets the vibration rate when the ABS system activates.

**How it feels:**
- **10 Hz:** Slow, heavy "thump-thump-thump" (like hitting rumble strips)
- **20 Hz:** Moderate "chatter" (realistic ABS feel)
- **40-50 Hz:** Fast "buzz" (sharp, precise, DD-wheel optimized)

### 2. Lockup Vibration Pitch
**Location:** Tuning → Braking & Lockup → Lockup Vibration → Vibration Pitch  
**Range:** 0.5x - 2.0x (multiplier)  
**Default:** 1.0x

**What it does:** Scales the frequency of tire lockup vibration (judder when wheels lock under braking).

**How it feels:**
- **0.5x:** Lower, "grumbling" lockup (belt-driven friendly)
- **1.0x:** Default speed-based frequency (10Hz + speed × 1.5)
- **2.0x:** Higher, "screeching" lockup (DD-wheel optimized)

### 3. Spin Vibration Pitch
**Location:** Tuning → Traction Loss & Textures → Spin Vibration → Spin Pitch  
**Range:** 0.5x - 2.0x (multiplier)  
**Default:** 1.0x

**What it does:** Scales the frequency of wheel spin vibration (when front tires lose traction under acceleration).

**How it feels:**
- **0.5x:** Slower, "juddering" spin (feels like fighting for grip)
- **1.0x:** Default speed-based frequency (10Hz + slip speed × 2.5)
- **2.0x:** Faster, "screaming" spin (high-RPM burnout feel)

---

## Quick Start: Recommended Settings by Wheel Type

### Belt-Driven Wheels (Thrustmaster T300, T500, TX, Fanatec CSL DD)

**Characteristics:**
- Smooth, quiet operation
- Best response: 15-25 Hz
- Can feel "buzzy" at high frequencies

**Recommended Settings:**
```
ABS Pulse Frequency:     15 Hz
Lockup Vibration Pitch:  0.7x
Spin Vibration Pitch:    0.8x
```

**Why:** Belt-driven wheels have mechanical damping that smooths out high frequencies. Lower settings feel more natural and prevent the "electric toothbrush" sensation.

---

### Gear-Driven Wheels (Logitech G29, G920, G923, G27)

**Characteristics:**
- Mechanical "notchiness"
- Best response: 20-30 Hz
- Can mask subtle low frequencies

**Recommended Settings:**
```
ABS Pulse Frequency:     25 Hz
Lockup Vibration Pitch:  1.0x
Spin Vibration Pitch:    1.0x
```

**Why:** Gear-driven wheels have inherent mechanical vibration. Mid-range frequencies cut through the gear noise and feel distinct.

---

### Direct-Drive Wheels (Simucube, VRS, Moza, Fanatec DD1/DD2)

**Characteristics:**
- Ultra-precise, zero latency
- Can reproduce 10-80 Hz accurately
- Risk of feeling "too sharp" or fatiguing

**Recommended Settings (Realistic):**
```
ABS Pulse Frequency:     30 Hz
Lockup Vibration Pitch:  1.2x
Spin Vibration Pitch:    1.2x
```

**Recommended Settings (Aggressive/Sim-Cade):**
```
ABS Pulse Frequency:     40 Hz
Lockup Vibration Pitch:  1.5x
Spin Vibration Pitch:    1.5x
```

**Why:** DD wheels can handle higher frequencies without distortion. Use realistic settings for immersion, or aggressive settings for maximum feedback clarity.

---

## Tuning Methodology: Finding Your Sweet Spot

### Step 1: Baseline Test (ABS Pulse Frequency)

1. **Load a car with ABS** (e.g., GT3 car in LMU)
2. **Set ABS Pulse Frequency to 20 Hz** (default)
3. **Drive and brake hard** on a straight
4. **Feel the ABS activation** - does it feel:
   - ✅ **Clear and distinct?** → You're done, move to Step 2
   - ❌ **Too buzzy/harsh?** → Lower to 15 Hz and re-test
   - ❌ **Too weak/vague?** → Raise to 30 Hz and re-test

5. **Iterate in 5 Hz steps** until you find the frequency that feels most "real"

**Target Feel:** The ABS should feel like **rapid, distinct pulses** - not a continuous buzz, not a vague rumble.

---

### Step 2: Lockup Vibration Pitch

1. **Disable ABS** (in-game or use a car without ABS)
2. **Set Lockup Vibration Pitch to 1.0x** (default)
3. **Brake hard enough to lock the wheels** (listen for tire squeal)
4. **Feel the lockup judder** - does it feel:
   - ✅ **Like tires skipping/chattering?** → You're done, move to Step 3
   - ❌ **Too high-pitched/screechy?** → Lower to 0.7x and re-test
   - ❌ **Too slow/mushy?** → Raise to 1.3x and re-test

5. **Fine-tune in 0.1x steps**

**Target Feel:** Lockup should feel like **tire rubber juddering against asphalt** - a mid-frequency vibration that builds with slip.

---

### Step 3: Spin Vibration Pitch

1. **Enable Spin Vibration** (Tuning → Traction Loss & Textures)
2. **Set Spin Vibration Pitch to 1.0x** (default)
3. **Floor the throttle in 1st gear** (RWD car on cold tires)
4. **Feel the wheel spin vibration** - does it feel:
   - ✅ **Like tires fighting for grip?** → You're done!
   - ❌ **Too harsh/fatiguing?** → Lower to 0.7x and re-test
   - ❌ **Too subtle/boring?** → Raise to 1.3x and re-test

5. **Fine-tune in 0.1x steps**

**Target Feel:** Spin should feel like **tires slipping and re-gripping** - a dynamic vibration that changes with slip speed.

---

## Advanced Tuning: Understanding the Math

### ABS Pulse Frequency (Absolute)

**Formula:** `sin(m_abs_freq_hz * 2π * time)`

**What this means:**
- This is a **fixed frequency** oscillator
- 20 Hz = 20 complete vibration cycles per second
- Higher values = faster pulses
- **Independent of car speed or brake pressure**

**Use Case:** Match your wheel's resonance for maximum ABS clarity.

---

### Lockup Vibration Pitch (Speed-Based Scalar)

**Formula:** `(10 Hz + car_speed_m/s × 1.5) × m_lockup_freq_scale`

**What this means:**
- Base frequency starts at **10 Hz** (standing still)
- Increases with **car speed** (realistic - faster lockup = higher pitch)
- At 20 m/s (~45 mph): `10 + 20×1.5 = 40 Hz`
- Your **pitch scalar multiplies this**

**Examples:**
- `0.5x` at 20 m/s: `40 × 0.5 = 20 Hz` (low, grumbling)
- `1.0x` at 20 m/s: `40 × 1.0 = 40 Hz` (default)
- `2.0x` at 20 m/s: `40 × 2.0 = 80 Hz` (high, screeching)

**Use Case:** Lower the scalar if high-speed lockups feel too harsh on your wheel.

---

### Spin Vibration Pitch (Slip-Speed-Based Scalar)

**Formula:** `(10 Hz + slip_speed_m/s × 2.5) × m_spin_freq_scale`

**What this means:**
- Base frequency starts at **10 Hz** (minimal slip)
- Increases with **slip speed** (how fast the tire is spinning vs. ground speed)
- At 10 m/s slip (~22 mph difference): `10 + 10×2.5 = 35 Hz`
- Your **pitch scalar multiplies this**
- **Capped at 80 Hz** to prevent ultrasonic frequencies

**Examples:**
- `0.5x` at 10 m/s slip: `35 × 0.5 = 17.5 Hz` (low, juddering)
- `1.0x` at 10 m/s slip: `35 × 1.0 = 35 Hz` (default)
- `2.0x` at 10 m/s slip: `35 × 2.0 = 70 Hz` (high, screaming)

**Use Case:** Lower the scalar if burnouts feel too "buzzy" on your wheel.

---

## Troubleshooting

### Problem: "I can't feel the ABS at all"

**Possible Causes:**
1. **ABS Pulse Gain too low** → Increase "Pulse Gain" slider (not frequency)
2. **Frequency too high for your wheel** → Lower ABS Pulse Frequency to 15 Hz
3. **Frequency too low** → Raise ABS Pulse Frequency to 30 Hz
4. **ABS not actually activating** → Check in-game ABS settings

**Solution:** Start with 20 Hz and adjust Pulse Gain first, then frequency.

---

### Problem: "Lockup feels like a continuous buzz, not judder"

**Cause:** Frequency too high for your wheel's mechanical response time.

**Solution:** Lower Lockup Vibration Pitch to 0.6x or 0.7x.

---

### Problem: "Wheel spin feels weak and vague"

**Possible Causes:**
1. **Spin Strength too low** → Increase "Spin Strength" slider (not pitch)
2. **Frequency doesn't match your wheel** → Try 0.8x or 1.2x

**Solution:** Adjust Spin Strength first, then fine-tune pitch.

---

### Problem: "Effects feel fatiguing after long sessions"

**Cause:** Frequencies too high, causing constant high-frequency vibration.

**Solution:**
- Lower all pitch scalars to 0.7x - 0.8x
- Lower ABS Pulse Frequency to 15 Hz
- Consider reducing overall gains (Lockup Strength, ABS Pulse Gain)

---

## Real-World Examples

### Example 1: Thrustmaster T300 (Belt-Driven)

**User Report:** "ABS feels like an electric razor, lockup is barely noticeable"

**Diagnosis:** Default 20 Hz ABS is too high, lockup frequency is getting lost in belt damping.

**Solution:**
```
ABS Pulse Frequency:     15 Hz  (lowered from 20)
ABS Pulse Gain:          1.5    (increased for clarity)
Lockup Vibration Pitch:  0.6x   (lowered from 1.0x)
Lockup Strength:         1.2    (increased for clarity)
```

**Result:** "ABS now feels like distinct pulses, lockup has a nice low rumble I can feel building up."

---

### Example 2: Simucube 2 Pro (Direct-Drive)

**User Report:** "Everything feels great but lockup is too subtle at high speed"

**Diagnosis:** DD wheel can handle higher frequencies, user wants more aggressive feedback.

**Solution:**
```
ABS Pulse Frequency:     40 Hz  (raised from 20)
Lockup Vibration Pitch:  1.5x   (raised from 1.0x)
Lockup Rear Boost:       2.0x   (increased for rear lock clarity)
```

**Result:** "Now I can feel exactly when the rears are about to lock. High-speed braking feels alive."

---

### Example 3: Logitech G29 (Gear-Driven)

**User Report:** "Can't tell the difference between ABS and lockup"

**Diagnosis:** Both effects using similar frequencies, getting masked by gear noise.

**Solution:**
```
ABS Pulse Frequency:     30 Hz  (raised for distinction)
Lockup Vibration Pitch:  0.8x   (lowered for contrast)
Spin Vibration Pitch:    1.0x   (default)
```

**Result:** "ABS is a fast chatter, lockup is a slower judder. I can finally tell them apart!"

---

## Frequency vs. Gain: What's the Difference?

**Frequency (Hz / Pitch):**
- **What:** How *fast* the vibration oscillates
- **Feel:** "Buzzy" vs. "Thumpy"
- **Analogy:** Musical pitch (high note vs. low note)

**Gain (Strength):**
- **What:** How *strong* the vibration is
- **Feel:** "Weak" vs. "Strong"
- **Analogy:** Musical volume (quiet vs. loud)

**Rule of Thumb:**
1. **Tune frequency first** to match your wheel's resonance
2. **Tune gain second** to set the intensity you prefer

---

## Preset Recommendations

### Conservative (Smooth, Realistic)
```
ABS Pulse Frequency:     18 Hz
Lockup Vibration Pitch:  0.8x
Spin Vibration Pitch:    0.8x
```
**Best for:** Long endurance races, belt-driven wheels, users who prefer subtle feedback.

---

### Balanced (Default)
```
ABS Pulse Frequency:     20 Hz
Lockup Vibration Pitch:  1.0x
Spin Vibration Pitch:    1.0x
```
**Best for:** Most users, general racing, mixed wheel types.

---

### Aggressive (Sharp, Competitive)
```
ABS Pulse Frequency:     35 Hz
Lockup Vibration Pitch:  1.3x
Spin Vibration Pitch:    1.3x
```
**Best for:** Direct-drive wheels, sprint races, users who want maximum feedback clarity.

---

## FAQ

### Q: Can I set different frequencies for front vs. rear lockup?

**A:** Not directly. However, the engine automatically applies:
- **Rear lockups:** 0.3x frequency multiplier (lower pitch)
- **Rear lockups:** 1.5x amplitude boost (stronger)

This helps you distinguish rear lock (dangerous) from front lock (understeer).

---

### Q: Why does lockup frequency change with speed?

**A:** This is realistic! In real life:
- **Slow lockup** (parking lot): Low-frequency judder (tire stick-slip at low speed)
- **Fast lockup** (highway): High-frequency vibration (rapid tire chatter)

The pitch scalar lets you adjust this relationship to taste.

---

### Q: What's the maximum safe frequency?

**A:** The engine caps spin vibration at **80 Hz** to prevent:
- Ultrasonic frequencies (inaudible, feel like continuous buzz)
- Potential motor overheating on some wheels
- Fatigue from constant high-frequency vibration

ABS is user-controlled up to **50 Hz** (safe for all wheels tested).

---

### Q: Will higher frequencies damage my wheel?

**A:** No. The frequencies used (10-50 Hz) are well within the safe operating range of all modern FFB wheels. These are the same frequencies used by:
- Road texture effects
- Curb vibrations
- In-game FFB effects

**However:** Very high gains + high frequencies can cause:
- Motor heating (reduce gain if your wheel gets hot)
- Mechanical wear (same as any aggressive FFB use)
- User fatigue (take breaks!)

---

### Q: Can I save different frequency settings per car?

**A:** Yes! Frequency settings are saved in **Presets**. You can create:
- "GT3 - Aggressive" preset with high frequencies
- "Vintage - Smooth" preset with low frequencies
- "Endurance - Comfortable" preset with moderate frequencies

Save and switch presets as needed.

---

## Summary: The 5-Minute Tuning Process

1. **Start with defaults** (20 Hz, 1.0x, 1.0x)
2. **Test ABS** → Adjust frequency until pulses feel distinct
3. **Test lockup** → Adjust pitch until judder feels realistic
4. **Test wheel spin** → Adjust pitch until vibration feels dynamic
5. **Save as preset** for this car/track combination

**Remember:** There's no "perfect" setting - tune to **your** wheel and **your** preference!

---

## Additional Resources

- **FFB Formulas:** `docs/dev_docs/FFB_formulas.md` - Mathematical details
- **Telemetry Reference:** `docs/dev_docs/references/Reference - telemetry_data_reference.md` - Parameter ranges
- **Community Presets:** (Coming soon) Share your optimal settings!

---

**Document Version:** 1.0  
**Compatible with:** lmuFFB v0.6.20+  
**Last Updated:** 2025-12-27
