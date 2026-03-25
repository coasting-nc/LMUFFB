# Implementation Plan - Issue #481: Preset choice not persisted

## 1. Problem Statement
The user reported that their selected preset is not persisted after restarting the application. The application defaults back to the "Default" preset. This can be dangerous if the user has tuned their FFB and is suddenly met with unexpected forces.
Additionally, Issue #475 noted that the `Lateral Load` slider value was not correctly taken from config (clamped to 2.0 while GUI goes to 10.0).

## 2. Analysis
- `Config::m_last_preset_name` is used to store the name of the last applied preset in `config.toml`.
- In `GuiLayer_Common.cpp`, the matching of `selected_preset` (the UI index) with `m_last_preset_name` only happens inside the `ImGui::TreeNodeEx("Presets and Configuration", ...)` block. If this tree node is not open on the first frame, `first_run` remains true or matching never happens.
- When matching DOES happen, it relies on exact string comparison.
- Normalization peaks (`m_session_peak_torque` and `m_auto_peak_front_load`) are reset on every app start, which can cause FFB surges until the app "re-learns" the peaks for the session.

## 3. Proposed Changes

### Core Configuration (`src/core/Config.cpp`)
- Update `Config::Save` to persist `engine.m_session_peak_torque` and `engine.m_auto_peak_front_load` into the `[System]` table.
- Update `Config::Load` to read these values back into the engine if they exist.

### FFB Physics (`src/ffb/FFBConfig.h`)
- Update `LoadForcesConfig::Validate()` to increase the `lat_load_effect` clamp from 2.0 to 10.0 (fixing Issue #475).

### GUI Layer (`src/gui/GuiLayer_Common.cpp`)
- Move the `selected_preset` initialization logic from inside the tree node to the start of `DrawTuningWindow`. This ensures it runs once on app start regardless of UI state.

## 4. Verification Plan

### Automated Tests
- Create `tests/repro_issue_481.cpp` to verify that `m_last_preset_name` is correctly saved and restored, and that it results in the correct UI index being selected.
- The test will also verify that `lat_load_effect` can be saved and loaded up to 10.0.

### Manual Verification
- Not possible in this environment (requires GUI/Windows). Relying on automated tests and code review.

## 5. Implementation Notes

### Iteration 1
- Initial implementation addressed both #481 and #475.
- Persistence for `m_session_peak_torque` and `m_auto_peak_front_load` was added to `Config::Save` and `Config::Load`.
- `selected_preset` initialization logic moved to the start of `DrawTuningWindow`.

### Iteration 2 (Post-Review)
- Reverted changes for #475 to maintain single-issue focus.
- Restored encapsulation in `FFBEngine` by moving normalization members back to private and using `friend class Config;`.
- Added strict sanitization (finite and range checks) when loading normalization peaks from TOML.
- Cleaned up accidental binary and scratch files.
- Updated `VERSION` and `CHANGELOG_DEV.md`.

### Deviations
- Strictly focused on #481 after initial feedback.
- Added friendship to `FFBEngine` instead of making members public.

### Suggestions for the future
- The GUI initialization logic could be further modularized to avoid `static` state in `DrawTuningWindow`.
- A more robust UI state management system could handle preset matching automatically on any config change.
