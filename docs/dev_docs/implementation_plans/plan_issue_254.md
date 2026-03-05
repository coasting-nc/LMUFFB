# Implementation Plan - Optimize Telemetry Exports (Binary Format) (#254)

## Context
Issue #254 reports that CSV telemetry logs grow too quickly in size and consume significant CPU for string formatting. The goal is to switch to a binary format, increase the sampling rate to 400Hz (native physics rate), and update the Python analyzer to support the new format.

## Design Rationale
- **Binary Format:** Reduces file size by ~60% compared to CSV and eliminates expensive `std::stringstream` formatting in the logger thread. Binary storage is also much faster for both writing (C++) and reading (Python/Pandas).
- **400Hz Sampling:** Provides enough resolution to analyze high-frequency FFB effects (ABS, road textures, suspension bottoming) without aliasing, which was lost with 100Hz decimation.
- **Raw vs. Processed Data:** Logging both raw 100Hz telemetry and 400Hz upsampled data side-by-side allows for precise debugging of the signal processing chain (prediction accuracy, filter lag).
- **LZ4 Compression:** Telemetry data at 400Hz contains many repeating patterns (especially the 100Hz raw "staircase"). LZ4 provides 80-90% compression with near-zero CPU overhead, keeping log sizes manageable (tens of MBs instead of hundreds).
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
    - Group fields into "Raw Telemetry (100Hz)", "Upsampled Telemetry (400Hz)", "Algorithm State", and "Outputs".
    - Include raw 100Hz equivalents for critical channels (steering, load, patch velocities, accelerations).
    - Wrap with `#pragma pack(push, 1)` and `#pragma pack(pop)`.
    - Change `bool clipping` and `bool marker` to `uint8_t`.
- **Update `AsyncLogger::Start`**:
    - Change default filename extension to `.bin`.
    - Open `m_file` with `std::ios::binary`.
- **Update `AsyncLogger::WriteHeader`**:
    - Append `[DATA_START]\n` as the final line.
- **Update `AsyncLogger::Log`**:
    - Set `DECIMATION_FACTOR = 1` to log every frame.
    - Ensure `m_decimation_counter` is reset to 0 after logging.
- **Implement LZ4 Compression**:
    - Add `m_lz4_enabled` (bool, default false).
    - In `WorkerThread`, if enabled, compress the `m_buffer_writing` block using `LZ4_compress_default` before writing.
    - Prepend each compressed block with a 4-byte size header.

### 2. `tools/lmuffb_log_analyzer/loader.py`
- Define `LOG_FRAME_DTYPE` matching the new augmented `LogFrame`.
- Implement `load_bin(filepath)`:
    - Handle both uncompressed and LZ4-compressed streams.
    - If LZ4 is used, decompress blocks using the `lz4` Python package.
    - Locate `[DATA_START]\n` marker robustly using `seek()`.
    - Use `np.frombuffer` to read the binary payload into a structured array.

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
        - `sizeof(LogFrame)` is exactly 294 bytes (packed, v0.7.128).
        - Binary data matches the input values when read back from the file.
- **Updated Tests**:
    - `tests/test_async_logger.cpp`: Updated to reflect 400Hz frame counts.
    - `tests/test_ffb_accuracy_tools.cpp`: Updated to read and verify binary surface type data.

## Deliverables
- [x] Modified `src/AsyncLogger.h`
- [x] Modified `tools/lmuffb_log_analyzer/loader.py`
- [x] Modified `tools/lmuffb_log_analyzer/cli.py`
- [x] New `tests/test_async_logger_binary.cpp`
- [x] Updated `VERSION`
- [x] Implementation Notes

## Implementation Notes
- **Issue Resolved**: Transitioned telemetry exports from CSV to binary format and increased sampling rate to 400Hz.
- **Result**: Reduced file size by ~60% and improved logging fidelity, enabling analysis of high-frequency FFB effects without aliasing.
- **Challenges**: Handling Python's buffered file reading which caused `f.read()` to skip data when using a line iterator. Resolved by using `chunk.find()` and `seek()`.
- **Deviations**: Re-aligned the C++ `LogFrame` struct fields to match the legacy CSV column order. This ensures that the binary layout is logically consistent with historical logs and simplifies the Python mapping.
- **Iterative Review Process**:
    - **Iteration 1**: Rated #Partially Correct#. Feedback noted potential counter overflow, broken Python reading due to buffering, and field alignment mismatch.
    - **Iteration 2**: Rated #Mostly Correct#. Production code was correct but Python tests were not updated for the final 61-field (262-byte) augmented struct.
    - **Iteration 3**: Rated #Correct#. Updated Python tests and implementation plan to reflect the final 294-byte struct layout (v0.7.128). Verified all tests pass.
- **Recommendations**: Monitor user feedback regarding the loss of direct Excel compatibility. The added `--export-csv` flag in the Python analyzer provides a workaround.

## Final Implementation Notes
- **LZ4 Integration**: Switched from `FetchContent` to manual vendor downloads in CI (`vendor/lz4/`) to resolve build issues on Windows and ensure predictable header discovery.
- **Robust LZ4 Block Format**: Implemented an 8-byte block header `[compressed_size, uncompressed_size]` to allow the Python `lz4` package to decompress without ambiguity, resolving `LZ4BlockError` encountered during development.
- **Comprehensive Verification**:
    - Added `tests/test_async_logger_lz4.cpp` to verify C++ block compression and flushing.
    - Updated `test_binary_loader.py` to verify Python-side decompression of the new 8-byte header format.
    - Verified all 413 C++ tests and 20 Python tests pass on Linux.
- **Windows Test Resilience**:
    - Fixed a regression in `test_logger_lz4_compression` that caused failures on Windows. The issue was an unclosed `std::ifstream` handle that held an exclusive lock on the log file, preventing `std::filesystem::remove_all` from cleaning up the test directory.
    - Resolved by scoping the file handle within a `{}` block to ensure deterministic closure before cleanup.
- **License Compliance**: Updated `LICENSE` to include the BSD 2-Clause license text for the linked LZ4 library components.
