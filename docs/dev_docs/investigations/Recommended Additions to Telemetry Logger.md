### Recommended Additions to Telemetry Logger (`AsyncLogger.h`)

To properly diagnose the "Singularities" and "Steady-State" issues discussed in the previous step, we need to log the **internal state of the math**, not just the inputs and outputs.

Here are the specific changes to make to `AsyncLogger.h` and `FFBEngine.h`.

#### A. Update `LogFrame` Struct
We need to capture the "Hold" state (to verify the fix for low active time) and the raw math components (to verify the fix for singularities).

**File:** `src/AsyncLogger.h`

```cpp
struct LogFrame {
    // ... existing fields ...

    // --- NEW DIAGNOSTIC FIELDS ---
    float slope_raw_unclamped; // The raw result of (num / den) before clamping to +/-20
    float slope_numerator;     // dG * dAlpha
    float slope_denominator;   // dAlpha^2 + epsilon
    float hold_timer;          // Value of m_slope_hold_timer (are we in steady state?)
    float input_slip_smoothed; // The pre-smoothed slip angle feeding the derivative
    
    // ... existing fields ...
};
```

#### B. Update CSV Header
**File:** `src/AsyncLogger.h` (inside `WriteHeader`)

```cpp
    void WriteHeader(const SessionInfo& info) {
        // ... existing headers ...
        
        // Add new columns to the CSV header string
        m_file << "Time,DeltaTime,Speed,LatAccel,LongAccel,YawRate,Steering,Throttle,Brake,"
               << "SlipAngleFL,SlipAngleFR,SlipRatioFL,SlipRatioFR,GripFL,GripFR,LoadFL,LoadFR,"
               << "CalcSlipAngle,CalcGripFront,CalcGripRear,GripDelta,"
               << "dG_dt,dAlpha_dt,SlopeCurrent,SlopeSmoothed,Confidence,"
               // NEW COLUMNS HERE:
               << "SlopeRaw,SlopeNum,SlopeDenom,HoldTimer,InputSlipSmooth," 
               << "FFBTotal,FFBBase,FFBSoP,GripFactor,SpeedGate,Clipping,Marker\n";
    }
```

#### C. Update Write Logic
**File:** `src/AsyncLogger.h` (inside `WriteFrame`)

```cpp
    void WriteFrame(const LogFrame& frame) {
        m_file << std::fixed << std::setprecision(4)
               // ... existing fields ...
               << frame.dG_dt << "," << frame.dAlpha_dt << "," << frame.slope_current << "," << frame.slope_smoothed << "," << frame.confidence << ","
               
               // NEW FIELDS
               << frame.slope_raw_unclamped << "," 
               << frame.slope_numerator << "," 
               << frame.slope_denominator << "," 
               << frame.hold_timer << ","
               << frame.input_slip_smoothed << ","
               
               << frame.ffb_total << "," // ... rest of line
    }
```

#### D. Populate Data in Engine
**File:** `src/FFBEngine.h` (inside `calculate_force`, where `LogFrame` is populated)

You will need to expose these internal variables from `calculate_slope_grip` to the class scope or return them in a struct so `calculate_force` can access them.

```cpp
// Inside FFBEngine::calculate_force logging block:

frame.slope_raw_unclamped = (float)m_debug_slope_raw; // You need to store this in class member during calc
frame.slope_numerator     = (float)m_debug_slope_num;
frame.slope_denominator   = (float)m_debug_slope_den;
frame.hold_timer          = (float)m_slope_hold_timer;
frame.input_slip_smoothed = (float)m_slope_slip_smoothed;
```

---

### 3. Recommended Additions to Log Analyser (Python/Tool)

Since I cannot see the code, I suggest implementing these specific analyses in your tool. These are designed to visualize the specific failures identified in the reports.

#### A. The "Hysteresis Loop" Plot (G vs. Slip)
The most powerful way to visualize slope detection is not a time-series graph, but an X-Y scatter plot.

*   **X-Axis:** `CalcSlipAngle` (or `InputSlipSmooth`)
*   **Y-Axis:** `LatAccel`
*   **Color:** `SlopeCurrent` (Gradient from Red=Negative to Blue=Positive)

**Why:**
*   **Linear Region:** Points should form a straight line going up.
*   **Understeer:** The line should flatten out (plateau).
*   **The Bug:** If you see a "cloud" of points with no structure, your input noise is too high. If you see the color flickering rapidly between Red/Blue in the linear region, your derivative calculation is unstable.

#### B. The "Derivative Phase Plane" (dG vs. dAlpha)
To diagnose the "Singularities" (8268 events):

*   **X-Axis:** `dAlpha_dt`
*   **Y-Axis:** `dG_dt`
*   **Filter:** Only plot points where `HoldTimer > 0` (Active calculation).

**Why:**
*   Ideally, points should cluster along a diagonal line (Slope = 1.0).
*   **Singularities:** Look for points close to the Y-axis (where X is near 0) but high up on Y. These are the math explosions.
*   **Deadzone:** Visualize your `m_slope_alpha_threshold`. Draw vertical lines at +/- threshold. Are valid cornering events being ignored inside this zone?

#### C. State Distribution Pie Chart
Analyze the `HoldTimer` and `dAlpha_dt` to categorize every frame into one of three states:

1.  **Transient (Active):** `abs(dAlpha) > threshold`
2.  **Steady-State (Hold):** `abs(dAlpha) < threshold` AND `HoldTimer > 0`
3.  **Decay/Reset:** `abs(dAlpha) < threshold` AND `HoldTimer <= 0`

**Goal:**
*   In the McLaren report, "Active Time" was 13.8%.
*   With the fix, you want to see **"Steady-State (Hold)"** take up a significant portion (e.g., 30-40%) during cornering. If "Decay" is high during cornering, the Hold Timer is too short.

#### D. Noise Frequency Analysis (FFT)
Perform a Fast Fourier Transform (FFT) on `dAlpha_dt`.

*   **Goal:** Identify the specific frequency of the noise (reported as 8.69 Hz).
*   **Action:** If the peak is at 8-10Hz, it confirms suspension oscillation or FFB vibration feeding back into the wheel. If the peak is >50Hz, it's sensor noise. This tells you exactly how to tune the `input_tau` smoothing factor.