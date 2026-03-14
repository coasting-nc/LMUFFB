# Code Review: FFBEngine Refactoring (v0.7.184)

**Review Date:** March 14, 2026
**Status:** Approved with Recommendations

## 1. Overview
This refactoring extracts non-physics responsibilities from the `FFBEngine` class into modular manager components. This is a significant architectural improvement that reduces the footprint of the core physics engine and centralizes state management for metadata, safety, and diagnostics.

### Components Evaluated:
- **`FFBSafetyMonitor`**: Centralizes safety gating, spike detection, and slew rate limiting.
- **`FFBMetadataManager`**: Manages vehicle, track, and class state.
- **`FFBDebugBuffer`**: Thread-safe circular buffer for diagnostic snapshots.
- **`FFBSnapshot.h`**: Segregated telemetry/FFB data structure.

---

## 2. Logic Parity & Requirements Compliance
The refactoring successfully maintains logic parity with the legacy implementation across the 530-test regression suite.

### A. Safety Layer Logic
- **`IsFFBAllowed`**: The logic gating for `mIsPlayer`, `mControl`, and `mInGarageStall` is preserved exactly. The transition behavior for FINISHED/DNF status (Issue #126) remains correct.
- **`ApplySafetySlew`**: The slew rate limiting logic (including the distinction between NORMAL and RESTRICTED modes) is functionally identical.
- **Spike Detection**: The 5-frame sustained spike detection logic is preserved. One minor logic refinement was discovered where `TriggerSafetyWindow` now resets the `spike_counter` to zero upon activation. This is a beneficial change as it ensures a fresh detection window after a safety event has already been triggered.

### B. Metadata & Initialisation
- **Car Change Detection**: The detection of vehicle/class changes correctly triggers the `InitializeLoadReference` routine, maintaining the integrity of the class-aware normalization system.
- **Warning Resets**: The reset of `m_warned_invalid_range` on car change or manual normalization reset is correctly preserved.

---

## 3. Notable Improvements & Intentional Changes

### A. Numerical Stability (Slew Validation)
The refactoring includes a defensive update to configuration validation:
- **Change**: `m_safety_slew_full_scale_time_s` clamping increased from `0.0f` to `0.01f`.
- **Impact**: This prevents a potential division-by-zero or `Inf` result when calculating the slew limit (`1.0 / time`). A 10ms floor has no perceptible impact on haptic response but significantly improves code robustness.

### B. Log Throttling Refinement
The monitor now uses a more granular approach to log throttling. In the legacy code, different safety reasons shared a single timer. The new implementation logs a new entry immediately if the *reason* for the safety window changes, improving diagnostic clarity.

---

## 4. Issues Identified & Recommendations

During the review, the following minor issues or areas for improvement were identified:

### 1. Dead Member Variable in `FFBEngine`
- **Issue**: `FFBEngine.h` still defines `m_last_output_force` (Line 447). This variable is no longer updated or read in `FFBEngine.cpp`, as its role has been replaced by `m_safety.safety_smoothed_force`.
- **Recommendation**: Remove `m_last_output_force` from `FFBEngine` to avoid confusion and reduce memory footprint.

### 2. Lock Inconsistency in `FFBMetadataManager`
- **Issue**: `UpdateMetadata` uses a local `m_mutex`, but `UpdateInternal` does not. 
- **Recommendation**: While `UpdateInternal` is called from the FFB thread (which is already protected by `g_engine_mutex`), the manager should be self-consistent. Either add the lock to `UpdateInternal` or document that it is a "hot-path" method restricted to the FFB thread for performance reasons.

### 3. Public Member Access in Managers
- **Issue**: several methods in `FFBEngine` (e.g., `ResetNormalization`) still directly write to public members of the managers, such as `m_metadata.m_warned_invalid_range = false` (Line 679).
- **Recommendation**: Encapsulate these actions into methods within the manager classes (e.g., `m_metadata.ResetWarnings()`). This would complete the "Option A" refactoring goal of full encapsulation.

### 4. Inline Implementations in `FFBDebugBuffer.h`
- **Issue**: All circular buffer methods are implemented inline in the header. While acceptable for simple templates, for this specific class, it slightly deviates from the project's standard `.cpp` separation pattern.
- **Recommendation**: Move the logic to `FFBDebugBuffer.cpp` unless performance profiling shows inlining is critical for the `Push()` method.

### 5. Stateful Test Overload in `FFBSafetyMonitor`
- **Issue**: The `ApplySafetySlew(double, double, bool)` overload (Line 1374) introduces an internal `m_last_now` clock to support legacy unit tests.
- **Recommendation**: Clearly tag this overload as `[[nodiscard]]` and add a comment that it is primarily for standalone testing, to prevent accidental use in the main engine logic where a real time source is available.

---

## 5. Conclusion
The refactoring is technically sound and provides a much-needed structural cleanup of the FFB Engine. By isolating the math from the state management, the codebase has significantly higher "Readability vs. Complexity" ratio.

**The staged changes are safe to commit.** Addressing the recommendations above will further polish the implementation but they are not blockers for the current version release.
