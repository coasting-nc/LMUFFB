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

## 3. Core Logic Refactoring

### Phase 1: Math and Physics Constants
**Goal**: Standardize universal constants like PI and Gravity.

**1.1 Universal Physical Constants**
- **Example Warning**: `src/physics/GripLoadEstimation.cpp:254:41: warning: 9.81 is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: ~3 occurrences (PI, Gravity).
- **Steps**:
  1. Audit `src/physics/GripLoadEstimation.cpp` and `src/ffb/FFBEngine.cpp` for `9.81` or `9.80665`.
  2. Use `FFBEngine::GRAVITY_MS2` and `LMUFFB::PI` from `MathUtils.h`.

**1.2 Common Mathematical Factors**
- **Example Warning**: `src/physics/GripLoadEstimation.cpp:62:43: warning: 0.5 is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: ~35 occurrences.
- **Steps**:
  1. Define `static constexpr double HALF = 0.5;` in `MathUtils.h`.
  2. Replace non-obvious scaling factors (e.g., used in smoothing or blending) with named constants.

### Phase 2: Wheel and Axle Indices
**Goal**: Eliminate remaining hardcoded indices (0-3) for wheel and axle data.

**2.1 Direct Array Indexing**
- **Example Warning**: `src/ffb/UpSampler.cpp:25:34: warning: 2 is a magic number; consider replacing it with a named constant [readability-magic-numbers]` (In `m_history[2]`)
- **Current Warning Count**: ~75 occurrences.
- **Steps**:
  1. Use `[WHEEL_FL]`, `[WHEEL_FR]`, etc. from `WheelConstants.h`.
  2. For `UpSampler`, define `HISTORY_LATEST = 2`, `HISTORY_PREV = 1`, etc.

**2.2 Standardizing Loop Bounds**
- **Example Warning**: `src/ffb/FFBEngine.cpp:23:25: warning: 4 is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: Low.
- **Steps**: Use `NUM_WHEELS` and `NUM_AXLES` from `src/core/WheelConstants.h`.

---

## 4. Signal Processing & Thresholds

### Phase 3: Frequency, Time & Upsampling
**Goal**: Standardize update rates and time-conversion factors.

**3.1 Update Rates and Intervals**
- **Example Warning**: `src/core/main.cpp:90:35: warning: 1000 is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: ~30 occurrences.
- **Steps**: Define `TICKS_PER_SECOND = 1000.0` and `SECONDS_PER_TICK = 0.001`.

**3.2 Upsampling & Filter Tuning**
- **Example Warning**: `src/ffb/FFBEngine.cpp:20:35: warning: 0.95 is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: ~20 occurrences.
- **Steps**: Define named constants like `DRIVER_INPUT_SMOOTHING = 0.95`.

**3.3 Resampling Ratios**
- **Example Warning**: `src/ffb/UpSampler.cpp:46:20: warning: 5 is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: ~10 occurrences.
- **Steps**: Define `RESAMPLE_UP = 5` and `RESAMPLE_DOWN = 2`.

### Phase 4: Physics and Telemetry Thresholds
**Goal**: Replace arbitrary thresholds used in physics calculations.

**4.1 Speed-Based Logic**
- **Example Warning**: `src/physics/GripLoadEstimation.cpp:33:17: warning: 2.0 is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: ~15 occurrences.
- **Steps**: Define `MIN_LEARNING_SPEED = 2.0`, `AERO_TRANSITION_SPEED = 15.0`.

**4.2 Force and Load Baselines**
- **Example Warning**: `src/physics/GripLoadEstimation.cpp:35:32: warning: 100.0 is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: ~10 occurrences.
- **Steps**: Define `MIN_VALID_LOAD_N = 100.0`, `LEARNED_LOAD_THRESHOLD_N = 1000.0`.

---

## 5. GUI Layer Refactoring

### Phase 5: GUI Theme and Style Constants
**Goal**: Standardize colors and rounding values.

**5.1 Color Definitions (ImVec4)**
- **Example Warning**: `src/gui/GuiLayer_Common.cpp:87:44: warning: 0.12f is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: ~60 occurrences.
- **Steps**: Define `GuiColors` namespace with named `ImVec4` constants.

**5.2 Style Rounding and Padding**
- **Example Warning**: `src/gui/GuiLayer_Common.cpp:76:28: warning: 5.0f is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: ~15 occurrences.
- **Steps**: Define `STYLE_WINDOW_ROUNDING = 5.0f`, etc.

### Phase 6: GUI Layout and Spacing
**Goal**: Standardize window sizes and structural spacing.

**6.1 Window and Panel Dimensions**
- **Example Warning**: `src/gui/GuiLayer_Common.cpp:69:34: warning: 500.0f is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: ~10 occurrences.
- **Steps**: Define `MAIN_WINDOW_WIDTH`, `CONFIG_PANEL_WIDTH`.

**6.2 Structural Spacing (SameLine/Spacing)**
- **Example Warning**: `src/gui/GuiLayer_Common.cpp:328:24: warning: 20.0f is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: ~20 occurrences.
- **Steps**: Define `LAYOUT_INDENT = 20.0f`, `ITEM_SPACING = 10.0f`.

### Phase 7: GUI Widget Parameters
**Goal**: Standardize slider limits and default values.

**7.1 Slider and Input Bounds**
- **Example Warning**: `src/gui/GuiLayer_Common.cpp:569:70: warning: 5.0f is a magic number; consider replacing it with a named constant [readability-magic-numbers]`
- **Current Warning Count**: ~150+ occurrences.
- **Steps**: Define group-specific `MIN`/`MAX` constants for UI controls.

---

## 6. Verification and Regression Testing
- Every change **must** be verified by running the existing regression tests.
- For physics-related changes, compare the output of `lmuffb_log_analyzer` on a sample log.
- For UI changes, verify visual consistency across different window sizes.

## 7. Documentation
- Update `AGENTS.md` if any new coding standards regarding constants are established during this process.
