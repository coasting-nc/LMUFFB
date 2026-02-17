# Proposal: Ancillary Diagnosis Codes Extraction for `FFBEngine.h`

## Overview
This proposal builds upon the previous extraction of math and vehicle utilities (v0.7.57) and suggests further decoupling focusing on **Diagnostics, Logging, and Visualization**.

Currently, `FFBEngine.h` contains significant logic for:
1.  **FFB Snapshots**: Capturing real-time state for GUI graphs.
2.  **Telemetry Logging**: Writing CSV files for offline analysis.
3.  **Warning Logs**: Checking and logging missing telemetry data.

While these are critical for observability, they are not strictly "FFB Physics." Moving them could reduce the core engine header size by another ~200-300 lines.

## Proposed Extractions

### 1. Snapshot Processor (`src/FFBSnapshot.h` or similar)
**What to move:**
- The `struct FFBSnapshot` definition.
- The `SNAPSHOT` block logic (currently inline around Lines 1349-1418).

**Pros:**
- **Clarity**: The `calculate_force` method is currently cluttered with a 70-line block purely for GUI visualization.
- **Separation**: The GUI's data requirements often change independently of physics logic.

**Cons / Challenges:**
- **Data Access**: The snapshot logic needs access to *almost every internal variable* of the `calculate_force` scope (inputs, intermediate context, outputs).
- **Performance**: Copying 50+ floats every frame (400Hz) needs to be efficient. Passing all these variables to an external function might require a large "context" struct or many arguments, which could be *more* verbose than the current inline block.

**Verdict:** **Undecided / Low Priority.**
Unless we aggregate all local variables into a unified `FFBState` struct that can be passed by reference, extracting the snapshot logic might result in a function signature with 20+ arguments, which is worse for maintainability.

### 2. Telemetry Logger (`src/TelemetryLogger.h` / `.cpp`)
**What to move:**
- The `Telemetry Logging (v0.7.x)` block (Lines 1420-1450+).
- The `AsyncLogger` integration logic.

**Pros:**
- **Clean Separation**: Logging is a distinct concern from physics calculation.
- **Header Dependency**: Removes dependency on `AsyncLogger.h` from the main engine header (if moved to .cpp).

**Cons / Challenges:**
- **Data Access**: Similar to snapshots, the logger needs access to raw inputs, vehicle state, and calculated outputs.
- **Performance**: This runs at 100Hz (throttled).

**Verdict:** **Good Candidate.**
The logger block interacts mostly with `TelemInfoV01` and `FFBCalculationContext`. It could be extracted into a method like:
`void LogTelemetry(const TelemInfoV01* data, const FFBCalculationContext& ctx, ...);`

### 3. Missing Data Warnings
**What to move:**
- The repeated `if (data->mGripFract == 0.0) { LogWarning(...) }` blocks scattered throughout the code.

**Pros:**
- **Noise Reduction**: These checks clutter the physics logic.
- **Centralization**: Logic for "is this car encrypted/broken?" can be centralized.

**Cons:**
- **Context Loss**: Warnings are often triggered deep inside specific effect calculations (e.g., inside `calculate_road_texture`). Moving them out requires either checking beforehand (pre-validation) or passing error states out.

**Verdict:** **Strong Candidate for Refactoring (Pre-Validation).**
Instead of checking for missing data *inside* every effect, we should have a `ValidateTelemetry(data)` step at the start of `calculate_force`. This method detects missing channels once per frame/session and sets flags in `FFBCalculationContext` (e.g., `ctx.has_grip_data`, `ctx.has_suspension_data`).
The effects then just check the flag or rely on `ctx` having safe fallbacks.

## Recommendations

1.  **Refactor Warnings First**: Move the scattered missing data checks into a single `ValidateTelemetry()` method at the beginning of the frame. This cleans up the core logic immediately without creating new files.
2.  **Keep Snapshots Inline (For Now)**: The complexity of passing all local state to an external snapshotter outweighs the benefit of removing the code block, unless a major data structure refactor (e.g., `FFBState`) is undertaken.
3.  **Extract Telemetry Logging**: Move the logging block to a private helper method `LogTelemetryFrame(...)` at the bottom of the file or in a separate util header if it grows.

## Impact on FFBEngine.h
- **Warnings**: ~50 lines removed/consolidated.
- **Logging**: ~30-50 lines moved to helper.
- **Snapshots**: No change recommended yet.

## Advanced Structure Proposal: Unified State Aggregation

To enable the clean extraction of Snapshot and Logging logic, we propose aggregating local state variables into unified structures. This solves the "function signature explosion" problem where extracted functions would otherwise require 20+ arguments.

### 4. Unified `FFBState` for Snapshots
**The Problem:** The `SNAPSHOT` block currently accesses a mix of:
- `TelemInfoV01* data` (Input telemetry)
- `FFBCalculationContext ctx` (Intermediate calculations)
- `TelemWheelV01 fl, fr` (Local aliases for front wheels)
- `double base_input`, `double norm_force` (Local scalars)
- `double raw_torque`, `double raw_load`, `double raw_grip` (Debug/Raw values)
- Member variables (`m_slope_current`, `m_ffb_rate`, etc.)

**The Solution:** Introduce a structured container that wraps these sources for the purpose of state capture.

```cpp
struct FFBStateContainer {
    const TelemInfoV01* data;
    const FFBCalculationContext& ctx;
    const FFBEngine& engine; // Access to member variables (rates, slopes)
    
    // Local variables that aren't in ctx or data
    double base_input;
    double norm_force;
    double raw_torque;
    double raw_load;
    double raw_grip;
    
    // Helpers
    const TelemWheelV01& fl() const { return data->mWheel[0]; }
    const TelemWheelV01& fr() const { return data->mWheel[1]; }
};
```

**Refactoring Strategy:**
1.  Define `FFBStateContainer` (likely internal or protected struct).
2.  At the snapshot point in `calculate_force`, instantiate this container:
    ```cpp
    FFBStateContainer state = { data, ctx, *this, base_input, norm_force, ... };
    CaptureSnapshot(state);
    ```
3.  Move the 70-line snapshot logic into `void CaptureSnapshot(const FFBStateContainer& state)`.

**Pros:**
- **Clean Main Loop:** Removes a massive block of non-physics code from the critical path.
- **Testability:** We can unit test the snapshot mapping logic by constructing a synthetic `FFBStateContainer`.
- **Extensibility:** adding new fields to the snapshot only requires adding them to the container, not changing function signatures.

**Cons:**
- **Struct Overhead:** Creating this struct on the stack every frame (400Hz) has a non-zero cost, though likely negligible compared to the locking and memory copying that follows.
- **Reference Management:** Needs careful handling of references to avoid lifetime issues, though strictly within `calculate_force` scope it is safe.

### 5. Unified Context for Telemetry Logging
**The Problem:** The `Telemetry Logging` block similarly accesses `data`, `ctx`, `fl`, `fr`, and engine members. It performs redundant calculations (e.g., `calculate_wheel_slip_ratio` is called again for logging).

**The Solution:** Telemetry logging should ideally reuse the `FFBStateContainer` proposed above, or a subset of it. Since the data requirements for Snapshots and Telemetry Logging overlap by ~90% (both need inputs, vehicle state, tire data), a single unified state structure is preferred.

**Proposed Logic Flow:**
```cpp
// Inside calculate_force
{
    // ... physics calculations ...

    // Consolidate State once
    FFBStateContainer state = { data, ctx, *this, ... };

    // 1. GUI Snapshot
    if (ShouldCaptureSnapshot()) {
        CaptureSnapshot(state);
    }

    // 2. Disk Logging
    if (AsyncLogger::Get().IsLogging()) {
        LogTelemetry(state);
    }
}
```

**Pros:**
- **DRY (Don't Repeat Yourself):** Consolidates variable gathering.
- **Consistency:** Ensures what we see on the GUI (Snapshot) matches exactly what we log to disk (Telemetry), as they source from the same state object.

**Recommendation:** Proceed with defining a local or member struct `FFBStateContainer` (or `FFBFrameState`) to facilitate the extraction of both Snapshots and Logging. This is a prerequisite for clean extraction.

## Impact on FFBEngine.h
- **Warnings**: ~50 lines removed/consolidated.
- **Logging**: ~30-50 lines moved to helper.
- **Snapshots**: ~70 lines moved to helper.
- **New Structs**: +15 lines definition.

This approach preserves the "Single File" nature for the core physics while cleaning up the noise.
