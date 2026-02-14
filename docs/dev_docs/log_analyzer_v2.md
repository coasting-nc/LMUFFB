# Log Analyzer v2 - Advanced Analysis Specification

This document defines the technical specifications for new diagnostic features in the external `lmuffb_log_analyzer` tool, leveraging the expanded telemetry data introduced in versions 0.7.38 and 0.7.39.

## 1. Phase Lag Analysis (Cross-Correlation)

To optimize the Slope Detection Savitzky-Golay window size (`m_slope_sg_window`), we need to measure the physical phase lag between steering input and tire lateral force generation.

### 1.1 Inputs
- Primary: `dAlpha_dt` (Steering Rate) from telemetry log.
- Secondary: `dG_dt` (Lateral G Rate) from telemetry log.
- Context: `SurfaceFL`, `SurfaceFR`, `Speed`.

### 1.2 Algorithm
1.  **Data Filtering**:
    - Exclude all frames where `SurfaceFL != 0` or `SurfaceFR != 0` (non-asphalt surfaces introduce excessive noise).
    - Exclude all frames where `Speed < 10.0 m/s`.
    - Focus on transient cornering sections (where `abs(dAlpha_dt) > 0.05`).
2.  **Signal Normalization**:
    - Apply Z-score normalization to both `dAlpha_dt` and `dG_dt` within the analyzed segment to ensure consistent correlation scaling.
3.  **Cross-Correlation Calculation**:
    - Compute the Cross-Correlation $R(\tau)$ between the normalized signals for lags $\tau$ ranging from $0ms$ to $100ms$.
4.  **Peak Detection**:
    - Identify $\tau_{peak}$ where $R(\tau)$ is maximized. This represents the average physical lag (tire relaxation length + mechanical compliance).

### 1.3 Recommended Outputs
- **Measured Physical Lag**: $X$ ms.
- **Recommended Window Size**: $N = \tau_{peak} / dt$ samples.
    - *Note*: The window size should ideally match or slightly exceed the physical lag to ensure the derivative calculation covers the full tire response event.

## 2. Surface-Aware Filtering

The inclusion of `SurfaceFL` and `SurfaceFR` allows the analyzer to automatically ignore curb strikes and off-track excursions, which previously caused false "oscillation" or "instability" warnings in the reports.

### 2.1 Filtering Rules
- **Curb Detection**: Any frame where `SurfaceType == 5`.
- **Off-Track Detection**: Any frame where `SurfaceType \in {2, 3, 4}`.
- **Rule**: These frames should be excluded from `SlopeCurrent` stability statistics and "Peak Grip" identification.

## 3. Implementation Targets
These features are intended for implementation in the Python-based `tools/lmuffb_log_analyzer/` package.
