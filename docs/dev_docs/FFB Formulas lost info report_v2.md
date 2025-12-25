Based on a detailed comparison between the **Old Version (v0.4.12+)** and the **New Version (v0.6.2)**, the document has largely evolved to reflect updated physics logic (particularly in braking and decoupling).

However, **several specific details, formulas, and implementation nuances have been lost or omitted** in the transition to the v0.6.2 document.

### 🔴 Critical Omissions (Potential Feature Removal or Documentation Gaps)

**1. Oversteer Boost Logic ($K_{oversteer}$)**
*   **Old Version:** Explicitly detailed a "Lateral G Boost" feature where the SoP force was boosted if Front Grip > Rear Grip.
    *   *Formula:* $F_{sop\_boosted} = F_{sop\_base} \times (1.0 + (\text{Grip}_{\text{delta}} \times K_{\text{oversteer}} \times 2.0))$
*   **New Version:** This entire section is **missing**. The SoP section (C) lists Lateral G, Yaw, and Rear Aligning Torque, but the Oversteer Boost logic and the $K_{oversteer}$ coefficient are completely absent.

**2. Telemetry Variable Mapping**
*   **Old Version:** Included a "Legend: Variables & Coefficients" section that explicitly mapped internal math symbols to LMU API variables (e.g., $\text{Vel}_{\text{car}}$ = `mLocalVel.z`, $\text{Chassis\_Lat\_Accel}$ = `mLocalAccel.x`).
*   **New Version:** This mapping is gone. It only lists "Physics Constants." The link between the math and the specific API field names (e.g., `mLocalVel.z`) is lost.

**3. Explicit Filter Math (Time-Corrected LPF)**
*   **Old Version:** Provided the exact mathematical implementation for the Time-Corrected Low Pass Filter:
    *   $\alpha = dt / (\tau + dt)$
*   **New Version:** References "Time-Corrected LPF" multiple times but no longer documents the underlying math/alpha calculation used to achieve it.

---

### 🟡 Nuance & Detail Reductions

**1. Load Factor Calculation & Robustness**
*   **Old Version:** Explicitly stated that the Front Load Factor was the **average** of Front Left and Front Right: `(FL + FR) / 2`. It also detailed a "Robustness Check" where Load defaulted to 4000N if telemetry reported ~0 while moving.
*   **New Version:** Simplifies this to just `Load / 4000.0`. The averaging logic and the specific "Zero Load" fallback protection are no longer documented.

**2. Gyroscopic Derivation**
*   **Old Version:** Explained how Steering Velocity was derived from Steering Angle: $Angle = \text{Input} \times (\text{Range} / 2.0)$.
*   **New Version:** Simply lists `SteerVel_smooth` as an input without explaining the derivation from the steering angle range.

**3. Slide Frequency Constants**
*   **Old Version:** Frequency = $40 + (\text{Vel} \times 17.0)$ Hz.
*   **New Version:** Frequency = $10 + (\text{Vel} \times 5.0)$ Hz.
*   *Note:* This is likely a physics update rather than a documentation error, but the baseline frequency has dropped significantly (40Hz to 10Hz).

---

### 🟢 Logic Changes (Evolution vs. Loss)

*   **Braking/Lockup:** The old simple sine wave formula ($10 + Vel \times 1.5$) is completely gone, replaced by a much more complex "Predictive Logic" system in v0.6.2. This is an update, not a loss.
*   **Master Formula:** The old version hardcoded a division by `20.0`. The new version divides by `m_max_torque_ref` and handles the `20.0` scaling in a separate "Signal Scalers" section. The logic is preserved but refactored.

### Summary of Action Required
If you are maintaining the code based on these docs, you should verify:
1.  Is **Oversteer Boost** ($K_{oversteer}$) still in the codebase? If so, it is missing from the v0.6.2 documentation.
2.  Do you still need the **Zero-Load Fallback** (Robustness Check) defined in v0.4.12? It is not present in the v0.6.2 specs.