# Marvin's AIRA - Calibration System & Accuracy Analysis Report

## Table of Contents
1. [Overview](#overview)
2. [Calibration System Architecture](#calibration-system-architecture)
3. [Bundled Calibration Data](#bundled-calibration-data)
4. [Calibration Sharing Mechanism](#calibration-sharing-mechanism)
5. [Accuracy Analysis](#accuracy-analysis)
   - [Does It Work Across All Tracks?](#does-it-work-across-all-tracks)
   - [Does It Work for All Car Setups?](#does-it-work-for-all-car-setups)
   - [Does It Work for All Track Conditions?](#does-it-work-for-all-track-conditions)
   - [Summary of Accuracy Factors](#summary-of-accuracy-factors)
6. [Theoretical Limitations](#theoretical-limitations)
7. [Practical Recommendations](#practical-recommendations)
8. [Conclusion](#conclusion)

---

## Overview

This report addresses key questions about the calibration system used by Marvin's AIRA for understeer/oversteer detection, including:
- Whether every user must perform calibration for each car
- Whether pre-calibrated data is bundled with the app
- Whether there's a sharing/contribution mechanism
- How accurate the system is across different conditions

---

## Calibration System Architecture

### Per-Car, Per-Setup Calibration Files

Calibration files are stored as CSV files with the naming convention:
```
{Car Screen Name} - {Car Setup Name}.csv
```

For example:
- `Ferrari 488 GT3 - baseline.csv`
- `BMW M4 GT3 - bathurst.csv`
- `Chevrolet Camaro ZL1 Class A - cota_gp.csv`

### File Location
Calibration files are stored in the user's Documents folder:
```
{UserDocuments}\MarvinsAIRA Refactored\Calibration\
```

### File Format
- **First line:** Version, Car Screen Name, Car Setup Name
- **Second line:** Header row (`Steering Wheel Angle,Yaw Rate`)
- **Data rows:** Pairs of steering angle (degrees) and normalized yaw rate (°/s/kph)

### Auto-Selection Behavior
When a user loads a car, the app:
1. Scans the Calibration folder for files matching `{Car Screen Name} - *.csv`
2. Populates the dropdown with all matching calibration files
3. If no calibration file was previously selected, it **auto-selects the first available match**

This means users don't need to manually select a calibration file if one already exists for their car.

---

## Bundled Calibration Data

### Yes, Calibration Files ARE Bundled With The App!

**Good news:** The app ships with **120+ pre-calibrated cars**, so most users will never need to perform calibration themselves.

The installer (InnoSetup) copies calibration files from:
```
InnoSetup\Calibration\*.csv
```
To:
```
{UserDocuments}\MarvinsAIRA Refactored\Calibration\
```

### List of Bundled Calibration Files

Based on the InnoSetup directory, the following cars have bundled calibration data:

#### GT3 Cars
- Acura NSX GT3 EVO 22
- Aston Martin Vantage GT3 EVO
- Audi R8 LMS EVO II GT3
- BMW M4 GT3 / M4 GT3 EVO
- Chevrolet Corvette Z06 GT3.R
- Ferrari 296 GT3 / 488 GT3 / 488 GT3 Evo 2020
- Lamborghini Huracan GT3 EVO
- McLaren 720S GT3 / GT3 Evo
- Mercedes AMG GT3 2020
- Porsche 911 GT3.R / 992 GT3.R

#### GT4 Cars
- Aston Martin Vantage GT4
- BMW M4 GT4
- Mercedes AMG GT4
- Porsche 718 Cayman GT4

#### Prototypes
- Acura ARX-06
- Audi R18 2016
- BMW M Hybrid V8
- Cadillac V-Series.R
- Corvette C7 Daytona Prototype
- Dallara P217 LMP2
- Ferrari 499P
- Porsche 963

#### Open Wheel
- Dallara IR01 / IR18 / IR-05 / DW-12
- Dallara F312 F3
- Dallara IL15 Indy NXT
- FIA F4
- Mercedes W14 F1

#### GTE/GT1
- Aston Martin DBR9 GT1
- BMW M8 GTE
- Chevrolet Corvette C6R GT1 / C8.R
- Ferrari 488 GTE
- Ford GT GTE
- Porsche 911 RSR / 992 GT3 Cup

#### Stock Cars & Ovals
- NASCAR Cup Series cars (Chevrolet, Ford, Toyota)
- ARCA cars
- 87 Ford Thunderbird
- Late Model Stock
- Street Stock

#### Dirt Racing
- Dirt 358 Modified
- Dirt Big Block Modified
- Dirt Midget
- Dirt Mini Stock
- Dirt UMP Modified

#### TCR & Touring
- Audi RS 3 LMS TCR
- Honda Civic Type R
- Hyundai Elantra N TC

#### Other
- FIA Cross Car
- Ford Mustang FR500S
- Global Mazda MX-5 Cup
- Lotus 79
- Nissan GTP ZX-T
- Radical SR10
- Various national series cars

### Setup Names Used
Most bundled calibrations use one of these setup names:
- `baseline` - The most common (works for most scenarios)
- `road` - For road course specific cars
- `centripetal` - Calibrated on the Centripetal Circuit test track
- `bathurst` - Track-specific calibration
- `cota_gp` - Track-specific calibration
- `limerock` - Track-specific calibration

---

## Calibration Sharing Mechanism

### Current State: No Built-in Sharing System

Based on the codebase analysis, **there is NO built-in mechanism for users to upload or share calibration files with the developer or other users.**

The CloudService component only handles:
- Checking for app updates
- Downloading new versions

There is **no endpoint or feature** for:
- Uploading calibration files
- Contributing calibration data to a community pool
- Downloading updated calibration data separately from app updates
- User accounts or login for contribution tracking

### How Bundled Calibrations Are Created

Based on the naming patterns (most use `baseline` or specific track names), the bundled calibration files appear to be:
1. **Created by the developer (Marvin Herbold)** using the calibration procedure
2. **Bundled with each new release** of the application
3. **Updated when new cars are added** to iRacing

### Manual Sharing
Users *can* manually share calibration files by:
1. Copying CSV files from their Calibration folder
2. Sharing via forums, Discord, email, etc.
3. Recipient places files in their Calibration folder
4. Files appear in the dropdown automatically

This is **not an integrated feature** but is technically possible due to the simple CSV file format.

---

## Accuracy Analysis

### Does It Work Across All Tracks?

#### Short Answer: **Yes, with caveats**

#### Technical Explanation

The calibration captures the relationship between steering angle and yaw rate at **low speed (15 kph)** with **full tyre grip**. This establishes the car's **geometric turning characteristics** which are determined by:
- Wheelbase length
- Steering gear ratio
- Front axle geometry (Ackermann, toe, caster)

These properties are **car-specific, not track-specific**. Therefore, a calibration file created at "Centripetal Circuit" should work at any other track.

#### Caveats

1. **Track Surface Grip:** The calibration assumes the tyres have full grip. If a track has much lower or higher baseline grip than where calibration was performed, the baseline expected yaw rate may be slightly off.

2. **Track Gradient:** The app calculates normalized yaw rate based on flat-surface physics. On steep hills or banked corners, the relationship between steering and yaw may differ slightly.

3. **Bumps and Elevation Changes:** Sudden elevation changes can momentarily affect yaw rate readings, potentially causing brief false triggers.

### Does It Work for All Car Setups?

#### Short Answer: **Mostly yes, but setup-dependent variations exist**

#### Technical Explanation

The key setup parameters that affect steering-to-yaw relationship are:
- **Front toe settings:** Affect turn-in response
- **Front camber:** Affects mechanical grip at various steering angles
- **Steering ratio/rack:** Changes how wheel angle translates to front wheel angle
- **Rear toe/camber:** Affects rear stability (oversteer tendency)
- **Anti-roll bars:** Affect weight transfer and balance
- **Ride height:** Affects suspension geometry

#### Implications

1. **Most setups will work with baseline calibration:** The differences between setups are typically small compared to the thresholds used for understeer detection.

2. **Extreme setups may require recalibration:** If a setup dramatically changes the car's handling balance (e.g., extreme front toe-out for aggressive turn-in, or very stiff front ARB), the baseline calibration may trigger too early or too late.

3. **The "baseline" calibration is intentionally neutral:** Using a baseline/default setup for calibration provides a middle-ground reference that works reasonably well for most configurations.

### Does It Work for All Track Conditions?

#### Rubbered vs. Green Track

| Condition | Impact on Detection | Notes |
|-----------|---------------------|-------|
| **Green track** | May trigger earlier | Less grip = understeer detected at lower lateral loads |
| **Rubbered track** | May trigger later | More grip = higher forces before tyres slip |

The thresholds (min/max) are user-configurable, allowing adjustment for different grip levels.

#### Track Temperature

iRacing provides `WeatherDeclaredWet` as telemetry but **does NOT expose track temperature or rubbering level** as real-time telemetry variables available to third-party apps.

**Impact:** The app cannot automatically compensate for temperature-related grip changes. Users may need to adjust thresholds for hot vs. cold track sessions.

#### Wet Track

The app **does** track wet conditions via the `WeatherDeclaredWet` iRacing variable and supports "per wet/dry" context switching. This means:
- Users can save **separate settings for wet and dry conditions**
- When iRacing declares the track wet, the app can automatically load wet-specific thresholds

However, **the calibration file itself is not adjusted for wet conditions**. Users would need to:
1. Create a separate calibration in wet conditions, OR
2. Adjust thresholds higher for wet conditions (since wet grip is lower)

#### Summary Table

| Condition | Works? | Notes |
|-----------|--------|-------|
| **Different tracks** | ✅ Yes | Same car geometry applies everywhere |
| **Different setups** | ✅ Mostly | May need threshold adjustment for extreme setups |
| **Green track** | ✅ Yes | May trigger earlier; adjust thresholds |
| **Rubbered track** | ✅ Yes | May trigger later; adjust thresholds |
| **Cold track** | ✅ Yes | Less grip = earlier trigger |
| **Hot track** | ✅ Yes | More grip = later trigger |
| **Wet track (declared)** | ⚠️ Limited | Per-wet-dry settings help, but may need higher thresholds |
| **Transitional (drying)** | ⚠️ Limited | No automatic tracking of drying line |

### Summary of Accuracy Factors

| Factor | Affects Calibration? | Affects Detection? | Can Be Compensated? |
|--------|---------------------|-------------------|---------------------|
| **Car model** | ✅ Primary factor | ✅ Yes | Per-car calibration |
| **Car setup** | ⚠️ Minor effect | ⚠️ Minor | Per-setup calibration available |
| **Track layout** | ❌ No | ❌ No | N/A |
| **Track surface grip** | ❌ No (calibrated at 15kph) | ⚠️ Minor | Threshold adjustment |
| **Track temperature** | ❌ Not exposed | ⚠️ Minor | Threshold adjustment |
| **Wet/dry** | ❌ No | ✅ Yes | Per-wet-dry settings |
| **Rubbering** | ❌ Not exposed | ⚠️ Minor | Threshold adjustment |
| **Tyre compound** | ❌ Not considered | ⚠️ Yes | Threshold adjustment |
| **Tyre wear** | ❌ Not considered | ⚠️ Yes | Dynamic (detection changes over stint) |
| **Fuel load** | ❌ Not considered | ⚠️ Very minor | Weight affects grip but minimally |

---

## Theoretical Limitations

### 1. Steady-State Assumption
The calibration captures **steady-state** yaw rate vs. steering angle. During **transient maneuvers** (quick direction changes, weight transfer), the instantaneous yaw rate may differ from the expected value even with full grip.

**Mitigation:** Users can increase the minimum threshold to filter out transient false positives.

### 2. Speed Normalization Linearity
The formula assumes yaw rate scales linearly with speed:
```
normalizedYawRate = YawRate / Speed
```

In reality, at very high speeds, **aerodynamic effects** change the relationship:
- Downforce increases grip
- Air resistance affects yaw damping

**Mitigation:** The speed threshold (1-20 kph fade-in) means effects only activate at "racing speeds" where these effects are consistent.

### 3. Single-Point Calibration
Calibration is performed at **one speed (15 kph)** and assumes the relationship scales to all speeds.

**Mitigation:** At 15 kph, there's essentially no aerodynamic influence, capturing the pure mechanical characteristics. This is actually **the most stable reference point**.

### 4. No Tyre Model Integration
The app doesn't know:
- Tyre compound
- Tyre temperature
- Tyre wear percentage
- Slip angle

It only observes the **outcome** (actual yaw rate vs. expected) rather than **modeling the cause** (tyre physics).

**Advantage:** This makes it universally applicable without per-tyre tuning.
**Disadvantage:** Cannot predict grip loss before it happens.

---

## Practical Recommendations

### For General Use
1. **Use the bundled calibration files** for most cars - they work well
2. **Adjust thresholds** rather than recalibrating for different conditions
3. **Start with default thresholds** and tune based on feel

### For Serious Competitive Use
1. **Create a custom calibration** for your primary car/setup combo
2. **Use per-setup context switching** to save different threshold settings
3. **Create separate wet calibrations** if you race in variable conditions

### For New/Unbundled Cars
1. **Perform calibration at Centripetal Circuit** as documented
2. Use the **baseline/default setup** for calibration
3. **Share your calibration** with the community if comfortable

### For App Developers Looking to Implement Similar Features
1. **Yaw rate / speed normalization** is the key insight
2. **Low-speed calibration** isolates mechanical characteristics
3. **Comparison to baseline** is more robust than absolute thresholds
4. **User-adjustable thresholds** are essential for personal preference
5. **Consider exposing per-condition settings** (wet/dry, hot/cold)

---

## Conclusion

### Does Every User Need to Calibrate?
**No.** The app bundles 120+ calibration files covering most popular iRacing cars. Users only need to calibrate for:
- Cars not included in the bundle
- Custom setups that significantly alter handling
- Creating personalized calibrations for competitive edge

### Is There a Sharing/Contribution System?
**No built-in system exists.** Calibration data is bundled by the developer with each release. Users can manually share CSV files, but there's no automated upload/download mechanism.

### How Accurate Is It?
**Quite accurate for its purpose**, with the following understanding:
- It detects **relative grip loss**, not absolute grip level
- It works across all tracks for a given car
- It's robust to setup changes for typical adjustments
- Extreme conditions (wet, temperature extremes) may require threshold tuning
- The user-configurable thresholds provide flexibility for different preferences

### Key Strength
The approach of comparing **actual behavior to calibrated baseline behavior** is elegant because it:
- Automatically accounts for each car's unique characteristics
- Doesn't require complex tyre physics modeling
- Works with any car iRacing adds
- Provides intuitive feedback that correlates with what drivers feel

### Key Limitation
**No automatic adaptation** to changing conditions (temperature, rubbering, tyre wear). Users must manually adjust thresholds or accept that sensitivity may vary throughout a session.
