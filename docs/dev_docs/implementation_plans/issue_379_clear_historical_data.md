# Implementation Plan - Issue #379: Clear historical data across context changes

This plan addresses Issue #379, which involves investigating and fixing issues where historical data is not correctly cleared across context changes such as switching cars or teleporting from the garage to the track.

## Context
In a high-frequency (400Hz) stateful physics engine, failing to reset historical states when the physical context changes leads to "math explosions" (discontinuous derivatives), NaN propagation, and violent FFB jolts.

**Issue Number:** #379
**Issue Title:** Investigate, verify and fix issue due to failing to clear historical data across context changes (like switching cars, or teleporting from the garage to the track).

## Analysis
The issue description identifies 4 specific bugs:
1. **Slope Detection Buffers:** Circular buffers used for Savitzky-Golay derivatives are not cleared on car change.
2. **REST API Steering Range:** `m_fallbackRangeDeg` carries over between cars if they both require REST API fallback.
3. **Teleport Derivative Spikes:** Previous frame data (`m_prev_*`) is not re-seeded when transitioning from "unallowed" (Garage/Menu) to "allowed" (Driving) state.
4. **Incomplete Normalization Reset:** `ResetNormalization()` misses `m_static_rear_load` and `m_last_raw_torque`.

### Design Rationale
- **Slope Detection:** These buffers must be cleared to prevent the derivative from being calculated across a car-switch boundary.
- **REST API:** Clearing the fallback range ensures that the system doesn't use the old car's range while waiting for the new one.
- **Teleport:** Re-seeding derivatives on state transition prevents the first frame after teleport from having an astronomical delta.
- **Normalization:** A clean slate for normalization is essential for consistent gain and spike rejection behavior.

## Proposed Changes

### 1. `src/physics/GripLoadEstimation.cpp`
- Update `FFBEngine::InitializeLoadReference` to reset slope detection buffers and state.

### 2. `src/io/RestApiProvider.h`
- Add `ResetSteeringRange()` method to clear `m_fallbackRangeDeg`.

### 3. `src/ffb/FFBMetadataManager.cpp`
- Update `FFBMetadataManager::UpdateInternal` to call `RestApiProvider::Get().ResetSteeringRange()` when a car change is detected.

### 4. `src/ffb/FFBEngine.h`
- Add `bool m_derivatives_seeded` member variable.

### 5. `src/ffb/FFBEngine.cpp`
- In `calculate_force`, set `m_derivatives_seeded = false` when transitioning from allowed to unallowed.
- In `calculate_force`, re-seed derivative variables if `!m_derivatives_seeded`.
- Update `ResetNormalization()` to include `m_last_raw_torque` and `m_static_rear_load`.

## Test Plan
- **Unit Tests:**
    - Create a test case that simulates a car change and verifies that slope buffers and REST API range are reset.
    - Create a test case that simulates a "Garage to Track" transition (allowed flag flip) and verifies that `m_derivatives_seeded` is reset and subsequently re-seeded, preventing spikes.
    - Verify `ResetNormalization()` resets all intended fields.
- **Linux Verification:**
    - Run existing tests to ensure no regressions.
    - Compile with `cmake --build build`.

### Design Rationale
Using unit tests to simulate these transitions is the only way to verify these fixes without the actual game. We can mock the `TelemInfoV01` data to trigger these conditions.

## Implementation Notes
### Encountered Issues
- The `m_static_rear_load` reset value in `ResetNormalization` needs to match how it's initialized in `InitializeLoadReference` to avoid test failures. It was later found that `ResetNormalization()` was missing the restoration of the saved rear load from config, which was subsequently fixed.
- Friend class declarations in `FFBEngine.h` needed careful ordering with forward declarations of the test access classes.
- Some variables like `m_prev_susp_force` were only being seeded for front wheels (indices 0 and 1) in the original issue description; updated to seed all 4 wheels for consistency.
- **Test Suite Overhaul**: The implementation of the 'Seeding Gate' causes the first frame of every session to return zero force. This is an intentional safety feature but broke approximately 16 legacy unit tests that assumed immediate force calculation. I systematically updated the test suite to include warmup frames or explicit state seeding to restore full coverage.

### Deviations from the Plan
- Updated `m_prev_susp_force` and `m_prev_rotation` and other `m_prev_*` arrays to seed all 4 wheels where applicable, even if current effects only use a subset.
- Added explicit `g_engine_mutex` locking in the new regression test to match engine thread-safety standards.
- **Stats Latching Relocation**: Relocated Torque and Grip statistics latching to the start of `calculate_force` to ensure a consistent interval-based reset state for unit testing.
- **Derived Accel Compatibility (REVERTED)**: Initially added a hack to prefer raw inputs if they are non-zero and upsampling is inactive, providing compatibility for legacy tests that manually inject acceleration. This was removed after code review because it undermined safety for 100Hz legacy mode users. Instead, the legacy tests in `tests/test_issue_325_longitudinal_g.cpp` were properly updated to simulate velocity over time to trigger the correct derived acceleration math.

### Suggestions for the Future
- Consider a unified `State::Reset()` method for various physics sub-modules to avoid scattered reset logic in `FFBEngine`.

## Additional Questions
- Are there any other historical variables in `UpSampler` that should be reset? (The issue mentions `m_upsample_shaft_torque.Reset()`, which is already there).
