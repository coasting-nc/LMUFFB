# Code Review: PR #16 (Auto-connect to LMU)

## Overview
This PR introduces an auto-connect feature that periodically attempts to connect to the LMU shared memory and process. It also handles process exit detection to automatically reset the connection state.

## Implementation Review

### `src/GameConnector.cpp` & `src/GameConnector.h`

**1. Resource Leaks (Critical)**
There is a logic flaw in `TryConnect()` that causes resource leaks (handles and memory views) when:
*   `TryConnect()` fails at the `OpenProcess` stage.
*   `TryConnect()` is called after a disconnect (detected by `IsConnected()`).

In both cases, `OpenFileMappingA` and `MapViewOfFile` are called, overwriting the existing `m_hMapFile` and `m_pSharedMemLayout` without closing/unmapping the previous ones.

**2. Destructor Cleanup**
The `~GameConnector()` destructor cleans up the shared memory handles but fails to close `m_hProcess`. This is a handle leak.

**3. State Management**
The `mutable` keyword usage for `m_connected` and `m_hProcess` inside `IsConnected()` is a necessary evil given the current design, but it can be improved by centralizing the cleanup logic.

### `src/GuiLayer.cpp`

**1. Connection Logic**
The usage of a static `last_check_time` correctly implements a polling interval. The behavior upon disconnection (immediate first retry, then 2s intervals) provides a good user experience.

## Recommendations

### Required Fixes

1.  **Implement a `Disconnect()` method** in `GameConnector` to centralize cleanup.
    ```cpp
    void GameConnector::Disconnect() {
        if (m_pSharedMemLayout) {
            UnmapViewOfFile(m_pSharedMemLayout);
            m_pSharedMemLayout = nullptr;
        }
        if (m_hMapFile) {
            CloseHandle(m_hMapFile);
            m_hMapFile = NULL;
        }
        if (m_hProcess) {
            CloseHandle(m_hProcess);
            m_hProcess = NULL;
        }
        m_smLock.reset();
        m_connected = false;
    }
    ```

2.  **Update `TryConnect()`** to ensure a clean slate before attempting connection, avoiding leaks.
    ```cpp
    bool GameConnector::TryConnect() {
        if (m_connected) return true;
        
        // Ensure we don't leak handles from a previous partial/failed attempt
        Disconnect(); 

        // ... existing logic ...
        
        // On failure paths, call Disconnect() to clean up partial resources
        // e.g. if OpenProcess fails:
        if (!m_hProcess) {
             Disconnect();
             return false;
        }
    }
    ```

3.  **Update `IsConnected()`** to use `Disconnect()` when it detects the process has exited.

4.  **Update Destructor** to call `Disconnect()` (or ensure `m_hProcess` is closed).

### Documentation & Process

1.  **Changelog**: Update `CHANGELOG.md` with the new feature.
2.  **Version**: Bump the version in `VERSION` file (e.g., v0.6.39).
3.  **Tests**: While unit testing Windows API calls is difficult, ensure manual verification of the reconnect loop (start game, close game, restart game) confirms stability and no memory accumulation.

## Conclusion
The feature is valuable, but the current implementation introduces resource leaks that must be addressed before merging, especially for a long-running application where the user might restart the game multiple times.
