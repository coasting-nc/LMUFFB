# Lateral Load Normalization Analysis & Challenges (Issue #213)

## Overview
This report discusses the technical challenges and architectural recommendations arising from the implementation of the **Lateral Load** Seat-of-the-Pants (SoP) effect in v0.7.121. The primary goal of this task was to transition from (or supplement) the acceleration-based "Lateral G" effect with a physically-normalized tire load transfer model.

## Challenges: Sign Convention and Directionality
One of the most critical aspects of implementing lateral FFB effects is ensuring correct directionality across multiple coordinate systems (Game Physics -> Internal App Physics -> DirectInput API).

### The "Reverse Steering" Risk
In v0.4.19, the project underwent a major coordinate system fix. Reintroducing a new lateral source (Tire Load) posed a risk of reverting these fixes if the sign convention was not meticulously verified.

**Verification Logic:**
1.  **Game Physics**: In a **Right Turn**, centrifugal force pushes the car's body to the **LEFT**. The game reports this as a **POSITIVE** `mLocalAccel.x`.
2.  **Tire Load**: In this same Right Turn, vertical load transfers to the **LEFT** tires (Outside). Thus, Front-Left (FL) Load > Front-Right (FR) Load.
3.  **Normalization**: `(FL_Load - FR_Load) / Total_Load` results in a **POSITIVE** value.
4.  **FFB Output**: The project's global inversion logic expects a POSITIVE internal lateral signal to produce a **NEGATIVE** (Left-pulling) force at the wheel to simulate steering weight.

**Result**: The implementation successfully verified that POSITIVE load transfer maps directly to the existing POSITIVE `lat_g` sign convention, maintaining the correct stabilizer pull without requiring additional per-effect inversions.

## Challenges: Telemetry Availability
A significant portion of LMU's vehicle roster (specifically newer DLC) has "encrypted" telemetry where `mTireLoad` returns 0.0. To prevent the new Lateral Load effect from disappearing on these cars, we leveraged the existing `calculate_kinematic_load` fallback. This ensures the user experience remains consistent regardless of the car selected.

## Recommendations for Future Effects

### 1. Separation of Source and Synthesis
Current effects often mix "where the data comes from" with "how the force is calculated" inside the same method. For future effects (e.g., longitudinal weight transfer or advanced aero-loading), we recommend:
-   **Source Decoupling**: Extracting the "Normalized Physical Source" (e.g., `GetNormalizedLateralLoad()`) into a standalone helper.
-   **Pure Synthesis**: Having the effect handlers take these normalized [0, 1] signals as inputs, making them agnostic to whether the source is raw G-force, tire load, or a kinematic estimate.

### 2. Standardized Orientation Checks
As the codebase grows, adding a standardized "Orientation Matrix" test helper for all lateral/longitudinal effects would prevent regression of coordinate fixes. This helper should simulate "G-Positive/G-Negative" and "Load-Positive/Load-Negative" scenarios and verify the final `FFBSnapshot` output sign.

### 3. Haptic Frequency Budgeting
With the introduction of more additive effects (Lateral G + Lateral Load), there is a risk of "muddying" the haptic signal. Future work should consider a "Haptic Mixer" approach where different frequencies or components are prioritized or phased to prevent interference between overlapping cues.
