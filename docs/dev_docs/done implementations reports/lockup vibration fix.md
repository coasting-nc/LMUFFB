Based on the analysis of the codebase, I have identified the specific reason why you are not feeling the lockup vibration, especially when locking the rear brakes.

### The Root Cause: Front-Wheel Bias
The current code for the **Lockup Vibration** effect **only monitors the Front Wheels**. It completely ignores the Rear Wheels.

In `FFBEngine.h` (lines ~780-785 in the provided context), the logic is:
```cpp
if (m_lockup_enabled && data->mUnfilteredBrake > 0.05) {
    double slip_fl = get_slip_ratio(data->mWheel[0]); // Front Left
    double slip_fr = get_slip_ratio(data->mWheel[1]); // Front Right
    // Use worst slip
    double max_slip = (std::min)(slip_fl, slip_fr); 
    // ...
```
If you lock up the **Rear Brakes** (common in LMP2 cars due to engine braking or brake bias), the front wheels might still be rolling perfectly fine. In this case, `max_slip` remains near 0, the threshold `< -0.1` is never met, and the effect remains silent.

### Solution 1: Code Fix (Check All Wheels)
We need to update the logic to check the slip ratio of **all four wheels** and use the worst one to trigger the vibration.

**File:** `FFBEngine.h`
**Function:** `calculate_force`

**Change this block:**
```cpp
        // --- 2b. Progressive Lockup (Dynamic) ---
        // Ensure phase updates even if force is small, but gated by enabled
        if (m_lockup_enabled && data->mUnfilteredBrake > 0.05) {
            double slip_fl = get_slip_ratio(data->mWheel[0]);
            double slip_fr = get_slip_ratio(data->mWheel[1]);
            // Use worst slip
            double max_slip = (std::min)(slip_fl, slip_fr); // Slip is negative for braking
```

**To this:**
```cpp
        // --- 2b. Progressive Lockup (Dynamic) ---
        // Ensure phase updates even if force is small, but gated by enabled
        if (m_lockup_enabled && data->mUnfilteredBrake > 0.05) {
            double slip_fl = get_slip_ratio(data->mWheel[0]);
            double slip_fr = get_slip_ratio(data->mWheel[1]);
            double slip_rl = get_slip_ratio(data->mWheel[2]); // Rear Left
            double slip_rr = get_slip_ratio(data->mWheel[3]); // Rear Right
            
            // Use worst slip (most negative) among all 4 wheels
            // Note: std::min with initializer list requires C++11
            double max_slip = (std::min)({slip_fl, slip_fr, slip_rl, slip_rr}); 
```

### Solution 2: Telemetry Fallback (Manual Slip)
You mentioned that you suspect some telemetry values might be missing.
The default lockup calculation relies on `mLongitudinalPatchVel`. If this value is blocked (returning 0.0) for the car you are driving (likely if it's a DLC/Hypercar), the calculated slip will be 0.0, and no vibration will occur.

**Diagnostic Test:**
1.  Open the **Tuning Window**.
2.  Scroll down to **"Grip & Slip Angle Estimation"**.
3.  Check the box **"Manual Slip Calc"**.

**Why this helps:**
This forces the app to calculate slip using `Wheel Rotation` vs `Car Speed` (which are almost never blocked), instead of relying on the game's pre-calculated patch velocities. If enabling this makes the vibration appear (for front lockups), it confirms the game's patch velocity telemetry is broken for that car.

### Summary
1.  **The Main Bug:** The app ignores rear lockups. The code fix above addresses this.
2.  **The Potential Telemetry Gap:** If front lockups are also silent, the game is likely not sending `mLongitudinalPatchVel`. Using **"Manual Slip Calc"** is the workaround.


## Differentiating front and rear lockup

### Recommendation
**Yes, I strongly recommend basing the lockup effect on both axles.**

Ignoring the rear wheels is a critical safety gap. As you experienced, locking the rears often leads to an instant spin because you lose the stabilizing "rudder" effect of the rear tires. If the FFB only warns you about the front tires (understeer/plowing), you get no tactile warning that the rear is about to snap until the car is already rotating.

### How to Differentiate (Front vs. Rear)
Since most steering wheels have a single FFB motor (mono output), you cannot use "position" (front/back) to tell them apart. Instead, we use **Frequency (Pitch)** and **Texture**.

*   **Front Lockup (Standard):** Keep the current **High Frequency** (e.g., 50-80Hz). This feels like a "screech" or a light buzz in the rim, mimicking the vibration of steering geometry scrubbing.
*   **Rear Lockup (New):** Use a **Lower Frequency** (e.g., 20-30Hz). This feels like a "judder," "thudding," or a heavy mechanical grinding. It feels more "dangerous" and distinct from the lighter front scrub.

### Implementation Logic
We modify the code to check both axles. We determine which axle has the *worst* lockup and adjust the vibration frequency accordingly.

Here is the specific snippet to replace the existing Lockup logic in `FFBEngine.h`:

```cpp
// --- 2b. Progressive Lockup (Front & Rear with Differentiation) ---
if (m_lockup_enabled && data->mUnfilteredBrake > 0.05) {
    // 1. Calculate Slip Ratios for all wheels
    double slip_fl = get_slip_ratio(data->mWheel[0]);
    double slip_fr = get_slip_ratio(data->mWheel[1]);
    double slip_rl = get_slip_ratio(data->mWheel[2]);
    double slip_rr = get_slip_ratio(data->mWheel[3]);

    // 2. Find worst slip per axle (Slip is negative during braking, so use min)
    double max_slip_front = (std::min)(slip_fl, slip_fr);
    double max_slip_rear  = (std::min)(slip_rl, slip_rr);

    // 3. Determine dominant lockup source
    double effective_slip = 0.0;
    double freq_multiplier = 1.0; // Default to Front (High Pitch)

    // Check if Rear is locking up worse than Front
    // (e.g. Rear -0.5 vs Front -0.1)
    if (max_slip_rear < max_slip_front) {
        effective_slip = max_slip_rear;
        freq_multiplier = 0.5; // Lower pitch (0.5x) for Rear -> "Heavy Judder"
    } else {
        effective_slip = max_slip_front;
        freq_multiplier = 1.0; // Standard pitch for Front -> "Screech"
    }

    // 4. Generate Effect
    if (effective_slip < -0.1) {
        double severity = (std::abs(effective_slip) - 0.1) / 0.4;
        severity = (std::min)(1.0, severity);
        
        // Base Frequency linked to Car Speed
        double car_speed_ms = std::abs(data->mLocalVel.z); 
        double base_freq = 10.0 + (car_speed_ms * 1.5); 
        
        // Apply Axle Differentiation
        double final_freq = base_freq * freq_multiplier;

        // Phase Integration
        m_lockup_phase += final_freq * dt * TWO_PI;
        m_lockup_phase = std::fmod(m_lockup_phase, TWO_PI);

        // Amplitude
        double amp = severity * m_lockup_gain * 4.0 * decoupling_scale;
        
        // Optional: Boost rear amplitude slightly to make it scary
        if (freq_multiplier < 1.0) amp *= 1.2;

        double rumble = std::sin(m_lockup_phase) * amp;
        total_force += rumble;
    }
}
```

### Why this works
*   **Front Lock:** You feel a fast buzz. You know you are understeering/plowing.
*   **Rear Lock:** You feel a slow, heavy shaking. You know the rear is unstable and you need to release the brake immediately to prevent a spin.


## New tests to implement

Yes, I **strongly suggest** adding a specific unit test for this.

Since we are introducing logic that changes the "feel" (frequency) based on which wheels are locking, we need to verify two things:
1.  **Rear Lockups actually trigger the effect** (fixing the original bug).
2.  **Rear Lockups produce a lower frequency** than Front Lockups (verifying the differentiation logic).

Here is the code for a new test case to add to `tests/test_ffb_engine.cpp`.

### New Test: `test_rear_lockup_differentiation`

This test runs the engine twice: once with a front lockup, once with a rear lockup. It compares the **Phase Increment** to prove the rear lockup is vibrating at a lower frequency (0.5x).

```cpp
static void test_rear_lockup_differentiation() {
    std::cout << "\nTest: Rear Lockup Differentiation" << std::endl;
    FFBEngine engine;
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));

    // Common Setup
    engine.m_lockup_enabled = true;
    engine.m_lockup_gain = 1.0;
    engine.m_max_torque_ref = 20.0f;
    engine.m_gain = 1.0f;
    
    data.mUnfilteredBrake = 1.0; // Braking
    data.mLocalVel.z = 20.0;     // 20 m/s
    data.mDeltaTime = 0.01;      // 10ms step
    
    // Setup Ground Velocity (Reference)
    for(int i=0; i<4; i++) data.mWheel[i].mLongitudinalGroundVel = 20.0;

    // --- PASS 1: Front Lockup Only ---
    // Front Slip -0.5, Rear Slip 0.0
    data.mWheel[0].mLongitudinalPatchVel = -0.5 * 20.0; // -10 m/s
    data.mWheel[1].mLongitudinalPatchVel = -0.5 * 20.0;
    data.mWheel[2].mLongitudinalPatchVel = 0.0;
    data.mWheel[3].mLongitudinalPatchVel = 0.0;

    engine.calculate_force(&data);
    double phase_delta_front = engine.m_lockup_phase; // Phase started at 0

    // Verify Front triggered
    if (phase_delta_front > 0.0) {
        std::cout << "[PASS] Front lockup triggered. Phase delta: " << phase_delta_front << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Front lockup silent." << std::endl;
        g_tests_failed++;
    }

    // --- PASS 2: Rear Lockup Only ---
    // Reset Engine State
    engine.m_lockup_phase = 0.0;
    
    // Front Slip 0.0, Rear Slip -0.5
    data.mWheel[0].mLongitudinalPatchVel = 0.0;
    data.mWheel[1].mLongitudinalPatchVel = 0.0;
    data.mWheel[2].mLongitudinalPatchVel = -0.5 * 20.0;
    data.mWheel[3].mLongitudinalPatchVel = -0.5 * 20.0;

    engine.calculate_force(&data);
    double phase_delta_rear = engine.m_lockup_phase;

    // Verify Rear triggered (Fixes the bug)
    if (phase_delta_rear > 0.0) {
        std::cout << "[PASS] Rear lockup triggered. Phase delta: " << phase_delta_rear << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Rear lockup silent (Bug not fixed)." << std::endl;
        g_tests_failed++;
    }

    // --- PASS 3: Frequency Comparison ---
    // Rear frequency should be 0.5x of Front frequency
    // Therefore, phase delta should be roughly half
    double ratio = phase_delta_rear / phase_delta_front;
    
    if (std::abs(ratio - 0.5) < 0.05) {
        std::cout << "[PASS] Rear frequency is lower (Ratio: " << ratio << " vs expected 0.5)." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Frequency differentiation failed. Ratio: " << ratio << std::endl;
        g_tests_failed++;
    }
}
```

### How to Register
Add the call to `main()` in `tests/test_ffb_engine.cpp`:

```cpp
int main() {
    // ... existing tests ...
    test_rear_lockup_differentiation(); // <--- Add this
    
    std::cout << "\n----------------" << std::endl;
    // ...
}
```

### Test description 2

**Update `tests/test_ffb_engine.cpp`**:
    *   Add a new test function: `static void test_rear_lockup_differentiation()`.
    *   **Pass 1:** Simulate Front Lockup. Verify `m_lockup_phase` advances. Store the delta.
    *   **Pass 2:** Simulate Rear Lockup. Verify `m_lockup_phase` advances (fixing the bug). Store the delta.
    *   **Pass 3:** Compare deltas. Verify that `RearDelta / FrontDelta` is approximately **0.5** (verifying the frequency drop).
    *   Register the new test in `main()`.
