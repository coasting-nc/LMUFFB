# Proposal: Source Code Directory Reorganization

## Overview
As the LMUFFB project has grown, the `src/` directory has accumulated many files, making it increasingly difficult to navigate. This proposal suggests reorganizing the source files into functional subdirectories to improve maintainability and developer experience.

## Proposed Directory Structure

### 1. `src/core/`
**Responsibility:** Central configuration, versioning, and the main entry point of the application.
- `main.cpp`
- `Config.cpp`
- `Config.h`
- `Version.h.in`

### 2. `src/ffb/`
**Responsibility:** The core Force Feedback engine, resampling logic, and hardware-specific output (DirectInput).
- `FFBEngine.cpp`
- `FFBEngine.h`
- `DirectInputFFB.cpp`
- `DirectInputFFB.h`
- `UpSampler.cpp`
- `UpSampler.h`
- `aceFFB/` (Subdirectory for aceFFB specific logic)

### 3. `src/physics/`
**Responsibility:** Physics modeling, vehicle dynamics, and telemetry signal processing (e.g., grip and load estimation).
- `GripLoadEstimation.cpp`
- `VehicleUtils.cpp`
- `VehicleUtils.h`
- `SteeringUtils.cpp`

### 4. `src/gui/`
**Responsibility:** All UI-related code, platform-specific windowing implementations, and graphics utilities.
- `GuiLayer.h`
- `GuiLayer_Common.cpp`
- `GuiLayer_Win32.cpp`
- `GuiLayer_Linux.cpp`
- `GuiWidgets.h`
- `GuiPlatform.h`
- `Tooltips.h`
- `DXGIUtils.cpp`
- `DXGIUtils.h`
- `res.rc.in`
- `resource.h`

### 5. `src/io/`
**Responsibility:** External data acquisition and interfaces. This includes the game-specific shared memory interfaces and the REST API.
- `GameConnector.cpp`
- `GameConnector.h`
- `RestApiProvider.cpp`
- `RestApiProvider.h`
- `lmu_sm_interface/` (Shared memory interface for LMU)
- `rF2/` (Interface for rFactor 2)

### 6. `src/logging/`
**Responsibility:** System-wide logging, health monitoring, and performance tracking.
- `AsyncLogger.h`
- `Logger.h`
- `HealthMonitor.h`
- `PerfStats.h`
- `RateMonitor.h`

### 7. `src/utils/`
**Responsibility:** Generic utility functions and shared mathematical routines used across multiple modules.
- `MathUtils.h`
- `StringUtils.h`

---

## Rationale for Groupings

- **InputOutput (`io/`):** Separates the *source* of the data (game shared memory, API requests) from how it is *processed*.
- **GUI (`gui/`):** Consolidates all visual and platform-specific windowing logic, which is often distinct from the real-time FFB processing.
- **Physics (`physics/`):** Isolate the complex mathematical modeling of vehicle dynamics. These files are highly specialized and often updated together when refining the "feel" of the car.
- **FFBEngine (`ffb/`):** Keeps the primary FFB logic and the final output stage (DirectInput) together, as they represent the core "business logic" of the app.
- **Logging (`logging/`):** High-frequency logging and health monitoring have specific requirements (like lock-free buffers) that justify a dedicated category.
- **Utils (`utils/`):** Standard "junk drawer" for small, reusable pieces of code that don't belong to any specific domain but are needed everywhere.

## Implementation Steps
1. **Update CMakeLists.txt:** The build system must be updated to include the new subdirectory paths.
2. **Fix Include Paths:** All `#include` statements within the source files will need to be updated to reflect the new structure (e.g., `#include "ffb/FFBEngine.h"`).
3. **Migrate Files:** Move the files to their new locations.
4. **Validation:** Perform a clean build and run all tests to ensure no regressions were introduced.
