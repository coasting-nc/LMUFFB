# Comparisons with Other FFB Apps

LMUFFB draws inspiration from established tools in the sim racing community. Here is how it compares.

## vs iRFFB (iRacing)

**iRFFB** is the benchmark for external FFB apps.

*   **Telemetry Source**: iRFFB reads from the iRacing API. iRacing's telemetry is output at 60Hz (disk) or 360Hz (memory). iRFFB effectively upsamples or uses the high-rate buffer. LMUFFB reads from rF2/LMU Shared Memory, which typically runs at physics rate (400Hz).
*   **Philosophy**:
    *   **iRFFB**: Often uses "Reconstruction" (interpolation) to smooth the 60Hz signal or "360Hz" mode to use raw physics. It calculates aligning torque using a rack force model and mixes in SoP.
    *   **LMUFFB**: Relies on the rFactor 2 engine's already high-fidelity `SteeringArmForce`. Its primary value add is **modulating** this force based on `GripFract` (which rF2 provides directly) rather than estimating grip loss from slip angles.
*   **Understeer/Oversteer**: iRFFB estimates understeer via slip angle vs. torque curves. LMUFFB uses the explicit "Grip Fraction" variable exposed by the rF2 engine.

## vs Marvin's AIRA (iRacing)

**Marvin's AIRA** (and its successor MAIRA Refactored) is another advanced FFB tool for iRacing.

*   **FFB Philosophy**:
    *   **Marvin's AIRA**: Uses a philosophy of **Detail Augmentation** and specific effect injection. It separates effects into distinct components (Road Texture, Curb effects, Slip effects) and allows granular control over each. It focuses heavily on "Tactile" feedback logic and correcting iRacing's specific lack of certain tire feel properties. It often employs a "Detail Booster" to amplify small signal variations that get lost in the main torque signal.
    *   **LMUFFB**: Currently focuses on a macro-level "Grip Modulation" approach. It does not yet perform signal analysis (FFT or high-pass filtering) to boost road details like Marvin's does.
*   **Architecture**: Marvin's AIRA is a complex WPF/.NET application with a plugin-based architecture for effects ("Components"). LMUFFB is a lightweight Python script focused on a single logical pipeline.

## vs TinyPedal (rFactor 2 / LMU)

**TinyPedal** is a popular overlay tool for rF2/LMU.

*   **Similarities**: Both use the **rFactor 2 Shared Memory Map Plugin** and Python `mmap` to read data. The code for reading the memory buffer is functionally identical.
*   **Differences**: TinyPedal is a passive **Overlay** (Read-Only -> Display). LMUFFB is an active **Control** app (Read-Write -> vJoy).

## Missing Features (vs iRFFB / Marvin's AIRA)

LMUFFB is currently a prototype/MVP. It lacks several advanced features present in the mature iRacing apps:

1.  **Detail/Road Texture Boosting**: Marvin's AIRA excels at extracting and amplifying "road noise" (bumps, curbs). LMUFFB currently passes through the game's force or modulates it, but does not isolate high-frequency details.
2.  **Suspension/Bump Stop Effects**: iRFFB allows adding "Bump Stop" forces. LMUFFB does not calculate suspension travel limits yet.
3.  **Low-Latency Direct Mode**: iRFFB has a "Direct" mode that bypasses vJoy for some wheels to reduce latency. LMUFFB relies entirely on vJoy.
4.  **GUI**: Both reference apps have rich GUIs for tweaking curves, gain, and smoothing in real-time. LMUFFB uses hardcoded constants or simple variables.
5.  **Telemetry Logging**: iRFFB can log telemetry for analysis (Motec). LMUFFB does not logging (though DAMPlugin handles this separately for rF2).
