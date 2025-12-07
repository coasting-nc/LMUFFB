Based on the `FFBEngine.h` file provided in the source code, here is the complete mathematical breakdown of the Force Feedback calculation.

The final output sent to the DirectInput driver is a normalized value between **-1.0** and **1.0**.

### 1. The Master Formula

$$ F_{final} = \text{Clamp}\left( \left( \frac{F_{total}}{4000} \times K_{gain} \right), -1.0, 1.0 \right) $$

Where **$F_{total}$** is the sum of all physics components:

$$ F_{total} = (F_{base} + F_{sop} + F_{vib\_lock} + F_{vib\_spin} + F_{vib\_slide} + F_{vib\_road} + F_{vib\_bottom}) \times M_{spin\_drop} $$

*(Note: $M_{spin\_drop}$ is a reduction multiplier active only during traction loss).*

---

### 2. Component Breakdown

#### A. Global Factors (Pre-calculated)
**Load Factor ($L_{factor}$)**: Scales texture effects based on how much weight is on the front tires.
$$ L_{factor} = \text{Clamp}\left( \frac{\text{Load}_{FL} + \text{Load}_{FR}}{2 \times 4000}, 0.0, 1.5 \right) $$

#### B. Base Force (Understeer / Grip Modulation)
This modulates the raw steering rack force from the game based on front tire grip.
$$ F_{base} = F_{steering\_arm} \times \left( 1.0 - \left( (1.0 - \text{Grip}_{avg}) \times K_{understeer} \right) \right) $$
*   $\text{Grip}_{avg}$: Average of Front Left and Front Right `mGripFract`.

#### C. Seat of Pants (SoP) & Oversteer
This injects lateral G-force and rear-axle aligning torque to simulate the car body's rotation.

1.  **Smoothed Lateral G ($G_{lat}$)**: Calculated via Low Pass Filter (Exponential Moving Average).
    $$ G_{smooth} = G_{prev} + \alpha \times \left( \frac{\text{AccelX}_{local}}{9.81} - G_{prev} \right) $$
    *   $\alpha$: User setting `m_sop_smoothing_factor`.

2.  **Base SoP**:
    $$ F_{sop\_base} = G_{smooth} \times K_{sop} \times 1000.0 $$

3.  **Oversteer Boost**:
    If Front Grip > Rear Grip:
    $$ F_{sop\_boosted} = F_{sop\_base} \times \left( 1.0 + (\text{Grip}_{delta} \times K_{oversteer} \times 2.0) \right) $$

4.  **Rear Aligning Torque**:
    $$ T_{rear} = \frac{\text{LatForce}_{RL} + \text{LatForce}_{RR}}{2} \times 0.05 \times K_{oversteer} $$

$$ F_{sop} = F_{sop\_boosted} + T_{rear} $$

#### D. Dynamic Textures (Vibrations)

**1. Progressive Lockup ($F_{vib\_lock}$)**
Active if Brake > 5% and Slip Ratio < -0.1.
*   **Frequency**: $10 + (|\text{Vel}_{car}| \times 1.5)$ Hz
*   **Amplitude**: $A = \text{Severity} \times K_{lockup} \times 800.0 \times L_{factor}$
*   **Force**: $A \times \sin(\text{phase})$

**2. Wheel Spin / Traction Loss ($F_{vib\_spin}$ & $M_{spin\_drop}$)**
Active if Throttle > 5% and Rear Slip Ratio > 0.2.
*   **Torque Drop Multiplier**: $M_{spin\_drop} = 1.0 - (\text{Severity} \times K_{spin} \times 0.6)$
*   **Frequency**: $10 + (\text{SlipSpeed} \times 2.5)$ Hz (Capped at 80Hz)
*   **Amplitude**: $A = \text{Severity} \times K_{spin} \times 500.0$
*   **Force**: $A \times \sin(\text{phase})$

**3. Slide Texture ($F_{vib\_slide}$)**
Active if Slip Angle > 0.15 rad (~8.5°).
*   **Frequency**: $30 + (\text{LateralGroundVel} \times 20.0)$ Hz
*   **Waveform**: Sawtooth
*   **Amplitude**: $A = K_{slide} \times 300.0 \times L_{factor}$
*   **Force**: $A \times \text{Sawtooth}(\text{phase})$

**4. Road Texture ($F_{vib\_road}$)**
High-pass filter on suspension movement.
*   $\Delta_{vert} = (\text{Deflection}_{current} - \text{Deflection}_{prev})$
*   **Force**: $(\Delta_{vert\_L} + \Delta_{vert\_R}) \times 5000.0 \times K_{road} \times L_{factor}$

**5. Suspension Bottoming ($F_{vib\_bottom}$)**
Active if Max Tire Load > 8000N.
*   **Magnitude**: $\sqrt{\text{Load}_{max} - 8000} \times K_{bottom} \times 0.5$
*   **Frequency**: Fixed 50Hz sine wave pulse.

---

### 3. Post-Processing (Min Force)

After calculating the normalized force ($F_{norm}$), the Min Force logic is applied to overcome wheel friction:

If $|F_{norm}| > 0.0001$ AND $|F_{norm}| < K_{min\_force}$:
$$ F_{final} = \text{sign}(F_{norm}) \times K_{min\_force} $$

---

### 4. Legend: Variables & Coefficients

**Telemetry Inputs (from Shared Memory):**
*   $F_{steering\_arm}$: `mSteeringArmForce`
*   $\text{Load}$: `mTireLoad` (N)
*   $\text{GripFract}$: `mGripFract` (0.0 to 1.0)
*   $\text{AccelX}_{local}$: `mLocalAccel.x` ($m/s^2$)
*   $\text{LatForce}$: `mLateralForce` (N)
*   $\text{Vel}_{car}$: `mLocalVel.z` (m/s)
*   $\text{LateralGroundVel}$: `mLateralGroundVel` (m/s)
*   $\text{Deflection}$: `mVerticalTireDeflection` (m)

**User Settings (Coefficients from GUI):**
*   $K_{gain}$: Master Gain (0.0 - 2.0)
*   $K_{understeer}$: Understeer Effect (0.0 - 1.0)
*   $K_{sop}$: SoP Effect (0.0 - 2.0)
*   $K_{oversteer}$: Oversteer Boost (0.0 - 1.0)
*   $K_{lockup}, K_{spin}, K_{slide}, K_{road}$: Texture Gains
*   $K_{min\_force}$: Min Force (0.0 - 0.20)

**Hardcoded Constants (Tuning):**
*   **4000.0**: Reference Max Force (Newtons) for normalization.
*   **4000.0**: Reference Tire Load (Newtons) for Load Factor.
*   **1000.0**: SoP Scaling factor.
*   **5000.0**: Road Texture stiffness.
*   **8000.0**: Bottoming threshold (Newtons).