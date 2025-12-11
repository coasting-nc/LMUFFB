Based on the analysis of the provided codebase (`FFBEngine.h`, `InternalsPlugin.hpp`) and the constraint that `mGripFract` and `mTireLoad` are currently returning **0.0**, here is the detailed breakdown of the impact and solutions.

### 1. Effects "Disabled" by Missing Data

When `mGripFract` and `mTireLoad` are 0, the `FFBEngine` triggers sanity checks (fallbacks) or fails conditions, effectively neutralizing specific dynamic behaviors.

*   **Understeer Effect (Grip Modulation):**
    *   *Current Logic:* `grip_factor = 1.0 - ((1.0 - avg_grip) * m_understeer_effect)`.
    *   *Impact:* The sanity check sets `avg_grip` to **1.0** (Full Grip) when it detects 0. Consequently, `grip_factor` becomes `1.0`.
    *   *Result:* The steering force is never reduced. The "lightening" of the wheel during understeer is **completely disabled**.
*   **Oversteer Boost:**
    *   *Current Logic:* Depends on `grip_delta = avg_grip - avg_rear_grip`.
    *   *Impact:* If all tires report 0 grip (and fallback to 1.0), the delta is `1.0 - 1.0 = 0`.
    *   *Result:* The boost multiplier is never applied. The effect is **disabled**.
*   **Suspension Bottoming:**
    *   *Current Logic:* `if (max_load > 8000.0)`.
    *   *Impact:* The sanity check sets load to **4000.0** (fallback). Since $4000 < 8000$, the condition is never met.
    *   *Result:* The effect is **completely disabled**.
*   **Dynamic Amplitude Scaling (Slide, Road, Lockup):**
    *   *Current Logic:* These effects multiply their output by `load_factor`.
    *   *Impact:* `load_factor` becomes a static **1.0** (4000/4000) due to the fallback.
    *   *Result:* The effects still work (you hear/feel them), but they are **static**. They do not get heavier in compressions (Eau Rouge) or lighter over crests.

---

### 2. Workarounds to Approximate `mTireLoad` and `mGripFract`

Yes, we can approximate these values using other available telemetry from `InternalsPlugin.hpp`.

#### Approximating `mTireLoad` (Vertical Load)
We can reconstruct a dynamic load using Suspension Force and Aerodynamics.
*   **Primary Proxy:** **`mSuspForce`** (found in `TelemWheelV01`).
    *   *Why:* This represents the pushrod load. While it excludes unsprung mass (wheel weight), it captures weight transfer and bumps perfectly.
    *   *Formula:* `ApproxLoad = mSuspForce + StaticUnsprungWeight` (Estimate ~300N).
*   **Secondary Proxy (Aero):**
    *   `TelemInfoV01` provides **`mFrontDownforce`** and **`mRearDownforce`**.
    *   You can add `(mFrontDownforce / 2)` to the front wheels' static weight to get a better load estimate at speed.

#### Approximating `mGripFract` (Grip Usage)
Since we cannot know the exact friction coefficient of the asphalt/tire combo, we must infer grip loss from **Slip Angles**.
*   **Logic:** Tires generally reach peak grip at a specific slip angle (e.g., ~0.15 radians or 8-10 degrees). Beyond this, grip falls off (Understeer).
*   **Calculation:**
    1.  Calculate **Slip Angle** ($\alpha$) manually (see Section 5).
    2.  Map $\alpha$ to a curve.
    *   *Formula:* `ApproxGrip = 1.0 - max(0.0, (abs(SlipAngle) - OptimalSlip) * FalloffRate)`.

---

### 3. Alternative Formulations for Disabled Effects

We can rewrite the logic in `FFBEngine.h` to bypass the missing variables.

#### A. Alternative Understeer (Slip-Based)
Instead of relying on the game to tell us we lost grip (`mGripFract`), we calculate it geometrically.

```cpp
// In calculate_force()
double slip_angle_fl = std::atan2(std::abs(fl.mLateralPatchVel), std::abs(fl.mLongitudinalGroundVel));
double slip_angle_fr = std::atan2(std::abs(fr.mLateralPatchVel), std::abs(fr.mLongitudinalGroundVel));
double avg_slip = (slip_angle_fl + slip_angle_fr) / 2.0;

// Threshold: 0.15 rad (approx 8.5 degrees) is a typical peak slip for GT cars
double grip_factor = 1.0;
if (avg_slip > 0.15) {
    // Reduce force as we exceed peak slip
    double excess_slip = avg_slip - 0.15;
    grip_factor = 1.0 - (excess_slip * m_understeer_effect * 2.0); 
    grip_factor = std::max(0.2, grip_factor); // Don't go below 20%
}
double output_force = game_force * grip_factor;
```

#### B. Alternative Bottoming (Deflection-Based)
Instead of Load, use the physical suspension travel limit.
*   **Source:** `mSuspensionDeflection` (meters).
*   **Logic:** Most cars have ~10-15cm travel. If deflection > 0.10m (tunable), trigger effect.

```cpp
if (fl.mSuspensionDeflection > 0.12) { // 12cm threshold
    // Trigger bottoming crunch
}
```

---

### 4. Additional FFB Effects (New Features)

Given the data in `InternalsPlugin.hpp`, we can add:

1.  **Hybrid/Electric Motor Haptics (LMU Specific):**
    *   **Source:** `mElectricBoostMotorTorque` and `mElectricBoostMotorState`.
    *   **Effect:** Inject a high-frequency sine wave (150Hz) when `mElectricBoostMotorState == 2` (Propulsion) or a rougher texture when `== 3` (Regen).
2.  **Engine Vibration:**
    *   **Source:** `mEngineRPM`.
    *   **Effect:** Inject a subtle vibration at `(RPM / 60.0) * 2` Hz. Adds immersion at idle/revving.
3.  **Surface/Kerb Rumble:**
    *   **Source:** `mTerrainName` (char array) or `mSurfaceType` (enum).
    *   **Effect:** If `mSurfaceType == 5` (Rumblestrip), boost the Road Texture gain by 2x. If `2` (Grass), add low-frequency wobble.
4.  **Gear Shift Kick:**
    *   **Source:** Monitor `mGear`.
    *   **Effect:** When `mGear` changes, inject a single frame high-force impulse (jolt).

---

### 5. Using Tire Patch Velocity

`mLateralPatchVel` and `mLongitudinalPatchVel` are the most valuable "raw" physics data points available.

**Improving Existing Effects:**
*   **Slide Texture Frequency:** Currently implemented in v0.4.0. Keep this. It is physically correct ($Frequency \propto Velocity$).
*   **Slip Angle Calculation:** As mentioned in Section 3A, you can calculate the *true* slip angle using:
    $$ \alpha = \arctan\left(\frac{\text{LateralPatchVel}}{\text{LongitudinalGroundVel}}\right) $$
    This allows you to detect understeer even if `mSlipAngle` or `mGripFract` provided by the game are broken.

**New Effect: Scrub Drag**
*   If `mLateralPatchVel` is high, the tires are dragging sideways. You can add a constant force *opposing* the direction of the slide (sign of `mLateralPatchVel`) to simulate the physical resistance of the rubber dragging on asphalt.

---

### 6. Using Wheel Rotation & Radius (Lockup/Slip Inference)

Yes, we can accurately calculate dynamic lockup and slip effects without relying on the game's pre-calculated `mSlipRatio`.

**Data Needed:**
*   $\omega$ (`mRotation`): Wheel angular velocity (rad/s).
*   $r$ (`mStaticUndeflectedRadius`): Tire radius (needs conversion: the struct says `unsigned char` in cm? *Check `InternalsPlugin.hpp` carefully, it might be `mTireRadius` in `TelemWheelV01` if available, otherwise estimate 0.33m*).
*   $V_{car}$ (`mLocalVel.z`): Car speed (m/s).

**Calculations:**

1.  **Wheel Surface Speed ($V_{wheel}$):**
    $$ V_{wheel} = \omega \times r $$
    *(Note: Check units. If radius is not available, you can calibrate it: when coasting straight, $r = V_{car} / \omega$)*.

2.  **Inferred Slip Ratio:**
    $$ \text{Ratio} = \frac{V_{wheel} - V_{car}}{V_{car}} $$

**Inferring Lockup (Braking):**
*   **Condition:** `mUnfilteredBrake > 0.1` AND `Ratio < -0.2`.
*   **Proximity:** The closer `Ratio` gets to -1.0 (Full Lock, $V_{wheel}=0$), the higher the vibration amplitude.
*   **Dynamic Effect:**
    *   *Frequency:* Based on $V_{car}$ (Scrubbing speed).
    *   *Amplitude:* Based on `abs(Ratio)`.

**Inferring Wheel Spin (Acceleration):**
*   **Condition:** `mUnfilteredThrottle > 0.1` AND `Ratio > 0.2`.
*   **Proximity:** As `Ratio` increases (wheel spinning faster than car), increase vibration frequency (revving sensation).

**Conclusion:**
Yes, calculating these manually is **more robust** than relying on the game's `mSlipRatio`, especially if the game's tire data is partially zeroed. It guarantees the FFB matches the visual wheel rotation.
