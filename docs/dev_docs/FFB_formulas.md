# FFB Mathematical Formulas (v0.4.12+)

> **⚠️ API Source of Truth**  
> All telemetry data units and field names are defined in **`src/lmu_sm_interface/InternalsPlugin.hpp`**.  
> This is the official Studio 397 interface specification for LMU 1.2.  
> Critical: `mSteeringShaftTorque` is in **Newton-meters (Nm)**, not Newtons.

Based on the `FFBEngine.h` file, here is the complete mathematical breakdown of the Force Feedback calculation.

The final output sent to the DirectInput driver is a normalized value between **-1.0** and **1.0**.

### 1. The Master Formula

$$ F_{final} = \text{Clamp}\left( \left( \frac{F_{total}}{20.0} \times K_{gain} \right), -1.0, 1.0 \right) $$

**Note**: Reference changed from 4000 (N, old rF2 API) to 20.0 (Nm, LMU 1.2 API) in v0.4.0+.

Where **$F_{total}$** is the sum of all physics components:

$$ F_{total} = (F_{base} + F_{sop} + F_{vib\_lock} + F_{vib\_spin} + F_{vib\_slide} + F_{vib\_road} + F_{vib\_bottom}) \times M_{spin\_drop} $$

*(Note: $M_{spin\_drop}$ is a reduction multiplier active only during traction loss).*

---

### 2. Component Breakdown

#### A. Global Factors (Pre-calculated)
**Load Factor ($Front\_Load\_Factor$)**: Scales texture effects based on how much weight is on the front tires.
$$ Front\_Load\_Factor = \text{Clamp}\left( \frac{\text{Front\_Load}_{FL} + \text{Front\_Load}_{FR}}{2 \times 4000}, 0.0, 1.5 \right) $$

*   **Robustness Check:** If $\text{Front\_Load} \approx 0.0$ and $|Velocity| > 1.0 m/s$, $\text{Front\_Load}$ defaults to 4000N to prevent signal dropout.
*   **Safety Clamp (v0.4.6):** $Front\_Load\_Factor$ is hard-clamped to a maximum of **2.0** (regardless of configuration) to prevent unbounded forces during aero-spikes.

#### B. Base Force (Understeer / Grip Modulation)
This modulates the raw steering rack force from the game based on front tire grip.
$$ F_{base} = T_{steering\_shaft} \times \left( 1.0 - \left( (1.0 - \text{Front\_Grip}_{avg}) \times K_{understeer} \right) \right) $$
*   $\text{Front\_Grip}_{avg}$: Average of Front Left and Front Right `mGripFract`.
    *   **Fallback (v0.4.5+):** If telemetry grip is missing ($\approx 0.0$) but Load $> 100N$, grip is approximated from **Slip Angle**.
        * **Low Speed Trap (v0.4.6):** If CarSpeed < 5.0 m/s, Grip = 1.0.
        * **Slip Angle LPF (v0.4.6):** Slip Angle is smoothed using an Exponential Moving Average ($\alpha \approx 0.1$).
        * $\text{Slip} = \text{atan2}(V_{lat}, V_{long})$
        * **Refined Formula (v0.4.12):**
            * $\text{Excess} = \max(0, \text{Slip} - 0.10)$ (Threshold tightened from 0.15)
            * $\text{Grip} = \max(0.2, 1.0 - (\text{Excess} \times 4.0))$ (Multiplier increased from 2.0)
        * **Safety Clamp (v0.4.6):** Calculated Grip never drops below **0.2**.

#### C. Seat of Pants (SoP) & Oversteer
This injects lateral G-force and rear-axle aligning torque to simulate the car body's rotation.

1.  **Smoothed Lateral G ($G_{lat}$)**: Calculated via Low Pass Filter (Exponential Moving Average).
    *   **Input Clamp (v0.4.6):** Raw AccelX is clamped to +/- 5G ($49.05 m/s^2$) before processing.
    $$ G_{smooth} = G_{prev} + \alpha \times \left( \frac{\text{Chassis\_Lat\_Accel}}{9.81} - G_{prev} \right) $$
    *   $\alpha$: User setting `m_sop_smoothing_factor`.

2.  **Base SoP**:
    $$ F_{sop\_base} = G_{smooth} \times K_{sop} \times 20.0 $$
    
    **Note**: Scaling changed from 5.0 to 20.0 in v0.4.10 to provide stronger baseline Nm output.

3.  **Oversteer Boost**:
    If Front Grip > Rear Grip:
    $$ F_{sop\_boosted} = F_{sop\_base} \times \left( 1.0 + (\text{Grip}_{delta} \times K_{oversteer} \times 2.0) \right) $$
    where $\text{Grip}_{delta} = \text{Front\_Grip}_{avg} - \text{Rear\_Grip}_{avg}$
    *   **Fallback (v0.4.6+):** Rear grip now uses the same **Slip Angle approximation** fallback as Front grip if telemetry is missing, preventing false oversteer detection.

4.  **Rear Aligning Torque (v0.4.10 Workaround)**:
    Since LMU 1.2 reports 0.0 for rear `mLateralForce`, we calculate it manually.
    
    **Step 1: Approximate Rear Load**
    $$ F_{z\_rear} = \text{SuspForce} + 300.0 $$
    (300N represents approximate unsprung mass).
    
    **Step 2: Calculate Lateral Force**
    $$ F_{lat\_calc} = \text{SlipAngle}_{rear} \times F_{z\_rear} \times 15.0 $$
    *   **Safety Clamp:** Clamped to +/- 6000.0 N.
    
    **Step 3: Calculate Torque**
    $$ T_{rear} = F_{lat\_calc} \times 0.001 \times K_{rear\_align} $$
    
    **Note**: Coefficient changed from 0.00025 to 0.001 in v0.4.11.

$$ F_{sop} = F_{sop\_boosted} + T_{rear} $$

#### D. Dynamic Textures (Vibrations)

**1. Progressive Lockup ($F_{vib\_lock}$)**
Active if Brake > 5% and Slip Ratio < -0.1.
*   **Manual Slip Trap (v0.4.6):** If using manual slip calculation and CarSpeed < 2.0 m/s, Slip Ratio is forced to 0.0.
*   **Frequency**: $10 + (|\text{Vel}_{car}| \times 1.5)$ Hz
*   **Amplitude**: $A = \text{Severity} \times K_{lockup} \times 4.0$
    
    **Note**: Amplitude scaling changed from 800.0 to 4.0 in v0.4.1 (Nm units).
*   **Force**: $A \times \sin(\text{phase})$

**2. Wheel Spin / Traction Loss ($F_{vib\_spin}$ & $M_{spin\_drop}$)**
Active if Throttle > 5% and Rear Slip Ratio > 0.2.
*   **Torque Drop Multiplier**: $M_{spin\_drop} = 1.0 - (\text{Severity} \times K_{spin} \times 0.6)$
*   **Frequency**: $10 + (\text{SlipSpeed} \times 2.5)$ Hz (Capped at 80Hz)
*   **Amplitude**: $A = \text{Severity} \times K_{spin} \times 2.5$
    
    **Note**: Amplitude scaling changed from 500.0 to 2.5 in v0.4.1 (Nm units).
*   **Force**: $A \times \sin(\text{phase})$

**3. Slide Texture ($F_{vib\_slide}$)**
Active if Lateral Patch Velocity > 0.5 m/s.
*   **Frequency**: $40 + (\text{LateralVel} \times 17.0)$ Hz
*   **Waveform**: Sawtooth
*   **Amplitude**: $A = K_{slide} \times 1.5 \times Front\_Load\_Factor$
    
    **Note**: Amplitude scaling changed from 300.0 to 1.5 in v0.4.1 (Nm units).
*   **Force**: $A \times \text{Sawtooth}(\text{phase})$

**4. Road Texture ($F_{vib\_road}$)**
High-pass filter on suspension movement.
*   **Delta Clamp (v0.4.6):** $\Delta_{vert}$ is clamped to +/- 0.01 meters per frame.
*   $\Delta_{vert} = (\text{Deflection}_{current} - \text{Deflection}_{prev})$
*   **Force**: $(\Delta_{vert\_L} + \Delta_{vert\_R}) \times 50.0 \times K_{road} \times Front\_Load\_Factor$
    
    **Note**: Amplitude scaling changed from 25.0 to 50.0 in v0.4.11.
*   **Scrub Drag (v0.4.5+):** Constant resistance force opposing lateral slide.
    *   **Force**: $F_{drag} = \text{DragDir} \times K_{drag} \times 5.0 \times \text{Fade}$
    *   **Note**: Multiplier changed from 2.0 to 5.0 in v0.4.11.
    *   **Fade In (v0.4.6):** Linearly scales from 0% to 100% between 0.0 and 0.5 m/s lateral velocity.

**5. Suspension Bottoming ($F_{vib\_bottom}$)**
Active if Max Tire Load > 8000N or Ride Height < 2mm.
*   **Magnitude**: $\sqrt{\text{Load}_{max} - 8000} \times K_{bottom} \times 0.0025$
    
    **Note**: Magnitude scaling changed from 0.5 to 0.0025 in v0.4.1 (Nm units).
*   **Frequency**: Fixed 50Hz sine wave pulse.

---

### 3. Post-Processing (Min Force)

After calculating the normalized force ($F_{norm}$), the Min Force logic is applied to overcome wheel friction:

If $|F_{norm}| > 0.0001$ AND $|F_{norm}| < K_{min\_force}$:
$$ F_{final} = \text{sign}(F_{norm}) \times K_{min\_force} $$

---

### 4. Legend: Variables & Coefficients

**Telemetry Inputs (from LMU 1.2 Shared Memory - `TelemInfoV01`/`TelemWheelV01`):**
*   $T_{steering\_shaft}$: `mSteeringShaftTorque` (Nm) - **Changed in v0.4.0 from** `mSteeringArmForce` (N)
*   $\text{Load}$: `mTireLoad` (N)
*   $\text{GripFract}$: `mGripFract` (0.0 to 1.0)
*   $\text{Chassis\_Lat\_Accel}$: `mLocalAccel.x` ($m/s^2$) - **Renamed from** `AccellXlocal`
*   $\text{LatForce}$: `mLateralForce` (N)
*   $\text{Vel}_{car}$: `mLocalVel.z` (m/s)
*   $\text{LateralGroundVel}$: `mLateralGroundVel` (m/s)
*   $\text{Deflection}$: `mVerticalTireDeflection` (m)

**User Settings (Coefficients from GUI):**
*   $K_{gain}$: Master Gain (0.0 - 2.0)
*   $K_{understeer}$: Understeer Effect (0.0 - 1.0)
*   $K_{sop}$: SoP Effect (0.0 - 2.0)
*   $K_{oversteer}$: Oversteer Boost (0.0 - 1.0)
*   $K_{rear\_align}$: Rear Align Torque (0.0 - 2.0)
*   $K_{lockup}, K_{spin}, K_{slide}, K_{road}, K_{drag}$: Texture/Effect Gains
*   $K_{min\_force}$: Min Force (0.0 - 0.20)

**Hardcoded Constants (v0.4.1+):**
*   **20.0**: Reference Max Torque (Nm) for normalization (was 4000.0 N in old API)
*   **4000.0**: Reference Tire Load (N) for Load Factor (unchanged, loads still in Newtons)
*   **20.0**: SoP Scaling factor (was 5.0 in v0.4.x)
*   **25.0**: Road Texture stiffness (was 5000.0 before Nm conversion)
*   **8000.0**: Bottoming threshold (N, unchanged)
