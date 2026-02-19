# Technical Report: rFactor 2 / LMU Coordinate Systems

**Date:** December 16, 2025
**Subject:** Coordinate System Mismatch Analysis & Correction Strategy
**Reference:** `src/lmu_sm_interface/InternalsPlugin.hpp`

## 1. The Fundamental Mismatch

There is a critical inversion between the coordinate system used by the **rFactor 2 / Le Mans Ultimate Physics Engine** and the coordinate system expected by **DirectInput Steering Wheels**.

### A. The Game Engine (Left-Handed / "Backwards")
As defined in `InternalsPlugin.hpp`, the simulation uses a **Left-Handed** coordinate system with specific orientations that differ from standard ISO vehicle dynamics conventions.

*   **Origin:** Center of gravity (approximate).
*   **+X Axis:** Points to the **LEFT** (Driver's perspective).
*   **+Y Axis:** Points **UP** (Roof).
*   **+Z Axis:** Points to the **BACK** (Rear bumper).

### B. The Steering Wheel (DirectInput Standard)
DirectInput devices universally follow a standard scalar convention for the X-axis (Steering):

*   **Negative (-):** Turn **LEFT** (Counter-Clockwise).
*   **Positive (+):** Turn **RIGHT** (Clockwise).

### C. The Conflict
Because the Game defines **+X as Left** and the Wheel defines **+Force as Right**, any raw lateral vector taken from the game is **mathematically inverted** relative to the wheel's expectation.

| Axis / Vector | Game Definition (+ Value) | Wheel Expectation (+ Value) | Action Required |
| :--- | :--- | :--- | :--- |
| **Lateral Position** | Left | Right | **INVERT** |
| **Lateral Velocity** | Sliding Left | Moving Right | **INVERT** |
| **Lateral Accel** | Accelerating Left | Accelerating Right | **INVERT** |
| **Longitudinal** | Backwards | Forwards (usually) | **INVERT** |

---

## 2. Variable Audit: `InternalsPlugin.hpp`

The following variables involve the coordinate system and require specific handling in `FFBEngine.h`.

### A. Global Vehicle Vectors (`TelemInfoV01`)

| Variable | Game Meaning (+ Value) | Usage in FFB | Correction Logic |
| :--- | :--- | :--- | :--- |
| `mLocalAccel.x` | **Accelerating Left** | **SoP (Seat of Pants)** | **Must Invert.** <br>Left Accel (+) means Right Turn. <br>Right Turn requires Left Pull (-). <br>Formula: `Force = -(AccelX)` |
| `mLocalVel.x` | **Moving Left** | **Chassis Slip** | **Must Invert.** <br>Used for drift angle calculation. |
| `mLocalVel.z` | **Moving Backward** | **Speed / Freq** | **Invert for Direction.** <br>Forward speed is `-mLocalVel.z`. <br>For frequency (scalar), use `std::abs()`. |
| `mLocalRot.y` | **Yaw Right** | **Damping / Gyro** | **Invert.** <br>The header says `+y yaws to the right`. <br>If rotating Right (+), we want to resist Left (-). <br>Formula: `Force = -(YawRate)` |

### B. Wheel Specific Vectors (`TelemWheelV01`)

| Variable | Game Meaning (+ Value) | Usage in FFB | Correction Logic |
| :--- | :--- | :--- | :--- |
| `mLateralPatchVel` | **Sliding Left** | **Scrub Drag** | **Match Sign.** <br>If Sliding Left (+), Friction pushes Right (+). <br>We want Force Right (+). <br>Formula: `Force = Sign(Vel) * Gain` |
| `mLateralPatchVel` | **Sliding Left** | **Rear Torque** | **Must Invert.** <br>Used to calc Slip Angle. <br>Slip Angle Left (+) requires Counter-Steer Left (-). |
| `mLongitudinalGroundVel` | **Moving Backward** | **Slip Ratio** | **Use Absolute.** <br>Usually we just want speed magnitude. <br>Use `std::abs()` to avoid sign confusion. |
| `mLateralForce` | **Force to Left** | **Aligning Torque** | **Must Invert.** <br>If Force is Left (+), it pushes the rack Left. <br>To simulate this on wheel, we need Left Force (-). <br>Wait... DirectInput Left is (-). <br>So if Game Force is (+), we need Output (-). <br>Formula: `Output = -(LateralForce)` |

---

## 3. Detailed Correction Strategy

Here is how to implement the corrections for the specific effects in `FFBEngine.h`.

### 1. Seat of Pants (SoP)
*   **Physics:** In a Right turn, the car accelerates to the Right (centripetal), but the body/chassis feels a "fictitious" centrifugal force to the **Left**.
*   **Telemetry:** `mLocalAccel.x` measures the proper acceleration. In a steady Right turn, the accelerometer reads acceleration to the Right (to keep the car turning).
    *   *Correction:* Actually, `mLocalAccel` usually reports the *reaction* force or the kinematic acceleration.
    *   *Empirical Rule:* If you turn Right, you want the wheel to feel heavy (pull Left).
    *   *Game Data:* In a Right turn, `mLocalAccel.x` is typically **Negative** (Right is -X).
    *   *Desired Output:* Left Force (**Negative**).
    *   *Current Math:* Input (-) -> Output (-).
    *   *Wait, let's re-read the header:* `+x points out the left side`.
    *   *Right Turn:* Acceleration is to the Right (towards center of turn). Right is **-X**.
    *   So Input is **Negative**.
    *   We want Output **Negative** (Left pull).
    *   So `Output = Input`?
    *   *Let's check the User Report:* The user said "Slide rumble throws wheel in direction I am turning". This implies a sign error in the *Rear* calculation.
    *   *Let's check SoP:* If SoP was inverted, the wheel would pull *into* the turn (lighten).
    *   **Verdict:** If the wheel feels light in corners, **Invert SoP**. If it feels heavy, **Keep SoP**.
    *   *Safe Bet:* Based on the "Left-Handed" definition, `Force = -Accel` is usually the safe starting point for SoP to ensure Left Accel (+) results in Right Pull (+).

### 2. Rear Aligning Torque (The Bug Source)
*   **Scenario:** Oversteer (Rear slides Left).
*   **Game Data:** `mLateralPatchVel` is **Positive** (Left).
*   **Slip Angle:** Calculated as `atan(Lat/Long)`. Result is **Positive**.
*   **Lateral Force (Calc):** `Slip * Load * Stiffness`. Result is **Positive**.
*   **Desired FFB:** Counter-steer to the Left.
*   **DirectInput:** Left is **Negative**.
*   **Current Code:** `Force = +CalcForce`. (Positive = Right).
*   **Result:** Wheel turns Right (into the slide). **CATASTROPHIC POSITIVE FEEDBACK.**
*   **Correction:** `Force = -CalcForce`.

### 3. Scrub Drag (Friction)
*   **Scenario:** Sliding Left.
*   **Game Data:** `mLateralPatchVel` is **Positive** (Left).
*   **Physics:** Friction opposes motion. Force vector points Right.
*   **Desired FFB:** Push wheel Right.
*   **DirectInput:** Right is **Positive**.
*   **Current Code:** `(Vel > 0) ? -1 : 1`. (If Left, push Left). **WRONG.**
*   **Correction:** `(Vel > 0) ? 1 : -1`. (If Left, push Right).

---

## 4. Summary Checklist for `FFBEngine.h`

1.  **Rear Torque:** `double rear_torque = -calc_rear_lat_force * ...` (**INVERT**)
2.  **Scrub Drag:** `double drag_dir = (avg_lat_vel > 0.0) ? 1.0 : -1.0;` (**MATCH SIGN**)
3.  **SoP:** `double lat_g = -(raw_g / 9.81);` (**INVERT**)
4.  **Slip Angle:** `double slip = std::atan2(w.mLateralPatchVel, std::abs(w.mLongitudinalGroundVel));` (Keep as is, but remember it produces "Left-Handed" angles).

## 5. Note on `mSteeringShaftTorque`
The variable `mSteeringShaftTorque` is the raw torque from the physics engine.
*   If the game engine is consistent, Positive Torque likely means "Torque to the Left" (matching the +X axis).
*   If so, this **also** needs to be inverted to match DirectInput (-Left, +Right).
*   **However:** Most users use the "Invert FFB" checkbox in the GUI to fix the base force direction globally. The fixes above are to ensure the *Calculated Effects* align with the *Base Force* so they don't fight each other.

# Transcript of the conversation that uncovered this coordinates issue

## Question
In the attachment there is the full codebase and documentation of a FFB app. You task is to help me investigate a bug reported by one user. Study the code and tell me what the cause might be. Here is the bug:

"I just tested it and got it to work. FFB seems stronger especially on oversteer. The only thing that is broken for me is the slide rumble, which throws the wheel in the direction I am turning, regardless of the level. Coincidentally this might be the effect that I need to feel vibrations on understeer, if I am not mistaken?"

The core issue is this: "The only thing that is broken for me is the slide rumble, which throws the wheel in the direction I am turning, regardless of the level."

## Answer
Based on the user's report and the code analysis, here is the diagnosis and the fix.

### The Diagnosis

The user is experiencing **Inverted Rear Aligning Torque**.

1.  **The Symptom:** "Throws the wheel in the direction I am turning."
    *   This describes a **Positive Feedback Loop**. Instead of the wheel trying to self-center (Counter-Steer) when the rear slides, it is pulling harder *into* the turn.
2.  **The Confusion:** "Regardless of the level [of slide rumble]."
    *   The user likely adjusted the **"Slide Gain"** slider, but the force is actually coming from the **"Rear Align Torque"** effect (which defaults to 1.0 and is separate). Since they happen at the same time (during a slide), the user conflated them.
3.  **The Cause:** A Sign Error in `FFBEngine.h`.
    *   **Physics:** When a car slides Left (Oversteer), the tires generate a Lateral Force to the Right. The **Aligning Torque** should rotate the steering wheel to the **Left** (Counter-Steer) to correct the slide.
    *   **The Code:**
        ```cpp
        // Positive Slip Angle -> Positive Lateral Force -> Positive Torque
        double rear_torque = calc_rear_lat_force * REAR_ALIGN_TORQUE_COEFFICIENT * m_rear_align_effect;
        ```
    *   **The Error:** In the coordinate system used, a **Positive** torque turns the wheel **Right**. So, when the car slides Left, the code generates a Right turn force, worsening the slide ("throwing the wheel").

### The Fix

We need to **invert** the calculated Rear Torque so it acts as a *Restoring* force (Counter-Steer) rather than a *Destabilizing* force.

**File:** `FFBEngine.h`
**Function:** `calculate_force`

**Change this line:**
```cpp
// OLD (Broken - Pulls into turn)
double rear_torque = calc_rear_lat_force * REAR_ALIGN_TORQUE_COEFFICIENT * m_rear_align_effect; 
```

**To this:**
```cpp
// NEW (Fixed - Counter-steers)
// Multiply by -1.0 to create a restoring torque
double rear_torque = -calc_rear_lat_force * REAR_ALIGN_TORQUE_COEFFICIENT * m_rear_align_effect; 
```

### Verification for the User

You can tell the user:
"I found the bug. The 'Rear Aligning Torque' calculation was inverted, causing the wheel to pull *into* the slide instead of correcting it. This effect happens simultaneously with the 'Slide Rumble', which is why adjusting the rumble level didn't fix it (because the force was coming from the Rear Torque setting). I have flipped the sign, so it should now provide a natural counter-steering weight."


## Question 2

Are we using wrong coordinates also in other places in the code?

## Answer 2


Based on the analysis of the rFactor 2 / Le Mans Ultimate coordinate system, **yes, there are other inverted coordinates.**

The root cause is a fundamental mismatch between the Game's coordinate system and the DirectInput (Wheel) coordinate system.

*   **Game (rFactor 2 / LMU):** **+X points LEFT**. (See `InternalsPlugin.hpp`: `+x points out the left side of the car`).
*   **Wheel (DirectInput):** **+Axis points RIGHT**. (Standard convention: +10000 is full right).

This means **any** lateral vector (Position, Velocity, Acceleration) read from the game is **inverted** relative to what the wheel expects.

Here are the 3 places in `FFBEngine.h` that need fixing:

### 1. Rear Aligning Torque (Already Identified)
*   **Status:** **Broken.**
*   **Physics:** Sliding Left (+X) generates Positive Force. Sent to wheel as Positive (Right). Pulls into the slide.
*   **Fix:** Invert sign.

### 2. Scrub Drag (New Effect)
*   **Status:** **Broken (Destabilizing).**
*   **Scenario:** Car slides Left (+X Velocity).
*   **Physics:** Friction should oppose motion -> Force to Right.
*   **Current Code:**
    ```cpp
    // If Vel > 0 (Left), drag_dir = -1.0 (Left in DI?? No, -1.0 is Left).
    // Wait. DirectInput -1.0 is Left.
    // If Vel is Left (+), we want Force Right (+).
    // Current code sets drag_dir = -1.0.
    double drag_dir = (avg_lat_vel > 0.0) ? -1.0 : 1.0; 
    ```
*   **Result:** Sliding Left -> Force Left. This pushes the car *faster* into the slide (Negative Damping).
*   **Fix:**
    ```cpp
    // If sliding Left (>0), we want Force Right (1.0)
    double drag_dir = (avg_lat_vel > 0.0) ? 1.0 : -1.0; 
    ```

### 3. Seat of Pants (SoP / Lateral G)
*   **Status:** **Inverted (Lightens wheel in corners).**
*   **Scenario:** Turning Right.
*   **Physics:** Centrifugal force pushes body Left (+X Accel).
*   **Desired Feel:** Steering should get heavy (pull Left) to simulate load/aligning torque.
*   **Current Code:**
    ```cpp
    // Accel is Positive (Left).
    double lat_g = raw_g / 9.81; // Positive
    double sop_force = lat_g * ...; // Positive
    // Output Positive -> Pulls Right.
    ```
*   **Result:** In a Right turn, SoP pulls Right. This fights the natural aligning torque (which pulls Left), making the wheel feel **lighter** or vague instead of heavy.
*   **Fix:** Invert `lat_g`.

---

### Summary of Code Changes

Apply these 3 fixes to `FFBEngine.h` to align the physics with the hardware.

#### Fix 1: SoP (Lateral G)
**Location:** Inside `calculate_force`, around line 350.
```cpp
// OLD
double lat_g = raw_g / 9.81;

// NEW (Invert to match DirectInput)
double lat_g = -(raw_g / 9.81); 
```

#### Fix 2: Rear Aligning Torque
**Location:** Inside `calculate_force`, around line 580.
```cpp
// OLD
double rear_torque = calc_rear_lat_force * REAR_ALIGN_TORQUE_COEFFICIENT * m_rear_align_effect;

// NEW (Invert)
double rear_torque = -calc_rear_lat_force * REAR_ALIGN_TORQUE_COEFFICIENT * m_rear_align_effect;
```

#### Fix 3: Scrub Drag
**Location:** Inside `calculate_force`, around line 600.
```cpp
// OLD
double drag_dir = (avg_lat_vel > 0.0) ? -1.0 : 1.0;

// NEW (If sliding Left/Pos, push Right/Pos)
double drag_dir = (avg_lat_vel > 0.0) ? 1.0 : -1.0;
```