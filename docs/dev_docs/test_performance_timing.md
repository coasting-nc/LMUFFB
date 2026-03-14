# Test Performance Timing in LMUFFB

## Contextualization
As the LMUFFB test suite grows (currently over 500 test cases), the total execution time becomes a significant factor in the developer feedback loop. Identifying "slow" tests is crucial for maintaining a fast CI/CD pipeline and ensuring that regressions in the physics engine or logger logic don't manifest as performance bottlenecks.

The goal of this task was to integrate a performance timer into the custom C++ test runner that automatically tracks the execution duration of every test case and reports the top 5 slowest tests in a summary report.

## Possible Solutions

### 1. External Profiling Tools
Using tools like `valgrind --tool=callgrind` or `gprof` allows for very deep analysis of where time is spent (function-level). However, these tools are often slow to run, platform-dependent (Valgrind is Linux-only), and provide "too much" data when the goal is simply a per-test case summary.

### 2. Migration to Standard Frameworks
Modern test frameworks like **Google Test** or **Catch2** provide built-in timing for every test case. While robust, migrating a custom-built harness of this size would require significant refactoring of all existing `TEST_CASE` macros and assertion logic.

### 3. Custom Instrumentation (Adopted)
This approach involves adding timing code directly into the test execution loop. It is the most lightweight solution and maintains the existing "zero-dependency" philosophy for the test runner.

## The Adopted Solution: Custom `std::chrono` Instrumentation

The implementation utilizes the C++11 `<chrono>` library to measure duration.

### Implementation Details:
1.  **Measurement**: In `tests/test_ffb_common.cpp`, the `Run()` function was modified to wrap each test call:
    ```cpp
    auto start = std::chrono::high_resolution_clock::now();
    test.func();
    auto end = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
    ```
2.  **Storage**: Test names and their durations are stored in a `std::vector<TestDuration>`.
3.  **Reporting**: In `tests/main_test_runner.cpp`, the results are sorted using `std::sort` with a lambda comparator and printed using `<iomanip>` to ensure a clean, tabular output.

### Justification:
- **Low Overhead**: `std::chrono` calls are extremely fast and do not significantly impact the test results themselves.
- **Portability**: The code works across both Linux (mocks) and Windows environments.
- **Seamless Integration**: By modifying the centralized `Run()` loop, all 500+ tests were instrumented instantly without touching individual test files.

## C++ Standards and Best Practices

### Standards Used
- **`std::chrono`**: The modern standard for time measurement. It is type-safe and avoids the pitfalls of legacy C functions like `clock()` or `gettimeofday()`.
- **`std::high_resolution_clock`**: Chosen because it provides the highest available precision on the platform, which is necessary when tests execute in sub-millisecond ranges.

### Best Practices Suitable for this Project
- **Monotonic Clocks**: While `high_resolution_clock` is usually monotonic, for duration measurements, it is critical to use clocks that do not "jump" due to NTP adjustments.
- **Minimal Global State**: While the project uses `extern` variables for its test runner (a legacy architectural choice), the storage of durations is localized to the `FFBEngineTests` namespace.
- **Separation of Concerns**: The logic for *measuring* (in the runner loop) is separated from the logic for *reporting* (in the main summary).
- **Single Source of Truth**: Shared types like `TestDuration` are defined in a dedicated header (`tests/test_performance_types.h`) to ensure binary compatibility and follow the DRY (Don't Repeat Yourself) principle.

## Challenges and Implementation Issues

1.  **Namespace Synchronisation**: The test runner is split across several files (`main_test_runner.cpp`, `test_ffb_common.cpp`, and `test_ffb_common.h`). Ensuring the `TestDuration` struct was visible and correctly declared in all contexts required the use of a shared header and careful `extern` management within the `FFBEngineTests` namespace to avoid ODR violations.
2.  **Output Truncation**: During development, tool output was frequently truncated, making it difficult to verify the exact structure of the `Run()` loop without using `sed` or `tail` to read specific segments of the large source files.
3.  **Sorting Logic**: Since the test runner is used in a headless environment, the summary needs to be concise. Implementing the "Top 5" filter using `std::sort` and a simple counter was chosen for its balance of efficiency and simplicity.
4.  **Precision Handling**: Some tests run extremely fast (0.01ms). Using `double` for `duration_ms` and `std::fixed` with `std::setprecision(2)` in the output ensures that these very fast tests are still represented accurately in the report.
