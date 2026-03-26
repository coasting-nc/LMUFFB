# Implementation Plan: Incrementally Fix `readability-magic-numbers` Warnings

This plan outlines an incremental approach to eliminate magic numbers in the LMUFFB codebase, improving readability and maintainability.

## 1. Objectives
- Replace hardcoded magic numbers with descriptive, named constants.
- Centralize common constants (Math, Physics, Wheel indices, Frequency) to prevent duplication.
- Systematically resolve `readability-magic-numbers` warnings produced by Clang-Tidy.

## 2. Iterative Workflow (The Process)
For each iteration (Phase/Step):
1.  **Identify Warnings**: Run Clang-Tidy on the target module to get the latest warnings.
    ```bash
    run-clang-tidy -p build -checks="-*,readability-magic-numbers" "src/<module>/.*"
    ```
    *Note: If Clang-Tidy crashes on a large file (a known issue with version 18), run it directly on isolated snippets or use `grep` to find the literals.*
2.  **Define Constants**: Create or update a header file with the necessary `static constexpr` constants.
3.  **Apply Fixes**: Perform a search-and-replace (or manual edit) to use the new constants.
4.  **Verify Build**: Build the project to ensure no syntax errors.
5.  **Run Tests**: Execute the test suite (`run_combined_tests`) to ensure no logic regressions.
6.  **Verify Clang-Tidy**: Re-run the Clang-Tidy check to confirm the warnings for that phase are resolved.

---

## 3. Iteration Phases

### Phase 1: Math and Physics Constants
**Goal**: Standardize universal constants like PI and Gravity.

#### 1.1 Universal Physical Constants
- **Example Warning**: `src/physics/GripLoadEstimation.cpp:254:41: warning: 9.81 is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: ~3 occurrences (PI, Gravity).
- **Implementation Steps**:
  1. Audit `src/physics/GripLoadEstimation.cpp` and `src/ffb/FFBEngine.cpp` for `9.81` or `9.80665`.
  2. Ensure `src/ffb/FFBEngine.h` contains `static constexpr double GRAVITY_MS2 = 9.81;`.
  3. Replace literals with `FFBEngine::GRAVITY_MS2`.
  4. Audit all files for `3.14159...` and replace with `LMUFFB::PI` from `MathUtils.h`.

#### 1.2 Common Mathematical Factors
- **Example Warning**: `src/physics/GripLoadEstimation.cpp:62:43: warning: 0.5 is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: ~35 occurrences (specifically `0.5` used for scaling/halving).
- **Implementation Steps**:
  1. Identify recurring simple factors like `0.5`, `2.0` (when used as a divisor/multiplier for scaling).
  2. Define `static constexpr double HALF = 0.5;` in `MathUtils.h`.
  3. Replace at least the non-obvious factors (e.g., used in smoothing or blending) with named constants.

### Phase 2: Wheel and Axle Indices
**Goal**: Eliminate remaining hardcoded indices (0-3) for wheel and axle data.

#### 2.1 Direct Array Indexing
- **Example Warning**: `src/ffb/UpSampler.cpp:25:34: warning: 2 is a magic number; consider replacing it with a named constant [readability-magic-numbers]` (In `m_history[2]`)
- **Current Warning Count**: ~75 occurrences (Manual indexing like `[0]`, `[1]`, `[2]`, `[3]`).
- **Implementation Steps**:
  1. Identify manual indexing of wheel or history arrays.
  2. Replace with `[WHEEL_FL]`, `[WHEEL_FR]`, etc. from `WheelConstants.h`.
  3. For `UpSampler`, define `HISTORY_LATEST = 2`, `HISTORY_PREV = 1`, etc.

#### 2.2 Standardizing Loop Bounds
- **Example Warning**: `src/ffb/FFBEngine.cpp:23:25: warning: 4 is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: Low (Most loops already use `NUM_WHEELS`, but some edge cases remain).
- **Implementation Steps**:
  1. Search for literal `4` and `2` used in wheel/axle loops.
  2. Replace with `NUM_WHEELS` and `NUM_AXLES` from `src/core/WheelConstants.h`.

### Phase 3: Frequency, Time & Upsampling
**Goal**: Standardize update rates and time-conversion factors.

#### 3.1 Update Rates and Intervals
- **Example Warning**: `src/core/main.cpp:90:35: warning: 1000 is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: ~30 occurrences (1000.0, 400.0, 0.001).
- **Implementation Steps**:
  1. Find usages of `1000.0` and `0.001` in `main.cpp` and physics modules.
  2. Define `TICKS_PER_SECOND = 1000.0` and `SECONDS_PER_TICK = 0.001`.
  3. Replace literals to clarify the logic's dependency on the 1000Hz loop rate.

#### 3.2 Upsampling & Filter Tuning
- **Example Warning**: `src/ffb/FFBEngine.cpp:20:35: warning: 0.95 is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: ~20 occurrences (Filter Alphas/Betas in FFBEngine).
- **Implementation Steps**:
  1. Identify recurring filter alphas/betas in `FFBEngine::InitializeEngine`.
  2. Group them by category (e.g., `DRIVER_INPUT_SMOOTHING = 0.95`, `DRIVER_INPUT_PREDICTION = 0.10`).
  3. Replace the magic floats in `Configure()` calls.

#### 3.3 Resampling Ratios
- **Example Warning**: `src/ffb/UpSampler.cpp:46:20: warning: 5 is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: ~10 occurrences (Specific to the 5/2 ratio in `UpSampler.cpp`).
- **Implementation Steps**:
  1. Update `UpSampler.cpp` to use named constants for the 5/2 ratio.
  2. Define `static constexpr int RESAMPLE_UP = 5;` and `static constexpr int RESAMPLE_DOWN = 2;`.

### Phase 4: Physics and Telemetry Thresholds
**Goal**: Replace arbitrary thresholds used in physics calculations.

#### 4.1 Speed-Based Logic
- **Example Warning**: `src/physics/GripLoadEstimation.cpp:33:17: warning: 2.0 is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: ~15 occurrences (2.0, 5.0, 15.0 m/s thresholds).
- **Implementation Steps**:
  1. Identify speed thresholds in `GripLoadEstimation.cpp`.
  2. Define `static constexpr double MIN_LEARNING_SPEED = 2.0;` and `static constexpr double AERO_TRANSITION_SPEED = 15.0;`.
  3. Replace literals in the `if` conditions.

#### 4.2 Force and Load Baselines
- **Example Warning**: `src/physics/GripLoadEstimation.cpp:35:32: warning: 100.0 is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: ~10 occurrences (100.0, 1000.0 N load validation).
- **Implementation Steps**:
  1. Identify load thresholds used to validate telemetry.
  2. Define named constants like `MIN_VALID_LOAD_N = 100.0;` and `LEARNED_LOAD_THRESHOLD_N = 1000.0;`.

### Phase 5: GUI and Layout Constants
**Goal**: Standardize the ImGui layout and styling values.

#### 5.1 Main Layout Dimensions
- **Example Warning**: `src/gui/GuiLayer_Common.cpp:69:34: warning: 500.0f is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: ~215 occurrences (All float literals in GUI code).
- **Implementation Steps**:
  1. Audit `GuiLayer_Common.cpp` for window and panel sizes.
  2. Consolidate into a `GuiConstants` namespace.

#### 5.2 Style and Colors
- **Example Warning**: `src/gui/GuiLayer_Common.cpp:76:28: warning: 5.0f is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Implementation Steps**:
  1. Define standard rounding values (e.g., `DEFAULT_WINDOW_ROUNDING = 5.0f`).
  2. Move hardcoded `ImVec4` color values to named constants like `COLOR_ACCENT_BLUE`.

---

## 4. Verification and Regression Testing
- Every change **must** be verified by running the existing regression tests.
- For physics-related changes (Phase 4), compare the output of `lmuffb_log_analyzer` on a sample log before and after the change to ensure identical FFB output.
- For UI changes, verify visual consistency across different window sizes.

## 5. Documentation
- Update `AGENTS.md` if any new coding standards regarding constants are established during this process.
