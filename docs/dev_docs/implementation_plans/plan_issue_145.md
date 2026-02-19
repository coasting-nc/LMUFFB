# Implementation Plan - Issue #145: Rename "Direct Torque" to "In-Game FFB" and Improve Visibility

The goal is to improve the discoverability and clarity of the high-fidelity FFB source by renaming "Direct Torque" to "In-Game FFB" and adding a more prominent toggle for it in the UI.

## Context
User feedback indicates that the "Direct Torque" terminology is obscure and the setting is buried within a dropdown menu. Renaming it to "In-Game FFB" and adding a visible toggle makes this recommended feature easier to find and understand.

## Reference Documents
- GitHub Issue #145
- [Implementation Plan - Issue #142: Fix Direct Torque Strength and Passthrough](docs/dev_docs/implementation_plans/plan_issue_142.md)

## Codebase Analysis Summary
- `src/GuiLayer_Common.cpp`: Main UI implementation using ImGui. Defines the "Torque Source" dropdown and diagnostic plots.
- `src/Config.h` / `src/Config.cpp`: Manages `torque_source` (int: 0=Shaft, 1=Direct) and `torque_passthrough` (bool).
- `src/FFBEngine.cpp`: Uses `m_torque_source` to select the base torque input signal.

## FFB Effect Impact Analysis
- This is a terminology and UI layout change.
- It clarifies that "Direct Torque" is the game's native FFB.
- No impact on the underlying physics algorithms.

## Proposed Changes

### 1. Terminological Rename
Modify `src/GuiLayer_Common.cpp`:
- Rename `torque_sources` labels:
    - Index 0: "Shaft Torque (100Hz Legacy)"
    - Index 1: "In-Game FFB (400Hz LMU 1.2+)"
- Update tooltip for "Torque Source":
    - Explain that "In-Game FFB" is the actual FFB signal from the game.
- Update tooltip for "Pure Passthrough":
    - Update "Direct Torque (400Hz)" reference to "In-Game FFB (400Hz)".
- Rename diagnostic plot:
    - "Direct Torque (400Hz)" -> "In-Game FFB (400Hz)"
- Update Plot tooltip.

### 2. UI Visibility Improvement
Modify `src/GuiLayer_Common.cpp`:
- Add a new checkbox in the "General FFB" section:
    ```cpp
    bool use_in_game_ffb = (engine.m_torque_source == 1);
    if (BoolSetting("Use In-Game FFB (400Hz Native)", &use_in_game_ffb,
                    "Recommended for LMU 1.2+. Uses the high-frequency FFB channel directly from the game.\n"
                    "Matches the game's internal physics rate for maximum fidelity.")) {
        engine.m_torque_source = use_in_game_ffb ? 1 : 0;
        Config::Save(engine);
    }
    ```
- This checkbox will be synchronized with the existing "Torque Source" dropdown.

### 3. Versioning
- Increment `VERSION` and `src/Version.h` (implicitly via CMake/`VERSION` file).
- Update `CHANGELOG_DEV.md` and `USER_CHANGELOG.md`.

## Test Plan
- **Verification**:
    - Ensure all "Direct Torque" strings in the UI are replaced.
    - Ensure the new toggle correctly sets `m_torque_source`.
- **Regression**:
    - Run `run_combined_tests` to ensure no logic was broken.

## Deliverables
- [ ] Modified `src/GuiLayer_Common.cpp`
- [ ] Updated `VERSION`
- [ ] Updated `CHANGELOG_DEV.md`
- [ ] Updated `USER_CHANGELOG.md`
- [ ] This Implementation Plan with final notes.

---
## Implementation Notes

### Unforeseen Issues
- The code review correctly pointed out that although the `DrawTuningWindow` function is already protected by a recursive mutex at the entry point, explicitly adding a local `std::lock_guard` when modifying shared state like `engine.m_torque_source` is best practice and aligns with the project's reliability standards.

### Plan Deviations
- Added an explicit `std::lock_guard` for the new toggle modification to satisfy reliability standards.
- Performed build using `BUILD_HEADLESS=ON` to accommodate the Linux environment limitations.

### Challenges
- Ensuring all tooltips were updated to reflect the new terminology across the entire `GuiLayer_Common.cpp` file.

### Recommendations
- Continue monitoring user feedback on the "In-Game FFB" toggle to see if further UI simplifications are needed (e.g., hiding the legacy dropdown by default).

### Final Status
- **UI Labels**: All instances of "Direct Torque" renamed to "In-Game FFB".
- **Toggle**: "Use In-Game FFB (400Hz Native)" toggle added and synchronized.
- **Reliability**: Explicit mutex locks added to state modifications in the GUI.
- **Versioning**: Version bumped to 0.7.65.
- **Tests**: All tests passed (1160 assertions).
- **Reviews**: Greenlight achieved on Iteration 3.
