# Implementation Plan - Issue #271: Additional plots from the log analyzer (2)

This plan outlines the enhancements to the C++ telemetry logger and the Python log analyzer to include new diagnostic channels and plots for yaw dynamics and grip analysis.

**GitHub Issue:** #271 "Additional plots from the log analyzer (2)"

---

## Design Rationale

### Overall Approach
The goal is to provide deeper insights into Force Feedback (FFB) behavior, specifically focusing on "pull" forces and the relationship between grip loss and yaw kick spikes. By adding more raw and derived data to the telemetry logs, we can mathematically validate proposed changes to the FFB algorithms (like switching from game-provided raw acceleration to velocity-derived acceleration) before implementing them.

### Physics-Based Reasoning
1.  **Pull Detection**: FFB signals often contain high-frequency noise that masks sustained, low-frequency forces ("pulls"). A rolling average acts as a low-pass filter, revealing the DC offset of the signal which represents the constant torque felt by the driver during a slide or sustained rotation.
2.  **Unopposed Force Analysis**: Yaw kick effects are intended to provide seat-of-the-pants (SoP) feel. However, if these spikes occur exactly when the `GripFactor` (and thus the base steering weight) drops to zero, the wheel becomes "loose" and the yaw kick can yank the wheel violently. Visualizing this correlation is critical for tuning effect balancing.
3.  **Derived Acceleration**: Raw acceleration from game engines is often noisy or stepped (100Hz). Calculating acceleration from the derivative of 400Hz upsampled velocity may provide a cleaner signal for FFB calculation.

---

## 1. Codebase Analysis

### 1.1 Current Architecture Overview
- **C++ Logger (`AsyncLogger.h`)**: Uses a packed `LogFrame` struct (511 bytes) to store 400Hz telemetry. Data is written to binary files, optionally compressed with LZ4.
- **FFB Engine (`FFBEngine.cpp`)**: Populates the `LogFrame` at the end of each physics tick (400Hz). It uses `LinearExtrapolator` to upsample 100Hz game data.
- **Python Log Analyzer**: Uses `numpy.frombuffer` with a structured `dtype` (`LOG_FRAME_DTYPE`) to ingest binary logs. Plots are generated using `matplotlib`.

### 1.2 Impacted Functionalities
- **Telemetry Binary Format**: Adding fields to `LogFrame` changes the struct size and offsets, requiring a synchronized update in the Python loader.
- **FFB Logging Loop**: Additional math (derivative) and data assignment in the hot path of the FFB thread.
- **Diagnostic Plotting Suite**: New visualization capabilities in the analyzer.

### 1.3 Design Rationale
- **`LogFrame` Expansion**: We add fields at the end of the physics/algorithm sections to maintain some logical grouping, though the binary format is strictly ordered.
- **Derived Math**: Performing the derivative in C++ ensures it is captured at the full 400Hz resolution using the exact `dt` used by the engine.

---

## 2. FFB Effect Impact Analysis

| Affected FFB Effect | Technical Changes | Expected User-Facing Changes |
| :--- | :--- | :--- |
| **Yaw Kick** | None (Logging only) | No direct feel change, but provides data to justify future filtering changes. |
| **Grip Factor** | None (Logging only) | Improved understanding of how grip loss affects wheel weight during spikes. |

### Design Rationale
This task is purely diagnostic. No logic affecting the actual FFB output is being modified in this phase. The "passive test" of derived yaw acceleration is for comparison only.

---

## 3. Proposed Changes

### 3.1 C++ Changes (src/)

#### `src/AsyncLogger.h`
- Add `float extrapolated_yaw_accel;` and `float derived_yaw_accel;` to `LogFrame` struct.
- Update `WriteHeader` CSV header string to include these fields.

#### `src/FFBEngine.h`
- Add `double m_prev_yaw_rate_log = 0.0;` as a private member.

#### `src/FFBEngine.cpp`
- In `FFBEngine::calculate_force` (logging section):
    - `frame.extrapolated_yaw_accel = (float)upsampled_data->mLocalRotAccel.y;`
    - `double current_yaw_rate = upsampled_data->mLocalRot.y;`
    - `frame.derived_yaw_accel = (float)((current_yaw_rate - m_prev_yaw_rate_log) / ctx.dt);`
    - `m_prev_yaw_rate_log = current_yaw_rate;`

### 3.2 Python Changes (tools/lmuffb_log_analyzer/)

#### `tools/lmuffb_log_analyzer/loader.py`
- Update `LOG_FRAME_DTYPE` to include `extrapolated_yaw_accel` and `derived_yaw_accel` as `np.float32`.
- Update `mapping` dictionary to map these to `ExtrapolatedYawAccel` and `DerivedYawAccel`.

#### `tools/lmuffb_log_analyzer/plots.py`
- **`plot_pull_detector(df, ...)`**:
    - Calculate 0.5s rolling average of `SmoothedYawAccel` and `FFBYawKick`.
    - Plot both on a single panel to show low-frequency alignment.
- **`plot_unopposed_force(df, ...)`**:
    - Plot `GripFactor` on left Y-axis (0.0 to 1.0).
    - Plot `FFBYawKick` on right Y-axis (Nm).

#### `tools/lmuffb_log_analyzer/cli.py`
- Add calls to the new plots in `_run_plots` when `--all` is specified.

### 3.3 Version Increment
- Increment version in `VERSION` from `0.7.137` to `0.7.138`.

---

## 4. Test Plan

### 4.1 Unit Tests
- **C++**: Build project to verify struct alignment and no compilation errors.
- **Python**: Run existing tests. Add a check in `loader` tests to ensure the new fields are present in the resulting DataFrame.

### 4.2 Diagnostic Verification
- Generate a dummy log file (or use an existing one if possible) to verify that the new plots render without exceptions.
- Verify rolling average window size (200 samples at 400Hz = 0.5s).

### Design Rationale
The primary risk is a mismatch between the C++ struct and the Python `dtype`. Verifying the build and basic loading ensures the binary interface is intact.

---

## 5. Deliverables
- [ ] Modified `src/AsyncLogger.h`
- [ ] Modified `src/FFBEngine.h`
- [ ] Modified `src/FFBEngine.cpp`
- [ ] Modified `tools/lmuffb_log_analyzer/loader.py`
- [ ] Modified `tools/lmuffb_log_analyzer/plots.py`
- [ ] Modified `tools/lmuffb_log_analyzer/cli.py`
- [ ] Updated `VERSION` and changelogs.
- [ ] Implementation Notes (updated after completion).

---

## 6. Implementation Notes
- **Unforeseen Issues:**
    - The code review initially flagged a potential CSV corruption. Upon investigation, it was confirmed that the "CSV Header" in the binary log file is purely informational, and since the binary struct and the informational header were both updated synchronously, the log remains internally consistent for its intended use case (Python Log Analyzer).
- **Plan Deviations:**
    - Added a `m_yaw_rate_log_seeded` flag to ensure `m_prev_yaw_rate_log` is initialized with the current value on the first logged frame, preventing a large initial derivative spike.
- **Challenges:**
    - Ensuring the mathematical definition of "Yaw Accel" correctly utilized the derivative of angular velocity (`mLocalRot.y`). Critical verification of the ISI/LMU telemetry structure confirmed that `mLocalRot` represents velocity in radians/sec, making its time-derivative acceleration.
- **Recommendations:**
    - The "passive test" data captured in this version should be analyzed to compare `ExtrapolatedYawAccel` (noisy game data) with `DerivedYawAccel` (cleaner velocity derivative) to justify a full switch in the physics engine.
