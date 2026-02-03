# Slope Detection Diagnostic Driving Guide

This guide provides step-by-step instructions for generating telemetry data to diagnose issues with the **Slope Detection** feature in lmuFFB.

> **Purpose:** Help developers and testers capture reproducible telemetry data that can be analyzed to identify and fix slope detection problems.

---

## Prerequisites

Before starting the diagnostic session:

### 1. Enable Telemetry Logging
1. Open lmuFFB
2. Navigate to the **Troubleshooting** or **Advanced** section
3. Click **"Start Recording"** to begin telemetry logging
4. Verify the recording indicator shows "RECORDING"

### 2. Configure Slope Detection Settings
Ensure these settings are applied:

| Setting | Recommended Value | Notes |
|---------|-------------------|-------|
| **Enable Slope Detection** | ✅ ON | Required for diagnosis |
| **Filter Window** | 15 (default) | Standard latency |
| **Sensitivity** | 0.5 (default) | Normal response |
| **Negative Threshold** | -0.3 (default) | Standard trigger point |
| **Output Smoothing** | 0.04s (default) | Balanced smoothing |

### 3. Select a Known Car
For consistent results, use one of these verified cars:
- ✅ **Mercedes-AMG GT3** (Recommended - stable telemetry)
- ✅ **Porsche 911 GT3 R** (Good alternative)
- ⚠️ **McLaren 720S GT3** (Known issues - useful for comparison)

### 4. Select a Track
Choose a track with varied corner types:
- **Spa-Francorchamps** (Recommended - mix of fast and slow corners)
- **Barcelona-Catalunya** (Good alternative)

---

## Diagnostic Test Procedures

Complete all tests in order. Press the **"Mark Event"** button at key moments to create markers in the log that help with analysis.

---

### Test 1: Baseline (Straight Line at Speed)

**Purpose:** Establish baseline behavior. On straights, grip should remain at 100% (1.0).

**Duration:** ~30 seconds

**Procedure:**
1. Exit the pit lane and get up to **150+ km/h** on a straight section
2. **Hold the car straight** - no steering input
3. Maintain constant speed for **10 seconds**
4. Press **"Mark Event"** while driving straight
5. Continue straight for another **5 seconds**

**Expected Behavior:**
| Indicator | Expected Value | If Incorrect |
|-----------|----------------|--------------|
| Grip % | 95-100% | Slope is "stuck" at negative value |
| Slope Value | Near 0 | Algorithm calculating when it shouldn't |
| FFB Feel | Normal, consistent | Unwanted understeer modulation |

**What to Look For:**
- ❌ **Problem:** Grip drops below 90% on straights → Slope not decaying to neutral
- ✅ **Good:** Grip stays at 95-100% → Algorithm correctly inactive on straights

---

### Test 2: Steady-State Cornering

**Purpose:** Test slope detection during sustained cornering at consistent speed.

**Duration:** ~45 seconds

**Procedure:**
1. Find the **long sweeping left-hander at Eau Rouge approach** (Spa) or similar constant-radius corner
2. Enter the corner at a moderate, sustainable speed (~120 km/h)
3. **Maintain constant steering angle and speed** for at least **5 seconds**
4. Press **"Mark Event"** while in steady-state cornering
5. Hold the corner for another **5 seconds**
6. Exit the corner smoothly

**Expected Behavior:**
| Indicator | Expected Value | If Incorrect |
|-----------|----------------|--------------|
| Grip % | 80-95% (slight reduction) | Too high = not detecting; too low = over-aggressive |
| Slope Value | Slightly negative (-0.1 to -0.5) | Extreme values indicate noise |
| FFB Feel | Slight lightening proportional to grip reduction | Heavy jolts or oscillations |

**What to Look For:**
- ❌ **Problem:** Grip oscillates wildly between 30% and 100% → Slope calculation unstable
- ❌ **Problem:** Grip stays at 100% in corner → Detection not triggering
- ✅ **Good:** Grip progressively drops as you push harder, stable in steady-state

---

### Test 3: Corner Entry (Transient Response)

**Purpose:** Test how the algorithm responds to rapid steering input (turn-in).

**Duration:** ~60 seconds (3 corner entries)

**Procedure:**
1. Approach a **medium-speed corner** (e.g., La Source hairpin at Spa)
2. Brake to **60-80 km/h**
3. **Turn in briskly** (quick steering input)
4. Press **"Mark Event"** exactly at the moment of turn-in
5. Continue through the corner normally
6. Repeat for **3 different corners**

**Expected Behavior:**
| Phase | Grip % | Notes |
|-------|--------|-------|
| Approach (straight) | 100% | Full grip |
| Turn-in (0-1 second) | Gradual drop to 85-95% | Should NOT spike down to 20% |
| Mid-corner | Stable at 75-90% | Proportional to slip |
| Exit (straightening) | Returns to 100% | Should recover smoothly |

**What to Look For:**
- ❌ **Problem:** Grip spikes to 20% immediately on turn-in → dAlpha/dt threshold too low
- ❌ **Problem:** Strong FFB "hit" on turn-in → Slope calculating extreme values
- ✅ **Good:** Smooth, progressive grip reduction during turn-in

---

### Test 4: Zig-Zag Slalom (Rapid Direction Changes)

**Purpose:** Test algorithm stability during rapid left-right-left maneuvers.

**Duration:** ~30 seconds

**Procedure:**
1. Find a **wide straight section** or pit road
2. Reduce speed to **60 km/h** (typical speed limit)
3. Perform a **left-right-left-right slalom** (quick steering inputs)
4. Press **"Mark Event"** during the maneuver
5. Continue slalom for **10 seconds**
6. Straighten and continue normally

**Expected Behavior:**
| Indicator | Expected Value | If Incorrect |
|-----------|----------------|--------------|
| Grip % | Should fluctuate but NOT drop below 50% | Hits 20% floor = algorithm too aggressive |
| Slope Value | Will oscillate, but range should be -5 to +5 | Extreme -20 to +20 indicates noise |
| FFB Feel | May have slight modulation | Heavy "hits" or "jolts" indicate problem |

**What to Look For:**
- ❌ **Problem:** Grip repeatedly hits 20% floor → Sensitivity too high or threshold too low
- ❌ **Problem:** "Notchy" feeling with hard FFB hits → Oscillation in calculations
- ✅ **Good:** Some grip variation, but smooth and proportional to steering activity

---

### Test 5: Understeer Comparison (Static vs Dynamic)

**Purpose:** Compare understeer feel with Slope Detection ON vs OFF.

**Duration:** ~3 minutes

**Procedure:**

**Part A (Slope Detection ON):**
1. Ensure **Slope Detection is ENABLED**
2. Drive 3 laps at moderate pace, pushing slightly in corners
3. Pay attention to **understeer feel** in mid-corner
4. Note: Does FFB lighten progressively as you push harder?

**Part B (Slope Detection OFF):**
1. **DISABLE Slope Detection** (but keep Understeer Effect ON)
2. Drive 3 laps at the same pace
3. Compare the **understeer feel** to Part A
4. Note: Which felt more natural and progressive?

**Expected Comparison:**
| Aspect | With Slope Detection | Without Slope Detection |
|--------|---------------------|------------------------|
| Understeer onset | Earlier, more progressive | Later, more sudden |
| FFB reduction | Proportional to slip | Based on static threshold |
| Recovery | Gradual return of grip | Faster snap-back |
| Extreme understeer | Clear, distinct feel | Can feel more binary |

---

### Test 6: Car Comparison (If Reporting Car-Specific Issues)

**Purpose:** Identify if the issue is car-specific or universal.

**Duration:** ~5 minutes per car

**Procedure:**
1. Run Tests 1-4 with **Mercedes-AMG GT3** (baseline car)
2. Run Tests 1-4 with the **problematic car** (e.g., McLaren)
3. Use **Mark Event** at the same points in each session
4. Note any differences in:
   - Grip percentage ranges
   - Slope value ranges
   - FFB feel

**What to Report:**
- Specific car name where issues occur
- Comparison with a working car
- Screenshots of "Advanced Slope Settings" during problem moments

---

## How to Submit Diagnostic Data

After completing the tests:

### 1. Stop Recording
Click **"Stop Recording"** in lmuFFB. The log file will be saved automatically.

### 2. Locate Log Files
Log files are saved to:
```
%APPDATA%/lmuFFB/logs/
```

Filename format:
```
lmuffb_log_YYYY-MM-DD_HH-MM-SS_CarName_TrackName.csv
```

### 3. Prepare Submission
Include the following in your report:
- [ ] Log file(s) from the diagnostic session
- [ ] Screenshot of your FFB settings
- [ ] Description of the issue (what you expected vs what happened)
- [ ] Car and track used
- [ ] lmuFFB version number

### 4. Submit
- **GitHub Issue:** Attach files to a new issue on the lmuFFB repository
- **Discord:** Share in the #bug-reports channel
- **Email:** Send to the developer (if applicable)

---

## Interpreting Your Results

### Signs of a Working Slope Detection:
✅ Grip at 95-100% on straights  
✅ Gradual grip reduction in corners  
✅ Smooth FFB feel, no jolts  
✅ Slope values predominantly in -2 to +2 range  
✅ Recovery to full grip when straightening  

### Signs of a Problem:
❌ Grip dropping below 50% on straights  
❌ Grip hitting 20% floor frequently  
❌ Slope values oscillating to ±10 or beyond  
❌ "Notchy" or "heavy" FFB feel  
❌ Strong FFB hits on turn-in  
❌ Constant understeer feel even on straights  

---

## Quick Reference Card

| Test | Speed | Duration | Key Observation |
|------|-------|----------|-----------------|
| 1. Straight Line | 150+ km/h | 30s | Grip should stay 95-100% |
| 2. Steady Corner | 120 km/h | 45s | Grip stable, slight reduction |
| 3. Corner Entry | 60-80 km/h | 60s | Smooth progressive reduction |
| 4. Zig-Zag | 60 km/h | 30s | No 20% floor hits |
| 5. Comparison | Varies | 3 min | Compare ON vs OFF |
| 6. Car Test | Varies | 5 min | Compare cars |

---

## Troubleshooting Common Issues During Testing

### "Telemetry Logging Won't Start"
- Ensure you're connected to the game (telemetry active)
- Check that the logs folder exists and is writable
- Try restarting lmuFFB

### "Mark Event Button Not Responding"
- Markers only work while recording is active
- Verify the "RECORDING" indicator is visible

### "Log File Too Small (Few Frames)"
- Ensure you drove for at least 30 seconds
- Check that the game was running during recording
- Verify telemetry connection is established

---

*Guide Version: 1.0*  
*Compatible with lmuFFB v0.7.2+*  
*Last Updated: 2026-02-03*
