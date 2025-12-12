## Question 
I am also considering implementing a tool (a separate executable) that reads one log, and perform some diagnostics / "tests" / checks to see if there are any issue, and then writes a report. As a preliminary design discussion, how would you implement such analysis tool? Which diagnostic checks would you make it do?

## Answer 
This is an excellent idea. An automated analysis tool moves debugging from "staring at lines on a screen" to "data-driven engineering."

Since this is an offline tool (not real-time), we have more flexibility in technology choice.

### 1. Technology Choice: Python vs. C++

While your main app is C++, **Python** is vastly superior for this specific task.
*   **Libraries:** `pandas` (data manipulation) and `numpy` (math) allow you to write complex statistical checks in one line of code that would take 50 lines in C++.
*   **Plotting:** `matplotlib` can generate static PNG graphs for the report automatically.
*   **Distribution:** You can compile it to a standalone `.exe` using **PyInstaller** so users don't need to install Python.

**Recommendation:** Build this tool in **Python**.

---

### 2. Diagnostic Checks (The "Test Suite")

Here are the specific heuristics the tool should run on the CSV data.

#### A. Telemetry Health (The "Game Integration" Check)
*   **The "Zero Load" Bug:**
    *   *Check:* Calculate the percentage of frames where `RawLoad == 0.0` while `CarSpeed > 1.0 m/s`.
    *   *Threshold:* If > 90%, report: **"CRITICAL: Game is not sending Tire Load data. Fallback logic is active."**
*   **The "Frozen" Sensor:**
    *   *Check:* Calculate the standard deviation of `RawGrip`.
    *   *Threshold:* If `std_dev < 0.0001` (perfectly flat) while moving, report: **"WARNING: Grip data appears frozen/static."**
*   **Performance Jitter:**
    *   *Check:* Analyze the `Time` column delta.
    *   *Threshold:* If `max(delta_time) > 0.020` (20ms, i.e., < 50Hz), report: **"WARNING: FFB Loop Stutter detected. CPU bottleneck?"**

#### B. FFB Quality (The "Feeling" Check)
*   **Clipping Analysis:**
    *   *Check:* Percentage of frames where `abs(FFB_Total) >= 1.0`.
    *   *Report:*
        *   `> 5%`: "Severe Clipping. Reduce Master Gain."
        *   `1% - 5%`: "Moderate Clipping. Acceptable for peaks."
        *   `0%`: "No Clipping. Consider increasing Gain for more detail."
*   **Oscillation Detection (Ping-Pong):**
    *   *Check:* Count how many times `FFB_Total` flips sign (positive to negative) within a 100ms window while `SteeringTorque` is relatively constant.
    *   *Report:* **"WARNING: High-frequency oscillation detected. Increase Smoothing or reduce Scrub Drag."**
*   **Deadzone Check:**
    *   *Check:* Histogram of forces. If there is a massive spike exactly at `0.0` while inputs are non-zero.
    *   *Report:* "Potential Deadzone issue."

#### C. Physics Logic Verification (The "Workaround" Check)
*   **Load Approximation Accuracy:**
    *   *Check:* If `RawLoad` is valid (non-zero), calculate the correlation coefficient (R²) between `RawLoad` and `CalcLoad`.
    *   *Report:* "Approximation Accuracy: 85% match with real physics." (Helps you tune the 300N constant).
*   **Understeer Trigger:**
    *   *Check:* Find moments where `CalcSlipAngle > 0.15` (Limit). Check if `FFB_Total` decreases relative to `SteeringTorque` in those moments.
    *   *Report:* "Understeer Effect Active: Yes/No."

---

### 3. Implementation Design (Python)

#### Structure
```python
import pandas as pd
import numpy as np

class LogAnalyzer:
    def __init__(self, csv_path):
        self.df = pd.read_csv(csv_path)
        
    def run_diagnostics(self):
        report = []
        report.append(self.check_telemetry_health())
        report.append(self.check_clipping())
        report.append(self.check_oscillation())
        return "\n".join(report)

    def check_telemetry_health(self):
        # Filter for moving car
        moving = self.df[self.df['RawLatVel'].abs() > 1.0]
        
        # Check Zero Load
        zero_load_count = (moving['RawLoadFL'] == 0).sum()
        if zero_load_count / len(moving) > 0.9:
            return "[FAIL] Raw Tire Load is missing (LMU Bug confirmed)."
        return "[PASS] Raw Tire Load is present."

    def check_clipping(self):
        clip_count = (self.df['FFB_Total'].abs() >= 0.99).sum()
        pct = (clip_count / len(self.df)) * 100
        return f"[INFO] Clipping: {pct:.2f}% of session."
```

### 4. The Output Report (Example)

The tool should generate a `report.txt` (or print to console):

```text
==================================================
LMUFFB DIAGNOSTIC REPORT
Log: lmuffb_log_2025-12-12.csv
Duration: 4m 32s
==================================================

1. TELEMETRY HEALTH
   [FAIL] Raw Tire Load: MISSING (100% of frames are 0.0)
          -> The app is correctly using the Calculated Fallback.
   [PASS] Raw Grip: Present.
   [WARN] Loop Rate: Min 380Hz, Max 410Hz. (Stable).

2. FFB SIGNAL QUALITY
   [WARN] Clipping: 12.5% (High). 
          -> Recommendation: Reduce Master Gain or Max Torque Ref.
   [PASS] Oscillation: None detected.
   [INFO] Average Force: 8.4 Nm.

3. PHYSICS LOGIC
   [INFO] Scrub Drag: Active (Max force 2.5 Nm).
   [INFO] Bottoming: Triggered 3 times.
          -> Timestamps: 12.5s, 45.2s, 1m20s.

==================================================
```

### 5. How to Integrate
1.  **Development:** Write the script in `tools/log_analyzer.py`.
2.  **Build:** Use `pyinstaller --onefile tools/log_analyzer.py` to create `LogAnalyzer.exe`.
3.  **Distribution:** Include `LogAnalyzer.exe` in the LMUFFB release zip.
4.  **Usage:** User drags and drops a CSV file onto `LogAnalyzer.exe`, and it opens a window with the text report.
