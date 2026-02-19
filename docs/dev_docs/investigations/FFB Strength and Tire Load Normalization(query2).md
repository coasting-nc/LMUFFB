# Deep Research Query: Dynamic FFB Normalization, Load Learning Windows, and Hardware Scaling

## Research Objective
Investigate and define the optimal logic for a **Session-Learned Dynamic Normalization** framework for Force Feedback (FFB). The goal is to validate and refine the "User Summary" recommendations—specifically the **2-15 m/s learning window** for static load and **User-Defined Torque Reference** scaling—while determining how to integrate the **Soft-Knee Compression** and **Signal Filtering** safety nets identified in the initial research.

## Context: The LMUFFB App

LMUFFB is a C++ application that intercepts telemetry from Le Mans Ultimate (similar to rFactor 2) and generates its own FFB signal using DirectInput.

- It uses the game's **Steering Shaft Torque** (in Nm) as a base.

- It calculates additional effects (Understeer, SoP, Road Texture, ABS/Lockup) using telemetry like **Tire Load** (in Newtons), **Slip Angles**, and **G-forces**.


## Technical Problem 1: Overall FFB Strength (Steering Weight)

Different car classes produce vastly different peak steering torque values:

- **GT3/GTE**: Peak torque at the rack is roughly **25-30 Nm**.

- **Hypercars/Prototypes**: Peak torque can reach **45-50 Nm** due to massive aerodynamic downforce.



**The Issue**: If a user sets their "Master Gain" or "Max Torque Reference" for a GT3 car, the FFB will be way too heavy or clip heavily when they switch to a Hypercar. Conversely, settings tuned for a Hypercar feel "limp" in a GT3.



**Current Implementation**: No automatic normalization. Users must manually change a `max_torque_ref` setting or `gain`.

**Proposed Implementation**: Use a class-aware lookup table of "Reference Peak Torques" (e.g., 25Nm for GT3, 45Nm for HC) to scale the base Nm signal to a common internal baseline (e.g., 30Nm) before final output scaling.



### Investigation Questions for Strength:

1. What is the standard industry approach for "Auto-Strength" in FFB utilities (e.g., irFFB, custom Simucube profiles)?

2. Should normalization be based on a **fixed class-based constant**, or a **session-learned peak** (monitoring the maximum Nm actually seen during a few laps)?

3. How do simulators like iRacing or Assetto Corsa handle the "Max Force" setting vs. the physical car's steering rack physics?

4. Is it better to normalize the input Nm to a common range, or to dynamically adjust the output gain?



## Technical Problem 2: Tire Load Normalization (Tactile Effects)

Effects like **Road Texture** and **Braking Haptics** are scaled by the current tire load to make them feel "heavier" and more detailed at high speeds/loads.

- **GT3 Static Load**: ~3500N per corner.

- **Hypercar High-Speed Load**: Can exceed **12000N** due to aero.



**The Issue**: If these effects are normalized by a "chasing peak" (the highest load seen in the session so far), the normalization factor eventually reaches 1.0 even at high speed, meaning the "extra detail" intended for high-load scenarios is neutralized.



**Current Implementation (WIP)**: Move from "Chasing Peak" to **Static Load Baseline**.

- Use the corner weight of the car at rest (or learned at low speeds) as the 1.0x baseline.

- At high speeds, the `Load / StaticLoad` ratio might be 2.5x or 3.0x, allowing road textures to naturally become stronger as aero load increases.



### Investigation Questions for Load:

1. What is the mathematically correct way to normalize tactile effects that depend on tire load? Should they be relative to **Static Weight**, **Suspension Travel**, or a **Global Peak**?

2. How do reputable FFB algorithms (like those in high-end direct drive software) handle the scaling of "Road Feel" as a function of speed and downforce?

3. Should there be a "soft-knee" or logarithmic compression applied to load-scaling to prevent haptic effects from becoming violent at 300km/h while remaining perceptible at 50km/h?

## Proposed Solutions to Analyze:

1. **Class-Aware Reference Mapping**: Using a lookup table (GT3=X Nm, HC=Y Nm) to normalize base forces.

2. **Dynamic Peak Chasing with Hysteresis**: Learning the session peak Nm and using it as the 1.0x reference, but with very slow decay.

3. **Static Load Anchoring**: Normalizing tactile effects against the static corner weight (learned at < 20 km/h).

4. **Pure Physics Passthrough**: Avoiding normalization and letting the user's "Max Torque" setting handle everything (with potential for frequent clipping).

## Focus Area 1: Dynamic Peak Normalization (Steering Weight)
The comparison of previous reports identified "Session-Learned Peak" as superior to fixed lookup tables. We need to define the algorithm's behavior to ensure stability.
*   **Algorithm Mechanics**: How should a "Fast-Attack, Slow-Decay" envelope follower be tuned for FFB?
    *   *Investigation*: What are optimal **decay rates** (e.g., linear 0.01 Nm/s vs. exponential) to ensure the wheel doesn't become "light" during long straights or after a single high-load corner?
*   **Outlier Rejection**: How to prevent "physics noise" (e.g., a 200Nm spike from hitting a wall) from permanently skewing the session peak and ruining the FFB for the rest of the race?
    *   *Investigation*: Look for "clipping protection logic" or "spike rejection filters" in telemetry apps.

## Focus Area 2: Refined Static Load Learning (Tactile Haptics)
The user suggested a specific speed window to calibrate the "1.0x" baseline for tire load effects.
*   **The 2-15 m/s Window**: Verify why this specific range is optimal.
    *   *Hypothesis*: Speeds < 2 m/s may have noisy/unstable telemetry (divide-by-zero errors or physics settling). Speeds > 15 m/s allow aerodynamic downforce to corrupt the "static" weight reading.
*   **Implementation**: How should the app "hold" this value? Should it be an average over time within this window, or a latching value?

## Focus Area 3: User-Defined Torque Reference (Output Scaling)
The comparison of previous reports favored scaling to the user's hardware limit (`m_max_torque_ref`) rather than an arbitrary internal constant.
*   **Mathematical Mapping**: Define the formula for mapping `Normalized_Game_Torque` to `Physical_Wheel_Torque`.
    *   *Context*: If a game outputs 0-100% signal, how does `m_max_torque_ref` (e.g., 20Nm) interact with the user's actual wheelbase driver settings?
*   **Linearity vs. Saturation**: When mapping to a user's physical limit, should the signal be strictly linear up to the limit, or is there a benefit to "Soft-Knee" compression *before* the hardware limit to preserve detail near the peak?

## Focus Area 4: Signal Conditioning & Synthesis
*   **Integration**: How do we combine **Dynamic Peaks** (User Report) with **EMA Filtering** (Generated Research)?
    *   *Problem*: Raw telemetry is "stepped" (60Hz/100Hz). Dynamic normalization factors calculated from stepped data can cause "gain stepping" (sudden jumps in force).
    *   *Investigation*: Where in the pipeline should the smoothing occur? On the *input telemetry* (Load/Torque) or on the *calculated normalization factor*?

## Search Keywords & Concepts
*   "FFB peak detection algorithm decay rate"
*   "iRacing auto force logic outlier rejection"
*   "Telemetry tire load static weight calibration speed threshold"
*   "Scaling FFB linearity to direct drive torque limits"
*   "Soft-knee signal compression for racing simulation force feedback"
*   "Exponential Moving Average (EMA) for telemetry smoothing"
*   "Hysteresis in dynamic gain control systems"
*   "FFB Auto Strength algorithms"
*   "Steering rack torque normalization in racing simulators"
*   "Telemetry-based FFB road texture scaling tire load"
*   "irFFB Auto-Strength implementation details"
*   "Wheelbase torque clipping vs car steering Nm"
*   "Tactile effect normalization static vs dynamic load"

---

## Expected Outcome (Ultimate Synthesis)
A final report that inclusdes as discussion (determining if it is the best solution of whether the research outcomes suggest that some parts could use better solutions) of a unified C++ logic flow for LMUFFB, combining:
1.  **Dynamic Strength**: A session-peak follower with specific decay/rejection logic.
2.  **Calibrated Haptics**: A static load learner gated by the 2-15 m/s window.
3.  **Hardware Scaling**: A user-centric output formula.
4.  **Safety Nets**: Soft-knee limits and EMA filters to ensure the dynamic adjustments feel smooth and organic.