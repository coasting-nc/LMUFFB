
# Core issue

The core issue is this: "sudden pulls/loss of FFB off centre at random times. Almost like the “reverse FFB” setting was working intermittently".


# Proposed fix

Based on the user's report and the code analysis, I have identified the bug. It is a **logic error in `FFBEngine.h`** related to how the **Rear Aligning Torque** and **Slip Angle** are calculated.

### The Root Cause: Conditional Execution of Physics

The user reports "random" pulls and "intermittent" behavior. This is caused by the fact that the **Rear Aligning Torque** effect is currently **turning ON and OFF randomly** based on the quality of the game's connection, and when it turns on, it uses stale data that causes violent spikes.

**The Bug in `calculate_grip` (FFBEngine.h):**
Currently, the code *only* calculates the Slip Angle (and updates its smoothing filter) **IF** the game fails to provide Grip data.

```cpp
// Current Logic (Simplified)
if (grip < 0.0001) { 
    // 1. Telemetry is missing (Bugged frame)
    // 2. Calculate Slip Angle (and update LPF state)
    // 3. Use Slip Angle for Rear Torque
} else {
    // 1. Telemetry is good (Normal frame)
    // 2. Slip Angle is set to 0.0
    // 3. Rear Torque becomes 0.0
}
```

**The Consequence:**
1.  **Intermittent "Reverse" Feel:** As long as the game sends valid grip data, the **Rear Aligning Torque** (which provides the counter-steer force) is **0.0**.
2.  **The Spike (Lock Up):** LMU telemetry is known to be "glitchy" (dropping frames). When a frame drops (Grip becomes 0), the `if` block executes.
    *   The Rear Torque suddenly jumps from **0 Nm** to **~6 Nm** (due to the new v0.4.11 scaling).
    *   Worse, the **Smoothing Filter (LPF)** for the slip angle hasn't been updated for seconds/minutes. It tries to smooth the *current* slide against *ancient* history, potentially resulting in a mathematical spike or erratic value.
3.  **Result:** The wheel kicks violently ("sudden pull") or locks up because the force toggles instantly between "Game Only" and "Game + Calculated Rear Torque".

### The Fix

We must calculate the Slip Angle **every single frame**, regardless of whether we need it for the Grip Fallback. This ensures:
1.  The **Rear Aligning Torque** is always active and consistent (no on/off toggling).
2.  The **Smoothing Filter** is always up-to-date (no spikes from stale state).

### Corrected Code for `FFBEngine.h`

You need to modify the `calculate_grip` function.

**File:** `FFBEngine.h`
**Function:** `calculate_grip` (Lines ~260-295)

```cpp
    // Helper: Calculate Grip with Fallback (v0.4.6 Hardening)
    GripResult calculate_grip(const TelemWheelV01& w1, 
                              const TelemWheelV01& w2,
                              double avg_load,
                              bool& warned_flag,
                              double& prev_slip1,
                              double& prev_slip2,
                              double car_speed) {
        GripResult result;
        result.original = (w1.mGripFract + w2.mGripFract) / 2.0;
        result.value = result.original;
        result.approximated = false;
        
        // --- FIX START: ALWAYS CALCULATE SLIP ANGLE ---
        // We must calculate this every frame to keep the Low Pass Filter (LPF) 
        // state (prev_slip) updated and to provide valid data for Rear Aligning Torque.
        double slip1 = calculate_slip_angle(w1, prev_slip1);
        double slip2 = calculate_slip_angle(w2, prev_slip2);
        result.slip_angle = (slip1 + slip2) / 2.0;
        // --- FIX END ---

        // Fallback condition: Grip is essentially zero BUT car has significant load
        if (result.value < 0.0001 && avg_load > 100.0) {
            result.approximated = true;
            
            // Low Speed Cutoff (v0.4.6)
            if (car_speed < 5.0) {
                // Note: We still keep the calculated slip_angle in result.slip_angle
                // for visualization/rear torque, even if we force grip to 1.0 here.
                result.value = 1.0; 
            } else {
                // Use the pre-calculated slip angle
                double excess = (std::max)(0.0, result.slip_angle - 0.10);
                result.value = 1.0 - (excess * 4.0);
            }
            
            // Safety Clamp (v0.4.6): Never drop below 0.2 in approximation
            result.value = (std::max)(0.2, result.value);
            
            if (!warned_flag) {
                std::cout << "[WARNING] Missing Grip. Using Approx based on Slip Angle." << std::endl;
                warned_flag = true;
            }
        }
        
        result.value = (std::max)(0.0, (std::min)(1.0, result.value));
        return result;
    }
```

### Summary of Changes for the User
You can explain to the user:
"I found the issue. The 'Rear Aligning Torque' effect was accidentally turning itself ON and OFF depending on whether the game telemetry skipped a frame. This caused the random 'kicks' and the feeling of the FFB reversing, because the force would suddenly jump from 0% to 100% in a split second. I have fixed it so the physics calculation runs continuously."

# Documentation updates

Here is the documentation package to ensure this fix is preserved and understood in the future.

I have divided this into three parts:
1.  **In-Code Comments (`FFBEngine.h`)**: The first line of defense.
2.  **New Incident Report (`docs/dev_docs/`)**: A detailed explanation of the "Why".
3.  **Update to `AGENTS_MEMORY.md`**: To ensure future AI sessions respect this constraint.

---

### 1. Code Comments (Apply to `FFBEngine.h`)

Replace the relevant section in `calculate_grip` with this heavily commented version. The `CRITICAL` tag helps future developers/AI spot it immediately.

```cpp
    // Helper: Calculate Grip with Fallback (v0.4.6 Hardening)
    GripResult calculate_grip(const TelemWheelV01& w1, 
                              const TelemWheelV01& w2,
                              double avg_load,
                              bool& warned_flag,
                              double& prev_slip1,
                              double& prev_slip2,
                              double car_speed) {
        GripResult result;
        result.original = (w1.mGripFract + w2.mGripFract) / 2.0;
        result.value = result.original;
        result.approximated = false;
        
        // ==================================================================================
        // CRITICAL LOGIC FIX (v0.4.14) - DO NOT MOVE INSIDE CONDITIONAL BLOCK
        // ==================================================================================
        // We MUST calculate slip angle every single frame, regardless of whether the 
        // grip fallback is triggered or not.
        //
        // Reason 1 (Physics State): The Low Pass Filter (LPF) inside calculate_slip_angle 
        //           relies on continuous execution. If we skip frames (because telemetry 
        //           is good), the 'prev_slip' state becomes stale. When telemetry eventually 
        //           fails, the LPF will smooth against ancient history, causing a math spike.
        //
        // Reason 2 (Dependency): The 'Rear Aligning Torque' effect (calculated later) 
        //           reads 'result.slip_angle'. If we only calculate this when grip is 
        //           missing, the Rear Torque effect will toggle ON/OFF randomly based on 
        //           telemetry health, causing violent kicks and "reverse FFB" sensations.
        // ==================================================================================
        
        double slip1 = calculate_slip_angle(w1, prev_slip1);
        double slip2 = calculate_slip_angle(w2, prev_slip2);
        result.slip_angle = (slip1 + slip2) / 2.0;

        // Fallback condition: Grip is essentially zero BUT car has significant load
        if (result.value < 0.0001 && avg_load > 100.0) {
            // ... [Rest of fallback logic] ...
```

---

### 2. New Documentation File

Create a new file: **`docs/dev_docs/bug_analysis_rear_torque_instability.md`**

```markdown
# Bug Analysis: Rear Torque Instability & "Reverse FFB" Kicks

**Date:** 2025-12-14
**Fixed In:** v0.4.14
**Severity:** Critical
**Component:** `FFBEngine.h` -> `calculate_grip`

## The Symptom
Users reported "sudden pulls," "loss of FFB off-center," and behavior that felt like the "Reverse FFB" setting was toggling intermittently. This often resulted in the wheel locking up fully to the left or right.

## The Root Cause
The issue stemmed from **Conditional Physics Execution**.

In versions prior to v0.4.14, the **Slip Angle** calculation was nested *inside* the Grip Fallback logic:

```cpp
// BROKEN LOGIC (Do not use)
if (grip_telemetry_missing) {
    slip_angle = calculate_slip_angle(...); // Updates LPF state
    // ... use slip angle for grip ...
} 
// else: slip_angle remains 0.0
```

### The Chain Reaction
1.  **Dependency:** The **Rear Aligning Torque** effect (introduced in v0.4.11) relies on `m_grip_diag.rear_slip_angle`.
2.  **The Toggle:** 
    *   **Frame A (Good Telemetry):** Grip is valid. The `if` block is skipped. `slip_angle` is 0.0. **Rear Torque is 0.0.**
    *   **Frame B (Bad Telemetry):** Grip drops to 0 (common LMU bug). The `if` block runs. `slip_angle` is calculated (e.g., 0.15 rad). **Rear Torque jumps to ~6.0 Nm.**
3.  **The Spike:** Because the Low Pass Filter (LPF) inside `calculate_slip_angle` wasn't running during the "Good" frames, its internal state (`prev_slip`) was stale (potentially seconds old). When Frame B hit, the filter tried to smooth the current value against ancient history, often resulting in a mathematical spike.

## The Fix
The Slip Angle calculation was moved **outside** the conditional block. It now runs **every frame**.

```cpp
// CORRECT LOGIC
slip_angle = calculate_slip_angle(...); // Always update LPF state

if (grip_telemetry_missing) {
    // ... use slip angle for grip ...
}
```

## Regression Prevention
**Do not optimize this code by moving the calculation back inside the `if` block.** 
Even if `slip_angle` seems unused for the *Grip* calculation when telemetry is valid, it is **required** for the *Rear Aligning Torque* effect which runs downstream.
```

---

### 3. Update `AGENTS_MEMORY.md`

Append this to the **"Critical Constraints & Math"** section of `AGENTS_MEMORY.md`. This ensures future AI agents don't try to "optimize" the code by putting the calculation back inside the if-statement.

```markdown
### Continuous Physics State (Anti-Glitch)
*   **Rule:** Never make the calculation of physics state variables (like Slip Angle, RPM smoothing, or LPFs) conditional on telemetry health or other flags.
*   **Why:** 
    1.  **Filters:** Low Pass Filters (LPF) rely on a continuous stream of `dt` updates. If you stop calling them, their internal state becomes stale. When you call them again, they produce a spike.
    2.  **Downstream Dependencies:** A variable calculated in a "Fallback" block (like `slip_angle` in `calculate_grip`) might be used by a completely different effect later (like `Rear Aligning Torque`).
*   **Incident:** See `docs/dev_docs/bug_analysis_rear_torque_instability.md`. We caused violent wheel kicks by only calculating Slip Angle when Grip was missing.
```

# Other sources of instability to fix

Based on a thorough inspection of the code, I have identified **two additional sources of instability** related to "Conditional State Updates". These are similar in nature to the bug we just fixed, where internal history variables become "stale" when an effect is disabled, causing violent spikes when re-enabled or when switching modes.

### 1. The "Road Texture" Spike
**Location:** `FFBEngine.h` inside `calculate_force` (Lines ~600-620)
**The Issue:**
The variable `m_prev_vert_deflection` (used to calculate the suspension velocity for road bumps) is only updated **inside** the `if (m_road_texture_enabled)` block.

```cpp
if (m_road_texture_enabled) {
    // ... calculation ...
    double delta_l = vert_l - m_prev_vert_deflection[0]; // Uses OLD state
    // ...
    m_prev_vert_deflection[0] = vert_l; // Updates state
}
```

**The Scenario:**
1.  User disables "Road Details". `m_prev_vert_deflection` stops updating (e.g., stays at 0.05m).
2.  User drives a lap. Suspension moves to 0.08m.
3.  User enables "Road Details".
4.  **The Spike:** The code calculates `delta = 0.08 - 0.05 = 0.03m`. This is a massive instantaneous jump (equivalent to hitting a 3cm curb in 0.002 seconds).
5.  **Result:** A violent "BANG" in the wheel upon enabling the effect. (Note: The v0.4.6 clamp limits this to 0.01m, reducing the damage, but it's still a max-force kick).

### 2. The "Bottoming" Method Switch Spike
**Location:** `FFBEngine.h` inside `calculate_force` (Lines ~630-650)
**The Issue:**
The variable `m_prev_susp_force` (used for Method B: Suspension Spike) is only updated when `m_bottoming_enabled` is true **AND** `m_bottoming_method == 1`.

**The Scenario:**
1.  User runs with "Method A" (Scraping). `m_prev_susp_force` is stale (e.g., 0 or old value).
2.  User switches to "Method B" (Spike) while driving.
3.  **The Spike:** The derivative calculation `(current_force - prev_force) / dt` sees a jump from 0 to 3000N instantly.
    *   `dForce = 3000 / 0.0025 = 1,200,000 N/s`.
4.  **Result:** This exceeds the threshold (100,000), triggering a false "Bottoming" crunch immediately upon switching methods.

---

### The Fix: Unconditional State Updates

We must move the state update logic **outside** of the conditional effect blocks. The "Previous State" must always track the "Current State" of the physics, regardless of whether the FFB effect is currently active.

**Apply this change to `FFBEngine.h`:**

1.  **Remove** the lines `m_prev_vert_deflection[...] = ...` and `m_prev_susp_force[...] = ...` from their respective `if` blocks.
2.  **Add** a new section at the very end of `calculate_force` (before the return) to update these states unconditionally.

```cpp
    double calculate_force(const TelemInfoV01* data) {
        // ... [All existing logic] ...

        // --- 5. Suspension Bottoming ---
        if (m_bottoming_enabled) {
             // ... [Existing logic, BUT REMOVE m_prev_susp_force updates from here] ...
        }

        // ... [Min Force Logic] ...

        // ==================================================================================
        // CRITICAL: UNCONDITIONAL STATE UPDATES (Fix for Toggle Spikes)
        // ==================================================================================
        // We must update history variables every frame, even if effects are disabled.
        // This prevents "stale state" spikes when effects are toggled on.
        
        // Road Texture State
        m_prev_vert_deflection[0] = fl.mVerticalTireDeflection;
        m_prev_vert_deflection[1] = fr.mVerticalTireDeflection;

        // Bottoming Method B State
        m_prev_susp_force[0] = fl.mSuspForce;
        m_prev_susp_force[1] = fr.mSuspForce;
        // ==================================================================================

        // --- SNAPSHOT LOGIC ---
        // ...
        
        return (std::max)(-1.0, (std::min)(1.0, norm_force));
    }
```

### Recommendation
Applying this fix along with the previous "Rear Torque" fix will eliminate the remaining sources of "random" instability caused by toggling settings or telemetry glitches.
