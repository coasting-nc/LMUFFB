# FFBEngine.cpp — Refactoring Analysis

**Date:** 2026-02-20
**File analysed:** `src/FFBEngine.cpp` (1506 lines, 28 functions)

---

## Current State Summary

The file contains the following functional groups:

| Category | Functions | Lines (approx) |
|----------|-----------|----------------|
| **Core FFB pipeline** | `calculate_force`, `apply_signal_conditioning` | ~620 |
| **Grip estimation** | `calculate_grip`, `calculate_slope_grip`, `calculate_slope_confidence`, `calculate_slip_angle`, `calculate_raw_slip_angle_pair`, `calculate_manual_slip_ratio` | ~230 |
| **Load estimation** | `approximate_load`, `approximate_rear_load`, `calculate_kinematic_load`, `update_static_load_reference`, `InitializeLoadReference` | ~100 |
| **SoP / Lateral effects** | `calculate_sop_lateral`, `calculate_gyro_damping` | ~90 |
| **Tactile effects** | `calculate_lockup_vibration`, `calculate_wheel_spin`, `calculate_slide_texture`, `calculate_road_texture`, `calculate_abs_pulse`, `calculate_suspension_bottoming`, `calculate_soft_lock` | ~300 |
| **Safety & utility** | `IsFFBAllowed`, `ApplySafetySlew`, `GetDebugBatch`, `calculate_wheel_slip_ratio` | ~60 |
| **Snapshot + Logging** (inline in `calculate_force`) | — | ~140 |

---

## Why Grip + Load Estimation Is a Good Extraction Candidate

### 1. Clear input/output boundary

These functions take raw wheel telemetry (`TelemWheelV01`, `TelemInfoV01`) as inputs and return a `GripResult` or a `double` load value. The FFB pipeline simply consumes those outputs — it does not care *how* they were produced. This is a textbook seam.

### 2. "Data preparation" vs "FFB logic"

The conceptual split is natural:

- **`GripLoadEstimation.cpp`**: *"Given raw telemetry — possibly broken or encrypted — produce the best available grip and load values."*
- **`FFBEngine.cpp`**: *"Given known-good grip and load values, compute the force feedback signal."*

This framing matches the user's own intuition: the FFB engine is a consumer of grip and load information, whether from real game data or from a fallback estimator. The source of those values is an implementation detail that doesn't belong in the FFB signal chain.

### 3. Self-contained complexity

`calculate_slope_grip` alone is **114 lines** of Savitzky-Golay derivative estimation, hold-and-decay logic, torque-based pneumatic trail anticipation, and a fusion layer. It owns **15+ private member variables** (`m_slope_*`). A developer reading the main FFB pipeline should not need to understand this algorithm to follow the signal flow. Extracting it removes the biggest readability obstacle in the file.

### 4. Testability

The grip and load estimation functions are already independently tested in the test suite. Moving them to a separate translation unit makes the boundary even more explicit without breaking any existing tests.

---

## Three Approaches for the Extraction

### Approach A — Separate `.cpp` file, same class (source-level split only)

Move the target functions verbatim into a new file `src/GripLoadEstimation.cpp`. All method signatures remain `FFBEngine::methodName(...)`. `FFBEngine.h` is **unchanged**. The new file `#include`s `FFBEngine.h` just like the original.

| Pros | Cons |
|------|------|
| Zero risk — purely mechanical | Does not improve testability (same class boundary) |
| No test changes required | No change to the API surface |
| Immediately yields a shorter `FFBEngine.cpp` | Member-variable coupling is unchanged |
| Consistent with existing pattern (`VehicleUtils.cpp`) | |

### Approach B — Composition with an embedded helper class

Introduce a `GripEstimator` class that owns its own smoothing state (`m_slope_*`, `m_prev_slip_angle`, etc.). `FFBEngine` holds a `GripEstimator` member and delegates the calls.

| Pros | Cons |
|------|------|
| Clean state encapsulation | Significant refactor — member variables must be migrated |
| Improved testability (can instantiate `GripEstimator` standalone) | Settings (sliders) are on `FFBEngine` — need pass-through |
| Natural extension point for future grip model variants | Breaks test-access patterns that currently poke at `FFBEngine` internals |

### Approach C — Free functions + explicit state struct

Grip/load functions become free functions accepting an explicit `GripEstimatorState&` struct that holds all their required state. Most testable approach.

| Pros | Cons |
|------|------|
| Maximum testability | Largest refactor |
| Zero coupling to `FFBEngine` | Not idiomatic given the rest of the codebase |
| Could live in `MathUtils.h` or its own header | Requires migrating ~15 member variables to a new struct |

---

## Recommendation

**Start with Approach A. Evolve towards Approach B as the codebase matures.**

Approach A gives the primary readability win immediately with minimal risk. It is purely a mechanical source split — no logic moves, no API changes, no test changes. This mirrors exactly how `VehicleUtils.cpp` was extracted from `FFBEngine.h`.

Once `GripEstimator` state can be clearly enumerated (all `m_slope_*`, `m_prev_slip_angle[]`, `m_front/rear_grip_smoothed_state`, etc.), Approach B becomes a clean follow-up.

---

## Functions to Move (Approach A)

These 12 functions transfer to `src/GripLoadEstimation.cpp`:

| Function | Lines | Reason |
|----------|-------|--------|
| `calculate_raw_slip_angle_pair` | 122–133 | Slip geometry primitive |
| `calculate_slip_angle` | 135–158 | Slip angle LPF |
| `calculate_grip` | 160–271 | Core grip estimation + fallback |
| `approximate_load` | 273–278 | Simple load fallback |
| `approximate_rear_load` | 280–285 | Simple load fallback |
| `calculate_kinematic_load` | 287–329 | Physics-based load reconstruction |
| `calculate_manual_slip_ratio` | 331–351 | Slip ratio primitive |
| `calculate_slope_grip` | 353–466 | Complex slope-based grip estimator |
| `calculate_slope_confidence` | 468–476 | Slope confidence helper |
| `calculate_wheel_slip_ratio` | 478–485 | Unified slip ratio (used by lockup/spin too) |
| `update_static_load_reference` | 63–91 | Load learning algorithm |
| `InitializeLoadReference` | 93–120 | Load reference seeding |

**Estimated lines removed from `FFBEngine.cpp`:** ~420  
**Resulting `FFBEngine.cpp` size:** ~1080 lines

---

## Additional Suggestions (Future Work)

Beyond the grip/load extraction, the following would further improve `calculate_force` readability:

### 1. Extract snapshot building (lines 963–1035, ~70 lines)

The entire `FFBSnapshot` population block inside `calculate_force` is pure data marshalling — it assigns values to struct fields but contains no FFB logic. Extracting it to a dedicated method eliminates clutter from the main pipeline:

```cpp
void FFBEngine::build_snapshot(const TelemInfoV01* data, const FFBCalculationContext& ctx,
                                double norm_force, double base_input,
                                double grip_factor_applied, float genFFBTorque);
```

### 2. Extract telemetry logging (lines 1038–1106, ~70 lines)

The `AsyncLogger` frame population block is similarly pure marshalling. Extracting it gives:

```cpp
void FFBEngine::log_telemetry_frame(const TelemInfoV01* data, const FFBCalculationContext& ctx,
                                     double norm_force, float genFFBTorque);
```

### 3. Extract missing data detection (lines 715–776, ~80 lines)

Five near-identical hysteresis blocks check for missing telemetry channels (`mSuspForce`, `mSuspensionDeflection`, `mLateralForce` ×2, `mVerticalTireDeflection`). These could be unified with a helper:

```cpp
void FFBEngine::check_missing_channel(double value, double threshold, double speed,
                                       int& frame_counter, bool& warned_flag,
                                       const char* channel_name, const char* vehicle_name);
```
Or extracted into a single `detect_missing_telemetry(data, ctx)` method.

### 4. Address the existing `calculate_soft_lock` TODO

A TODO on line 1431 already acknowledges this:
```
// TODO: move this function to another utils file or other ancillary file,
// since this (soft lock) can be considered not FFB logic.
```

### Combined Future Impact

| Extraction | Lines saved from `calculate_force` |
|------------|-------------------------------------|
| `build_snapshot` | ~70 |
| `log_telemetry_frame` | ~70 |
| `detect_missing_telemetry` | ~80 |
| **Total** | **~220** |

This would reduce `calculate_force` from ~540 lines to approximately **320 lines**, making the core FFB signal chain easy to follow from top to bottom.

---

## Implementation Checklist

- [x] **Phase 1: Grip & Load Estimation Extraction (Approach A)**
  - Moved `calculate_grip`, `calculate_slope_grip`, `calculate_kinematic_load`, etc. to `GripLoadEstimation.cpp`.
  - Reduced `FFBEngine.cpp` complexity and file size.
- [ ] **Phase 2: Extract Snapshot Building**
  - Extract the manual `FFBSnapshot` field assignments from `calculate_force` into a dedicated `build_snapshot` method.
- [ ] **Phase 3: Extract Telemetry Logging**
  - Extract the `AsyncLogger` frame population from `calculate_force` into a dedicated `log_telemetry_frame` method.
- [ ] **Phase 4: Extract Missing Telemetry Detection**
  - Extract the hysteresis-based missing data warnings into a dedicated `detect_missing_telemetry` method or use a unified helper.
- [x] **Phase 5: Modularize Ancillary Effects**
  - Moved `calculate_soft_lock` to `SteeringUtils.cpp` as per the TODO in `FFBEngine.cpp`.
