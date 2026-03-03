# Implementation Plan: Up-sample FFB Part 1 (100Hz to 400Hz)

## Issue Reference
This plan addresses Issue #216: "Up-sample FFB Part 1 - Up-sample LMU telemetry channels from 100Hz to 400Hz".

## Context
The goal of this task is to up-sample the 100Hz telemetry data provided by Le Mans Ultimate (LMU) to match the 400Hz internal physics loop of `lmuFFB`. This is Part 1 of a two-stage upsampling initiative, focusing strictly on **Input Conditioning**. 

### Design Rationale
Currently, the 100Hz telemetry signal appears as a "staircase" to the 400Hz FFB loop (one frame of change followed by three frames of identical data). This causes severe mathematical issues for algorithms relying on derivatives ($d/dt$), specifically the **Slope Detection** algorithm, which oscillates wildly between infinite and zero slope. By up-sampling the inputs, we convert these staircases into continuous ramps or smooth curves, restoring mathematical continuity, eliminating derivative spikes, and significantly improving the tactile smoothness of the Force Feedback.

## Reference Documents
*   [Upsampling Research Report](docs/dev_docs/reports/upsampling.md)

---

## Codebase Analysis Summary

### Current Architecture Overview
*   **`main.cpp`**: Runs a high-priority thread at 400Hz (`target_period(2500)` microseconds). It polls `GameConnector` and passes the `TelemInfoV01` struct to `FFBEngine::calculate_force`.
*   **`FFBEngine.cpp`**: Consumes the telemetry data. Currently, it reads `data->mWheel[i]` directly. Because the game updates at 100Hz, 3 out of 4 calls to `calculate_force` receive identical telemetry values.
*   **`MathUtils.h`**: Contains DSP utilities like `BiquadNotch` and `apply_adaptive_smoothing`.

### Impacted Functionalities
1.  **`FFBEngine::calculate_force`**: Needs to intercept raw telemetry, detect when a *new* 100Hz frame has arrived, apply upsampling filters, and use the upsampled values for all subsequent physics calculations.
2.  **`MathUtils.h`**: Needs new DSP classes for upsampling (`LinearExtrapolator` and `HoltWintersFilter`).

### Design Rationale
Intercepting the data at the very beginning of `calculate_force` and creating a persistent working copy of the `TelemInfoV01` struct ensures that all downstream functions (like `calculate_grip`, `calculate_road_texture`) automatically benefit from the high-resolution data without needing to be individually rewritten. Using a persistent member (`m_working_info`) instead of stack allocation avoids large stack churn (2KB+) every 2.5ms.

---

## FFB Effect Impact Analysis

| Effect | Technical Changes | User-Facing Changes |
| :--- | :--- | :--- |
| **Slope Detection** | Inputs (`mLateralPatchVel`, `mLongitudinalPatchVel`) upsampled using Linear Extrapolation. | **Massive stability improvement.** Eliminates "Singularities" and "Binary Residence" issues. Grip loss detection will be much more accurate and less prone to false positives. |
| **Road Texture** | Input (`mVerticalTireDeflection`) upsampled using Linear Extrapolation. | **Removes 100Hz "buzz".** The texture will feel like actual road surface rather than a digital, robotic stepping artifact. |
| **Base Steering Torque** | Input (`mSteeringShaftTorque`) upsampled using Holt-Winters. | **Smoother, organic feel.** The wheel will load up in corners with a fluid, rubber-like resistance rather than a grainy, stepped resistance. |
| **Haptic Oscillators** | Inputs (`Throttle`, `Brake`, `Accel`) upsampled using Linear Extrapolation. | **Consistent ABS and Spin feel.** Eliminates artifacts in high-frequency haptic effects caused by stepped telemetry inputs. |

### Design Rationale
*   **Linear Extrapolation** is chosen for derivative-critical channels because it guarantees a constant, non-zero derivative across the 400Hz sub-frames, which is a mathematical necessity for Slope Detection and vibration oscillators.
*   **Holt-Winters (Method B)** is chosen for Steering Shaft Torque because it acts as both an upsampler and a low-pass filter, rounding off the sharp edges of the 100Hz signal to create a more organic, less robotic feel while maintaining phase accuracy.

---

## Proposed Changes

### 1. Add Upsampling DSP Classes to `src/MathUtils.h`
Add two new classes to the `ffb_math` namespace:

*   **`LinearExtrapolator`**:
    *   *Logic:* Calculates the rate of change over the last game tick. Inter-frame interpolation (dead reckoning) projects the value forward between game frames. Clamped to 1.5x game tick to prevent runaway if the game pauses.
*   **`HoltWintersFilter`**:
    *   *Logic:* Second-order double exponential smoothing. Tracks both level and trend. Returns authoritative raw input on frame boundaries to minimize perceived lag while maintaining a smooth internal trend for upsampling.

### 2. Update `src/FFBEngine.h`
Add state variables to track the filters and frame timing:
```cpp
TelemInfoV01 m_working_info;
double m_last_telemetry_time = -1.0;
ffb_math::LinearExtrapolator m_upsample_steering;
ffb_math::LinearExtrapolator m_upsample_lat_patch_vel[4];
// ... other upsamplers
ffb_math::HoltWintersFilter  m_upsample_shaft_torque;
```

### 3. Modify `src/FFBEngine.cpp` (`calculate_force`)
Implement frame detection and upsampling logic. Crucially, use `upsampled_data` for all state updates (`m_prev_*`) to ensure derivatives are calculated over the correct 2.5ms interval.

### 4. Synchronize Loop Timing in `main.cpp`
Force the physics `dt` to 0.0025s in the `calculate_force` call to ensure consistent internal physics regardless of game engine jitter.

---

## Test Plan (TDD-Ready)

### Design Rationale
The tests must prove that the upsamplers correctly convert a 100Hz stepped signal into a continuous 400Hz signal and that the engine correctly consumes this data. We also must verify that legacy tests (running at variable Hz) still pass by supporting a "legacy/test mode" fallback.

### 1. `test_upsampling_logic`
Verifies that constant input remains stable across multiple 400Hz ticks.

### 2. `test_upsampling_interpolation`
Verifies that changing input produces different values across 400Hz ticks (interpolation active).

### 3. `test_upsampling_extrapolation`
Verifies that derivative-critical channels like `mLateralPatchVel` are correctly projected forward.

### 4. Regression Testing
Run all 396 existing physics tests to ensure zero regressions in standard FFB behavior.

---

## Implementation Notes

### Unforeseen Issues
*   **Derivative Spike Regression:** Initial implementation used raw 100Hz data for `m_prev_*` state updates. Combined with the new 400Hz `dt`, this quadrupled derivative spikes at telemetry boundaries. This was fixed by using `upsampled_data` for all state updates.
*   **Legacy Test Compatibility:** Existing tests assume every call to `calculate_force` is a new frame. Modified upsampling logic to detect "legacy/test mode" when `override_dt <= 0`, ensuring 100% backward compatibility.
*   **Tool-Induced Corruption & Resets:** Repeated file resets and deletions were required because automated refactoring tools (specifically `sed` patterns) corrupted `src/FFBEngine.cpp` with undeclared identifiers and malformed variable names. Manual repair cycles were necessary to stabilize the build. Additionally, accidental truncation of `CHANGELOG_DEV.md` required git restoration to preserve project history.

### Plan Deviations
*   **Persistent Working Copy:** Implemented `m_working_info` as a persistent member to optimize memory usage and simplify helper function calls.
*   **Expanded Scope:** Upsampling was expanded to all derivative-critical channels including `mUnfilteredThrottle`, `mUnfilteredBrake`, `mLocalAccel`, and `mLocalRotAccel.y` per review feedback.

### Recommendations
*   **Part 2 (Output Upsampling):** Consider output interpolation to hardware at 1000Hz+ to further reduce latency feel on high-end wheels.

## Deliverables
- [x] Update `src/MathUtils.h` with `LinearExtrapolator` and `HoltWintersFilter`.
- [x] Update `src/FFBEngine.h` with filter instances and persistent working copy.
- [x] Update `src/FFBEngine.cpp` with upsampling integration and fixed state updates.
- [x] Update `src/main.cpp` to force 400Hz physics `dt`.
- [x] Create `tests/test_upsampling.cpp`.
- [x] Increment `VERSION` to 0.7.116.
- [x] Update `CHANGELOG_DEV.md`.
- [x] Update implementation plan with notes and rationales.
