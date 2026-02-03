# AIRA vs lmuFFB: Understeer Effect & Grip Estimation Comparison Report

**Date:** 2026-02-03  
**Purpose:** Investigate whether similar formulas from Marvin's AIRA can be implemented in lmuFFB for understeer effect and front tire grip estimation.

---

## Executive Summary

After analyzing Marvin's AIRA (Advanced iRacing Assistant) FFB implementation and comparing it with lmuFFB's current Slope Detection algorithm and grip estimation systems, this report concludes:

1. **AIRA's calibration-based yaw rate comparison approach is fundamentally different from lmuFFB's derivative-based slope detection**, and while both aim to detect grip loss, they operate on different principles.

2. **lmuFFB's current Slope Detection algorithm (v0.7.0) is already more sophisticated than AIRA's approach** for the specific use case of front tire grip estimation.

3. **Some AIRA concepts could enhance lmuFFB**, particularly the speed normalization factor and the explicit threshold-based effect output mapping.

4. **Direct implementation of AIRA formulas is NOT recommended** due to fundamental differences in game telemetry availability and the calibration requirements of AIRA's approach.

---

## 1. Telemetry Data Comparison

### 1.1 Available Data in iRacing (AIRA)

| Variable | Unit | Purpose |
|----------|------|---------|
| `SteeringWheelAngle` | radians | Current steering input |
| `SteeringWheelAngleMax` | radians | Maximum steering rotation |
| `YawRate` | rad/s | Rate of yaw rotation |
| `Speed` | m/s | Vehicle speed |
| `VelocityY` | m/s | Lateral velocity (sideslip) |
| `LatAccel` | m/s² | Lateral acceleration |

> **Critical Note:** iRacing does NOT provide direct tire grip level telemetry.

### 1.2 Available Data in LMU (lmuFFB)

| Variable | Unit | Purpose | Status |
|----------|------|---------|--------|
| `mGripFract` | 0.0-1.0 | Direct tire grip fraction | **Always 0.0 (encrypted)** |
| `mLateralPatchVel` | m/s | Tire contact patch lateral velocity | ✓ Available |
| `mLongitudinalPatchVel` | m/s | Tire contact patch longitudinal velocity | ✓ Available |
| `mLocalAccel.x` | m/s² | Lateral acceleration (body frame) | ✓ Available |
| `mLocalVel.z` | m/s | Longitudinal velocity | ✓ Available |
| `mSteeringShaftTorque` | Nm | Raw steering torque (SAT) | ✓ Available |
| `YawRate` (mLocalRotAccel.y) | rad/s² | Yaw acceleration | ✓ Available |

> **Key Difference:** LMU provides slip velocities directly at the tire contact patch level, whereas iRacing requires inferring grip loss from vehicle-level yaw rate.

---

## 2. AIRA Understeer Detection Approach

### 2.1 Core Algorithm

AIRA uses a **calibration-based expected vs. actual yaw rate comparison**:

```
1. CALIBRATION PHASE (15 kph, constant speed):
   - Record: expected_yaw_rate[steering_angle] = normalized_yaw_rate
   - normalized_yaw_rate = |YawRate| × (180/π) ÷ Speed_kph

2. REAL-TIME DETECTION:
   deviation = actual_normalized_yaw_rate - expected_normalized_yaw_rate
   
   if deviation < 0:  # Car turning LESS than expected
       → UNDERSTEER detected
   if deviation > 0:  # Car turning MORE than expected
       → OVERSTEER detected

3. EFFECT INTENSITY:
   if |deviation| >= understeerMinThreshold:
       understeerEffect = inverseLerp(minThreshold, maxThreshold, |deviation|)
   else:
       understeerEffect = 0
   
   FinalEffect = speedFade × understeerEffect  (range: 0.0 - 1.0)
```

### 2.2 AIRA Formulas

**Speed Normalization:**
```
normalizedYawRate = |YawRate| × 57.2958 ÷ Speed_kph
```

**Speed Fade (Low Speed Suppression):**
```
speedFade = smoothstep(1, 20, Speed_kph)
where smoothstep(start, end, t):
    t = clamp((t - start) / (end - start), 0, 1)
    return t × t × (3 - 2t)
```

**Effect Intensity Mapping:**
```
inverseLerp(min, max, value) = clamp((value - min) / (max - min), 0, 1)
```

### 2.3 AIRA Strengths

| Aspect | Description |
|--------|-------------|
| **Self-Calibrating** | Automatically learns car-specific handling characteristics |
| **No Tire Physics Required** | Works with any car without knowing tire models |
| **Intuitive Mapping** | Deviation directly maps to driver perception |
| **Low Latency** | No filtering required on yaw rate signal |

### 2.4 AIRA Limitations

| Limitation | Impact |
|------------|--------|
| **Requires Manual Calibration** | User must drive at 15 kph and sweep steering |
| **Per-Car Calibration Files** | Creates maintenance burden for users |
| **Speed Dependency** | Calibration at 15 kph may not predict behavior at 200 kph |
| **Surface Assumption** | Calibration assumes flat, consistent grip surface |
| **No Real-Time Adaptation** | Cannot adapt to tire temperature/wear during session |

---

## 3. lmuFFB Current Approaches

lmuFFB has **two grip estimation methods** that work together:

### 3.1 Static Threshold Method (Legacy)

**Used when:** `m_slope_detection_enabled = false` (default)

```cpp
// Combined Friction Circle Approximation
lat_metric = |slip_angle| / m_optimal_slip_angle    // e.g., 0.10 rad
long_metric = avg_slip_ratio / m_optimal_slip_ratio  // e.g., 0.12

combined_slip = sqrt(lat_metric² + long_metric²)

if combined_slip > 1.0:
    grip = 1.0 / (1.0 + (combined_slip - 1.0) × 2.0)  // Sigmoid decay
else:
    grip = 1.0
```

**Understeer Effect:**
```cpp
grip_loss = (1.0 - grip) × m_understeer_effect
grip_factor = max(0.0, 1.0 - grip_loss)
output_force = base_force × grip_factor
```

### 3.2 Slope Detection Method (v0.7.0)

**Used when:** `m_slope_detection_enabled = true`

```cpp
// Savitzky-Golay Derivative Calculation
dG_dt = calculate_sg_derivative(lateral_g_buffer, window, dt)
dAlpha_dt = calculate_sg_derivative(slip_angle_buffer, window, dt)

// Chain Rule: dG/dAlpha = (dG/dt) / (dAlpha/dt)
if |dAlpha_dt| > 0.001:
    slope_current = dG_dt / dAlpha_dt

// Grip Factor from Slope
if slope_current < m_slope_negative_threshold:  // e.g., -0.1
    excess = m_slope_negative_threshold - slope_current
    grip_factor = 1.0 - (excess × 0.1 × m_slope_sensitivity)

// Apply smoothing with EMA
grip_factor = EMA(grip_factor, tau=0.02s)
```

### 3.3 lmuFFB Strengths

| Aspect | Description |
|--------|-------------|
| **No Calibration Required** | Works immediately without user setup |
| **Real-Time Adaptation** | Responds to changing tire conditions |
| **Physically Grounded** | Based on actual tire force/slip relationship |
| **Direct Telemetry Access** | Uses slip velocities from contact patch |
| **Configurable Latency** | User can tune filter window for responsiveness |

### 3.4 lmuFFB Limitations

| Limitation | Impact |
|------------|--------|
| **Static Thresholds** (Legacy) | Requires manual tuning of optimal slip values |
| **Derivative Noise Sensitivity** | Must use filtering which introduces latency |
| **Front-Only Slope Detection** | Rear grip still uses static threshold |
| **No Per-Car Profiles** | Same parameters for all cars |

---

## 4. Feature-by-Feature Comparison

### 4.1 Grip Estimation Mechanism

| Feature | AIRA | lmuFFB (Slope Detection) |
|---------|------|--------------------------|
| **Primary Signal** | Yaw Rate (vehicle level) | Lateral G + Slip Angle (tire level) |
| **Reference** | Pre-calibrated curve | Real-time derivative analysis |
| **Adaptation** | None (per-car calibration) | Continuous (reacts to tire state) |
| **Latency** | ~0 ms (direct comparison) | 10-30 ms (SG filter window) |
| **Setup Effort** | High (calibration per car) | Low (configure once) |

### 4.2 Understeer Effect Output

| Feature | AIRA | lmuFFB |
|---------|------|--------|
| **Output Range** | 0.0 - 1.0 (normalized) | 0.0 - 1.0 (grip factor) |
| **Threshold System** | Min/Max threshold with linear interpolation | Negative slope threshold |
| **Speed Gating** | Smoothstep 1-20 kph | Linear m_speed_gate_lower to upper |
| **Haptic Modes** | Vibration + Constant Force Decay | Constant Force Decay only |

### 4.3 Oversteer Detection

| Feature | AIRA | lmuFFB |
|---------|------|--------|
| **Mechanism** | Positive deviation from expected yaw | Front-rear grip differential |
| **Output** | Separate oversteer effect | Lateral G Boost multiplier |
| **Real-Time** | Yes | Yes |
| **Accuracy** | Good (direct yaw measurement) | Good (grip comparison) |

---

## 5. Can AIRA Formulas Be Implemented in lmuFFB?

### 5.1 Yaw Rate Comparison Approach

**Feasibility:** ⚠️ **Partially Possible, Not Recommended**

**Why Not:**
1. **Calibration Burden:** AIRA requires users to perform a calibration run for every car. This significantly increases setup friction for users.

2. **LMU Advantage Lost:** LMU provides direct tire slip telemetry that AIRA doesn't have access to. Using yaw rate comparison would ignore this superior data source.

3. **No Adaptation:** The calibrated curve doesn't adapt to tire wear, temperature, or wet conditions — problems that lmuFFB's Slope Detection inherently handles.

**What Could Be Adopted:**
- The **speed normalization formula** could be used to enhance low-speed behavior detection.
- The **smoothstep speed fade** could replace the current linear speed gate for smoother transitions.

### 5.2 Speed Normalization for Yaw Rate

**Potential Enhancement:**

AIRA normalizes yaw rate by speed:
```
normalizedYawRate = |YawRate| × (180/π) ÷ Speed_kph
```

lmuFFB could apply this to its yaw kick calculation:
```cpp
// Current (FFBEngine.h ~line 1335):
double raw_yaw_accel = data->mLocalRotAccel.y;

// Proposed Enhancement:
double raw_yaw_accel = data->mLocalRotAccel.y;
double speed_kph = ctx.car_speed * 3.6;
if (speed_kph > 5.0) {
    raw_yaw_accel = raw_yaw_accel / (speed_kph / 50.0);  // Normalize
}
```

**Benefit:** The yaw kick effect would feel more consistent across speed ranges.

### 5.3 Smoothstep Speed Gating

**Current lmuFFB:**
```cpp
speed_gate = (car_speed - m_speed_gate_lower) / (m_speed_gate_upper - m_speed_gate_lower);
speed_gate = clamp(speed_gate, 0, 1);  // Linear transition
```

**AIRA-Inspired Smoothstep:**
```cpp
double t = (car_speed - m_speed_gate_lower) / (m_speed_gate_upper - m_speed_gate_lower);
t = clamp(t, 0, 1);
speed_gate = t * t * (3.0 - 2.0 * t);  // Smoothstep (S-curve)
```

**Benefit:** Smoother FFB engagement when transitioning from pit lane to racing speed.

### 5.4 InverseLerp Threshold Mapping

**AIRA Effect Curve:**
```
effect = inverseLerp(minThreshold, maxThreshold, |deviation|)
```

This could be applied to lmuFFB's slope-to-grip conversion:
```cpp
// Current:
if (m_slope_current < m_slope_negative_threshold) {
    double excess = m_slope_negative_threshold - m_slope_current;
    current_grip_factor = 1.0 - (excess * 0.1 * m_slope_sensitivity);
}

// AIRA-Style:
if (m_slope_current < m_slope_min_threshold) {
    double t = inverseLerp(m_slope_min_threshold, m_slope_max_threshold, m_slope_current);
    current_grip_factor = 1.0 - t;  // Linear mapping 0→1 becomes 1→0 grip
}
```

**Benefit:** More predictable and tunable grip loss curve.

---

## 6. Recommendations

### 6.1 Short-Term: Enhanced Speed Gating (Low Effort)

**Change:** Replace linear speed gate with smoothstep function.

**File:** `FFBEngine.h`, around line 1039

```cpp
// Replace:
ctx.speed_gate = (ctx.car_speed - (double)m_speed_gate_lower) / speed_gate_range;
ctx.speed_gate = (std::max)(0.0, (std::min)(1.0, ctx.speed_gate));

// With:
double t = (ctx.car_speed - (double)m_speed_gate_lower) / speed_gate_range;
t = (std::max)(0.0, (std::min)(1.0, t));
ctx.speed_gate = t * t * (3.0 - 2.0 * t);  // Smoothstep
```

**Impact:** Smoother FFB transitions at low speed.

### 6.2 Medium-Term: Improved Threshold Mapping (Moderate Effort)

Add configurable min/max slope thresholds with inverseLerp mapping, similar to AIRA's approach. This would make the slope detection response more predictable and easier to tune.

### 6.3 Long-Term: Yaw Rate Cross-Validation (High Effort)

Consider adding yaw rate comparison as a **secondary validation** mechanism:
- Use slope detection as primary
- Use yaw rate comparison as a "sanity check" or confidence multiplier
- If both agree (tire sliding AND yaw rate deviation), boost the understeer signal

### 6.4 NOT Recommended: Full AIRA Calibration System

The calibration-based approach adds significant user burden without providing benefits over lmuFFB's current derivative-based system. LMU's direct tire telemetry makes calibration unnecessary.

---

## 7. Summary Table

| Aspect | AIRA Approach | lmuFFB Current | Recommendation |
|--------|---------------|----------------|----------------|
| **Grip Estimation** | Yaw rate vs. calibrated curve | Slope of G-force/slip derivative | Keep lmuFFB approach |
| **Calibration** | Required per-car | Not required | No change |
| **Speed Normalization** | Normalized yaw rate | Linear speed gate | Adopt smoothstep |
| **Threshold Mapping** | inverseLerp min/max | Single threshold | Consider adopting |
| **Haptic Output** | Vibration + force decay | Force decay only | Could add vibration mode |
| **Oversteer** | Positive deviation | Grip differential | Both valid |
| **Real-Time Adaptation** | No | Yes (slope detection) | Keep lmuFFB advantage |

---

## 8. Conclusion

**The investigation reveals that lmuFFB's Slope Detection algorithm (v0.7.0) is conceptually more advanced than AIRA's calibration-based approach** because:

1. It adapts in real-time to tire conditions (temperature, wear, wet)
2. It requires zero user calibration
3. It uses direct tire-level telemetry (slip velocities) rather than inferring grip from vehicle-level yaw rate

**However, AIRA offers valuable lessons:**
- The smoothstep speed fade is more elegant than linear interpolation
- The explicit min/max threshold with inverseLerp provides better tunability
- Speed normalization for yaw-based effects could improve consistency

**The recommended path forward is to selectively adopt AIRA's signal processing refinements (smoothstep, inverseLerp) while retaining lmuFFB's superior derivative-based grip estimation core.**

---

## Document History

| Version | Date | Author | Notes |
|---------|------|--------|-------|
| 1.0 | 2026-02-03 | Antigravity | Initial comparison report |

---

## References

1. Marvin's AIRA Technical Report: `docs/dev_docs/tech_from_other_apps/Marvin's AIRA Refactored FFB_Effects_Technical_Report.md`
2. Slope Detection Implementation Plan: `docs/dev_docs/implementation_plans/plan_slope_detection.md`
3. FFB Slope Detection Research: `docs/dev_docs/FFB Slope Detection for Grip Estimation.md`
4. Understeer Investigation Report: `docs/dev_docs/understeer_investigation_report.md`
