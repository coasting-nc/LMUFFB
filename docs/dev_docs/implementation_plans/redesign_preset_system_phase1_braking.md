# Implementation Plan - Redesign Preset System Phase 1 (BrakingConfig)

This document describes the refactoring of the Braking parameters into a grouped logical category, following the 'Strangler Fig' pattern as part of the Phase 1 redesign of the presets system.

## Changes Performed

1.  **Defined `BrakingConfig` Struct**: Created a new struct in `src/ffb/FFBConfig.h` to hold all braking parameters (lockup, ABS, etc.). Included `Equals` for epsilon-based comparison and `Validate` for range clamping and consistency checks.
2.  **Updated `Preset` Struct**: Replaced loose braking variables in `src/core/Config.h` with a `BrakingConfig braking` member. Updated `Apply`, `UpdateFromEngine`, `Validate`, and `Equals` methods to delegate to the new struct. Updated fluent setters to maintain API compatibility.
3.  **Updated `FFBEngine` Class**: Replaced loose `m_lockup_...` and `m_abs_...` variables in `src/ffb/FFBEngine.h` with a `BrakingConfig m_braking` member.
4.  **Refactored Member References**: Updated all occurrences of the old loose variables in `FFBEngine.cpp`, `Config.cpp`, `GuiLayer_Common.cpp`, and other files to use the new nested structure.
5.  **Refactored Test Suite**: Updated unit and regression tests to use the new member paths.

## Encountered Issues

- *TBD*

## Deviations from the Plan

- *TBD*

## Suggestions for the Future

- Continue Phase 1 refactoring for remaining logical categories (`VibrationConfig`, `AdvancedConfig`, `SafetyConfig`).
- Transition to TOML for automated serialization in Phase 2.
