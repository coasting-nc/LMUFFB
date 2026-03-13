# Fix Lockup Vibration and Suspension Bottoming with correct interpretation of mSuspForce as the pushrod/spring load (not the wheel load) #355

**Open**
**coasting-nc** opened this issue on Mar 13, 2026

## Description

Fix Lockup Vibration and Suspension Bottoming with correct interpretation of mSuspForce as the pushrod/spring load (not the wheel load)

Review any use of mSuspForce to verify any misunderstanding of the telemetry channel mSuspForce (it is the pushrod/spring load, not the wheel load)

mSuspForce is the pushrod/spring load, not the wheel load. Because of the suspension geometry (motion ratio), the inboard spring typically sees significantly more force than the tire contact patch. In prototypes (like the Cadillac Hypercar in your report), the motion ratio is often around 0.5 (meaning the pushrod sees 2x the force of the wheel).

Among the uses mSuspForce to review, include:
* bool is_grounded = (w.mSuspForce > PREDICTION_LOAD_THRESHOLD);
* void FFBEngine::calculate_suspension_bottoming()

See how this was fixed in the use of mSuspForce to estimate tire load:
* Improve unsprung mass approximation in approximate_load and approximate_rear_load #345 (comment)
* ded44aa

TODO:
* implement the fixes described in the report below. However, do a refactoring to avoid the magic numbers for Motion Ratio. Establish a single source of truth for the values of Motion Ratio.

## Report

This misunderstanding of mSuspForce (treating pushrod load as wheel load) has cascading effects across several FFB features, causing them to behave inconsistently between car classes.

Because prototypes have a motion ratio (MR) around 0.5, their pushrod sees twice the force and twice the rate of change compared to the actual wheel contact patch.

Here is a review of the specific uses of mSuspForce in the codebase, why they are currently flawed, and how we must fix them.

---

### 1. Lockup Vibration: is_grounded Check

Location: `FFBEngine::calculate_lockup_vibration`
Current Code:
```cpp
bool is_grounded = (w.mSuspForce > PREDICTION_LOAD_THRESHOLD); // Threshold is 50.0
```

The Flaw:
If a car catches air (e.g., over a large curb), the wheel drops. The unsprung mass (wheel, tire, brakes ~45kg/450N) hangs off the suspension, pulling the pushrod down. This means mSuspForce actually goes negative (e.g., -450N / MR).
While > 50.0 technically works to detect if it's not airborne, it's physically arbitrary. Furthermore, if the suspension has droop limiters, mSuspForce might just read 0.0.

The Fix:
We should check the actual Tire Load at the contact patch. If the telemetry is encrypted, we use our newly improved `approximate_load()` function, which correctly accounts for the unsprung mass and motion ratio.

Update in `src/FFBEngine.cpp` (inside `calculate_lockup_vibration`):
```cpp
        // Signal Quality Check (Reject surface bumps)
        double susp_vel = std::abs(w.mVerticalTireDeflection - m_prev_vert_deflection[i]) / ctx.dt;
        bool is_bumpy = (susp_vel > (double)m_lockup_bump_reject);

        // Pre-conditions
        bool brake_active = (data->mUnfilteredBrake > PREDICTION_BRAKE_THRESHOLD);

        // FIX: Use actual tire load (or our accurate approximation if encrypted)
        double current_load = ctx.frame_warn_load ? approximate_load(w) : w.mTireLoad;
        bool is_grounded = (current_load > PREDICTION_LOAD_THRESHOLD);
```

---

### 2. Suspension Bottoming: Impulse Detection (Method B)

Location: `FFBEngine::calculate_suspension_bottoming`
Current Code:
```cpp
        double dForceL = (data->mWheel[0].mSuspForce - m_prev_susp_force[0]) / ctx.dt;
        double dForceR = (data->mWheel[1].mSuspForce - m_prev_susp_force[1]) / ctx.dt;
        double max_dForce = (std::max)(dForceL, dForceR);

        if (max_dForce > BOTTOMING_IMPULSE_THRESHOLD_N_S) { // 100kN/s impulse
```

The Flaw:
This calculates the derivative (rate of change) of the pushrod force. If a GT3 car (MR = 0.65) and a Hypercar (MR = 0.50) hit the exact same bump with the exact same wheel force, the Hypercar's pushrod will experience a much larger force spike.
Because of this, the `BOTTOMING_IMPULSE_THRESHOLD_N_S` (100,000 N/s) will trigger constantly on prototypes, but rarely on GT cars. The FFB will feel like the prototype is constantly bottoming out over minor bumps.

The Fix:
We must multiply the pushrod force derivative by the Motion Ratio to normalize the impulse back to the wheel.

Update in `src/FFBEngine.cpp` (inside `calculate_suspension_bottoming`):
```cpp
    } else {
        // Method 1: Suspension Force Impulse (Rate of Change)

        // FIX: Normalize pushrod impulse to wheel impulse using Motion Ratio
        double mr = 0.65; // Default GT
        if (m_current_vclass == ParsedVehicleClass::HYPERCAR ||
            m_current_vclass == ParsedVehicleClass::LMP2_UNRESTRICTED ||
            m_current_vclass == ParsedVehicleClass::LMP2_RESTRICTED ||
            m_current_vclass == ParsedVehicleClass::LMP2_UNSPECIFIED ||
            m_current_vclass == ParsedVehicleClass::LMP3) {
            mr = 0.50; // Prototypes
        }

        double dForceL = ((data->mWheel[0].mSuspForce - m_prev_susp_force[0]) * mr) / ctx.dt;
        double dForceR = ((data->mWheel[1].mSuspForce - m_prev_susp_force[1]) * mr) / ctx.dt;
        double max_dForce = (std::max)(dForceL, dForceR);

        if (max_dForce > BOTTOMING_IMPULSE_THRESHOLD_N_S) { // 100kN/s impulse at the WHEEL
            triggered = true;
            intensity = (max_dForce - BOTTOMING_IMPULSE_THRESHOLD_N_S) / BOTTOMING_IMPULSE_RANGE_N_S;
        }
    }
```

---

### 3. Suspension Bottoming: Safety Trigger Fallback

Location: `FFBEngine::calculate_suspension_bottoming`
Current Code:
```cpp
    // Safety Trigger: Raw Load Peak (Catches Method 0/1 failures)
    if (!triggered) {
        double max_load = (std::max)(data->mWheel[0].mTireLoad, data->mWheel[1].mTireLoad);
        double bottoming_threshold = m_static_front_load * BOTTOMING_LOAD_MULT;
```

The Flaw:
This safety fallback relies entirely on `mTireLoad`. If the user is driving an encrypted car (like the Cadillac Hypercar), `mTireLoad` is 0.0. Therefore, `max_load` is 0.0, and this safety trigger will never fire for encrypted cars.

The Fix:
Use the same fallback logic we use everywhere else.

Update in `src/FFBEngine.cpp` (inside `calculate_suspension_bottoming`):
```cpp
    // Safety Trigger: Raw Load Peak (Catches Method 0/1 failures)
    if (!triggered) {
        // FIX: Support encrypted cars by using the approximation fallback
        double load_l = ctx.frame_warn_load ? approximate_load(data->mWheel[0]) : data->mWheel[0].mTireLoad;
        double load_r = ctx.frame_warn_load ? approximate_load(data->mWheel[1]) : data->mWheel[1].mTireLoad;
        double max_load = (std::max)(load_l, load_r);

        double bottoming_threshold = m_static_front_load * BOTTOMING_LOAD_MULT;
        if (max_load > bottoming_threshold) {
            triggered = true;
            double excess = max_load - bottoming_threshold;
            intensity = std::sqrt(excess) * BOTTOMING_INTENSITY_SCALE; // Non-linear response
        }
    }
```

## Summary

By making these three changes, we completely decouple the FFB logic from the raw pushrod forces. The Lockup Prediction and Bottoming effects will now feel mathematically consistent across all car classes, regardless of whether their suspension geometry uses a 0.5 or 0.8 motion ratio, and regardless of whether their telemetry is encrypted.
