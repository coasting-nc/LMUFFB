# Research Report: Rear Grip Estimation and Lateral G Boost Compatibility

## Context

**Date:** 2026-02-02  
**Related Documents:**
- `docs/dev_docs/investigations/slope_detection_issues_v0.7.0.md`
- `docs/dev_docs/implementation_plans/plan_slope_detection_fixes_v0.7.1.md`

**Update:**
The research has been completed and the findings are documented here:
- `docs/dev_docs/research/deep research report - rear grip estimation.md`

**Question Under Investigation:**  
Can we improve rear grip estimation to make it compatible with the front Slope Detection algorithm, thereby preserving the Lateral G Boost effect?

---

# Part 1: Assessment and Recommendations

## 1.1 Understanding the Current Asymmetry

### Why Front and Rear Currently Use Different Methods

The current implementation uses Slope Detection only for **front tires** because the algorithm relies on the `dG/dAlpha` relationship (lateral G-force vs slip angle derivative). Here's the fundamental problem:

| Data Source | Availability | Notes |
|-------------|--------------|-------|
| `mLocalAccel.x` | Vehicle CG | Single value for entire car - not per-axle |
| `mWheel[i].mLateralForce` | Per wheel | Force in Newtons (not normalized grip) |
| `mWheel[i].mGripFract` | Per wheel | Often encrypted/missing for DLC cars |
| `mWheel[i].mLateralPatchVel` | Per wheel | Raw slip velocity at contact patch |

The lateral G-force (`mLocalAccel.x`) is measured at the **vehicle center of gravity** - it cannot be decomposed into front vs rear contributions without additional physics modeling.

### Why This Matters for Lateral G Boost

The Lateral G Boost effect is designed to:
1. Detect when front grip > rear grip (oversteer condition)
2. Amplify the Lateral G (SoP) force to make the wheel feel "heavier" during a slide

With Slope Detection enabled:
- **Front grip:** Dynamically calculated (can be 0.3 one frame, 0.9 the next)
- **Rear grip:** Static threshold calculation (stable, slower to change)

This creates **artificial grip differentials** that don't represent real oversteer conditions.

---

## 1.2 Is Lateral G Boost Important?

### The Purpose of Lateral G Boost

Looking at the user guide and code, Lateral G Boost serves a specific purpose in the FFB model:

| Effect | Purpose | Timing |
|--------|---------|--------|
| **Yaw Kick** | Sharp impulse at onset of oversteer | Instant (first signal) |
| **Rear Align Torque** | Counter-steering pull direction | Fast (builds with slip angle) |
| **Lateral G Boost** | Weight of the slide | Sustained (proportional to delta) |

The Lateral G Boost is the **sustained component** that tells the driver how big the slide is. Without it:
- The driver still feels the direction (via Rear Align Torque)
- The driver still feels the initial snap (via Yaw Kick)
- But the "momentum" feel is reduced - the wheel may feel too similar during small and large slides

### My Assessment: Important But Not Critical

**Verdict:** Lateral G Boost is **important for immersion** but **not critical for safety or basic car control**. The driver can still catch slides without it.

For v0.7.1, **disabling it when Slope Detection is ON is acceptable** as a safety measure to prevent oscillations. However, for a complete solution in a future version, we should aim to preserve this effect.

---

## 1.3 Can We Estimate Rear Grip Better?

### Available Approaches

I've analyzed the available telemetry data and identified four potential approaches for estimating rear grip dynamically:

#### Approach A: Rear Slip Angle Saturation Model

**Concept:** Use the shape of the tire force curve. Beyond peak slip angle, lateral force saturates or decreases while slip angle continues to increase.

**Available Data:**
- `mWheel[2,3].mLateralPatchVel` - Lateral slip velocity at rear wheels
- `mWheel[2,3].mLongitudinalGroundVel` - Ground velocity for normalization
- We already calculate `rear_slip_angle` (via `calculate_grip()` with `is_front=false`)

**Implementation:**
```cpp
// Calculate rear slip angle rate of change
double d_rear_slip = (current_rear_slip - prev_rear_slip) / dt;

// Calculate rear lateral force rate of change  
double rear_lat_force = (w[2].mLateralForce + w[3].mLateralForce) / 2.0;
double d_rear_lat = (rear_lat_force - prev_rear_lat_force) / dt;

// Slope: d(Force) / d(Slip)
// If slope is negative, rear tires are past peak grip
double rear_slope = d_rear_lat / (d_rear_slip + epsilon);
```

**Pros:**
- Uses actual rear tire data (not CG data)
- Conceptually similar to front Slope Detection
- Available in the telemetry

**Cons:**
- `mLateralForce` may also be encrypted for DLC cars
- Requires tuning different thresholds for force vs G-based slope
- More complex derivative calculation (force in Newtons vs G in m/s²)

**Feasibility: MEDIUM-HIGH** - This is the most promising approach.

---

#### Approach B: Yaw Rate vs Steering Correlation (Understeer Gradient)

**Concept:** In normal driving, yaw rate is proportional to steering angle at a given speed. When rear tires lose grip, yaw rate exceeds what the steering angle commands (oversteer). The opposite occurs for understeer.

**Available Data:**
- `mLocalRot.y` - Yaw rotation rate (radians/sec)
- `mUnfilteredSteering` - Steering input (-1 to +1)
- `mLocalVel.z` - Forward velocity

**Implementation:**
```cpp
// Expected yaw rate for this speed and steering (linearized bicycle model)
double steer_angle = steering_input * wheel_range / 2.0;
double expected_yaw_rate = (car_speed * steer_angle) / wheelbase;

// Actual yaw rate
double actual_yaw_rate = data->mLocalRot.y;

// Oversteer indicator: actual > expected means rear is slipping
double yaw_excess = actual_yaw_rate - expected_yaw_rate;
double rear_grip_indicator = 1.0 - clamp(yaw_excess * sensitivity, 0.0, 0.8);
```

**Pros:**
- Uses vehicle-level dynamics (cannot be encrypted)
- Physically meaningful understeer/oversteer indicator
- Works even with encrypted wheel data

**Cons:**
- Requires knowing wheelbase (or estimating it)
- Speed-sensitive calculation (singularity at low speeds)
- Only indicates relative front/rear balance, not absolute grip

**Feasibility: MEDIUM** - Good for oversteer/understeer delta, not for absolute grip.

---

#### Approach C: Rear Lateral Force Rate Limiting

**Concept:** Rear lateral force plateaus or decreases when at the limit. Track the maximum force generated and compare current to max.

**Available Data:**
- `mWheel[2,3].mLateralForce` - Per-wheel lateral force
- `mWheel[2,3].mTireLoad` - Tire vertical load (for normalization)

**Implementation:**
```cpp
// Normalize force by load to get friction coefficient
double rear_mu = rear_lat_force / rear_tire_load;

// Track maximum observed mu for this stint/temperature
static double max_rear_mu = 0.0;
if (rear_mu > max_rear_mu) max_rear_mu = rear_mu;

// Grip is ratio of current to maximum
double rear_grip = clamp(rear_mu / max_rear_mu, 0.2, 1.0);
```

**Pros:**
- Self-calibrating (learns tire capability)
- Works with any tire compound
- Conceptually simple

**Cons:**
- Requires stable "baseline" conditions
- Cold tires or early in session give incorrect readings
- Max mu changes with tire temperature/wear

**Feasibility: LOW-MEDIUM** - Too many edge cases for reliable use.

---

#### Approach D: Symmetric Slope Detection (Apply to Rear)

**Concept:** Instead of using vehicle-level `mLocalAccel.x`, use the rear tire lateral force as a proxy for "rear lateral G."

**Available Data:**
- `mWheel[2,3].mLateralForce` - Use as "rear equivalent of lateral G"
- Rear slip angle (already calculated)

**Implementation:**
```cpp
// Normalize rear lateral force to equivalent G range
double rear_lat_g_equiv = (rear_lat_force / reference_load) * scaling_factor;

// Apply same Slope Detection algorithm
double rear_grip = calculate_slope_grip(rear_lat_g_equiv, rear_slip_angle, dt);
```

**Pros:**
- Same algorithm, different input source
- Keeps front and rear calculations symmetric
- Eliminates the "different methods cause differential" problem

**Cons:**
- Requires careful calibration of `reference_load` and `scaling_factor`
- `mLateralForce` may be encrypted
- Different physical units require conversion

**Feasibility: MEDIUM-HIGH** - Best option if `mLateralForce` is available.

---

## 1.4 My Recommendation

### Short Term (v0.7.1): Disable Lateral G Boost
As proposed in the implementation plan, disabling the Lateral G Boost when Slope Detection is ON is the correct short-term fix. It's safe and eliminates oscillations.

### Medium Term (v0.8.0): Implement Approach A or D
I recommend implementing **Approach A (Rear Slip Angle Saturation Model)** or **Approach D (Symmetric Slope Detection)** because:

1. They use actual per-wheel data, not vehicle CG data
2. They can be implemented with similar code patterns to front Slope Detection
3. They create symmetric calculation methods for front and rear

The choice between A and D depends on:
- If `mLateralForce` is reliably available → Use Approach D
- If `mLateralForce` is often encrypted → Use Approach A with `mLateralPatchVel`

### Long Term (v0.9.0+): Hybrid with Yaw-Based Fallback
Combine Approach A/D with Approach B as a fallback:
- Primary: Rear force/slip slope detection
- Fallback (if data encrypted): Yaw rate correlation for relative front/rear balance

---

## 1.5 Impact on User Experience

### If We Don't Fix This (Lateral G Boost stays disabled)

| Scenario | Current (with Slope Detection ON) | After Fix |
|----------|-----------------------------------|-----------|
| Small slide | Wheel feels appropriate | Same |
| Large slide | Wheel feels too similar to small slide | Wheel feels heavier |
| Catch slide | Possible (via Rear Align) | Same |
| Immersion | Reduced "momentum" feel | Full "weight" of slide |

Users who rely on Lateral G Boost for oversteer feel will notice the difference. The fix is important for immersion, but the Yaw Kick and Rear Align Torque still provide essential oversteer cues.

### Workaround for Affected Users

Until a proper rear grip estimation is implemented, users who want both Slope Detection AND Lateral G Boost-like behavior can:
1. Increase **SoP Effect** (Lateral G) to compensate
2. Increase **Rear Align Effect** for stronger counter-steer pull
3. Or disable Slope Detection and use the classic static threshold method

---

# Part 2: Deep Research Query

## Research Topic: Dynamic Rear Tire Grip Estimation for Racing Simulation FFB

### Background Context

We are developing a Force Feedback (FFB) engine for racing simulators that estimates tire grip in real-time to modulate steering force and provide oversteer/understeer effects. Our current implementation uses a "Slope Detection" algorithm to estimate front tire grip by monitoring the derivative of lateral G-force vs slip angle (`dG/dAlpha`). This works well for front tires because the Self-Aligning Torque (and by extension, the FFB) is primarily driven by front tire physics.

However, we need to estimate **rear tire grip** to implement effects like:
- **Lateral G Boost:** Amplifying SoP force when rear grip is lower than front (oversteer condition)
- **Slide Texture:** Scaling vibration intensity based on grip differential

The challenge is that our primary grip signal (lateral G-force at CG) cannot be decomposed into front vs rear contributions without physics modeling. We have access to per-wheel data including lateral force, slip angle, patch velocity, and tire load.

### Specific Questions for Expert Research

#### Q1: Tire Force Curve Analysis
How can we detect when rear tires are past their peak grip using only the following data streams?
- Rear wheel lateral force (Newtons)
- Rear wheel slip angle (radians, calculated from patch velocity / ground velocity)
- Rear wheel tire load (Newtons)
- Rear wheel slip velocity (m/s at contact patch)

Specifically:
- What derivative relationship should we monitor (d(Force)/d(Slip), d(mu)/d(Slip))?
- What threshold values indicate transition from linear to saturation region?
- How does tire temperature and wear affect these thresholds?

#### Q2: Understeer Gradient and Yaw Correlation
Can we reliably estimate relative front/rear grip balance using the understeer gradient?
- How do we calculate expected yaw rate from steering input and speed?
- What is the typical relationship: `yaw_rate = f(steering_angle, speed, wheelbase, f/r_grip_ratio)`?
- How do we handle low-speed singularities?
- How do we account for lateral load transfer affecting the calculation?

#### Q3: Normalized Friction Coefficient Tracking
Is it viable to estimate rear grip by tracking the friction coefficient (mu = F_lateral / F_normal)?
- How stable is max_mu across a typical racing stint?
- How quickly does max_mu change with tire temperature (warmup phase)?
- Is there a way to estimate instantaneous tire capability versus historical maximum?

#### Q4: Synchronizing Front and Rear Grip Calculations
Our front grip uses a derivative-based algorithm (monitors slope of lateral-G vs slip) while rear currently uses a static threshold. This asymmetry causes oscillations when we compute `grip_delta = front_grip - rear_grip`. 

Expert input needed:
- Should we apply the same derivative algorithm to rear tires (using rear lateral force as proxy for rear G)?
- What are the calibration considerations for converting force (Newtons) to equivalent G for symmetric comparison?
- Are there racing simulator FFB implementations that successfully handle asymmetric front/rear grip estimation without oscillation?

#### Q5: Pacejka Magic Formula Application
Can we use a simplified Pacejka Magic Formula to model the rear tire slip-force curve?
- What parameters can we realistically estimate from runtime telemetry?
- Can we fit a local approximation around the current operating point?
- How do we handle the fact that each car/tire combination has different Pacejka coefficients?

#### Q6: Signal Filtering for Derivatives
When taking derivatives of slip angle and lateral force:
- What filter type is best for noise rejection while preserving slope detection sensitivity (Savitzky-Golay, Butterworth, Kalman)?
- What are typical latency vs noise tradeoff considerations for 360Hz telemetry rate?
- Should we use different filter settings for front (steering-coupled) vs rear (free-floating)?

### Desired Output Format

For each question, please provide:
1. **Theoretical foundation** - The physics/math behind the approach
2. **Practical implementation** - Pseudocode or algorithm description
3. **Calibration parameters** - What values to tune and typical ranges
4. **Limitations/caveats** - When the approach fails or requires fallback
5. **Academic/industry references** - Papers, racing telemetry guides, or sim-racing FFB implementations that use similar techniques

### Domain-Specific Context

The target application is racing simulation FFB with the following constraints:
- Telemetry rate: ~360-400 Hz
- Processing latency budget: < 5ms for grip estimation
- Cars: GT3, LMDh, LMP2, GTE (high downforce, high grip)
- Available data: See `InternalsPlugin.hpp` struct documentation
- No access to raw tire model coefficients (only runtime telemetry)
- Some wheel data fields may be encrypted (especially `mGripFract`)

### Keywords for Search

- Tire slip angle saturation detection
- Rear tire grip estimation racing simulation
- Understeer gradient calculation FFB
- Pacejka tire model real-time estimation
- Yaw rate steering correlation oversteer detection
- Racing simulator force feedback oversteer effects
- Lateral force derivative grip estimation

---

*Research query created: 2026-02-02*  
*For use in v0.8.0+ planning*
