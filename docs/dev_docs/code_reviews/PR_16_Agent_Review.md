# Code Review: PR #16 (Auto-connect to LMU) - Agent Review

## Overview
This review focuses on the changes in `src/GameConnector.cpp` and `src/GameConnector.h` related to the auto-connect feature, specifically evaluating resource management, state consistency, and thread safety.

## Detailed Analysis

### 1. Resource Management
The introduction of the `Disconnect()` method significantly improves resource management by centralizing cleanup logic.

*   **Positive**: `Disconnect()` correctly handles `UnmapViewOfFile`, `CloseHandle` (for both `m_hMapFile` and `m_hProcess`), and resets the `m_smLock` optional.
*   **Positive**: `TryConnect()` calls `Disconnect()` at the beginning and on failure paths, preventing resource leaks during retries.
*   **Destructor**: The destructor now calls `Disconnect()`, ensuring proper cleanup on exit.
*   **Issue**: In `IsConnected()`, when the process is detected as exited (`WAIT_OBJECT_0` or `WAIT_FAILED`), `m_connected` is set to `false`, but `Disconnect()` is **not called**. This means the handles (`m_hProcess`, `m_hMapFile`) and memory view remain valid/open until the *next* call to `TryConnect()`. While `TryConnect()` will eventually clean them up, it is cleaner to release resources immediately upon detection of disconnection.

### 2. State Consistency
The `m_connected` flag is generally managed correctly, acting as a high-level gate. However, the internal state (`m_pSharedMemLayout`, `m_smLock`) must remain consistent with `m_connected`.

*   **Risk**: If `Disconnect()` is called (setting `m_connected = false` and nullifying pointers), any subsequent call to `CopyTelemetry` or `IsInRealtime` must properly check `m_connected` or the pointers before access.
*   **Current Checks**:
    *   `CopyTelemetry`: Checks `!m_connected || !m_pSharedMemLayout || !m_smLock.has_value()` before proceeding. Correct.
    *   `IsInRealtime`: Checks `!m_connected || !m_pSharedMemLayout || !m_smLock.has_value()` before proceeding. Correct.

### 3. Thread Safety (CRITICAL)
This is the most significant issue found. The `GameConnector` is a Singleton accessed by two concurrent threads:
1.  **Main/FFB Thread**: Runs at ~400Hz (in `main.cpp`), calling `CopyTelemetry()` and `IsInRealtime()`.
2.  **GUI Thread**: Runs at ~60Hz (in `GuiLayer.cpp`), calling `IsConnected()` and `TryConnect()` (which calls `Disconnect()`).

**The Race Condition:**
If the GUI thread calls `TryConnect()` -> `Disconnect()` while the FFB thread is inside `CopyTelemetry()`:
1.  FFB Thread passes the `if (!m_connected ...)` check.
2.  **Context Switch to GUI Thread**.
3.  GUI Thread calls `Disconnect()`, which:
    *   Unmaps `m_pSharedMemLayout`.
    *   Resets `m_smLock`.
4.  **Context Switch back to FFB Thread**.
5.  FFB Thread attempts to use `m_smLock->Lock()` or access `m_pSharedMemLayout`.
6.  **CRASH**: Access violation (reading unmapped memory or dereferencing invalid optional/null pointer).

**Fix Requirement**: Access to shared state (`m_connected`, `m_pSharedMemLayout`, `m_smLock`, handles) must be protected by a `std::mutex`.

### 4. Error Handling
*   **TryConnect**: Error logging to `std::cerr` is good for debugging. Logic correctly handles failure at each stage (Map, View, Lock, Process).
*   **IsConnected**: `WaitForSingleObject` with `0` timeout is the correct non-blocking way to check process state.

## Recap of Issues and Recommendations

### Critical Issues
1.  **Thread Safety/Race Condition**: The `GameConnector` class is **not thread-safe**. Concurrent access by the FFB loop (reading data) and the GUI loop (managing connection/disconnection) causes a high probability of crash during a reconnection event.
    *   **Recommendation**: Add a `std::mutex m_mutex` to `GameConnector`. Lock this mutex in `TryConnect`, `Disconnect`, `IsConnected`, `CopyTelemetry`, and `IsInRealtime`.

### Improvements
2.  **Immediate Cleanup**: `IsConnected()` detects process exit but leaves handles open.
    *   **Recommendation**: Call `Disconnect()` inside `IsConnected()` when `WaitForSingleObject` indicates the process is dead. This ensures resources are freed immediately.

### Code Quality
3.  **Mutable Members**: `m_hProcess` and `m_connected` are mutable to allow modification in `IsConnected()` (a const method). With the introduction of the mutex (which will also need to be `mutable`), this pattern is acceptable, though making `IsConnected` non-const might be cleaner design if strictly following const-correctness isn't required for the interface.

---
**Status**: **CHANGES REQUIRED**
The thread safety issue must be addressed before this code is production-ready.
