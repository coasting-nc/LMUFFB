# Implementation Plan - Slope Detection Fixes (Asymmetry & Tuning)

## 1. Context
A code review of the Slope Detection feature (specifically the advanced Torque Slope logic introduced in v0.7.40) identified a **critical logic bug** causing asymmetric behavior between left and right turns. Additionally, hardcoded constants in the confidence logic limit the feature's effectiveness for users with slow steering racks or smooth driving styles.

**Goal:**
1.  **Fix Asymmetry:** Ensure Torque Slope calculation uses absolute values to function correctly for both left and right turns.
2.  **Improve Tuning:** Expose the confidence upper bound as a configurable parameter (`slope_confidence_max_rate`) to accommodate different steering speeds.
3.  **Standardize Thresholds:** Replace hardcoded constants with configurable thresholds where appropriate.

**Reference Documents:**
*   Code Review Analysis: `docs/dev_docs/code_reviews/review_v_0.7.40_slope_detection.md`
*   `docs/dev_docs/implementation_plans/Slope Detection Advanced Features.md`

## 2. Codebase Analysis

### 2.1 Architecture Overview
*   **Physics Engine (`src/FFBEngine.h`):**
    *   `calculate_slope_grip`: Handles signal smoothing and slope calculation. Currently smooths *signed* torque but *absolute* steering, causing the asymmetry.
    *   `calculate_slope_confidence`: Uses a hardcoded `0.10` value for the `smoothstep` upper bound.
*   **Configuration (`src/Config.h`, `src/Config.cpp`):**
    *   Manages user settings. Needs updates to support the new `slope_confidence_max_rate` parameter.

### 2.2 Impacted Functionalities
*   **Slope Detection:** The core grip estimation logic will be altered to be direction-invariant.
*   **Confidence Logic:** The "confidence" scalar (which attenuates the effect when steering inputs are noisy or small) will become tunable.

## 3. FFB Effect Impact Analysis

| Effect | Technical Impact | User Perspective |
| :--- | :--- | :--- |
| **Understeer (Slope)** | **Fix.** The `m_slope_torque_current` calculation will now use `std::abs(torque)`. | **Bug Fix.** Left turns will no longer trigger false "grip loss" sensations. The FFB feel will be consistent in both directions. |
| **Slope Confidence** | **New Parameter.** `slope_confidence_max_rate` added to config. | **Tuning.** Users with slow steering racks (trucks, vintage cars) can lower this value to get full effect strength without needing to jerk the wheel. |

## 4. Proposed Changes

### 4.1 File: `src/FFBEngine.h`

**A. Fix Asymmetry in `calculate_slope_grip`**
Change the smoothing logic to use absolute torque.
```cpp
// OLD:
// m_slope_torque_smoothed += alpha * (data->mSteeringShaftTorque - m_slope_torque_smoothed);

// NEW:
m_slope_torque_smoothed += alpha * (std::abs(data->mSteeringShaftTorque) - m_slope_torque_smoothed);
```

**B. Add Member Variable**
```cpp
float m_slope_confidence_max_rate = 0.10f; // Default 0.10 rad/s
```

**C. Update `calculate_slope_confidence`**
Replace hardcoded `0.10` with the member variable.
```cpp
return smoothstep((double)m_slope_alpha_threshold, (double)m_slope_confidence_max_rate, std::abs(dAlpha_dt));
```

**D. Update Torque Slope Threshold**
Replace hardcoded `0.01` with `m_slope_alpha_threshold` (or a derived value) for consistency.
```cpp
// Line 707 approx
if (std::abs(dSteer_dt) > (double)m_slope_alpha_threshold) { ... }
```

### 4.2 File: `src/Config.h`

**A. Update `Preset` Struct**
Add `float slope_confidence_max_rate = 0.10f;`.

**B. Update `Preset` Methods**
*   **`Apply`**: `engine.m_slope_confidence_max_rate = (std::max)(m_slope_alpha_threshold + 0.01f, slope_confidence_max_rate);`
*   **`Validate`**: Ensure `slope_confidence_max_rate > slope_alpha_threshold`.
*   **`UpdateFromEngine`**: Copy value from engine.
*   **`Equals`**: Compare value.

### 4.3 File: `src/Config.cpp`

**A. Update `ParsePresetLine`**
Add parsing for `slope_confidence_max_rate`.

**B. Update `WritePresetFields`**
Add writing for `slope_confidence_max_rate`.

**C. Update `Config::Save` / `Config::Load`**
Persist the new parameter in `config.ini`.

### 4.4 File: `VERSION` & `src/Version.h`
*   Increment version (e.g., `0.7.40` -> `0.7.41`).

## 5. Test Plan (TDD)

**New Test File:** `tests/test_ffb_slope_edge_cases.cpp`

### Test 1: `test_slope_asymmetry_fix`
*   **Goal:** Verify Left vs Right turn produces same grip factor.
*   **Setup:**
    *   Initialize Engine. Enable Slope Detection.
    *   **Scenario A (Right):** Ramp Steering $0 \to +0.5$, Torque $0 \to +5.0$.
    *   **Scenario B (Left):** Ramp Steering $0 \to -0.5$, Torque $0 \to -5.0$.
*   **Assertion:** `grip_factor` from Scenario A must equal `grip_factor` from Scenario B. (Previously B would drop grip).

### Test 2: `test_slope_confidence_tuning`
*   **Goal:** Verify `slope_confidence_max_rate` affects output.
*   **Setup:**
    *   Set `dAlpha_dt` input to `0.05`.
    *   **Case 1:** `max_rate = 0.10` (Default). Confidence should be ~0.5 (mid-range).
    *   **Case 2:** `max_rate = 0.05`. Confidence should be ~1.0 (maxed).
*   **Assertion:** Confidence in Case 2 > Confidence in Case 1.

### Test 3: `test_torque_slope_timing`
*   **Goal:** Verify Torque Slope drops before G Slope (Anticipation).
*   **Setup:**
    *   Simulate "Pneumatic Trail Loss": Torque drops while G continues rising.
*   **Assertion:** `grip_factor` drops immediately when Torque drops, validating the fusion logic.

## 6. Deliverables

*   [x] **Code:** Updated `src/FFBEngine.h` (Logic fixes).
*   [x] **Code:** Updated `src/Config.h` & `src/Config.cpp` (New parameter).
*   [x] **Tests:** New `tests/test_ffb_slope_edge_cases.cpp`.
*   [x] **Docs:** Update `docs/dev_docs/implementation_plans/Slope Detection Fixes v0.7.41.md` with implementation notes.

## Implementation Notes

- **Unforeseen Issues:** Changing internal slope detection to use absolute values (`std::abs`) for G-force and Torque required updating existing unit tests in `tests/test_ffb_slope_detection.cpp`. These tests previously relied on signed inputs to produce negative slopes, which no longer worked because the engine now compares magnitudes. Tests were updated to use decreasing magnitudes (with large offsets to avoid zero-crossing) to correctly simulate grip loss symmetrically.
- **Plan Deviations:** None. All planned features (asymmetry fix, confidence tuning, threshold standardization) were implemented as specified.
- **Challenges Encountered:** Managing the transition to absolute values while maintaining the "negative slope = grip loss" concept required careful adjustment of test telemetry generation.
- **Recommendations for Future Plans:** When changing fundamental signal processing (like moving from signed to absolute), always include a step to audit and update dependent regression tests.
