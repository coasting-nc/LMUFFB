# Implementation Plan - Issue #213: Replace "Lateral G" effect with lateral tire load

**Issue:** #213 - Replace "Lateral G" effect with lateral tire load.

## Context
Currently, the Seat-of-the-Pants (SoP) lateral effect is derived from the chassis' raw X-axis acceleration. While effective, this value varies significantly between car classes (e.g., Hypercars with high downforce vs. GT3s), leading to inconsistent FFB weight. Replacing this with normalized lateral tire load transfer provides a more consistent "feel" that is directly tied to the physical state of the suspension and tires.

## Design Rationale (MANDATORY)
The shift from acceleration-based to load-based SoP is driven by the desire for **physical normalization**.
1. **Consistency:** Acceleration (G-force) is an outcome of grip and speed. High-downforce cars pull much higher Gs, requiring separate gain tuning per car. Lateral load transfer is a more fundamental representation of the car's state. By normalizing it `(L-R)/(L+R)`, we get a signal that always scales with the car's physical limit, regardless of whether that limit is 1.5G or 4G.
2. **Realism:** Real steering wheels transmit torque based on the mechanical load on the tires. While SoP is a "fake" effect to compensate for the lack of G-forces in a sim, tying it to tire load makes it behave more like a real structural force.
3. **Safety & Stability:** Normalizing the signal prevents unexpected force spikes in high-downforce corners while maintaining detail in low-speed hairpins.
4. **Reliability:** Using `calculate_kinematic_load` as a fallback ensures that users don't lose this effect on cars with encrypted telemetry. The `ctx.frame_warn_load` flag (defined in `FFBCalculationContext` in `FFBEngine.h`) is used to detect per-frame telemetry availability.

## Reference Documents
- `docs/dev_docs/references/Reference - coordinate_system_reference.md`
- `docs/dev_docs/reports/FFB Strength and Tire Load Normalization_proposal_1.md` (Contextual background)

## Codebase Analysis Summary
- **src/FFBEngine.cpp**: contains `FFBEngine::calculate_sop_lateral` where the core logic resides.
- **src/FFBEngine.h**: Defines `FFBSnapshot`, `FFBCalculationContext`, and constants like `WEIGHT_TRANSFER_SCALE`.
- **src/GripLoadEstimation.cpp**: Implements `calculate_kinematic_load`.
- **src/GuiLayer_Common.cpp**: Implements the GUI sliders and labels.
- **src/Tooltips.h**: Contains the user-facing documentation for settings.

## FFB Effect Impact Analysis
| Effect | Technical Changes | User Perspective |
|--------|-------------------|------------------|
| SoP Lateral | Change input from `mLocalAccel.x` to normalized load transfer. | More consistent steering weight across different car types. "Weight" feels more progressive and tied to lean. |
| Oversteer Boost | Modified by the new SoP base signal. | The "heaviness" during a slide will feel more tied to the car's lean. |
| GUI / Tooltips | Rename "Lateral G" to "Lateral Load". | Better understanding of what the force represents. |

## Proposed Steps

1. **Create Test Baseline**: Create `tests/test_issue_213_lateral_load.cpp` to verify current vs new behavior. Ensure it fails initially if checking for load-based logic.
2. **Implement Physics Change**: Modify `FFBEngine::calculate_sop_lateral` in `src/FFBEngine.cpp` to use normalized lateral load transfer.
   - Use `data->mWheel[0].mTireLoad` and `data->mWheel[1].mTireLoad`.
   - Implement fallback to `calculate_kinematic_load` if `ctx.frame_warn_load` is true.
3. **Verify Physics Change**: Use `read_file` to confirm the new logic in `src/FFBEngine.cpp`.
4. **Update GUI Labels**: Modify `src/GuiLayer_Common.cpp` to rename "Lateral G" references to "Lateral Load".
5. **Verify GUI Change**: Use `read_file` to confirm label updates in `src/GuiLayer_Common.cpp`.
6. **Update Tooltips**: Modify `src/Tooltips.h` to update descriptions for `LATERAL_G` and `OVERSTEER_BOOST`.
7. **Verify Tooltips**: Use `read_file` to confirm tooltip updates.
8. **Update Versioning**: Increment `VERSION` from `0.7.120` to `0.7.121`.
9. **Run Unit Tests**: Execute the new unit test and verify success.
10. **Full Regression Test**: Run `./build/tests/run_combined_tests` to ensure no other effects are broken.
11. **Final Quality Assurance**: Complete pre-commit steps to ensure proper testing, verification, review, and reflection are done.
12. **Submit**: Use the `submit` tool to push the changes.

## Deliverables
- [x] Modified `src/FFBEngine.cpp`
- [x] Modified `src/GuiLayer_Common.cpp`
- [x] Modified `src/Tooltips.h`
- [x] New test file `tests/test_issue_213_lateral_load.cpp`
- [x] Updated `VERSION`
- [x] Implementation Notes added to this document.

## Implementation Notes
- **Unforeseen Issues**: Several existing tests relied on `mLocalAccel.x` being the sole driver of SoP force. Since they provided 0 for `mTireLoad`, these tests initially failed after the physics change.
- **Plan Deviations**: Updated `tests/test_ffb_core_physics.cpp`, `tests/test_ffb_features.cpp`, `tests/test_ffb_coordinates.cpp`, `tests/test_ffb_internal.cpp`, and `tests/test_ffb_slope_detection.cpp` to provide valid load data consistent with the simulated lateral G.
- **Challenges**: Ensuring the sign convention remained identical to prevent "reverse steering" was critical. Verified that +X (Centrifugal Left) corresponds to (FL > FR) and positive load transfer, which maintains the existing stabilizer pull.
- **Recommendations**: For future effects, consider explicitly separating "Physical Sources" from "Haptic Synthesis" early in the pipeline to make these kinds of migrations even cleaner.
