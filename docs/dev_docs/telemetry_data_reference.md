# Telemetry Data Reference (LMU 1.2 API)

> **⚠️ API Source of Truth**  
> The official and authoritative reference for all telemetry data structures, field names, types, and units is:  
> **`src/lmu_sm_interface/InternalsPlugin.hpp`**  
> 
> This file is provided by Studio 397 as part of the LMU 1.2 shared memory interface. All code must defer to this header for:
> - **Units** (Newtons, Newton-meters, meters, radians, etc.)
> - **Field names** (e.g., `mSteeringShaftTorque`, not `mSteeringArmForce`)
> - **Data types** and struct layouts
> - **API version compatibility**
>
> When in doubt about telemetry interpretation, consult `InternalsPlugin.hpp` as the definitive source.

---

## Overview

This document lists the physics data available from the **Le Mans Ultimate 1.2 Native Shared Memory Interface** (structs `TelemInfoV01` and `TelemWheelV01`). It documents which values lmuFFB currently uses and explores potential future uses for enhanced Force Feedback.

**Changes from rFactor 2:** LMU 1.2 introduced native shared memory support with:
- **Direct torque measurement**: `mSteeringShaftTorque` (Nm) replaced force-based `mSteeringArmForce` (N)
- **Native tire data**: Direct access to `mTireLoad`, `mGripFract`, `mLateralPatchVel`
- **Patch velocities**: `mLateralPatchVel` and `mLongitudinalPatchVel` for accurate slip calculations
- **No plugin required**: Built directly into LMU, no external DLL needed

---

## 1. Global Vehicle Telemetry (`TelemInfoV01`)

These values describe the state of the vehicle chassis and engine.

| Variable | Units | Description | Current Usage | Future Potential |
| :--- | :--- | :--- | :--- | :--- |
| `mDeltaTime` | seconds | Time since last physics update | **Used**: Phase integration for oscillators | |
| `mElapsedTime` | seconds | Session time | Unused | Timestamping for logging |
| **`mSteeringShaftTorque`** | **Nm** | **Torque around steering shaft** (replaces `mSteeringArmForce`) | **Used**: Primary FFB source (v0.4.0+) | |
| `mLocalAccel` | m/s² | Acceleration in car-local space (X=Lat, Y=Vert, Z=Long) | **Used**: `x` for SoP (Seat of Pants) effect | `z` for braking dive/acceleration squat cues |
| `mLocalRot`, `mLocalRotAccel` | rad/s, rad/s² | Rotation rate/accel (Yaw/Pitch/Roll) | Unused | **High Priority**: Use Yaw Rate vs Steering Angle to detect oversteer more accurately than Grip Delta |
| `mLocalVel` | m/s | Velocity in local coordinates | **Used**: `z` for speed-based frequency scaling & sanity checks | |
| `mUnfilteredThrottle` | 0.0-1.0 | Raw throttle input | **Used**: Trigger for Wheel Spin effects | |
| `mUnfilteredBrake` | 0.0-1.0 | Raw brake input | **Used**: Trigger for Lockup effects | |
| `mEngineRPM` | RPM | Engine rotation speed | Unused | **Engine Vibration**: Inject RPM-matched vibration into the wheel |
| `mFuel`, `mEngineWaterTemp` | liters, °C | Vehicle health/status | Unused | Dash display data |
| `mElectricBoostMotorTorque` | Nm | Hybrid motor torque | Unused | **Hybrid Haptics**: Vibration during deployment/regen |
| `mElectricBoostMotorState` | enum | 0=unavailable, 2=propulsion, 3=regen | Unused | Trigger for hybrid-specific effects |

---

## 2. Wheel & Tire Telemetry (`TelemWheelV01`)

Available for each of the 4 wheels (`mWheel[0]`=FL, `[1]`=FR, `[2]`=RL, `[3]`=RR).

### Forces & Grip

| Variable | Units | Description | Current Usage | Future Potential |
| :--- | :--- | :--- | :--- | :--- |
| **`mTireLoad`** | **N** | **Vertical load on tire** | **Used**: Load scaling, Bottoming effect (v0.4.0+) | **Load Sensitivity**: Reduce FFB gain if front tires are unloaded |
| **`mGripFract`** | **0.0-1.0** | **Grip usage fraction** (0=full grip available, 1=at limit) | **Used**: Understeer/Oversteer detection (v0.4.0+) | |
| `mLateralForce` | N | Force acting sideways on tire contact patch | **Used**: Rear Oversteer calculation (Aligning Torque) | Front pneumatic trail calculation refinement |
| `mLongitudinalForce` | N | Force acting forward/back (Accel/Brake) | Unused | ABS pulse simulation |
| `mSuspForce` | N | Pushrod load | **Used**: Front Load approximation fallback (v0.4.7) | Suspension stress feedback |

### Motion & Slip

| Variable | Units | Description | Current Usage | Future Potential |
| :--- | :--- | :--- | :--- | :--- |
| **`mLateralPatchVel`** | **m/s** | **Lateral velocity at contact patch** | **Used**: Slide Texture frequency (v0.4.0+) | More accurate "scrub" feel |
| **`mLongitudinalPatchVel`** | **m/s** | **Longitudinal velocity at contact patch** | **Used**: Slip ratio calculation (v0.4.0+) | |
| `mLateralGroundVel` | m/s | Lateral velocity of ground under tire | Unused | Slip angle refinement |
| `mLongitudinalGroundVel` | m/s | Longitudinal velocity of ground under tire | **Used**: Slip ratio calculation | |
| `mRotation` | rad/s | Wheel rotation speed | Unused | Damage wobble effects |

### Suspension & Surface

| Variable | Units | Description | Current Usage | Future Potential |
| :--- | :--- | :--- | :--- | :--- |
| `mVerticalTireDeflection` | m | Compression of tire rubber | **Used**: Road Texture (High-pass filter) | |
| `mSuspensionDeflection` | m | Compression of spring/damper | Unused | **Bottoming Out**: Harsh "thud" if deflection hits max travel |
| `mRideHeight` | m | Chassis height | **Used**: Visualized in Telemetry Inspector (v0.4.7) | Scraping effects |
| `mTerrainName` | char[16] | Name of surface (e.g., "ROAD", "GRASS") | Unused | **Surface FX**: Different rumble for Kerbs/Grass/Gravel |
| `mSurfaceType` | unsigned char | 0=dry, 1=wet, 2=grass, 3=dirt, 4=gravel, 5=rumblestrip, 6=special | Unused | Faster lookup for Surface FX |
| `mCamber`, `mToe` | radians | Wheel alignment | Unused | Setup analysis |

### Condition

| Variable | Units | Description | Current Usage | Future Potential |
| :--- | :--- | :--- | :--- | :--- |
| `mTemperature[3]` | Kelvin | Inner/Middle/Outer tire temps | Unused | **Cold Tire Feel**: Reduce grip when cold |
| `mWear` | 0.0-1.0 | Tire wear fraction | Unused | **Wear Feel**: Reduce overall gain as tires wear |
| `mPressure` | kPa | Tire pressure | Unused | Pressure-sensitive handling |
| `mBrakeTemp` | °C | Brake disc temperature | Unused | **Brake Fade**: Judder when overheated |
| `mFlat`, `mDetached` | bool | Damage flags | Unused | **Damage FX**: Wobble if tire is flat |

---

## 3. Critical Unit Changes (v0.4.0+)

### Steering Force → Torque
**Old API (rFactor 2):** `mSteeringArmForce` (Newtons)  
**New API (LMU 1.2):** `mSteeringShaftTorque` (Newton-meters)

**Impact:** This required a ~200x scaling reduction in all FFB effect amplitudes to account for:
1. Unit change (Force → Torque)
2. Lever arm elimination (shaft measurement vs. rack measurement)

**Typical values:**
- Racing car steering torque: **15-25 Nm**
- Old force scaling: ~4000 N (incorrect for torque)
- New torque scaling: ~20 Nm (physically accurate)

---

## 4. Summary of "Low Hanging Fruit"

These are features that would provide high value with relatively low implementation effort:

1.  **Surface Effects**: Reading `mTerrainName`/`mSurfaceType` to detect "Rumble Strips" or "Kerbs" and injecting a specific vibration pattern.
2.  **Hybrid Haptics** (LMU-specific): Use `mElectricBoostMotorTorque` and `mElectricBoostMotorState` to add deployment/regen vibration.
3.  **Engine Vibration**: Adding a subtle RPM-based hum (`mEngineRPM`) adds immersion.
4.  **Suspension Bottoming**: Triggering a heavy jolt when `mSuspensionDeflection` or `mFront3rdDeflection` limits are reached.

---

## 5. Data Validation & Sanity Checks (v0.4.1+)

lmuFFB implements robust fallback logic for missing/invalid telemetry:

- **Missing Load**: If `mTireLoad < 1.0` while `|mLocalVel.z| > 1.0` for >20 frames (50ms), defaults to 4000N
- **Missing Grip**: If `mGripFract < 0.0001` while `mTireLoad > 100N`, defaults to 1.0
- **Invalid DeltaTime**: If `mDeltaTime <= 0.000001`, defaults to 0.0025s (400Hz)

These checks prevent FFB dropout during telemetry glitches.

---

## 6. Coordinate Systems & Sign Conventions (v0.4.30+)

Understanding the coordinate systems is critical for effect direction (e.g., ensuring SoP pulls the correct way).

### LMU / rFactor 2 Coordinate System
*   **X (Lateral)**: **+X is LEFT**, -X is RIGHT.
*   **Y (Vertical)**: +Y is UP, -Y is DOWN.
*   **Z (Longitudinal)**: +Z is REAR, -Z is FRONT.
*   **Rotation**: Left-handed system. +Y rotation (Yaw) is to the **RIGHT**.

### InternalsPlugin.hpp Note
The SDK explicitly warns:
> "Note that ISO vehicle coordinates (+x forward, +y right, +z upward) are right-handed. If you are using that system, **be sure to negate any rotation or torque data** because things rotate in the opposite direction."

### Effect Implementations
1.  **Lateral G (SoP)**:
    *   **Source**: `mLocalAccel.x` (Linear Acceleration).
    *   **Right Turn**: Car accelerates LEFT (+X).
    *   **Desired Force**: Aligning torque should pull LEFT (+).
    *   **Implementation**: **No Inversion**. Use `+mLocalAccel.x`.
2.  **Yaw Acceleration (Kick)**:
    *   **Source**: `mLocalRotAccel.y` (Rotational Acceleration).
    *   **Right Oversteer**: Car rotates RIGHT (+Y).
    *   **Desired Force**: Counter-steer kick should pull RIGHT (-).
    *   **Implementation**: **Invert**. Use `-mLocalRotAccel.y` (as per SDK "negate rotation" note).
3.  **Rear Aligning Torque**:
    *   **Source**: `mLateralPatchVel` (Linear Velocity).
    *   **Right Turn**: Rear slides LEFT (+Vel).
    *   **Desired Force**: Aligning torque should pull LEFT (+).
    *   **Implementation**: The formula `double rear_torque = -calc_rear_lat_force` correctly produces a Positive output for Positive velocity inputs due to the negative coefficient in the `calc` helper. **Already Correct.**
