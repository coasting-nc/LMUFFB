# Report: DirectInput FFB Sentinel State Bug

## Overview
A functional bug was identified in the `DirectInputFFB` class where the member variable `m_last_force` is not reset when a device is released or a new one is selected. This results in the potential suppression of the initial force feedback (FFB) commands sent to a newly initialized device.

## Technical Context
The `DirectInputFFB` class uses a "last-value" optimization to minimize redundant calls to the USB driver. Every update, the requested force is scaled and compared against a cached value:

```cpp
// From src\ffb\DirectInputFFB.cpp
long magnitude = static_cast<long>(normalizedForce * 10000.0);
if (magnitude == m_last_force) return false; // Skip if same as last time
m_last_force = magnitude;
```

To ensure the **very first** command is actually sent, `m_last_force` is initialized to a sentinel value (`-999999`) that falls outside the standard DirectInput range of `[-10000, 10000]`.

## The Issue
The variable `m_last_force` is **only** initialized in the class definition (header) and is **never reset** during the lifetime of the singleton instance.

### Step-by-Step Scenario:
1.  **User Selects Device A**: Starts driving. The force magnitude is `5000` (50%).
2.  **`m_last_force` updates**: It is now `5000`.
3.  **User Releases Device A**: (e.g., via GUI unbind). `ReleaseDevice()` is called. `m_pDevice` is nulled, but `m_last_force` remains `5000`.
4.  **User Selects Device B**: (e.g., via GUI bind). `SelectDevice()` initializes the new hardware. The physical motor state for Device B is `0.0`.
5.  **User Resumes Driving**: The physics engine requests `0.5` (5000). 
6.  **The Bug Traps**: `UpdateForce(0.5)` calculates `magnitude = 5000`. It then checks `if (5000 == 5000)` which evaluates to **true**.
7.  **Result**: The function returns `false` without calling the driver. Device B stays at `0` torque until the user turns the wheel or hits a bump that changes the requested force.

## Safety Analysis
| Category | Status | Evaluation |
| :--- | :--- | :--- |
| **Memory Safety** | ✅ PASS | No risk of buffer overflows or null pointer dereferences (the skip prevents access). |
| **Logic/UX Safety** | ❌ FAIL | Causes unpredictable "dead FFB" on device transitions. |
| **Numerical Safety** | ✅ PASS | `-999,999` fits comfortably in a 32-bit `long` and will not collide with valid magnitude values. |

## Recommended Fix
Reset `m_last_force` to its sentinel value in `DirectInputFFB::ReleaseDevice()` in `src\ffb\DirectInputFFB.cpp`:

```cpp
void DirectInputFFB::ReleaseDevice() {
#ifdef _WIN32
    // ... release logic ...
#endif
    m_active = false;
    m_isExclusive = false;
    m_deviceName = "None";
    m_last_force = -999999; // <--- FIX: Ensure next device starts fresh
}
```

## Maintenance Note
While `-999999` is safe, using a named constant or `std::numeric_limits<long>::min()` would improve code clarify and prevent future maintenance errors if the scaling range (10,000) were ever doubled or changed.
