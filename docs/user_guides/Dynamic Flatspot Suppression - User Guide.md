# Dynamic Flatspot Suppression - User Guide

**Feature Version:** v0.4.43+  
**Last Updated:** 2025-12-21

---

![lmuFFB GUI](screenshots/Signal%20Filtering.png)

## What Is It?

Dynamic Flatspot Suppression is a **surgical filter** that removes speed-dependent vibrations from your force feedback wheel without dulling the rest of your steering feel. It's specifically designed to eliminate:

- **Flat spots** on tires (from lockups or wear)
- **Tire polygon effects** (low-resolution tire models)
- **Unbalanced wheels** (manufacturing imperfections)
- **Any vibration linked to wheel rotation speed**

### Why Is It Different?

Unlike traditional smoothing filters that add latency and dull ALL feedback, this filter:

✅ **Zero Latency** - Uses a Biquad IIR notch filter with zero group delay at steering frequencies  
✅ **Surgical Precision** - Only removes the exact frequency of wheel rotation  
✅ **Preserves Detail** - Road bumps, curbs, and steering inputs pass through untouched  
✅ **Adaptive** - Automatically tracks changing wheel speed

---

## When Should You Use It?

### ✅ **Use It When:**
- You feel a rhythmic "thud-thud-thud" or "buzz" that speeds up with the car
- You've flat-spotted your tires during a race
- You're driving cars with noticeable tire polygon effects
- You want a perfectly smooth wheel at high speeds

### ❌ **Don't Use It When:**
- You have no vibration issues (adds unnecessary processing)
- You want to feel flat spots as a penalty for lockups (realism preference)
- The vibration is NOT speed-linked (e.g., engine vibration, random bumps)

---

## How To Enable

### Step 1: Open the Tuning Window

1. Launch LMUFFB
2. The main **Tuning Window** should appear automatically
3. If not visible, check your system tray and click the LMUFFB icon

### Step 2: Locate Signal Filtering Section

1. Scroll down in the Tuning Window
2. Find the **"Signal Filtering"** section (should be expanded by default)
3. It's located between "Max Torque Ref" and "Advanced Tuning"

### Step 3: Enable the Filter

1. Check the box: **"Dynamic Flatspot Suppression"**
2. The filter is now active!

---

## Tuning the Filter

### Notch Width (Q Factor)

Once enabled, you'll see a slider: **"Notch Width (Q)"**

**Range:** 0.5 to 10.0  
**Default:** 2.0 (Balanced)

#### What Does Q Do?

The Q factor controls how "narrow" the filter is:

| Q Value | Behavior | Best For |
|---------|----------|----------|
| **0.5 - 1.0** | Wide, soft filter | Gentle suppression, preserves more texture |
| **2.0** | Balanced (Default) | Good compromise for most users |
| **3.0 - 5.0** | Narrow, surgical | Precise removal, minimal side effects |
| **5.0 - 10.0** | Very narrow | Extreme precision, may miss slight frequency variations |

#### Recommended Settings

**For Most Users:**
- Start with **Q = 2.0**
- If vibration persists, increase to **Q = 3.0 - 4.0**

**For Direct Drive Wheels:**
- Use **Q = 3.0 - 5.0** for surgical precision

**For Belt-Driven Wheels (T300, G29):**
- Use **Q = 1.5 - 2.5** for smoother feel

### Suppression Strength

New in **v0.4.43**, the **"Suppression Strength"** slider allows you to control how aggressively the filter is applied.

**Range:** 0.0 to 1.0 (0% to 100%)  
**Default:** 1.0 (Full Suppression)

#### When to lower strength:
- **Realism:** If you want to *feel* that you have a flat spot (for immersion) but want to reduce the violent shaking to a manageable level.
- **Diagnostics:** To confirm how much vibration the filter is actually removing by toggling between 0.0 and 1.0.
- **Preference:** If full suppression feels "too clinical" or you want to keep some tire texture.

| Strength | Behavior |
|----------|----------|
| **1.0** | Full filter. Vibration at the wheel frequency is maximum attenuated. |
| **0.5** | 50/50 Blend. You will feel exactly half of the original vibration intensity. |
| **0.0** | No filtering. Same as disabling the checkbox. |

---

## Static Noise Filter (v0.4.43+)

While the Dynamic Filter tracks your car speed, the **Static Noise Filter** targets a **fixed frequency**. This is specifically designed to eliminate mechanical hums, hardware resonances, or constant road-surface "buzz" that doesn't change with speed.

### When to Use It?
✅ **Constant Hum:** Your wheel makes a "buzzing" or "droning" sound/feel even when driving at a steady speed or on specific surfaces.
✅ **Hardware Resonance:** Your rig or wheel base rattles at a specific frequency (e.g., 50Hz).
✅ **Engine Vibration:** If the game produces a constant engine vibration that you find distracting.

### Configuration

1. Locate **"Static Noise Filter"** in the Signal Filtering section.
2. Enable the checkbox.
3. Use the **"Target Frequency"** slider to find the "hum."

**Frequency Range:** 10 Hz to 100 Hz

> ⚠️ **WARNING:** High values on this slider (e.g., 40Hz - 80Hz) will remove genuine road detail at that specific frequency. Use the narrowest possible setup to preserve feel.

### Why Q is fixed at 5.0?
To keep the filter as "surgical" as possible, the Static Notch uses a fixed **Q factor of 5.0**. This ensures that only a paper-thin slice of the frequency spectrum is removed, leaving your steering feel 99% intact.

---

## Verifying It's Working

### Method 1: The Diagnostic Window

1. Check **"Show Troubleshooting Graphs"** at the bottom of the Tuning Window
2. Open the **"FFB Analysis"** window
3. Expand **"C. Raw Game Telemetry (Input)"**
4. Scroll to the bottom to find **"Signal Analysis"**

You'll see two frequency readouts:

```
Est. Vibration Freq: 15.3 Hz
Theoretical Wheel Freq: 15.1 Hz
```

**What This Means:**
- **Est. Vibration Freq** - The actual vibration detected in your FFB signal
- **Theoretical Wheel Freq** - The expected frequency based on car speed and tire size

**If they match (within ±2 Hz):**  
✅ The vibration IS a flat spot → Filter will work perfectly

**If they don't match:**  
⚠️ The vibration is NOT speed-linked → Filter won't help (try other settings)

### Method 2: The Feel Test

1. **Before Enabling:** Drive at constant speed (e.g., 120 km/h) and feel the vibration
2. **Enable Filter:** Check "Dynamic Flatspot Suppression"
3. **After Enabling:** The rhythmic vibration should disappear instantly

**What Should Still Feel Normal:**
- ✅ Random road bumps (different frequencies)
- ✅ Curb impacts (transient events)
- ✅ Steering weight and resistance
- ✅ Understeer/oversteer feedback

**What Should Be Gone:**
- ❌ Rhythmic "thud-thud-thud" linked to speed
- ❌ High-frequency "buzz" that changes with speed

---

## Step-by-Step Testing Guide

### Creating a Flat Spot (For Testing)

1. **Find a straight section** of track
2. **Accelerate to 80-100 km/h**
3. **Brake HARD and hold** until wheels lock completely
4. **Keep braking** until the car almost stops (5-10 seconds)
5. **Release brakes** and accelerate back to 120 km/h

**Result:** You should now have a noticeable flat spot

### Testing the Filter

1. **Drive at 120 km/h** on a straight
2. **Feel the vibration** (should be rhythmic and speed-linked)
3. **Open Tuning Window** (Alt+Tab if needed)
4. **Check "Dynamic Flatspot Suppression"**
5. **Return to game** (Alt+Tab back)
6. **The vibration should be gone!**

### Fine-Tuning

If vibration persists:
1. Increase **Notch Width (Q)** to 3.0 - 4.0
2. Check the Diagnostic Window to verify frequencies match
3. If frequencies don't match, the vibration is NOT a flat spot

---

## Troubleshooting

### "The filter doesn't seem to do anything"

**Possible Causes:**
1. **No flat spot exists** - The vibration you feel is from something else
2. **Q is too high** - Try lowering to 2.0
3. **Speed too low** - Filter only activates above ~7 km/h (1 Hz wheel frequency)

**Solution:**
- Check the Diagnostic Window - do the frequencies match?
- Try creating an extreme flat spot (long lockup) for testing

### "The steering feels different/weird"

**Possible Causes:**
1. **Q is too low** - Filter is too wide and affecting nearby frequencies
2. **Placebo effect** - The filter should NOT affect steering feel

**Solution:**
- Increase Q to 3.0 - 5.0 for narrower filtering
- Do a blind test (have someone else toggle it without telling you)

### "Vibration comes back after a while"

**Possible Causes:**
1. **Multiple flat spots** - Different wheels have different frequencies
2. **Tire wear changing** - Flat spot shape evolving

**Solution:**
- This is normal - the filter tracks ONE frequency (front left wheel)
- If rear wheels have different flat spots, they may still vibrate slightly

### "The diagnostic frequencies don't match"

**Meaning:**
- The vibration is NOT linked to wheel rotation speed
- Could be: engine vibration, suspension resonance, or game bug

**Solution:**
- The filter won't help in this case
- Try adjusting other FFB settings (smoothing, gain, etc.)

---

## Technical Details

### How It Works

1. **Frequency Calculation:**  
   `Wheel Frequency (Hz) = Car Speed (m/s) / (2π × Tire Radius)`

2. **Filter Type:**  
   Biquad IIR Notch Filter (Audio EQ Cookbook implementation)

3. **Tracking:**  
   Filter coefficients update every frame based on current speed

4. **Safety:**  
   - Filter bypassed below 1 Hz (very low speeds)
   - State reset when stopped (prevents ringing on startup)
   - Frequency clamped to Nyquist limit (prevents aliasing)

### Performance Impact

- **CPU Overhead:** ~25 floating-point operations per frame
- **Memory:** ~72 bytes additional state
- **Latency:** Zero group delay at steering frequencies (0-5 Hz)

**Conclusion:** Negligible impact on modern systems

---

## Frequently Asked Questions

### Q: Will this work with all sims?

**A:** Yes, as long as LMUFFB is receiving telemetry data. The filter operates on the final FFB signal, so it's sim-agnostic.

### Q: Can I use this with other smoothing filters?

**A:** Yes, but it's usually not necessary. This filter is designed to replace traditional smoothing for flat spot issues.

### Q: Does it work in real-time or is there a delay?

**A:** Real-time with zero latency. The filter updates every frame (400 Hz) and has zero group delay at steering frequencies.

### Q: Will it remove ALL vibrations?

**A:** No, only vibrations at the wheel rotation frequency. Random bumps, curbs, and other effects pass through.

### Q: Can I save this setting in a preset?

**A:** Yes! The filter state is saved with your configuration and can be included in custom presets.

### Q: What if I have different tire sizes front/rear?

**A:** The filter uses the front-left tire radius. If rear tires are significantly different, rear vibrations may not be fully suppressed.

---

## Best Practices

### ✅ **Do:**
- Start with default settings (Q = 2.0)
- Use the Diagnostic Window to verify it's working
- Save your configuration after finding the right Q value
- Disable it when you don't need it (saves CPU cycles)

### ❌ **Don't:**
- Set Q too low (< 1.0) - may affect steering feel
- Set Q too high (> 7.0) - may miss the target frequency
- Expect it to fix non-speed-linked vibrations
- Use it as a substitute for proper tire management in races

---

## Summary

**Dynamic Flatspot Suppression** and the **Static Noise Filter** are powerful tools for eliminating speed-dependent and constant vibrations without compromising your FFB quality. When used correctly, they provide a perfectly smooth wheel at high speeds while preserving all the important steering feedback you need for fast, precise driving.

**Quick Start:**
1. **For Flatspots:** Enable "Dynamic Flatspot Suppression", set Q = 2.0.
2. **For Mechanical Hum:** Enable "Static Noise Filter", adjust Target Frequency until the buzz disappears.
3. Drive and enjoy smooth FFB!

**For Advanced Users:**
- Use Diagnostic Window to verify frequency matching
- Tune Q factor based on your wheel type
- Experiment with different values for your specific use case

---

**Need Help?**  
- Check the Troubleshooting section above
- Review the Diagnostic Window for frequency analysis
- Consult the main LMUFFB documentation for general FFB tuning

**Happy Racing! 🏁**
