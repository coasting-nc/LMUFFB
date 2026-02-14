# Implementation Plan - Slope Detection Fixes & Telemetry Enhancements

## 1. Context
The Slope Detection feature, intended to estimate front tire grip loss by analyzing the relationship between Lateral G and Slip Angle, is currently exhibiting mathematical instability (singularities) and logic failures during steady-state cornering. This results in erratic FFB behavior ("banging" forces) and loss of understeer feel during long corners. Additionally, the current telemetry logging lacks sufficient internal state data to fully diagnose these complex mathematical behaviors.

**Goal:** 
1.  Replace the unstable division-based slope calculation with a robust "Projected Slope" method.
2.  Implement "Hold-and-Decay" logic to maintain understeer feel during steady-state cornering.
3.  Expand the `AsyncLogger` to capture internal math states for validation.

**Reference Documents:**
*   Diagnostic Reports: 
    - `https://github.com/coasting-nc/LMUFFB/issues/25#issuecomment-3899222192`
    - `https://github.com/coasting-nc/LMUFFB/issues/25#issuecomment-3899252429`

*   `docs\dev_docs\investigations\Recommended Additions to Telemetry Logger.md`
*   `docs\dev_docs\investigations\improve_slope_Detection_v0.7.35+.md`
*   `docs\dev_docs\investigations\slope_detection_feasibility.md`

## 2. Codebase Analysis

### 2.1 Architecture Overview
*   **Physics Engine (`src/FFBEngine.h`):** Contains the core `calculate_slope_grip` function responsible for the logic. It uses a circular buffer and Savitzky-Golay filters to compute derivatives.
*   **Telemetry Logging (`src/AsyncLogger.h`):** Handles the high-frequency recording of physics data to CSV. It currently logs inputs and final outputs but misses intermediate calculation steps.
*   **Configuration (`src/Config.h`):** Manages user settings. While no new user settings are strictly required, the interpretation of existing sensitivity/threshold settings will change slightly due to the new math.

### 2.2 Impacted Functionalities
*   **Slope Detection Algorithm:** The fundamental math changing from `y/x` to `(x*y)/(x^2 + e)` to avoid division by zero.
*   **State Management:** New state variables are needed in `FFBEngine` to track "Hold" timers and pre-smoothed inputs.
*   **CSV Output:** The log file format will change (new columns), which may affect external analysis tools.

## 3. FFB Effect Impact Analysis

| Effect | Technical Impact | User Perspective |
| :--- | :--- | :--- |
| **Understeer (Front Grip Loss)** | **Major Change.** The `calculate_slope_grip` function drives the `ctx.grip_factor`. <br> **Old:** Fluctuated wildly (0% <-> 100%) due to noise/singularities; dropped to 0% effect (full grip) during steady turns. <br> **New:** Will use Projected Slope for stability and Hold Timer for continuity. | **Smoother & More Consistent.** <br> - No more random "jerks" or "banging" in the wheel. <br> - The wheel will stay light (understeering) during long, steady corners instead of artificially regaining weight. <br> - "Slope Sensitivity" may feel slightly less aggressive, requiring retuning. |
| **General FFB** | **Minor.** Reduced high-frequency noise injection into the main loop due to input pre-smoothing. | Slightly "cleaner" feeling FFB signal. |

## 4. Proposed Changes

### 4.1 File: `src/FFBEngine.h`

**A. Add Internal State Members**
Add variables to track pre-smoothed inputs and the hold timer.
```cpp
private:
    // ... existing slope members ...
    
    // NEW: Input Smoothing State
    double m_slope_lat_g_smoothed = 0.0;
    double m_slope_slip_smoothed = 0.0;

    // NEW: Steady State Logic
    double m_slope_hold_timer = 0.0;
    static constexpr double SLOPE_HOLD_TIME = 0.25; // 250ms hold
    
    // NEW: Debug members for Logger
    double m_debug_slope_raw = 0.0;
    double m_debug_slope_num = 0.0;
    double m_debug_slope_den = 0.0;
```

**B. Rewrite `calculate_slope_grip`**
Implement the robust logic:
1.  **Pre-Smoothing:** Apply LPF (tau ~0.01s) to `lateral_g` and `slip_angle` *before* buffering.
2.  **Projected Slope:** Calculate `slope = (dG * dAlpha) / (dAlpha^2 + epsilon)`.
3.  **Hold Logic:** 
    *   If `abs(dAlpha) > threshold`: Update slope, reset timer.
    *   Else: Decrement timer. If timer > 0, hold previous slope. If timer <= 0, decay slope to 0.

**C. Update `calculate_force`**
Populate the new debug members into the `LogFrame` struct before calling `AsyncLogger::Log`.

### 4.2 File: `src/AsyncLogger.h`

**A. Update `LogFrame` Struct**
Add fields for internal math state.
```cpp
struct LogFrame {
    // ...
    float slope_raw_unclamped;
    float slope_numerator;
    float slope_denominator;
    float hold_timer;
    float input_slip_smoothed;
    // ...
};
```

**B. Update `WriteHeader`**
Add columns: `SlopeRaw,SlopeNum,SlopeDenom,HoldTimer,InputSlipSmooth`.

**C. Update `WriteFrame`**
Output the new fields to the CSV stream.

### 4.3 File: `VERSION` & `src/Version.h`
*   Increment version (e.g., `0.7.35` -> `0.7.36`).

## 5. Test Plan (TDD)

**New Test File:** `tests/test_ffb_slope_fix.cpp`

### Test 1: `test_slope_singularity_rejection`
*   **Goal:** Verify math stability when `dAlpha` is near zero but `dG` is non-zero (e.g., bump while driving straight).
*   **Setup:** 
    *   Initialize Engine.
    *   Feed telemetry where `SlipAngle` is constant (dAlpha ~ 0).
    *   Inject a spike in `LateralG` (dG >> 0).
*   **Assertion:** 
    *   **Old Behavior:** Slope explodes (Singularity).
    *   **New Behavior:** `m_slope_current` remains near 0. `slope_denominator` should be `epsilon`.

### Test 2: `test_slope_steady_state_hold`
*   **Goal:** Verify the "Hold" logic maintains understeer during steady cornering.
*   **Data Flow Script:**
    1.  **Frames 1-20 (Transient):** Ramp `SlipAngle` and `LateralG` to simulate entering a corner. `dAlpha` > threshold.
        *   *Check:* `m_slope_hold_timer` resets to `SLOPE_HOLD_TIME`. Grip factor drops (Understeer).
    2.  **Frames 21-40 (Steady):** Hold `SlipAngle` and `LateralG` constant. `dAlpha` ~ 0.
        *   *Check:* `m_slope_hold_timer` decreases but > 0.
        *   *Check:* `m_slope_current` does **NOT** decay. Grip factor remains low.
    3.  **Frames 41+ (Decay):** Continue holding constant.
        *   *Check:* Once timer expires, `m_slope_current` decays toward 0.

### Test 3: `test_input_smoothing`
*   **Goal:** Verify 8Hz noise is attenuated.
*   **Setup:** Feed noisy `SlipAngle` signal (sine wave at 50Hz).
*   **Assertion:** `m_slope_slip_smoothed` amplitude < Raw Input amplitude.

## 6. Deliverables

*   [ ] **Code:** Updated `src/FFBEngine.h` (Logic fix).
*   [ ] **Code:** Updated `src/AsyncLogger.h` (Logging enhancement).
*   [ ] **Tests:** New `tests/test_ffb_slope_fix.cpp`.
*   [ ] **Docs:** Update `docs/dev_docs/plans/plan_{{TASK_ID}}.md` with implementation notes.
*   [ ] **Docs:** Create `docs/dev_docs/log_analyzer_spec.md` describing the new CSV columns for the analyzer tool.

```json
{
  "status": "success",
  "plan_path": "docs/dev_docs/plans/plan_slope_fix.md",
  "backlog_items": []
}
```