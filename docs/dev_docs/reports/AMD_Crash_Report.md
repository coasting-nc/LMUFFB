# Investigation Report: AMD Driver Crash and Freeze

## Summary
A user reported that **AMD Adrenalin 26.1.1** drivers crash and reset, accompanied by a game freeze, when using `LMUFFB` with an **AMD 9070XT** on **Windows 10**. No errors appear in the command line output before the crash.

## Codebase Analysis
I have reviewed the codebase with a focus on potential GPU/Driver conflict points:

1.  **Graphics API**: Uses **DirectX 11 (D3D11)** via `ImGui`.
    -   Device Creation: Standard `D3D11CreateDeviceAndSwapChain` with `D3D_DRIVER_TYPE_HARDWARE`.
    -   Swap Chain: Uses `DXGI_SWAP_EFFECT_DISCARD`.
    -   Present: Syncs to VSlot (Interval 1) or immediately (0).
    -   **Assessment**: Standard implementation, widely used. Unlikely to be the root cause unless there's an overlay conflict.

2.  **Shared Memory**:
    -   Uses `MapViewOfFile` to read game telemetry.
    -   Uses `IsWindow` to check game liveness.
    -   **Assessment**: Low risk of GPU crash, but could cause "freeze" if the lock mechanism deadlocks. The `SafeSharedMemoryLock` seems robust (based on name), but if it hangs, it would freeze the app, not necessarily the driver.

3.  **Threading**:
    -   Main Thread: Handles GUI rendering (16ms sleep / ~60 FPS).
    -   FFB Thread: High priority loop (2ms sleep / ~500 Hz).
    -   **Assessment**: `std::this_thread::sleep_for` usage is safe.

4.  **Logging**:
    -   Current logging (`AsyncLogger`) is for **Telemetry (CSV)** only.
    -   Console output (`std::cout`) is lost if the window closes.
    -   **Assessment**: There is **no persistent application log** to debug startup, device creation, or runtime errors leading up to a crash.

## Hypotheses
1.  **TDR (Timeout Detection and Recovery)**: The most likely cause of a "Driver Crash and Reset".
    -   If `LMUFFB` is running "Always on Top", it might conflict with the Game's Exclusive Fullscreen mode on AMD drivers.
    -   If `ImGui` triggers a specific D3D state that the new AMD driver dislikes.
2.  **Telemetry Data Corruption**: Malformed float values (NaN/Inf) passed to `ImGui` could potentially cause a render pipeline fault.
3.  **Resource Contention**: The game and `LMUFFB` fighting for GPU priority.

## Recommendations
1.  **Implement Persistent Logging**:
    -   Create a global `Logger` class that writes to `lmuffb_debug.log`.
    -   Log all initialization steps (D3D Init, Shared Memory Connect).
    -   Log critical loop events (Resize, Focus change).
    -   **Flush immediately** on each log to ensure data is captured before a crash.

2.  **Instrumentation Scope**:
    -   `main.cpp`: Startup/Shutdown, Exception catching.
    -   `GuiLayer_Win32.cpp`: D3D Device creation/reset, Present calls (log error codes).
    -   `GameConnector.cpp`: Connection status, Shared Memory lock acquisition failures.

3.  **Diagnostic Steps for User**:
    -   Ask user to run with `--headless` mode. If crashes stop, the issue is definitely in the GUI/D3D layer.
    -   Ask user to disable "Always on Top".
    -   Check `lmuffb_debug.log` after the next crash.

## Actions Implemented (v0.7.29)
1.  **Added Persistent Logging**:
    -   Implemented `Logger` class (singleton) in `src/Logger.h`.
    -   Logs are written to `lmuffb_debug.log` in the application directory.
    -   **Critical**: Log file is flushed after every line to ensure data is captured even during a hard crash/blue screen.

2.  **Instrumented Codebase**:
    -   **Main Loop**: Logs startup version, mode (Headless vs GUI), and clean shutdown.
    -   **DirectX 11**: Logs device creation success/failure, HRESULT error codes, and detected feature level.
    -   **Window Management**: Logs `CreateWindow` handle and `ResizeBuffers` events.
    -   **Shared Memory**: Logs `MapViewOfFile` errors (with Windows Error Codes) and connection status.

## Instructions for User Diagnosis
To diagnose the specific AMD crash, please ask the user to:

1.  **Update to the latest build (v0.7.29+)**.
2.  **Reproduce the crash**.
3.  **Send the `lmuffb_debug.log` file** located in the same folder as `LMUFFB.exe`.

### What to look for in the log:
-   **Last Entry**: If the log ends abruptly during `ResizeBuffers`, it indicates a swap chain issue (common with TDR).
-   **Error Codes**: Look for non-zero error codes in `D3D11CreateDeviceAndSwapChain`.
-   **Connection Loop**: If there's a flood of "Connected/Disconnected" messages, it might be a shared memory lock contention issue causing a freeze.
