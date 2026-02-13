# Investigation Report: FFB Regression (Issue #100)

## Overview
A regression was reported in version 0.7.34 where Force Feedback (FFB) becomes "dull" and loses detail when the LMUFFB window is minimized or covered by another window. This report analyzes the changes between version 0.7.26 and 0.7.34 to identify the root cause.

## Findings

### 1. Root Cause Identification
The investigation identified a critical flaw in the main application loop in `src/main.cpp`.

```cpp
    while (g_running) {
        bool active = GuiLayer::Render(g_engine);
        if (active) std::this_thread::sleep_for(std::chrono::milliseconds(16));
        else std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
```

This logic was likely introduced during the GUI refactoring in version 0.7.32 (Introduction of `IGuiPlatform`). It attempts to save CPU by sleeping for 100ms when the GUI is "inactive".

In `src/GuiLayer_Win32.cpp` (and `src/GuiLayer_Linux.cpp`), `GuiLayer::Render` determines "activity" based on focus:
```cpp
    return ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow) || ImGui::IsAnyItemActive();
```

When the user is playing the game (the primary use case), the LMUFFB window is NOT focused. This causes the main loop to sleep for 100ms between frames, effectively throttling the Win32 message loop to 10Hz.

### 2. Impact on FFB
While FFB calculation happens in a separate thread (`FFBThread`), DirectInput operations (specifically `SetParameters` used in `UpdateForce`) often depend on the message loop of the thread that created the device (the main thread).

Throttling the main thread to 10Hz causes:
- **Reduced FFB Update Rate**: `SetParameters` may block or wait for the message loop to process, causing the FFB thread (which should run at 400Hz) to be dragged down to the message loop's 10Hz rate.
- **Loss of Detail**: High-frequency FFB effects like "Road Texture" and "Slide Rumble" (which operate in the 40Hz - 200Hz range) are completely lost or aliased when the update rate drops to 10Hz, leading to the reported "dull" feeling.
- **Exclusive Mode Issues**: DirectInput exclusive mode is highly sensitive to window state and message loop health. Throttling the message loop can cause DirectInput to drop exclusivity or fail to re-acquire it.

### 3. Visibility vs. Minimization
The user reported that the issue is not present when the window is visible on a 4th monitor, even if unfocused.
- **Visible (Unfocused)**: The loop still calls `Present(1, 0)`, which waits for VSync (~16ms). The total cycle time is ~116ms (100ms sleep + 16ms wait).
- **Minimized/Covered**: `Present` may return immediately (0ms) because nothing is being rendered. The loop cycles exactly every 100ms.
- **Internal Focus**: ImGui may retain "internal" focus if it was the last thing the user touched *within that app*, which might return `active = true` even if the Win32 window is not the foreground window. However, minimizing the window definitely clears this state.

### 4. Regression Analysis
Version 0.7.26 did not have this aggressive power-saving sleep logic. The refactoring in 0.7.32 to support cross-platform GUI layers introduced the `active` return value and the subsequent 100ms sleep in `main.cpp`, which is inappropriate for a hardware driver application that must maintain low latency in the background.

## Conclusion
The issue is a confirmed regression caused by the 100ms sleep in the main loop when the window is not focused.

### Originating Commit
The regression was introduced in commit **`b1eb6e2754bd968c4ba3504df98bd63d4d7723a8`** ("Port GUI to Linux using GLFW and OpenGL 3"), which was part of the **v0.7.32** release.

This commit refactored the GUI layer to support multiple platforms and introduced a power-saving mechanism in `src/main.cpp`. The mechanism was designed to reduce CPU usage when the application window was not active by increasing the sleep duration between loop iterations from 16ms to 100ms. However, it failed to account for DirectInput's dependency on the main thread's message loop frequency for background FFB updates.

## Recommended Fix
1. **Eliminate the 100ms sleep**: The main loop should run at a consistent rate (e.g., always 16ms / 60Hz) to ensure the message loop remains healthy for DirectInput.
2. **Remove focus-based throttling**: A background driver should not throttle itself based on window focus.
3. **Thread Priority**: (Optional but recommended) Set the `FFBThread` to a higher priority to ensure it is not throttled by Windows when the app is in the background.

## Implementation Plan
I will proceed with an implementation plan to remove this throttling and ensure consistent FFB performance regardless of window state.
