# Implementation Plan - Issue #393: Fix additional issues related to the FIR filter

**Issue:** #393 - Fix additional issues related to the FIR filter

## Context
This plan addresses two critical flaws in the up-sampling pipeline that contribute to "cogwheel" and irregular vibration feelings:
1.  **Polyphase Resampler Phase Misalignment (The "Stutter" Bug):** The resampler shifts its history buffer too early, using "future" data for the current fractional phase.
2.  **Holt-Winters Filter 100Hz Sawtooth (The "Buzz" Bug):** The filter returns raw input on frame boundaries, bypassing smoothing and creating a 100Hz sawtooth artifact.

## Design Rationale
### Overall Approach
The goal is to achieve mathematical continuity in the FFB signal. The up-sampling pipeline must provide a smooth transition between physics samples without introducing artificial jumps or aliasing artifacts.

### Polyphase Resampler (Stutter Bug)
**Why:** In the 5/2 ratio (400Hz to 1000Hz), a new physics sample arrives every 2.5 ticks on average. Shifting the history buffer immediately upon arrival of a new sample is incorrect because the filter still needs the "current" history to calculate the remaining fractional phases of the previous interval.
**Solution:** Decouple data arrival from buffer shifting. Store the new sample in a `m_pending_sample` and only shift it into the active `m_history` when the phase accumulator wraps around, indicating we are moving to a new input interval.

### Holt-Winters Filter (Buzz Bug)
**Why:** The current implementation returns `raw_input` on the frame where it arrives. Since LMU telemetry can be noisy, this creates a spike every 10ms (100Hz) followed by smoothed values, resulting in a sawtooth wave.
**Solution:** Return the smoothed `m_level` instead of `raw_input`. This ensures the output is always the result of the smoothing process, even at frame boundaries.

## Codebase Analysis Summary
### Current Architecture
- `PolyphaseResampler` (in `src/ffb/UpSampler.cpp`): Implements 400Hz to 1000Hz upsampling using a polyphase FIR filter.
- `HoltWintersFilter` (in `src/utils/MathUtils.h`): Implements double exponential smoothing for raw steering torque (100Hz to 400Hz).
- `FFBThread` (in `src/core/main.cpp`): Orchestrates the 1000Hz loop and calls the resampler.

### Impacted Functionalities
- **FFB Up-sampling:** Correcting the timing and continuity of the signal.
- **Steering Torque Smoothing:** Removing the 100Hz sawtooth artifact.

### Design Rationale
These modules are the core of the signal reconstruction pipeline. Any discontinuity here is directly felt by the user as vibration or "cogwheel" effect.

## FFB Effect Impact Analysis
| Affected FFB Effect | Technical Changes | Expected User-Facing Changes |
| :--- | :--- | :--- |
| All Effects | Smoother reconstruction via `PolyphaseResampler` | Reduction in "micro-stutter" and high-frequency "cogwheel" feel. |
| Primary Torque | Continuity fix in `HoltWintersFilter` | Elimination of 100Hz "buzz" or "vibration" in the wheel. |

### Design Rationale
Physics-based FFB relies on a clean torque signal. By ensuring mathematical continuity, we preserve the intended physics while removing artifacts introduced by the sampling rate mismatch.

## Implementation Steps

1.  **Create regression test for Polyphase Resampler Phase Misalignment.**
    - Create `tests/test_upsampler_issue_393.cpp` with a test case that feeds a step input and verifies that the output at the `is_new_physics_tick` frame does NOT yet reflect the new sample.
2.  **Create regression test for Holt-Winters 100Hz Sawtooth.**
    - Create `tests/test_math_utils_issue_393.cpp` that verifies `HoltWintersFilter` returns a smoothed value rather than raw input on frame boundaries.
3.  **Run tests and verify failures.**
    - Build and run the newly created tests to confirm they fail as expected.
4.  **Modify `src/ffb/UpSampler.h` to add pending sample state.**
    - Add `m_needs_shift` and `m_pending_sample` members.
5.  **Modify `src/ffb/UpSampler.cpp` to implement decoupled shifting.**
    - Update `Reset()` and `Process()` logic to use `m_pending_sample` and shift only on phase wrap.
6.  **Verify Polyphase Resampler fix.**
    - Run `run_combined_tests --filter=upsampler_issue_393` and verify it passes.
7.  **Modify `src/utils/MathUtils.h` to fix Holt-Winters sawtooth.**
    - Return `m_level` instead of `raw_input` in `HoltWintersFilter::Process()` when `is_new_frame` is true.
8.  **Verify Holt-Winters fix.**
    - Run `run_combined_tests --filter=math_utils_issue_393` and verify it passes.
9.  **Update existing tests.**
    - Adjust expected values in `tests/test_upsampler_issue_385.cpp` and `tests/test_upsampling.cpp` if necessary.
10. **Increment Version.**
    - Update `VERSION` and `src/core/Version.h.in` (if applicable) or where `LMUFFB_VERSION` is defined.
11. **Update `CHANGELOG_DEV.md`.**
    - Document the fixes for Issue #393.
12. **Perform Full Test Suite Run.**
    - Run all tests using `./build/tests/run_combined_tests` to ensure no regressions.
13. **Complete pre-commit steps.**
    - Complete pre-commit steps to ensure proper testing, verification, review, and reflection are done.

## Deliverables
- Modified `src/ffb/UpSampler.h`
- Modified `src/ffb/UpSampler.cpp`
- Modified `src/utils/MathUtils.h`
- New test file `tests/test_upsampler_issue_393.cpp`
- New test file `tests/test_math_utils_issue_393.cpp`
- Updated `VERSION`
- Updated `CHANGELOG_DEV.md`

## Implementation Notes
### Encountered Issues
- **Polyphase Resampler Shift Timing**: The original test cases in `test_upsampler_issue_385.cpp` had to be updated because the resampler now correctly delays the buffer shift until the phase wraps. This changes the output sequence for step inputs.
- **Holt-Winters Filter Settling**: Fixing the Holt-Winters filter to return smoothed values instead of raw input caused `test_long_load_multiplier_behavior` to fail. The test assumed an instantaneous jump in the torque signal, but the filter (with alpha=0.8) now correctly smooths this transition. The test was updated to allow the filter to settle over multiple frames.
- **Environment Failures**: `test_analyzer_bundling_integrity` fails in the current environment due to missing build artifacts in the expected `build/tools` path. This appears to be an environment configuration issue and not a regression.

### Deviations from the Plan
- Updated `tests/test_ffb_engine.cpp` to account for signal smoothing in `test_long_load_multiplier_behavior`.

### Suggestions for the Future
- Consider making the Holt-Winters alpha configurable per effect if some effects require faster response than the default 0.8.
