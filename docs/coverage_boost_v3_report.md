# Coverage Improvement Report v3

## Overview
This task focused on increasing the code coverage of the LMUFFB project, specifically targeting branch coverage which was the lowest. The strategy involved adding meaningful tests for edge cases, migration logic, and error handling without altering the main application logic, as per the updated instructions.

## Strategies to Increase Coverage

### 1. Edge Case Testing in Physics and Steering
- **NaN/Inf Handling**: Added tests for `FFBEngine::calculate_soft_lock` (via `SteeringUtils.cpp`) to verify that `NaN` or `Inf` steering inputs are handled gracefully by returning early, reaching 100% branch coverage for this file.
- **Empty Buffers**: Tested `FFBEngine::GetDebugBatch()` with an empty buffer to exercise the `if (!m_debug_buffer.empty())` branch.
- **Threshold Transitions**: Simulated low speed and low frequency conditions in `apply_signal_conditioning()` to exercise branches related to idle smoothing and wheel frequency thresholds.
- **Disabled Effects**: Called `calculate_force()` with all effect enable flags set to false to exercise early return paths in effect-specific calculation methods.
- **Safety Fallbacks**: Triggered the suspension bottoming safety fallback by providing telemetry where Method 0/1 were not active but tire load exceeded the static threshold.

### 2. Config Migration and Robustness
- **Malformed Input**: Created configuration files with non-numeric values for numeric keys to exercise `catch (...)` blocks in the INI parser.
- **Legacy Migration**: Tested the migration of legacy `understeer` (range 0-200) and `max_torque_ref` (> 40.0 Nm) values to ensure they are correctly mapped to the new parameters and ranges.
- **Out-of-Bounds Indices**: Tested preset management methods (`DeletePreset`, `DuplicatePreset`, `ApplyPreset`, `ExportPreset`) with invalid indices to ensure they handle them without crashing.
- **File Errors**: Simulated `ImportPreset` failure by providing a non-existent file path.
- **Exhaustive Key Coverage**: Created a comprehensive INI file containing all supported keys to ensure `Config::Load()` exercises every parser branch.

### 3. System and GUI Diagnostics
- **AsyncLogger Errors**: Attempted to start the logger with an invalid directory path to exercise error handling during directory creation.
- **Signal Handling**: Directly invoked the Linux signal handler `handle_sigterm` to verify it correctly sets the application stop flag.
- **UI Color Branches**: Simulated various FFB rates in the GUI to exercise the red/yellow/green color logic in `DisplayRate`.
- **UI Visibility**: Toggled `Config::show_graphs` to exercise branches related to window geometry and the visibility of the analysis window.

## Issues and Challenges Encountered

### Build and Linker Errors
- **Private Member Access**: Encountered build errors when trying to call `calculate_soft_lock` directly as it is private in `FFBEngine`. This was resolved by adding a helper method `CallCalculateSoftLock` to the `FFBEngineTestAccess` friend class.
- **Missing Includes and Constants**: `SIGTERM` was initially undefined in the test file; added `<csignal>`. Also resolved a type overflow warning for `mStaticUndeflectedRadius` by using a valid `unsigned char` value.
- **Linker Errors with Global Symbols**: When testing `handle_sigterm` and `g_running` from `main.cpp`, I initially encountered linker errors because they were declared inside a namespace in the test file while being defined in the global namespace in the source file. This was resolved by moving the `extern` declarations outside the test namespace.

### Shared Memory Mocking
Testing the `FFBThread` and `main` entry points required careful mocking of shared memory layouts. Reusing the `MockSM` system from previous tests was effective but required ensuring that the "LMU_Data" map was correctly sized and populated before starting threads.

### REDUNDANT Logic in Source Files
Some branches were unreachable because of redundant safety checks (e.g., ternary operators checking the same condition as an enclosing `if`). Following the instruction to avoid changing main code, I kept these redundant checks but added comments (e.g., `// NOTE: The ternary below is redundant but kept for safety.`) to signal this for future refactors.

## Coverage Results Analysis

| File | Line Coverage Change | Branch Coverage Change | Function Coverage Change |
| :--- | :--- | :--- | :--- |
| `src/Config.cpp` | 96.3% -> 96.4% | 58.2% -> 58.8% | 92.9% -> 100.0% |
| `src/main.cpp` | 39.5% -> 47.6% | 21.4% -> 22.8% | 40.0% -> 80.0% |
| `src/SteeringUtils.cpp` | 100% -> 100% | 87.5% -> 100% | 100% -> 100% |
| `src/AsyncLogger.h` | 97.3% -> 97.3% | 62.0% -> 66.9% | 100% -> 100% |
| `src/GuiLayer_Common.cpp` | 60.6% -> 61.1% | 32.1% -> 32.5% | 95.2% -> 95.2% |

The function coverage for `Config.cpp` reached 100%, and `main.cpp` saw a significant increase in both line and function coverage by exercising the headless entry point and signal handlers. Branch coverage improvements were achieved by targeting specific logic paths in `AsyncLogger.h`, `Config.cpp`, and `SteeringUtils.cpp`.
