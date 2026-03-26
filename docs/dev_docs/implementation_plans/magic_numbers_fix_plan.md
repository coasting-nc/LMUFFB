# Implementation Plan: Incrementally Fix `readability-magic-numbers` Warnings

This plan outlines an incremental approach to eliminate magic numbers in the LMUFFB codebase, improving readability and maintainability.

## 1. Objectives
- Replace hardcoded magic numbers with descriptive, named constants.
- Centralize common constants (Math, Physics, Wheel indices, Frequency) to prevent duplication.
- Systematically resolve `readability-magic-numbers` warnings produced by Clang-Tidy.

## 2. Iterative Workflow (The Process)
For each iteration (Phase):
1.  **Identify Warnings**: Run Clang-Tidy on the target module to get the latest warnings.
    ```bash
    run-clang-tidy -p build -checks="-*,readability-magic-numbers" "src/<module>/.*"
    ```
2.  **Define Constants**: Create or update a header file with the necessary `static constexpr` constants.
3.  **Apply Fixes**: Perform a search-and-replace (or manual edit) to use the new constants.
4.  **Verify Build**: Build the project to ensure no syntax errors.
5.  **Run Tests**: Execute the test suite (`run_combined_tests`) to ensure no logic regressions.
6.  **Verify Clang-Tidy**: Re-run the Clang-Tidy check to confirm the warnings for that phase are resolved.

---

## 3. Iteration Phases

### Phase 1: Math and Physics Constants
**Goal**: Standardize universal constants like PI and Gravity.
- **Identify**: Find usages of `3.14159...`, `9.81`, `9.80665`.
- **Constants**: Use `LMUFFB::PI` in `src/utils/MathUtils.h` and `LMUFFB::GRAVITY_MS2` in `src/ffb/FFBEngine.h` (or move to a new `PhysicsConstants.h`).
- **Files**: `src/physics/*.cpp`, `src/ffb/*.cpp`.

### Phase 2: Wheel and Axle Indices
**Goal**: Eliminate hardcoded indices (0-3) for wheel and axle data.
- **Identify**: Find loops like `for (int i = 0; i < 4; ++i)` and array accesses like `m_wheels[0]`.
- **Constants**: Use `WHEEL_FL`, `WHEEL_FR`, `WHEEL_RL`, `WHEEL_RR`, `NUM_WHEELS`, and `NUM_AXLES` from `src/core/WheelConstants.h`.
- **Files**: `src/ffb/FFBEngine.cpp`, `src/physics/GripLoadEstimation.cpp`, `src/logging/ChannelMonitor.h`, and related tests.

### Phase 3: Frequency and Time Constants
**Goal**: Standardize update rates and time-conversion factors.
- **Identify**: Find usages of `1000.0`, `400.0`, `0.001`, `0.0025`.
- **Constants**:
  - `TICKS_PER_SECOND = 1000.0` (FFB Loop)
  - `PHYSICS_TICKS_PER_SECOND = 400.0` (Telemetry rate)
  - `SECONDS_PER_TICK = 0.001`
  - `PHYSICS_DT = 0.0025`
- **Files**: `src/core/main.cpp`, `src/ffb/UpSampler.cpp`, `src/physics/*.cpp`.

### Phase 4: Physics and Telemetry Thresholds
**Goal**: Replace arbitrary thresholds used in physics calculations.
- **Identify**: Values like `2.0`, `15.0` (speed thresholds), `100.0`, `1000.0` (load thresholds).
- **Constants**:
  - `MIN_SPEED_FOR_LOAD_ESTIMATION = 2.0`
  - `AERO_SPEED_THRESHOLD = 15.0`
  - `LOAD_THRESHOLD_N = 1000.0`
- **Files**: `src/physics/GripLoadEstimation.cpp`, `src/physics/SteeringUtils.cpp`.

### Phase 5: GUI and Layout Constants
**Goal**: Standardize the ImGui layout and styling values.
- **Identify**: Hardcoded padding, widths, and color values in the UI code.
- **Constants**: Define `static constexpr float` values at the top of `src/gui/GuiLayer_Common.cpp` or in a new `GuiConstants.h`.
- **Files**: `src/gui/GuiLayer_Common.cpp`.

---

## 4. Verification and Regression Testing
- Every change **must** be verified by running the existing regression tests.
- For physics-related changes (Phase 4), compare the output of `lmuffb_log_analyzer` on a sample log before and after the change to ensure identical FFB output.
- For UI changes, verify visual consistency across different window sizes.

## 5. Documentation
- Update `AGENTS.md` if any new coding standards regarding constants are established during this process.
