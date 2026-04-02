# Implementation Plan - Issue #446: In-Game FFB "No Signal" Warning

This plan addresses Issue #446 by implementing a visual warning in the GUI when the user has selected "In-Game FFB" but no signal is being received from the game.

## Design Rationale
- **User Reliability**: Provides immediate, localized feedback when a critical FFB source is missing, reducing user frustration and troubleshooting time.
- **Architectural Consistency**: Leverages the existing `HealthMonitor` diagnostic system rather than creating a redundant detection loop.
- **Non-Intrusive**: The warning only appears when there is a definite mismatch (source selected but zero activity while driving).

## Reference Documents
- GitHub Issue: `docs/dev_docs/github_issues/issue_446.md`

## Codebase Analysis Summary
- **src/logging/HealthMonitor.h**: Logic for determining if system sample rates are healthy. It needs to be updated to explicitly flag when the In-Game FFB signal is missing despite being selected.
- **src/gui/GuiLayer_Common.cpp**: The primary UI rendering logic. It needs to check the `HealthStatus` and apply visual styling (red text/flashing) to the "Use In-Game FFB" checkbox.
- **src/gui/Tooltips.h**: Contains tooltip strings for GUI widgets.
- **README.md**: Main project documentation with setup instructions.

## FFB Effect Impact Analysis
- **Affected Effects**: None. This is a UI/Diagnostic change only.
- **Technical Perspective**: No physics calculations are modified. The `HealthMonitor` already tracks the `genTorque` rate.
- **User Perspective**: Users will see a clear "NO SIGNAL" warning if they forget to enable FFB in the LMU game settings while using the "In-Game FFB" option in lmuFFB.

## Proposed Changes

### 1. Update Diagnostics (HealthMonitor)
- **File**: `src/logging/HealthMonitor.h`
- **Change**: Add `ingame_ffb_missing` boolean to `HealthStatus` struct.
- **Logic**: In `HealthMonitor::Check`, set `status.ingame_ffb_missing = (torqueSource == 1 && torque < 1.0 && isRealtime && playerControl == 0)`.

### 2. Update Tooltips
- **File**: `src/gui/Tooltips.h`
- **Change**: Update `USE_INGAME_FFB` string.
- **New Value**: `"Recommended for LMU 1.2+. Uses the high-frequency FFB channel\ndirectly from the game.\nMatches the game's internal physics rate for maximum fidelity.\n\nIMPORTANT: Ensure Force Feedback is ENABLED in LMU settings and Strength is > 0%."`

### 3. Update GUI Visuals
- **File**: `src/gui/GuiLayer_Common.cpp`
- **Logic**: In `DrawTuningWindow`, when rendering the "Use In-Game FFB" checkbox:
    - Perform a check using `HealthMonitor::Check` (or similar logic).
    - If `ingame_ffb_missing` is true:
        - Determine if it should be highlighted (e.g., blinking red).
        - Use `ImGui::PushStyleColor(ImGuiCol_Text, ...)` to make it red.
        - Use a label like `"Use In-Game FFB (400Hz Native) - NO SIGNAL! CHECK GAME SETTINGS"`
        - implement blinking using `(fmod(ImGui::GetTime(), 1.0) < 0.5)`.

### 4. Update Documentation
- **File**: `README.md`
- **Change**: Update the "In-Game Force Feedback settings in LMU" section to recommend 100% strength and explicitly state it must be enabled.

### 5. Version Increment
- **File**: `VERSION`
- **Change**: Increment version from `0.7.275` to `0.7.276`.

## Test Plan (TDD-Ready)
- **Test File**: `tests/repro_issue_446.cpp`
- **Design Rationale**: Verify that the detection logic in `HealthMonitor` correctly flags the missing signal under the right conditions (driving, in-game source selected, no signal) and does NOT flag it otherwise (menu, signal present).
- **Test Cases**:
    1. `test_ingame_ffb_missing_detection`:
        - Inputs: `torqueSource=1`, `torque=0.0`, `isRealtime=true`, `playerControl=0`.
        - Expected: `ingame_ffb_missing == true`.
    2. `test_ingame_ffb_present_detection`:
        - Inputs: `torqueSource=1`, `torque=400.0`, `isRealtime=true`, `playerControl=0`.
        - Expected: `ingame_ffb_missing == false`.
    3. `test_ingame_ffb_missing_not_driving`:
        - Inputs: `torqueSource=1`, `torque=0.0`, `isRealtime=false`, `playerControl=0`.
        - Expected: `ingame_ffb_missing == false`.

## Deliverables
- [ ] Modified `src/logging/HealthMonitor.h`
- [ ] Modified `src/gui/GuiLayer_Common.cpp`
- [ ] Modified `src/gui/Tooltips.h`
- [ ] Modified `README.md`
- [ ] New `tests/repro_issue_446.cpp`
- [ ] Updated `VERSION`
- [ ] Updated `CHANGELOG_DEV.md`
- [ ] Implementation Notes updated in this plan.

## Implementation Notes

### Unforeseen Issues
- **Tooltip Line Lengths**: The initial update to `Tooltips.h` triggered a test failure in `test_tooltip_line_lengths` because the new line was 81 characters long (limit is 80). Fixed by adding an explicit `\n`.

### Plan Deviations
- **Blinking Logic**: Added a simple time-based blink (`fmod(ImGui::GetTime(), 1.0f) < 0.5f`) to the red warning text for increased visibility as suggested in the original issue.

### Challenges
- **Headless Testing**: Since I cannot run the GUI, I relied on unit tests in `repro_issue_446.cpp` to verify the `HealthMonitor` detection logic. The UI logic was verified by code review and adherence to established ImGui patterns.

### Recommendations
- Future diagnostics could include a "Setup Guide" popup if multiple critical signals are missing, guiding users through the necessary LMU settings.
