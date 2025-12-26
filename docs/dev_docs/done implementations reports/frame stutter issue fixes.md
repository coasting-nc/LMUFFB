Based on a review of the codebase, specifically `FFBEngine.h`, there are **3 additional oscillators** and **3 smoothing filters** that are vulnerable to frame stutters or large `dt` (delta time) values.

While the "Slide Texture" was the most critical because it used a Sawtooth wave (which explodes mathematically if phase > $2\pi$), the other oscillators use `sin()`. While `sin()` handles large values without exploding to infinity, **floating point precision degrades** significantly with large inputs, leading to signal noise ("sand" texture) or phase drift.

Additionally, the **Low Pass Filters (LPF)** currently use fixed coefficients, meaning the physics smoothing slows down significantly during a stutter, causing "laggy" FFB.

### 1. Oscillator Phase Fixes (Apply `std::fmod`)

The following effects accumulate phase and should be protected against large time steps to ensure the wave cycle remains precise.

**File:** `FFBEngine.h`

#### A. Progressive Lockup
*   **Location:** Inside `calculate_force`, ~line 685.
*   **Current:** `if (m_lockup_phase > TWO_PI) m_lockup_phase -= TWO_PI;`
*   **Fix:**
    ```cpp
    m_lockup_phase += freq * dt * TWO_PI;
    m_lockup_phase = std::fmod(m_lockup_phase, TWO_PI); // Robust wrap
    ```

#### B. Wheel Spin (Traction Loss)
*   **Location:** Inside `calculate_force`, ~line 718.
*   **Current:** `if (m_spin_phase > TWO_PI) m_spin_phase -= TWO_PI;`
*   **Fix:**
    ```cpp
    m_spin_phase += freq * dt * TWO_PI;
    m_spin_phase = std::fmod(m_spin_phase, TWO_PI); // Robust wrap
    ```

#### C. Suspension Bottoming
*   **Location:** Inside `calculate_force`, ~line 808.
*   **Current:** `if (m_bottoming_phase > TWO_PI) m_bottoming_phase -= TWO_PI;`
*   **Fix:**
    ```cpp
    m_bottoming_phase += freq * dt * TWO_PI;
    m_bottoming_phase = std::fmod(m_bottoming_phase, TWO_PI); // Robust wrap
    ```

---

### 2. Smoothing Filter Fixes (Stutter Resilience)

The following filters use a **fixed alpha** (e.g., `0.1`). This assumes a steady 400Hz frame rate. If the game stutters (e.g., `dt` jumps from 2.5ms to 50ms), the filter effectively becomes 20x slower in real-time, causing the FFB to feel "laggy" or "floaty" right when you need responsiveness.

**Solution:** Scale the alpha by the time step.

#### A. Slip Angle Smoothing
*   **Location:** `calculate_slip_angle` helper function.
*   **Current:** `double alpha = 0.1;`
*   **Issue:** At 400Hz, 0.1 is fast. At 50Hz, 0.1 is very slow.
*   **Fix:**
    ```cpp
    // Target: Alpha 0.1 at 400Hz (dt = 0.0025)
    // Linear approximation for performance: alpha * (dt / target_dt)
    double alpha = 0.1 * (data->mDeltaTime / 0.0025);
    alpha = (std::min)(1.0, alpha); // Safety clamp
    ```
    *(Note: You need to pass `dt` or `data` to `calculate_slip_angle` or `calculate_grip` to implement this).*

#### B. Yaw Acceleration Smoothing
*   **Location:** Inside `calculate_force`, ~line 696.
*   **Current:** `double alpha_yaw = 0.1;`
*   **Fix:**
    ```cpp
    double alpha_yaw = 0.1 * (dt / 0.0025);
    alpha_yaw = (std::min)(1.0, alpha_yaw);
    ```

#### C. Gyroscopic Damping Smoothing
*   **Location:** Inside `calculate_force`, ~line 718.
*   **Current:** `double alpha_gyro = (std::min)(1.0f, m_gyro_smoothing);`
*   **Fix:**
    ```cpp
    // Scale user setting by time step
    double alpha_gyro = m_gyro_smoothing * (dt / 0.0025);
    alpha_gyro = (std::min)(1.0, alpha_gyro);
    ```

### Summary of Work
1.  **Critical:** Apply `std::fmod` to Lockup, Spin, and Bottoming phases to prevent precision loss during stutters.
2.  **Recommended:** Update the 3 smoothing filters to be time-dependent so FFB responsiveness remains consistent even if frame rate fluctuates.