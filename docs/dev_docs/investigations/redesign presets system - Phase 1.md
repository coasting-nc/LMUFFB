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