# Implementation Plan - Fix Log Analyzer Buffer Mismatch (v1.2)

## Context
The LMUFFB Log Analyzer tool fails to load telemetry logs with the error `buffer size must be a multiple of element size`. This is due to a mismatch between the C++ `LogFrame` structure (v1.2) and the Python NumPy `dtype` definition in the analyzer.

## Design Rationale
The core problem is that the C++ telemetry structure was updated (adding `ffb_stationary_damping`), but the Python loader was not. To ensure robustness and backward compatibility:
1.  **Versioned Dtypes:** We will define specific dtypes for each version of the log format (v1.1 and v1.2).
2.  **Robust Detection:** The loader will first attempt to read the version from the log header. As a fallback, it will use the buffer size modulo to auto-detect the correct record size, preventing crashes on "unknown" or mismatched headers.
3.  **Numerical Safety:** Calculations like gradients are prone to division-by-zero errors when telemetry timestamps are duplicated or extremely close. We will add guards and data cleaning to these paths.

## Codebase Analysis Summary
### Impacted Functionalities
- **Log Loading (`loader.py`)**: `load_bin` and `load_log` are the entry points for reading binary data. They must handle different frame sizes.
- **Data Modeling (`models.py`)**: The `LogMetadata` and field mappings need to include the new `ffb_stationary_damping` field.
- **Analysis & Plotting (`plots.py`, `yaw_analyzer.py`)**: Numerical calculations using `np.gradient` must handle edge cases in telemetry data.

### Design Rationale
Modifying `loader.py` is the most direct fix for the buffer mismatch. Updating `models.py` ensures the new data is accessible to the rest of the application. The numerical fixes in `plots.py` and `yaw_analyzer.py` address secondary issues discovered during testing with real-world logs.

## Proposed Changes

### 1. tools/lmuffb_log_analyzer/loader.py
- Define `LOG_FRAME_DTYPE_V11` (535 bytes).
- Define `LOG_FRAME_DTYPE_V12` (539 bytes) including `ffb_stationary_damping`.
- Update `load_log` to parse the version header.
- Update `load_bin` to select the dtype based on the detected version.
- Implement a fallback check: `if len(buffer) % itemsize != 0`, try the other supported `itemsize`.

### 2. tools/lmuffb_log_analyzer/models.py
- Update `LogMetadata` or field mappings to include `FFBStationaryDamping`.

### 3. tools/lmuffb_log_analyzer/plots.py & yaw_analyzer.py
- In `plot_true_tire_curve`, use `.copy()` when slicing DataFrames to avoid `SettingWithCopyWarning`.
- In gradient calculations, ensure x-axis values (timestamps/bin centers) are unique and strictly increasing.
- Use `np.errstate(divide='ignore', invalid='ignore')` around sensitive math operations.

## Test Plan (TDD-Ready)
### Design Rationale
Tests must verify that both old (v1.1) and new (v1.2) log formats can be loaded without error. We also need to verify the auto-detection fallback works when the header is misleading.

### Test Cases
1.  **`test_v11_loading`**: Create a mock v1.1 binary file. Verify `load_log` identifies it as v1.1 and loads the correct number of frames.
2.  **`test_v12_loading`**: Create a mock v1.2 binary file (with the extra float). Verify `load_log` identifies it as v1.2 and the new field is present.
3.  **`test_autodetect_fallback`**: Create a file with v1.2 data but a v1.1 header. Verify the loader successfully falls back to v1.2 based on buffer size.
4.  **`test_gradient_stability`**: Call plotting/analysis functions with data containing duplicate timestamps. Verify no `RuntimeWarning` is raised.

## Deliverables
- [x] Code changes in `loader.py`, `models.py`, `plots.py`, `yaw_analyzer.py`.
- [x] New test file `tools/lmuffb_log_analyzer/tests/test_version_compat.py`.
- [x] Updated `CHANGELOG_DEV.md`.
- [x] Implementation Notes.

## Implementation Notes
- **Unforeseen Issues:**
    - Discovered that `np.gradient` raises `RuntimeWarning` when coordinates are non-unique. This was not immediately obvious but became apparent when processing high-frequency logs with duplicate timestamps.
    - LZ4 decompression adds a small overhead, but the buffer size check must be performed on the *decompressed* data.
- **Plan Deviations:**
    - Initially planned to only update the header parsing, but added the modulo-based auto-detection as a safety measure after realizing some dev logs might have incorrect headers.
- **Challenges Encountered:**
    - Mapping the exact byte offset for `ffb_stationary_damping` (index 112) required careful alignment with the C++ `LogFrame` struct to ensure data integrity.
- **Recommendations for Future Plans:**
    - For any telemetry-related changes, always include a step to verify the Python-side NumPy `dtype` parity with the C++ source.
