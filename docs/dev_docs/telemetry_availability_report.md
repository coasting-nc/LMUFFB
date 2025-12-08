# Le Mans Ultimate Telemetry Availability Report

**Date:** 2025-05-23
**Version:** 1.0
**Target:** LMUFFB v0.3.2

## Executive Summary
Recent investigations into the Le Mans Ultimate (LMU) community forums indicate that while the game uses the rFactor 2 Shared Memory Plugin (DAMPlugin), certain telemetry data points are inconsistent, hidden, or intermittently broken across updates. 

While most complaints focus on Dashboard data (ERS, Temperatures, Fuel), there is a risk that physics-related fields used by LMUFFB could be affected, particularly those related to tire state (Grip, Load).

## Critical Telemetry Dependencies
LMUFFB relies on the following fields from `rF2Telemetry`. If these are zero or static, specific effects will fail.

| Field | Effect | Risk Level | Notes |
| :--- | :--- | :--- | :--- |
| `mSteeringArmForce` | **Master FFB** | Low | Core game FFB. If missing, no force at all. |
| `mTireLoad` | Slide Texture, Bottoming | Medium | Used for amplitude scaling. If 0, effects are silent. |
| `mVerticalTireDeflection` | Road Texture | Medium | Used for bump detection. |
| `mSlipAngle` | Slide Texture | Low | Essential for physics; unlikely to be hidden. |
| `mSlipRatio` | Lockup, Spin | Low | Essential for physics. |
| `mLateralPatchVel` | Slide Texture (Freq) | Medium | Advanced physics field; new in v0.3.2. |
| `mLocalAccel` | SoP (Lateral G) | Low | Required for Motion Rigs; likely present. |
| `mLocalVel` | Freq Scaling | Low | Basic vector. |
| `mGripFract` | Understeer (Grip Loss) | **High** | Derived from Tire Temp/Wear/Surface. **Temps reported broken.** |
| `mLateralForce` | Oversteer Boost | Low | Core physics. |

## Findings from Community Research
1.  **Dashboard Data Issues:** Users report missing ERS State (SOC), TC Level, ABS Level, and Motor Maps. This confirms LMU does not expose the full rFactor 2 telemetry set.
2.  **Tire Data Instability:** Reports from Feb 2024 indicate `mTemperature` and `mPressure` were broken in a specific build, then partially fixed.
    *   **Impact:** If Tire Temperature is not simulated or exposed, `mGripFract` (which usually depends on temp) might be calculated incorrectly or return a static value.
3.  **Plugin Compatibility:** The standard rF2 DAMPlugin works but causes "poor performance & pit menu flicker" for some users. 
    *   **Mitigation:** LMUFFB only *reads* the memory mapped file; it does not inject a plugin DLL into the game process (unlike the official DAMPlugin which *writes* the memory). **However**, the user *must* have the DAMPlugin installed and running for the memory map to exist. If the plugin causes instability, users might disable it, breaking LMUFFB.

## Recommendations for LMUFFB

### 1. Robustness Checks (Implemented v0.3.19)
We have implemented "Sanity Checks" in `FFBEngine` to mitigate missing data:
*   **Tire Load:** If `mTireLoad` is 0.0 while the car is moving (> 1 m/s), it defaults to 4000N. This ensures Slide and Road textures remain audible/active even if load data is missing.
*   **Grip Fraction:** If `mGripFract` is 0.0 but load exists (> 100N), it defaults to 1.0 (Full Grip). This prevents the Understeer effect from muting the FFB entirely.
*   **Delta Time:** If `mDeltaTime` is invalid (<= 0), it defaults to 0.0025s (400Hz) to prevent division-by-zero errors in integrators.

### 2. Telemetry Inspector (Implemented v0.3.12)
The GUI now includes "Rolling Trace Plots" for telemetry inputs. In v0.3.19, we added specific **Warning Indicators** (Red Text) that trigger if the fallback logic above is activated.

### 3. Fallback Logic
If `mLateralPatchVel` (newly used) proves unreliable, we should fallback to `mLateralGroundVel` (which is standard kinematic data).

## Conclusion
The core physics vectors (Velocity, Acceleration, Force) are likely safe as they are essential for motion platforms which are supported. The highest risk is with **Tire State** data (`mGripFract`, `mTireLoad`) which might be simplified or hidden in LMU compared to rFactor 2.

With the v0.3.19 robustness update, the application is now resilient against total loss of tire state data.
