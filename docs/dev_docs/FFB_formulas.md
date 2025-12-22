# FFB Mathematical Formulas (v0.4.12+)

> **⚠️ API Source of Truth**  
> All telemetry data units and field names are defined in **`src/lmu_sm_interface/InternalsPlugin.hpp`**.  
> That is the official Studio 397 interface specification for LMU 1.2.  
> Critical: `mSteeringShaftTorque` is in **Newton-meters (Nm)**, not Newtons.

Based on the `FFBEngine.h` file, here is the complete mathematical breakdown of the Force Feedback calculation.

The final output sent to the DirectInput driver is a normalized value between **-1.0** and **1.0**.

### 1. The Master Formula

$$
F_{\text{final}} = \text{Clamp}\left( \left( \frac{F_{\text{total}}}{20.0} \times K_{\text{gain}} \right), -1.0, 1.0 \right)
$$

**Note**: Reference changed from 4000 (N, old rF2 API) to 20.0 (Nm, LMU 1.2 API) in v0.4.0+.

Where **$F_{\text{total}}$** is the sum of all physics components:

$$
F_{\text{total}} = (F_{\text{base}} + F_{\text{sop}} + F_{\text{vib\\_lock}} + F_{\text{vib\\_spin}} + F_{\text{vib\\_slide}} + F_{\text{vib\\_road}} + F_{\text{vib\\_bottom}} + F_{\text{gyro}}) \times M_{\text{spin\\_drop}}
$$

*(Note: $M_{\text{spin\\_drop}}$ is a reduction multiplier active only during traction loss).*

---

### 2. Component Breakdown

#### A. Global Factors (Pre-calculated)

**Load Factor ($\text{Front\\_Load\\_Factor}$)**: Scales texture effects based on how much weight is on the front tires.

$$
\text{Front\\_Load\\_Factor} = \text{Clamp}\left( \frac{\text{Front\\_Load}_{\text{FL}} + \text{Front\\_Load}_{\text{FR}}}{2 \times 4000}, 0.0, 1.5 \right)
$$

*   **Robustness Check:** If $\text{Front\\_Load} \approx 0.0$ and $|Velocity| > 1.0 m/s$, $\text{Front\\_Load}$ defaults to 4000N to prevent signal dropout.
*   **Safety Clamp (v0.4.6):** $\text{Front\\_Load\\_Factor}$ is hard-clamped to a maximum of **2.0** (regardless of configuration) to prevent unbounded forces during aero-spikes.
*   **Adaptive Kinematic Load (v0.4.38):** If telemetry load (`mTireLoad`) AND suspension force (`mSuspForce`) are missing (encrypted content), tire load is estimated kinematically:
    $$ F_{z} = F_{\text{static}} + F_{\text{aero}} + F_{\text{long\\_transfer}} + F_{\text{lat\\_transfer}} $$
    *   $F_{\text{static}}$: Mass (1100kg) distributed by weight bias (55% Rear).
    *   $F_{\text{aero}}$: $K_{\text{aero}} \times V^2$.
    *   $F_{\text{transfer}}$: Derived from **Smoothed** Chassis Acceleration (simulating inertia).

#### B. Base Force (Understeer / Grip Modulation)

This modulates the raw steering rack force from the game based on front tire grip.

### B. Base Force (Understeer / Grip Modulation) - Updated v0.4.13

The base force logic has been expanded to support debugging modes and attenuation.

**1. Base Input Selection (Mode)**
Depending on `m_base_force_mode` setting:
*   **Mode 0 (Native):** $\text{Base}_{\text{input}} = T_{\text{steering\\_shaft}}$
*   **Mode 1 (Synthetic):**
    *   If $|T_{\text{steering\\_shaft}}| > 0.5$ Nm: $\text{Base}_{\text{input}} = \text{sign}(T_{\text{steering\\_shaft}}) \times \text{MaxTorqueRef}$
    *   Else: $\text{Base}_{\text{input}} = 0.0$
    *   *Purpose: Constant force to tune Grip Modulation in isolation.*
*   **Mode 2 (Muted):** $\text{Base}_{\text{input}} = 0.0$

**2. Modulation & Attenuation**

$$
F_{\text{base}} = (\text{Base}_{\text{input}} \times K_{\text{shaft\\_gain}}) \times \left( 1.0 - \left( (1.0 - \text{Front\\_Grip}_{\text{avg}}) \times K_{\text{understeer}} \right) \right)
$$

*   $K_{\text{shaft\\_gain}}$: New slider (0.0-1.0) to attenuate base force without affecting telemetry.
*   $\text{Front\\_Grip}_{\text{avg}}$: Average of Front Left and Front Right `mGripFract`.
    *   **Fallback (v0.4.5+):** If telemetry grip is missing ($\approx 0.0$) but Load $> 100N$, grip is approximated from **Slip Angle**.
        * **Low Speed Trap (v0.4.6):** If CarSpeed < 5.0 m/s, Grip = 1.0.
        * **Slip Angle LPF (v0.4.37 Update):** Slip Angle is smoothed using **Time-Corrected** Exponential Moving Average.
            * $\alpha = dt / (0.0225 + dt)$
            * Target: Equivalent to $\alpha=0.1$ at 400Hz ($\tau \approx 0.0225s$).
            * Ensures consistent physics response regardless of frame rate.
        * $\text{Slip} = \text{atan2}(V_{\text{lat}}, V_{\text{long}})$
        * **Combined Friction Circle (v0.4.38):**
            * Grip loss is now calculated from the vector sum of Lateral Slip ($\alpha$) and Longitudinal Slip ($\kappa$).
            * $\text{Metric}_{\text{lat}} = \alpha / 0.10$
            * $\text{Metric}_{\text{long}} = \kappa / 0.12$
            * $\text{Combined} = \sqrt{\text{Metric}_{\text{lat}}^2 + \text{Metric}_{\text{long}}^2}$
            * If $\text{Combined} > 1.0$: $\text{Grip} = 1.0 / (1.0 + (\text{Combined} - 1.0) \times 2.0)$.
        * **Safety Clamp (v0.4.6):** Calculated Grip never drops below **0.2**.

#### C. Seat of Pants (SoP) & Oversteer

This injects lateral G-force and rear-axle aligning torque to simulate the car body's rotation.

1.  **Smoothed Lateral G (v0.4.6):** Calculated via Time-Corrected LPF.
    *   **Input Clamp (v0.4.6):** Raw AccelX is clamped to +/- 5G ($49.05 m/s^2$) before processing.

    $$
    G_{\text{smooth}} = G_{\text{prev}} + \alpha_{\text{time\\_corrected}} \times \left( \frac{\text{Chassis\\_Lat\\_Accel}}{9.81} - G_{\text{prev}} \right)
    $$

    *   $\alpha_{\text{time\\_corrected}} = dt / (\tau + dt)$ where $\tau$ is derived from user setting `m_sop_smoothing_factor`.
    *   $\alpha$: User setting `m_sop_smoothing_factor`.

2.  **Base SoP**:

    $$
    F_{\text{sop\\_base}} = G_{\text{smooth}} \times K_{\text{sop}} \times 20.0
    $$
    
    **Note**: Scaling changed from 5.0 to 20.0 in v0.4.10 to provide stronger baseline Nm output.

3.  **Yaw Acceleration (The Kick) - New v0.4.16, Smoothed v0.4.18**:

    $$
    F_{\text{yaw}} = -1.0 \times \text{YawAccel}_{\text{smoothed}} \times K_{\text{yaw}} \times 5.0
    $$
    
    *   Injects `mLocalRotAccel.y` (Radians/sec²) to provide a predictive kick when rotation starts.
    *   **v0.4.20 Fix:** Inverted calculation ($ -1.0 $) to provide counter-steering cue. Positive Yaw (Right rotation) now produces Negative Force (Left torque). verified by SDK note: **"negate any rotation or torque data"**.
    *   **v0.4.18 Fix (Updated v0.4.37):** Applied **Time-Corrected Low Pass Filter** ($\tau=0.0225s$) to prevent noise feedback loop.
        *   **Problem:** Slide Rumble vibrations caused yaw acceleration (a derivative) to spike with high-frequency noise.
        *   **Solution:** $\alpha = dt / (0.0225 + dt)$
        *   $\text{YawAccel}_{\text{smoothed}} = \text{YawAccel}_{\text{prev}} + \alpha \times (\text{YawAccel}_{\text{raw}} - \text{YawAccel}_{\text{prev}})$
        *   This filters out high-frequency noise while preserving actual rotation kicks regardless of frame rate.
    *   $K_{\text{yaw}}$: User setting `m_sop_yaw_gain` (0.0 - 2.0).

4.  **Oversteer Boost**:
    If Front Grip > Rear Grip:

    $$
    F_{\text{sop\\_boosted}} = F_{\text{sop\\_base}} \times \left( 1.0 + (\text{Grip}_{\text{delta}} \times K_{\text{oversteer}} \times 2.0) \right)
    $$

    where $\text{Grip}_{\text{delta}} = \text{Front\\_Grip}_{\text{avg}} - \text{Rear\\_Grip}_{\text{avg}}$
    *   **Fallback (v0.4.6+):** Rear grip now uses the same **Slip Angle approximation** fallback as Front grip if telemetry is missing, preventing false oversteer detection.

5.  **Rear Aligning Torque (v0.4.10 Workaround)**:
    Since LMU 1.2 reports 0.0 for rear `mLateralForce`, we calculate it manually.
    
    **Step 1: Approximate Rear Load**

    $$
    F_{z\text{\\_rear}} = \text{SuspForce} + 300.0
    $$

    (300N represents approximate unsprung mass).
    
    **Step 2: Calculate Lateral Force**

    $$
    F_{\text{lat\\_calc}} = \text{SlipAngle}_{\text{rear}} \times F_{z\text{\\_rear}} \times 15.0
    $$

    *   **Safety Clamp:** Clamped to +/- 6000.0 N.
    
    **Step 3: Calculate Torque**

    $$
    T_{\text{rear}} = F_{\text{lat\\_calc}} \times 0.001 \times K_{\text{rear\\_align}}
    $$
    
    **Note**: Coefficient changed from 0.00025 to 0.001 in v0.4.11.

$$
F_{\text{sop}} = F_{\text{sop\\_boosted}} + T_{\text{rear}} + F_{\text{yaw}}
$$

#### D. Dynamic Textures (Vibrations)

**1. Progressive Lockup ($F_{\text{vib\\_lock}}$)**
Active if Brake > 5% and Slip Ratio < -0.1.
*   **Manual Slip Trap (v0.4.6):** If using manual slip calculation and CarSpeed < 2.0 m/s, Slip Ratio is forced to 0.0.
*   **Frequency**: $10 + (|\text{Vel}_{\text{car}}| \times 1.5)$ Hz
*   **Amplitude**: $A = \text{Severity} \times K_{\text{lockup}} \times 4.0$
    
    **Note**: Amplitude scaling changed from 800.0 to 4.0 in v0.4.1 (Nm units).
*   **Force**: $A \times \sin(\text{phase})$

**2. Wheel Spin / Traction Loss ($F_{\text{vib\\_spin}}$ & $M_{\text{spin\\_drop}}$)**
Active if Throttle > 5% and Rear Slip Ratio > 0.2.
*   **Torque Drop Multiplier**: $M_{\text{spin\\_drop}} = 1.0 - (\text{Severity} \times K_{\text{spin}} \times 0.6)$
*   **Frequency**: $10 + (\text{SlipSpeed} \times 2.5)$ Hz (Capped at 80Hz)
*   **Amplitude**: $A = \text{Severity} \times K_{\text{spin}} \times 2.5$
    
    **Note**: Amplitude scaling changed from 500.0 to 2.5 in v0.4.1 (Nm units).
*   **Force**: $A \times \sin(\text{phase})$

**3. Slide Texture ($F_{\text{vib\\_slide}}$)**
Active if Lateral Patch Velocity > 0.5 m/s.
*   **Frequency**: $40 + (\text{LateralVel} \times 17.0)$ Hz
*   **Waveform**: Sawtooth
*   **Amplitude (Work-Based Scrubbing v0.4.38)**: 
    $$ A = K_{\text{slide}} \times 1.5 \times \text{Front\\_Load\\_Factor} \times (1.0 - \text{Grip}) $$
    *   Scales with **Load** (Force pressing tire to road).
    *   Scales with **Slip** ($1.0 - \text{Grip}$). Scrubbing requires sliding; a gripping tire rolls silent.
    *   **Note**: Amplitude scaling changed from 300.0 to 1.5 in v0.4.1 (Nm units).
*   **Force**: $A \times \text{Sawtooth}(\text{phase})$

**4. Road Texture ($F_{\text{vib\\_road}}$)**
Active if Road Texture Enabled.
*   **Oscillator Safety (v0.4.37):** All oscillator phases (Lockup, Spin, Slide, Bottoming) are now wrapped using `fmod(phase, 2PI)` to prevent phase explosion during large time steps (stutters).
High-pass filter on suspension movement.
*   **Delta Clamp (v0.4.6):** $\Delta_{\text{vert}}$ is clamped to +/- 0.01 meters per frame.
*   $\Delta_{\text{vert}} = (\text{Deflection}_{\text{current}} - \text{Deflection}_{\text{prev}})$
*   **Force**: $(\Delta_{\text{vert\\_L}} + \Delta_{\text{vert\\_R}}) \times 50.0 \times K_{\text{road}} \times \text{Front\\_Load\\_Factor}$
    
    **Note**: Amplitude scaling changed from 25.0 to 50.0 in v0.4.11.
*   **Scrub Drag (v0.4.5+):** Constant resistance force opposing lateral slide.
    *   **Force**: $F_{\text{drag}} = \text{DragDir} \times K_{\text{drag}} \times 5.0 \times \text{Fade}$
    *   **DragDir (v0.4.20 Fix):** If $Vel_{\text{lat}} > 0$ (Sliding Left), $DragDir = -1.0$ (Force Left/Negative). Opposes the slide to provide stabilizing torque.
    *   **Coordinate Note (v0.4.30):** Sliding Left (+Vel) -> requires Force Right (Negative Torque) for damping. But LMU +X is Left. Wait, if +X is Left, a Left Slide (+Vel) needs a Right Force (-Force). So DragDir = -1.0. Correct.
    *   **Note**: Multiplier changed from 2.0 to 5.0 in v0.4.11.
    *   **Fade In (v0.4.6):** Linearly scales from 0% to 100% between 0.0 and 0.5 m/s lateral velocity.

**5. Suspension Bottoming ($F_{\text{vib\\_bottom}}$)**
Active if Max Tire Load > 8000N or Ride Height < 2mm.
*   **Magnitude**: $\sqrt{\text{Load}_{\text{max}} - 8000} \times K_{\text{bottom}} \times 0.0025$
    
    **Note**: Magnitude scaling changed from 0.5 to 0.0025 in v0.4.1 (Nm units).
*   **Frequency**: Fixed 50Hz sine wave pulse.

**6. Synthetic Gyroscopic Damping ($F_{\text{gyro}}$) - New v0.4.17**
Stabilizes the wheel by opposing rapid steering movements (prevents "tank slappers").
*   $Angle$: Steering Input $\times$ (Range / 2.0).
*   $Vel$: $(Angle - Angle_{\text{prev}}) / dt$.
*   $Vel_{\text{smooth}}$: Smoothed derivative of steering angle (Time-Corrected LPF).
    * $\tau_{\text{gyro}} = K_{\text{smoothness\\_clamped}} \times 0.1$
    * $\alpha = dt / (\tau_{\text{gyro}} + dt)$
*   $F_{\text{gyro}} = -1.0 \times Vel_{\text{smooth}} \times K_{\text{gyro}} \times (\text{CarSpeed} / 10.0)$
*   **Note**: Scales with car speed (faster = more stability needed).

---

### 3. Post-Processing (Min Force)

After calculating the normalized force ($F_{\text{norm}}$), the Min Force logic is applied to overcome wheel friction:

If $|F_{\text{norm}}| > 0.0001$ AND $|F_{\text{norm}}| < K_{\text{min\\_force}}$:

$$
F_{\text{final}} = \text{sign}(F_{\text{norm}}) \times K_{\text{min\\_force}}
$$

---

### 4. Legend: Variables & Coefficients

**Telemetry Inputs (from LMU 1.2 Shared Memory - `TelemInfoV01`/`TelemWheelV01`):**
*   $T_{\text{steering\\_shaft}}$: `mSteeringShaftTorque` (Nm) - **Changed in v0.4.0 from** `mSteeringArmForce` (N)
*   $\text{Load}$: `mTireLoad` (N)
*   $\text{GripFract}$: `mGripFract` (0.0 to 1.0)
*   $\text{Chassis\\_Lat\\_Accel}$: `mLocalAccel.x` ($m/s^2$) - **Renamed from** `AccellXlocal`
*   $\text{LatForce}$: `mLateralForce` (N)
*   $\text{Vel}_{\text{car}}$: `mLocalVel.z` (m/s)
*   $\text{LateralGroundVel}$: `mLateralGroundVel` (m/s)
*   $\text{Deflection}$: `mVerticalTireDeflection` (m)

**User Settings (Coefficients from GUI):**
*   $K_{\text{gain}}$: Master Gain (0.0 - 2.0)
*   $K_{\text{shaft\\_gain}}$: Steering Shaft Gain (0.0 - 1.0) **(New v0.4.13)**
*   $K_{\text{understeer}}$: Understeer Effect (0.0 - 1.0)
*   $K_{\text{sop}}$: SoP Effect (0.0 - 2.0)
*   $K_{\text{yaw}}$: SoP Yaw Gain (0.0 - 2.0) **(New v0.4.15)**
*   $K_{\text{gyro}}$: Gyroscopic Damping Gain (0.0 - 1.0) **(New v0.4.17)**
*   $K_{\text{oversteer}}$: Oversteer Boost (0.0 - 1.0)
*   $K_{\text{rear\\_align}}$: Rear Align Torque (0.0 - 2.0)
*   $K_{\text{lockup}}, K_{\text{spin}}, K_{\text{slide}}, K_{\text{road}}, K_{\text{drag}}$: Texture/Effect Gains
*   $K_{\text{min\\_force}}$: Min Force (0.0 - 0.20)

**Hardcoded Constants (v0.4.1+):**
*   **20.0**: Reference Max Torque (Nm) for normalization (was 4000.0 N in old API)
*   **4000.0**: Reference Tire Load (N) for Load Factor (unchanged, loads still in Newtons)
*   **20.0**: SoP Scaling factor (was 5.0 in v0.4.x)
*   **25.0**: Road Texture stiffness (was 5000.0 before Nm conversion)
*   **8000.0**: Bottoming threshold (N, unchanged)
