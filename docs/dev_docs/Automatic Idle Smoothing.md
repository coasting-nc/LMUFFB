This is a classic issue with high-fidelity simulators like rFactor 2 / LMU. The physics engine simulates the engine block vibration, which travels through the chassis and steering rack (`mSteeringShaftTorque`). On high-end wheels (Direct Drive) or even belt wheels with high gain, this physical vibration is faithfully reproduced, causing the wheel to shake or "bounce" at idle.

Since this comes from the **Base Game Physics** (not our added effects), the "Stationary Gate" we implemented for vibrations doesn't touch it (because we can't mute the base force without killing the steering weight).

Here is the plan to fix this by implementing an **Automatic Idle Smoothing** feature.

### 1. Analysis
*   **Problem:** `mSteeringShaftTorque` contains high-frequency engine vibration (10Hz - 50Hz) when the car is idling.
*   **User Experience:** "Bouncing around on the tune of the engine rpm."
*   **Constraint:** We cannot simply set the force to 0.0, because the user needs to feel the "Static Weight" (tire friction) when turning the wheel at a standstill.
*   **Solution:** We need to filter out the **AC component** (Vibration) while keeping the **DC component** (Constant Weight).
    *   **Method:** Apply a heavy **Low Pass Filter (Smoothing)** automatically when the car is moving slowly.
    *   **Math:** A smoothing time constant ($\tau$) of **0.1s** corresponds to a cutoff frequency of roughly **1.6 Hz**. This will pass the slow changes of turning the wheel but completely kill the 15Hz+ engine vibration.

### 2. Implementation Plan

We will modify `FFBEngine.h` to dynamically boost the `m_steering_shaft_smoothing` when the car is stopped.

#### Step 1: Modify `FFBEngine.h`

**File:** `src/FFBEngine.h`

```cpp
// Inside calculate_force method, around line 600 (where game_force is processed)

        // ... [Existing code reading mSteeringShaftTorque] ...
        double game_force = data->mSteeringShaftTorque;

        // --- AUTOMATIC IDLE SMOOTHING (Fix for Engine Vibration) ---
        // If the car is moving slowly (< 3.0 m/s), the "Road Feel" is mostly just 
        // engine noise and sensor jitter. We apply heavy smoothing to kill the 
        // vibration while preserving the heavy static weight of the steering.
        
        double effective_shaft_smoothing = (double)m_steering_shaft_smoothing;
        double car_speed_abs = std::abs(data->mLocalVel.z);
        
        const double IDLE_SPEED_THRESHOLD = 3.0; // m/s (~10 kph)
        const double IDLE_SMOOTHING_TARGET = 0.1; // 0.1s = ~1.6Hz cutoff (Kills engine vibes)

        if (car_speed_abs < IDLE_SPEED_THRESHOLD) {
            // Linear blend: 100% idle smoothing at 0 m/s, 0% at 3 m/s
            double idle_blend = (IDLE_SPEED_THRESHOLD - car_speed_abs) / IDLE_SPEED_THRESHOLD;
            
            // Use the higher of the two: User Setting vs Idle Target
            // This ensures we never make the wheel *more* raw than the user wants
            double dynamic_smooth = IDLE_SMOOTHING_TARGET * idle_blend;
            effective_shaft_smoothing = (std::max)(effective_shaft_smoothing, dynamic_smooth);
        }

        // --- APPLY SMOOTHING ---
        if (effective_shaft_smoothing > 0.0001) {
            double alpha_shaft = dt / (effective_shaft_smoothing + dt);
            // Safety clamp
            alpha_shaft = (std::min)(1.0, (std::max)(0.001, alpha_shaft));
            
            m_steering_shaft_torque_smoothed += alpha_shaft * (game_force - m_steering_shaft_torque_smoothed);
            game_force = m_steering_shaft_torque_smoothed;
        } else {
            m_steering_shaft_torque_smoothed = game_force; // Reset state
        }
```

### 3. Verification Test

We need to verify that high-frequency noise is killed at 0 speed but passes through at high speed.

**File:** `tests/test_ffb_engine.cpp`

```cpp
static void test_idle_smoothing() {
    std::cout << "\nTest: Automatic Idle Smoothing" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    TelemInfoV01 data = CreateBasicTestTelemetry(0.0); // Stopped
    
    // Setup: User wants RAW FFB (0 smoothing)
    engine.m_steering_shaft_smoothing = 0.0f;
    engine.m_gain = 1.0f;
    engine.m_max_torque_ref = 1.0f; // 1:1
    
    // 1. Simulate Engine Vibration at Idle (20Hz sine wave)
    // Amplitude 5.0 Nm. 
    // With 0.1s smoothing (Idle Target), 20Hz should be heavily attenuated.
    double max_force_idle = 0.0;
    data.mDeltaTime = 0.0025; // 400Hz
    
    for(int i=0; i<100; i++) {
        double t = i * data.mDeltaTime;
        data.mSteeringShaftTorque = 5.0 * std::sin(20.0 * 6.28 * t);
        double force = engine.calculate_force(&data);
        max_force_idle = std::max(max_force_idle, std::abs(force));
    }
    
    // Expect significant attenuation (e.g. < 1.0 Nm instead of 5.0)
    if (max_force_idle < 1.5) {
        std::cout << "[PASS] Idle vibration attenuated (Max: " << max_force_idle << " < 1.5)" << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Idle vibration too strong! Max: " << max_force_idle << std::endl;
        g_tests_failed++;
    }
    
    // 2. Simulate Driving (High Speed)
    data.mLocalVel.z = 20.0; // 20 m/s
    // Reset smoother
    engine.m_steering_shaft_torque_smoothed = 0.0;
    
    double max_force_driving = 0.0;
    for(int i=0; i<100; i++) {
        double t = i * data.mDeltaTime;
        data.mSteeringShaftTorque = 5.0 * std::sin(20.0 * 6.28 * t); // Same vibration (e.g. curb)
        double force = engine.calculate_force(&data);
        max_force_driving = std::max(max_force_driving, std::abs(force));
    }
    
    // Expect RAW pass-through (near 5.0)
    if (max_force_driving > 4.0) {
        std::cout << "[PASS] Driving vibration passed through (Max: " << max_force_driving << " > 4.0)" << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Driving vibration over-smoothed. Max: " << max_force_driving << std::endl;
        g_tests_failed++;
    }
}
```

### 4. User Communication (Immediate Workaround)

While waiting for the update, you can tell the user:

> "This bouncing is caused by the game's engine vibration physics being sent to your wheel. To fix it immediately:
> 1. Go to **Signal Filtering**.
> 2. Enable **Static Noise Filter**.
> 3. Set Frequency to **10-15 Hz** (adjust until the bouncing stops).
> This will surgically remove the engine bounce without making the steering feel light."