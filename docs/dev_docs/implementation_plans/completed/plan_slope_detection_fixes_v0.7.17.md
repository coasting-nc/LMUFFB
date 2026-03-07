# Implementation Plan - Slope Detection Fixes (v0.7.21)

## Context
Following the analysis of slope detection instability (Task `slope_detection_v0.7.16_analysis`), we confirmed critical instability in the algorithm. This task addresses the root cause: division-by-small-number artifacts resulting in unclamped slope values. We will implement safety clamps and a confidence ramp to mitigate this.

## Reference Documents
- **Investigation Report:** `docs/dev_docs/investigations/slope_detection_v0.7.16_analysis.md`
- **Unit Test Plan:** `docs/dev_docs/implementation_plans/plan_slope_detection_tests_v0.7.17.md`

## Codebase Analysis Summary
### Current Architecture
The core logic resides in `FFBEngine::calculate_slope_grip` (src/FFBEngine.h).
- Calculates `dG` and `dAlpha` via Savitzky-Golay.
- Computes `Slope = dG / dAlpha`.
- Applies binary threshold `dAlpha > m_slope_alpha_threshold`.
- Maps slope to grip loss (Linear Interp).

### Impacted Functionalities
1.  **Slope Calculation (`FFBEngine::calculate_slope_grip`)**:
    *   Add Hard Clamp logic.
    *   Integrate Confidence Ramp logic.
2.  **Configuration (`Config.h`, `FFBEngine.h`)**:
    *   Expose `m_slope_ramp_max` (the upper limit of confidence ramp) if needed, or document hardcoded behavior. (We will hardcode for now to minimize config complexity as per investigation).

## FFB Effect Impact Analysis
*   **Feel:**
    *   **Less abruptness:** The "Exploded" jolts will be gone.
    *   **Smoother Transitions:** Confidence ramp will gently blend strictly "Cornering" forces.
    *   **More Predictable:** No random grip loss on straights (due to noise + bump).

## Proposed Changes

### 1. Safety Clamping (Hotfix)
*   **File:** `src/FFBEngine.h`
*   **Method:** `calculate_slope_grip`
*   **Change:**
    ```cpp
    // After calculation:
    m_slope_current = std::clamp(m_slope_current, -20.0, 20.0);
    ```
    *   **Reason:** Prevent downstream logic (smoothing, lerp) from seeing infinity/NaN equivalents.

### 2. Confidence Ramp (Stability)
*   **File:** `src/FFBEngine.h`
*   **Method:** `calculate_slope_confidence`
*   **Change:**
    ```cpp
    // Replace:
    double conf_raw = std::abs(dAlpha_dt) / 0.1;

    // With (Smoothstep):
    double lower = m_slope_alpha_threshold; // e.g. 0.02
    double upper = 0.10; // Hardcoded reasonable upper bound for linear range
    double confidence = smoothstep(lower, upper, std::abs(dAlpha_dt));
    ```
    *   **Method:** `calculate_slope_grip`
    *   **Update:** Remove the `if (abs(dAlpha) > threshold)` gate completely (or lower it significantly, e.g. 0.005) and rely on `confidence` to zero out the result when `dAlpha` is tiny.
    *   **Or (simpler):** Keep the gate at 0.02 but clamp confidence [0..1] range starting *at* 0.02.
        *   **Decision:** The investigation recommends: "Remove `bool active = dAlpha > threshold`... Introduce `float confidence`...".
        *   **Implementation:**
            *   Calculate raw slope always (protected by min denominator).
            *   Calculate confidence based on `dAlpha` magnitude (0.01 -> 0.10).
            *   Output Grip Loss = `RawLoss * confidence`.

### 3. Denominator Protection
*   **File:** `src/FFBEngine.h`
*   **Change:**
    ```cpp
    double abs_dAlpha = std::abs(dAlpha_dt);
    double sign_dAlpha = (dAlpha_dt >= 0) ? 1.0 : -1.0;
    double protected_denom = (std::max)(0.005, abs_dAlpha) * sign_dAlpha;
    m_slope_current = dG_dt / protected_denom;
    ```

## Parameter Synchronization Checklist
*   **N/A**: No new user-facing parameters. Tuning `m_slope_alpha_threshold` defaults might be needed, but we will keep existing 0.02 for now as the lower bound of the ramp.

## Version Increment Rule
*   Increment `VERSION` file: Patch +1 (e.g., 0.7.16 -> 0.7.17).

## Test Plan (TDD-Ready)
See `docs/dev_docs/implementation_plans/plan_slope_detection_tests_v0.7.17.md` for specific test cases.
*   **Execution:** Create the tests first, verify they fail (e.g., singularity test explodes), then implement fixes.

## Deliverables
- [ ] Updated `FFBEngine.h` with clamping and ramp logic.
- [ ] Updated `VERSION`.
- [ ] Updated `CHANGELOG_DEV.md`.
- [ ] Passing Unit Tests.

## Implementation Notes - v0.7.21 (Actual Release)

The plan was fully implemented in version `0.7.21` with the following refinements:

1.  **Hybrid Gating**: While the plan suggested removing the `if (abs(dAlpha) > threshold)` gate, it was maintained for the `m_slope_current` *state update* to ensure the real-time graph remains stable on straights (preventing noise-amplification when `abs(dAlpha)` is near zero).
2.  **Smoothstep Blending**: The binary gate was successfully removed from the *grip-loss calculation*. Instead, `smoothstep` provides a continuous confidence ramp starting at the threshold (`m_slope_alpha_threshold`). This ensures that even if the physics state "jumps" when entering the gate, the transmitted force transitions seamlessly from zero effect.
3.  **Hard Clamping**: `std::clamp(..., -20.0, 20.0)` was used as planned to bound the sensitivity of the algorithm.
4.  **Denominator Protection**: The `(std::max)(0.005, abs_dAlpha)` logic was implemented within the gate to provide extra safety against edge-case singularities.
5.  **Test Results**: All 962 assertions passed, confirming that the new continuous ramp architecture maintains compatibility with established noise-rejection and decay-rate requirements.
6.  **Versioning**: The fixes were tagged as `v0.7.21` (incremented from `v0.7.20`).