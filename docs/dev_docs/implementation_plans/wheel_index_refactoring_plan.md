# Implementation Plan: Standardizing Wheel Indices

This plan outlines the refactoring steps to replace magic numbers related to wheel indices (0-3) with named constants.

## Objectives
- Eliminate magic numbers (0, 1, 2, 3) used for wheel indexing.
- Improve code readability by using descriptive names like `WHEEL_FL`, `WHEEL_FR`.
- Centralize wheel index definitions.

## 1. Create Central Constants Header
Create `src/core/WheelConstants.h` to hold the wheel index definitions.

```cpp
#pragma once

namespace LMUFFB {

/**
 * @brief Standard wheel indices used for rFactor 2 / LMU telemetry arrays.
 * 
 * Mapping:
 * 0: Front Left
 * 1: Front Right
 * 2: Rear Left
 * 3: Rear Right
 */
enum WheelIndex : int {
    WHEEL_FL = 0,
    WHEEL_FR = 1,
    WHEEL_RL = 2,
    WHEEL_RR = 3
};

// Total number of wheels in the simulation
static constexpr int NUM_WHEELS = 4;

} // namespace LMUFFB
```

## 2. Refactor Source Code
Update the following key files and replace hardcoded indices:

### `src/ffb/FFBEngine.cpp` & `src/ffb/FFBEngine.h`
- Update loops: `for (int i = 0; i < 4; i++)` -> `for (int i = 0; i < NUM_WHEELS; i++)`
- Update direct accesses: `mWheel[0]` -> `mWheel[WHEEL_FL]`, etc.
- Update upsampler arrays and other wheel-related members.

### `src/physics/GripLoadEstimation.cpp`
- Update wheel access logic.

### `src/logging/ChannelMonitor.h`
- Update telemetry capture logic.

### `src/io/rF2/rF2Data.h`
- Update `rF2Telemetry` struct documentation or internal usage. (Wait, `rF2Data.h` is likely fixed by the protocol, but we can use the constants for internal access).

## 3. Refactor Tests
The project has a large number of tests (~155 files). 
Focus on:
- `tests/test_ffb_common.h` (Often contains helper methods for multiple tests)
- `tests/test_ffb_engine.cpp`
- `tests/test_grip_load_estimation.cpp`
- `tests/test_ffb_magic_numbers.cpp` (The name itself suggests it might be relevant)

## 4. Verification
- Compile the project on Windows.
- Run the test suite to ensure no regressions.
- Verify that logging and UI still report correct wheel data.

## Rationale
Using magic numbers for wheel indices is prone to errors, especially when distinguishing between front and rear axles. Named constants make the physical meaning of the code immediately obvious.
