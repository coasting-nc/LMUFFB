# Telemetry Data Reference

This document lists the physics data available from the **rFactor 2 Shared Memory Map Plugin** (struct `rF2Telemetry` and `rF2Wheel`). It documents which values LMUFFB currently uses and explores potential future uses for enhanced Force Feedback.

## 1. Global Vehicle Telemetry (`rF2Telemetry`)

These values describe the state of the vehicle chassis and engine.

| Variable | Description | Current Usage | Future Potential |
| :--- | :--- | :--- | :--- |
| `mTime`, `mDeltaTime` | Simulation time/step. | **Used**: Driving oscillators (sine waves) for effects. | Timestamping for logging. |
| `mElapsedTime` | Session time. | **Used**: Phase for texture generation. | |
| `mSteeringArmForce` | Torque on the steering rack (Game Physics). | **Used**: Primary FFB source. | |
| `mLocalAccel` | Acceleration in car-local space (X=Lat, Y=Vert, Z=Long). | **Used**: `x` for SoP (Seat of Pants) effect. | `z` for braking dive/acceleration squat cues. |
| `mLocalRot`, `mLocalRotAccel` | Rotation rate/accel (Yaw/Pitch/Roll). | Unused. | **High Priority**: Use Yaw Rate vs Steering Angle to detect oversteer more accurately than Grip Delta. |
| `mSpeed` | Vehicle speed (m/s). | Unused. | Speed-sensitive damping (reduce FFB oscillations at high speed). |
| `mEngineRPM` | Engine rotation speed. | Unused. | **Engine Vibration**: Inject RPM-matched vibration into the wheel (common in fanatec pedals/wheels). |
| `mUnfilteredThrottle` | Raw throttle input. | **Used**: Trigger for Wheel Spin effects. | |
| `mUnfilteredBrake` | Raw brake input. | **Used**: Trigger for Lockup effects. | |
| `mPos`, `mLocalVel`, `mOri` | World position/velocity/orientation. | **Used**: `z` velocity for frequency scaling & sanity checks. | Motion platform integration? (Out of scope for FFB). |
| `mFuel`, `mEngineWaterTemp`, etc. | Vehicle health/status. | Unused. | Dash display data. |

---

## 2. Wheel & Tire Telemetry (`rF2Wheel`)

Available for each of the 4 wheels (`mWheels[0]`=FL, `[1]`=FR, `[2]`=RL, `[3]`=RR).

### Forces & Grip
| Variable | Description | Current Usage | Future Potential |
| :--- | :--- | :--- | :--- |
| `mSteeringArmForce` (Global) | **Note**: This is global, but derived from FL/FR tie rods. | **Used**. | |
| `mLateralForce` | Force acting sideways on the tire contact patch. | **Used**: Rear Oversteer calculation (Aligning Torque). | Front pneumatic trail calculation refinement. |
| `mLongitudinalForce` | Force acting forward/back (Accel/Brake). | Unused. | ABS pulse simulation (modulate brake force). |
| `mTireLoad` | Vertical load (N) on the tire. | **Used**: Slide Texture, Bottoming (Includes fallback for 0-value glitches). | **Load Sensitivity**: Reduce FFB gain if front tires are unloaded (cresting a hill). |
| `mGripFract` | Grip usage fraction (0.0=No Grip used, 1.0=Limit). | **Used**: Understeer lightness & Oversteer logic. | |
| `mMaxLatGrip` | Theoretical max lateral grip. | Unused. | Normalizing force values across different cars. |

### Motion & Slip
| Variable | Description | Current Usage | Future Potential |
| :--- | :--- | :--- | :--- |
| `mSlipAngle` | Angle between tire direction and velocity vector. | **Used**: Slide Texture trigger. | Pneumatic trail calculation (Slip * Trail Curve). |
| `mSlipRatio` | Difference between wheel rotation and road speed. | **Used**: Lockup & Spin progressive effects. | |
| `mLateralPatchVel` | Velocity of the contact patch sliding sideways. | **Used**: Slide Texture Frequency. | More accurate "scrub" sound/feel than Slip Angle alone. |
| `mRotation` | Wheel rotation speed (rad/s). | Unused. | |

### Suspension & Surface
| Variable | Description | Current Usage | Future Potential |
| :--- | :--- | :--- | :--- |
| `mVerticalTireDeflection` | Compression of the tire rubber. | **Used**: Road Texture (High-pass filter). | |
| `mSuspensionDeflection` | Compression of the spring/damper. | Unused. | **Bottoming Out**: Add a harsh "thud" if deflection hits max travel. |
| `mRideHeight` | Chassis height. | Unused. | Scraping effects. |
| `mTerrainName` | Name of surface (e.g., "ROAD", "GRASS"). | Unused. | **Surface FX**: Different rumble patterns for Kerbs vs Grass vs Gravel. |
| `mSurfaceType` | ID of surface. | Unused. | Faster lookup for Surface FX. |
| `mCamber`, `mToe` | Static/Dynamic alignment. | Unused. | Setup analysis (not FFB). |

### Condition
| Variable | Description | Current Usage | Future Potential |
| :--- | :--- | :--- | :--- |
| `mTemperature[3]` | Inner/Middle/Outer tire temps. | Unused. | **Cold Tire Feel**: Reduce grip/force when tires are below optimal temp range. |
| `mWear` | Tire wear fraction. | Unused. | **Wear Feel**: Reduce overall gain as tires wear out (long stint simulation). |
| `mPressure` | Tire pressure. | Unused. | |
| `mFlat`, `mDetached` | Damage flags. | Unused. | **Damage FX**: Add wobble if tire is flat or wheel detached. |

---

## 3. Summary of "Low Hanging Fruit"

These are features that would provide high value with relatively low implementation effort:

1.  **Surface Effects**: Reading `mTerrainName` to detect "Rumble Strips" or "Kerbs" and injecting a specific vibration pattern. rFactor 2 FFB is usually good at this, but enhancing it (like iRFFB's "Kerb Effect") is popular.
2.  **Engine Vibration**: Adding a subtle RPM-based hum (`mEngineRPM`) adds immersion, especially for idle/revving.
3.  **Suspension Bottoming**: Triggering a heavy jolt when `mSuspensionDeflection` limits are reached.
