# Revised Implementation Plan: Fix Focus-Based FFB Throttling (Issue #100)

## Context
A regression was identified where FFB becomes "dull" and loses detail when the LMUFFB window is minimized or covered. This is caused by an aggressive power-saving sleep logic in the main loop that throttles the Win32 message loop to 10Hz when the window is not focused.

## Proposed Changes

1. **Modify `src/main.cpp`**: Remove the conditional sleep logic in the main loop. Always sleep for 16ms (approx. 60Hz) to ensure a consistent message loop rate for DirectInput background operations.
2. **Verify `src/main.cpp`**: Use `read_file` to confirm the changes.
3. **Modify `src/GuiLayer_Win32.cpp`**: Update `GuiLayer::Render` to return a simple "keep running" status based on `g_running` rather than focus state.
4. **Verify `src/GuiLayer_Win32.cpp`**: Use `read_file` to confirm the changes.
5. **Modify `src/GuiLayer_Linux.cpp`**: Update `GuiLayer::Render` to match the Win32 behavior.
6. **Verify `src/GuiLayer_Linux.cpp`**: Use `read_file` to confirm the changes.
7. **Create Regression Test**: Create `tests/test_issue_100_timing.cpp` that mocks the GUI layer and verifies the main loop frequency stays at ~60Hz regardless of the "active" state previously used.
8. **Verify Test File**: Use `read_file` to confirm the test file creation.
9. **Update VERSION**: Increment the version in `VERSION` to `0.7.36`.
10. **Build and Run All Tests**: Use `cmake` to build and run the combined test suite to ensure no regressions.
11. **Complete Pre-Commit Steps**: Ensure proper testing, verification, review, and reflection are done.
12. **Submit**: Create a new branch and submit the changes with a descriptive commit message.

## FFB Effect Impact Analysis
- **High-Freq Textures**: Detail will be restored when the window is backgrounded.
- **FFB Thread Stability**: `SetParameters` calls in the FFB thread will no longer be delayed by a throttled main thread message loop.
- **Overall Feel**: Consistent FFB performance regardless of window state.
