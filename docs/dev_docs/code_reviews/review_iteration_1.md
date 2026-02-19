# Code Review Iteration 1

The proposed code change is a high-quality, comprehensive fix for the identified issues with the "Direct Torque" (400Hz) FFB source. It correctly addresses both the scaling issue (weak force) and the requested "Pure Passthrough" (TIC mode) functionality.

### User's Goal:
Correct the scaling of the 400Hz Direct Torque FFB signal and implement a "Pure Passthrough" mode that bypasses LMUFFB's internal physics modulation (Understeer and Dynamic Weight) while retaining secondary tactile effects.

### Evaluation of the Solution:

1.  **Core Functionality:**
    *   **Scaling Fix:** The patch correctly identifies that `genFFBTorque` is a normalized signal ([-1.0, 1.0]). By multiplying it by `m_max_torque_ref` at the input stage, it aligns with the engine's internal Nm-based physics pipeline. Since the pipeline later divides the total sum by `m_max_torque_ref` for output normalization, the net result is a restoration of the signal's full dynamic range.
    *   **Passthrough Logic:** The implementation of `m_torque_passthrough` effectively bypasses the `grip_factor` (understeer) and `dynamic_weight_factor` multipliers when enabled. This allows the raw game signal to pass through to the final output while still allowing secondary effects (SoP, Textures, etc.) to be summed on top, perfectly matching the "TIC mode" requirement.
    *   **UI/Config Integration:** The new setting is fully integrated into the configuration system (parsing, saving, loading, presets) and the GUI.

2.  **Safety & Side Effects:**
    *   **Regressions:** The patch includes updates to existing tests (e.g., `test_ffb_diag_updates.cpp`) to ensure that the change in scaling logic doesn't break baseline diagnostics.
    *   **Thread Safety:** While the GUI directly modifies engine members, this is consistent with the project's existing architecture for tuning parameters. The FFB thread uses these values in a read-only manner during calculation.
    *   **Logic Verification:** The logic for `dw_factor_applied` and `grip_factor_applied` is safe and defaults to standard behavior.

3.  **Completeness:**
    *   The patch updates all relevant call-sites, including the `AsyncLogger` for telemetry analysis, the `Preset` system for profile management, and the `main.cpp` FFB thread loop.
    *   The addition of `tests/test_ffb_issue_142.cpp` provides high confidence in the fix by verifying scaling, passthrough, and snapshot reporting.

### Merge Assessment:
The patch is complete, follows project standards, and includes robust unit tests. It is ready for merge.

### Final Rating: #Correct#
