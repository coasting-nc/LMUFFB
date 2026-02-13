# Implementation Plan - Fix Slope Threshold Disconnect (Issue #104)

## Context
The 'Slope Threshold' slider in the GUI is currently disconnected from the physics engine. The GUI controls a deprecated variable m_slope_negative_threshold, while the physics engine uses m_slope_min_threshold. This plan outlines the steps to unify these variables and ensure the user's setting correctly affects the physics.

## Reference Documents
*   docs/dev_docs/investigations/issue_100_investigation_report.md (Original discovery)
*   Bug Report Issue #104

## Codebase Analysis Summary
*   **Current Architecture**: 
    *   FFBEngine.h: Defines both m_slope_negative_threshold (unused) and m_slope_min_threshold (used).
    *   Config.h/cpp: Serializes both variables independently.
    *   GuiLayer_Common.cpp: Binds the slider to the unused variable.
*   **Impacted Functionalities**:
    *   **Slope Detection Physics**: Currently uses a hardcoded default (-0.3) for the minimum threshold because the GUI slider changes a different variable.
    *   **Configuration**: Existing user presets saving 'slope_negative_threshold' will need migration.
    *   **GUI**: The 'Slope Threshold' slider needs to be rebound.

## FFB Effect Impact Analysis
*   **Affected Effects**: Slope Detection (Grip Loss).
*   **Technical Changes**: 
    *   Physics calculation in FFBEngine::calculate_slope_grip remains unchanged (it correctly uses m_slope_min_threshold).
    *   The variable feeding this calculation will now be controllable by the user.
*   **User Perspective**:
    *   **Feel Change**: The 'Slope Threshold' slider will now actually work. Users may notice their slope detection settings suddenly taking effect if they had tuned them away from default.
    *   **Settings**: No new settings, but the existing setting will become functional.
    *   **Presets**: Legacy presets will be auto-migrated.

## Proposed Changes

### 1. Refactor FFBEngine.h
*   Remove loat m_slope_negative_threshold.

### 2. Refactor Config.h
*   **Preset Struct**: Remove loat slope_negative_threshold.
*   **Parameter Synchronization**:
    *   Remove slope_negative_threshold from Apply, UpdateFromEngine, Equals, Validate.
    *   Update SetSlopeDetection to remove the deprecated argument.

### 3. Refactor Config.cpp
*   **ParsePresetLine**: 
    *   If key is slope_negative_threshold, parse value into slope_min_threshold.
*   **Load**:
    *   If key is slope_negative_threshold, parse value into engine.m_slope_min_threshold.
*   **Save/WritePresetFields**:
    *   Stop writing slope_negative_threshold.
    *   Ensure slope_min_threshold is written.

### 4. Refactor GuiLayer_Common.cpp
*   Bind 'Slope Threshold' slider to engine.m_slope_min_threshold.
*   Update tooltip if necessary (seems accurate: 'Slope value below which grip loss begins').

### 5. Refactor main.cpp
*   Update info.slope_threshold assignment to use m_slope_min_threshold.

### 6. Version Increment
*   Increment VERSION and src/Version.h by +1 (0.7.36 -> 0.7.37).

## Test Plan (TDD-Ready)

### Test 1: 	est_slope_config_migration
*   **Goal**: Verify that loading a legacy config with slope_negative_threshold correctly updates m_slope_min_threshold.
*   **Steps**:
    1.  Create a temporary ini file with slope_negative_threshold = -0.8 and slope_min_threshold missing (or default).
    2.  Load into FFBEngine.
    3.  Assert engine.m_slope_min_threshold is approx -0.8.

### Test 2: 	est_slope_config_persistence
*   **Goal**: Verify that saving the config writes slope_min_threshold and NOT slope_negative_threshold.
*   **Steps**:
    1.  Set engine.m_slope_min_threshold = -0.5.
    2.  Save to temporary ini.
    3.  Read file content.
    4.  Assert file contains slope_min_threshold=-0.5.
    5.  Assert file does NOT contain slope_negative_threshold.

## Deliverables
*   [ ] Refactored code (FFBEngine, Config, GUI, Main).
*   [ ] Regression tests (	ests/test_issue_104_slope_disconnect.cpp).
*   [ ] Updated Version.
*   [ ] Implementation Notes in this plan.
