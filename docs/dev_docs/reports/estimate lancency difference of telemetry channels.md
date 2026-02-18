Yes, you can absolutely perform signal analysis to determine the latency difference, and it is a standard procedure in control systems engineering.

Here is how you can approach the analysis for both scenarios.

### 1. The "Gold Standard" Method: Cross-Correlation
This method works for **both** cases (identical or slightly different signals) and is the most mathematically robust way to measure time delay (lag).

**The Concept:**
You slide one signal over the other in time. At each time step (lag), you calculate how much they "overlap" or match. The time shift ($\tau$) that produces the maximum correlation value is your latency.

**Handling the Differences (Case 2):**
If `FFBTorque` contains extra high-frequency noise (ABS, road texture) that `mSteeringShaftTorque` lacks, raw cross-correlation might be noisy.
*   **Solution:** Apply a **Low-Pass Filter (LPF)** to *both* signals before analysis (e.g., a 10Hz Butterworth filter).
*   **Why:** This strips away the high-frequency "effects" (ABS, kerbs) and leaves only the low-frequency "macro" physics (weight transfer, self-aligning torque), which should be present in both signals.

**The Algorithm:**
1.  **Resample:** Upsample the 100Hz `mSteeringShaftTorque` to 400Hz to match `FFBTorque`. Use **"Zero-Order Hold"** (repeat the last value) rather than linear interpolation, as this accurately represents how the game updates the value (step-wise).
2.  **Normalize:** Since one might be in Newton-meters (e.g., 15.0) and the other normalized (e.g., 1.0), apply **Z-Score Normalization** to both:
    $$x' = \frac{x - \mu}{\sigma}$$
    (Subtract mean, divide by standard deviation).
3.  **Filter:** Apply a 10Hz Low-Pass Filter to both normalized signals.
4.  **Correlate:** Compute the Cross-Correlation function.
5.  **Find Peak:** The index of the maximum value in the correlation array corresponds to the frame delay.
    $$Latency (ms) = PeakIndex \times \Delta t_{400Hz} (2.5ms)$$

### 2. The "Event Detection" Method (Peak Matching)
This is useful if the signals are significantly different (Case 2) or if you want to visualize specific instances of lag.

**The Concept:**
Identify a sharp, distinct event—like hitting a curb or a rapid change in direction—and measure the timestamp difference between when it appears in Signal A vs Signal B.

**The Algorithm:**
1.  **Calculate Derivative:** Compute the rate of change ($d/dt$) for both signals. This highlights rapid changes.
2.  **Thresholding:** Find points where the derivative exceeds a high threshold (an "Event").
3.  **Window Search:**
    *   Find a peak in `FFBTorque` at time $T_1$.
    *   Look for the *nearest* peak in `mSteeringShaftTorque` within a reasonable window (e.g., $T_1$ to $T_1 + 100ms$).
    *   Record $\Delta t = T_{peak2} - T_{peak1}$.
4.  **Average:** Repeat for 10-20 events and average the result to remove jitter.

### Python Implementation Example
Since you already have a Python log analyzer, here is a snippet you can add to `analyzers/slope_analyzer.py` or a new module to perform this check automatically.

```python
import numpy as np
import pandas as pd
from scipy import signal

def calculate_signal_latency(df: pd.DataFrame) -> dict:
    """
    Calculates the latency lag of ShaftTorque (100Hz) vs FFBTorque (400Hz).
    Positive lag means ShaftTorque is BEHIND FFBTorque.
    """
    # 1. Check columns exist
    if 'mSteeringShaftTorque' not in df.columns or 'FFBTorque' not in df.columns:
        return {"error": "Missing columns"}

    # 2. Extract Data
    # Assuming df is already at the high-frequency rate (log rows)
    sig_fast = df['FFBTorque'].values
    sig_slow = df['mSteeringShaftTorque'].values
    
    # 3. Pre-processing (Handle NaN and Z-Score Normalize)
    sig_fast = np.nan_to_num(sig_fast)
    sig_slow = np.nan_to_num(sig_slow)
    
    # Normalize to handle unit differences (Nm vs %)
    sig_fast_norm = (sig_fast - np.mean(sig_fast)) / (np.std(sig_fast) + 1e-6)
    sig_slow_norm = (sig_slow - np.mean(sig_slow)) / (np.std(sig_slow) + 1e-6)

    # 4. Low-Pass Filter (Critical if signals have different effects)
    # Filter at 10Hz to isolate the main steering forces
    sos = signal.butter(4, 10, 'low', fs=400, output='sos')
    sig_fast_filt = signal.sosfilt(sos, sig_fast_norm)
    sig_slow_filt = signal.sosfilt(sos, sig_slow_norm)

    # 5. Cross-Correlation
    # We only need to search a small window (e.g., +/- 200ms)
    window_size = int(0.2 * 400) # 80 frames
    correlation = signal.correlate(sig_fast_filt, sig_slow_filt, mode='full')
    lags = signal.correlation_lags(len(sig_fast_filt), len(sig_slow_filt), mode='full')
    
    # 6. Find Peak
    peak_idx = np.argmax(correlation)
    lag_frames = lags[peak_idx]
    
    # Convert to ms (assuming 400Hz / 2.5ms dt)
    latency_ms = lag_frames * 2.5
    
    # Check correlation strength (0.0 to 1.0)
    # If signals are inverted, peak will be negative, so take abs value of correlation
    correlation_strength = np.abs(correlation[peak_idx]) / np.sqrt(np.dot(sig_fast_filt, sig_fast_filt) * np.dot(sig_slow_filt, sig_slow_filt))

    return {
        "latency_ms": latency_ms,
        "correlation_strength": correlation_strength,
        "is_inverted": correlation[peak_idx] < 0
    }
```

### Interpreting the Results

1.  **Latency > 0ms (e.g., 20-40ms):**
    *   This confirms `mSteeringShaftTorque` is delayed.
    *   **Action:** You should definitely switch to `FFBTorque` for your derivative calculations (Slope Detection), as 40ms of lag is huge for catching a slide.

2.  **Latency $\approx$ 0ms:**
    *   This would imply `FFBTorque` is just a copy of the 100Hz signal, upsampled by the game.
    *   **Check:** Look at the "stair-stepping" in the plots. If `FFBTorque` is smooth while `ShaftTorque` looks like stairs, `FFBTorque` is true 400Hz. If both look like stairs, `FFBTorque` is fake 400Hz.

3.  **Correlation Strength < 0.5:**
    *   The signals are too different (Case 2 is extreme).
    *   **Action:** Rely on the Low-Pass Filtered visual comparison (plot them on top of each other) rather than the calculated number.

### Summary
Yes, **Cross-Correlation on Low-Pass Filtered data** is the robust solution. It handles different units, different noise levels, and even inverted signals, giving you a precise measurement of the "age" of the `mSteeringShaftTorque` data compared to the live `FFBTorque`.