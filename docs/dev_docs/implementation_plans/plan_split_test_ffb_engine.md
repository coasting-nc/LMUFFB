# Implementation Plan: Split `test_ffb_engine.cpp` Into Modular Test Files

**Status:** PENDING  
**Version Target:** 0.7.4 (Infrastructure change - no FFB logic changes)  
**Date Created:** 2026-02-03  

---

## Context

The `test_ffb_engine.cpp` file has grown to **7,263 lines** (~288 KB) containing over 100 test functions. This size significantly hampers maintainability, navigation, and code review efficiency. This plan details the process of splitting the monolithic test file into **9 smaller, logically grouped files** plus a shared header.

**This is a pure refactoring task.** No FFB logic, algorithms, or user-facing behavior changes are involved. The test suite should produce identical results before and after the split.

---

## Reference Documents

1. **Proposal Document:**
   - `docs/dev_docs/proposal_split_test_ffb_engine.md` - Detailed analysis and rationale

---

## Codebase Analysis Summary

### Current Architecture

```
tests/
├── CMakeLists.txt              # Builds run_combined_tests executable
├── main_test_runner.cpp        # Entry point, calls FFBEngineTests::Run()
├── test_ffb_engine.cpp         # 7,263 lines - TARGET FOR SPLIT
├── test_persistence_v0625.cpp  # Standalone persistence tests
├── test_persistence_v0628.cpp  # Standalone persistence tests
├── test_screenshot.cpp         # Windows-specific screenshot tests
├── test_windows_platform.cpp   # Windows-specific platform tests
└── test_gui_interaction.cpp    # GUI interaction tests
```

### Current `test_ffb_engine.cpp` Structure

| Section | Lines | Content |
|---------|-------|---------|
| Includes & Namespace | 1-16 | 7 include statements |
| Test Framework Macros | 17-57 | `ASSERT_TRUE`, `ASSERT_NEAR`, `ASSERT_GE`, `ASSERT_LE` |
| Test Constants | 58-62 | `FILTER_SETTLING_FRAMES = 40` |
| Forward Declarations | 63-170 | ~105 test function declarations |
| Helper Functions | 172-259 | `CreateBasicTestTelemetry()`, `InitializeEngine()` |
| Test Implementations | 260-7260 | ~100+ test functions |
| Namespace Close | 7262-7263 | `} // namespace FFBEngineTests` |

### Impacted Functionalities

| Component | Impact |
|-----------|--------|
| `test_ffb_engine.cpp` | **DELETED** - Split into 9 files |
| `main_test_runner.cpp` | **MODIFIED** - Call sub-runners instead of single `Run()` |
| `CMakeLists.txt` | **MODIFIED** - Add new source files |
| **New Files** | `test_ffb_common.h`, `test_ffb_common.cpp`, + 9 category files |

---

## FFB Effects Impact Analysis

**NOT APPLICABLE** - This is a test infrastructure refactoring. No FFB algorithms, physics calculations, or user-facing behavior is changed.

---

## Proposed Changes

### Phase 1: Create Shared Infrastructure

#### File 1: `tests/test_ffb_common.h` (NEW)

**Purpose:** Shared header containing includes, macros, constants, and helper function declarations.

```cpp
// test_ffb_common.h
#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <cstdio>
#include <random>
#include <sstream>

#include "../src/FFBEngine.h"
#include "../src/lmu_sm_interface/InternalsPlugin.hpp"
#include "../src/lmu_sm_interface/LmuSharedMemoryWrapper.h"
#include "../src/Config.h"

namespace FFBEngineTests {

// --- Test Counters (defined in test_ffb_common.cpp) ---
extern int g_tests_passed;
extern int g_tests_failed;

// --- Assert Macros ---
#define ASSERT_TRUE(condition) \
    if (condition) { \
        std::cout << "[PASS] " << #condition << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #condition << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
        g_tests_failed++; \
    }

#define ASSERT_NEAR(a, b, epsilon) \
    if (std::abs((a) - (b)) < (epsilon)) { \
        std::cout << "[PASS] " << #a << " approx " << #b << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #a << " (" << (a) << ") != " << #b << " (" << (b) << ")" << std::endl; \
        g_tests_failed++; \
    }

#define ASSERT_GE(a, b) \
    if ((a) >= (b)) { \
        std::cout << "[PASS] " << #a << " >= " << #b << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #a << " (" << (a) << ") < " << #b << " (" << (b) << ")" << std::endl; \
        g_tests_failed++; \
    }

#define ASSERT_LE(a, b) \
    if ((a) <= (b)) { \
        std::cout << "[PASS] " << #a << " <= " << #b << std::endl; \
        g_tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << #a << " (" << (a) << ") > " << #b << " (" << (b) << ")" << std::endl; \
        g_tests_failed++; \
    }

// --- Test Constants ---
const int FILTER_SETTLING_FRAMES = 40;

// --- Helper Functions ---
TelemInfoV01 CreateBasicTestTelemetry(double speed = 20.0, double slip_angle = 0.0);
void InitializeEngine(FFBEngine& engine);

// --- Sub-Runner Declarations ---
void Run_CorePhysics();
void Run_SlipGrip();
void Run_Understeer();
void Run_SlopeDetection();
void Run_Texture();
void Run_YawGyro();
void Run_Coordinates();
void Run_Config();
void Run_SpeedGate();

} // namespace FFBEngineTests
```

#### File 2: `tests/test_ffb_common.cpp` (NEW)

**Purpose:** Definition of global counters and helper functions.

```cpp
// test_ffb_common.cpp
#include "test_ffb_common.h"

namespace FFBEngineTests {

// --- Global Test Counters ---
int g_tests_passed = 0;
int g_tests_failed = 0;

// --- Helper: Create Basic Test Telemetry ---
TelemInfoV01 CreateBasicTestTelemetry(double speed, double slip_angle) {
    TelemInfoV01 data;
    std::memset(&data, 0, sizeof(data));
    
    data.mDeltaTime = 0.01; // 100Hz
    data.mLocalVel.z = -speed; // Game uses -Z for forward
    
    for (int i = 0; i < 4; i++) {
        data.mWheel[i].mGripFract = 0.0;
        data.mWheel[i].mTireLoad = 4000.0;
        data.mWheel[i].mStaticUndeflectedRadius = 30;
        data.mWheel[i].mRotation = speed * 3.33f;
        data.mWheel[i].mLongitudinalGroundVel = speed;
        data.mWheel[i].mLateralPatchVel = slip_angle * speed;
        data.mWheel[i].mBrakePressure = 1.0;
        data.mWheel[i].mSuspForce = 4000.0;
        data.mWheel[i].mVerticalTireDeflection = 0.001;
    }
    
    return data;
}

// --- Helper: Initialize Engine with Test Defaults ---
void InitializeEngine(FFBEngine& engine) {
    Preset::ApplyDefaultsToEngine(engine);
    engine.m_max_torque_ref = 20.0f;
    engine.m_invert_force = false;
    
    // Zero all effects for clean physics testing
    engine.m_steering_shaft_smoothing = 0.0f; 
    engine.m_slip_angle_smoothing = 0.0f;
    engine.m_sop_smoothing_factor = 1.0f;
    engine.m_yaw_accel_smoothing = 0.0f;
    engine.m_gyro_smoothing = 0.0f;
    engine.m_chassis_inertia_smoothing = 0.0f;
    
    engine.m_sop_effect = 0.0f;
    engine.m_sop_yaw_gain = 0.0f;
    engine.m_oversteer_boost = 0.0f;
    engine.m_rear_align_effect = 0.0f;
    engine.m_gyro_gain = 0.0f;
    
    engine.m_slide_texture_enabled = false;
    engine.m_road_texture_enabled = false;
    engine.m_lockup_enabled = false;
    engine.m_spin_enabled = false;
    engine.m_abs_pulse_enabled = false;
    engine.m_scrub_drag_gain = 0.0f;
    engine.m_min_force = 0.0f;
    
    engine.m_speed_gate_lower = -10.0f;
    engine.m_speed_gate_upper = -5.0f;
}

} // namespace FFBEngineTests
```

### Phase 2: Create Category Test Files

Each file follows this pattern:
1. Include `test_ffb_common.h`
2. Define `static` test functions
3. Define a public `Run_<Category>()` function that calls all tests

#### File 3: `tests/test_ffb_core_physics.cpp` (NEW)

**Tests to include:**
- `test_base_force_modes`
- `test_grip_modulation`
- `test_min_force`
- `test_zero_input`
- `test_grip_low_speed`
- `test_gain_compensation`
- `test_high_gain_stability`
- `test_stress_stability`
- `test_smoothing_step_response`
- `test_time_corrected_smoothing`

**Runner function:**
```cpp
void Run_CorePhysics() {
    std::cout << "\n=== Core Physics Tests ===" << std::endl;
    test_base_force_modes();
    test_grip_modulation();
    // ... all tests
}
```

#### File 4: `tests/test_ffb_slip_grip.cpp` (NEW)

**Tests to include:**
- `test_sop_effect`
- `test_combined_grip_loss`
- `test_optimal_slip_buffer_zone`
- `test_progressive_loss_curve`
- `test_grip_floor_clamp`
- `test_grip_threshold_sensitivity`

#### File 5: `tests/test_ffb_understeer.cpp` (NEW)

**Tests to include:**
- `test_understeer_output_clamp`
- `test_understeer_range_validation`
- `test_understeer_effect_scaling`
- `test_preset_understeer_only_isolation`

#### File 6: `tests/test_ffb_slope_detection.cpp` (NEW)

**Tests to include (22 tests):**
- All `test_slope_*` functions
- All v0.7.1 and v0.7.3 slope detection tests

#### File 7: `tests/test_ffb_texture.cpp` (NEW)

**Tests to include:**
- `test_road_texture_teleport`
- `test_scrub_drag_fade`
- `test_lockup_pitch_scaling`
- `test_progressive_lockup`
- `test_abs_frequency_scaling`
- `test_abs_pulse_v060`
- `test_notch_filter_attenuation`
- `test_notch_filter_bandwidth`
- `test_notch_filter_edge_cases`
- `test_static_notch_integration`
- `test_frequency_estimator`
- `test_predictive_lockup_v060`
- `test_rear_lockup_differentiation`
- `test_split_load_caps`
- `test_dynamic_thresholds`
- `test_universal_bottoming`

#### File 8: `tests/test_ffb_yaw_gyro.cpp` (NEW)

**Tests to include:**
- `test_sop_yaw_kick`
- `test_sop_yaw_kick_direction`
- `test_yaw_kick_threshold`
- `test_yaw_kick_edge_cases`
- `test_yaw_kick_signal_conditioning`
- `test_gyro_damping`
- `test_gyro_stability`
- `test_yaw_accel_smoothing`
- `test_yaw_accel_convergence`
- `test_chassis_inertia_smoothing_convergence`
- `test_kinematic_load_braking`
- `test_kinematic_load_cornering`
- `test_rear_align_effect`
- `test_rear_force_workaround`

#### File 9: `tests/test_ffb_coordinates.cpp` (NEW)

**Tests to include:**
- `test_coordinate_sop_inversion`
- `test_coordinate_rear_torque_inversion`
- `test_coordinate_scrub_drag_direction`
- `test_coordinate_debug_slip_angle_sign`
- `test_coordinate_all_effects_alignment`
- `test_regression_yaw_slide_feedback`
- `test_regression_no_positive_feedback`
- `test_regression_phase_explosion`
- `test_regression_road_texture_toggle`
- `test_regression_bottoming_switch`
- `test_regression_rear_torque_lpf`

#### File 10: `tests/test_ffb_config.cpp` (NEW)

**Tests to include:**
- `test_config_safety_clamping`
- `test_config_defaults_v057`
- `test_config_safety_validation_v057`
- `test_legacy_config_migration`
- `test_preset_initialization`
- `test_all_presets_non_negative_speed_gate`
- `test_snapshot_data_integrity`
- `test_snapshot_data_v049`
- `test_zero_effects_leakage`
- `test_missing_telemetry_warnings`
- `test_game_state_logic`
- `test_refactor_abs_pulse`
- `test_refactor_torque_drop`
- `test_refactor_snapshot_sop`
- `test_refactor_units`
- `test_wheel_slip_ratio_helper`
- `test_signal_conditioning_helper`
- `test_unconditional_vert_accel_update`
- `FFBEngineTestAccess::test_unit_sop_lateral`
- `FFBEngineTestAccess::test_unit_gyro_damping`
- `FFBEngineTestAccess::test_unit_abs_pulse`

#### File 11: `tests/test_ffb_speed_gate.cpp` (NEW)

**Tests to include:**
- `test_stationary_gate`
- `test_idle_smoothing`
- `test_stationary_silence`
- `test_driving_forces_restored`
- `test_smoothstep_helper_function`
- `test_smoothstep_vs_linear`
- `test_smoothstep_edge_cases`
- `test_speed_gate_uses_smoothstep`
- `test_smoothstep_stationary_silence_preserved`

### Phase 3: Update Build Infrastructure

#### Update `tests/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.10)
project(LMUFFB_Tests)

set(CMAKE_CXX_STANDARD 17)

include_directories(..)
include_directories(../src)

# Combined Test Executable - UPDATED for split files
set(TEST_SOURCES 
    main_test_runner.cpp 
    test_ffb_common.cpp
    test_ffb_core_physics.cpp
    test_ffb_slip_grip.cpp
    test_ffb_understeer.cpp
    test_ffb_slope_detection.cpp
    test_ffb_texture.cpp
    test_ffb_yaw_gyro.cpp
    test_ffb_coordinates.cpp
    test_ffb_config.cpp
    test_ffb_speed_gate.cpp
    test_persistence_v0625.cpp
    test_persistence_v0628.cpp
    ../src/Config.cpp
)

if(WIN32)
    list(APPEND TEST_SOURCES 
        test_windows_platform.cpp 
        test_screenshot.cpp
        test_gui_interaction.cpp
        ../src/DirectInputFFB.cpp 
        ../src/GuiLayer.cpp
        ../src/GameConnector.cpp
        ${IMGUI_SOURCES}
    )
endif()

enable_testing()
add_executable(run_combined_tests ${TEST_SOURCES})

if(WIN32)
    target_link_libraries(run_combined_tests dinput8 dxguid version imm32 winmm d3d11 d3dcompiler dxgi)
endif()

add_test(NAME CombinedTests COMMAND run_combined_tests)
```

#### Update `tests/main_test_runner.cpp`

```cpp
// Forward declarations - UPDATED for sub-runners
namespace FFBEngineTests { 
    extern int g_tests_passed; 
    extern int g_tests_failed; 
    void Run_CorePhysics();
    void Run_SlipGrip();
    void Run_Understeer();
    void Run_SlopeDetection();
    void Run_Texture();
    void Run_YawGyro();
    void Run_Coordinates();
    void Run_Config();
    void Run_SpeedGate();
}

// In main():
// --- FFB Engine Tests (Split) ---
try {
    std::cout << "\n========================================" << std::endl;
    std::cout << "       FFB ENGINE TESTS (Modular)       " << std::endl;
    std::cout << "========================================" << std::endl;
    
    FFBEngineTests::Run_CorePhysics();
    FFBEngineTests::Run_SlipGrip();
    FFBEngineTests::Run_Understeer();
    FFBEngineTests::Run_SlopeDetection();
    FFBEngineTests::Run_Texture();
    FFBEngineTests::Run_YawGyro();
    FFBEngineTests::Run_Coordinates();
    FFBEngineTests::Run_Config();
    FFBEngineTests::Run_SpeedGate();
    
    total_passed += FFBEngineTests::g_tests_passed;
    total_failed += FFBEngineTests::g_tests_failed;
} catch (...) {
    total_failed++;
}
```

### Phase 4: Delete Original File

After verification that all tests pass, delete:
- `tests/test_ffb_engine.cpp`

---

## Version Increment Rule

**Version Increment:** `0.7.3` → `0.7.4`

Per project versioning rules, the version should be incremented by the **smallest possible increment** (+1 to the rightmost number). Since this is an infrastructure change with no user-facing impact, a patch version bump is appropriate.

**Files to update:**
- `VERSION` (0.7.3 → 0.7.4)
- `src/Version.h` (if applicable)

---

## Test Plan (TDD-Ready)

### Pre-Split Verification

**CRITICAL:** Before any code changes, capture the baseline test count.

```powershell
# Record baseline
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -SkipAutomaticLocation
cmake -S . -B build
cmake --build build --config Release --clean-first
.\build\tests\Release\run_combined_tests.exe > baseline_tests.log
```

**Expected:** Record exact number of PASS/FAIL assertions.

### Post-Split Verification

After completing the split:

1. **Build must succeed without errors**
2. **Test count must be identical to baseline**
3. **No tests should be missing or duplicated**

### Verification Tests

#### Test 1: Build Verification

```powershell
cmake -S . -B build
cmake --build build --config Release --clean-first
# Expected: Build succeeds with 0 errors
```

#### Test 2: Test Count Verification

```powershell
.\build\tests\Release\run_combined_tests.exe
# Expected: TOTAL PASSED == Baseline, TOTAL FAILED == 0
```

#### Test 3: File Size Sanity Check

| File | Expected Lines | Tolerance |
|------|---------------|-----------|
| `test_ffb_common.h` | ~80 | ±10 |
| `test_ffb_common.cpp` | ~60 | ±10 |
| `test_ffb_core_physics.cpp` | ~800-1200 | ±200 |
| `test_ffb_slope_detection.cpp` | ~1000-1400 | ±200 |
| **Total lines across all new files** | ~7,400 | ±500 |

---

## Deliverables

### Code Changes

- [ ] Create `tests/test_ffb_common.h`
- [ ] Create `tests/test_ffb_common.cpp`
- [ ] Create `tests/test_ffb_core_physics.cpp`
- [ ] Create `tests/test_ffb_slip_grip.cpp`
- [ ] Create `tests/test_ffb_understeer.cpp`
- [ ] Create `tests/test_ffb_slope_detection.cpp`
- [ ] Create `tests/test_ffb_texture.cpp`
- [ ] Create `tests/test_ffb_yaw_gyro.cpp`
- [ ] Create `tests/test_ffb_coordinates.cpp`
- [ ] Create `tests/test_ffb_config.cpp`
- [ ] Create `tests/test_ffb_speed_gate.cpp`
- [ ] Update `tests/CMakeLists.txt`
- [ ] Update `tests/main_test_runner.cpp`
- [ ] Delete `tests/test_ffb_engine.cpp`
- [ ] Update `VERSION` (0.7.3 → 0.7.4)
- [ ] Update `src/Version.h` (if applicable)

### Tests

- [ ] Verify build succeeds with new file structure
- [ ] Verify test count matches pre-split baseline exactly
- [ ] Verify no duplicate test function names across files

### Documentation

- [ ] Update CHANGELOG.md with v0.7.4 entry:
  ```markdown
  ## [0.7.4] - 2026-02-XX
  ### Changed
  - Refactored `test_ffb_engine.cpp` into 9 modular test files for improved maintainability
  - Added shared test infrastructure (`test_ffb_common.h/cpp`)
  ```

### Implementation Notes

- [ ] **Update this document** with:
  - Unforeseen Issues encountered
  - Plan Deviations (if any)
  - Challenges and how they were resolved
  - Recommendations for future refactoring tasks

---

## Risk Mitigation

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Missing test after split | Medium | High | Compare test counts before/after |
| Duplicate test function name | Low | Medium | Use `static` keyword, unique names |
| Include order issues | Low | Low | Common header pattern handles this |
| Build failure | Low | Medium | Incremental migration, frequent builds |
| Test isolation issues | Low | Medium | Shared counters in common.cpp |

---

## Implementation Order

**Recommended execution order:**

1. **Capture baseline test count** (MANDATORY FIRST STEP)
2. Create `test_ffb_common.h` and `test_ffb_common.cpp`
3. Update `CMakeLists.txt` to include new files
4. Create one test file at a time, moving tests from original
5. After each file, verify build and test count
6. Update `main_test_runner.cpp` 
7. Delete original `test_ffb_engine.cpp`
8. Final verification
9. Update version files
10. Update CHANGELOG

---

## Document History

| Version | Date | Author | Notes |
|---------|------|--------|-------|
| 1.0 | 2026-02-03 | Gemini | Initial implementation plan |
