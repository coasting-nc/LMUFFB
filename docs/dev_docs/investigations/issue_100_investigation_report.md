# Investigation Report: FFB Regression (Issue #100)

## Overview
A regression was reported in version 0.7.34 where Force Feedback (FFB) becomes "dull" and loses detail when the LMUFFB window is minimized or covered by another window. This report identifies the root cause introduced during recent refactoring efforts.

## Findings

### 1. Regression Origin
The investigation identified that the aggressive 100ms background sleep was actually introduced much earlier, in commit **`267822c66802e5e40e6c387b3724c3e80f97906d`** (v0.5.14), when the application structure was first organized into the `src/` directory.

However, it only became a critical regression in the **v0.7.32** release cycle (around commit **`b1eb6e2754bd968c4ba3504df98bd63d4d7723a8`**).

### 2. Why it became a problem in v0.7.32
The v0.7.32 refactoring (Linux Port & Platform Abstraction) formalized the `bool active = GuiLayer::Render()` return logic. Before this refactoring, the monolithic `GuiLayer.cpp` was more tightly coupled with the Win32 message pump.

The refactoring made the following changes that exacerbated the issue:
- **Platform Abstraction**: The `IGuiPlatform` interface now dictates when the main loop should sleep. The Windows implementation specifically uses `ImGui::IsWindowFocused(...)` to determine activity.
- **Message Loop Formalization**: By moving the `PeekMessage` loop inside `GuiLayer_Win32.cpp` and gating the main loop's frequency based on its return value, the application effectively tied **DirectInput hardware performance** to **ImGui focus state**.
- **DirectInput Synchronization**: DirectInput devices acquired in `DISCL_BACKGROUND` mode (as LMUFFB does) still rely on the message queue of the window they are associated with. When the main loop sleeps for 100ms, the message queue is only drained at 10Hz. This causes `SetParameters` calls from the FFB thread to be delayed or throttled by the Windows OS to match the message pump's frequency, resulting in the "dull" and "detail-less" feeling reported by the user.

In earlier versions, despite the sleep being present, there was enough "activity noise" or different message handling in the monolithic GUI code that often kept the loop running at 60Hz. The cleaner v0.7.32 implementation made the "Lazy Rendering" optimization too efficient, revealing the underlying architectural flaw.

### 3. Root Cause Identification
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

### 4. Impact on FFB
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
