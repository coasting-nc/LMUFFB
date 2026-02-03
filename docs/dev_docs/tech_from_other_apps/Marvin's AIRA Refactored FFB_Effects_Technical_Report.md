# Marvin's AIRA - Force Feedback Effects Technical Report

## Table of Contents
1. [Overview](#overview)
2. [Understeer Effect](#understeer-effect)
   - [Core Concept](#understeer-core-concept)
   - [iRacing Physics Data Used](#understeer-iracing-physics-data-used)
   - [Mathematical Formulas](#understeer-mathematical-formulas)
   - [Calibration Process](#calibration-process)
   - [Effect Output Application](#understeer-effect-output-application)
3. [Tyre Grip Estimation](#tyre-grip-estimation)
4. [Oversteer Effect](#oversteer-effect)
5. [Seat-of-Pants Effect](#seat-of-pants-effect)
6. [Other FFB Effects](#other-ffb-effects)
   - [ABS Vibration Effect](#abs-vibration-effect)
   - [Gear Change Vibration Effect](#gear-change-vibration-effect)
   - [Crash Protection](#crash-protection)
   - [Curb Protection](#curb-protection)
   - [Soft Lock](#soft-lock)
   - [LFE (Low Frequency Effects)](#lfe-low-frequency-effects)
   - [Wheel Lock Detection](#wheel-lock-detection)
   - [Wheel Spin Detection](#wheel-spin-detection)
7. [Core FFB Processing Algorithms](#core-ffb-processing-algorithms)
8. [Helper Functions Reference](#helper-functions-reference)
9. [Signal Processing Techniques](#signal-processing-techniques)
   - [Smoothstep Speed Gating](#smoothstep-speed-gating)
   - [Threshold System (Min/Max with Linear Interpolation)](#threshold-system-minmax-with-linear-interpolation)
10. [Summary](#summary)


---

## Overview

Marvin's AIRA (Advanced iRacing Assistant) is a Force Feedback enhancement application for iRacing. It processes telemetry data from iRacing at 60Hz and 360Hz and generates additional FFB effects such as understeer, oversteer, and seat-of-pants feedback. The effects are applied to both the steering wheel (via DirectInput FFB) and optionally to haptic pedals (Simagic HPR pedals).

The application uses the **IRSDKSharper** library to receive telemetry data from iRacing at 60Hz with additional 360Hz sub-samples for high-resolution steering wheel torque data.

---

## Understeer Effect

### Understeer Core Concept

The understeer effect detects when the car is **understeering** (i.e., the front tyres are losing grip and the car is turning less than expected for the given steering input). This is achieved by comparing the **actual yaw rate** to an **expected yaw rate** based on a pre-calibrated steering angle vs. yaw rate curve.

**Key Principle:**
- **Calibration establishes baseline:** During a calibration run at constant low speed (15 kph), the app records the relationship between steering angle and yaw rate for the specific car.
- **Real-time comparison:** During normal driving, the app compares the current yaw rate to the expected yaw rate for the current steering angle.
- **Negative deviation = understeer:** If the actual yaw rate is *less* than expected, the front tyres are slipping (understeer).
- **Positive deviation = oversteer:** If the actual yaw rate is *greater* than expected, the rear tyres are slipping (oversteer).

### Understeer iRacing Physics Data Used

The following iRacing telemetry variables are used for the understeer effect:

| Variable Name | Type | Unit | Description |
|---------------|------|------|-------------|
| `SteeringWheelAngle` | float | radians | Current steering wheel angle (positive = right, negative = left) |
| `SteeringWheelAngleMax` | float | radians | Maximum steering wheel rotation for this car |
| `YawRate` | float | rad/s | Rate of yaw rotation (how fast the car is turning) |
| `Speed` | float | m/s | Car body speed (longitudinal) |
| `VelocityY` | float | m/s | Lateral velocity (side-slip velocity, used for constant force direction) |

> **Note:** iRacing does **not** directly provide tyre grip level as a real-time telemetry variable. The understeer/oversteer detection is achieved by comparing expected vs. actual yaw rates, which is an indirect measure of grip loss.

### Understeer Mathematical Formulas

#### Speed Normalization
The yaw rate is **normalized by speed** to make comparisons valid across different velocities:

```
normalizedYawRate = |YawRate| × (180/π) ÷ Speed_kph
```

Where:
- `Speed_kph = Speed × (18/5)` (conversion from m/s to kph)
- `(180/π) ≈ 57.2958` (conversion from radians to degrees)

The normalized yaw rate has units of **degrees per second per kilometer per hour** (°/s/kph), which represents how many degrees the car turns per second for each kph of speed.

#### Steering Angle Conversion
```
steeringAngle_degrees = SteeringWheelAngle × (180/π) - SteeringOffset
```

Where `SteeringOffset` is the car's configured steering offset from the setup.

#### Expected Yaw Rate Lookup
The expected yaw rate is obtained from a pre-calibrated lookup table indexed by steering angle. Linear interpolation is used between table entries:

```
// Compute float index into the expected yaw rate array
angleIndex = clamp(steeringAngle_degrees + 450, 0, 900)

// Get the two nearest integer indices
lowerIndex = floor(angleIndex)
upperIndex = ceiling(angleIndex)

// Linearly interpolate
t = angleIndex - lowerIndex
expectedYawRate = lerp(expectedYawRate[lowerIndex], expectedYawRate[upperIndex], t)
```

#### Deviation Calculation
```
deviation = actualNormalizedYawRate - expectedNormalizedYawRate
```

**Interpretation:**
- `deviation < 0` → **Understeer** (car turning less than expected)
- `deviation > 0` → **Oversteer** (car turning more than expected)

#### Understeer Effect Intensity
```
if deviation < 0 AND |deviation| >= understeerMinThreshold:
    understeerEffect = inverseLerp(understeerMinThreshold, understeerMaxThreshold, |deviation|)
else:
    understeerEffect = 0
```

Where `inverseLerp(min, max, value)` returns:
```
inverseLerp = clamp((value - min) / (max - min), 0, 1)
```

#### Speed Fade (Low Speed Suppression)
To avoid false triggering at very low speeds, effects are faded out:

```
speedFade = smoothstep(1, 20, Speed_kph)
```

Where `smoothstep(start, end, t)` is:
```
t = clamp((t - start) / (end - start), 0, 1)
smoothstep = t × t × (3 - 2t)
```

**Final understeer effect:**
```
UndersteerEffect = speedFade × understeerEffect
```

The `UndersteerEffect` is a value in the range **[0, 1]** where:
- `0` = no understeer
- `1` = maximum understeer (deviation ≥ maximum threshold)

### Calibration Process

#### Purpose
Calibration creates a baseline curve of "expected yaw rate vs. steering angle" for a specific car. This is critical because different cars have different steering ratios and handling characteristics.

#### Calibration Procedure
1. The car is driven at a **constant speed of 15 kph** in first gear
2. The steering wheel is swept from **full left to full right** (or within the car's maximum steering angle, up to ±450°)
3. At each integer degree of steering angle, **10 samples** (1/6 second at 60Hz) of yaw rate are collected and averaged
4. The raw data is smoothed using a **20-point moving average filter**
5. The smoothed curve is stored in a CSV file for later use

#### Calibration Data Format
The calibration file contains:
- **Version header:** File format version (currently version 2)
- **Data rows:** `steering_angle_degrees, normalized_yaw_rate`

#### Yaw Rate Normalization During Calibration
```
calibrationYawRate = |YawRate| × (180/π) ÷ Speed_kph
```

This is identical to the real-time calculation, ensuring consistency.

#### Post-Processing of Calibration Data
1. **Binary search interpolation:** The raw discrete samples are interpolated to create entries for every integer degree from -450° to +450°
2. **Averaging filter:** A 20-point moving window average is applied to smooth out noise from suspension oscillations
3. **Edge fading:** Values at the edges of the calibration domain are faded to avoid discontinuities

### Understeer Effect Output Application

The `UndersteerEffect` value (0-1) is used to generate haptic feedback in several ways:

#### 1. Wheel Vibration
```
vibrationTorque = waveform(frequency) × strength × pow(UndersteerEffect, curvePower)
```

Available waveforms:
- **Sine wave:** `sin(t × 2π × frequency)`
- **Square wave:** `sign(sin(t × 2π × frequency))`
- **Triangle wave:** `4 × |((t × frequency) mod 1) - 0.5| - 1`
- **Sawtooth in/out:** `±((t × frequency) mod 1)`

The vibration frequency scales with the effect intensity:
- When not understeering: `minimumFrequency` (e.g., 15 Hz)
- When fully understeering: `maximumFrequency` (e.g., 50 Hz)

#### 2. Constant Force Effect
The understeer can also modify the baseline FFB torque:
- **DecreaseForce mode:** `outputTorque = lerp(outputTorque, 0, constantForceStrength × pow(UndersteerEffect, curve))`
- **IncreaseForce mode:** `outputTorque += sign(VelocityY) × constantForceStrength × pow(UndersteerEffect, curve)`

This simulates the lightening or resistance of the steering that drivers feel when the front tyres begin to slide.

---

## Tyre Grip Estimation

### Does iRacing Provide Direct Grip Data?

**No, iRacing does not provide direct real-time tyre grip level telemetry.** There is no telemetry variable like "TyreFrontGrip" or "TyreRearGrip" available through the iRacing SDK.

### How This App Estimates Grip

Instead of using direct grip data, Marvin's AIRA uses an **indirect behavioral approach**:

1. **Calibration establishes "ideal grip" behavior:** When calibrating at low speed on a flat surface, the car's yaw rate response to steering input represents the behavior with full grip.

2. **Deviation from ideal = grip loss:** When driving normally, any deviation from the calibrated curve indicates grip loss:
   - **Less yaw rate than expected** → front tyres losing grip (understeer)
   - **More yaw rate than expected** → rear tyres losing grip (oversteer)

3. **The deviation magnitude correlates with grip loss severity:**
   - Small deviation = minor grip loss
   - Large deviation = significant grip loss / slide

### Advantages of This Approach
- Works across all cars without per-tyre modeling
- Automatically accounts for the specific car's steering geometry
- Provides intuitive "feel" that matches driver perception

### Limitations
- Requires per-car calibration
- Speed normalization assumes linear relationship (approximation)
- Surface variations (bumps, camber) can affect readings
- Calibration must be done on a flat surface at consistent speed

---

## Oversteer Effect

The oversteer effect uses the **same core formulas** as understeer but triggers when `deviation > 0` (rear grip loss instead of front):

### Mathematical Formulas

```
if deviation > 0 AND |deviation| >= oversteerMinThreshold:
    oversteerEffect = inverseLerp(oversteerMinThreshold, oversteerMaxThreshold, |deviation|)
else:
    oversteerEffect = 0

OversteerEffect = speedFade × oversteerEffect
```

### iRacing Physics Data Used
Same as understeer: `SteeringWheelAngle`, `YawRate`, `Speed`, `VelocityY`

### Effect Output Application
Identical structure to understeer (vibration + optional constant force), with separate threshold and strength settings.

---

## Seat-of-Pants Effect

The Seat-of-Pants effect simulates the lateral forces a driver feels when cornering. Three algorithms are available:

### Algorithm 1: Y Acceleration (Default)
```
seatOfPantsEffect = LatAccel ÷ 9.80665
```

This directly uses **lateral acceleration** (in g's) as the effect intensity.

**iRacing Variable:** `LatAccel` (m/s²)

### Algorithm 2: Y Velocity
```
seatOfPantsEffect = -VelocityY
```

Uses **lateral velocity** (side-slip velocity) directly.

**iRacing Variable:** `VelocityY` (m/s)

### Algorithm 3: Y Velocity Over X Velocity (Slip Angle)
```
seatOfPantsEffect = -VelocityY ÷ max(VelocityX, 20) × 20
```

This approximates the **slip angle** by comparing lateral velocity to forward velocity.

**iRacing Variables:** `VelocityX`, `VelocityY` (m/s)

### Effect Thresholding
```
absSeatOfPants = |seatOfPantsEffect|

if absSeatOfPants >= seatOfPantsMinThreshold:
    seatOfPantsEffect = sign(seatOfPantsEffect) × inverseLerp(minThreshold, maxThreshold, absSeatOfPants)
else:
    seatOfPantsEffect = 0

SeatOfPantsEffect = speedFade × seatOfPantsEffect
```

The final `SeatOfPantsEffect` is a **signed value** in the range [-1, 1] indicating left/right direction.

---

## Other FFB Effects

### ABS Vibration Effect

Triggers when iRacing's ABS system is active:

```
if BrakeABSactive == true:
    frequency = 50 Hz
    waveform = triangle wave
    vibrationTorque += 0.05 × (4 × |(phase - 0.5)| - 1)
```

**iRacing Variable:** `BrakeABSactive` (bool)

### Gear Change Vibration Effect

Provides brief vibration feedback when shifting:

```
if Gear changed AND Gear != 0:
    timerMS = 100ms
    frequency = 40 Hz
    waveform = square wave at ±0.05 amplitude
```

**iRacing Variable:** `Gear` (int)

### Crash Protection

Reduces FFB output when high g-forces are detected to protect the driver:

```
LongitudinalGForce = |LongAccel| ÷ 9.80665
LateralGForce = |LatAccel| ÷ 9.80665

if LongitudinalGForce >= crashThresholdLong OR LateralGForce >= crashThresholdLat:
    activate crash protection
    crashProtectionTimerMS = duration × 1000 + 1000 (recovery time)
    
crashProtectionScale = 1 - forceReduction × (timerMS ≤ 1000 ? timerMS/1000 : 1)
outputTorque *= crashProtectionScale
```

**iRacing Variables:** `LongAccel`, `LatAccel` (m/s²)

### Curb Protection

Reduces FFB spikes when hitting curbs, detected via shock velocity:

```
maxShockVelocity = max(
    |CFShockVel_ST|, |CRShockVel_ST|,
    |LFShockVel_ST|, |LRShockVel_ST|,
    |RFShockVel_ST|, |RRShockVel_ST|
)

if maxShockVelocity >= curbProtectionShockThreshold:
    activate curb protection
```

**iRacing Variables:** `LFshockVel_ST`, `RFshockVel_ST`, `LRshockVel_ST`, `RRshockVel_ST`, `CFshockVel_ST`, `CRshockVel_ST` (6-sample 360Hz arrays, m/s)

### Soft Lock

Provides resistance when approaching the car's steering limit:

```
deltaToMax = (SteeringWheelAngleMax × 0.5) - |SteeringWheelAngle|

if deltaToMax < 0:
    sign = sign(SteeringWheelAngle)
    outputTorque += sign × deltaToMax × 2 × softLockStrength
    
    if sign(wheelVelocity) != sign:
        outputTorque += wheelVelocity × softLockStrength
```

**iRacing Variable:** `SteeringWheelAngleMax` (radians)

### LFE (Low Frequency Effects)

Captures audio from a microphone/loopback device and injects it as FFB for bass shakers sent to the wheel:

```
outputTorque += LFEMagnitude × LFEStrength
```

The LFE magnitude is computed by:
1. Capturing 8kHz audio samples
2. Converting from PCM16 to float [-1,1]
3. Averaging samples in batches
4. Synchronizing with the FFB update loop

### Wheel Lock Detection (Pedal Haptics)

Detects wheel lockup using RPM/speed ratio:

```
rpmSpeedRatio = VelocityX ÷ RPM
expectedRatio = RPMSpeedRatios[Gear]  // learned during normal driving

difference = currentRatio - expectedRatio
differencePct = (difference ÷ expectedRatio) - (1 - sensitivity)

if differencePct > 0:
    // Wheel lock detected - tyres rotating slower than expected
    amplitude = lerp(0, baseAmplitude, differencePct / 0.03)
```

**iRacing Variables:** `RPM`, `VelocityX`, `Gear`, `Brake`, `Clutch`

### Wheel Spin Detection (Pedal Haptics)

Inverse of wheel lock - detects when tyres are spinning faster than the car is moving:

```
difference = expectedRatio - currentRatio
differencePct = (difference ÷ expectedRatio) - (1 - sensitivity)

if differencePct > 0:
    // Wheel spin detected - tyres rotating faster than expected
```

**iRacing Variables:** Same as wheel lock detection

---

## Core FFB Processing Algorithms

The app offers multiple FFB processing algorithms for the steering torque signal:

### 1. Native 60Hz
```
outputTorque = steeringWheelTorque60Hz ÷ maxForce
```

### 2. Native 360Hz
Uses the 360Hz sub-samples for smoother output:
```
outputTorque = steeringWheelTorque500Hz ÷ maxForce
```

### 3. Detail Booster
Amplifies high-frequency detail while maintaining baseline:
```
detailBoost = lerp(1 + detailBoostSetting, 1, curbProtectionFactor)
runningTorque = lerp(
    runningTorque + (current500Hz - previous500Hz) × detailBoost,
    current500Hz,
    detailBoostBias
)
outputTorque = runningTorque ÷ maxForce
```

### 4. Delta Limiter
Limits the rate of change of the FFB signal:
```
deltaLimit = deltaLimitSetting ÷ 500
limitedDelta = clamp(current500Hz - previous500Hz, -deltaLimit, deltaLimit)
runningTorque = lerp(runningTorque + limitedDelta, current500Hz, bias)
```

### 5. Slew and Total Compression
Combines slew rate limiting with dynamic range compression:
```
// Slew compression
if |normalizedDelta| > threshold:
    normalizedDelta = sign × (threshold + (|delta| - threshold) × (1 - rate))

// Total compression
outputTorque = compression(outputTorque, rate, threshold, width)
```

### Output Curve
A power curve can be applied to modify the output response:
```
power = 1 + curve × 4      (if curve ≥ 0)
power = 1 + curve × 0.75   (if curve < 0)

outputTorque = sign(outputTorque) × |outputTorque|^power
```

### Soft Limiter
A smooth limiter to prevent clipping:
```
if |value| ≤ threshold:
    return value
else:
    magnitude = 1 + 0.5 × (|value| - threshold - halfWidth) 
              + 0.5 × (width/π) × sin(π × (|value| - threshold + halfWidth) / width)
    return sign(value) × magnitude
```

---

## Helper Functions Reference

### Unit Conversions
```
KPH_TO_MPS = 5/18 ≈ 0.2778
MPS_TO_KPH = 18/5 = 3.6
RADIANS_TO_DEGREES = 180/π ≈ 57.2958
DEGREES_TO_RADIANS = π/180 ≈ 0.01745
ONE_G = 9.80665 m/s²
```

### Mathematical Functions

**Saturate (Clamp to 0-1):**
```
Saturate(value) = clamp(value, 0, 1)
```

**Linear Interpolation:**
```
Lerp(start, end, t) = start + (end - start) × Saturate(t)
```

**Inverse Linear Interpolation:**
```
InverseLerp(start, end, value):
    if |end - start| < ε:
        return (value < start) ? 0 : 1
    else:
        return Saturate((value - start) / (end - start))
```

**Smoothstep (Smooth S-curve):**
```
Smoothstep(start, end, t):
    t = Saturate((t - start) / (end - start))
    return t × t × (3 - 2t)
```

**Hermite Interpolation (4-point cubic):**
```
InterpolateHermite(v0, v1, v2, v3, t):
    a = 2 × v1
    b = v2 - v0
    c = 2×v0 - 5×v1 + 4×v2 - v3
    d = -v0 + 3×v1 - 3×v2 + v3
    return 0.5 × (a + b×t + c×t² + d×t³)
```

**Soft Knee Compression:**
```
Compression(value, rate, threshold, width):
    halfWidth = width × 0.5
    
    Region 1 (|value| ≤ threshold - halfWidth):
        return value  // Pass-through
    
    Region 2 (threshold - halfWidth < |value| < threshold + halfWidth):
        // Sine-eased soft knee
        t = |value| - threshold + halfWidth
        delta = 0.5 × rate × (t - (width/π) × sin(π × t / width))
        return sign(value) × (|value| - delta)
    
    Region 3 (|value| ≥ threshold + halfWidth):
        // Linear compression
        return sign(value) × (threshold + (|value| - threshold) × (1 - rate))
```

---

## Signal Processing Techniques

This section provides an in-depth explanation of two fundamental signal processing techniques used throughout the AIRA application to transform raw physics data into smooth, responsive haptic feedback.

### Smoothstep Speed Gating

#### Purpose
Smoothstep Speed Gating prevents false effect triggering at very low speeds where physics data can be noisy or unreliable. Instead of using a hard on/off cutoff, the smoothstep function provides a **gradual fade** that feels natural and avoids sudden haptic changes.

#### The Problem It Solves
At low speeds (especially below 5 kph):
- **Yaw rate normalization** (`yawRate / speed`) becomes unstable as speed approaches zero
- **Minor steering inputs** can produce large deviation values
- **Parking lot maneuvers** would otherwise trigger understeer/oversteer effects
- **Pit lane driving** would produce distracting false vibrations

A simple threshold like `if (speed > 10) enableEffect()` would cause a jarring on/off transition.

#### Implementation

The app uses **smoothstep** to create a smooth fade-in curve between two speed values:

```
speedFade = smoothstep(startSpeed, endSpeed, currentSpeed)
```

Where:
- `startSpeed = 1 kph` - Below this, effects are completely off
- `endSpeed = 20 kph` - Above this, effects are at full strength
- `currentSpeed` - The player's current speed in kph

#### Mathematical Formula

```
function smoothstep(start, end, value):
    // Normalize the value to 0-1 range
    t = clamp((value - start) / (end - start), 0, 1)
    
    // Apply the S-curve polynomial
    return t × t × (3 - 2t)
```

#### Breakdown of the Formula

1. **Normalization**: `t = (value - start) / (end - start)`
   - At `value = start` (1 kph): `t = 0`
   - At `value = end` (20 kph): `t = 1`
   - Values are clamped to [0, 1]

2. **S-Curve**: `t² × (3 - 2t)`
   - This is a **Hermite interpolation** polynomial
   - At `t = 0`: returns `0`
   - At `t = 1`: returns `1`
   - At `t = 0.5`: returns `0.5`
   - **Crucially**: The first derivative is **zero** at both endpoints

#### Why Smoothstep (Not Linear)?

| Approach | Formula | Disadvantage |
|----------|---------|--------------|
| **Hard threshold** | `if speed > 10 then 1 else 0` | Jarring on/off transition |
| **Linear fade** | `(speed - start) / (end - start)` | Perceivable kink at start/end points |
| **Smoothstep** | `t²(3 - 2t)` | ✅ Smooth at both endpoints with zero velocity |

The smoothstep polynomial ensures:
- **Zero velocity at start**: Effect fades in gradually, not abruptly
- **Zero velocity at end**: Effect reaches full strength smoothly
- **Symmetrical S-curve**: Equal transition feel on both sides

#### Visual Representation

```
Effect Intensity (0-1)
    ^
1.0 |                         ************
    |                    *****
    |                ****
0.5 |             ***
    |          ***
    |       ***
0.0 |*******
    +---------------------------------> Speed (kph)
         1     5     10    15    20
         ^                       ^
       start                    end
        (effects off)     (full effects)
```

#### Application in AIRA

The `speedFade` factor is multiplied with every effect output:

```
UndersteerEffect = speedFade × understeerEffect
OversteerEffect = speedFade × oversteerEffect
SeatOfPantsEffect = speedFade × seatOfPantsEffect
SkidSlip = speedFade × skidSlip
```

**Result**: All effects smoothly fade in as the car accelerates from 1 to 20 kph, and fade out when decelerating, preventing any abrupt haptic changes during slow driving.

---

### Threshold System (Min/Max with Linear Interpolation)

#### Purpose
The threshold system converts a **continuous physical measurement** (like deviation from expected yaw rate) into a **normalized effect intensity** (0 to 1) that can drive haptic outputs. This system provides:
- **Dead zone** elimination (ignore noise below minimum threshold)
- **Progressive response** (gradual increase between thresholds)
- **Saturation** (limit effect at maximum threshold)

#### The Problem It Solves
Raw physics data (e.g., yaw rate deviation) varies across a wide range:
- Very small values = normal driving, should produce no effect
- Medium values = mild understeer, should produce proportional feedback
- Large values = severe understeer, should produce maximum effect

Without thresholding:
- Tiny deviations would trigger subtle unwanted vibrations
- The effect would never reach "full intensity" since deviation has no theoretical maximum
- Users would have no control over sensitivity

#### Implementation: InverseLerp with Thresholds

AIRA uses an **inverse linear interpolation** (InverseLerp) between user-configurable minimum and maximum thresholds:

```
if |deviation| >= minThreshold:
    effectIntensity = inverseLerp(minThreshold, maxThreshold, |deviation|)
else:
    effectIntensity = 0  // Dead zone
```

#### Mathematical Formula

```
function inverseLerp(min, max, value):
    // Handle edge case where min == max
    if |max - min| < epsilon:
        return (value < min) ? 0 : 1
    
    // Calculate normalized position
    t = (value - min) / (max - min)
    
    // Clamp to valid range
    return clamp(t, 0, 1)
```

#### Breakdown of the Formula

Given:
- `minThreshold` = 0.05 (default understeer minimum)
- `maxThreshold` = 0.15 (default understeer maximum)
- `deviation` = current measured deviation

**Case Analysis:**

| Deviation Value | Calculation | Effect Intensity |
|-----------------|-------------|------------------|
| 0.02 | Below min threshold | 0.0 (dead zone) |
| 0.05 | `(0.05 - 0.05) / 0.10 = 0.00` | 0.0 (threshold start) |
| 0.08 | `(0.08 - 0.05) / 0.10 = 0.30` | 0.3 (linear region) |
| 0.10 | `(0.10 - 0.05) / 0.10 = 0.50` | 0.5 (midpoint) |
| 0.15 | `(0.15 - 0.05) / 0.10 = 1.00` | 1.0 (maximum) |
| 0.25 | `(0.25 - 0.05) / 0.10 = 2.00` → clamped | 1.0 (saturated) |

#### Visual Representation

```
Effect Intensity (0-1)
    ^
1.0 |                    ************  (saturation)
    |                  **
    |                **
0.5 |              **
    |            **
    |          **
0.0 |**********                        (dead zone)
    +---------------------------------> |Deviation|
         0.00  0.05  0.10  0.15  0.20
               ^           ^
            minThresh   maxThresh
```

#### Why Two Thresholds (Not One)?

| Approach | Behavior | Problem |
|----------|----------|---------|
| **Single threshold** | `if deviation > threshold then effect = 1` | Binary on/off, no progressive feedback |
| **Single threshold + linear** | `effect = deviation × sensitivity` | No dead zone, no saturation |
| **Min/Max thresholds** | Progressive between min and max | ✅ Dead zone + linear response + saturation |

#### User-Controllable Parameters

In AIRA, users can adjust these thresholds for each effect:

**Understeer Effect:**
- `SteeringEffectsUndersteerMinimumThreshold` (default: 0.05)
- `SteeringEffectsUndersteerMaximumThreshold` (default: 0.15)

**Oversteer Effect:**
- `SteeringEffectsOversteerMinimumThreshold` (default: 0.05)
- `SteeringEffectsOversteerMaximumThreshold` (default: 0.15)

**Seat-of-Pants Effect:**
- `SteeringEffectsSeatOfPantsMinimumThreshold` (default varies per algorithm)
- `SteeringEffectsSeatOfPantsMaximumThreshold` (default varies per algorithm)

#### Threshold Tuning Guidelines

| Adjustment | Effect |
|------------|--------|
| **Increase minThreshold** | Larger dead zone, ignores more subtle slip |
| **Decrease minThreshold** | More sensitive, may trigger on minor variations |
| **Increase maxThreshold** | Slower ramp-up, full effect only at severe slip |
| **Decrease maxThreshold** | Faster ramp-up, reaches full intensity sooner |
| **Narrow range (min ≈ max)** | Sharp on/off behavior at threshold |
| **Wide range (max >> min)** | Very gradual, progressive response |

#### Code Implementation in AIRA

```csharp
// From SteeringEffects.cs UpdateEffects()

// Calculate deviation from expected yaw rate
var deviation = actualYawRate - expectedYawRate;
var absDeviation = MathF.Abs(deviation);

// Apply threshold system for understeer (deviation < 0)
var understeerEffect = 0f;
if ((deviation < 0f) && (absDeviation >= settings.SteeringEffectsUndersteerMinimumThreshold))
{
    understeerEffect = MathZ.InverseLerp(
        settings.SteeringEffectsUndersteerMinimumThreshold,
        settings.SteeringEffectsUndersteerMaximumThreshold,
        absDeviation
    );
}

// Apply speed gating
UndersteerEffect = speedFade * understeerEffect;
```

#### Combined Effect: Threshold + Speed Gating

The final effect intensity is the **product** of both processing stages:

```
finalEffect = speedFade × thresholdedEffect
```

This means:
- At **low speed** (< 1 kph): Effect is always 0 regardless of deviation
- At **high speed** with **small deviation**: Effect is 0 (dead zone)
- At **high speed** with **medium deviation**: Effect scales linearly
- At **high speed** with **large deviation**: Effect saturates at 1.0

**Example Calculation:**

Given:
- Speed = 15 kph
- Deviation = 0.10
- minThreshold = 0.05
- maxThreshold = 0.15

```
speedFade = smoothstep(1, 20, 15) = 0.843  (approximately)
thresholdedEffect = inverseLerp(0.05, 0.15, 0.10) = 0.50
finalEffect = 0.843 × 0.50 = 0.42
```

The understeer effect would be at **42% intensity** - partially gated by speed (not yet at 20 kph) and in the middle of the threshold range.

---

## Summary

This document provides the technical details necessary to implement similar FFB effects for another racing simulation. The key insights are:

1. **Understeer/Oversteer detection** relies on comparing actual vs. expected yaw rates, not direct grip telemetry
2. **Calibration is essential** to establish the baseline handling curve for each car
3. **Yaw rate is normalized by speed** to enable valid comparisons across velocities
4. **The deviation magnitude is thresholded** and scaled to produce a 0-1 effect intensity
5. **Multiple output modalities** (vibration, constant force) can be applied using the effect intensity
6. **Speed fading** prevents false triggers at very low speeds

For implementation in another game, the core requirements are:
- Steering wheel angle input
- Yaw rate (angular velocity around vertical axis)
- Vehicle speed
- Ideally: lateral acceleration and lateral velocity for Seat-of-Pants effects
- For pedal effects: brake/throttle input, wheel lockup detection (via slip ratio or ABS status)
