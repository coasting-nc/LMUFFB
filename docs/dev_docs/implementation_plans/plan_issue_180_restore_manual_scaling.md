# Implementation Plan - Issue #180: Restore Manual Torque Scaling

## Context
The "Session-Learned Dynamic Normalization" introduced in v0.7.67 aimed to automate FFB scaling across different car classes. However, real-world testing revealed significant consistency issues. The system's "Peak Follower" approach is highly reactive to momentary torque spikes (e.g., wall hits or aggressive curbing), which permanently or semi-permanently (due to slow decay) raises the normalization ceiling. This results in "limp" or inconsistent steering weight for the remainder of a session.

Issue #180 and #207 request a return to predictable manual scaling. This plan replaces the automatic peak follower with a user-configurable "Car Max Torque" slider, providing a stable Newton-meter (Nm) baseline for all car-specific scaling while retaining the hardware-specific "Wheelbase Max" and "Target Rim Torque" model.

## Reference Documents
- GitHub Issue #180: "Restore max_torque_ref to scale SteeringShaftTorque until a working solution for auto normalization is found"
- GitHub Issue #207: "Disable by default the Session-Learned Dynamic Normalization for structural forces"
- Discussion Report: `docs/dev_docs/implementation_plans/FFB Strength Normalization Plan Stage 5 - Issues.md`

## Codebase Analysis Summary
### Current Scaling Architecture
The current FFB engine handles two main torque sources with divergent logic:
1.  **100Hz Shaft Torque (`mSteeringShaftTorque`)**: Ingested as raw Nm, then scaled by a volatile `m_session_peak_torque` that is updated frame-by-frame via a leaky integrator.
2.  **400Hz In-Game FFB (`genFFBTorque`)**: Ingested as a normalized percentage `[-1.0, 1.0]`, then scaled by `m_wheelbase_max_nm` to mimic Nm before joining the pipeline.

This "split pipeline" causes the 100Hz path to feel inconsistent while the 400Hz path feels stable. Furthermore, the use of `m_wheelbase_max_nm` for 400Hz ingestion is mathematically questionable as it couples car-physics scaling with user-hardware limits.

### Impacted Functionalities
- **`FFBEngine::calculate_force`**: The core loop must be refactored to unify these paths. Both should be converted to Absolute Nm using a single car-specific reference (`m_car_max_torque_nm`) before any structural effects are added.
- **`Config` / `Preset`**: These structures must be expanded to persist the new toggle and Nm slider. Migration logic is required to ensure existing users transition to a safe "Normalization OFF" state by default.
- **GUI Layer**: The user interface must present these controls clearly, differentiating between "Hardware Scaling" (User's wheel) and "Physics Scaling" (The car).

## FFB Effect Impact Analysis
| FFB Effect | Category | Technical Change | User-Facing Change |
| :--- | :--- | :--- | :--- |
| **Base Steering** | Structural | Converges both 100Hz and 400Hz paths into a unified Absolute Nm pipeline using `m_car_max_torque_nm`. | Steering weight becomes rock-solid and predictable. No more "limp" wheel after crashes. |
| **SoP / Yaw / Gyro** | Structural | Scaled relative to the unified Absolute Nm baseline. | These effects maintain their relative strength to the base steering regardless of car class or driving style. |
| **Tactile Textures** | Tactile | No change. These already operate in absolute Nm (Issue #153). | Consistent haptic intensity across all cars and settings. |
| **Soft Lock** | Texture/Abs | No change. Already moved to absolute Nm group (Issue #181). | Consistent rack-limit resistance. |

## Proposed Changes & Design Rationale

### 1. Unified Scaling Logic (`src/FFBEngine.cpp`)
**Analysis**: The existing Divergent Scaling Problem means that 100Hz and 400Hz torque sources are treated as different physical entities. 100Hz is Nm, 400Hz is a percentage scaled by the user's wheelbase. This makes it impossible to have a consistent "feel" when switching sources. Furthermore, the 100Hz path relies on a "Peak Follower" that can be "poisoned" by single high-torque events like wall hits, leading to permanently reduced force (Issue #175, #180).

**Solution**: The core improvement is the transition from "Learned Peak" to "Manual Reference".
- **400Hz Ingestion**: Instead of scaling by `m_wheelbase_max_nm`, we will scale the normalized `genFFBTorque` by `m_car_max_torque_nm`. This correctly interprets the game's intent (percentage of car's peak) as a physical force. It decouples the car physics from the user's hardware.
- **Dynamic Selection**: A new toggle `m_dynamic_normalization_enabled` will select between the legacy `m_session_peak_torque` (for those who prefer the experimental automatic approach) and the new manual `m_car_max_torque_nm`.
- **EMA Stability**: The structural gain multiplier (`m_smoothed_structural_mult`) will now smoothly transition when the user adjusts the Car Torque slider or toggles normalization. By applying the EMA to the *target* multiplier, we ensure that the force response relaxes or tightens over ~250ms rather than snapping, which protects hardware and provides a better user experience.

### 2. Configuration & Persistence (`src/Config.h` / `src/Config.cpp`)
**Analysis**: Users need per-car control over scaling because a GT3 car and a Hypercar have very different steering rack forces. The previous global `max_torque_ref` was a "hack" that users often set to 100 Nm to stop the app from interfering, which broke the Physical Target Model.

**Solution**:
- **Preset Expansion**: `dynamic_normalization_enabled` and `car_max_torque_nm` will be added to the `Preset` struct. This allows users to save "20 Nm" for their GT3 preset and "45 Nm" for their Hypercar preset, achieving true 1:1 scaling for both.
- **Validation**: `Preset::Validate()` is updated to ensure `car_max_torque_nm` is never below 1.0 Nm. This is a critical reliability check; if a user manually edits their INI to 0.0, the resulting infinite gain would likely trigger hardware safety or cause a crash.
- **Migration Logic**: In `Config::Load`, we will explicitly default the normalization toggle to `false` for any config file that doesn't have it. This fulfills the requirement of Issue #207 (Disable by default) while maintaining backward compatibility for users who haven't updated their configs yet.

### 3. User Interface (`src/GuiLayer_Common.cpp`)
**Analysis**: The relationship between Wheelbase Torque, Target Rim Torque, and Car Physics Torque is complex. The previous UI didn't distinguish between "What my wheel can do" and "What the car is doing".

**Solution**:
- A new **"Scaling & Normalization"** group will be added to the General FFB section. This centralizes all scaling parameters.
- **Car Max Torque (Nm)** slider: Range 1-100 Nm. This is the "Master Key" for 1:1 scaling. If set to 20 Nm, and the car produces 20 Nm, the app knows the car is at its limit.
- **Enable Dynamic Normalization** checkbox: Clearly labeled to inform users that it is a legacy/experimental feature. Showing the "Live Session Peak" only when enabled keeps the UI clean.

## Instructions Clarity & Future Improvements
**Analysis of Instructions**: The instructions provided in `A.1_architect_prompt.md` and the general "Fixer" role mandate a focus on "what" and "how" (the implementation steps and toolkit), but they were initially perceived as less explicit about requiring a deep "why" (physics/design rationale) for *every* subsection of the plan.

**Suggested Improvement**: The plan template should include a specific `### Design Rationale` header under each `### Proposed Changes` section. This would force the Architect to explicitly document the reasoning behind each architectural choice, bridging the gap between technical implementation and physical requirements.

### 4. Versioning
**Analysis**: This change alters the fundamental way forces are scaled in the application, potentially affecting every car and hardware combination. It represents a significant architectural pivot from "Automatic/Intelligent" back to "Manual/Predictable" control.
**Solution**: Increment version to `0.7.109`. This clearly marks the transition in the changelog and ensures that any bug reports following this update can be immediately identified as part of the new scaling regime.

## Test Plan (TDD-Ready)
**Analysis**: Testing must prove that the unified pipeline correctly handles both raw Nm (100Hz) and percentage (400Hz) signals, and that the manual car torque reference is correctly applied in both cases. We must also verify that the EMA smoothing doesn't introduce instabilities during state transitions.

### Manual Scaling Verification
- **Test Case 1 (100Hz)**: Verify that with normalization OFF and car torque set to 20Nm, a 10Nm input produces exactly 50% structural output. This confirms the manual baseline is correctly used as a denominator.
- **Test Case 2 (400Hz)**: Verify that with normalization OFF and car torque set to 30Nm, a 50% game signal produces 15Nm internally, resulting in 50% structural output. This confirms the new 400Hz ingestion logic correctly interprets percentage as car-relative force.

### State Transition Verification
- **Test Case 3**: Verify that toggling normalization ON/OFF correctly switches the source of the `structural_mult` between `m_session_peak_torque` and `m_car_max_torque_nm`. This ensures the state machine logic in `calculate_force` is robust.

### Persistence Verification
- **Test Case 4**: Ensure new parameters are correctly round-tripped through the `config.ini` file and that the migration logic correctly defaults to the safe (Manual) state. This is critical for ensuring no users are surprised by "limp" steering after updating.

## Instructions Clarity & Future Improvements
The instructions in `A.1_architect_prompt.md` and the "Fixer" role guidelines could be improved by explicitly requiring a "Rationale and Analysis" subsection for each major change. Currently, the templates focus heavily on "What" (the list of changes) and "How" (the technical steps), but less on "Why" (the physics-based or reliability-based reasoning).

**Suggested Improvement**: Add a mandatory "Design Rationale" block to each section in the implementation plan template to force explicit documentation of the architect's reasoning.

## Deliverables
- [x] Modified `src/FFBEngine.h` and `src/FFBEngine.cpp`
- [x] Modified `src/Config.h` and `src/Config.cpp`
- [x] Modified `src/GuiLayer_Common.cpp`
- [x] Modified `VERSION` and `src/Version.h`
- [x] Modified `CHANGELOG_DEV.md`
- [x] New test file `tests/test_ffb_normalization_toggle.cpp`
- [x] Updated `tests/CMakeLists.txt`
- [x] Updated Implementation Plan (this document) with analytical notes.

## Implementation Notes

### Unforeseen Issues
- **Legacy Test Regression**: Many existing unit tests (e.g., `test_dynamic_weight_scaling`, `test_grip_modulation`) were authored assuming the "Session-Learned Dynamic Normalization" was always active. When I disabled it by default in `FFBEngine`, these tests began producing incorrect normalized values.
- **Solution**: I updated `InitializeEngine()` in `tests/test_ffb_common.cpp` to explicitly set `m_dynamic_normalization_enabled = true`. This preserves the historical baseline for legacy tests while allowing new tests to verify the manual scaling path.

### Plan Deviations
- **Initialize session peak in Config.h**: Added logic to `Preset::Apply` to initialize `m_session_peak_torque` from `m_car_max_torque_nm`. This ensures that if a user *does* enable dynamic normalization mid-session, the learner starts at their manually defined baseline rather than the hardcoded 25Nm default, preventing a sudden "jolt" or "dip" in force.
- **Epsilon Protection**: Standardized the use of `EPSILON_DIV` (1e-9) in the structural multiplier calculation to guarantee mathematical stability even if a user manages to set `m_car_max_torque_nm` to zero (though validation should prevent this).

### Challenges
- **Unifying Ingestion**: The most challenging aspect was ensuring that the 400Hz `genFFBTorque` signal (which is a percentage) and the 100Hz `mSteeringShaftTorque` (which is Nm) arrived at the same point in the pipeline with identical physical units. By scaling the 400Hz signal by `m_car_max_torque_nm` early, I was able to treat the entire remainder of the structural group as Absolute Nm, simplifying all subsequent effect calculations (SoP, Yaw, etc.).

### Recommendations
- **Auto-Calculated Presets**: In the future, we could add a "Detect Car Torque" button in the GUI that sets `m_car_max_torque_nm` based on a momentary peak hold, similar to iRacing's "Auto" button, instead of relying on continuous background learning.
