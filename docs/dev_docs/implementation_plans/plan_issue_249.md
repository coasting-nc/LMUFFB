# Implementation Plan - Export All Telemetry Channels to Binary Log (#249)

## Context
Issue #249 requires exporting all telemetry channels used for FFB calculations to the binary log (`AsyncLogger`). While basic binary logging was implemented in #254, many intermediate physics values and raw game channels are missing, hindering offline analysis and reproduction of FFB effects.

### Design Rationale
- **Complete Observability:** Logging every raw input and intermediate calculation allows for perfect reproduction of FFB behavior in the Python analyzer. This is critical for debugging complex effects.
- **Physical Integrity:** Grouping fields by their role (RAW, PROCESSED, COMPONENT, SYSTEM) maintains a clear data flow mapping from the game to the FFB output.
- **Efficient Packing:** Using `#pragma pack(push, 1)` and 4-byte floats maintains a manageable log size (~500 bytes per frame at 400Hz) while providing high fidelity. LZ4 compression will further reduce size by ~80% due to the 100Hz "staircase" pattern in raw fields.
- **Reliability:** Using `uint8_t` bitfields for warnings and markers keeps the struct compact while capturing diagnostic state.

## Reference Documents
- GitHub Issue #249: Export to the binary log all the telemetry channels we use for FFB.
- `src/AsyncLogger.h`: Current binary log definition.
- `src/FFBEngine.h`: `FFBSnapshot` and `FFBCalculationContext` definitions.

## Codebase Analysis Summary
- **Current Architecture:**
    - `LogFrame` in `AsyncLogger.h` captures 73 fields (294 bytes).
    - `FFBEngine::calculate_force` populates `LogFrame` at 400Hz and sends it to `AsyncLogger`.
    - `loader.py` in `tools/lmuffb_log_analyzer` parses the binary file into a Pandas DataFrame.
- **Impacted Functionalities:**
    - `AsyncLogger.h`: `LogFrame` struct layout and human-readable CSV header comment.
    - `FFBEngine.cpp`: Data population logic in `calculate_force`.
    - `loader.py`: `LOG_FRAME_DTYPE` and `mapping` dictionary.

### Design Rationale
These modules form the backbone of the logging pipeline. `AsyncLogger.h` defines the schema, `FFBEngine.cpp` acts as the producer, and `loader.py` as the consumer. Updating them in unison is required to maintain data integrity.

## FFB Effect Impact Analysis
| Effect | Technical Changes | User-Facing Impact |
| :--- | :--- | :--- |
| **All Effects** | Exported to binary log with full fidelity. | No change to real-time FFB, but enables high-fidelity offline analysis. |
| **Log Format** | Expanded from 294 to 511 bytes per frame. | Log files will be larger (~70% increase before compression), but more informative. |

### Design Rationale
By logging discrete components (e.g., `ffb_abs_pulse`, `ffb_soft_lock`), we can visualize exactly how each effect contributes to the total steering torque, which is essential for "Feel" optimization.

## Proposed Changes

1. **Update `src/AsyncLogger.h`**
   - Expand `struct LogFrame` to 118 fields.
   - Group fields: RAW (8 gen + 32 wheel), ALGO (24 state + 8 calculations + 8 misc), SLOPE (40 state + 16 misc), SETTINGS (32), COMPONENTS (64 + 20 misc), SYSTEM (4 + 3 packed).
   - Update `WriteHeader` field list.
   - **Design Rationale:** Using a flat struct with fixed-size fields ensures predictable binary layout and high performance during asynchronous logging.

2. **Update `src/FFBEngine.cpp`**
   - In `calculate_force`, capture intermediate values like `steering_angle_deg` and `understeer_drop` before snapshots.
   - Populate the entire `LogFrame` struct from the current physics context.
   - **Design Rationale:** Population must happen at the end of the 400Hz loop to ensure all "Previous Frame" states are correctly latched and all "Current Frame" calculations are final.

3. **Update `tools/lmuffb_log_analyzer/loader.py`**
   - Synchronize `LOG_FRAME_DTYPE` with the C++ struct.
   - Expand the `mapping` dictionary for Pandas column naming.
   - **Design Rationale:** NumPy structured arrays provide the fastest way to ingest large binary buffers into Pandas.

## Test Plan (TDD Approach)

1. **C++ Schema Validation (`tests/test_async_logger_binary.cpp`)**
   - Update `test_log_frame_packing` to assert `sizeof(LogFrame) == 511`.
   - Update integrity test to verify new fields round-trip correctly.
   - **Design Rationale:** Size assertions catch unexpected padding or field omissions immediately.

2. **Python Loader Verification (`test_binary_loader.py`)**
   - Update mock generation to use the new 511-byte format.
   - Verify alignment of critical fields (Speed, SurfaceType, Time).
   - **Design Rationale:** Proves the Python analyzer is in sync with the C++ engine.

3. **Regression Testing**
   - Run `./build/tests/run_combined_tests`.
   - Run `pytest tools/lmuffb_log_analyzer/tests/`.
   - **Design Rationale:** Ensures no side effects on core FFB physics from the logging additions.

## Deliverables
- [x] Modified \`src/AsyncLogger.h\`
- [x] Modified \`src/FFBEngine.cpp\`
- [x] Modified \`tools/lmuffb_log_analyzer/loader.py\`
- [x] Updated \`tests/test_async_logger_binary.cpp\`
- [x] Updated \`VERSION\` (0.7.129)
- [x] Updated \`CHANGELOG_DEV.md\`
- [x] Implementation Notes

## Implementation Notes
- **Issue Resolved**: Exported all 118 telemetry and FFB component channels to the binary log, providing complete observability for offline analysis.
- **Data Fidelity**: Logged raw 100Hz game data side-by-side with 400Hz processed data, enabling precise verification of the upsampling and derivative algorithms.
- **Analyzer Sync**: Updated the Python Log Analyzer to version 1.1 of the binary format, including full CamelCase mapping for all new fields.
- **Binary Stability**: Verified exact 511-byte packing using C++ static assertions (via unit tests) and verified round-trip integrity in both C++ and Python.
- **Challenges**: Carefully aligning the struct to avoid padding was achieved using \`#pragma pack(1)\`, and verified across systems.

## Additional Questions
- None at this time.
