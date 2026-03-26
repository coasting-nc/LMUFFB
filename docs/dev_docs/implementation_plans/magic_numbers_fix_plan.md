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
- **Example Warning**: `warning: 9.81 is a magic number; consider replacing it with a named constant`
- **Implementation Steps**:
  1. Audit `src/physics/GripLoadEstimation.cpp` and `src/ffb/FFBEngine.cpp` for `9.81` or `9.80665`.
  2. Ensure `src/ffb/FFBEngine.h` contains `static constexpr double GRAVITY_MS2 = 9.81;`.
  3. Replace literals with `FFBEngine::GRAVITY_MS2`.
  4. Audit all files for `3.14159...` and replace with `LMUFFB::PI` from `MathUtils.h`.

#### 1.2 Common Mathematical Factors
- **Example Usage**: `m_static_front_load = m_auto_peak_front_load * 0.5;`
- **Implementation Steps**:
  1. Identify recurring simple factors like `0.5`, `2.0` (when used as a divisor/multiplier for scaling).
  2. Evaluate if they should be named (e.g., `HALF = 0.5`) or if they are excluded by `Clang-Tidy` configuration (often `0` and `1` are ignored).

### Phase 2: Wheel and Axle Indices
**Goal**: Eliminate hardcoded indices (0-3) for wheel and axle data.

#### 2.1 Loop Bounds
- **Example Warning**: `warning: 4 is a magic number; consider replacing it with a named constant`
- **Implementation Steps**:
  1. Search for `for (int i = 0; i < 4; ++i)` and `for (int i = 0; i < 2; ++i)`.
  2. Replace with `for (int i = 0; i < NUM_WHEELS; ++i)` and `for (int i = 0; i < NUM_AXLES; ++i)`.
  3. Update related array declarations if they use literals.

#### 2.2 Direct Array Indexing
- **Example Usage**: `m_upsample_brake_pressure[0].Process(...)`
- **Implementation Steps**:
  1. Identify manual indexing of wheel arrays: `[0]`, `[1]`, `[2]`, `[3]`.
  2. Replace with `[WHEEL_FL]`, `[WHEEL_FR]`, `[WHEEL_RL]`, `[WHEEL_RR]`.
  3. Apply to `FFBEngine.cpp`, `GripLoadEstimation.cpp`, and `ChannelMonitor.h`.

### Phase 3: Frequency, Time & Upsampling
**Goal**: Standardize update rates and time-conversion factors.

#### 3.1 Update Rates and Intervals
- **Example Usage**: `dt / 0.001`
- **Implementation Steps**:
  1. Find usages of `1000.0` and `0.001` in `main.cpp` and physics modules.
  2. Define `TICKS_PER_SECOND = 1000.0` and `SECONDS_PER_TICK = 0.001`.
  3. Replace literals to clarify that the logic depends on the 1000Hz loop rate.

#### 3.2 Upsampling & Filter Tuning
- **Example Usage**: `m_upsample_steering.Configure(0.95, 0.10);`
- **Implementation Steps**:
  1. Identify recurring filter alphas/betas in `FFBEngine::InitializeEngine`.
  2. Group them by category (e.g., `DRIVER_INPUT_SMOOTHING = 0.95`, `DRIVER_INPUT_PREDICTION = 0.10`).
  3. Replace the magic floats in `Configure()` calls.

#### 3.3 Resampling Ratios
- **Example Warning**: `src/ffb/UpSampler.cpp:46:20: warning: 5 is a magic number`
- **Implementation Steps**:
  1. Update `UpSampler.cpp` to use named constants for the 5/2 ratio.
  2. Define `static constexpr int RESAMPLE_UP = 5;` and `static constexpr int RESAMPLE_DOWN = 2;`.

### Phase 4: Physics and Telemetry Thresholds
**Goal**: Replace arbitrary thresholds used in physics calculations.

#### 4.1 Speed-Based Logic
- **Example Usage**: `if (speed > 2.0 && speed < 15.0)`
- **Implementation Steps**:
  1. Identify speed thresholds in `GripLoadEstimation.cpp`.
  2. Define `static constexpr double MIN_LEARNING_SPEED = 2.0;` and `static constexpr double AERO_TRANSITION_SPEED = 15.0;`.
  3. Replace literals in the `if` conditions.

#### 4.2 Force and Load Baselines
- **Example Usage**: `if (m_static_front_load < 1000.0)`
- **Implementation Steps**:
  1. Identify load thresholds (e.g., `100.0`, `1000.0`) used to validate telemetry.
  2. Define named constants like `MIN_VALID_LOAD_N = 100.0;` and `LEARNED_LOAD_THRESHOLD_N = 1000.0;`.

### Phase 5: GUI and Layout Constants
**Goal**: Standardize the ImGui layout and styling values.

#### 5.1 Main Layout Dimensions
- **Example Usage**: `static const float CONFIG_PANEL_WIDTH = 500.0f;`
- **Implementation Steps**:
  1. Audit `GuiLayer_Common.cpp` for window and panel sizes.
  2. Consolidate into a `GuiConstants` namespace if they are used in multiple places.

#### 5.2 Style and Colors
- **Example Usage**: `style.WindowRounding = 5.0f;`
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
