# Slope Detection Algorithm - Technical Guide

**Version:** 0.7.1  
**Status:** Stable / Recommended  
**Last Updated:** February 2, 2026

---

## Table of Contents

1. [Overview](#overview)
2. [Why Slope Detection?](#why-slope-detection)
3. [How It Works](#how-it-works)
4. [Understanding the Settings](#understanding-the-settings)
5. [Latency Explained](#latency-explained)
6. [Tuning Guide](#tuning-guide)
7. [Troubleshooting](#troubleshooting)
8. [Technical Deep Dive](#technical-deep-dive)

---

## Overview

**Slope Detection** is an adaptive algorithm in lmuFFB v0.7.1 that dynamically estimates tire grip by monitoring the **rate of change** (slope) of the tire's performance curve in real-time, rather than using static thresholds.

**Key Benefits:**
- 🎯 **Adaptive** - Automatically adjusts to different tire compounds, temperatures, and wear states
- 🏁 **Track-Agnostic** - Works consistently across all tracks without manual tuning
- 📊 **Physically Accurate** - Detects the actual saturation point of the tire, not an arbitrary threshold
- 🔄 **Dynamic** - Responds to changing conditions during the session (tire warm-up, degradation)

**Trade-off:**
- Introduces small latency (6-50ms depending on settings) due to signal processing

---

## Why Slope Detection?

### The Problem with Static Thresholds

Traditional grip estimation uses **fixed thresholds** like "Optimal Slip Angle = 0.10 rad". This approach has several limitations:

❌ **One-Size-Fits-All** - A single threshold can't accommodate:
- Different tire compounds (street vs slick)
- Temperature variations (cold vs optimal vs overheated)
- Tire wear progression
- Different car setups (high vs low downforce)

❌ **Binary Detection** - Either you're below the threshold (full grip) or above it (reduced grip), with no smooth transition representing the actual tire curve

❌ **Requires Manual Tuning** - Users must adjust the threshold for each car/track combination to get realistic feedback

### The Tire Performance Curve

Real tires follow a characteristic performance curve:

```
Lateral Force (G)
    │
1.8 │         ╭─────╮         ← Peak (Maximum Grip)
    │        ╱       ╲
1.5 │       ╱         ╲
    │      ╱           ╲╲
1.2 │     ╱             ╲╲    ← Post-Peak (Sliding)
    │    ╱               ╲╲╲
0.9 │   ╱                 ╲╲╲
    │  ╱
0.6 │ ╱
    │╱
    └────────────────────────── Slip Angle (rad)
    0   0.05  0.10  0.15  0.20
    
        ↑              ↑
    Building Grip   Saturating
```

**Key Insight:** The **slope** (steepness) of this curve tells us where we are:
- **Positive slope (rising):** Tire is building grip - safe to push harder
- **Zero slope (plateau):** Tire is at peak grip - maximum cornering force
- **Negative slope (falling):** Tire is saturating - sliding/losing grip

Slope Detection monitors this slope in real-time to provide accurate grip feedback regardless of the tire's characteristics.

---

## How It Works

### The Algorithm (Simplified)

1. **Data Collection:** Every 2.5ms (400Hz), lmuFFB receives:
   - Lateral G-force (how much the car is cornering)
   - Slip Angle (difference between where the tire points vs where it's moving)

2. **Buffering:** Recent samples are stored in a circular buffer (5-41 samples depending on settings)

3. **Derivative Calculation:** A **Savitzky-Golay (SG) filter** calculates the slope:
   ```
   Slope = Change in Lateral G ÷ Change in Slip Angle
   ```
   
4. **Grip Factor Estimation:**
   - **Positive slope** → Grip Factor = 1.0 (100% grip)
   - **Negative slope** → Grip Factor decreases based on how steep the decline is

5. **FFB Adjustment:** The Understeer Effect is scaled by the grip factor:
   ```
   FFB Force = Base Force × (1.0 - Grip Loss × Understeer Effect)
   ```

### Why Savitzky-Golay Filtering?

Telemetry data is **noisy**. Raw derivatives would produce this:

```
Raw Derivative (No Filtering)
    │  ╱╲  ╱╲╱╲
    │ ╱  ╲╱    ╲╱╲  ╱╲
────┼──────────────────────  → Unusable (too jittery)
    │      ╲╱      ╲╱
```

SG filtering smooths the signal while preserving the true trend:

```
SG Filtered Derivative
    │      ╭───────╮
    │     ╱         ╲
────┼────────────────────────  → Clean, usable slope
    │                 ╲
```

**SG filtering provides:**
✅ Smooth derivatives from noisy data  
✅ Preserves the shape of the underlying signal  
✅ Mathematically rigorous (fits a polynomial to the data window)  
✅ Widely used in scientific data analysis

---

## Understanding the Settings

### Enable Slope Detection
**Toggle:** ON / OFF  
**Default:** OFF

When enabled, replaces the static "Optimal Slip Angle" threshold with dynamic slope monitoring. The "Optimal Slip Angle" and "Optimal Slip Ratio" settings are ignored when this is ON.

> ⚠️ **Note:** When Slope Detection is enabled, **Lateral G Boost (Slide)** is automatically disabled to prevent oscillations caused by asymmetric calculation methods between axles.

---

### Filter Window
**Range:** 5 - 41 samples  
**Default:** 15 samples  
**Unit:** Number of telemetry samples

**What it does:** Determines how many recent samples are used to calculate the slope.

**Effect on Latency:**
```
Latency (ms) = (Window Size ÷ 2) × 2.5ms

Examples:
Window =  5  →  6.25 ms latency  (Very Responsive, Noisier)
Window = 15  → 18.75 ms latency  (Default - Balanced)
Window = 21  → 26.25 ms latency  (Smooth, Higher Latency)
Window = 31  → 38.75 ms latency  (Very Smooth, Sluggish)
```

**Must be ODD:** The algorithm requires an odd number for symmetry (5, 7, 9, 11, ... 41). The GUI enforces this automatically.

**Tuning Guidance:**
- **Start with 15** - Good balance for most users
- **Lower to 7-11** if you want sharper, more immediate feedback (accept some noise)
- **Raise to 21-31** if FFB feels jittery or twitchy (smoother but slower)

---

### Sensitivity
**Range:** 0.1x - 5.0x  
**Default:** 0.5x (v0.7.1)
**Unit:** Multiplier

**What it does:** Scales how aggressively the slope is converted to grip loss.

**Effect:**
- **1.0x:** Normal sensitivity (recommended starting point)
- **<1.0x:** Less sensitive - wheel stays heavier even when sliding
- **>1.0x:** More sensitive - wheel lightens more dramatically when sliding

**Example:**
```
Slope = -0.15 (tire is past peak)
Sensitivity = 1.0x  →  Grip Factor = 0.85 (wheel lightens 15%)
Sensitivity = 2.0x  →  Grip Factor = 0.70 (wheel lightens 30%)
Sensitivity = 0.5x  →  Grip Factor = 0.92 (wheel lightens 8%)
```

**Tuning Guidance:**
- Increase if you want a more pronounced "light wheel" warning
- Decrease if the understeer effect feels too exaggerated
- Interacts with "Understeer Effect" slider - adjust both together

---

### Advanced Settings (Collapsed by Default)

#### Slope Threshold
**Range:** -1.0 to 0.0  
**Default:** -0.3 (v0.7.1)
**Unit:** (Lateral G / Slip Angle)

The slope value below which grip loss begins. More negative = later detection.

**Most users should leave this at default.** This is a safety parameter to prevent false positives from measurement noise.

---

#### Output Smoothing
**Range:** 0.005s - 0.100s  
**Default:** 0.040s (40ms) (v0.7.1)
**Unit:** Seconds (time constant)

Applies an exponential moving average to the final grip factor to prevent abrupt FFB changes.

**Formula:**
```
α = dt / (tau + dt)
Smoothed Output = Previous Output + α × (New Grip Factor - Previous Output)
```

**Higher values** = slower transitions (smoother FFB)  
**Lower values** = faster transitions (more responsive)

**Tuning Guidance:**
- Default (0.02s) is well-tested
- Reduce to 0.01s if FFB feels laggy
- Increase to 0.05s if you experience sudden FFB jerks

---

### Live Diagnostics
```
Live Slope: 0.142 | Grip: 100% | Cur derivative: -0.15
```

**Live Slope:** The current filtered slope value used for grip loss calculation.  
**Grip:** The current grip percentage (100% = full grip, lower = reduced grip).  
**Slope Graph:** A live scrolling graph available in the "Internal Physics" header to visualize the tire curve derivative.

Use these to understand what the algorithm is detecting during driving.

---

## Latency Explained

### What is Latency?

**Latency** is the time delay between a physical event (tire starts to slide) and when you feel it in the FFB.

**Sources of latency in lmuFFB:**
1. **Game Engine:** 2.5ms per frame (400Hz physics)
2. **SG Filter:** (Window / 2) × 2.5ms
3. **Output Smoothing:** ~1× tau (default 20ms)
4. **DirectInput API:** 1-2ms
5. **Wheel Hardware:** 5-30ms (varies by model)

**Total typical latency:** 25-60ms with default settings

---

### Why Can't We Have 0 Latency?

**Short Answer:** You cannot calculate a **derivative** (rate of change) from a single instant in time. You need at least 2 samples, which introduces delay.

**Long Answer:**

#### 1. Derivatives Require Time Comparison

To calculate slope, you need:
```
Slope = (Later Value - Earlier Value) / Time Between Them
         ↑               ↑
      Future         Past
```

You **cannot** know the "later value" until time has passed. This is a fundamental limitation of physics, not a software issue.

**Minimum possible latency:** 1 sample = 2.5ms (using only the current and previous frame)

---

#### 2. Noise Requires Averaging

Raw telemetry looks like this:
```
Sample 1: Lateral G = 1.523
Sample 2: Lateral G = 1.487  (Dropped 0.036 G in 2.5ms?!)
Sample 3: Lateral G = 1.531  (Jumped 0.044 G?!)
Sample 4: Lateral G = 1.509
```

If we calculate slope from just 2 samples:
```
Slope = (1.487 - 1.523) / 0.0025 = -14.4  ← FALSE ALARM!
```

The tire isn't actually losing grip - this is just **measurement noise** from:
- Suspension vibrations
- Track surface bumps
- Numerical precision limits
- Sensor sampling artifacts

**SG filtering averages multiple samples** to reveal the true trend:
```
5-sample average: Slope = -0.03  (Slight decline - tire at peak)
```

This is the correct reading, but it requires looking at 5 samples = 12.5ms of history.

---

#### 3. The Latency-Noise Trade-off

```
Window Size vs Performance

Latency (ms)
    │
 50 │                         ● (Window=41)
    │                    ●
 40 │               ●
    │          ●
 30 │     ●                    ← Sweet Spot (Window=15-21)
    │ ●                           Good balance
 20 │●
    │
 10 │● (Window=5)              ← Too Jittery
    │
  0 └────────────────────────────────── Noise Rejection
      Low                          High
      
      ↑                             ↑
   Fast but noisy            Smooth but laggy
```

**The goal:** Find the smallest window that still filters noise effectively.

**Why 5-41 is the range:**
- **Minimum (5):** Below this, noise dominates and the slope calculation is unreliable
- **Maximum (41):** Beyond this, latency becomes perceptible and the tire state may have changed significantly

---

### Why Low Latency Isn't Always Better

**Human perception:** Studies show humans cannot detect FFB changes faster than ~15-20ms. Below this threshold, you're fighting noise for zero perceptual benefit.

**Example:**
```
Scenario: You enter a corner too fast

Window = 5 (6ms latency):
  FFB becomes jittery and unstable as noise causes false grip loss signals
  → Confused driver, harder to correct

Window = 15 (19ms latency):
  FFB smoothly transitions as tire approaches and exceeds peak grip
  → Clear feedback, easy to modulate

Window = 31 (39ms latency):
  FFB change lags behind, tire may have recovered grip before wheel lightens
  → Delayed feedback, harder to react
```

**Best practice:** Use the **largest window that still feels responsive enough** for your driving style. More filtering = more reliable signal.

---

## Tuning Guide

### Quick Start (Recommended Settings)

**For Most Users:**
```
Enable Slope Detection: ON
Filter Window: 15
Sensitivity: 0.5 (Default)
```

Then adjust "Understeer Effect" slider (in the main Front Axle section) to taste:
- **0.5-0.7:** Subtle grip loss feel
- **1.0:** Realistic 1:1 grip scaling (recommended)
- **1.5-2.0:** Exaggerated light-wheel warning

---

### Fine-Tuning Process

#### Step 1: Find Your Ideal Window Size

1. **Enable Slope Detection** and set Sensitivity to 1.0
2. **Drive a familiar corner** at high speed (one where you know the limit)
3. **Experiment with window sizes:**

**If FFB feels jittery/noisy:**
- Increase window: 15 → 21 → 27
- Check live diagnostics - slope should be smooth, not jumping wildly

**If FFB feels laggy/delayed:**
- Decrease window: 15 → 11 → 7
- Wheel should lighten BEFORE you feel the backend step out

**Target:** Smooth transition as you approach the limit, without feeling disconnected

---

#### Step 2: Adjust Sensitivity

Once window size feels right:

**If wheel doesn't lighten enough when understeering:**
- Increase Sensitivity: 1.0 → 1.5 → 2.0
- OR increase "Understeer Effect" in the main settings

**If wheel becomes too light, feels unrealistic:**
- Decrease Sensitivity: 1.0 → 0.7 → 0.5
- OR decrease "Understeer Effect" in the main settings

---

#### Step 3: Validate Across Conditions

Test in different scenarios:
- **Cold tires** (first lap out of pits): Should feel more grip loss
- **Optimal temp** (3-5 laps in): Should feel most responsive
- **Worn tires** (late stint): Should feel gradual degradation

If slope detection adapts well to these changing conditions without manual adjustment, you've found your sweet spot!

---

### Presets for Different Wheel Types

#### Direct Drive (High Bandwidth, Low Noise)
```
Filter Window: 11
Sensitivity: 1.2
Output Smoothing: 0.015s
```
DD wheels can handle faster response without feeling twitchy.

---

#### Belt Drive (Moderate Bandwidth)
```
Filter Window: 15  (Default)
Sensitivity: 1.0
Output Smoothing: 0.020s
```
Balanced settings work best.

---

#### Gear Drive (Lower Bandwidth, More Noise)
```
Filter Window: 21
Sensitivity: 0.8
Output Smoothing: 0.030s
```
Gear-driven wheels benefit from more smoothing to hide mechanical noise.

---

## Troubleshooting

### "FFB feels jittery, wheel shakes when cornering"

**Cause:** Filter window too small, noise not being filtered effectively

**Solution:**
1. Increase Filter Window: Try 21, 25, or 31
2. Increase Output Smoothing: 0.04s → 0.06s
3. Lower Sensitivity: 0.5 → 0.3
4. Check that window size is **odd** (should be automatic)

---

### "Wheel lightens too late, I'm already sliding"

**Cause:** Too much latency

**Solution:**
1. Decrease Filter Window: Try 11, 9, or 7
2. Decrease Output Smoothing: 0.02s → 0.01s
3. Check live diagnostics - slope should go negative BEFORE you feel understeer

---

### "Wheel doesn't lighten at all when I push too hard"

**Possible Causes:**

**A. Slope Detection not actually enabled**
- Check the toggle is ON
- Restart lmuFFB if in doubt

**B. Sensitivity too low**
- Increase Sensitivity to 1.5 or 2.0
- Check "Understeer Effect" slider isn't set to 0

**C. Live diagnostics show slope staying positive**
- You may not be pushing hard enough to exceed peak grip
- Try a slower corner where you can really overdrive

---

### "Ride height or suspension changes affect FFB"

**Cause:** Slope detection is working correctly! Different ride heights change the tire's slip angle characteristics.

**This is a feature, not a bug.** The algorithm adapts to your setup changes, just like a real car would feel different with different suspension settings.

---

### "Oscillations when turning in or cornering"

**Cause:** Conflict between Slope Detection (Front only) and Lateral G Boost (Global effect).

**Fix in v0.7.1:** lmuFFB now automatically disables **Lateral G Boost (Slide)** when Slope Detection is enabled. This eliminates the feedback loop that caused oscillations in v0.7.0. If you still experience oscillations, try increasing the **Filter Window** (e.g., 21 or 25).

---

### "Slope goes negative randomly even on straights"

**Cause:** Measurement noise, possibly from:
- Curbs/bumps
- Flatspotted tires
- Very low downforce setups

**Solution:**
1. Increase Filter Window to 25+
2. Increase Slope Threshold (make more negative): -0.1 → -0.15
3. Consider using static threshold method for this car/track

---

## Technical Deep Dive

### Savitzky-Golay Mathematics

The SG filter fits a **quadratic polynomial** to the data window and analytically computes its derivative.

**For a window of size M (half-width):**

```math
Derivative Coefficient w_k = k × (Scaling Factor)

where Scaling Factor = 2M(M+1) / [(2M+1) × S₂]

and S₂ = M(M+1)(2M+1) / 3
```

**Example for Window=15 (M=7):**
```
Coefficients: w[-7] = -7×SF, w[-6] = -6×SF, ... w[0] = 0, ... w[7] = 7×SF

Derivative = Σ(w[k] × G[k]) / (Δt × Σ|w[k]|)
```

**Key properties:**
- Coefficients are anti-symmetric: w[-k] = -w[k]
- Center point has zero weight (doesn't bias toward current value)
- Edge points have maximum weight (captures the overall trend)

**Reference:** Savitzky, A.; Golay, M.J.E. (1964). "Smoothing and Differentiation of Data by Simplified Least Squares Procedures". *Analytical Chemistry* 36 (8): 1627-1639.

---

### Grip Factor Calculation

```cpp
// Calculate slope
double slope = dG_dt / dAlpha_dt;  // Units: G-force per radian

// Convert slope to grip loss
if (slope < m_slope_negative_threshold) {
    double raw_loss = -slope × m_slope_sensitivity;
    current_grip_factor = 1.0 - raw_loss;
} else {
    current_grip_factor = 1.0;  // Full grip
}

// Safety clamp
current_grip_factor = max(0.2, min(1.0, current_grip_factor));

// Apply smoothing (EMA)
double alpha = dt / (m_slope_smoothing_tau + dt);
m_slope_smoothed_output = m_slope_smoothed_output + alpha × (current_grip_factor - m_slope_smoothed_output);
```

**Safety Features:**
- Grip factor cannot go below 0.2 (20%) to prevent complete FFB loss
- Slope spikes are ignored when slip angle isn't changing (dt < 0.001)
- Slope detection only applies to front axle (lateral G is a whole-vehicle measurement)

---

### Front Axle Only

**Why?** Lateral G-force is measured at the **center of mass**, not at individual tires. The slope of G-vs-slip represents the **vehicle's overall lateral response**, which is dominated by the front tires (steering inputs).

**Rear grip** continues to use static threshold detection based on wheel slip and slip angle, which is appropriate for detecting oversteer and rear grip loss.

This asymmetry is intentional and physically justified.

---

## Comparison: Slope Detection vs Static Threshold

| Aspect | Static Threshold | Slope Detection |
|--------|------------------|-----------------|
| **Adaptability** | Fixed, must be tuned per car/track | Automatically adapts to conditions |
| **Latency** | ~0ms (instant) | 6-50ms (configurable) |
| **Accuracy** | Approximation based on arbitrary threshold | Detects actual tire saturation point |
| **Tuning Required** | High (Optimal Slip Angle must be dialed in) | Low (mostly set-and-forget) |
| **Noise Sensitivity** | Low (simple comparison) | Medium (requires filtering) |
| **Tire Degradation** | No adaptation (threshold stays fixed) | Adapts as grip degrades |
| **Best For** | Users who want instant response and minimal latency | Users who want accurate, adaptive, and organic understeer feel |

**Both methods are valid.** Choose the one that matches your priorities.

---

## Frequently Asked Questions

### Q: Should I use Slope Detection or stick with Static Threshold?

**Use Slope Detection if:**
- You drive multiple car types (GT3, LMP2, etc.) and want one setting to work for all
- You want realistic tire warm-up and degradation feel
- You're willing to accept 15-25ms of latency for more accurate feedback

**Use Static Threshold if:**
- You prioritize absolute minimum latency
- You drive mostly one car and can dial in the perfect threshold
- You prefer the "binary" feel of grip/no-grip

---

### Q: Can I use Slope Detection for drifting?

**Not recommended.** Drifting involves sustained large slip angles where the tire is always past peak. Slope detection assumes you're operating near the peak and transitioning across it.

For drifting, use:
- Static threshold method with high Optimal Slip Angle (0.15-0.20)
- Focus on "Rear Align Torque" and "Oversteer Boost" effects
- Enable "Slide Texture" for scrub feedback

---

### Q: Does Slope Detection work with keyboard/mouse?

**Yes**, but you won't feel the FFB changes (obviously). The grip estimation still happens and affects the car's behavior if FFB is routed to vJoy or similar virtual controller.

---

### Q: Why is my latency different from the displayed value?

The displayed latency is **only the SG filter contribution**. Total latency includes:
- Game engine frame time
- Output smoothing
- DirectInput API
- Your wheel's internal processing

Actual perceived latency will be higher than displayed.

---

### Q: Can I reduce latency below 6ms?

**Current answer (v0.7.0):** Not without bypassing the SG filter entirely. A 1-sample derivative (2.5ms latency) is possible but:
- Would be extremely noisy
- Defeats the purpose of slope detection
- Better to just disable the feature

**6ms (Window=5) is the practical minimum** for usable slope calculation.

**Coming in v0.7.1:** We're planning to lower the minimum window size from 5 to 3, which will provide:
- **3.75ms latency** (40% reduction from current minimum)
- Still mathematically valid (SG filtering requires minimum 3 points)
- Warning tooltip for users selecting very small windows
- Suitable for Direct Drive wheels and advanced users who can tolerate some noise

This will be an **optional enhancement** - the default will remain at window=15 for most users.

---

## What's Next? (Planned for v0.7.1)

We're actively improving Slope Detection based on early feedback:

### Ultra-Low Latency Option
**Coming Soon:** Filter Window minimum will be lowered from 5 to 3 samples, providing:
- **3.75ms minimum latency** (down from 6.25ms)
- Dynamic warning tooltips for aggressive settings
- Better suited for high-bandwidth Direct Drive wheels

### Your Feedback Matters
This feature is **experimental in v0.7.0**. Please share your experiences:
- What window size works best for your wheel?
- Does the default sensitivity (1.0x) feel realistic?
- Any scenarios where slope detection doesn't work well?

Your input helps shape the next version! 🏁

---

## Conclusion

Slope Detection represents a significant advancement in FFB grip estimation, moving from static approximations to dynamic, physics-based detection. With proper tuning, it provides adaptive, consistent feedback across all driving conditions.

**Recommended approach:**
1. Start with defaults (Window=15, Sensitivity=1.0)
2. Adjust window size for your wheel type and driving style
3. Fine-tune sensitivity to match your preference
4. Let the algorithm adapt - avoid constant tweaking

**Remember:** This feature is **experimental** in v0.7.0. Your feedback helps improve it for future releases. Share your findings on the forum!

---

## Additional Resources

- **Implementation Plan:** `docs/dev_docs/plans/plan_slope_detection.md`
- **Code Review:** `docs/dev_docs/code_reviews/code_review_slope_detection.md`
- **Source Code:** `src/FFBEngine.h` (lines 784-818, 862-934)
- **Forum Thread:** [Link to be added]

---

**Document Version:** 1.1 (v0.7.1)
**lmuFFB Version:** 0.7.1  
**Author:** lmuFFB Development Team  
**License:** This document is distributed with lmuFFB under the same MIT license.
