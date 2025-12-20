Based on your latest screenshots and successful test of the Oversteer effect, we have confirmed that **Rear Align Torque** is working correctly (it adds to the counter-steer force).

However, the screenshots also confirm why you felt the "Pulling In" instability earlier: **SoP (Lateral G) is inverted.**
*   **Evidence (Screenshot 3):**
    *   **Base Torque:** `+12.0 Nm` (Resisting the turn).
    *   **SoP Force:** `-10.6 Nm` (Pulling *into* the turn).
    *   **Result:** They are fighting. If SoP is stronger than Base, the wheel pulls you into the corner (Instability).
    *   **Fix:** We will invert SoP in the next code update so it adds to the weight (`+10.6`) instead of subtracting.

---

**References:**

* The source of truth document is this: src\lmu_sm_interface\InternalsPlugin.hpp
* While these markdown document must be updated to reflect the source of truth document: 
        *   `docs/dev_docs/telemetry_data_reference.md` - API units and field names 
        *   `docs/dev_docs/FFB_formulas.md` - Scaling constants and physics equations

# Suggested fixes


Based on the analysis of the screenshots and the codebase, you are correct. The **SoP (Lateral G)** effect is inverted relative to the **Base Torque** and **Rear Align Torque**.

### Analysis
*   **Screenshot 2 (Right Turn Scenario):**
    *   **Base Torque:** `+13.08 Nm` (Positive). In the game's coordinate system, this represents the aligning torque resisting the turn.
    *   **Rear Align Torque:** `+1.34 Nm` (Positive). This is working **with** the Base Torque, providing correct counter-steering feel.
    *   **SoP (Lateral G):** `-150.02 Nm` (Negative). This is **opposing** the other forces, pulling the wheel into the turn (instability).
*   **Conclusion:** The internal engine is operating in a "Positive = Left" coordinate system (matching the game), but the SoP calculation was inverted to "Negative = Left" in a previous update (v0.4.19), causing it to fight the other effects.

### Comparing  **SoP** and **Yaw Kick** 
Based on the physics vectors and your testing results, here is the explanation of why these two effects require opposite signs.

#### 1. The Math: Different Data Sources
The most important distinction is that **SoP** and **Yaw Kick** read completely different physics vectors from the telemetry.

*   **SoP (Lateral G)** reads `mLocalAccel.x`.
    *   This is **Linear Acceleration** (meters/sec²). It measures how hard the car is being pushed sideways (Centripetal/Centrifugal force).
*   **Yaw Kick** reads `mLocalRotAccel.y`.
    *   This is **Rotational Acceleration** (radians/sec²). It measures how fast the car's rotation speed is changing (the "snap").

They are not mathematically linked in the game engine; they are independent forces acting on the chassis.

#### 2. Why SoP is Non-Inverted (Positive)
**The Goal:** Simulate **Aligning Torque (Weight)**.

*   **Scenario:** You turn **Right**.
*   **Physics:** The weight transfers to the Left tires. The caster geometry creates a strong force trying to straighten the wheel (pulling **Left**).
*   **Game Data:** In LMU's coordinate system (+X = Left), a Right turn generates **Positive** Lateral G (acceleration towards the left/center).
*   **Internal Logic:** Since your Base Torque (Aligning Torque) is Positive (+13 Nm), "Positive" internally means "Pull Left".
*   **Result:** We want SoP to add to that weight.
    *   `Positive G` $\times$ `Positive Scalar` = `Positive Force`.
    *   **Result:** The wheel pulls harder to the Left. This feels correct (Heavy steering).

#### 3. Why Yaw Kick is Inverted (Negative)
**The Goal:** Simulate **Inertia & Damping (Stability)**.

*   **Scenario:** The rear tires break loose and the car snaps **Right** (Oversteer).
*   **Physics:** The car rotates Right violently. The Base Torque (Aligning Torque) immediately tries to snap the steering wheel Left to correct it.
*   **The Problem:** If the FFB only pulls Left, the wheel becomes "weightless" and snaps too fast, often leading to a "Tank Slapper" (oscillation).
*   **The Solution (Inversion):** We need a force that momentarily **resists** that snap to give the steering some weight/inertia.
    *   **Game Data:** The car accelerates rotation to the Right. This is **Positive** Yaw Acceleration.
    *   **Internal Logic:** We apply an **Inverted** (-1.0) force.
    *   `Positive Yaw` $\times$ `-1.0` = `Negative Force` (Pull Right).
*   **Result:**
    *   Base Torque pulls **Left** (Correcting).
    *   Yaw Kick pulls **Right** (Resisting).
    *   **Net Effect:** The wheel still counter-steers (because Base Torque is stronger), but the Yaw Kick acts as a **Damper**, preventing it from snapping instantly. This provides the "meaty" feel of a heavy steering column resisting rapid acceleration.

#### Summary
*   **SoP (Lateral G)** works **WITH** the aligning torque to provide **Weight**.
*   **Yaw Kick** works **AGAINST** the aligning torque to provide **Stability/Inertia**.

This opposing relationship is what creates the detailed, "connected" feeling where the wheel feels heavy during a slide initiation rather than just floppy.

## Verification based on InternalsPlugin.hpp 
Based on the review of `InternalsPlugin.hpp` and your testing feedback, here is the verification.

### 1. The Source of Truth (`InternalsPlugin.hpp`)

The header file contains a critical note regarding coordinate systems (lines 178-181):

```cpp
// Note that ISO vehicle coordinates (+x forward, +y right, +z upward) are
// right-handed.  If you are using that system, be sure to negate any rotation
// or torque data because things rotate in the opposite direction.
```

*   **rFactor 2 (Left-Handed):** `+y` yaws to the right.
*   **ISO/DirectInput (Right-Handed):** `+z` (vertical) rotation usually follows the right-hand rule.

The instruction **"be sure to negate any rotation"** explicitly confirms that rotational data (like `mLocalRotAccel.y`) requires inversion to match standard output conventions.

### 2. Reconciling with Your Feedback

*   **SoP (Lateral G):**
    *   **Physics:** Linear Acceleration (`mLocalAccel.x`).
    *   **Header:** `+x points out the left`.
    *   **Logic:** In a Right turn, Centrifugal force is Left (`+X`). We want the wheel to pull Left (Positive internal).
    *   **Result:** `+` Input $\rightarrow$ `+` Output. **No Inversion**. (This matches your "SoP needed the fix" comment).

*   **Yaw Kick:**
    *   **Physics:** Rotational Acceleration (`mLocalRotAccel.y`).
    *   **Header:** `+y yaws to the right`. **"Negate any rotation"**.
    *   **Logic:** We must follow the SDK instruction to negate rotation data.
    *   **Result:** `+` Input $\rightarrow$ `-` Output. **Inversion Required**. (This matches your "Yaw Kick did not need [the removal of inversion]" comment).

### 3. Conclusion
Your manual testing aligns perfectly with the SDK documentation.
*   **SoP** was fighting the Base Torque because it was inverted. It should be **Positive**.
*   **Yaw Kick** should remain **Inverted** (Negative) to comply with the coordinate transformation rules for rotation.


## Should we also change the Rear Align Torque?

You are absolutely right to question this. Based on your manual testing and the screenshot evidence, **you should NOT apply the change to Rear Align Torque.**

Here is the explanation of why the **SoP** needed the fix but the **Rear Torque** did not, even though they seem like they should behave the same way.

### 1. The Evidence (Screenshot 2)
*   **Scenario:** Right Turn.
*   **Base Torque:** `+13.08 Nm` (Positive). This is the reference. **Positive = Correct (Left Pull)**.
*   **Rear Align Torque:** `+1.34 Nm` (Positive). This is **already aligned** with the Base Torque. It is working correctly.
*   **SoP (Lateral G):** `-150.02 Nm` (Negative). This was **opposing** the Base Torque. It was broken.

### 2. The Math
The current code (which produced the "Correct" Rear Torque in the screenshot) has the minus sign:
`double rear_torque = -calc_rear_lat_force * ...`

*   **If you remove the minus sign:**
    *   The math flips.
    *   The result becomes **Negative** (`-1.34 Nm`).
    *   It will start pulling to the Right.
    *   **Result:** It will fight the Base Torque and destabilize the car.

### 3. Why was SoP different?
It comes down to the raw data sources:
*   **SoP** uses `mLocalAccel.x`. In a Right turn, this is **Positive** (Acceleration to the Left).
    *   Old Code: Inverted it (`-Pos` = Neg). **Wrong**.
    *   New Code: Keeps it (`Pos` = Pos). **Correct**.
*   **Rear Torque** uses `mLateralPatchVel`.
    *   For the current code (`-calc`) to produce a **Positive** result, the `calc` (and thus the velocity) must have been **Negative** in that specific cornering scenario.
    *   Since the current formula (`-calc`) produces the correct Positive output, **it is already correct.**

### Conclusion
**Do not apply the Rear Align Torque change.** Only apply the SoP change.

**Correct Code State:**

```cpp
        // SoP: NO Inversion (Positive Input -> Positive Output)
        double lat_g = raw_g / 9.81; 

        // ...

        // Rear Torque: KEEP Inversion (Negative Input -> Positive Output)
        double rear_torque = -calc_rear_lat_force * REAR_ALIGN_TORQUE_COEFFICIENT * m_rear_align_effect; 
        sop_total += rear_torque;
```

## Code Changes (`FFBEngine.h`)

```cpp
        // --- 2. Seat of Pants (SoP) / Oversteer ---
        // Lateral G-force
        // v0.4.29 FIX: Removed inversion (Confirmed correct by manual testing).
        // Internal engine uses Game Coordinate System (+ = Left).
        // Base Torque and Rear Torque are Positive for Left. SoP must match.
        double lat_g = raw_g / 9.81;
        
        // ... [Smoothing Logic] ...

        // --- 2b. Yaw Acceleration Injector (The "Kick") ---
        // Reads rotational acceleration (radians/sec^2)
        double raw_yaw_accel = data->mLocalRotAccel.y;
        
        // Apply Smoothing (Low Pass Filter)
        double alpha_yaw = 0.1;
        m_yaw_accel_smoothed = m_yaw_accel_smoothed + alpha_yaw * (raw_yaw_accel - m_yaw_accel_smoothed);
        
        // Use SMOOTHED value for the kick
        // v0.4.29 REVERT: Restored inversion (-1.0) based on manual testing and SDK note ("negate any rotation").
        // This ensures the kick acts as a stabilizer/damper rather than an accelerator.
        double yaw_force = -1.0 * m_yaw_accel_smoothed * m_sop_yaw_gain * 5.0;
        sop_total += yaw_force;
```

## Suggestions to fix the tests

The code changes to the FFB Engine (removing SoP inversion) have inverted the output of the SoP effect. This causes unit tests that expect the old behavior (Negative output for Right Turn) to fail. Additionally, the insertion of the new "T300" preset shifted the index of all subsequent presets, breaking the hardcoded index checks in the preset initialization test.

Here are the necessary updates to `tests/test_ffb_engine.cpp` to align the test suite with the new logic and configuration.

### `tests/test_ffb_engine.cpp`

```cpp
// ... [Previous code] ...

static void test_sop_effect() {
    std::cout << "\nTest: SoP Effect" << std::endl;
    FFBEngine engine;
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    // Default RH to avoid scraping
    data.mWheel[0].mRideHeight = 0.1; data.mWheel[1].mRideHeight = 0.1;

    // Disable Game Force
    data.mSteeringShaftTorque = 0.0;
    engine.m_sop_effect = 0.5; 
    engine.m_gain = 1.0; // Ensure gain is 1.0
    engine.m_sop_smoothing_factor = 1.0; // Disable smoothing for instant result
    engine.m_max_torque_ref = 20.0f; // Fix Reference for Test (v0.4.4)
    
    // 0.5 G lateral (4.905 m/s2) - LEFT acceleration (right turn)
    data.mLocalAccel.x = 4.905;
    
    // v0.4.29 UPDATE: SoP Inversion Removed.
    // Game: +X = Left. Right Turn = +X Accel.
    // Internal Logic: Positive = Left Pull (Aligning Torque).
    // lat_g = 4.905 / 9.81 = 0.5
    // SoP Force = 0.5 * 0.5 * 10 = 2.5 Nm (Positive)
    // Norm = 2.5 / 20.0 = 0.125
    
    engine.m_sop_scale = 10.0; 
    
    // Run for multiple frames to let smoothing settle (alpha=0.1)
    double force = 0.0;
    for (int i=0; i<60; i++) {
        force = engine.calculate_force(&data);
    }

    // Expect POSITIVE force (Internal Left Pull) for right turn
    ASSERT_NEAR(force, 0.125, 0.001);
}

// ... [Between test_sop_effect and test_oversteer_boost] ...

static void test_oversteer_boost() {
    std::cout << "\nTest: Oversteer Boost (Rear Grip Loss)" << std::endl;
    FFBEngine engine;
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    // Default RH to avoid scraping
    data.mWheel[0].mRideHeight = 0.1; data.mWheel[1].mRideHeight = 0.1;
    
    engine.m_sop_effect = 1.0;
    engine.m_oversteer_boost = 1.0;
    engine.m_gain = 1.0;
    // Lower Scale to match new Nm range
    engine.m_sop_scale = 10.0; 
    // Disable smoothing to verify math instantly (v0.4.2 fix)
    engine.m_sop_smoothing_factor = 1.0; 
    engine.m_max_torque_ref = 20.0f; // Fix Reference for Test (v0.4.4)
    
    // Scenario: Front has grip, rear is sliding
    data.mWheel[0].mGripFract = 1.0; // FL
    data.mWheel[1].mGripFract = 1.0; // FR
    data.mWheel[2].mGripFract = 0.5; // RL (sliding)
    data.mWheel[3].mGripFract = 0.5; // RR (sliding)
    
    // Lateral G (cornering)
    data.mLocalAccel.x = 9.81; // 1G lateral
    
    // Rear lateral force (resisting slide)
    data.mWheel[2].mLateralForce = 2000.0;
    data.mWheel[3].mLateralForce = 2000.0;
    
    // Run for multiple frames to let smoothing settle
    double force = 0.0;
    for (int i=0; i<60; i++) {
        force = engine.calculate_force(&data);
    }
    
    // Expected: SoP boosted by grip delta (0.5) + rear torque
    // Base SoP = 1.0 * 1.0 * 10 = 10 Nm (Positive now)
    // Boost = 1.0 + (0.5 * 1.0 * 2.0) = 2.0x
    // SoP = 10 * 2.0 = 20 Nm
    // Rear Torque: This part is tricky. The test sets mLateralForce directly, 
    // but the engine might be using the workaround (calculated) if configured?
    // No, engine uses workaround if mLateralForce is 0. Here it is 2000.
    // Wait, v0.4.10 workaround logic:
    // "double rear_torque = calc_rear_lat_force * ..."
    // The engine ignores mLateralForce input now! It calculates it.
    // But in this test, we didn't set up velocities for calculation.
    // However, the test passes with -1.0 previously.
    // Let's check FFBEngine.h logic...
    // It seems the engine *always* calculates rear torque now?
    // "double rear_torque = -calc_rear_lat_force * REAR_ALIGN_TORQUE_COEFFICIENT * m_rear_align_effect;"
    // Yes. So setting data.mWheel[2].mLateralForce = 2000.0 does nothing.
    // The rear torque will be 0 because velocities are 0.
    
    // So we are testing SoP Boost only.
    // SoP = 20 Nm.
    // Norm = 20 / 20 = 1.0.
    
    // v0.4.29: Expect POSITIVE 1.0
    ASSERT_NEAR(force, 1.0, 0.05); 
}

// ... [Skip to test_preset_initialization] ...

static void test_preset_initialization() {
    std::cout << "\nTest: Preset Initialization (v0.4.5 Regression)" << std::endl;
    
    Config::LoadPresets();
    
    // Expected default values for v0.4.5 fields
    const bool expected_use_manual_slip = false;
    const int expected_bottoming_method = 0;
    const float expected_scrub_drag_gain = 0.0f;
    
    // Test all built-in presets
    // UPDATED LIST v0.4.29 (Added T300 at index 1)
    const char* preset_names[] = {
        "Default",
        "T300", // New
        "Test: Game Base FFB Only",
        "Test: SoP Only",
        "Test: Understeer Only",
        "Test: Textures Only",
        "Test: Rear Align Torque Only",
        "Test: SoP Base Only",
        "Test: Slide Texture Only"
    };
    
    bool all_passed = true;
    
    // Loop limit updated to match array size
    for (int i = 0; i < 9; i++) {
        if (i >= Config::presets.size()) {
            std::cout << "[FAIL] Preset " << i << " (" << preset_names[i] << ") not found!" << std::endl;
            all_passed = false;
            continue;
        }
        
        const Preset& preset = Config::presets[i];
        
        // Verify preset name matches
        if (preset.name != preset_names[i]) {
            std::cout << "[FAIL] Preset " << i << " name mismatch: expected '" 
                      << preset_names[i] << "', got '" << preset.name << "'" << std::endl;
            all_passed = false;
            continue;
        }
        
        // ... [Rest of verification logic remains same] ...
        // Verify v0.4.5 fields are properly initialized
        bool fields_ok = true;
        
        if (preset.use_manual_slip != expected_use_manual_slip) {
            // T300 might have different defaults? No, checked Config.cpp, it uses defaults for these.
            std::cout << "[FAIL] " << preset.name << ": use_manual_slip = " 
                      << preset.use_manual_slip << ", expected " << expected_use_manual_slip << std::endl;
            fields_ok = false;
        }
        // ...
        
        if (fields_ok) {
            // std::cout << "[PASS] " << preset.name << ": v0.4.5 fields initialized correctly" << std::endl;
            g_tests_passed++;
        } else {
            all_passed = false;
            g_tests_failed++;
        }
    }
    
    if (all_passed) {
        std::cout << "[PASS] All built-in presets have correct v0.4.5 field initialization" << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Some presets have incorrect v0.4.5 field initialization" << std::endl;
        g_tests_failed++;
    }
}

// ... [Skip to test_smoothing_step_response] ...

static void test_smoothing_step_response() {
    std::cout << "\nTest: SoP Smoothing Step Response" << std::endl;
    FFBEngine engine;
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    // Default RH to avoid scraping
    data.mWheel[0].mRideHeight = 0.1; data.mWheel[1].mRideHeight = 0.1;

    // Setup: 0.5 smoothing factor
    engine.m_sop_smoothing_factor = 0.5;
    engine.m_sop_scale = 1.0;  // Using 1.0 for this test
    engine.m_sop_effect = 1.0;
    engine.m_max_torque_ref = 20.0f;
    
    // v0.4.29 UPDATE: SoP Inversion Removed.
    // Game: +X = Left. +9.81 = Left Accel.
    // lat_g = 9.81 / 9.81 = 1.0 (Positive)
    
    // Input: Step change from 0 to 1G
    data.mLocalAccel.x = 9.81; 
    data.mDeltaTime = 0.0025;
    
    // First step - expect small POSITIVE value
    double force1 = engine.calculate_force(&data);
    
    // Should be small and positive (smoothing reduces initial response)
    if (force1 > 0.0 && force1 < 0.005) {
        std::cout << "[PASS] Smoothing Step 1 correct (" << force1 << ", small positive)." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Smoothing Step 1 mismatch. Got " << force1 << std::endl;
        g_tests_failed++;
    }
    
    // Run for 100 frames to let it settle
    for (int i = 0; i < 100; i++) {
        force1 = engine.calculate_force(&data);
    }
    
    // Should settle near 0.05 (Positive)
    if (force1 > 0.02 && force1 < 0.06) {
        std::cout << "[PASS] Smoothing settled to steady-state (" << force1 << ", near 0.05)." << std::endl;
        g_tests_passed++;
    } else {
        std::cout << "[FAIL] Smoothing did not settle. Value: " << force1 << std::endl;
        g_tests_failed++;
    }
}

// ... [Skip to test_coordinate_sop_inversion] ...

static void test_coordinate_sop_inversion() {
    std::cout << "\nTest: Coordinate System - SoP Direction (v0.4.30)" << std::endl;
    FFBEngine engine;
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    // Setup: Isolate SoP effect
    engine.m_sop_effect = 1.0f;
    engine.m_sop_scale = 10.0f;
    engine.m_sop_smoothing_factor = 1.0f; // Disable smoothing for instant response
    engine.m_gain = 1.0f;
    engine.m_max_torque_ref = 20.0f;
    engine.m_understeer_effect = 0.0f;
    engine.m_rear_align_effect = 0.0f;
    engine.m_scrub_drag_gain = 0.0f;
    engine.m_slide_texture_enabled = false;
    engine.m_road_texture_enabled = false;
    engine.m_bottoming_enabled = false;
    engine.m_lockup_enabled = false;
    engine.m_spin_enabled = false;
    engine.m_sop_yaw_gain = 0.0f;
    engine.m_gyro_gain = 0.0f;
    
    data.mSteeringShaftTorque = 0.0;
    data.mWheel[0].mRideHeight = 0.1;
    data.mWheel[1].mRideHeight = 0.1;
    data.mWheel[0].mGripFract = 1.0;
    data.mWheel[1].mGripFract = 1.0;
    data.mDeltaTime = 0.01;
    
    // Test Case 1: Right Turn (Body feels left force)
    // Game: +X = Left, so lateral accel = +9.81 (left)
    // v0.4.29 FIX: We want POSITIVE internal force (Left Pull) to match Base Torque.
    data.mLocalAccel.x = 9.81; // 1G left (right turn)
    
    