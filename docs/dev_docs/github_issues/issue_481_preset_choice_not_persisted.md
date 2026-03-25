# Issue #481: Preset choice not persisted, app changes to default after reloading the app

**Original Link:** https://github.com/coasting-nc/LMUFFB/issues/481

## Description

Preset choice not persisted, app changes to default after reloading the app.

> "Also please be careful with the settings, I was playing around with them recently and after a restart the default profile got loaded without me noticing. When I left the garage the base went crazy and my finger still hurts three days later.:("
> https://community.lemansultimate.com/index.php?threads/lmuffb-app.10440/post-85347

## Implementation Notes (Fixer)

- **Root Cause Identified**: In `src/gui/GuiLayer_Common.cpp`, the matching of the UI `selected_preset` index with `Config::m_last_preset_name` was placed inside the `ImGui::TreeNodeEx("Presets and Configuration", ...)` block. If this tree node was not open on the first frame of the application, the initialization logic never ran, leaving the UI at index 0 ("Default").
- **Solution**:
  - Moved the `selected_preset` initialization logic to the beginning of the `DrawTuningWindow` function, before any ImGui branching. This ensures synchronization happens on the very first frame regardless of which UI sections are expanded.
  - Enhanced safety by persisting `m_session_peak_torque` and `m_auto_peak_front_load` normalization variables in the `[System]` table of `config.toml`. This prevents sudden FFB "punches" or surges when the app restarts by maintaining the session-learned peaks.
  - Added strict sanitization (finite and range checks) during configuration loading for these values.
  - Verified with a new automated regression test `tests/repro_issue_481.cpp`.
