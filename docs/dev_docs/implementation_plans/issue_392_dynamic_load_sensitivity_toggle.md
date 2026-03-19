# Implementation Plan - Issue #392: Add GUI checkbox to disable Dynamic Load Sensitivity

Add a GUI option (checkbox) to disable the addition of Dynamic Load Sensitivity to Tire Grip Estimation, so that users with direct drive wheelbases can do tests with and without it enabled, and see if this is introducing any graininess.

## Design Rationale
- **Reliability:** Providing a toggle allows users to revert to legacy-style (static) grip estimation if they experience unwanted tactile artifacts ("graininess") on high-end hardware.
- **Physics-Based Reasoning:** Dynamic load sensitivity scales the optimal slip angle based on vertical load ($F_z^{0.333}$), which is physically accurate for tires. However, if vertical load telemetry is noisy, it can manifest as graininess in the FFB. Disabling it provides a constant optimal slip angle baseline.
- **Architectural Integrity:** The toggle is integrated into the `FFBEngine` state, `Preset` system, and `Config` serialization to ensure it behaves like any other high-level physics setting.

---

## 1. Codebase Analysis Summary

### Affected Modules:
1.  **`FFBEngine` (Physics Core):** Needs a new state variable to control the logic branch.
2.  **`GripLoadEstimation` (Physics implementation):** The specific math for load sensitivity is located here.
3.  **`Config` / `Preset` (System):** For persistence and preset management.
4.  **`GuiLayer` (UI):** For user visibility and control.

### Impact Zone:
The primary impact is within the `calc_wheel_grip` lambda inside `FFBEngine::calculate_axle_grip`. This lambda is responsible for the fallback grip estimation used when telemetry data (like `mGripFract`) is missing or encrypted.

---

## 2. FFB Effect Impact Analysis

| Effect | Technical Changes | User-Facing Impact |
| :--- | :--- | :--- |
| **Understeer** | `dynamic_slip_angle` calculation will be branched. | If disabled, the "cliff edge" of grip loss remains at a fixed steering angle regardless of car load. |
| **Slide Texture** | Uses grip factor to scale vibration. | Texture might feel smoother if vertical load noise was being amplified by load sensitivity. |
| **Oversteer Boost** | Uses axle grip differential. | Minimal, but ensures consistency between front/rear axles if both use the toggle. |

### Design Rationale
The toggle specifically targets the Hertzian scaling part of the friction circle logic. By disabling it, we maintain the friction circle math but remove the load-dependent shift of the optimal slip point.

---

## 3. Proposed Changes

### Parameter Synchronization Checklist
For `load_sensitivity_enabled`:
- [x] Declaration in `FFBEngine.h` (`m_load_sensitivity_enabled`)
- [x] Declaration in `Preset` struct (`Config.h`)
- [x] Entry in `Preset::Apply()`
- [x] Entry in `Preset::UpdateFromEngine()`
- [x] Entry in `Config::Save()`
- [x] Entry in `Config::Load()` / `SyncPhysicsLine`
- [x] Validation logic (boolean, no complex clamping needed)

### Versioning
- Version numbers in `VERSION` and `src/core/Version.h.in` (if applicable) will be incremented by the smallest possible increment.

---

## 4. Test Plan (TDD-Ready)

### New Tests: `tests/test_issue_392.cpp`
- **test_issue_392_default_value:** Verify `FFBEngine` defaults to `true`.
- **test_issue_392_physics_toggle_impact:**
    - Set load sensitivity `ON`.
    - Run `calculate_force` at 1500N load and 6000N load.
    - Assert that grip values differ (`ASSERT_GT`).
    - Set load sensitivity `OFF`.
    - Run `calculate_force` at 1500N load and 6000N load.
    - Assert that grip values are near identical (`ASSERT_NEAR`).
- **test_issue_392_persistence:**
    - Verify `Preset::UpdateFromEngine` and `Preset::Apply` correctly sync the value.
    - Verify `Preset::Equals` handles the new field.

---

## 5. Implementation Notes

### Unforeseen Issues
- Initial build attempt timed out due to standard sandbox environment constraints during full compilation. Incremental builds was used instead.
- The `doctest.h` framework was not available; switched to the project's custom `test_ffb_common.h` framework.

### Plan Deviations
- Added `std::lock_guard` to the GUI checkbox callback specifically to satisfy user feedback regarding `g_engine_mutex` usage and to ensure thread-safe config saving.

### Recommendations
- Continue with 400Hz upsampling aware tests as recently introduced in Issue #397.
- Consider refactoring the `BoolSetting` local lambda in `GuiLayer_Common.cpp` to accept a callback for `g_engine_mutex` locking to avoid manual boiler-plate for new toggles.
