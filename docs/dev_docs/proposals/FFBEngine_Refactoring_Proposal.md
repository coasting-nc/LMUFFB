# Refactoring Report: Moving Non-Physics Functions out of `FFBEngine.cpp`

## 1. Introduction

`src/ffb/FFBEngine.cpp` has been refactored to isolate its core responsibility: real-time Force Feedback (FFB) calculation. Metadata management, safety enforcement, and diagnostic data buffering have been moved to dedicated manager classes.

## 2. Implementation Summary (March 2026)

The refactoring followed **Option A (Class Extraction)** to provide a clean, modular architecture. The following components were created:

### A. FFBMetadataManager
*   **Location:** `src/ffb/FFBMetadataManager.h/cpp`
*   **Role:** Centralizes vehicle and track state tracking.
*   **Logic:** Encapsulates `UpdateMetadata`, `UpdateInternal`, and steering range warning state. It ensures class-aware load references are initialized only when the vehicle identity changes.

### B. FFBSafetyMonitor
*   **Location:** `src/ffb/FFBSafetyMonitor.h/cpp`
*   **Role:** Centralizes FFB permission logic (`IsFFBAllowed`), torque spike detection, and safety slew rate limiting.
*   **Integration:** `FFBEngine` delegates all safety gating and mitigation to this manager. Configuration parameters (duration, gain reduction, thresholds) are now logically grouped under `m_safety` in `FFBEngine`.

### C. FFBDebugBuffer
*   **Location:** `src/ffb/FFBDebugBuffer.h/cpp`
*   **Role:** Implements a thread-safe circular buffer for `FFBSnapshot` data.
*   **Logic:** Decouples high-frequency data production (Physics) from lower-frequency consumption (GUI/Diagnostics).

### D. Unified Snapshot
*   **Location:** `src/ffb/FFBSnapshot.h`
*   **Role:** Consolidates all loggable FFB and Telemetry fields into a single header to avoid circular dependencies between managers.

---

## 3. Logic Parity and Parity Fixes

The primary goal was a "move-only" refactoring. However, during the synchronization of the 530-test regression suite, several logic refinements were necessary to maintain 100% parity with legacy behavior:

1.  **Safety Permission Gating**: Removed a redundant `gamePhase < 2` check in `IsFFBAllowed`. Tests expected FFB to remain functional in `GP_GARAGE (0)`, whereas the new implementation initially over-restricted it. parity was restored by deferring menu-muting to the `main.cpp` logic.
2.  **Stateful Test Support**: Legacy unit tests utilized a stateful 3-arg `ApplySafetySlew` signature. The manager now provides this overload, using an internal `m_last_now` clock to simulate time progression for standalone tests.
3.  **Log Throttling**: Fixed a bug in the implementation where different safety reasons (e.g., "FFB Unmuted" vs "Control Transition") were incorrectly sharing a single log-throttle timer. They now correctly bypass the timer if the reason has changed.
4.  **Parameter Nesting**: While parameters moved under `m_safety`, their validation and default initialization logic in `Config.cpp` and `Preset` was preserved exactly to ensure existing user `config.ini` files remain valid.

---

## 4. Issues and Challenges

### A. Vendor File Protection
During the refactoring, `SharedMemoryInterface.hpp` (a vendor file provided by Studio 397) was accidentally modified to include `<optional>`. This violated the project's mandate to keep vendor files unmodified. The change was reverted, and the necessary include was moved to the `LmuSharedMemoryWrapper.h` wrapper header.

### B. High-Frequency Time Synchronization
`FFBSafetyMonitor` requires access to `mElapsedTime` for log throttling and timer management. Passing this through every `ApplySafetySlew` call (400-1000 times per second) was inefficient. The challenge was solved by using a **Time Pointer strategy**: `FFBEngine` provides a pointer to its persistent `m_working_info.mElapsedTime` to the manager once during construction, ensuring the manager is always in sync with the latest telemetry time without redundant state copies or parameter bloat.

### C. Test Infrastructure Synchronization
The `FFBEngineTestAccess` friend class had to be significantly updated to grant tests access to the new manager members. This was critical for maintaining the 100% coverage baseline for private internal states like the `spike_counter` and `safety_timer`.

---

## 5. Deviations from Original Plan

1.  **Nested Configuration**: The original plan suggested moving parameters out of `FFBEngine`. Instead, they were **nested** inside a `m_safety` member. This maintained the `FFBEngine` as the central configuration hub for the GUI while satisfying the architectural goal of logic separation.
2.  **Stateful Overloads**: The plan initially envisioned purely stateless managers. However, the requirement to support the existing stateful unit test suite led to the implementation of stateful overloads in `FFBSafetyMonitor` to preserve test integrity.
3.  **Unified Snapshot Header**: The decision to create `FFBSnapshot.h` was a deviation from the individual manager plan, driven by the need to resolve circular include dependencies between the `FFBEngine` and its managers.

## 6. Numerical Stability and Parameter Validation

During the refactoring of the configuration persistence layer, a critical adjustment was made to the validation logic for safety parameters in `src/core/Config.h`.

### A. Safety Slew Time Clamping
The clamping for the safety slew full-scale time was increased from `0.0f` to `0.01f`:
*   **Old:** `engine.m_safety.m_safety_slew_full_scale_time_s = (std::max)(0.0f, safety_slew_full_scale_time_s);`
*   **New:** `engine.m_safety.m_safety_slew_full_scale_time_s = (std::max)(0.01f, safety_slew_full_scale_time_s);`

**Rationale:**
This change was implemented to ensure numerical stability within the `FFBSafetyMonitor::ApplySafetySlew` method. The monitor calculates the slew limit as the reciprocal of the full-scale time: `slew_limit = 1.0 / m_safety_slew_full_scale_time_s`. 

By enforcing a minimum floor of 10ms (0.01s), we prevent:
1.  **Division by Zero**: Eliminates the risk of `Inf` or `NaN` results if a user (or a malformed config) sets the time to exactly zero.
2.  **Validation Consistency**: Several regression tests specifically verify that the engine enforces sane minimums for haptic timing. This floor ensures that "instant" transitions are still bounded by a physically plausible limit, satisfying the `Issue #316` validation requirements.

## 7. Diagnostic Buffering and Thread Safety

The extraction of the diagnostic buffering logic into `FFBDebugBuffer` allowed for significant code cleanup in `src/ffb/FFBEngine.cpp`.

### A. Encapsulation of Mutex Logic
During the refactoring, manual synchronization and capacity checks were removed from the hot path of `calculate_force`:
*   **Removed:**
    ```cpp
    std::lock_guard<std::mutex> lock(m_debug_mutex);
    if (m_debug_buffer.size() < DEBUG_BUFFER_CAP) {
    ```
*   **Rationale:**
    These responsibilities are now fully encapsulated within the `FFBDebugBuffer::Push` method. By moving the `std::lock_guard` and the buffer size validation inside the manager, we improved the **Separation of Concerns**. `FFBEngine` no longer needs to manage low-level concurrency primitives or maintain knowledge of the buffer's capacity limits, reducing the risk of deadlocks or race conditions during future modifications.

## 8. Conclusion

The refactoring is complete. `FFBEngine.cpp` is now 100% focused on physics calculations. All 530 regression tests pass, and the system is verified stable on Windows (MSVC).
