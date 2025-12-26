# Investigation: Removing vJoy Dependency (vJoy-less Architecture)

**Date:** 2025-05-23
**Status:** Investigation

## Goal
Determine if LMUFFB can function without requiring the user to install vJoy and bind it in-game, similar to how "Marvin's AIRA" or other FFB tools might operate.

## Current Architecture (vJoy Dependent)
1.  **Game Output:** Le Mans Ultimate (rF2 Engine) requires a DirectInput device to "drive". It sends physics forces to this device.
2.  **The Trick:** We bind the game to a **vJoy Device** (Virtual Joystick). The game thinks it's driving a wheel.
3.  **Telemetry:** LMUFFB reads telemetry via Shared Memory.
4.  **App Output:** LMUFFB calculates forces and sends them to the **Physical Wheel** via DirectInput.

## Alternative: "Marvin's Method" (Hypothesis)

Marvin's AIRA (for iRacing) and irFFB operate differently because iRacing has a specific "FFB API" or allows disabling FFB output while keeping input active.

### rFactor 2 / LMU Constraints
rFactor 2 is an older engine. It typically couples Input (Steering Axis) with Output (FFB).
- If you unbind FFB, you might lose Steering Input? No, you can usually set FFB Type to "None".
- **Problem:** If you bind your **Physical Wheel** as the Steering Axis in-game, the game *automatically* attempts to acquire it for FFB (Exclusive Mode).
- **Conflict:** LMUFFB also needs to acquire the Physical Wheel (Exclusive Mode) to send forces.
- **Result:** Race condition. "Double FFB".

### How to solve this without vJoy?

#### Method 1: The "None" FFB Hack
If LMU allows binding an axis *without* acquiring the device for FFB:
1.  Bind Physical Wheel Axis X to Steering in-game.
2.  Set "FFB Type" to **"None"** in game settings.
3.  LMUFFB acquires the wheel via DirectInput.
    *   **Risk:** If the game opens the device in "Exclusive" mode just to read the Axis, LMUFFB might be blocked from writing FFB (or vice versa). DirectInput usually allows "Background" "Non-Exclusive" reads, but FFB writing often requires "Exclusive".
    *   **Test Required:** Can LMUFFB send FFB to a device that LMU is actively reading steering from?
        - If yes: vJoy is NOT needed.
        - If no (Access Denied): We need vJoy.
    *   **LMU Specific Note:** Users report LMU lacks a "None" FFB option. The workaround is setting FFB strength to 0 or disabling effects, which *might* not release the exclusive lock.

#### Method 2: Proxy Driver (HID Guardian)
Some apps use a filter driver (like HidGuardian) to hide the physical wheel from the game, presenting a virtual one instead. This effectively *is* vJoy but hidden. Complex to install.

#### Method 3: UDP / API Injection
If LMU had an API to inject steering input (like Assetto Corsa), we wouldn't need to bind a physical axis. It does not.

## Conclusion & Next Steps
The dependency on vJoy exists primarily to **prevent the game from locking the physical wheel**.

**Experiment for v0.4.0:**
Try "Method 1":
1.  User binds real wheel in game.
2.  User disables FFB in game (Type: None).
3.  LMUFFB attempts to acquire wheel in `DISCL_BACKGROUND | DISCL_EXCLUSIVE` mode.
4.  If it works, vJoy is obsolete.
5.  If it fails (Device Busy), we need vJoy.

**Verdict:** For now, vJoy is the most reliable method to ensure signal separation. Marvin's app likely benefits from iRacing's more modern non-exclusive input handling.
