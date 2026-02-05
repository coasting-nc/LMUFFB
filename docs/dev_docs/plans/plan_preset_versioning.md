# Implementation Plan - Preset Versioning and Telemetry Version Header

This plan outlines the implementation of version tracking for saved presets and telemetry logs. This ensures that shared presets can be correctly interpreted and migrated by different versions of the application, and telemetry logs are tagged with the version that generated them.

## Context
User presets in `config.ini` currently lack version metadata. When users share presets (often by pasting just the preset section), the receiving app version might have different scales or default behaviors. Adding a version field to each preset allows for automated migration. Similarly, telemetry logs should be tagged with the app version for better diagnostics.

## Reference Documents
- GitHub Issue: https://github.com/coasting-nc/LMUFFB/issues/55
- Existing Config Logic: `src/Config.cpp`
- Existing Logger Logic: `src/AsyncLogger.h`

## Codebase Analysis Summary

### Current Architecture Overview
- **Presets:** Stored as `Preset` structs in `src/Config.h`. Managed by `Config` class in `src/Config.cpp`.
- **Persistence:** Custom INI-style parsing in `Config::Load`, `Config::Save`, and `Config::LoadPresets`.
- **Telemetry:** `AsyncLogger` handles asynchronous CSV logging. It uses a `SessionInfo` struct for header metadata.
- **Version:** Controlled by `VERSION` file and `src/Version.h` (`LMUFFB_VERSION` macro).

### Impacted Functionalities
- **Preset Persistence:** `Config::Save` and `Config::LoadPresets` will be updated to include `app_version`.
- **Telemetry Initialization:** `GuiLayer.cpp` needs to populate the version in `SessionInfo`.
- **Migration System:** `Config::LoadPresets` will now have a hook for version-based migration of user presets.

## Proposed Changes

### 1. Preset Structure & Version Tracking

#### [src/Config.h]
- Add `std::string app_version` member to `Preset` struct.
- Initialize `app_version` to `LMUFFB_VERSION` in the `Preset(name, builtin)` constructor.
- Add `app_version = LMUFFB_VERSION;` to `Preset::UpdateFromEngine()`.

#### [src/Version.h] / [VERSION]
- Increment version to `0.7.12`.

### 2. Config Persistence & Migration

#### [src/Config.cpp]
- **`Config::LoadPresets()`**:
    - Add parsing for `app_version` key within `[Preset:Name]` sections.
    - Track if any preset required migration via a `needs_save` flag.
    - If `app_version` is different from `LMUFFB_VERSION` (or empty), trigger migration.
    - After loading all presets, if `needs_save` is true, call `Config::SaveManualPresetsOnly()` (or similar) to update the file on disk.
      - *Refinement:* Since `Config::Save` currently takes an `FFBEngine` and saves everything, I might need a way to just update the presets or accept that it saves current engine state too. Existing `Config::Save` is usually fine as it captures current UI state.
- **`Config::Save()`**:
    - In the preset loop (line ~897), write `file << "app_version=" << p.app_version << "\n";`.
- **Migration Logic**:
    - Implement (or expand) migration logic to handle changes between versions.
    - **Initial Implementation:** For legacy presets (empty version), migration simply involves setting \pp_version\ to \LMUFFB_VERSION\. 
    - Note: The app should auto-save the config if any user presets were migrated during load.

### 3. Telemetry Header

#### [src/GuiLayer.cpp]
- In the `AsyncLogger::Get().Start` call site (~line 869), ensure `info.app_version` is populated:
  ```cpp
  info.app_version = LMUFFB_VERSION;
  ```

#### [src/AsyncLogger.h]
- (Verified) `WriteHeader` already includes `info.app_version`.

## Parameter Synchronization Checklist
- **app_version**:
    - [x] Declaration in `Preset` struct (`Config.h`)
    - [x] Entry in `Preset::UpdateFromEngine()`
    - [x] Entry in `Config::Save()` (within Preset loop)
    - [x] Entry in `Config::LoadPresets()`
    - [ ] Entry in `Preset::Apply()` - *Not needed as it's metadata*
    - [ ] Entry in `Preset::UpdateFromEngine()` - *Already covered*

## Initialization Order Analysis
- No circular dependencies expected; `LMUFFB_VERSION` is a macro available via `Version.h`, which is already included in `Config.cpp`.

## Test Plan (TDD-Ready)

### New Tests: `tests/test_versioned_presets.cpp`
1.  **`test_preset_version_persistence`**:
    - Create a user preset.
    - Save it to a temporary INI.
    - Verify the INI contains `app_version=0.7.12` (relative to current version).
    - Load it back and verify the `Preset` object has the correct `app_version`.
2.  **`test_legacy_preset_migration`**:
    - Create an INI with a preset lacking an `app_version` (simulating v0.7.11 or older).
    - Load presets.
    - Verify the preset is migrated (e.g., version set to current, or specific legacy fixes applied).
    - Verify that an auto-save is triggered or that the next save includes the version.

### Updated Tests: `tests/test_async_logger.cpp`
1.  **`test_logger_header_version_check`**:
    - Start logging.
    - Stop logging.
    - Read the generated CSV.
    - Verify the second line starts with `# App Version: 0.7.`.

## Deliverables
- [ ] Code changes in `Config.h`, `Config.cpp`, `GuiLayer.cpp`, `Version.h`, `VERSION`.
- [ ] New test file `tests/test_versioned_presets.cpp`.
- [ ] Updated `tests/test_async_logger.cpp`.
- [ ] Updated `CHANGELOG_DEV.md` and `USER_CHANGELOG.md`.
- [ ] Implementation Notes (to be updated after execution).

