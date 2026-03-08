# Implementation Plan - Issue 293: Add analysis (plots, stats) to the log analyser to diagnose the Lateral Load effect

Add analysis (plots, stats) to the log analyser to diagnose the Lateral Load effect, which was introduced to enhance Seat-of-the-Pants (SoP) feel by incorporating real-time lateral load transfer from tire telemetry.

## Issue Reference
[Issue #293](https://github.com/coasting-nc/LMUFFB/issues/293) - Add analysis (plots, stats) to the log analyser to diagnose the Lateral Load effect

## Context
The "Lateral Load" effect was added to the SoP (Seat of the Pants) FFB component to provide better feedback about weight transfer. To effectively tune this effect, users need to see how it behaves relative to the traditional G-force based SoP effect. This requires logging more metadata (the effect settings) and adding specific diagnostic tools to the log analyzer.

## Analysis
To analyze the Lateral Load effect, we need:
1.  **Metadata**: The settings for `m_lat_load_effect`, `m_sop_scale`, and `m_sop_smoothing_factor` at the time of logging.
2.  **Telemetry**: Existing logged channels like `LatAccel`, `LatLoadNorm` (smoothed normalized load), and `FFBSoP` (total SoP force) are sufficient, but having raw load transfer for comparison is better.
3.  **Visualization**: A plot that decomposes the total SoP force into its G-based and Load-based components.
4.  **Statistics**: Calculation of how much each component contributes and correlation with lateral acceleration.

### Design Rationale: Metadata Logging
> [!IMPORTANT]
> Logging the settings into the file header is the only way to ensure the analyzer can accurately decompose the signals later, as the user might change settings between recording and analysis.

## Proposed Changes

### 1. Update C++ Logging Metadata
1.  **Modify `src/AsyncLogger.h`**:
    *   Add `lat_load_effect`, `sop_scale`, and `sop_smoothing` to `SessionInfo`.
    *   Update `WriteHeader` to include these in the text header.
2.  **Modify `src/main.cpp`**:
    *   Populate the new `SessionInfo` fields before starting the logger.
3.  **Verification**: Build the project using `cmake --build build`.

### 2. Update Python Log Analyzer Models and Loader
4.  **Modify `tools/lmuffb_log_analyzer/models.py`**:
    *   Add the new fields to `SessionMetadata`.
5.  **Modify `tools/lmuffb_log_analyzer/loader.py`**:
    *   Update `_parse_header` to read the new fields with backward-compatible defaults.
6.  **Verification**: Use `read_file` to confirm the changes.

### 3. Implement Lateral Analysis Logic
7.  **Create `tools/lmuffb_log_analyzer/analyzers/lateral_analyzer.py`**:
    *   Implement logic to calculate raw load transfer and decompose `FFBSoP`.
8.  **Verification**: Use `read_file` to confirm the content.

### 4. Implement Diagnostic Plots
9.  **Modify `tools/lmuffb_log_analyzer/plots.py`**:
    *   Add `plot_lateral_diagnostic` function.
10. **Verification**: Use `read_file` to confirm the code.

### 5. Integrate into CLI and Reports
11. **Modify `tools/lmuffb_log_analyzer/reports.py`**:
    *   Add "LATERAL LOAD ANALYSIS" section to the text report.
12. **Modify `tools/lmuffb_log_analyzer/cli.py`**:
    *   Add the new plot call to `plots` and `batch` commands.
13. **Verification**: Use `read_file` to confirm the code.

### 6. Finalize
14. **Update Version**: Increment `VERSION`.
15. **Update Changelog**: Add entry to `CHANGELOG_DEV.md`.

## Test Plan
1.  **Build Verification**: Run `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build`.
2.  **Functional Verification (C++)**: Run `./build/tests/run_combined_tests`.
3.  **Log Analysis Verification**:
    *   Since I cannot run the game to generate a real log, I will create a small Python script to generate a "fake" `.bin` log file with the new header and some dummy data.
    *   Run the analyzer on this fake log: `python3 -m tools.lmuffb_log_analyzer.cli analyze_full <fake_log>`.
    *   Verify the report and plots are generated without errors.

## Additional Questions
- Should we also log the `sop_effect` (G-based part)? It is already logged as `understeer_effect` or `sop_effect`?
    - Checking `AsyncLogger.h`: It logs `understeer_effect` and `sop_effect`. `sop_effect` in the logger seems to correspond to `m_sop_effect` (G-based).
    - Checking `main.cpp`: `info.sop_effect = g_engine.m_sop_effect;`.
    - So we have the G-based effect, we just need the new Load-based effect.

## Implementation Notes
(To be filled during development)
