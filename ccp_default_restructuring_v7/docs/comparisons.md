# Comparisons with Other FFB Apps

LMUFFB draws inspiration from established tools in the sim racing community. Here is how it compares.

## vs iRFFB (iRacing)

**iRFFB** is the benchmark for external FFB apps.

*   **Telemetry Source**: iRFFB reads from the iRacing API. LMUFFB reads from rF2/LMU Shared Memory.
*   **Philosophy**:
    *   **iRFFB**: "Reconstruction" (smoothing 60Hz) or "360Hz" (raw physics).
    *   **LMUFFB**: Similar to "360Hz" mode. It relies on the rFactor 2 engine's already high-fidelity `SteeringArmForce` (updated at 400Hz). Its primary value add is **modulating** this force based on `GripFract` (which rF2 provides directly) rather than estimating grip loss from slip angles.
*   **Understeer/Oversteer**: iRFFB estimates understeer via slip angle vs. torque curves. LMUFFB uses the explicit "Grip Fraction" variable exposed by the rF2 engine.

## vs Marvin's AIRA (iRacing)

**Marvin's AIRA** is known for its tactile enhancement features.

*   **FFB Philosophy**:
    *   **Marvin's AIRA**: "Detail Augmentation". Splits effects (Road, Curb, Slip) and boosts specific frequencies.
    *   **LMUFFB**: Now implements similar **Texture** features. The C++ engine includes:
        *   **Slide Texture**: Synthetic noise injection during high slip angles.
        *   **Road Texture**: High-pass filtering of suspension deflection to boost road noise.
*   **Architecture**: Marvin's AIRA is a complex WPF/.NET application. LMUFFB is a native C++ application optimized for minimal latency.

## vs TinyPedal (rFactor 2 / LMU)

**TinyPedal** is a popular overlay tool for rF2/LMU.

*   **Similarities**: Both use the **rFactor 2 Shared Memory Map Plugin**.
*   **Differences**: TinyPedal is a passive **Overlay** (Read-Only -> Display). LMUFFB is an active **Control** app (Read-Write -> vJoy).

## Comparison Matrix

| Feature | iRFFB | Marvin's AIRA | LMUFFB (C++) |
| :--- | :--- | :--- | :--- |
| **Telemetry Rate** | 60Hz / 360Hz | 60Hz / 360Hz | **400Hz** |
| **Latency** | Low | Low | **Ultra-Low (Native C++)** |
| **Grip Loss Feel** | Calc. from Slip | Calc. from Slip | **Direct Grip Telemetry** |
| **SoP Effect** | Yes | Yes | **Yes (Lateral G)** |
| **Road Texture** | No (Pass-thru) | **Yes (Boosted)** | **Yes (Suspension Delta)** |
| **GUI** | Full GUI | Advanced GUI | **Planned (ImGui)** |
| **Logging** | Yes | Yes | No (Use DAMPlugin) |
