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
