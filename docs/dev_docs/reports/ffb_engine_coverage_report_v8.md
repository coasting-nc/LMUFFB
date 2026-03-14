# FFB Engine Code Coverage Optimization Report (v8)

**Date:** March 14, 2026  
**Component:** `src/FFBEngine.cpp`  
**Current Branch Coverage:** ~85-88% (Estimated after V8 Boost)

## Overview
Following the implementation of `test_coverage_boost_v8.cpp`, branch coverage for the core physics engine has significantly increased. The latest tests addressed critical logical paths in safety transitions, dynamic normalization, and non-linear load transformations. However, several branches remain uncovered due to their reliance on specific timing, hardware states, or edge-case telemetry.

---

## Remaining Uncovered Branches

### 1. Snapshot Buffer Overflow
*   **Location:** `calculate_force` (Snapshot block)
*   **Logic:** `if (m_debug_buffer.size() < DEBUG_BUFFER_CAP)`
*   **Status:** Not covered.
*   **Why:** Requires pushing over 1000 snapshots in a single test iteration without the GUI (or test) draining the buffer. While easy to simulate, it hasn't been prioritized.

### 2. Mutex Contention
*   **Location:** Multiple entry points (`calculate_force`, `GetDebugBatch`, `SetTorqueSource`, etc.)
*   **Logic:** `std::lock_guard<std::recursive_mutex> lock(g_engine_mutex);`
*   **Status:** Not covered (Logical branch).
*   **Why:** The compiler generates branches for mutex acquisition success/failure (especially in recursive mode). Triggering these requires high-frequency multi-threaded stress tests, which are currently outside the scope of the unit test runner.

### 3. Steady Clock Latching
*   **Location:** Telemetry Processing
*   **Logic:** `if (std::chrono::duration_cast<std::chrono::seconds>(now - last_log_time).count() >= 1)`
*   **Status:** Partially covered.
*   **Why:** Requires the test to run for at least one physical second or for the system clock to be mocked. Since the test runner is optimized for speed, these branches are often skipped unless a `sleep` is explicitly added.

### 4. Specific Car Telemetry Fallbacks (DLC/Encrypted Content)
*   **Location:** Pre-calculations (Load/Grip Estimation)
*   **Logic:** `if (!m_warned_load)`, `if (!m_warned_susp_force)`, etc.
*   **Status:** Most covered in V8, but some rear-wheel specific variants remain.
*   **Why:** Requires simulating 50+ frames of missing telemetry specifically for RL/RR wheels while maintain speed > 3m/s.

### 5. Floating Point Edge Cases in Derivative Logic
*   **Location:** Signal Conditioning / Upsampling
*   **Logic:** `if (!std::isfinite(steer))`, `if (ffb_dt < 0.0001)`
*   **Status:** Partially covered.
*   **Why:** Some blocks check for `NaN` or `Inf` on every input channel. Covering every single one requires a combinatorial explosion of tests.

---

## Most Challenging to Cover

### Multi-threaded Race Conditions
Branches related to `g_engine_mutex` and `m_debug_mutex` are the most challenging. They represent "invisible" logic that ensures thread safety. To cover them, we would need to implement a dedicated stress test that concurrently calls `calculate_force` and `GetDebugBatch` at high frequencies, which may introduce instability into the CI pipeline.

### Real-time Frequency Monitoring
The `RateMonitor` branches (e.g., `m_ffb_rate`) rely on actual execution timing. In a virtualized or heavily loaded environment (like a CI runner), these timings can be unpredictable, making it difficult to assert that specific frequency-related branches are hit without non-deterministic test results.

---

## Easier to Cover (Recommended Next Steps)

### Combinatorial Filter Toggles
We can hit more branches in `apply_signal_conditioning` by toggling combinations of:
*   `m_static_notch_enabled` + `bw < MIN_NOTCH_WIDTH_HZ`
*   `m_flatspot_suppression` + `m_static_notch_enabled` (Interplay)
*   `m_steering_shaft_smoothing` variations.

### snapshot/Logging Logic
*   **Buffer Fill:** A simple test case that loops 1001 times to trigger the `DEBUG_BUFFER_CAP` branch.
*   **Logger Sanitization:** Feeding more complex "illegal" characters into the `AsyncLogger` to exercise all regex/replacement branches in filename generation.

### Understeer Gamma Edge Cases
*   **Gamma = 1.0:** Exercises the `std::pow` optimization (if the compiler optimizes `pow(x, 1)`).
*   **Grip = 0.0:** Exercises the `std::max(0.0, ...)` floor logic in the grip factor calculation.

---

## Summary of V8 Progress
The V8 boost successfully covered the "Logical Core" of the engine. The remaining ~12-15% of uncovered branches are largely **Defensive Programming** (mutexes, finite checks) and **Throttled Diagnostics** (one-second latching, once-per-car warnings). Further coverage should focus on the play-of-effects rather than basic logic.
