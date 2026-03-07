# Implementation Plan - Issue #214: Rename section for Vibration Effects

## Context
The goal is to rename "Tactile Textures" and "haptic effect" to "Vibration Effects" and related "vibration" naming across the UI, code, and documentation to improve clarity and consistency.

**Issue Number:** #214
**Issue Title:** Rename section for Vibration Effects

## Design Rationale
The terms "Tactile" and "Haptic" are often used interchangeably with "Vibration" in the context of sim racing FFB. However, "Vibration Effects" is more intuitive for many users and aligns better with how these effects (Road, Slide, Lockup, etc.) are perceived. Renaming these consistently across the UI, code, and config improves maintainability and user experience.

## Codebase Analysis Summary
- **FFBEngine.h/cpp**: Contains core members like `m_tactile_gain` and `m_smoothed_tactile_mult`, as well as logic for summing these effects.
- **Config.h/cpp**: Handles persistence of these settings in `config.ini` and presets.
- **GuiLayer_Common.cpp**: Defines the UI section "Tactile Textures" and related slider labels.
- **Tooltips.h**: Contains user-facing descriptions for these settings.
- **Tests**: Multiple test files verify these effects and their scaling/normalization.

## FFB Effect Impact Analysis
| Affected Effect | Technical Changes | User-Facing Changes |
| :--- | :--- | :--- |
| Road Texture | Renamed gain/scaling vars | Section header and slider labels |
| Slide Rumble | Renamed gain/scaling vars | Slider labels |
| Lockup Vibration | Renamed gain/scaling vars | Section header and slider labels |
| ABS Pulse | Renamed gain/scaling vars | Section header |
| Spin Vibration | Renamed gain/scaling vars | Slider labels |
| Bottoming | Renamed gain/scaling vars | Slider labels |

## Proposed Changes

### Core Engine (`src/FFBEngine.h`, `src/FFBEngine.cpp`, `src/GripLoadEstimation.cpp`)
- Rename `m_tactile_gain` -> `m_vibration_gain`.
- Rename `m_smoothed_tactile_mult` -> `m_smoothed_vibration_mult`.
- Rename `TACTILE_EMA_TAU` -> `VIBRATION_EMA_TAU`.
- Update comments to replace "tactile" and "haptic" with "vibration".

### Configuration (`src/Config.h`, `src/Config.cpp`)
- Rename `tactile_gain` -> `vibration_gain` in `Preset` struct.
- In `Config::Load`, add backward compatibility: if "tactile_gain" is found but "vibration_gain" is not, use the "tactile_gain" value.
- Update `Config::Save` to use "vibration_gain".

### UI and Tooltips (`src/GuiLayer_Common.cpp`, `src/Tooltips.h`)
- Rename section "Tactile Textures" -> "Vibration Effects".
- Rename "Tactile Strength" -> "Vibration Strength".
- Update all tooltips in `Tooltips.h` to use "vibration" terminology.

### Tests (`tests/`)
- Update `tests/test_issue_206_tactile_scaling.cpp`.
- Update `tests/test_ffb_tactile_normalization.cpp`.
- Update `tests/test_ffb_common.h` (friend accessors).
- Update other tests that reference "tactile" or "haptic" variables.

### Versioning
- Increment `VERSION` to `0.7.113`.

## Test Plan
- **Baseline Verification**: Ensure existing tests pass before changes.
- **Renaming Verification**: After renaming, ensure the project builds and all tests pass.
- **Persistence Verification**:
  - Save a config with `tactile_gain=0.8`.
  - Load it with the new version and verify it maps to `vibration_gain=0.8`.
  - Save it and verify it writes `vibration_gain=0.8`.

## Deliverables
- [x] Modified C++ source files.
- [x] Modified test files.
- [x] Updated `VERSION` and `CHANGELOG`.
- [x] Implementation Notes (in this plan).

## Implementation Notes
- **Renaming Strategy**: A systematic approach was used to rename all occurrences of "Tactile" and "Haptic" to "Vibration" where it made sense for the user (vibration effects, vibration strength). Internal code variables and constants were also updated for consistency (`m_vibration_gain`, `VIBRATION_EMA_TAU`).
- **Backward Compatibility**: Successfully implemented migration logic in `Config.cpp`. The app now checks for `tactile_gain` if `vibration_gain` is missing, ensuring users don't lose their settings when upgrading.
- **Test Suite**: Renamed key test files (`test_ffb_vibration_normalization.cpp`, `test_issue_206_vibration_scaling.cpp`) and updated `tests/CMakeLists.txt`. All 391 test cases (1610 assertions) passed on Linux.
- **Documentation**: Updated `Tooltips.h` and `CHANGELOG_DEV.md` to reflect the new terminology.
- **Challenges**: Ensuring all "tactile" references in tests were caught required multiple passes and `grep` verification. Renaming files in `tests/` required updating `CMakeLists.txt` to avoid build breakages.
