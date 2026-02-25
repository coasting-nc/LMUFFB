# Implementation Plan - Restore Manual Torque Scaling for 100Hz Source (#180)

**Issue:** #180 - Restore max_torque_ref to scale SteeringShaftTorque until a working solution for auto normalization is found.
**Related:** #175 - Verify if the normalization has introduced regressions for the 100 Hz SteeringTorque force.

## 1. Analysis
The introduction of Dynamic Normalization (Stage 1) in v0.7.67 replaced the fixed `max_torque_ref` scaling for the 100Hz `mSteeringShaftTorque` source with a session-learned `m_session_peak_torque`. While this helps equalize different car classes, it can lead to:
- "Limp" steering if a high-torque spike (e.g., a wall hit) causes the learned peak to stay too high.
- Inconsistent feel at the start of a session while the engine is still learning.
- Loss of absolute physical scaling that some users prefer.

The goal is to provide a toggle to disable Dynamic Normalization for the 100Hz source and return to a fixed scaling reference. Based on the Stage 2 refactor (v0.7.68), `m_wheelbase_max_nm` now serves as the logical equivalent of the old `max_torque_ref` for manual scaling.

## 2. Proposed Changes

### 2.1 FFBEngine (src/FFBEngine.h & src/FFBEngine.cpp)
- Add `bool m_dynamic_normalization_enabled` to `FFBEngine` class.
- Update `calculate_force` in `src/FFBEngine.cpp`:
  - Only update `m_session_peak_torque` if `m_dynamic_normalization_enabled` is `true`.
  - Calculate `target_structural_mult` based on the toggle:
    - If `m_torque_source == 0` (Legacy 100Hz) AND `m_dynamic_normalization_enabled` is `false`:
      - Use `1.0 / (m_wheelbase_max_nm + 1e-9)`.
    - Else (Dynamic enabled or 400Hz source):
      - Use current logic (learned peak for 100Hz, wheelbase max for 400Hz).

### 2.2 Configuration (src/Config.h & src/Config.cpp)
- Add `bool dynamic_normalization_enabled` to `Preset` struct (default `true` to maintain current behavior, or `false` to match the issue's request for 100Hz users). *Decision: Default to `false` for 100Hz users as requested by the issue context, or keep `true` for general consistency but allow override.* The issue says "until a working solution... is found", suggesting it might be better off by default for now. However, to avoid breaking current users who like it, I will default to `true` but ensure it's easy to toggle.
- Actually, the memory mentioned it should be `false` by default for 100Hz.
- Update `Preset::Apply` and `Preset::UpdateFromEngine`.
- Update `Config::Save` and `Config::Load` to persist the setting.
- Add migration logic if necessary (though not strictly needed as it's a new boolean).

### 2.3 User Interface (src/GuiLayer_Common.cpp)
- Add a checkbox: "Enable Dynamic Normalization (100Hz Legacy)" in the "General FFB" or "Internal Physics" section.
- Add a tooltip explaining what it does.

### 2.4 Versioning
- Increment `VERSION` to `0.7.80`.
- Update `CHANGELOG_DEV.md`.

## 3. Verification Plan

### 3.1 Automated Tests
- Create a new test file `tests/test_ffb_normalization_toggle.cpp`.
- **Test Case 1**: Verify that with `m_dynamic_normalization_enabled = true`, `m_session_peak_torque` updates when a high torque is applied.
- **Test Case 2**: Verify that with `m_dynamic_normalization_enabled = false`, `m_session_peak_torque` does NOT update.
- **Test Case 3**: Verify that with `m_dynamic_normalization_enabled = false` and `m_torque_source = 0`, the force scaling uses `m_wheelbase_max_nm`.
- **Test Case 4**: Verify that with `m_dynamic_normalization_enabled = false` and `m_torque_source = 1`, the force scaling REMAINS based on `m_wheelbase_max_nm` (no change for 400Hz).

### 3.2 Manual Verification (Static Analysis)
- Ensure no `NaN` or `Inf` can be introduced by the division.
- Ensure thread safety via `g_engine_mutex` when the GUI toggles the setting.

## 4. Implementation Notes (Post-Dev)
The feature has been successfully implemented and verified.
- **Toggle Added**: `m_dynamic_normalization_enabled` now allows opting out of Stage 1 normalization.
- **Thread Safety**: UI updates to the toggle are protected by `g_engine_mutex` to prevent race conditions with the FFB thread.
- **Fixed Reference**: When disabled, the 100Hz source uses `m_wheelbase_max_nm` as the fixed Nm reference. This matches the physical interpretation of the legacy `max_torque_ref`.
- **Regression Guard**: A new unit test `tests/test_ffb_normalization_toggle.cpp` confirms that the peak learner is gated and that force scaling correctly switches between dynamic and fixed modes.
- **Usability**: Tooltips were updated to clearly explain the interaction between these parameters.
