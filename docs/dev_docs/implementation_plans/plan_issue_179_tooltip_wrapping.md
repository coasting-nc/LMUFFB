# Implementation Plan - Issue #179: ToolTip Wrapping

## Context
Issue #179: ToolTips text gets cropped if line too long.
Users have reported that long tooltip strings in the GUI are being cut off by the window boundaries.
This plan aims to enforce a maximum line length for tooltips and provide a mechanism to verify this via unit tests.

## Reference Documents
- GitHub Issue #179
- `src/GuiLayer_Common.cpp`
- `src/GuiWidgets.h`

## Codebase Analysis Summary
- Tooltips are currently hardcoded as string literals in `src/GuiLayer_Common.cpp` and `src/GuiWidgets.h`.
- ImGui tooltips do not wrap automatically without specific configuration.
- The project uses a custom `GuiWidgets` namespace for standardized UI elements.

## Proposed Changes

### 1. Centralize Tooltip Strings
Create `src/Tooltips.h` to hold all tooltip strings. This allows the same strings to be used by both the GUI and the unit tests.

### 2. Refactor GUI to use Constants
Replace all string literals used for tooltips in `src/GuiLayer_Common.cpp` and `src/GuiWidgets.h` with the new constants.

### 3. Implement Verification Test
Create `tests/test_tooltips.cpp` to validate that no tooltip line exceeds the defined limit (80 characters).

## Parameter Synchronization Checklist
N/A (No new FFB parameters, only UI strings)

## Version Increment Rule
Smallest possible increment (+1 to the rightmost number).
Current Version: 0.7.78 -> 0.7.79

## Test Plan
- **test_tooltip_line_lengths**:
  - Input: All strings in `Tooltips::AllTooltips`.
  - Logic: Split each string by `\n`, check `length()` of each segment.
  - Assert: `segment.length() <= 80`.
  - Expected: Initially some might fail, requiring manual insertion of `\n`.

## Deliverables
- [x] `src/Tooltips.h`
- [x] Modified `src/GuiLayer_Common.cpp`
- [x] Modified `src/GuiWidgets.h`
- [x] `tests/test_tooltips.cpp`
- [x] Updated `VERSION` (0.7.79)
- [x] Updated `CHANGELOG_DEV.md`
- [x] Updated `USER_CHANGELOG.md`
- [x] Implementation Notes in this plan.

## Implementation Notes

### Unforeseen Issues
- Initially missed several tooltips that were passed as variables to `FloatSetting`, `BoolSetting`, etc., as well as those used in debug plots. Performed a comprehensive audit to ensure all were centralized.

### Plan Deviations
- Added `Tooltips::ALL` as a `std::vector` in the header to allow the test suite to iterate over all tooltips without needing complex reflection or manual string list maintenance in the test file itself.

### Challenges
- Ensuring that `\n` placement didn't just satisfy the 80-character limit but also maintained logical readability in the UI.

### Recommendations
- Future tooltips should always be added to `src/Tooltips.h` and the `ALL` vector to maintain test coverage.
- Consider implementing automatic wrapping in `GuiWidgets` if the number of tooltips continues to grow, though manual control currently provides better formatting.
