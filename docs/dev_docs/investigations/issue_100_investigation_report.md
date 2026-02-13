# Investigation Report: FFB Regression (Issue #100)

## Overview
A regression was reported in version 0.7.34 where Force Feedback (FFB) becomes "dull" and loses detail when the LMUFFB window is minimized or covered by another window. This report identifies the root cause introduced during recent refactoring efforts.

## Findings

### 1. Regression Origin
The investigation identified that the regression was introduced in commit **`b1eb6e2754bd968c4ba3504df98bd63d4d7723a8`** ("Port GUI to Linux using GLFW and OpenGL 3"), which was part of the **v0.7.32** release cycle. While the 100ms sleep existed in earlier versions, this commit formalized the `bool active` return logic from the GUI layer across the new platform abstraction, which became the primary trigger for the reported behavior.

### 2. Root Cause Identification
The investigation identified a critical flaw in the main application loop in `src/main.cpp`.

```cpp
    while (g_running) {
        bool active = GuiLayer::Render(g_engine);
        if (active) std::this_thread::sleep_for(std::chrono::milliseconds(16));
        else std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
```

This logic, although present in older versions, became a critical failure point after the GUI refactoring in version 0.7.32. The `GuiLayer::Render` function determines "activity" based on focus:
```cpp
    return ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow) || ImGui::IsAnyItemActive();
```

When the user is playing the game (the primary use case), the LMUFFB window is NOT focused. This causes the main loop to sleep for 100ms between frames, effectively throttling the Win32 message loop to 10Hz.

### 2. Impact on FFB
While FFB calculation happens in a separate thread (`FFBThread`), DirectInput operations (specifically `SetParameters` used in `UpdateForce`) depend on the Win32 message pump of the thread that created the device (the main thread).

Throttling the main thread to 10Hz causes:
- **Reduced FFB Update Rate**: `SetParameters` calls from the FFB thread are delayed until the main loop processes messages, effectively capping the hardware update rate to 10Hz.
- **Loss of Detail**: High-frequency FFB effects like "Road Texture" and "Slide Rumble" are completely lost or aliased at a 10Hz update rate, leading to the reported "dull" feeling.
- **Exclusive Mode Issues**: DirectInput exclusive mode requires a healthy message loop to maintain its connection to the hardware. Throttling can cause the driver to lose exclusive access.

### 3. Conclusion
The issue is a regression in intended behavior. While the 100ms sleep was designed for "Lazy Rendering", it is incompatible with a low-latency hardware driver that must remain responsive in the background.

## Recommended Fix
1.  **Eliminate conditional sleep**: The main loop should run at a constant ~60Hz (16ms) to ensure the Win32 message pump remains responsive for DirectInput background operations.
2.  **Update GUI layer**: `GuiLayer::Render` should not be used to drive application-wide throttling.
