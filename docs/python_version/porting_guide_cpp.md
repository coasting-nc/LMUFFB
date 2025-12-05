# Porting Guide: Python to C++

This guide outlines the steps required to port LMUFFB from Python to C++ for improved performance and lower latency.

## 1. Development Environment

*   **IDE**: Visual Studio 2022 (Community Edition or newer).
*   **Workload**: Desktop development with C++.
*   **SDKs**: Windows SDK (included with VS).

## 2. Shared Memory Access

In C++, you access the rFactor 2 Shared Memory directly using Windows APIs.

**Key APIs**:
*   `OpenFileMappingA`: Opens the named shared memory object.
*   `MapViewOfFile`: Maps the memory into your process's address space.

**Snippet**:
```cpp
#include <windows.h>
#include <iostream>
#include "rF2Telemetry.h" // You need the header with struct definitions

// Constants
const char* SHARED_MEMORY_NAME = "$rFactor2SMMP_Telemetry$";

int main() {
    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_READ, FALSE, SHARED_MEMORY_NAME);
    if (hMapFile == NULL) {
        std::cerr << "Could not open file mapping object (" << GetLastError() << ")." << std::endl;
        return 1;
    }

    auto* pTelemetry = (rF2Telemetry*)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, sizeof(rF2Telemetry));
    if (pTelemetry == NULL) {
        std::cerr << "Could not map view of file (" << GetLastError() << ")." << std::endl;
        CloseHandle(hMapFile);
        return 1;
    }

    // Main Loop
    while (true) {
        // Access data directly
        double engineRPM = pTelemetry->mEngineRPM;
        // ... calculation logic ...
        Sleep(2); // ~500Hz
    }

    UnmapViewOfFile(pTelemetry);
    CloseHandle(hMapFile);
    return 0;
}
```

## 3. Data Structures

You must define the `rF2Telemetry`, `rF2Wheel`, etc., structs exactly as they are in the Python code (which mirrors the C++ original).

*   **Alignment**: Ensure strict packing if the plugin uses it. Usually, standard alignment works, but `#pragma pack(push, 1)` might be needed if the plugin uses 1-byte packing. The standard rFactor 2 SDK usually relies on default alignment (4 or 8 bytes).
*   **Headers**: Refer to [The Iron Wolf's Plugin Source](https://github.com/TheIronWolfModding/rF2SharedMemoryMapPlugin) for the canonical header files (`rF2State.h`, etc.).

## 4. FFB Output (vJoy)

To replace `pyvjoy`, you will use the **vJoyInterface.dll** C SDK.

1.  **Download**: vJoy SDK from the vJoy website or GitHub.
2.  **Include**: `public.h` and `vjoyinterface.h`.
3.  **Link**: `vJoyInterface.lib`.

**Snippet**:
```cpp
#include "public.h"
#include "vjoyinterface.h"

// Initialize
UINT iInterface = 1;
AcquireVJD(iInterface);

// Update Axis (X Axis)
long min_val = 1;
long max_val = 32768;
// Calculate force (normalized -1.0 to 1.0)
double force = ...; 
long axis_val = (long)((force + 1.0) * 0.5 * (max_val - min_val) + min_val);

SetAxis(axis_val, iInterface, HID_USAGE_X);

// Cleanup
RelinquishVJD(iInterface);
```

## 5. FFB Engine Logic

Porting `ffb_engine.py` is straightforward.

*   `self.smoothing`: Implement using a simple exponential moving average or a low-pass filter class.
*   `mGripFract`: Directly accessible from the struct.
*   **Math**: Use `std::min`, `std::max`, `std::abs` from `<algorithm>` and `<cmath>`.

## 6. Optimization

*   **High Resolution Timer**: Use `QueryPerformanceCounter` for timing loop execution to ensure steady 400Hz updates, replacing `Sleep(2)` which can be imprecise.
*   **DirectInput**: For the long-term roadmap, replace vJoy with direct DirectInput FFB commands (`IDirectInputDevice8::SendForceFeedbackCommand`), which allows sending constant force packets without a virtual driver middleware.
