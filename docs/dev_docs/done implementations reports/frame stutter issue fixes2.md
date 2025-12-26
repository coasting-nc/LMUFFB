Based on a review of the codebase, the "Phase Explosion" bug in the oscillators (Lockup, Spin, Slide, Bottoming) appears to be **correctly fixed** in `FFBEngine.h` by the use of `std::fmod(phase, TWO_PI)`.

However, I have identified **three additional issues** related to frame stutters and stability:

### 1. "Time Dilation" Bug in Smoothing Filters (Frame Stutter Sensitivity)
While the oscillators are now safe, several Low Pass Filters (LPF) in the engine use a **Fixed Alpha** (coefficient) instead of a **Time-Corrected Alpha**.

*   **The Bug:** A fixed alpha (e.g., `0.1`) assumes a constant frame rate (400Hz). If a frame stutter occurs (e.g., `dt` jumps from 2.5ms to 50ms), the filter still only applies "2.5ms worth" of smoothing.
*   **Consequence:** After a stutter, values like **Slip Angle** and **Yaw Acceleration** will "lag" significantly behind reality. The FFB will feel unresponsive or "stuck" for several frames until the filter catches up.
*   **Affected Areas in `FFBEngine.h`:**
    1.  **Slip Angle Smoothing:** `double alpha = 0.1;` (Line ~260)
    2.  **Yaw Acceleration:** `double alpha_yaw = 0.1;` (Line ~520)
    3.  **Gyro Damping:** `double alpha_gyro = ...` (Line ~544) - Uses user setting directly as alpha.
*   **Fix:** Use the time-corrected formula: `alpha = dt / (tau + dt)`, where `tau` is the time constant (e.g., 0.025s).

### 2. Unstable Filter Configuration Risk
The **Gyroscopic Damping** smoothing factor (`m_gyro_smoothing`) is loaded from config but is **not clamped** to a safe range `[0.0, 1.0]` in the calculation logic.
*   **The Risk:** If a user manually edits `config.ini` and sets `gyro_gain` to a negative value (e.g., `-0.1`), the smoothing filter becomes mathematically unstable (exponential growth), causing an immediate force explosion.
*   **Fix:** Add `m_gyro_smoothing = (std::max)(0.0f, (std::min)(1.0f, m_gyro_smoothing));` in `calculate_force`.

### 3. Dangerous Code in Documentation
The file `docs/dev_docs/Yaw, Gyroscopic Damping... implementation.md` contains a code snippet for a proposed "Hydro-Grain" effect that **still uses the buggy logic**:
```cpp
// IN DOCUMENTATION (Unsafe):
if (m_hydro_phase > TWO_PI) m_hydro_phase -= TWO_PI; 
```
*   **Risk:** If a developer copies this snippet to implement wet weather effects, they will re-introduce the Phase Explosion bug.
*   **Fix:** Update the documentation to use `std::fmod`.

---

### Recommended Fixes

Here is the code to fix the **Time Dilation** (Fixed Alpha) bugs in `FFBEngine.h`.

#### A. Fix Slip Angle Smoothing
```cpp
// FFBEngine.h : calculate_slip_angle

// OLD (Fixed Alpha - Lags during stutter)
// double alpha = 0.1; 

// NEW (Time Corrected)
// Target ~0.1 at 400Hz (dt=0.0025). 
// alpha = dt / (tau + dt) -> 0.1 = 0.0025 / (tau + 0.0025) -> tau approx 0.0225s
const double tau = 0.0225; 
double dt = 0.0025; // You need to pass 'dt' into this helper function!
double alpha = dt / (tau + dt);

prev_state = prev_state + alpha * (raw_angle - prev_state);
```
*Note: You will need to update the `calculate_slip_angle` signature to accept `dt`.*

#### B. Fix Yaw Acceleration Smoothing
```cpp
// FFBEngine.h : calculate_force

// OLD
// double alpha_yaw = 0.1;

// NEW
const double tau_yaw = 0.0225; // Approx same smoothing as 0.1 at 400Hz
double alpha_yaw = dt / (tau_yaw + dt);

m_yaw_accel_smoothed = m_yaw_accel_smoothed + alpha_yaw * (raw_yaw_accel - m_yaw_accel_smoothed);
```

#### C. Fix Gyro Smoothing & Clamp
```cpp
// FFBEngine.h : calculate_force

// OLD
// double alpha_gyro = (std::min)(1.0f, m_gyro_smoothing);

// NEW
// Treat m_gyro_smoothing as "Smoothness" (0=Raw, 1=Slow) like SoP
double gyro_smoothness = (std::max)(0.0f, (std::min)(0.99f, m_gyro_smoothing)); // Clamp!
double tau_gyro = gyro_smoothness * 0.1; // Map to 0.0s - 0.1s time constant
double alpha_gyro = dt / (tau_gyro + dt);

m_steering_velocity_smoothed += alpha_gyro * (steer_vel - m_steering_velocity_smoothed);
```