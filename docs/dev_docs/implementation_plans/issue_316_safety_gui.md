# Implementation Plan - Issue #316: Safety fixes for FFB Spikes (3) - Add a new Safety section in the GUI

This plan outlines the steps to make FFB safety parameters user-configurable via the GUI and persistent through the configuration system.

## Context
Following the implementation of advanced safety features in Issue #314, users may need to tune these parameters based on their specific wheelbase hardware (e.g., Direct Drive vs. Belt Drive) and personal preference. Hardcoded constants prevent this flexibility.

**Issue:** [Safety fixes for FFB Spikes (3) - Add a new Safety section in the GUI to adjust safety features for FFB spikes #316](https://github.com/coasting-nc/LMUFFB/issues/316)

## Design Rationale
The "Safety First" architecture remains the core principle. By exposing these parameters, we empower users to find the optimal balance between protection and FFB fidelity. The configuration system must be updated to ensure these settings are saved per-preset, allowing different safety profiles for different cars or hardware.

## Reference Documents
- `docs/dev_docs/implementation_plans/issue_314_safety_fixes_2.md`: Most recent safety logic implementation.
- `src/FFBEngine.h`: Current hardcoded constants.

## Codebase Analysis Summary
- **FFBEngine**: Stores safety state in `SafetyMonitor` and uses hardcoded static constants for thresholds and window durations.
- **Config**: Manages `Preset` structures and serialization to `config.ini`.
- **GuiLayer_Common**: Implements the user interface using ImGui and `GuiWidgets`.

### Impacted Functionalities:
- **Safety Logic**: Transitions from constants to member variables.
- **Persistence**: New entries in `config.ini` and `Preset` management.
- **GUI**: New "FFB Safety" section in the tuning window.

### Design Rationale:
Exposing these parameters as member variables allows for real-time adjustment without recompilation. Integration into the `Preset` system ensures that safety settings can be tailored to specific vehicle classes or hardware configurations.

## FFB Effect Impact Analysis
| Effect | Technical Changes | User Perspective |
| :--- | :--- | :--- |
| **All Effects** | Safety mitigation (gain reduction, smoothing, slew limiting) now driven by configurable member variables. | Users can adjust how "intrusive" the safety mode is and how sensitive the spike detection should be. |

### Design Rationale:
Giving users control over these parameters allows them to relax the safety mode if they find it too aggressive, or tighten it further for extremely powerful wheelbases.

## Proposed Changes

1.  **Modify `src/FFBEngine.h` to convert constants to member variables.**
    - Convert `SAFETY_WINDOW_DURATION`, `SAFETY_GAIN_REDUCTION`, `SAFETY_SMOOTHING_TAU`, `SPIKE_DETECTION_THRESHOLD`, `IMMEDIATE_SPIKE_THRESHOLD`, and `SAFETY_SLEW_FULL_SCALE_TIME_S` to public member variables (with `m_` prefix).
    - Add `m_stutter_safety_enabled` (bool) and `m_stutter_threshold` (float) to control FFB disabling during lost frames.
    - **Design Rationale:** Public access allows the `Preset` system and GUI to modify these values directly (protected by `g_engine_mutex` where necessary).

2.  **Update `src/FFBEngine.cpp` and `src/main.cpp` to use member variables.**
    - Replace all usages of the old static constants with the new member variables in `FFBEngine.cpp`.
    - Update `src/main.cpp` to use `m_stutter_safety_enabled` and `m_stutter_threshold` in the "LOST FRAME DETECTION" block.
    - **Design Rationale:** Ensures the physics engine and main loop react immediately to GUI adjustments.

3.  **Update Configuration System in `src/Config.h` and `src/Config.cpp`.**
    - Add safety parameters to the `Preset` struct.
    - Update `Preset::Apply()`, `Preset::UpdateFromEngine()`, and `Preset::Validate()` to handle the new settings.
    - Update `Config::ParsePresetLine()` and `Config::WritePresetFields()` for `config.ini` persistence.
    - **Parameter Synchronization Checklist:**
        - [x] Declaration in `FFBEngine.h`
        - [x] Declaration in `Preset` struct (`Config.h`)
        - [x] Entry in `Preset::Apply()`
        - [x] Entry in `Preset::UpdateFromEngine()`
        - [x] Entry in `Config::Save()`
        - [x] Entry in `Config::Load()`
        - [x] Validation logic in `Preset::Validate()` and `Config::Load()`

4.  **Enhance User Interface.**
    - Add descriptive tooltips to `src/Tooltips.h`.
    - Implement the "FFB Safety" section in `src/GuiLayer_Common.cpp` using `GuiWidgets`.
    - Include a checkbox for "Safety on Stuttering" and a "Stutter Threshold" slider.
    - **Design Rationale:** Follows the established UI pattern for consistency.

5.  **Include Issue Documentation.**
    - Create `docs/issue_316_description.md` with the full text of the GitHub issue for transparency.

6.  **Version Increment.**
    - Increment version to `0.7.165`.

## Test Plan (TDD-Ready)
1.  **Unit Test: `test_safety_config_persistence`**
    - Modify safety settings in `FFBEngine`.
    - Save to a temporary config file.
    - Load back and verify values match.
2.  **Unit Test: `test_safety_preset_application`**
    - Create a custom preset with specific safety values.
    - Apply preset to `FFBEngine`.
    - Verify `FFBEngine` member variables match.
3.  **Unit Test: `test_safety_validation`**
    - Pass invalid values (e.g., negative duration, 0 gain reduction) and verify they are clamped to safe ranges.

### Design Rationale (Test Plan):
These tests ensure that the bridge between the GUI, the config file, and the physics engine is robust and that user errors in manual config editing are handled gracefully.

## Deliverables
- [x] Modified `FFBEngine.h/cpp`
- [x] Modified `Config.h/cpp`
- [x] Modified `Tooltips.h` and `GuiLayer_Common.cpp`
- [x] New unit tests in `tests/test_issue_316_safety_gui.cpp`
- [x] Updated `VERSION` and `CHANGELOG_DEV.md`
- [x] Updated Implementation Notes in this plan.

## Implementation Notes
- **Parameter Synchronization**: Successfully exposed 6 safety parameters across `FFBEngine`, `Preset`, and `Config` serialization.
- **GUI Integration**: Placed the new "FFB Safety Features" section before "Load Forces" for better visibility.
- **Thread Safety**: Implemented `g_engine_mutex` protection for GUI settings updates using a lambda decorator in `GuiLayer_Common.cpp`.
- **Validation**: Added comprehensive clamping in `Config.cpp` and `Preset::Validate()` to ensure numerical stability.
- **Testing**: New tests in `tests/test_issue_316_safety_gui.cpp` verify the full chain of defaults, application, and persistence, including stuttering settings.
- **Deviations**: None. All requirements met according to the plan.

## Code Review & Refinement
### Iteration 1
- **Status**: Mostly Correct
- **Feedback**: Missing `VERSION`, `CHANGELOG_DEV.md`, and process documentation. Redundant logging in `Config.cpp`.
- **Action**: Added administrative files. Cleaned up logging. Improved variable initialization using `DEFAULT_` constants. Added thread-safety decorators to GUI widgets.
### Iteration 2
- **Status**: Mostly Correct
- **Feedback**: Missing stuttering safety checkbox and threshold slider. Missing .md copy of the issue.
- **Action**: Added `m_stutter_safety_enabled` and `m_stutter_threshold` to `FFBEngine` and `Preset`. Updated GUI with new controls. Created `docs/issue_316_description.md`.
### Iteration 3
- **Status**: Correct
- **Feedback**: Functional and administrative requirements met. Minor initialization inconsistency and numbering error in plan.
- **Action**: Corrected stuttering variable initialization in `FFBEngine.h` to use `DEFAULT_` constants. Fixed numbering in implementation plan.
### Iteration 4
- **Status**: Greenlight
- **Feedback**: All issues resolved.
- **Action**: Finalized documentation.
