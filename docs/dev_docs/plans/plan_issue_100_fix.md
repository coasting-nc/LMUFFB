# Revised Implementation Plan: Fix Focus-Based FFB Throttling (Issue #100)

## Context
A regression was identified where FFB becomes "dull" and loses detail when the LMUFFB window is minimized or covered. This is caused by an aggressive power-saving sleep logic in the main loop that throttles the Win32 message loop to 10Hz when the window is not focused.

## Proposed Changes

1. **Modify `src/main.cpp`**: Remove the conditional sleep logic in the main loop. Always sleep for 16ms (approx. 60Hz) to ensure a consistent message loop rate for DirectInput background operations.
2. **Modify `src/GuiLayer_Win32.cpp`**: Update `GuiLayer::Render` to always return `true` while the app is running.
3. **Modify `src/GuiLayer_Linux.cpp`**: Update `GuiLayer::Render` and its headless stub to always return `true` while the app is running.
4. **Create Regression Test**: `tests/test_issue_100_timing.cpp` verifies that `GuiLayer::Render` always returns `true`.
5. **Fix Test Summary**: Update `tests/main_test_runner.cpp` with `std::flush` and a delay to ensure the summary is visible on Windows.
6. **Update VERSION**: Increment the version in `VERSION` to `0.7.36`.

## Implementation Notes
- **Root Cause Analysis:** The investigation revealed that the 100ms background sleep was introduced as early as v0.5.14 (commit `267822c6`). However, it only became a critical regression in v0.7.32 because the refactoring formalized the focus-based "Lazy Rendering" logic. DirectInput background acquisition relies on the message pump, so throttling the main loop to 10Hz severely degraded FFB update rates and detail.
- **Slope Detection Bug:** A separate bug was discovered where the "Slope Threshold" GUI slider was disconnected from the physics engine. This has been documented in `docs/dev_docs/investigations/slope_detection_threshold_bug.md` but is NOT fixed in this patch as per user instructions.
- **Test Infrastructure:** Replaced Win32-specific `Sleep()` with idiomatic `std::this_thread::sleep_for()` in the test runner. Added comprehensive cleanup of temporary test artifacts (logs, ini files) to ensure a clean workspace after testing.
