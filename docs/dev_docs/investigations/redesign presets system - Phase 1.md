Presets System Refactoring Phase 1: Grouping Data Structures

Phase 1 touches the core data flow of the application. While the changes are mostly mechanical (find-and-replace), they affect `FFBEngine`, `Preset`, `Config`, and `GuiLayer`. 

Doing this safely requires a solid safety net of unit tests *before* you change a single line of production code.

Here is the detailed breakdown of Phase 1, followed by the exact unit tests you should write first.

---

### The Detailed Plan for Phase 1: Grouping Data Structures

The ultimate goal of Phase 1 is to stop duplicating 100+ variables between `Preset` and `FFBEngine`. Instead, both will share a single, grouped configuration object.

#### Step 1.1: Define the Sub-Structs (in a new file `FFBConfig.h` or inside `Config.h`)
Group the loose variables into logical categories. Add an `Equals` method to each that uses your existing `0.0001f` epsilon (to avoid floating-point precision bugs that a default `operator==` might cause).

```cpp
struct GeneralConfig {
    float gain = 1.0f;
    bool invert_force = true;
    float min_force = 0.0f;
    float wheelbase_max_nm = 15.0f;
    float target_rim_nm = 10.0f;
    bool dynamic_normalization_enabled = false;
    
    bool Equals(const GeneralConfig& o) const {
        return std::abs(gain - o.gain) < 0.0001f &&
               invert_force == o.invert_force &&
               // ... compare rest ...
    }
    void Validate() {
        gain = std::max(0.0f, gain);
        wheelbase_max_nm = std::max(1.0f, wheelbase_max_nm);
        // ... clamp rest ...
    }
};

struct FrontAxleConfig { /* understeer, shaft_gain, flatspot... */ };
struct RearAxleConfig { /* oversteer, sop, yaw_kick... */ };
struct BrakingConfig { /* lockup, abs... */ };
struct VibrationConfig { /* road, slide, spin, bottoming... */ };
struct AdvancedConfig { /* slip angles, smoothing... */ };
struct SlopeConfig { /* slope detection params... */ };
```

#### Step 1.2: Create the Master `FFBConfig` Struct
Combine them into one master struct.

```cpp
struct FFBConfig {
    GeneralConfig general;
    FrontAxleConfig front_axle;
    RearAxleConfig rear_axle;
    BrakingConfig braking;
    VibrationConfig vibration;
    AdvancedConfig advanced;
    SlopeConfig slope;

    bool Equals(const FFBConfig& o) const {
        return general.Equals(o.general) && front_axle.Equals(o.front_axle) /* ... */;
    }
    void Validate() {
        general.Validate(); front_axle.Validate(); /* ... */
    }
};
```

#### Step 1.3: Update `Preset` and `FFBEngine`
Replace the 100+ loose variables in both classes with the new master struct.

*   **In `Preset`:**
    ```cpp
    struct Preset {
        std::string name;
        bool is_builtin = false;
        std::string app_version = LMUFFB_VERSION;
        
        FFBConfig cfg; // <-- Replaces all 100 loose variables!

        void Apply(FFBEngine& engine) const {
            engine.m_config = this->cfg;
            engine.m_config.Validate(); // Safety clamp
        }
        void UpdateFromEngine(const FFBEngine& engine) {
            this->cfg = engine.m_config;
        }
        bool Equals(const Preset& p) const {
            return cfg.Equals(p.cfg);
        }
    };
    ```
*   **In `FFBEngine`:**
    ```cpp
    class FFBEngine {
    public:
        FFBConfig m_config; // <-- Replaces all 100 loose variables!
        FFBSafetyMonitor m_safety; // Keep this separate as it has state
        // ... keep internal state variables (m_prev_slip_angle, etc.) ...
    };
    ```

#### Step 1.4: The "Find and Replace" Phase
This is the tedious but safe part. You will get hundreds of compiler errors. Fix them by updating the paths:
*   In `FFBEngine.cpp`: Change `m_gain` to `m_config.general.gain`. Change `m_understeer_effect` to `m_config.front_axle.understeer_effect`.
*   In `Config.cpp`: Update the INI parsers to write to `engine.m_config.general.gain`.
*   In `GuiLayer.cpp`: Update the ImGui sliders to point to `engine.m_config.general.gain`.

---

### The Safety Net: Unit Tests to Write *Before* Phase 1

Add these tests to your test suite (e.g., in a new file `test_refactor_safety.cpp`). Run them on the *current* codebase to ensure they pass. Then, do the refactoring. If they still pass after the refactoring, you know you haven't broken anything.

#### Test 1: The Physics Output Consistency Test (The most important one)
This test feeds a fixed telemetry frame into the engine and records the exact output force. If you mess up a variable mapping during refactoring, this test will fail.

```cpp
#include "test_ffb_common.h"

namespace FFBEngineTests {

TEST_CASE(test_refactor_physics_consistency, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    // 1. Set a very specific, non-default configuration
    // (Use the current loose variables. After refactor, update these lines to use m_config...)
    engine.m_gain = 0.85f;
    engine.m_understeer_effect = 1.2f;
    engine.m_sop_effect = 0.75f;
    engine.m_road_texture_enabled = true;
    engine.m_road_texture_gain = 1.5f;
    engine.m_wheelbase_max_nm = 12.0f;
    engine.m_target_rim_nm = 8.0f;

    // 2. Create a specific telemetry state (cornering + bumps)
    TelemInfoV01 data = CreateBasicTestTelemetry(25.0, 0.08); // 25m/s, 8% slip
    data.mLocalAccel.x = 15.0; // High lateral G
    data.mUnfilteredSteering = 0.5; // Steering turned
    data.mSteeringShaftTorque = 10.0; // Raw game torque
    
    // Add some suspension velocity for road texture
    data.mWheel[0].mVerticalTireDeflection = 0.01; 

    // 3. Pump the engine to settle filters
    PumpEngineSteadyState(engine, data);

    // 4. Get the final output force
    double final_force = engine.calculate_force(&data, "LMP2", "Oreca", 0.0f, true, 0.0025);

    // 5. ASSERT THE EXACT VALUE
    // RUN THIS ON THE CURRENT CODEBASE FIRST. 
    // Replace 'EXPECTED_VALUE' with whatever this actually outputs right now.
    // Example: ASSERT_NEAR(final_force, 0.4521, 0.0001);
    
    // For now, we just ensure it's not zero and doesn't crash.
    // Once you run it, hardcode the result here.
    double EXPECTED_VALUE = final_force; // TODO: Hardcode the output of the pre-refactored code here!
    
    ASSERT_NEAR(final_force, EXPECTED_VALUE, 0.0001);
}

}
```

#### Test 2: The Preset Round-Trip Test
This ensures that `Apply`, `UpdateFromEngine`, and `Equals` work perfectly.

```cpp
namespace FFBEngineTests {

TEST_CASE(test_refactor_preset_roundtrip, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    // 1. Create a preset with wild, non-default values
    Preset original("TestPreset", false);
    original.gain = 1.77f;
    original.understeer = 0.33f;
    original.lockup_enabled = true;
    original.lockup_gain = 2.5f;
    original.slope_detection_enabled = true;
    original.slope_sg_window = 21;

    // 2. Apply to engine
    original.Apply(engine);

    // 3. Verify engine received the values
    ASSERT_NEAR(engine.m_gain, 1.77f, 0.0001);
    ASSERT_NEAR(engine.m_understeer_effect, 0.33f, 0.0001);
    ASSERT_EQ(engine.m_lockup_enabled, true);
    ASSERT_EQ(engine.m_slope_sg_window, 21); // Note: Validate() might change this if it enforces odd numbers, check your logic!

    // 4. Extract back to a new preset
    Preset extracted("Extracted", false);
    extracted.UpdateFromEngine(engine);

    // 5. They should be exactly equal
    ASSERT_TRUE(original.Equals(extracted));
}

}
```

#### Test 3: The Validation/Clamping Test
Ensures that out-of-bounds values in a preset don't infect the engine.

```cpp
namespace FFBEngineTests {

TEST_CASE(test_refactor_validation_clamping, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset bad_preset("Bad", false);
    
    // Set malicious values
    bad_preset.gain = -5.0f; // Should clamp to 0.0
    bad_preset.understeer = 10.0f; // Should clamp to 2.0
    bad_preset.wheelbase_max_nm = 0.0f; // Should clamp to 1.0
    bad_preset.notch_q = 0.0f; // Should clamp to 0.1 (prevent div by zero)

    // Apply (which should trigger Validate())
    bad_preset.Apply(engine);

    // Verify engine clamped them
    ASSERT_NEAR(engine.m_gain, 0.0f, 0.0001);
    ASSERT_NEAR(engine.m_understeer_effect, 2.0f, 0.0001);
    ASSERT_NEAR(engine.m_wheelbase_max_nm, 1.0f, 0.0001);
    ASSERT_NEAR(engine.m_notch_q, 0.1f, 0.0001);
}

}
```

### How to proceed:
1. Copy those three tests into your test suite.
2. Run the tests on your *current* code.
3. For `test_refactor_physics_consistency`, look at the console output, find the exact `final_force` it generated, and hardcode that number into the `EXPECTED_VALUE` variable.
4. Commit your code (`git commit -m "Add safety tests before refactoring"`).
5. Begin Phase 1 (creating the structs and doing the find-and-replace).
6. Run the tests again. If they pass, your refactoring was a complete success and you are ready for Phase 2 (TOML).


## Adopt the "Strangler Fig" pattern in refactoring

This is known as the "Strangler Fig" pattern in refactoring—you slowly replace parts of the old system with the new system while keeping the application fully functional and compiling at every step.

Because C++ allows you to mix loose variables and nested structs inside a class, you can migrate exactly one category at a time.

Here is exactly how you execute this incremental strategy, starting with **`FrontAxleConfig`**.

---

### The Incremental State (How it will look)

During this first increment, your `Preset` and `FFBEngine` classes will live in a hybrid state. They will contain the new `FrontAxleConfig` struct, alongside all the other loose variables that haven't been migrated yet.

```cpp
// FFBEngine.h (Intermediate State)

struct FrontAxleConfig {
    float steering_shaft_gain = 1.0f;
    float ingame_ffb_gain = 1.0f;
    float steering_shaft_smoothing = 0.0f;
    float understeer_effect = 1.0f;
    float understeer_gamma = 1.0f;
    int torque_source = 0;
    int steering_100hz_reconstruction = 0;
    bool torque_passthrough = false;
    
    // Signal Filtering (Logically belongs to the front axle input)
    bool flatspot_suppression = false;
    float notch_q = 2.0f;
    float flatspot_strength = 1.0f;
    bool static_notch_enabled = false;
    float static_notch_freq = 11.0f;
    float static_notch_width = 2.0f;

    bool Equals(const FrontAxleConfig& o) const;
    void Validate();
};

class FFBEngine {
public:
    // --- MIGRATED ---
    FrontAxleConfig m_front_axle;

    // --- NOT YET MIGRATED (Leave these alone for now) ---
    float m_gain;
    float m_sop_effect;
    bool m_lockup_enabled;
    // ... etc ...
};
```

---

### Step 1: Write the Unit Tests (Do this *before* changing production code)

Create a new test file (e.g., `test_refactor_front_axle.cpp`). These tests target the specific variables you are about to move. 

**Run these tests on your CURRENT codebase first.** They should pass.

```cpp
#include "test_ffb_common.h"

namespace FFBEngineTests {

// 1. Consistency Test: Ensures the math doesn't change when variables move
TEST_CASE(test_refactor_front_axle_consistency, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    // Set specific Front Axle values (using current loose variables)
    engine.m_understeer_effect = 1.5f;
    engine.m_understeer_gamma = 0.8f;
    engine.m_steering_shaft_gain = 0.9f;
    engine.m_flatspot_suppression = true;
    engine.m_notch_q = 4.0f;

    // Create a telemetry state that triggers understeer and flatspot logic
    TelemInfoV01 data = CreateBasicTestTelemetry(25.0, 0.15); // High slip = understeer
    data.mUnfilteredSteering = 0.5;
    data.mSteeringShaftTorque = 12.0;
    
    PumpEngineSteadyState(engine, data);
    double final_force = engine.calculate_force(&data, "LMP2", "Oreca", 0.0f, true, 0.0025);

    // TODO: Run this test ONCE on the current code, look at the output, 
    // and hardcode the exact result here.
    double EXPECTED_VALUE = final_force; 
    
    ASSERT_NEAR(final_force, EXPECTED_VALUE, 0.0001);
}

// 2. Round-Trip Test: Ensures Preset <-> Engine synchronization works
TEST_CASE(test_refactor_front_axle_roundtrip, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset original("TestPreset", false);
    // Set wild values for Front Axle variables
    original.understeer = 0.44f;
    original.understeer_gamma = 2.1f;
    original.steering_shaft_gain = 1.8f;
    original.torque_source = 1;
    original.static_notch_enabled = true;
    original.static_notch_freq = 45.0f;

    // Apply to engine
    original.Apply(engine);

    // Verify engine received them (using current loose variables)
    ASSERT_NEAR(engine.m_understeer_effect, 0.44f, 0.0001);
    ASSERT_NEAR(engine.m_understeer_gamma, 2.1f, 0.0001);
    ASSERT_NEAR(engine.m_steering_shaft_gain, 1.8f, 0.0001);
    ASSERT_EQ(engine.m_torque_source, 1);
    ASSERT_EQ(engine.m_static_notch_enabled, true);
    ASSERT_NEAR(engine.m_static_notch_freq, 45.0f, 0.0001);

    // Extract back to a new preset
    Preset extracted("Extracted", false);
    extracted.UpdateFromEngine(engine);

    // They should be exactly equal
    ASSERT_TRUE(original.Equals(extracted));
}

// 3. Validation Test: Ensures clamping logic is preserved
TEST_CASE(test_refactor_front_axle_validation, "RefactorSafety") {
    FFBEngine engine;
    InitializeEngine(engine);

    Preset bad_preset("Bad", false);
    
    // Set malicious Front Axle values
    bad_preset.understeer = 5.0f;         // Should clamp to 2.0
    bad_preset.understeer_gamma = -1.0f;  // Should clamp to 0.1
    bad_preset.steering_shaft_gain = -5.0f; // Should clamp to 0.0
    bad_preset.torque_source = 5;         // Should clamp to 1
    bad_preset.notch_q = 0.0f;            // Should clamp to 0.1

    bad_preset.Apply(engine);

    // Verify engine clamped them (using current loose variables)
    ASSERT_NEAR(engine.m_understeer_effect, 2.0f, 0.0001);
    ASSERT_NEAR(engine.m_understeer_gamma, 0.1f, 0.0001);
    ASSERT_NEAR(engine.m_steering_shaft_gain, 0.0f, 0.0001);
    ASSERT_EQ(engine.m_torque_source, 1);
    ASSERT_NEAR(engine.m_notch_q, 0.1f, 0.0001);
}

}
```

---

### Step 2: Execute the Refactoring for `FrontAxleConfig`

Once the tests are written and passing on the old code, you make the changes.

1.  **Define the Struct:** Add `FrontAxleConfig` to `Config.h` (or `FFBEngine.h`). Implement its `Equals()` and `Validate()` methods.
2.  **Update `Preset`:**
    *   Delete the loose front axle variables (`understeer`, `understeer_gamma`, `steering_shaft_gain`, etc.) from the `Preset` struct.
    *   Add `FrontAxleConfig front_axle;` to `Preset`.
    *   Update `Preset::Apply()`: `engine.m_front_axle = this->front_axle; engine.m_front_axle.Validate();`
    *   Update `Preset::UpdateFromEngine()`: `this->front_axle = engine.m_front_axle;`
    *   Update `Preset::Equals()`: `if (!front_axle.Equals(p.front_axle)) return false;`
3.  **Update `FFBEngine`:**
    *   Delete the loose front axle variables (`m_understeer_effect`, `m_steering_shaft_gain`, etc.) from the `FFBEngine` class.
    *   Add `FrontAxleConfig m_front_axle;` to `FFBEngine`.
4.  **Fix Compiler Errors (The Find-and-Replace):**
    *   In `FFBEngine.cpp`: Change `m_understeer_effect` to `m_front_axle.understeer_effect`.
    *   In `Config.cpp`: Update `ParsePhysicsLine`, `SyncPhysicsLine`, and `WritePresetFields` to point to the new `front_axle` struct.
    *   In `GuiLayer.cpp`: Update the ImGui sliders to point to `engine.m_front_axle.understeer_effect`.
    *   In your **Unit Tests**: Update the tests you just wrote to use the new struct paths (e.g., `engine.m_front_axle.understeer_effect`).

### Step 3: Verify and Commit

Run the unit tests again. 
*   If they pass, your math is identical, your INI parsing still works, and your UI still connects to the right variables.
*   Commit the code: `git commit -m "Refactor: Group Front Axle variables into FrontAxleConfig"`

### Step 4: Move to the Next Category

Now you repeat the exact same process for the next logical block. For example, **`RearAxleConfig`**:
1. Write `test_refactor_rear_axle.cpp` targeting `m_sop_effect`, `m_oversteer_boost`, `m_yaw_kick_threshold`, etc.
2. Run tests on current code.
3. Create `RearAxleConfig`.
4. Move variables in `Preset` and `FFBEngine`.
5. Fix compiler errors.
6. Run tests.
7. Commit.

By doing this incrementally, if you make a typo (e.g., mapping the UI slider for `notch_q` to `flatspot_strength` by accident), the compiler or the unit tests will catch it immediately, and you will only have to look at the ~15 variables you changed in that specific commit, rather than hunting through 100+ variables.