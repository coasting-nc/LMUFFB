# Refactoring Proposal: Moving Non-Physics Functions out of `FFBEngine.cpp`

## 1. Introduction

`src/ffb/FFBEngine.cpp` is currently quite long and handles multiple responsibilities. While its core purpose should be the real-time calculation of Force Feedback (FFB) based on telemetry and physics, it has accumulated responsibilities related to metadata management, safety enforcement, and diagnostic data buffering. 

This document critically evaluates the choice of moving specific functions out of this file and proposes an architectural path forward.

## 2. Critical Discussion on Moving the Functions

Moving the following functions out of `FFBEngine.cpp` is highly recommended as it strictly aligns with the **Single Responsibility Principle (SRP)**. By isolating these concerns, `FFBEngine.cpp` can be dedicated solely to high-frequency physics and signal processing.

### A. Metadata Management
**Functions:** `UpdateMetadata`, `UpdateMetadataInternal`
*   **Discussion:** These functions manage the state of the FFB Engine regarding the current vehicle class, vehicle name, and track name. They handle low-frequency state changes (e.g., car swaps) and trigger static load reference initializations. They do not directly compute forces. Keeping them in the main calculation file adds unnecessary state management clutter.

### B. Safety and Protection Layer
**Functions:** `TriggerSafetyWindow`, `IsFFBAllowed`, `ApplySafetySlew`
*   **Discussion:** These functions constitute a "Safety Layer". They monitor the game phase, telemetry quality, and output force slew rates to prevent hardware damage or user injury (e.g., muting the FFB in the garage, detecting massive torque spikes, limiting slew rates). This is a distinct domain of concern. The physics engine should compute the *ideal* force, and the safety layer should *filter/clamp* it. Mixing the two makes the physics formulas harder to read and test.

### C. Diagnostics and Debugging
**Functions:** `GetDebugBatch`
*   **Discussion:** This function is entirely dedicated to the presentation and diagnostic layer (e.g., passing FFB snapshots to the GUI). Data consumption logic should not reside alongside the producer's complex math logic.

---

## 3. Proposal for Refactoring

There are two primary approaches for this refactoring: **Class Extraction** (preferred for architecture) and **Implementation Splitting** (easier for minimal invasiveness).

### Option A: Class Extraction (Highly Recommended)
Instead of just moving functions to different files, extract these responsibilities into dedicated classes. `FFBEngine` will then hold instances of these classes and delegate work to them.

1.  **`FFBMetadataManager` (or `FFBState`)**
    *   **File:** `src/ffb/FFBMetadataManager.cpp` / `.h`
    *   **Role:** Store the current vehicle class, name, and track. Expose methods like `Update(SharedMemoryObjectOut)` and `HasVehicleChanged()`.

2.  **`FFBSafetyMonitor`**
    *   **File:** `src/ffb/FFBSafetyMonitor.cpp` / `.h`
    *   **Role:** Encapsulate `m_safety` state.
    *   **Methods:** `IsAllowed(...)`, `ApplySlew(...)`, `TriggerWindow(...)`.
    *   **Integration:** `FFBEngine` calls `safetyMonitor.ApplySlew(target_force, dt)` right before outputting the final force.

3.  **`FFBDebugBuffer` (or `FFBSnapshotQueue`)**
    *   **File:** `src/ffb/FFBDebugBuffer.cpp` / `.h`
    *   **Role:** Thread-safe circular buffer for snapshots.
    *   **Methods:** `Push(const FFBSnapshot&)`, `GetBatch()`.

### Option B: Implementation Splitting (Partial Refactor)
If you wish to keep the functions as members of the `FFBEngine` class (to avoid header rewrites and dependency injection changes), you can simply implement them in separate `.cpp` files.
*   Move `UpdateMetadata` and `UpdateMetadataInternal` to `src/ffb/FFBEngine_Metadata.cpp`.
*   Move `TriggerSafetyWindow`, `IsFFBAllowed`, and `ApplySafetySlew` to `src/ffb/FFBEngine_Safety.cpp`.
*   Move `GetDebugBatch` to `src/ffb/FFBEngine_Diagnostics.cpp`.

*Note: Option B reduces the line count of `FFBEngine.cpp`, but Option A provides a much cleaner, modular, and testable architecture.*

## 4. Conclusion

Moving these functions is a very positive step toward a cleaner codebase. By stripping out the metadata, safety checks, and debug buffer management, `FFBEngine.cpp` will become a highly focused file dedicated strictly to what matters most: FFB effect calculations and signal conditioning. I strongly recommend proceeding with **Option A (Class Extraction)** for long-term maintainability.