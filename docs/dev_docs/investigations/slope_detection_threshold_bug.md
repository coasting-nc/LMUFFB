# Bug Report: Slope Detection Threshold Disconnect

## Overview
During the investigation of Issue #100, a disconnected logic bug was discovered in the Slope Detection physics and GUI.

## Findings
1.  **Field Redundancy**: The `FFBEngine` class contains two similar fields:
    *   `m_slope_negative_threshold`: Historically used to trigger the slope effect.
    *   `m_slope_min_threshold`: Introduced later (v0.7.11) as part of the min/max range mapping.
2.  **Logic Disconnect**:
    *   The **GUI** (`src/GuiLayer_Common.cpp`) uses a slider to adjust `m_slope_negative_threshold`.
    *   The **Physics Engine** (`src/FFBEngine.h`) uses `m_slope_min_threshold` to calculate the grip loss percentage.
3.  **Impact**: The "Slope Threshold" slider in the GUI has NO EFFECT on the actual physics calculation, as the physics engine is reading from a different, hardcoded variable (`m_slope_min_threshold` defaults to -0.3f).

## Recommendation
- The `m_slope_negative_threshold` field should be deprecated and removed.
- The GUI slider should be updated to modify `m_slope_min_threshold` directly.
- The `Config::Load` and `Config::Save` logic should map the legacy `slope_negative_threshold` key to the new field for backward compatibility.

*Note: As per user instructions, no code changes for this bug are included in the current patch.*
