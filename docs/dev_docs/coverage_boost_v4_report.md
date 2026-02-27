# Code Coverage Boost Report (v4)

## Overview
This patch increases the code coverage for the LMUFFB project, focusing on branch coverage in core files such as `Config.cpp`, `FFBEngine.cpp`, `GuiLayer_Common.cpp`, and `main.cpp`.

## Strategies to Increase Branch Coverage
1.  **Legacy Migration Testing**: Added tests that provide configuration files with legacy values (e.g., `understeer > 2.0`, `max_torque_ref > 40.0`). This exercises the migration logic that scales old configuration formats to the new internal units.
2.  **Input Validation and Clamping**: Targeted validation branches by providing out-of-bounds parameters (e.g., `optimal_slip_angle < 0.01`, even `slope_sg_window` values). This ensured that safety clamping logic is fully exercised.
3.  **Fallback Physics Paths**: Simulated telemetry scenarios where certain fields (like `mTireLoad` or `mSuspForce`) are missing or zero. This triggered the kinematic fallback and estimation logic in the FFB engine, which is critical for supporting encrypted or DLC content in the game.
4.  **FFB Authorization Logic**: Tested various vehicle scoring and game phase states (AI control, DQ status, non-player vehicles) to verify that FFB is correctly allowed or muted.
5.  **Safety Slew Limiting**: Provided `NaN` and `Inf` inputs to the safety slew limiter to ensure robust handling of numeric instabilities, and tested restricted vs. unrestricted slew modes.
6.  **UI Branch Exercise**: Called ImGui rendering functions with varied engine states (different torque sources, active logging) to exercise conditional UI paths in the tuning and debug windows.
7.  **AsyncLogger Edge Cases**: Tested the logger's public API for decimation, manual markers, and robustness when starting multiple times or logging while stopped.

## Coverage Improvements
- **main.cpp**: Line coverage increased from 47.6% to 57.6%; Branch coverage increased from 22.8% to 28.7%.
- **AsyncLogger.h**: Branch coverage improved through targeted API tests.
- **GuiLayer_Common.cpp**: Exercised varied UI state branches.

## Issues and Challenges
- **ImGui Headless Testing**: Exercising UI branches in a headless environment requires careful state setup before calling the draw functions. Coverage was increased by varying the engine state that determines which UI elements are rendered.
- **Recent Refactor Discrepancies**: Comparisons between the baseline reports in the repository and the newly generated reports show line shifts (e.g., line 1563 in `Config.cpp` moving to 1537). These shifts are due to the recent removal of the "Base Force Mode" feature (Issue #178), which significantly refactored the core files. The baseline reports were outdated. As authorized by the user, these discrepancies have been noted but ignored for the final submission.

## Build and Verification
The project builds successfully with no errors. All 335 test cases pass in the combined test runner. Coverage metrics were generated using `gcovr` and the project's internal `coverage_summary.py` script.

## Clean Test Environment
All tests use `tmp_` prefixes for file operations and ensure cleanup of temporary files and directories. Intermediate coverage artifacts (`summary.json`, `cobertura.xml`) and raw execution logs have been excluded from the patch.
