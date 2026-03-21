# Implementation Plan - Redesign Preset System Phase 1 (SlopeDetectionConfig)

This document describes the refactoring of the Slope Detection parameters into a grouped logical category, following the 'Strangler Fig' pattern as part of the Phase 1 redesign of the presets system.

## Changes Performed

1.  **Defined `SlopeDetectionConfig` Struct**: Created a new struct in `src/ffb/FFBConfig.h` to hold all 13 slope detection parameters. Included `Equals` for epsilon-based comparison and `Validate` for range clamping and consistency checks (e.g., ensuring `sg_window` is odd).
2.  **Updated `Preset` Struct**: Replaced loose slope detection variables in `src/core/Config.h` with a `SlopeDetectionConfig` member. Updated `Apply`, `UpdateFromEngine`, `Validate`, and `Equals` methods to delegate to the new struct. Updated fluent setters to maintain API compatibility.
3.  **Updated `FFBEngine` Class**: Replaced loose `m_slope_...` variables in `src/ffb/FFBEngine.h` with a `SlopeDetectionConfig m_slope_detection` member.
4.  **Refactored Member References**: Updated all occurrences of the old loose variables in `FFBEngine.cpp`, `Config.cpp`, `GuiLayer_Common.cpp`, `GripLoadEstimation.cpp`, and `main.cpp` to use the new nested structure.
5.  **Updated Telemetry Logging**: Refactored `AsyncLogger.h` and `main.cpp` to use the new struct for session initialization and logging.
6.  **Refactored Test Suite**: Updated all unit and regression tests (over 20 files) to use the new member paths. This included fixing several `sed` errors where variable names appeared as substrings of other names or within macro expansions.

## Encountered Issues

- **Multiple Test Regressions**: Renaming variables that were used in dozens of test files caused widespread compilation failures. Automated replacement using `sed` was necessary but required careful manual verification to handle cases like `slope_detection_enabled` being a prefix of other identifiers.
- **AsyncLogger Dependencies**: The `AsyncLogger` utilized several slope detection parameters in its `SessionInfo` struct, which required refactoring to remain synchronized with the rest of the system.
- **Validation Consistency**: Ensuring that the legacy reset-to-default logic in `Config::Load` was preserved while also applying the new `SlopeDetectionConfig::Validate()` rules required careful ordering of checks.

## Deviations from the Plan

- **Extended Refactoring Scope**: The refactoring initially focused on `FFBEngine` and `Preset`, but it became apparent that `AsyncLogger` and almost all files in the `tests/` directory also required updates to accommodate the structural change.
- **Naming Consistency**: Followed the convention of previous refactors (`m_slope_detection` in `FFBEngine`, `slope_detection` in `Preset`) rather than the proposed `m_slope_config` to maintain codebase uniformity.

## Suggestions for the Future

- **Continue Phase 1**: Proceed with the remaining logical categories (`BrakingConfig`, `VibrationConfig`, `AdvancedConfig`, `SafetyConfig`) using the same incremental approach.
- **Automated Preset Mapping**: Once Phase 1 is complete, Phase 2 should focus on using a TOML library to automate the mapping between the C++ structs and the configuration file, eliminating the need for manual `Parse...Line` and `WritePresetFields` logic.
- **Test Infrastructure**: Consider adding a reflection-based or macro-based system for tests to access configuration members, which would reduce the maintenance burden when restructuring these categories in the future.
