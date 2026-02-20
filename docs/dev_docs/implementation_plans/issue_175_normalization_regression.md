# Implementation Plan - Issue #175: Verify if the normalization has introduced regressions for the 100 Hz SteeringTorque force

## 1. Analysis of the Issue
The Dynamic FFB Normalization introduced in v0.7.67 (Stage 1) uses a session-learned peak torque (`m_session_peak_torque`) to normalize structural forces for the 100Hz legacy torque source.

### Problem Symptoms:
- **Limpness**: Steering feels extremely weak, especially after high-torque events (like hitting a wall or a high-G corner) which push the session peak reference very high.
- **Imbalance**: Since tactile textures (road noise, ABS, etc.) are rendered in absolute Nm and NOT normalized by the session peak, they feel relatively much stronger than the steering when normalization has shrunk the steering forces.
- **400Hz works better**: The 400Hz native source path uses `m_wheelbase_max_nm` as its reference instead of the session peak, resulting in predictable absolute scaling that aligns with user expectations for "Physical Target" feel.

### Root Cause:
The 100Hz path forces everything to be relative to the highest torque seen in the session. This is a radical change from the previous "absolute" feel and causes significant force loss as the peak grows.

---

## 2. Proposed Changes

### A. Core Physics Logic (`src/FFBEngine.cpp`, `src/FFBEngine.h`)
1.  **Introduce Toggle**: Add `m_dynamic_normalization_enabled` (bool) to allow users to opt-out of the session-learned normalization.
2.  **Conditional Normalization**:
    - If enabled: Continue using `m_session_peak_torque` for 100Hz.
    - If disabled (or for 400Hz): Use `m_wheelbase_max_nm` as the reference.
3.  **Default Value**: Default to `false` to restore the predictable FFB feel while allowing enthusiasts to experiment with dynamic scaling.

### B. Configuration & Presets (`src/Config.cpp`, `src/Config.h`)
1.  **Persist Setting**: Update `Config::Save`, `Config::Load`, and `Preset` struct to handle `dynamic_normalization_enabled`.
2.  **INI Key**: Add `dynamic_normalization_enabled=0` to `config.ini`.

### C. GUI Layer (`src/GuiLayer_Common.cpp`)
1.  **Add Toggle**: Add a "Enable Dynamic Normalization (Session Peak)" checkbox in the General FFB or Tuning section.
2.  **Tooltips**: Explain that disabling this restores absolute force scaling (recommended for consistent feel).

---

## 3. Verification Plan

### A. Automated Tests
1.  **Reproduction Test**: Create a test in `tests/test_ffb_normalization_regression.cpp` that:
    - Sets up a 100Hz source.
    - Simulates a 10Nm corner -> verifies output.
    - Simulates a 50Nm spike -> verifies `m_session_peak_torque` grew.
    - Simulates a 10Nm corner again -> verifies output is now significantly "limp".
2.  **Fix Verification**:
    - Disable dynamic normalization in the test.
    - Repeat the steps and verify that the output remains consistent regardless of the 50Nm spike.
3.  **Persistence Test**: Ensure the toggle saves and loads correctly from `config.ini`.

### B. Manual Verification (via Static Analysis)
- Verify that `m_smoothed_structural_mult` behaves correctly when the toggle is flipped.
- Ensure 400Hz path remains unaffected and correct.

---

## 4. Implementation Steps

1.  **Modify `FFBEngine.h`**: Add `m_dynamic_normalization_enabled`.
2.  **Modify `Config.h`**: Add `dynamic_normalization_enabled` to `Preset` struct.
3.  **Modify `FFBEngine.cpp`**:
    - Update `calculate_force` to use the toggle.
    - Update `calculate_force` to select reference (`m_session_peak_torque` vs `m_wheelbase_max_nm`) based on toggle and source.
4.  **Modify `Config.cpp`**: Update Save/Load and Preset mappings.
5.  **Modify `GuiLayer_Common.cpp`**: Add the checkbox.
6.  **Update `VERSION` and `CHANGELOG_DEV.md`**.

## 5. Implementation Notes
- **Limpness Confirmed**: The regression test `tests/test_ffb_normalization_regression.cpp` demonstrated that a 100Hz legacy source experienced a ~50% force reduction when the session peak torque grew from 20Nm to 40Nm.
- **Consistency Restored**: Disabling dynamic normalization (the new default) correctly restores absolute force scaling, making the 100Hz source behave predictably and matching the 400Hz source's behavior.
- **Thread Safety**: The GUI toggle implementation uses a local boolean and `std::lock_guard<std::recursive_mutex>` to ensure thread-safe updates to the engine state.
- **Backward Compatibility**: Existing tests for dynamic normalization were updated to explicitly enable the feature, ensuring the regression suite remains robust.
