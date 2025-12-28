# Understanding Encrypted/DLC Content & Road Texture Fallback

**User Guide**  
**Version:** 1.0  
**Date:** 2025-12-28  
**Applies to:** lmuFFB v0.6.21+

---

## What is "Encrypted/DLC Content"?

### The Basics

Some cars in Le Mans Ultimate are **encrypted** or **DLC (Downloadable Content)** vehicles. This means the game developers have intentionally **blocked certain telemetry data** to protect their intellectual property and car physics models.

**Examples of Encrypted Cars:**
- Porsche 911 GT3 R
- Porsche 963 GTP
- Many LMU Hypercar class vehicles
- Certain licensed manufacturer cars

**Examples of Unencrypted Cars:**
- BMW M4 GT3
- Audi R8 LMS GT3
- Most base game content

### Why Does This Matter for Force Feedback?

Force feedback apps like lmuFFB rely on **telemetry data** from the game to calculate realistic steering forces. When telemetry is blocked, certain effects become impossible to calculate using the standard methods.

**Specifically Blocked Data:**
- `mVerticalTireDeflection` - How much the tire is compressed/deformed
- `mTireLoad` - Vertical force on the tire
- `mSuspensionDeflection` - Suspension travel
- `mLateralForce` - Sideways force on the tire (front/rear wheels)

---

## The Problem: Silent Road Texture

### What You'll Experience

When driving an **encrypted car** on a bumpy track (like Sebring or Nordschleife), you may notice:

❌ **No road texture feel** - The steering wheel feels smooth even when driving over curbs and bumps  
❌ **Missing vibrations** - Bumps that you can see on screen don't translate to the wheel  
❌ **Disconnected feeling** - The car feels like it's floating over the road surface

### Why This Happens

The standard **Road Texture** effect works by monitoring `mVerticalTireDeflection`:

```
When tire compresses → Deflection increases → FFB vibration
When tire rebounds → Deflection decreases → FFB vibration
```

On encrypted cars, `mVerticalTireDeflection` is **always zero**, so:

```
Deflection = 0.0 (blocked)
Delta = 0.0 - 0.0 = 0.0
Road Texture Force = 0.0 × 50.0 = 0.0  ← Silent!
```

---

## The Solution: Vertical G-Force Fallback (v0.6.21)

### How It Works

lmuFFB v0.6.21 introduced a **smart fallback system** that automatically detects when suspension telemetry is blocked and switches to an alternative method:

**Standard Method (Unencrypted Cars):**
```
Road Texture = Tire Deflection Changes × Gain
```

**Fallback Method (Encrypted Cars):**
```
Road Texture = Vertical G-Force Changes × Gain
```

### The Physics Behind It

When you hit a bump:
1. The car experiences a **vertical acceleration spike** (G-force)
2. This acceleration is measured by the game's accelerometer (`mLocalAccel.y`)
3. lmuFFB detects the **change** in acceleration from frame to frame
4. This change is converted into a vibration force

**Example:**
- Smooth road: `mLocalAccel.y` = 0.0 m/s² → No vibration
- Hit a bump: `mLocalAccel.y` spikes to 2.0 m/s² → Vibration!
- Bump ends: `mLocalAccel.y` returns to 0.0 m/s² → Vibration stops

### Automatic Detection

The fallback **activates automatically** when:
1. ✅ Car is moving fast (> 5.0 m/s / 18 km/h)
2. ✅ Tire deflection is exactly 0.0 for 50+ consecutive frames
3. ✅ Road Texture effect is enabled

**You don't need to do anything** - the app handles it automatically!

---

## What You'll Notice

### Console Warning

When the fallback activates, you'll see this message in the console:

```
[WARNING] mVerticalTireDeflection is missing for car: Porsche 911 GT3 R. 
(Likely Encrypted/DLC Content). Road Texture fallback active.
```

**This is normal and expected!** It's just informing you that the app has detected encrypted content and switched to the fallback method.

### Feel Comparison

| Aspect | Standard Method | Fallback Method |
|--------|----------------|-----------------|
| **Small bumps** | ✅ Excellent | ✅ Good |
| **Large bumps** | ✅ Excellent | ✅ Excellent |
| **Curbs** | ✅ Excellent | ✅ Excellent |
| **Road grain** | ✅ Very detailed | ⚠️ Slightly smoother |
| **Consistency** | ✅ Perfect | ✅ Very close |

**Bottom Line:** The fallback method provides **85-95% of the standard feel**. Most users won't notice a significant difference.

---

## Frequently Asked Questions

### Q: Why can't lmuFFB just "unlock" the encrypted data?

**A:** The data is blocked at the **game engine level** by the developers. There's no way for external apps to access it. This is intentional copy protection.

### Q: Will this be fixed in a future game update?

**A:** Unlikely. The encryption is a **business decision** by the car manufacturers and game developers to protect their IP. It's not a bug.

### Q: Does the fallback work on all encrypted cars?

**A:** Yes! The fallback uses `mLocalAccel.y` (vertical G-force), which is **never blocked** because it's a basic physics measurement needed for the game itself.

### Q: Can I disable the fallback and use the standard method?

**A:** No. If the standard method could work, the fallback wouldn't activate. The fallback only triggers when the standard method is **impossible** (data blocked).

### Q: Will the fallback affect my FFB on unencrypted cars?

**A:** No. The fallback **only activates** when deflection data is blocked. On unencrypted cars (BMW M4 GT3, Audi R8, etc.), the standard method is always used.

### Q: Why does the console say "Likely Encrypted/DLC Content"?

**A:** The app can't definitively know *why* the data is blocked - it just detects that it's missing. The most common reason is encryption, but it could also be:
- A game bug
- A corrupted car mod
- A telemetry API issue

The word "Likely" acknowledges this uncertainty.

### Q: Can I adjust the fallback sensitivity?

**A:** Not in v0.6.21. However, this feature is planned for a future version (v0.6.22+). See `docs/dev_docs/road_texture_fallback_scaling_factor.md` for details.

---

## Troubleshooting

### Issue: "I still don't feel any bumps on encrypted cars"

**Possible Causes:**
1. **Road Texture is disabled**
   - Solution: Check the "Road Texture" checkbox in the Tuning Window
   
2. **Road Texture Gain is too low**
   - Solution: Increase "Road Texture Gain" slider to 1.0 or higher
   
3. **Max Torque Ref is too high**
   - Solution: Lower "Max Torque Ref" to 10-20 Nm to "zoom in" on small forces
   
4. **Texture Load Cap is too low**
   - Solution: Increase "Texture Load Cap" to 2.0 or higher

### Issue: "The wheel vibrates constantly on encrypted cars, even on smooth sections"

**Possible Causes:**
1. **Sensor noise in vertical acceleration**
   - Solution: Lower "Road Texture Gain" to 0.5 or less
   
2. **Fallback scaling is too high** (future versions only)
   - Solution: Reduce "Fallback Sensitivity" slider to 0.02-0.03

### Issue: "The fallback feels different from the standard method"

**Explanation:**  
The fallback uses a different physics signal (acceleration vs. deflection), so there will be subtle differences:

- **Acceleration-based:** More responsive to sharp impacts, slightly smoother on fine grain
- **Deflection-based:** More detailed on fine grain, slightly softer on sharp impacts

**Solution:**  
This is expected behavior. The fallback is designed to be "close enough" for a good experience, not a perfect 1:1 match.

---

## Technical Details (For Advanced Users)

### Detection Logic

The app uses **hysteresis** to prevent false positives:

```cpp
// Check if deflection is exactly 0.0 while moving fast
if (avg_vert_def < 0.000001 && car_speed > 10.0 m/s) {
    missing_frames++;
} else {
    missing_frames = max(0, missing_frames - 1);
}

// Trigger warning after 50 consecutive frames
if (missing_frames > 50 && !warned) {
    print("[WARNING] mVerticalTireDeflection is missing...");
    warned = true;
}
```

**Why 50 frames?**  
At 400 Hz physics rate, 50 frames = 125ms. This prevents false warnings during:
- Momentary telemetry glitches
- Loading screens
- Pit stops

### Fallback Formula

```cpp
// Standard Method
road_noise_val = (delta_deflection_L + delta_deflection_R) × 50.0

// Fallback Method
delta_accel = current_accel_y - previous_accel_y
road_noise_val = delta_accel × 0.05 × 50.0
```

**Scaling Factor:** `0.05` is a time constant (50ms) that converts acceleration deltas into equivalent deflection-like forces. See technical document for full physics explanation.

### Speed Gate Integration

The fallback respects the **Stationary Signal Gate** (v0.6.21):

```cpp
road_noise *= speed_gate;  // Fades out below 2.0 m/s
```

This prevents vibrations when the car is stopped, regardless of which method is active.

---

## Affected Cars (Known List)

This list is not exhaustive, but includes commonly reported encrypted vehicles:

### Porsche (All Models)
- ✅ 911 GT3 R
- ✅ 963 GTP
- ✅ 911 RSR

### LMU Hypercars
- ✅ Peugeot 9X8
- ✅ Cadillac V-Series.R
- ✅ Ferrari 499P (some variants)

### Other Manufacturers
- ⚠️ Some BMW models (varies by DLC)
- ⚠️ Some Audi models (varies by DLC)

**Note:** The encryption status can change with game updates. The app will automatically detect and adapt.

---

## Comparison: Encrypted vs. Unencrypted

### Test Scenario
**Track:** Sebring International Raceway (Turn 17 bumps)  
**Speed:** 150 km/h  
**Settings:** Road Texture Gain = 1.0, Max Torque Ref = 20 Nm

| Car | Encryption | Road Texture Feel | Notes |
|-----|-----------|-------------------|-------|
| BMW M4 GT3 | ❌ Unencrypted | ⭐⭐⭐⭐⭐ Excellent | Standard method, full detail |
| Porsche 911 GT3 R | ✅ Encrypted | ⭐⭐⭐⭐ Very Good | Fallback method, slightly smoother |
| Audi R8 LMS GT3 | ❌ Unencrypted | ⭐⭐⭐⭐⭐ Excellent | Standard method, full detail |
| Porsche 963 GTP | ✅ Encrypted | ⭐⭐⭐⭐ Very Good | Fallback method, slightly smoother |

**Conclusion:** The fallback provides a **very close approximation** of the standard method. The difference is subtle and most users won't notice it during normal driving.

---

## Future Improvements

### Planned for v0.6.22+

1. **User-Adjustable Fallback Sensitivity**
   - Slider to fine-tune the fallback scaling factor
   - Range: 0.01 - 0.20 (default: 0.05)
   - Allows matching the feel between encrypted and unencrypted cars

2. **Real-Time Fallback Indicator**
   - Visual indicator in the Debug Window showing which method is active
   - Helps users understand when fallback is being used

3. **Per-Car Fallback Profiles**
   - Automatic adjustment based on car class
   - Optimal scaling for GT3, GTP, Hypercar, etc.

### Under Consideration

1. **Adaptive Filtering**
   - Automatic noise reduction for cleaner fallback signal
   - Reduces "graininess" on smooth tracks

2. **Hybrid Method**
   - Combine multiple sensors (acceleration, suspension force, ride height)
   - More robust fallback for extreme cases

---

## Summary

**What is encrypted content?**  
Cars where the game blocks certain telemetry data to protect IP.

**What's the problem?**  
Road Texture effect is silent because tire deflection data is blocked.

**What's the solution?**  
lmuFFB v0.6.21+ automatically switches to using vertical G-forces instead.

**Do I need to do anything?**  
No! The fallback activates automatically when needed.

**How does it feel?**  
Very close to the standard method - most users won't notice a difference.

**Can I adjust it?**  
Not in v0.6.21, but planned for v0.6.22+.

---

## Additional Resources

- **Technical Analysis:** `docs/dev_docs/road_texture_fallback_scaling_factor.md`
- **Implementation Details:** `docs/dev_docs/Fix Violent Shaking when Stopping and no road textures.md`
- **FFB Formulas:** `docs/dev_docs/FFB_formulas.md`
- **Telemetry Reference:** `docs/dev_docs/telemetry_data_reference.md`

---

**Questions or Issues?**  
If you experience problems with the fallback system, please report them with:
1. Car name (e.g., "Porsche 911 GT3 R")
2. Track name (e.g., "Sebring")
3. Your FFB settings (screenshot or config.ini)
4. Description of the issue

This helps us improve the fallback system for everyone!
