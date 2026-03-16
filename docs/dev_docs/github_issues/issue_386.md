# Implement more logging for NaN/Inf detections #386

**coasting-nc opened this on Mar 16, 2026**

## Description

Implement more logging for NaN/Inf detections

## Answer / Report

You are absolutely right to be concerned about this. In software engineering, this is known as "swallowing errors". When you silently sanitize bad data (like turning NaN into 0.0), the app survives, but you blind yourself to underlying bugs in the game's telemetry or your own math.

If Studio 397 introduces a bug in a future LMU update that causes mLocalRot.y to randomly output NaN, you want to know about it in the logs, rather than just wondering why the FFB feels slightly numb.

To do this without spamming the log file (which would cause massive disk I/O lag and stuttering), we use a Cooldown Timer Pattern. You already have a similar pattern in FFBSafetyMonitor.cpp for spike detection.

Here is how to implement a robust, rate-limited diagnostic logging system for your sanitization checks.

### 1. Add Diagnostic Timers to FFBEngine.h

Add these variables to track the last time a specific type of error was logged.
```cpp
// Diagnostic Log Cooldowns (v0.7.x)
double m_last_core_nan_log_time = -999.0;
double m_last_aux_nan_log_time = -999.0;
double m_last_math_nan_log_time = -999.0;
```

### 2. Reset the Timers on Context Switch

In FFBEngine.cpp, inside the calculate_force transition block (where we wipe the filters clean when m_was_allowed != allowed), reset these timers so that a new session or a new car will immediately log any errors it encounters.
```cpp
if (m_was_allowed != allowed) {
    // ... existing resets ...
    m_derivatives_seeded = false;

    // --- NEW: Reset diagnostic timers ---
    m_last_core_nan_log_time = -999.0;
    m_last_aux_nan_log_time = -999.0;
    m_last_math_nan_log_time = -999.0;
}
```

### 3. Implement Rate-Limited Logging in calculate_force

Now, update the sanitization blocks we discussed earlier to include the logging logic. We will use a 5-second cooldown for each category.

#### A. Core Physics Crash Detection
```cpp
// --- 1. CORE PHYSICS CRASH DETECTION ---
if (!std::isfinite(data->mUnfilteredSteering) ||
    !std::isfinite(data->mLocalRot.y) ||
    !std::isfinite(data->mLocalAccel.x) ||
    !std::isfinite(data->mLocalAccel.z) ||
    !std::isfinite(data->mSteeringShaftTorque) ||
    !std::isfinite(genFFBTorque)) {

    // Rate-limited logging (once every 5 seconds)
    if (data->mElapsedTime > m_last_core_nan_log_time + 5.0) {
        Logger::Get().LogFile("[Diag] Core Physics NaN/Inf detected! (Steering, Accel, or Torque). FFB muted for this frame.");
        m_last_core_nan_log_time = data->mElapsedTime;
    }
    return 0.0;
}
```

#### B. Auxiliary Data Sanitization
Instead of just silently replacing the NaNs, we set a flag if any of them were triggered, and log it once.
```cpp
// --- 2. AUXILIARY DATA SANITIZATION ---
bool aux_nan_detected = false;
for (int i = 0; i < 4; i++) {
    if (!std::isfinite(m_working_info.mWheel[i].mTireLoad)) { m_working_info.mWheel[i].mTireLoad = 0.0; aux_nan_detected = true; }
    if (!std::isfinite(m_working_info.mWheel[i].mGripFract)) { m_working_info.mWheel[i].mGripFract = 0.0; aux_nan_detected = true; }
    if (!std::isfinite(m_working_info.mWheel[i].mSuspForce)) { m_working_info.mWheel[i].mSuspForce = 0.0; aux_nan_detected = true; }
    if (!std::isfinite(m_working_info.mWheel[i].mVerticalTireDeflection)) { m_working_info.mWheel[i].mVerticalTireDeflection = 0.0; aux_nan_detected = true; }
    if (!std::isfinite(m_working_info.mWheel[i].mLateralPatchVel)) { m_working_info.mWheel[i].mLateralPatchVel = 0.0; aux_nan_detected = true; }
    if (!std::isfinite(m_working_info.mWheel[i].mLongitudinalPatchVel)) { m_working_info.mWheel[i].mLongitudinalPatchVel = 0.0; aux_nan_detected = true; }
    if (!std::isfinite(m_working_info.mWheel[i].mRotation)) { m_working_info.mWheel[i].mRotation = 0.0; aux_nan_detected = true; }
    if (!std::isfinite(m_working_info.mWheel[i].mBrakePressure)) { m_working_info.mWheel[i].mBrakePressure = 0.0; aux_nan_detected = true; }
}

if (aux_nan_detected && data->mElapsedTime > m_last_aux_nan_log_time + 5.0) {
    Logger::Get().LogFile("[Diag] Auxiliary Wheel NaN/Inf detected and sanitized to 0.0.");
    m_last_aux_nan_log_time = data->mElapsedTime;
}
```

#### C. Final Math Output Catch-All
At the very end of calculate_force, right before returning the value to the hardware:
```cpp
// --- NEW: Final NaN catch-all ---
if (!std::isfinite(norm_force)) {
    if (data->mElapsedTime > m_last_math_nan_log_time + 5.0) {
        Logger::Get().LogFile("[Diag] Final output force is NaN/Inf! Internal math instability detected. Muting FFB.");
        m_last_math_nan_log_time = data->mElapsedTime;
    }
    norm_force = 0.0;
}

return (std::max)(-1.0, (std::min)(1.0, norm_force));
```

### Why this is the perfect approach:

1. **No Log Spam:** If a car is completely broken and outputs NaN 400 times a second, your log file will only get 1 line every 5 seconds. This prevents the log file from ballooning to gigabytes in size and prevents disk I/O from causing game stutters.
2. **Highly Greppable:** By prefixing these specific errors with [Diag], you or any user can easily open the log file and CTRL+F for [Diag] to instantly see if the game's telemetry or your internal math is failing.
3. **Categorized Failures:** You will instantly know where the failure happened. If you see Core Physics NaN, you know the game glitched. If you see Final output force is NaN, you know the game data was fine, but one of your internal C++ formulas (like a division by zero in the slip angle calculation) caused the math to explode.
