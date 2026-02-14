# Implementation Plan - Shadow Mode & Telemetry Validation

## 1. Context

Currently, `lmuFFB` uses estimators (Slope Detection, Friction Circle, Kinematic Load) when data is missing. To validate and tune these estimators, we need to run them in the background ("Shadow Mode") even when valid data is present, log the results, and compare them against the real telemetry.

**Goal:**
1.  **C++ Engine:** Implement "Shadow Mode" to calculate and log estimated values alongside real values without affecting FFB output.
2.  **Log Analyzer:** Add a validation module to visualize the correlation between Real Data (Ground Truth) and Estimated Data (Shadow).

**Reference Documents:**
*   `src/FFBEngine.h` (Current estimation logic)
*   `src/AsyncLogger.h` (Logging structure)
*   `tools/lmuffb_log_analyzer/` (Python analysis suite)

## 2. Codebase Analysis

### 2.1 Architecture Overview
*   **`FFBEngine::calculate_grip`:** Currently contains a conditional branch. If real grip is present, it skips the Slope/Friction Circle calculations.
*   **`FFBEngine::calculate_force`:** Currently calculates Kinematic Load only inside the fallback block when `m_missing_load_frames` triggers.
*   **`AsyncLogger`:** Writes a fixed CSV format. Needs expansion for new columns.
*   **Log Analyzer:** Modular Python tool using Pandas/Matplotlib. Needs a new subcommand for validation.

### 2.2 Impacted Functionalities
*   **Performance:** Slight increase in CPU usage per frame due to running estimation math unconditionally. This is negligible (simple trigonometry and algebra).
*   **FFB Output:** **No Change.** The shadow values are strictly for logging. The FFB logic will continue to prioritize real telemetry when available.
*   **Log File:** CSV file size will increase slightly (3 additional float columns).

## 3. Proposed Changes

### 3.1 File: `src/FFBEngine.h`

**A. Add Shadow State Members**
Add member variables to store the estimated values for the current frame.
```cpp
public: // or private, accessible by Logger
    // Shadow Telemetry (Validation)
    double m_shadow_grip_slope = 0.0;      // Slope Detection Estimate
    double m_shadow_grip_friction = 0.0;   // Friction Circle Estimate
    double m_shadow_load_kinematic = 0.0;  // Kinematic Load Estimate
```

**B. Refactor `calculate_grip`**
Move estimation logic *outside* the fallback conditional.
*   **Current:** `if (bad_data) { val = estimate(); }`
*   **New:**
    1.  `m_shadow_grip_slope = calculate_slope_grip(...)` (Always run)
    2.  `m_shadow_grip_friction = ...` (Always run)
    3.  `if (bad_data) { val = m_shadow_...; } else { val = real_data; }`

**C. Refactor `calculate_force` (Load Section)**
Move Kinematic Load calculation *outside* the fallback conditional.
*   **Current:** `if (missing_load) { val = calculate_kinematic_load(...); }`
*   **New:**
    1.  `m_shadow_load_kinematic = calculate_kinematic_load(...)` (Always run)
    2.  `if (missing_load) { val = m_shadow_load_kinematic; } else { val = real_data; }`

**D. Update `calculate_force` (Logging Section)**
Populate the `LogFrame` with the shadow members.

### 3.2 File: `src/AsyncLogger.h`

**A. Update `LogFrame` Struct**
```cpp
struct LogFrame {
    // ... existing fields ...
    float shadow_grip_slope;
    float shadow_grip_friction;
    float shadow_load_kinematic;
    // ...
};
```

**B. Update `WriteHeader`**
Add columns: `ShadowGripSlope,ShadowGripFriction,ShadowLoadKinematic`.

**C. Update `WriteFrame`**
Output the new fields to the CSV stream.

### 3.3 File: `tools/lmuffb_log_analyzer/` (Python)

**A. New File: `analyzers/validation.py`**
Implement statistical comparison logic.
*   Calculate Pearson correlation coefficient between `GripFL` and `ShadowGripSlope`.
*   Calculate Mean Absolute Error (MAE) for Load.

**B. New File: `plots_validation.py`**
Implement visualization functions using Matplotlib.
*   **Time Series:** Overlay Real vs. Shadow (Dual line plot).
*   **Scatter:** Real (X) vs. Shadow (Y) to show linearity.

**C. Update: `cli.py`**
Add a new command `validate`.
```bash
python -m lmuffb_log_analyzer validate <logfile.csv>
```

## 4. Test Plan

### Test 1: C++ Shadow Logic (Unit Test)
*   **File:** `tests/test_ffb_shadow_mode.cpp`
*   **Goal:** Verify shadow values are populated even when real data is perfect.
*   **Setup:**
    *   Inject telemetry with valid `mGripFract = 1.0` and `mTireLoad = 4000`.
    *   Configure Engine to enable Slope Detection.
*   **Action:** Call `calculate_force`.
*   **Assertion:**
    *   `ctx.avg_grip` should be `1.0` (Real data used).
    *   `m_shadow_grip_slope` should be non-zero (Estimator ran).
    *   `m_shadow_load_kinematic` should be non-zero (Estimator ran).

### Test 2: Log Integrity
*   **Goal:** Verify CSV columns align.
*   **Action:** Run a short session, open CSV.
*   **Assertion:** Ensure `ShadowGripSlope` column exists and contains varying data, not just zeros.

### Test 3: Analyzer Visualization
*   **Goal:** Verify the Python tool generates plots.
*   **Action:** Run `python -m lmuffb_log_analyzer validate test_log.csv`.
*   **Assertion:** Check `validation_results/` folder for `.png` images.

## 5. Deliverables

*   [ ] **Code:** Updated `src/FFBEngine.h` (Shadow logic).
*   [ ] **Code:** Updated `src/AsyncLogger.h` (New columns).
*   [ ] **Tool:** New `validate` command in Log Analyzer.
*   [ ] **Test:** New `tests/test_ffb_shadow_mode.cpp`.

## 6. Implementation Steps

### Step 1: C++ Core Updates
1.  Modify `FFBEngine.h` to add shadow members.
2.  Refactor `calculate_grip` to compute `m_shadow_grip_slope` and `m_shadow_grip_friction` unconditionally.
3.  Refactor `calculate_force` to compute `m_shadow_load_kinematic` unconditionally.
4.  Pass these values to `LogFrame` in `calculate_force`.

### Step 2: Logging Infrastructure
1.  Modify `AsyncLogger.h` to include new fields in `LogFrame`, `WriteHeader`, and `WriteFrame`.

### Step 3: Python Analyzer
1.  Create `tools/lmuffb_log_analyzer/plots_validation.py`.
2.  Update `tools/lmuffb_log_analyzer/cli.py` to register the command.

### Step 4: Verification
1.  Compile and run `tests/test_ffb_shadow_mode.cpp`.
2.  Perform a driving test with a car that has valid telemetry for tire grip and load to generate a "Gold Standard" log.

 