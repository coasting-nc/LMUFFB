# Test Scaling and Future Performance Strategies

## Parallel Test Execution

### Feasibility
Running tests in parallel is highly feasible in C++ but requires careful management of shared resources. The current LMUFFB test runner uses several global variables and singleton-like patterns (e.g., `g_engine`, `Config`, `Logger`, `AsyncLogger`) that are not currently thread-safe for concurrent test execution.

### Challenges
1.  **Global State Contention**: Many tests modify `g_engine` or the global `Config` state. Parallel execution would require each test to have its own isolated instance of the engine and configuration, or a robust locking mechanism that would likely negate the performance gains.
2.  **File System Collisions**: Many tests create and delete temporary files (e.g., `test_config.ini`, `test_logs/`). Parallel tests would overwrite each other's files unless unique, thread-id-based filenames are used for all artifacts.
3.  **Singleton Patterns**: The `Logger` and `AsyncLogger` classes use the Singleton pattern. Parallel tests would attempt to initialize/close the same logger instance simultaneously.

### Strategy: Hybrid Parallelism
A more achievable short-term goal is to run "Slow" tests in parallel while keeping the rest serial.
-   **Labeling**: Tests could be tagged with `{"Slow"}` or `{"Blocking"}` using the existing tagging system.
-   **Execution**: The runner could spawn separate threads for these slow tests at the start, ensuring they run in the background while the fast, serial suite progresses.
-   **Isolation**: These specific slow tests would need to be audited to ensure they don't touch the global `g_engine` or use conflicting filenames.

---

## Optimizing the Slowest Tests

Below is an analysis of the top 5 slowest tests and potential ways to improve their performance.

### 1. `test_main_app_logic` (~14,000 ms)
-   **Current Behavior**: This test simulates the full `FFBThread` and the main app loop. It includes multiple `std::this_thread::sleep_for` calls, including one for 5.2 seconds to verify health monitor branch coverage.
-   **Optimization**:
    -   **Mock Time**: Replace `std::this_thread::sleep_for` with a mockable clock. Instead of waiting for 5 seconds of real-time, the test could "teleport" the internal clock forward.
    -   **Reduced Iterations**: If the health monitor logic triggers at 5 seconds, we only need to simulate enough frames to hit that threshold, not wait in real-time.
-   **Challenges**: The health monitor relies on `std::chrono::steady_clock`. Injecting a mock clock would require refactoring the production code in `FFBEngine` and `main.cpp` to use a time provider interface.

### 2. `test_issue_312_logger_uniqueness` (~1,100 ms)
-   **Current Behavior**: Verifies that logs created at different times have unique filenames. It waits for 1.1 seconds to ensure the timestamp (which has 1-second resolution) increments.
-   **Optimization**:
    -   **Manual Filename Injection**: Instead of waiting, the test could verify the filename generation logic directly by providing different time points to the generator.
    -   **Sub-second Resolution**: Update the logger to include milliseconds in the filename, allowing for much shorter wait times (e.g., 10ms).
-   **Challenges**: Changing filename formats might affect external tools that rely on the current `YYYY-MM-DD_HH-MM-SS` format.

### 3. `test_rate_monitor_realtime` (~1,100 ms)
-   **Current Behavior**: Measures real-time event frequency by recording events for 1.1 seconds.
-   **Optimization**:
    -   **Deterministic Simulation**: Use `RecordEventAt(time_point)` with manually incremented time points (as seen in `test_rate_monitor_calculation`) instead of real-time measurement.
-   **Challenges**: The "real-time" aspect of the test is intended to verify how the monitor interacts with the system clock and scheduler jitter. A simulation would lose this "integration" aspect.

### 4. `test_issue_100_timing` (~250 ms)
-   **Current Behavior**: Simulates 10 iterations of a 16ms sleep loop (the main GUI loop frequency).
-   **Optimization**:
    -   **Reduce Sample Size**: Decrease the number of iterations from 10 to 3. This would reduce the time to ~50ms while still verifying the sleep logic.
-   **Challenges**: Fewer iterations might make the test more sensitive to OS scheduler jitter, leading to "flaky" failures on busy CI runners.

### 5. `test_async_logger_binary_integrity` (~200 ms)
-   **Current Behavior**: Logs frames to an async worker and waits 200ms for the worker to flush to disk before stopping.
-   **Optimization**:
    -   **Explicit Flush**: Add a `Flush()` method to `AsyncLogger` that blocks until the queue is empty, rather than using an arbitrary sleep.
-   **Challenges**: Implementing a thread-safe `Flush()` requires a condition variable or a specialized "poison pill" in the logging queue to signal completion to the caller.
