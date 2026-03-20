# Implementation Plan - Issue #430: Improve the Stationary Oscillations setting

Improve the Stationary Oscillations (Stationary Damping) setting by changing its default to 100%, updating its tooltip, formatting its slider as a percentage, and moving it to the General FFB section for better visibility.

## GitHub Issue
[Improve the Stationary Oscillations setting #430](https://github.com/coasting-nc/LMUFFB/issues/430)

## Design Rationale
The Stationary Damping feature is a critical safety and comfort feature that prevents violent wheel oscillations when the car is stopped (e.g., in the pit box). Currently, it defaults to 0%, making it effectively invisible to new users. Moving it to the "General FFB" section and defaulting it to 100% ensures that all users benefit from this protection immediately. Formatting the slider as a percentage makes it more intuitive and consistent with other global "Gain" style settings.

## Reference Documents
- [issue_430.md](../github_issues/issue_430.md)

## Codebase Analysis Summary
- **FFBEngine.h/cpp**: Contains the physics logic and the `m_stationary_damping` member.
- **Config.h**: Contains the `Preset` struct which stores the setting and its default value.
- **Tooltips.h**: Central repository for GUI tooltips.
- **GuiLayer_Common.cpp**: Implements the ImGui-based user interface.

### Impacted Functionalities
- **Default Initialization**: Both `FFBEngine` and `Preset` defaults are affected.
- **GUI Layout**: The position of the "Stationary Damping" slider changes.
- **GUI Presentation**: The formatting and tooltip of the slider are updated.

> ### Design Rationale
> The impact is primarily on the configuration defaults and the GUI layer. The core physics logic for stationary damping remains unchanged, but its "visibility" and "out-of-the-box" experience are significantly improved.

## FFB Effect Impact Analysis
| Effect | Technical Changes | User-Facing Changes |
| :--- | :--- | :--- |
| Stationary Damping | Default value changed in code. | Now enabled by default at 100%. |
| GUI | Slider moved and reformatted. | Easier to find, clearer tooltip with dynamic fade-out speed. |

> ### Design Rationale
> The physics of the effect is already implemented and verified in Issue #418. This task focuses on the "User Experience" (UX) and "Safety by Default" aspects of the feature.

## Proposed Changes

### 1. Update Default Values
- **src/ffb/FFBEngine.h**: Change `float m_stationary_damping = 0.0f;` to `1.0f`.
- **src/core/Config.h**: Change `float stationary_damping = 0.0f;` in `struct Preset` to `1.0f`.

### 2. Update Tooltips
- **src/gui/Tooltips.h**: Add `STATIONARY_DAMPING` constant and include it in the `ALL` list.

### 3. GUI Layout & Formatting
- **src/gui/GuiLayer_Common.cpp**:
    - Remove the slider from the "Rear Axle" section.
    - Add the slider to "General FFB" after "Min Force".
    - Use `FormatPct` for the value display.
    - Implement a dynamic tooltip showing the fade-out speed in km/h (`engine.m_speed_gate_upper * 3.6f`).

### Parameter Synchronization Checklist
- [x] Declaration in FFBEngine.h (m_stationary_damping) - *Already exists*
- [x] Declaration in Preset struct (Config.h) - *Already exists*
- [x] Entry in `Preset::Apply()` - *Already exists*
- [x] Entry in `Preset::UpdateFromEngine()` - *Already exists*
- [x] Entry in `Config::Save()` - *Already handled by Preset sync*
- [x] Entry in `Config::Load()` - *Already handled by Preset sync*
- [x] Validation logic (if applicable) - *Already exists in Preset::Validate()*

### Version Increment Rule
- Increment `VERSION` from `0.7.206` to `0.7.207`.

> ### Design Rationale
> Using a dynamic tooltip ensures the user always sees the correct fade-out speed, even if they have customized the "Speed Gate" settings in the "Advanced" section.

## Test Plan (TDD-Ready)

### 1. Default Value Verification
- **Test**: `test_stationary_damping_default`
- **Description**: Verify that a default-constructed `FFBEngine` has `m_stationary_damping` set to `1.0f`.
- **Expected Outcome**: `ASSERT_EQ(engine.m_stationary_damping, 1.0f);`

### 2. Regression Tests
- **Test Suite**: Run `./build/tests/run_combined_tests --filter=StationaryDamping`
- **Description**: Ensure existing logic for damping still works.

### 3. Tooltip Verification
- **Test**: `test_stationary_damping_tooltip_presence`
- **Description**: Verify `Tooltips::STATIONARY_DAMPING` is present and correctly formatted.

> ### Design Rationale
> Testing the default value is the most critical automated check for this task. The GUI changes will be verified by successful compilation and code review of the `GuiLayer_Common.cpp` logic.

## Deliverables
- [ ] Modified `src/ffb/FFBEngine.h`
- [ ] Modified `src/core/Config.h`
- [ ] Modified `src/gui/Tooltips.h`
- [ ] Modified `src/gui/GuiLayer_Common.cpp`
- [ ] Updated `VERSION` and `CHANGELOG_DEV.md`
- [ ] New test case for default value in `tests/test_issue_418_stationary_damping.cpp` (or a new file).
- [ ] Implementation plan updated with final notes.

## Implementation Notes
- Updated `FFBEngine.h` and `Config.h` to change default `stationary_damping` from `0.0f` to `1.0f`.
- Added `STATIONARY_DAMPING` tooltip to `Tooltips.h` and included it in the `ALL` vector.
- Relocated the "Stationary Damping" slider in `GuiLayer_Common.cpp` from the "Rear Axle" section to "General FFB", right after "Min Force".
- Changed slider formatting to use `FormatPct` and added a dynamic tooltip formatted with `StringUtils::SafeFormat`.
- Incremented project version to `0.7.207` and updated `CHANGELOG_DEV.md`.

## Encountered Issues
- **Tooltip Line Length**: The initial tooltip line was too long (90 chars), causing `test_tooltip_line_lengths` to fail (limit is 80). Fixed by adding a newline.
- **Regression in Issue #184 Test**: `test_soft_lock_stationary_not_allowed` failed because it expected `snap.total_output` to be exactly `-2.0`, but the new default stationary damping (active at 0 speed) added a small damping force, changing the output to `-2.05`. Fixed by explicitly disabling stationary damping in that test.

## Deviations from the Plan
- Modified `tests/test_issue_184_repro.cpp` to handle the side effect of the new default value on existing tests.
- Modified `src/gui/Tooltips.h` with an extra newline to satisfy line length constraints.

## Suggestions for the Future
- Consider making the "Speed Gate" settings (lower/upper) more visible if users frequently want to adjust when stationary damping fades out.
