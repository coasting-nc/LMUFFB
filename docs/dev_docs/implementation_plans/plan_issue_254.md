# Implementation Plan - Optimize Telemetry Exports (Binary Format) (#254)

## Context
Issue #254 reports that CSV telemetry logs grow too quickly in size and consume significant CPU for string formatting. The goal is to switch to a binary format, increase the sampling rate to 400Hz (native physics rate), and update the Python analyzer to support the new format.

## Design Rationale
- **Binary Format:** Reduces file size by ~60% compared to CSV and eliminates expensive `std::stringstream` formatting in the logger thread. Binary storage is also much faster for both writing (C++) and reading (Python/Pandas).
- **400Hz Sampling:** Provides enough resolution to analyze high-frequency FFB effects (ABS, road textures, suspension bottoming) without aliasing, which was lost with 100Hz decimation.
- **Memory Packing:** Using `#pragma pack(push, 1)` ensures that the C++ struct layout is predictable and lacks compiler-inserted padding, making it directly compatible with a Python `numpy.dtype`.
- **Header Metadata:** Keeping the metadata as plain text at the top of the file allows for easy human inspection of session settings without needing a full binary parser, while the `[DATA_START]\n` marker provides a clean delimiter for the binary stream.
- **Counter Safety:** Re-implemented the decimation counter reset to prevent eventual integer overflow, even when `DECIMATION_FACTOR` is 1.

## Reference Documents
- GitHub Issue #254: Reduce the size of csv exports with more optimized file format
- Investigation: `docs/dev_docs/investigations/reduce size of csv telemetry exports.md`

## Codebase Analysis Summary
- **Current Architecture:**
    - `src/AsyncLogger.h`: Manages a background thread that pulls `LogFrame` objects from a buffer and formats them as CSV strings using `std::iomanip`.
    - `tools/lmuffb_log_analyzer/loader.py`: Parses CSV files using `pd.read_csv`.
- **Impacted Functionalities:**
    - `AsyncLogger::Log`: Currently decimates 400Hz to 100Hz.
    - `AsyncLogger::WriteFrame`: Currently performs string formatting.
    - `AsyncLogger::Start`: Currently defines `.csv` extension and text mode.
    - `loader.py`: Needs a new binary loader path.

## FFB Effect Impact Analysis
| Effect | Technical Changes | User-Facing Impact |
| :--- | :--- | :--- |
| **All Effects** | Logged at 400Hz instead of 100Hz. | No change to the physical FFB feel, but users can now see high-frequency vibrations (ABS, Road) clearly in plots. |
| **Log Files** | Switched from `.csv` to `.bin`. | Files are smaller but require the analyzer tool to view. Excel cannot open them directly. |

## Proposed Changes

### 1. `src/AsyncLogger.h`
- **Modify `LogFrame` struct**:
    - Re-aligned field order to match the legacy CSV column order exactly. This simplifies the mapping and ensures that any partial reads or inspections find fields where expected.
    - Wrap with `#pragma pack(push, 1)` and `#pragma pack(pop)`.
    - Change `bool clipping` and `bool marker` to `uint8_t` for consistent sizing across compilers.
- **Update `AsyncLogger::Start`**:
    - Change default filename extension to `.bin`.
    - Open `m_file` with `std::ios::binary`.
- **Update `AsyncLogger::WriteHeader`**:
    - Append `[DATA_START]\n` as the final line.
- **Update `AsyncLogger::Log`**:
    - Set `DECIMATION_FACTOR = 1` to log every frame.
    - Ensure `m_decimation_counter` is reset to 0 after logging to prevent overflow.
- **Update `AsyncLogger::WorkerThread`**:
    - Optimized by writing the entire `m_buffer_writing` vector in a single `m_file.write` call.
- **Cleanup**:
    - Removed the unused `WriteFrame` method.

### 2. `tools/lmuffb_log_analyzer/loader.py`
- Define `LOG_FRAME_DTYPE` using `numpy.dtype` that matches the re-aligned `LogFrame` exactly.
- Implement `load_bin(filepath)`:
    - Open in binary mode.
    - Read header lines as bytes to extract metadata.
    - Locate `[DATA_START]\n` marker robustly using `seek()`.
    - Use `np.fromfile` to read the binary payload into a structured array.
- Update `load_log` to check the file extension and use either `load_bin` or the existing CSV loader.

### 3. `tools/lmuffb_log_analyzer/cli.py`
- Update `batch` command to include `*.bin` in the file search.
- Add an `--export-csv` option to the `info` and `analyze` commands.

### 4. Versioning
- Increment version from `0.7.125` to `0.7.126` in `VERSION`.

## Test Plan
- **New Unit Test (`tests/test_async_logger_binary.cpp`)**:
    - **Description**: Verify binary logging integrity.
    - **Assertions**:
        - File extension is `.bin`.
        - `sizeof(LogFrame)` is exactly 238 bytes (packed).
        - Binary data matches the input values when read back from the file.
- **Updated Tests**:
    - `tests/test_async_logger.cpp`: Updated to reflect 400Hz frame counts.
    - `tests/test_ffb_accuracy_tools.cpp`: Updated to read and verify binary surface type data.

## Implementation Notes
- Re-aligned `LogFrame` to match legacy CSV order (Speed, LatAccel, LongAccel, YawRate, Steering, Throttle, Brake...).
- Fixed Python `load_bin` to avoid `BufferedReader` iterator issues by using `chunk.find()` and `seek()`.
- Verified 100% pass on Logging tests.
