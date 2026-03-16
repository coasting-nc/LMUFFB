# Implementation Plan - Issue #374: Fix: reset data missing detection logic when changing cars

## 1. Context
When switching cars in LMU, some telemetry data might be available for one car but missing for another (e.g., encrypted/DLC content vs base content). The FFB engine uses counters and warning flags to detect missing data and switch to fallback estimation models. Currently, these counters and flags are not reset when switching cars, which can lead to persistent warnings or incorrect behavior (sticking in fallback mode) even when the new car has the necessary telemetry.

**GitHub Issue:** [Issue #374: Fix: reset data missing detection logic when changing cars](https://github.com/coasting-nc/LMUFFB/issues/374)

> **Design Rationale:**
> Resetting these states on car change ensures that the engine's diagnostic state is clean for each car, preventing "state leakage" between sessions with different vehicles. This is critical for reliability and correct physics fallback application.

## 2. Analysis
The following variables in `FFBEngine` need to be reset:

**Counters:**
- `m_missing_load_frames`
- `m_missing_lat_force_front_frames`
- `m_missing_lat_force_rear_frames`
- `m_missing_susp_force_frames`
- `m_missing_susp_deflection_frames`
- `m_missing_vert_deflection_frames`

**Warning Flags:**
- `m_warned_load`
- `m_warned_grip`
- `m_warned_rear_grip`
- `m_warned_lat_force_front`
- `m_warned_lat_force_rear`
- `m_warned_susp_force`
- `m_warned_susp_deflection`
- `m_warned_vert_deflection`

The `FFBEngine::InitializeLoadReference` function in `src/physics/GripLoadEstimation.cpp` is the ideal place for this reset, as it is triggered by `FFBMetadataManager` whenever a car change is detected.

> **Design Rationale:**
> `InitializeLoadReference` already handles other car-specific resets like normalization and static load seeding. Adding telemetry diagnostic resets here consolidates car-change logic and ensures thread safety via `g_engine_mutex`.

## 3. Proposed Changes

### `src/physics/GripLoadEstimation.cpp`
Modify `FFBEngine::InitializeLoadReference` to reset the counters and flags mentioned above.

```cpp
void FFBEngine::InitializeLoadReference(const char* className, const char* vehicleName) {
    std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);

    // v0.7.109: Perform a full normalization reset on car change
    ResetNormalization();

    // --- FIX #374: Reset all missing telemetry counters and warning flags ---
    m_metadata.ResetWarnings();
    m_missing_load_frames = 0;
    m_missing_lat_force_front_frames = 0;
    m_missing_lat_force_rear_frames = 0;
    m_missing_susp_force_frames = 0;
    m_missing_susp_deflection_frames = 0;
    m_missing_vert_deflection_frames = 0;

    m_warned_load = false;
    m_warned_grip = false;
    m_warned_rear_grip = false;
    m_warned_lat_force_front = false;
    m_warned_lat_force_rear = false;
    m_warned_susp_force = false;
    m_warned_susp_deflection = false;
    m_warned_vert_deflection = false;
    // -----------------------------------------------------------------------

    // ... existing logic ...
}
```

> **Design Rationale:**
> Explicit zeroing of counters and setting flags to `false` ensures a deterministic start for the next car's diagnostic phase.

## 4. Test Plan

### Automated Regression Test
Create a new test file `tests/test_issue_374_repro.cpp` that:
1. Initializes `FFBEngine`.
2. Simulates frames with missing `mTireLoad` (sets `mTireLoad = 0.0`) while the car is moving.
3. Verifies that `m_missing_load_frames` increases and eventually `m_warned_load` becomes `true`.
4. Calls `calculate_force` with a different `vehicleName` to trigger car change.
5. Verifies that `m_missing_load_frames` is now `0` and `m_warned_load` is `false`.

### Build Verification
- Run `cmake --build build` on Linux.
- Run `./build/tests/run_combined_tests --filter Issue374`.

> **Design Rationale:**
> Unit testing this logic via `calculate_force` provides an end-to-end verification of the car change detection and subsequent state reset.

## 5. Implementation Notes
- **Thread Safety:** Reset logic is protected by `g_engine_mutex` inside `InitializeLoadReference`.
- **Unified Resets:** Moved `m_metadata.ResetWarnings()` to be part of the unified car-change reset block for better visibility.
- **Iterative Reviews:** Early iterations missed `VERSION` and `CHANGELOG_DEV.md` updates; these were corrected in the second iteration. A temporary regression involving accidental deletion of changelog history was also resolved.
- **Verification:** Both the reproduction test and the full test suite (536 tests) pass successfully.

## 6. Additional Questions
- Should `m_warned_dt` also be reset? (Decision: No, it's typically a game-wide or plugin-wide issue, not car-specific).
- Are there any other "warned" flags? (Checked `FFBEngine.h`: `m_warned_dt` and `m_warned_invalid_range` - the latter is reset via `m_metadata.ResetWarnings()`).
