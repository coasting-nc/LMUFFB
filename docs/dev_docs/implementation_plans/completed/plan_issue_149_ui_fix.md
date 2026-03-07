# Implementation Plan - Issue #149: "Use In-Game FFB" toggle is not show correctly

## Context
The "Use In-Game FFB" toggle in the Tuning window is currently obscured or poorly aligned due to its proximity to the "System Health (Hz)" section. This issue aims to improve UI clarity by moving the advanced "System Health" diagnostics to the Graphs (Debug) window and fixing the alignment/spacing of the main toggle.

## Reference Documents
- GitHub Issue #149: "Use In-Game FFB" toggle is not show correctly.
- `src/GuiLayer_Common.cpp`: Main GUI implementation.

## Codebase Analysis Summary
- **Current Architecture:** The GUI is implemented using ImGui in `GuiLayer_Common.cpp`. `DrawTuningWindow` handles the main settings, and `DrawDebugWindow` handles the graphs/diagnostics.
- **Impacted Functionalities:**
    - `GuiLayer::DrawTuningWindow`: The "General FFB" section currently contains the "System Health" tree node.
    - `GuiLayer::DrawDebugWindow`: This is the destination for the moved "System Health" diagnostics.

## FFB Effect Impact Analysis
- **Affected FFB effects:** None. This is a pure UI/UX improvement.
- **User Perspective:**
    - The Tuning window will be cleaner and less cluttered.
    - Advanced users can still find health diagnostics in the Graphs window.
    - The "Use In-Game FFB" toggle will be more discoverable and correctly aligned.

## Proposed Changes
### `src/GuiLayer_Common.cpp`
- **Modify `DrawTuningWindow`**:
    - Remove the `if (ImGui::TreeNode("System Health (Hz)")) { ... }` block.
    - Add `ImGui::Spacing();` before the "Use In-Game FFB (400Hz Native)" checkbox to ensure it stands out and is correctly positioned.
- **Modify `DrawDebugWindow`**:
    - Add the "System Health (Hz)" diagnostics at the top of the window, or as a new section.
    - Since `DrawDebugWindow` doesn't use the same 2-column layout as `DrawTuningWindow`, the rendering logic will need slight adjustment for width.

### Parameter Synchronization Checklist
- N/A (No new parameters added).

### Initialization Order Analysis
- N/A (UI changes only).

### Version Increment Rule
- Version will be incremented from `0.7.65` to `0.7.66`.

## Test Plan (TDD-Ready)
- Since this is a UI change and we are running in headless mode, automated UI testing is limited.
- **Verification through Static Analysis & Build:**
    - Ensure the project builds successfully on Linux.
- **Functional Verification (Health Monitor):**
    - Existing tests in `tests/test_health_monitor.cpp` and `tests/test_ffb_diag_updates.cpp` must pass to ensure the data itself is still correctly captured.
- **Manual Verification (Instructions for Windows):**
    - Run the app on Windows.
    - Open the Tuning window.
    - Verify "System Health" is gone.
    - Verify "Use In-Game FFB" is correctly visible and spaced.
    - Enable "Graphs".
    - Verify "System Health" is visible in the Analysis window.

## Deliverables
- [x] Modified `src/GuiLayer_Common.cpp`.
- [x] Updated `VERSION`.
- [x] Updated `CHANGELOG_DEV.md`.
- [x] Implementation Notes (added to this plan upon completion).

## Implementation Notes
- **Unforeseen Issues:** None. The relocation of the ImGui code was straightforward.
- **Plan Deviations:**
    - Refactored the `DisplayRate` rendering logic into a static helper function in `GuiLayer_Common.cpp` following code review feedback to improve maintainability and avoid future duplication if more diagnostic windows are added.
    - Used a 5-column layout in the Debug window to better fit all rate monitors in a single horizontal row, maximizing vertical space for graphs.
- **Challenges Encountered:** Navigating the complex ImGui column nesting in `DrawTuningWindow` required careful deletion to ensure the 2-column "SettingsGrid" layout remained intact for subsequent widgets.
- **Recommendations for Future Plans:** Explicitly mention refactoring of local lambdas into static helpers when moving code between different UI functions.
