# Implementation Plan - Issue #178: Remove Base Force Mode

## Context
The "Base Force Mode" feature allowed users to switch between "Native" physics-based force and a "Synthetic" mode that provided max wheelbase torque whenever the game's torque exceeded a small deadzone. This feature is being removed to simplify the FFB signal chain and UI, as it is rarely used and deviates from the project's goal of providing realistic FFB.

## Reference Documents
- GitHub Issue #178: Remove Base Force Mode

## Codebase Analysis Summary
### Current Architecture Overview
- `FFBEngine` manages the force calculation loop.
- `m_base_force_mode` is a member of `FFBEngine` and is part of the `Preset` structure in `Config.h`.
- `FFBEngine::calculate_force` uses `m_base_force_mode` to determine the `base_input` value.
- `GuiLayer_Common.cpp` provides a UI element to change this setting.

### Impacted Functionalities
- **FFB Calculation**: `FFBEngine::calculate_force` will be simplified to always use the conditioned game torque.
- **Configuration & Presets**: `base_force_mode` will be removed from the configuration file and internal `Preset` structure.
- **User Interface**: The "Base Force Mode" selection will be removed from the Tuning window.

## FFB Effect Impact Analysis
| Effect | Technical Changes | User-Facing Changes |
| :--- | :--- | :--- |
| **Base Steering Force** | `base_input` will always be `game_force_proc`. | "Synthetic" mode removed. Force will always scale naturally with game telemetry. |
| **UI** | Remove slider/combo for "Base Force Mode". | Simplified UI with one less setting to manage. |

## Proposed Changes
### 1. Modify `src/FFBEngine.h`
- Remove `int m_base_force_mode;` declaration.
- Remove `static constexpr double SYNTHETIC_MODE_DEADZONE_NM = 0.5;` (private).
- **Verification**: Use `read_file` to confirm removal.

### 2. Modify `src/FFBEngine.cpp`
- Remove the code block in `calculate_force` that switches between `m_base_force_mode == 0` and `m_base_force_mode == 1`.
- Directly assign `base_input = game_force_proc;`.
- **Verification**: Use `read_file` to confirm changes.

### 3. Modify `src/Config.h`
- Remove `int base_force_mode = 0;` from `Preset` struct.
- Remove `SetBaseMode(int v)` from `Preset` struct.
- **Verification**: Use `read_file` to confirm removal.

### 4. Modify `src/Config.cpp`
- Remove all references to `base_force_mode` in `Apply`, `Validate`, `UpdateFromEngine`, `Equals`, `ParsePresetLine`, `WritePresetFields`, `Load`, and `Save`.
- **Verification**: Use `read_file` to confirm changes.

### 5. Modify `src/GuiLayer_Common.cpp`
- Remove the `IntSetting` block for "Base Force Mode".
- **Verification**: Use `read_file` to confirm removal.

### 6. Update `VERSION`
- Increment version to `0.7.83`.
- **Verification**: Use `read_file` to confirm update.

## Test Plan
### Unit Tests (TDD Approach)
1.  **Red Phase**:
    -   Modify `tests/test_ffb_core_physics.cpp`: Remove `test_base_force_modes`, add `test_base_force_passthrough`.
    -   Modify `tests/test_ffb_understeer.cpp`: Remove `ASSERT_TRUE(p.base_force_mode == 0);`.
    -   Modify `tests/test_config_comprehensive.cpp`: Remove writing/checking of `base_force_mode`.
    -   Verify that build fails due to missing `m_base_force_mode`.
2.  **Green Phase**:
    -   Implement the proposed changes in core files.
    -   Verify that build succeeds.
3.  **Verification**:
    -   Run the full test suite: `./build/tests/run_combined_tests`.
    -   Ensure "Combined Test Summary" shows 0 failures.

## Deliverables
- [x] Modified `src/FFBEngine.h` and `src/FFBEngine.cpp`
- [x] Modified `src/Config.h` and `src/Config.cpp`
- [x] Modified `src/GuiLayer_Common.cpp`
- [x] Updated tests in `tests/`
- [x] Updated `VERSION`
- [x] Implementation Notes (added to this plan upon completion)

## Implementation Notes
### Unforeseen Issues
- None. The feature removal was straightforward as the code was well-organized.

### Plan Deviations
- Added the removal of `BASE_FORCE_MODE` tooltip from `Tooltips.h`, which was implicitly part of the UI removal but not explicitly mentioned in the initial Proposed Changes section.

### Challenges Encountered
- Ensuring all hardcoded presets in `Config.cpp` were updated correctly. Some presets used "Muted" mode (mode 2) for testing purposes, which now default to native physics.

### Recommendations for Future Plans
- When removing features that are used in tests or presets, perform a case-insensitive grep early to identify all affected strings.

## Final Steps
1.  **Complete pre-commit steps** to ensure proper testing, verification, review, and reflection are done.
2.  **Submit the changes** with a descriptive commit message.
