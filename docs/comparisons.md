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
| **FFB Output Method** | **DirectInput** (Direct Mode) | **DirectInput** | **DirectInput** (Constant Force) |
| **vJoy Usage** | Optional (for Input Upsampling) | No | Optional (for Input Mapping) |
| **Latency** | Low (API Overhead) | Low (.NET Overhead) | **Ultra-Low (Native C++ / Shared Mem)** |
| **Grip Loss Feel** | Calc. from Slip | Calc. from Slip | **Direct Grip Telemetry** |
| **SoP Effect** | Yes | Yes | **Yes (Lateral G)** |
| **Road Texture** | No (Pass-thru) | **Yes (Boosted)** | **Yes (Suspension Delta)** |
| **GUI** | Full GUI | Advanced GUI | **Yes (Dear ImGui)** |

## DirectInput Implementation Deep Dive

A key differentiator for FFB apps is how they talk to the hardware.

### 1. iRFFB (The Pioneer)
*   **Method**: Uses DirectInput `Constant Force` effects.
*   **Latency Management**: Uses a "Direct Mode" that bypasses vJoy to talk to the wheel. It reads telemetry via the iRacing SDK (memory mapped file, updated at 360Hz or 60Hz depending on mode).
*   **Signal Processing**: Heavily relies on **Interpolation/Extrapolation**. Since iRacing's disk telemetry is 60Hz, iRFFB's "Reconstruction" filter predicts the signal to smooth out steps. In 360Hz mode, it uses the raw physics tick.
*   **Challenge**: The iRacing SDK update timing can jitter, so iRFFB has complex logic to sync the FFB thread.

### 2. Marvin's AIRA (The Specialist)
*   **Method**: Uses DirectInput, but often layers multiple effects.
*   **Architecture**: Built on .NET/WPF. While powerful, the managed runtime (C#) introduces potential Garbage Collection (GC) pauses, though usually micro-optimized to be imperceptible.
*   **Effect Layering**: Marvin's strength is splitting the signal. It might send a base `Constant Force` for torque, but overlay `Periodic` (Sine/Triangle) effects for road texture or engine vibration. This "Composition" approach allows for richer detail but higher complexity in managing effect slots on the wheel driver.

### 3. LMUFFB (This Project)
*   **Method**: Native C++ DirectInput `Constant Force` effect.
*   **Update Rate**: **Native 400Hz**. The rFactor 2 engine (LMU) updates physics at 400Hz and writes to Shared Memory at the same rate. LMUFFB reads this directly (Zero Copy via `MapViewOfFile`) and updates the DI effect immediately.
*   **Advantage**: **No Interpolation Needed**. Unlike iRFFB (in 60Hz mode), LMUFFB gets the raw high-frequency signal. The latency is purely the time to read memory + compute float math + call `SetParameters`.
*   **Simplicity**: LMUFFB uses a single "Master" Constant Force effect. All internal effects (SoP, Texture, Grip) are mathematically mixed into this single signal *before* sending it to the driver. This ensures we never run out of "Effect Slots" on cheaper wheels (some Logitech wheels only support 4 concurrent effects).
