Here is the comprehensive plan to investigate and fix the reported issues.

### 1. Analysis of Reported Issues

#### Issue A: Violent Shaking when Stopping (User: Oliver Johann, dISh)
*   **Symptoms:** The wheel shakes violently when the car is stationary (pits, track stop). User dISh notes it "shakes along with engine rpm".
*   **Root Cause:** The FFB engine currently processes all vibration effects regardless of car speed. Even when stationary, the physics engine generates micro-movements (engine idle vibration affecting suspension, sensor noise).
    *   **Road Texture:** The engine idle vibration causes tiny fluctuations in `mVerticalTireDeflection`. The high-pass filter amplifies these into "Road Noise".
    *   **Yaw Kick:** Sensor noise in `mLocalRotAccel.y` can trigger kicks (though v0.6.10 added a threshold, idle vibration might exceed it).
*   **Solution:** Implement a **"Stationary Signal Gate"**. We must fade out all AC (vibration/texture) effects when the car speed drops below a threshold (e.g., 2.0 m/s), while keeping DC effects (Steering Weight) active so the wheel doesn't go completely limp.

#### Issue B: Missing Road Texture (User: Oliver Johann)
*   **Symptoms:** No road texture felt on bumpy tracks (Sebring) even with max gain.
*   **Root Cause:** The user is likely driving a DLC/Encrypted car (e.g., 911 GT3 R). On these cars, LMU blocks specific telemetry channels to protect IP.
    *   **Blocked Data:** `mVerticalTireDeflection` often returns `0.0` on encrypted cars.
    *   **Result:** The Road Texture logic (`delta = current - prev`) calculates `0 - 0 = 0`, resulting in silence.
*   **Solution:** Implement the **"Gap A" Fallback** identified in `docs/dev_docs/encrypted_content_gaps.md`. If deflection data is static while moving, switch to using **Vertical G-Force** (`mLocalAccel.y`) to generate road noise.

#### Issue C: Missing Data Warnings (User: dISh)
*   **Symptoms:** Console warnings about missing `mTireLoad`. User suspects custom livery naming.
*   **Analysis:** The warnings are functioning correctly. The "911GT3R" is an encrypted car, so `mTireLoad` is indeed blocked. The app is correctly detecting this and using the Kinematic Fallback. The livery name is unrelated; it's the car physics model.
*   **Action:** No code fix needed for the logic, but we will update the warning message to be more informative (mentioning "Encrypted Content") to reduce user anxiety.

---

### 2. Implementation Plan

#### Step 1: Implement Stationary Signal Gate
We will modify `FFBEngine.h` to calculate a `speed_gate` scalar and apply it to all vibration effects.

**File:** `src/FFBEngine.h`

```cpp
// In FFBEngine class, add to member variables
double m_prev_vert_accel = 0.0; // For Road Texture Fallback

// In calculate_force method:

// ... [After calculating car_speed] ...

// 1. Calculate Stationary Gate (Fade out vibrations at low speed)
// Ramp from 0.0 (at < 0.5 m/s) to 1.0 (at > 2.0 m/s)
double speed_gate = (car_speed - 0.5) / 1.5;
speed_gate = std::max(0.0, std::min(1.0, speed_gate));

// ... [Inside Effect Calculations] ...

// Apply speed_gate to effects that shouldn't exist at standstill:

// A. Yaw Kick
// Existing logic has a hard cut at 5.0 m/s. We can keep that or replace with smooth gate.
// Current: if (car_v_long < 5.0) raw_yaw_accel = 0.0; -> This is already safe.

// B. ABS Pulse
if (abs_system_active) {
    // ...
    // Apply gate
    total_force += (float)(std::sin(m_abs_phase) * m_abs_gain * 2.0 * decoupling_scale * speed_gate);
}

// C. Lockup
// Already has speed-dependent frequency, but amplitude should also be gated
// ...
lockup_rumble *= speed_gate; // Apply at end of calculation

// D. Slide Texture
// Already checks effective_slip_vel > 0.5, so it's safe at standstill.

// E. Road Texture (CRITICAL FIX for Idle Shake)
// ...
road_noise *= speed_gate; 

// F. Bottoming
// ...
double crunch = std::sin(m_bottoming_phase) * bump_magnitude * speed_gate;
```

#### Step 2: Implement Road Texture Fallback (Vertical G)
We will modify the Road Texture block to use Vertical Acceleration if Deflection is dead.

**File:** `src/FFBEngine.h`

```cpp
// --- 4. Road Texture (High Pass Filter) ---
if (m_road_texture_enabled) {
    // ... [Scrub Drag Logic] ...

    double vert_l = fl.mVerticalTireDeflection;
    double vert_r = fr.mVerticalTireDeflection;
    
    // Delta from previous frame
    double delta_l = vert_l - m_prev_vert_deflection[0];
    double delta_r = vert_r - m_prev_vert_deflection[1];
    
    // v0.4.6: Delta Clamping
    delta_l = (std::max)(-0.01, (std::min)(0.01, delta_l));
    delta_r = (std::max)(-0.01, (std::min)(0.01, delta_r));

    double road_noise_val = 0.0;

    // FALLBACK LOGIC: Check if Deflection is active
    // If deltas are exactly 0.0 but we are moving fast, data is likely blocked.
    bool deflection_active = (std::abs(delta_l) > 0.000001 || std::abs(delta_r) > 0.000001);
    
    if (deflection_active || car_speed < 5.0) {
        // Standard Logic
        road_noise_val = (delta_l + delta_r) * 50.0;
    } else {
        // Fallback: Use Vertical Acceleration (Heave)
        // This captures bumps even if suspension telemetry is encrypted
        double vert_accel = data->mLocalAccel.y;
        double delta_accel = vert_accel - m_prev_vert_accel;
        
        // Scaling: Accel delta needs to be converted to equivalent force
        // Empirically, 1.0 m/s^2 delta ~ equivalent to small bump
        // Multiplier 0.05 gives similar magnitude to deflection method
        road_noise_val = delta_accel * 0.05 * 50.0; 
    }
    
    // Update History
    m_prev_vert_accel = data->mLocalAccel.y;

    // Apply Gain & Load
    road_noise = road_noise_val * m_road_texture_gain * decoupling_scale * texture_load_factor;
    
    // Apply Stationary Gate (Fix for Issue 1)
    road_noise *= speed_gate;

    total_force += road_noise;
}
```

#### Step 3: Update Warning Messages
Make the console warnings clearer about *why* data is missing.

**File:** `src/FFBEngine.h`

```cpp
if (!m_warned_load) {
    std::cout << "[WARNING] mTireLoad is missing for car: " << data->mVehicleName 
              << ". (Likely Encrypted/DLC Content). Using Kinematic Fallback." << std::endl;
    m_warned_load = true;
}
```

---

### 3. Verification Tests

We need to add a test case to verify the Stationary Gate works (i.e., noise is suppressed at 0 speed).

**File:** `tests/test_ffb_engine.cpp`

```cpp
static void test_stationary_gate() {
    std::cout << "\nTest: Stationary Signal Gate" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(0.0); // 0 speed
    
    // Enable Road Texture
    engine.m_road_texture_enabled = true;
    engine.m_road_texture_gain = 1.0;
    
    // Simulate Engine Idle Vibration (Deflection Delta)
    data.mWheel[0].mVerticalTireDeflection = 0.001; 
    data.mWheel[1].mVerticalTireDeflection = 0.001;
    // Previous was 0.0, so delta is 0.001
    
    // Calculate
    double force = engine.calculate_force(&data);
    
    // Should be 0.0 due to speed_gate
    if (std::abs(force) < 0.0001) {
        std::cout << "[PASS] Idle vibration suppressed at 0 speed." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Idle vibration leaked through! Force: " << force << std::endl;
        g_tests_failed++;
    }
}
```

### 4. Summary of Changes

1.  **`src/FFBEngine.h`**:
    *   Added `m_prev_vert_accel` member.
    *   Added `speed_gate` calculation based on `car_speed`.
    *   Applied `speed_gate` to `road_noise`, `lockup_rumble`, `bottoming_crunch`, `abs_force`.
    *   Implemented Fallback Logic in Road Texture block (switch to `mLocalAccel.y` if deflection static).
    *   Updated warning text.
2.  **`tests/test_ffb_engine.cpp`**:
    *   Added `test_stationary_gate`.
3.  **`CHANGELOG_DEV.md`**:
    *   Document fixes for "Violent Shaking at Stop" and "Missing Road Texture on DLC cars".