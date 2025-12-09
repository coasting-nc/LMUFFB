# Question

See the full code and documentation for my force feedback (FFB) app for Le Mans Ultimate (LMU). The app had the issue that LMU, until version 1.1 did not made available certain data through shared memory. Now version 1.2 has been released, with a new shared memory interface. I've put the two files of the interface in the codebase, here: src\lmu_sm_interface (src\lmu_sm_interface\InternalsPlugin.hpp and src\lmu_sm_interface\SharedMemoryInterface.hpp). One particular value that was needed before and was not available was the tire load data. But all other tire related date were missing, like grip fraction, patch acceleration, slip, etc.

Your task is to tell me which new telemetry / shared memory are now available that we could use. Tell me which ones replace older values that we could not use. Tell me about new ones that were introduced. And suggest how the code needs to be changed to use the new interface, including design changes, if any. 
Finally, do a summary assessment on if we can now calculate proper force feedback from our app using this interface.


# Answer

Based on the analysis of the new header files (`InternalsPlugin.hpp` and `SharedMemoryInterface.hpp`) provided in `src\lmu_sm_interface`, here is the assessment of the new LMU 1.2 Shared Memory Interface and how to adapt your application.

### 1. Assessment: Can we now calculate proper FFB?

**YES.**
The new interface explicitly exposes the tire physics data that was previously missing or zeroed out in the legacy interface.

*   **Tire Load (`mTireLoad`):** Now available directly in `TelemWheelV01`. This is critical for your "Slide Texture", "Lockup", and "Bottoming" effects.
*   **Grip Fraction (`mGripFract`):** Available. This enables your "Understeer" effect to work dynamically rather than relying on the fallback.
*   **Patch Velocity (`mLateralPatchVel`):** Available. This allows for the accurate, physics-based frequency generation for tire scrubbing sounds/feel.

### 2. Telemetry Changes: Old vs. New

The new interface uses structures that are very similar to the internal rFactor 2 plugin SDK, but wrapped in a new memory layout.

| Data Point | Old (`rF2Data.h`) | New (`InternalsPlugin.hpp`) | Status |
| :--- | :--- | :--- | :--- |
| **Tire Load** | `mTireLoad` (Often 0.0) | `mTireLoad` | **Available** (Newtons) |
| **Grip** | `mGripFract` (Often 0.0) | `mGripFract` | **Available** |
| **Steering Force** | `mSteeringArmForce` | `mSteeringShaftTorque` | **Renamed** (See note below) |
| **Slide Speed** | `mLateralPatchVel` | `mLateralPatchVel` | **Available** |
| **Acceleration** | `mLocalAccel` | `mLocalAccel` | Same |
| **Deflection** | `mVerticalTireDeflection` | `mVerticalTireDeflection` | Same |

**Critical Note on Steering Force:**
In `InternalsPlugin.hpp`, the variable is `mSteeringShaftTorque`. The comment explicitly states: *"torque around steering shaft (used to be mSteeringArmForce, but that is not necessarily accurate for feedback purposes)"*.
*   **Action:** You should use `mSteeringShaftTorque` as your primary `game_force`. You may need to check the scale (it might be in Nm instead of raw arm force), so you might need to adjust your `m_gain` or normalization factor (currently 4000.0).

### 3. Architectural Changes Required

The way you access data has changed fundamentally.

1.  **Memory Map Name:**
    *   Old: `$rFactor2SMMP_Telemetry$`
    *   New: `LMU_Data`
2.  **Synchronization (Crucial):**
    *   Old: Checked `mVersionUpdateBegin` vs `mVersionUpdateEnd`.
    *   New: Uses a **Spinlock/Mutex** mechanism defined in `SharedMemoryLock`. You **must** lock, copy the data to a local buffer, and unlock.
3.  **Player Indexing:**
    *   Old: The map usually contained just the player or a fixed structure.
    *   New: The map contains an array of 104 vehicles (`TelemInfoV01 telemInfo[104]`). You must read `playerVehicleIdx` to find the user's car.

---

### 4. Implementation Plan

Here is how to modify your code.

#### A. Update `FFBEngine.h`
Change the input type from `rF2Telemetry` to the new `TelemInfoV01`.

```cpp
// FFBEngine.h
#include "src/lmu_sm_interface/InternalsPlugin.hpp" // Include the new header

class FFBEngine {
public:
    // ... existing members ...

    // Change signature to use new struct
    double calculate_force(const TelemInfoV01* data) {
        if (!data) return 0.0;
        
        double dt = data->mDeltaTime;
        
        // Access wheels (Note: struct name is mWheel, not mWheels)
        const TelemWheelV01& fl = data->mWheel[0];
        const TelemWheelV01& fr = data->mWheel[1];

        // Update Steering Force access
        // Note: Check if this needs scaling compared to old ArmForce
        double game_force = data->mSteeringShaftTorque; 

        // ... Rest of logic remains mostly the same ...
        // Ensure you use the new member names if they differ slightly
        // e.g., mLocalAccel is still mLocalAccel.
        
        // ...
    }
};
```

#### B. Update `main.cpp`
This requires the most work to implement the locking and indexing logic.

```cpp
// main.cpp
#include "src/lmu_sm_interface/SharedMemoryInterface.hpp" // New Interface

// Globals
SharedMemoryLayout* g_pSharedMemLayout = nullptr;
SharedMemoryObjectOut g_localData; // Local copy to avoid locking for too long
std::optional<SharedMemoryLock> g_smLock;

// ...

int main(int argc, char* argv[]) {
    // ... [Init GUI / DI code] ...

    // 1. Setup Shared Memory (New LMU Name)
    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_READ, FALSE, LMU_SHARED_MEMORY_FILE);
    
    if (hMapFile && !g_pSharedMemLayout) {
        g_pSharedMemLayout = (SharedMemoryLayout*)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, sizeof(SharedMemoryLayout));
    }

    // 2. Initialize Lock
    g_smLock = SharedMemoryLock::MakeSharedMemoryLock();
    if (!g_smLock.has_value()) {
        std::cerr << "Failed to init LMU Shared Memory Lock" << std::endl;
    }

    // ... [Start Threads] ...
}

void FFBThread() {
    // ... [Setup] ...

    while (g_running) {
        if (g_ffb_active && g_pSharedMemLayout && g_smLock.has_value()) {
            
            // --- CRITICAL SECTION: READ DATA ---
            bool data_valid = false;
            
            // 1. Lock
            g_smLock->Lock();
            
            // 2. Copy to local storage (Fast copy)
            // We use the helper function provided in the header
            CopySharedMemoryObj(g_localData, g_pSharedMemLayout->data);
            
            // 3. Unlock
            g_smLock->Unlock();
            
            // 4. Find Player
            // The struct has a flag for this
            if (g_localData.telemetry.playerHasVehicle) {
                uint8_t idx = g_localData.telemetry.playerVehicleIdx;
                if (idx < 104) {
                    // Get pointer to specific car data
                    TelemInfoV01* pPlayerTelemetry = &g_localData.telemetry.telemInfo[idx];
                    
                    // Calculate FFB
                    double force = 0.0;
                    {
                        std::lock_guard<std::mutex> lock(g_engine_mutex);
                        force = g_engine.calculate_force(pPlayerTelemetry);
                    }
                    
                    // Send to DirectInput...
                    DirectInputFFB::Get().UpdateForce(force);
                    
                    // vJoy logic...
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    // ...
}
```

### 5. Summary of Changes Checklist

1.  **Include Path:** Ensure `src/lmu_sm_interface` is in your include path.
2.  **Struct Update:** Replace `rF2Data.h` usage with `InternalsPlugin.hpp`.
3.  **Logic Update:** In `FFBEngine`, rename `mWheels` to `mWheel` and `mSteeringArmForce` to `mSteeringShaftTorque`.
4.  **Main Loop:** Implement the `SharedMemoryLock` -> `Copy` -> `Find Player Index` pattern.
5.  **Sanity Checks:** You can likely relax the "Missing Load" warnings, but keep the logic: `if (mTireLoad < 1.0) ...` just in case the user is in the menu or a loading state.

### 6. Conclusion

This update is exactly what was needed. By switching to the `LMU_Data` map and using the `SharedMemoryLock`, you will bypass the broken legacy plugin entirely. You will have access to **real-time Tire Load and Grip**, allowing your "Slide Texture" and "Understeer" effects to function based on actual physics rather than estimates or fallbacks.
