# Implementation Plan - Log Analyzer Enhancements (v0.7.19)

## Context
Following the analysis of slope detection instability (Task `slope_detection_v0.7.16_analysis`), we identified that numerical explosions occur when `dAlpha/dt` is near the activation threshold. To better detect and visualize these issues in future logs, we need to enhance the Python Log Analyzer tool with specific metrics and plots for "singularity" events.

## Reference Documents
- **Investigation Report:** `docs/dev_docs/investigations/slope_detection_v0.7.16_analysis.md`
- **Current Analyzer Code:** `tools/lmuffb_log_analyzer/`

## Codebase Analysis Summary
### Current Architecture
The Log Analyzer is a Python package that processes CSV telemetry logs.
- `analyzers/` module contains specific analysis logic (e.g., `slope_analyzer.py`).
- `plots.py` handles matplotlib visualizations.
- `reports.py` generates the text summary.

### Impacted Functionalities
1.  **Slope Analysis (`analyzers/slope_analyzer.py`)**: Needs new logic to detect "Singularity Events" (high slope with low slip rate).
2.  **Visualization (`plots.py`)**: Needs new correlation plots (`dAlpha` vs `Slope`).
3.  **Reporting (`reports.py`)**: Needs to output these new metrics.

## FFB Effect Impact Analysis
*   **N/A**: This is an offline tool change, no runtime FFB impact.

## Proposed Changes

### 1. Singularity Detection Logic
*   **File:** `tools/lmuffb_log_analyzer/analyzers/slope_analyzer.py`
*   **New Function:** `detect_singularities(df, slope_thresh=10.0, alpha_rate_thresh=0.05)`
*   **Logic:**
    *   Filter rows where `abs(SlopeCurrent) > slope_thresh`.
    *   AND `abs(dAlpha_dt) < alpha_rate_thresh`.
    *   Return count of such events and the worst-case values.

### 2. New Text Metrics (Signal Quality)
*   **File:** `tools/lmuffb_log_analyzer/analyzers/slope_analyzer.py`
*   **Metrics:**
    *   **Zero-Crossing Rate (Hz):** Count sign changes / time duration.
    *   **Binary State Residence:** % of frames where Grip is in [0.2, 0.25] or [0.95, 1.0].
    *   **Derivative Energy Ratio:** `std(dG) / std(dAlpha)`.
*   **Reporting:** Output these in the text summary to indicate "Signal Quality" (Good/Noisy/Binary).

### 3. New Visualization
*   **File:** `tools/lmuffb_log_analyzer/plots.py`
*   **New Plot:** `plot_slope_correlation(df)`
    *   **Type:** Scatter Plot.
    *   **X-Axis:** `dAlpha_dt` (Slip Angle Rate).
    *   **Y-Axis:** `SlopeCurrent`.
    *   **Style:** Alpha=0.1 (transparency) to show density.
    *   **Annotation:** Draw vertical lines at ±0.02 (current threshold).

### 4. Report Update
*   **File:** `tools/lmuffb_log_analyzer/reports.py`
*   **Update:** Add a "Stability & Signal Quality" section to the Slope Report.

## Parameter Synchronization Checklist
*   N/A (No C++ parameters involved).

## Version Increment Rule
*   Increment `VERSION` file: Patch +1 (e.g., 0.7.16 -> 0.7.17).

## Test Plan
This is Python code, so we will update/add checks in `tools/lmuffb_log_analyzer/tests/`.

### 1. Test Singularity Detection
*   **File:** `tools/lmuffb_log_analyzer/tests/test_analyzers.py` (or create `test_slope_analyzer.py`)
*   **Case:** `test_detect_singularities`
    *   **Input:** DataFrame with engineered "exploded" slopes at low alpha rates.
    *   **Assert:** Tool correctly counts these rows as instability events.

## Deliverables
- [ ] Code changes in `slope_analyzer.py`, `plots.py`, `reports.py`.
- [ ] Updated `requirements.txt` (if new libs needed, unlikely).
- [ ] Updated `README.md` for the tool.

## Implementation Notes
- **Singularity Detection:** Successfully implemented with configurable thresholds. Tests verify correct counting.
- **Signal Quality Metrics:** Zero-Crossing Rate (Hz), Binary State Residence (%), and Derivative Energy Ratio added to `analyze_slope_stability`.
- **Visualization:** `plot_slope_correlation` added to `plots.py` and integrated into CLI `--all` flag.
- **CLI Improvements:** Added "Singularity Events" and "Worst Singularity" to the `analyze` command output table.
- **Version Increment:** Bumped to 0.7.19 (from 0.7.18).
- **No significant issues encountered. Implementation proceeded as planned.**
