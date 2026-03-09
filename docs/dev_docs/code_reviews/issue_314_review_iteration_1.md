# Code Review Iteration 1 - Issue #314

The code review raised the following points:
1.  **Logic Bug (Memory Effect):** `m_safety.safety_smoothed_force` is not reset when a new safety window starts, leading to stale EMA values.
2.  **Thread Safety:** `m_safety.safety_timer` is modified without holding `g_engine_mutex` in `TriggerSafetyWindow`.
3.  **Fragile Sentinel:** Using `0.0` as a sentinel for "not seeded" is fragile.

## Addressing the Feedback

### 1. Fix Memory Effect and Sentinel
- Add `safety_is_seeded` boolean to `SafetyMonitor` struct.
- In `TriggerSafetyWindow`, set `safety_is_seeded = false`.
- In `calculate_force`, check `safety_is_seeded` instead of checking if `safety_smoothed_force == 0.0`.

### 2. Fix Thread Safety
- Add `std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);` to `FFBEngine::TriggerSafetyWindow`.
- Verified that all other modifications to `m_safety` occur within functions already holding `g_engine_mutex` (`calculate_force`, `calculate_soft_lock`, `ApplySafetySlew` when called from `main.cpp`).

### 3. Verification
- Add a new test case `test_safety_reentry_smoothing` to `tests/test_issue_314_safety_v2.cpp` to specifically verify that smoothing is reset upon re-entering safety mode.
