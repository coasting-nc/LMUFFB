# Implementation Plan - Issue #213: Replace "Lateral G" effect with lateral tire load

**Issue:** #213 - Add lateral tire load effect (preserving "Lateral G").

## Context
Currently, the Seat-of-the-Pants (SoP) lateral effect is derived from the chassis' raw X-axis acceleration. While effective, this value varies significantly between car classes. Adding a normalized lateral tire load transfer effect provides a more consistent "feel" that is directly tied to the physical state of the suspension and tires, while allowing users to keep the existing G-force feel if preferred.

## Design Rationale (MANDATORY)
The addition of load-based SoP alongside acceleration-based SoP offers maximum flexibility:
1. **Consistency via Load**: Lateral load transfer `(L-R)/(L+R)` results in a normalized signal [0, 1] that scales with the car's physical cornering limit, regardless of aero.
2. **Hybrid Model**: Users can blend acceleration-based feel (raw Gs) with load-based feel (chassis lean) for a more comprehensive haptic experience.
3. **Legacy Preservation**: Keeping the existing "Lateral G" slider ensures no disruption for users happy with their current setup.
4. **Reliability**: Both effects use high-fidelity upsampled telemetry and robust fallbacks (Kinematic Load) for encrypted car data.

## Reference Documents
- `docs/dev_docs/references/Reference - coordinate_system_reference.md`
- `docs/dev_docs/reports/FFB Strength and Tire Load Normalization_proposal_1.md`

## Codebase Analysis Summary
- **src/FFBEngine.h**: Added `m_lat_load_effect` gain and `m_sop_load_smoothed` state.
- **src/FFBEngine.cpp**: Updated `calculate_sop_lateral` to combine both acceleration and load signals.
- **src/Config.h / src/Config.cpp**: Added `lateral_load` to `Preset` and handled INI persistence.
- **src/GuiLayer_Common.cpp**: Added a new "Lateral Load" slider below "Lateral G".
- **src/Tooltips.h**: Added description for the new effect.

## FFB Effect Impact Analysis
| Effect | Technical Changes | User Perspective |
|--------|-------------------|------------------|
| SoP Lateral | Combined signal: (Accel * Gain1 + Load * Gain2). | More nuanced and consistent steering weight. Ability to feel both inertia (G) and lean (Load). |
| Oversteer Boost | Modified by the combined SoP signal. | Heaviness during slides becomes more informative. |
| GUI / Tooltips | Added "Lateral Load" slider. | New tuning option for physical normalization. |

## Proposed Steps

1. **Add variables to FFBEngine**: Add `m_lat_load_effect` and `m_sop_load_smoothed` to `FFBEngine.h`.
2. **Update calculate_sop_lateral**: Modify `FFBEngine.cpp` to calculate both signals and sum them into `sop_base`.
   - Restore raw Lateral G calculation (`lat_g_accel`).
   - Implement Normalized Lateral Load Transfer calculation (`lat_load_norm`).
   - Apply smoothing to both using `m_sop_smoothing_factor`.
3. **Verify Physics Change**: Use `read_file` to confirm the additive logic in `src/FFBEngine.cpp`.
4. **Update Config system**: Update `Preset` struct and `Config` class to persist the new gain.
5. **Update GUI**: Add the new "Lateral Load" slider in `src/GuiLayer_Common.cpp`.
6. **Verify GUI Change**: Use `read_file` to confirm the new slider implementation.
7. **Update Tooltips**: Add `LATERAL_LOAD` in `src/Tooltips.h`.
8. **Orientation Matrix Helper**: Implement `VerifyOrientation` helper in `test_ffb_common` to standardize directional verification for haptic effects.
9. **Update Versioning**: Increment `VERSION` from `0.7.120` to `0.7.121`.
10. **Run Unit Tests**: Execute the new unit test and verify success.
11. **Full Regression Test**: Run `./build/tests/run_combined_tests` to ensure no other effects are broken.
12. **Final Quality Assurance**: Complete pre-commit steps to ensure proper testing, verification, review, and reflection are done.
13. **Submit**: Use the `submit` tool to push the changes.

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
