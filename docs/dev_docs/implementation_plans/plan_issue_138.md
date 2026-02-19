# Implementation Plan - Issue #138: Add Game FFB (FFBTorque) to the Graphs

## Context
The goal is to visualize the "Direct Torque" (`FFBTorque` from LMU 1.2+ shared memory) in the application's diagnostic graphs and telemetry logs. This allows users to compare the 400Hz direct torque signal with the legacy 100Hz steering shaft torque signal to diagnose weak or missing FFB issues.

## Reference Documents
- GitHub Issue #138: Add Game FFB (FFBTorque) to the Graphs
- Coordinate System Reference: `docs/dev_docs/references/Reference - coordinate_system_reference.md`
- FFB Formulas: `docs/dev_docs/references/FFB_formulas.md`

## Codebase Analysis Summary
- **`src/FFBEngine.h`**: Defines the `FFBSnapshot` structure which acts as a bridge between the high-frequency FFB thread and the GUI/Logging systems.
- **`src/FFBEngine.cpp`**: The `calculate_force` method populates the `FFBSnapshot` at the end of each physics tick.
- **`src/AsyncLogger.h`**: Defines the `LogFrame` structure and the CSV logging logic.
- **`src/GuiLayer_Common.cpp`**: Implements the ImGui-based diagnostic window, which uses `RollingBuffer` to store and render historical telemetry data from `FFBSnapshot` batches.
- **`src/main.cpp`**: The main FFB loop extracts `g_localData.generic.FFBTorque` from shared memory and passes it to the engine.

## FFB Effect Impact Analysis
This change is purely diagnostic and does not alter the FFB physics or feel.
- **Affected Effects**: None.
- **User Perspective**: Users will see a new graph in the "Raw Game Telemetry (Input)" section and two new columns in the CSV logs.

## Proposed Changes

### 1. FFB Engine Data Propagation
- **`src/FFBEngine.h`**:
    - Add `float raw_gen_torque;` to `FFBSnapshot` struct under `Header C`.
    - Add `float raw_shaft_torque;` to `FFBSnapshot` struct under `Header C`.
- **`src/FFBEngine.cpp`**:
    - In `FFBEngine::calculate_force`, populate `snap.raw_gen_torque` with the `genFFBTorque` argument.
    - Populate `snap.raw_shaft_torque` with `data->mSteeringShaftTorque`.

### 2. Telemetry Logging
- **`src/AsyncLogger.h`**:
    - Add `ffb_shaft_torque` and `ffb_gen_torque` to `LogFrame` struct.
    - Update `WriteHeader` to include `RawShaftTorque,RawGenTorque` in the CSV header.
    - Update `WriteFrame` to include these values in the CSV output line.
- **`src/FFBEngine.cpp`**:
    - In the `AsyncLogger` block of `calculate_force`, populate the new `LogFrame` fields.

### 3. GUI Visualization
- **`src/GuiLayer_Common.cpp`**:
    - Declare `static RollingBuffer plot_raw_gen_torque, plot_raw_shaft_torque;`.
    - In `GuiLayer::DrawDebugWindow`, extract these from `FFBSnapshot` and add to buffers.
    - In the `C. Raw Game Telemetry (Input)` section, add two new `PlotWithStats` calls for:
        - "Direct Torque (400Hz)"
        - "Shaft Torque (100Hz)"

### 4. Versioning
- Increment version in `VERSION` to `0.7.62`.

## Parameter Synchronization Checklist
N/A (No new user-configurable settings added, only diagnostic data).

## Test Plan
- **Unit Test**: Modify or add a test in `tests/test_ffb_core_physics.cpp` (or a new test file) to verify that `calculate_force` correctly populates the `FFBSnapshot` with the provided `genFFBTorque`.
- **Manual Verification**: (Note: Full app cannot be run on Linux, so we rely on unit tests and static analysis).
- **Test Case**: `TestFFBTorqueSnapshot`
    - Input: `genFFBTorque = 12.34f`, `mSteeringShaftTorque = 5.67f`.
    - Output: Check `snap.raw_gen_torque == 12.34f` and `snap.raw_shaft_torque == 5.67f`.

## Deliverables
- [x] Modified `src/FFBEngine.h`
- [x] Modified `src/FFBEngine.cpp`
- [x] Modified `src/AsyncLogger.h`
- [x] Modified `src/GuiLayer_Common.cpp`
- [x] Modified `VERSION`
- [x] New/Updated tests in `tests/`
- [x] Final Implementation Notes in this document

## Implementation Notes

### Unforeseen Issues
- None. The existing `FFBSnapshot` system was easy to extend.
- Needed to ensure the new CSV columns in `AsyncLogger` matched the header order exactly.

### Plan Deviations
- Added `raw_shaft_torque` in addition to `raw_gen_torque` to allow full comparison as requested in the issue (verify issues of weak FFB compared to the other channel).
- Renamed the existing "Steering Torque" plot in the GUI to "Selected Torque" to clarify which signal is currently being used for physics.

### Challenges
- None.

### Recommendations
- Consider adding latency monitoring between these two channels in a future update (Issue #136).
