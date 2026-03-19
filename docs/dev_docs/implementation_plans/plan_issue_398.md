# Implementation Plan - Issue #398: Fix HoltWintersFilter Snapback issue with mSteeringShaftTorque

**Issue:** #398
**Title:** Fix HoltWintersFilter Snapback issue with mSteeringShaftTorque

## Context
The `HoltWintersFilter` is used to upsample the 100Hz `mSteeringShaftTorque` to the FFB frequency (400Hz+). Currently, it uses extrapolation to achieve zero latency, but this can cause "snapbacks" and graininess when the torque changes rapidly. This plan implements a user-selectable mode between Zero Latency (Extrapolation) and Smooth (Interpolation).

## Design Rationale
- **Physics Reasoning:** Force feedback at 100Hz requires upsampling to avoid "staircase" artifacts. Extrapolation provides the most immediate response but is sensitive to noise and rapid changes, leading to overshoots (snapbacks). Interpolation (delaying by one 10ms frame) ensures a smooth transition between known data points, eliminating overshoots at the cost of a fixed 10ms delay.
- **Reliability:** Providing both options allows users to choose based on their hardware and preference. Some high-end direct-drive wheels might reveal the graininess of extrapolation, while others might prioritize the 10ms faster response.
- **Safety:** The implementation uses `std::clamp` and checks for initialization to prevent `NaN` or extreme values from entering the FFB loop.
- **Architectural Integrity:** The choice is integrated into the existing `Preset` and `Config` system, ensuring it is persistent and per-car/per-preset configurable.

## Reference Documents
- [GitHub Issue #398](../github_issues/issue_398.md)

## Codebase Analysis Summary
- **src/utils/MathUtils.h:** Contains the `HoltWintersFilter` class. It needs to be updated to support the two modes.
- **src/ffb/FFBEngine.h/cpp:** The engine that uses the filter. It needs to hold the setting and apply it to the filter instance.
- **src/core/Config.h/cpp:** Handles persistence. Needs to be updated to save/load the new setting.
- **src/gui/GuiLayer_Common.cpp:** The UI layer. Needs a new combo box to expose the setting.
- **src/gui/Tooltips.h:** Contains UI tooltips.

## FFB Effect Impact Analysis
| Effect | Technical Changes | User-Facing Change |
| :--- | :--- | :--- |
| **Primary Steering Force** | `mSteeringShaftTorque` upsampling mode change. | Reduced graininess/buzzing in "Smooth" mode; 10ms latency added. "Zero Latency" remains as is. |

### Design Rationale (FFB Impact)
The primary goal is to improve the "feel" of the legacy 100Hz torque source. By allowing interpolation, we eliminate the mathematical overshoot inherent in linear extrapolation of a noisy or rapidly changing signal.

## Steps

1. Update `src/utils/MathUtils.h` to add `m_prev_level` and `m_zero_latency` to `HoltWintersFilter`, and implement interpolation in `Process`.
   - Verify changes with `read_file`.
2. Update `src/ffb/FFBEngine.h` to add `m_steering_100hz_reconstruction`.
   - Verify changes with `read_file`.
3. Update `src/ffb/FFBEngine.cpp` to call `m_upsample_shaft_torque.SetZeroLatency()` in `calculate_force`.
   - Verify changes with `read_file`.
4. Update `src/core/Config.h` to add `steering_100hz_reconstruction` to the `Preset` struct and update `Apply`, `UpdateFromEngine`, and `Equals`.
   - Verify changes with `read_file`.
5. Update `src/core/Config.cpp` to handle `steering_100hz_reconstruction` in `ParsePhysicsLine`, `SyncPhysicsLine`, and `WritePresetFields`.
   - Verify changes with `read_file`.
6. Update `src/gui/GuiLayer_Common.cpp` to add the UI control for reconstruction mode.
   - Verify changes with `read_file`.
7. Update `src/gui/Tooltips.h` to add the `STEERING_100HZ_RECONSTRUCTION` tooltip.
   - Verify changes with `read_file`.
8. Add a new regression test `test_holt_winters_modes` in `tests/test_ffb_core.cpp` and update `FFBEngineTestAccess` in `tests/test_ffb_common.h`.
   - Verify changes with `read_file`.
9. Build the project and run the new test to ensure it passes.
   - Use `cmake --build build` and `./build/tests/run_combined_tests --filter=test_holt_winters_modes`.
10. Update `VERSION` and `CHANGELOG_DEV.md`.
    - Verify changes with `read_file`.
11. Execute the comprehensive test suite using `./build/tests/run_combined_tests` to ensure system-wide stability.
12. Request a code review and address any feedback.
    - Save reviews as `review_iteration_X.md` in `docs/dev_docs/code_reviews`.
13. Complete pre commit steps to ensure proper testing, verification, review, and reflection are done.
14. Submit the change.

## Deliverables
- [ ] Modified `src/utils/MathUtils.h`
- [ ] Modified `src/ffb/FFBEngine.h`
- [ ] Modified `src/ffb/FFBEngine.cpp`
- [ ] Modified `src/core/Config.h`
- [ ] Modified `src/core/Config.cpp`
- [ ] Modified `src/gui/GuiLayer_Common.cpp`
- [ ] Modified `src/gui/Tooltips.h`
- [ ] Updated `tests/test_ffb_core.cpp` (or similar) with new test.
- [ ] Updated `VERSION` and `CHANGELOG_DEV.md`.
- [ ] Implementation Notes in this plan.

## Implementation Notes

### Implementation Summary
The fix for Issue #398 was implemented by upgrading the `HoltWintersFilter` to support two reconstruction modes: **Zero Latency (Extrapolation)** and **Smooth (Interpolation)**.
- **Zero Latency** (Default) uses the signal trend to predict the future state, maintaining the existing low-latency feel but being susceptible to overshoots (snapbacks) and graininess.
- **Smooth** mode delays the signal by exactly one 100Hz frame (10ms) and performs linear interpolation between the previous and current target values, guaranteeing C0 continuity and elimination of extrapolation-related artifacts.

The choice is exposed in the GUI tuning window (conditionally visible when using the 100Hz source) and is fully integrated into the `Preset` and `Config` persistence layers.

### Unforeseen Issues
- No significant unforeseen issues were encountered. The mathematical implementation of interpolation within the existing Holt-Winters framework was straightforward once the 1-frame delay was introduced.

### Plan Deviations
- **Test Timing**: In the regression test `test_holt_winters_modes`, it was discovered that mode 0 (Extrapolation) requires at least two 100Hz frames to build a non-zero trend before it can demonstrate an overshoot on an intermediate 400Hz frame. The test was adjusted to ensure the trend is properly established.
- **Structural Multiplier**: To ensure predictable force outputs in tests, the `smoothed_structural_mult` was manually set via `FFBEngineTestAccess` to match the test's target Nm scaling, avoiding dependency on the auto-learning ramp.

### Challenges Encountered
- Simulating the exact 100Hz/400Hz cadence in a unit test requires careful handling of `mElapsedTime` and the `is_new_frame` flag. The `calculate_force` signature with `override_dt` proved essential for verifying the intermediate upsampled output.

### Recommendations for Future Plans
- When testing upsamplers or extrapolators, always explicitly document the required "warmup" sequence (number of frames and timing intervals) needed to reach a state where the algorithm's specific properties (like trend prediction) can be observed.

### Code Review Reflection
- The code review for Iteration 1 provided a "Greenlight" with no requested changes. The reviewer highlighted the safety and backward compatibility of the approach, as well as the attention to detail in variable renaming to avoid collision with new member variables.
