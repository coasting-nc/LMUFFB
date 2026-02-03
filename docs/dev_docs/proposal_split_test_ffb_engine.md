# Proposal: Splitting `test_ffb_engine.cpp` for Improved Maintainability

**Date:** 2026-02-03  
**Author:** Gemini (AI Assistant)  
**Status:** Proposal  
**Target File:** `tests/test_ffb_engine.cpp`  
**Current Size:** 7,263 lines (~288 KB)  

---

## Executive Summary

The `test_ffb_engine.cpp` file has grown to over 7,200 lines of code containing approximately **100+ test functions**. This makes the file difficult to navigate, maintain, and understand. This proposal recommends splitting it into **7-8 smaller, logically grouped files** based on the functionality being tested.

---

## Current State Analysis

### File Statistics
| Metric | Value |
|--------|-------|
| Total Lines | 7,263 |
| Total Size | ~288 KB |
| Outline Items | 240 |
| Test Functions | ~100+ |
| Forward Declarations | ~105 |
| Helper Functions | 2 |
| Version Range Covered | v0.4.x - v0.7.3 |

### Identified Test Categories

After analyzing the file structure, the tests can be grouped into the following logical categories:

#### 1. **Core FFB Physics** (Estimated: ~1,200 lines)
Tests for fundamental force feedback calculations:
- `test_base_force_modes`
- `test_grip_modulation`
- `test_min_force`
- `test_zero_input`
- `test_grip_low_speed`
- `test_gain_compensation`
- `test_high_gain_stability`
- `test_stress_stability`

#### 2. **Slip & Grip Effects** (Estimated: ~800 lines)
Tests related to slip angle and grip calculation:
- `test_sop_effect`
- `test_combined_grip_loss`
- `test_optimal_slip_buffer_zone`
- `test_progressive_loss_curve`
- `test_grip_floor_clamp`
- `test_grip_threshold_sensitivity`

#### 3. **Understeer Effects** (Estimated: ~600 lines)
Tests specific to understeer feedback:
- `test_understeer_output_clamp`
- `test_understeer_range_validation`
- `test_understeer_effect_scaling`
- `test_preset_understeer_only_isolation`

#### 4. **Slope Detection** (Estimated: ~1,100 lines)
All tests for the v0.7.x slope detection feature:
- `test_slope_detection_buffer_init`
- `test_slope_sg_derivative`
- `test_slope_grip_at_peak`
- `test_slope_grip_past_peak`
- `test_slope_vs_static_comparison`
- `test_slope_config_persistence`
- `test_slope_latency_characteristics`
- `test_slope_noise_rejection`
- `test_slope_buffer_reset_on_toggle`
- `test_slope_detection_no_boost_when_grip_balanced` (v0.7.1)
- `test_slope_detection_no_boost_during_oversteer` (v0.7.1)
- `test_lat_g_boost_works_without_slope_detection` (v0.7.1)
- `test_slope_detection_default_values_v071` (v0.7.1)
- `test_slope_current_in_snapshot` (v0.7.1)
- `test_slope_detection_less_aggressive_v071` (v0.7.1)
- `test_slope_decay_on_straight` (v0.7.3)
- `test_slope_alpha_threshold_configurable` (v0.7.3)
- `test_slope_confidence_gate` (v0.7.3)
- `test_slope_stability_config_persistence` (v0.7.3)
- `test_slope_no_understeer_on_straight_v073` (v0.7.3)
- `test_slope_decay_rate_boundaries` (v0.7.3)
- `test_slope_alpha_threshold_validation` (v0.7.3)

#### 5. **Texture & Vibration Effects** (Estimated: ~700 lines)
Tests for road feel and haptic feedback:
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

#### 6. **Yaw & Gyroscopic Effects** (Estimated: ~900 lines)
Tests for rotational dynamics feedback:
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

#### 7. **Coordinate System & Regressions** (Estimated: ~800 lines)
Tests for coordinate system fixes and regression prevention:
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

#### 8. **Configuration & Persistence** (Estimated: ~800 lines)
Tests for config handling:
- `test_config_safety_clamping`
- `test_config_defaults_v057`
- `test_config_safety_validation_v057`
- `test_legacy_config_migration`
- `test_preset_initialization`
- `test_all_presets_non_negative_speed_gate`
- `test_snapshot_data_integrity`
- `test_snapshot_data_v049`
- `test_zero_effects_leakage`

#### 9. **Speed Gate & Stationary Behavior** (Estimated: ~500 lines)
Tests for speed-based effect gating:
- `test_stationary_gate`
- `test_idle_smoothing`
- `test_stationary_silence`
- `test_driving_forces_restored`
- `test_smoothstep_helper_function` (v0.7.2)
- `test_smoothstep_vs_linear` (v0.7.2)
- `test_smoothstep_edge_cases` (v0.7.2)
- `test_speed_gate_uses_smoothstep` (v0.7.2)
- `test_smoothstep_stationary_silence_preserved` (v0.7.2)

---

## Proposed File Structure

```
tests/
├── CMakeLists.txt              (updated)
├── main_test_runner.cpp        (updated)
├── test_ffb_common.h           (NEW - shared test infrastructure)
├── test_ffb_core_physics.cpp   (NEW)
├── test_ffb_slip_grip.cpp      (NEW)
├── test_ffb_understeer.cpp     (NEW)
├── test_ffb_slope_detection.cpp (NEW)
├── test_ffb_texture.cpp        (NEW)
├── test_ffb_yaw_gyro.cpp       (NEW)
├── test_ffb_coordinates.cpp    (NEW - coordinates & regressions)
├── test_ffb_config.cpp         (NEW)
├── test_ffb_speed_gate.cpp     (NEW)
├── test_persistence_v0625.cpp  (existing)
├── test_persistence_v0628.cpp  (existing)
├── test_screenshot.cpp         (existing)
├── test_windows_platform.cpp   (existing)
└── test_gui_interaction.cpp    (existing)
```

---

## Implementation Details

### 1. Create Shared Header (`test_ffb_common.h`)

This header will contain:
- Include statements
- Namespace declaration
- Global test counters (`g_tests_passed`, `g_tests_failed`)
- Assert macros (`ASSERT_TRUE`, `ASSERT_NEAR`, `ASSERT_GE`, `ASSERT_LE`)
- Test constants (`FILTER_SETTLING_FRAMES`)
- Helper functions:
  - `CreateBasicTestTelemetry()`
  - `InitializeEngine()`

```cpp
// test_ffb_common.h
#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstring>
#include <algorithm>
#include "../src/FFBEngine.h"
#include "../src/lmu_sm_interface/InternalsPlugin.hpp"
#include "../src/lmu_sm_interface/LmuSharedMemoryWrapper.h"
#include "../src/Config.h"
#include <fstream>
#include <cstdio>
#include <random>
#include <sstream>

namespace FFBEngineTests {

// --- Test Counters ---
extern int g_tests_passed;
extern int g_tests_failed;

// --- Assert Macros ---
#define ASSERT_TRUE(condition) /* ... */
#define ASSERT_NEAR(a, b, epsilon) /* ... */
#define ASSERT_GE(a, b) /* ... */
#define ASSERT_LE(a, b) /* ... */

// --- Test Constants ---
const int FILTER_SETTLING_FRAMES = 40;

// --- Helper Functions ---
TelemInfoV01 CreateBasicTestTelemetry(double speed = 20.0, double slip_angle = 0.0);
void InitializeEngine(FFBEngine& engine);

} // namespace FFBEngineTests
```

### 2. Each Test File Pattern

Each new test file will follow this pattern:

```cpp
// test_ffb_<category>.cpp
#include "test_ffb_common.h"

namespace FFBEngineTests {

// Test implementations...
static void test_xxx() { /* ... */ }
static void test_yyy() { /* ... */ }

// Sub-namespace runner
void Run_CategoryName() {
    std::cout << "\n=== Category Name Tests ===" << std::endl;
    test_xxx();
    test_yyy();
    // ...
}

} // namespace FFBEngineTests
```

### 3. Update `main_test_runner.cpp`

Add forward declarations for each sub-runner:

```cpp
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
```

### 4. Update `CMakeLists.txt`

```cmake
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
```

---

## Benefits

| Benefit | Description |
|---------|-------------|
| **Improved Readability** | Each file focuses on one logical area, making it easier to understand |
| **Faster Navigation** | Developers can jump directly to the relevant test file |
| **Better Compilation** | Smaller translation units = faster incremental builds |
| **Easier Maintenance** | Bug fixes can be made without scrolling through 7000+ lines |
| **Clear Ownership** | Easier to assign ownership/review of specific test areas |
| **Parallel Development** | Multiple developers can work on different test files simultaneously |
| **Version Control** | Smaller diffs, cleaner git history |

---

## Risks & Mitigations

| Risk | Mitigation |
|------|------------|
| **Missing test after split** | Run full test suite before and after, compare pass counts |
| **Broken includes** | Use consistent shared header pattern |
| **Circular dependencies** | Keep helper functions in common header only |
| **Build breakage** | Update CMakeLists.txt carefully, verify builds after each file |

---

## Effort Estimate

| Task | Estimated Time |
|------|---------------|
| Create `test_ffb_common.h/cpp` | 1 hour |
| Split and migrate each category (9 files) | 4-5 hours |
| Update CMakeLists.txt | 30 minutes |
| Update main_test_runner.cpp | 30 minutes |
| Testing & verification | 1 hour |
| **Total** | **7-8 hours** |

---

## Recommendation

**RECOMMENDED: Proceed with the split.**

The file has grown beyond maintainable limits. The split will:
1. Reduce cognitive load when adding new tests
2. Make code reviews faster and more focused
3. Improve developer experience significantly
4. Follow best practices for test organization

The one-time investment of ~8 hours will pay off in every future development cycle.

---

## Alternative Considered: Keep Single File

**NOT RECOMMENDED.**

While keeping a single file avoids migration effort, the downsides are significant:
- Navigation is already painful at 7,200 lines
- The file will continue to grow (slope detection alone added ~600+ lines)
- IDE performance may degrade with very large files
- Merge conflicts become more likely with multiple contributors

---

## Next Steps

If approved:
1. Create implementation plan with specific task breakdown
2. Implement changes following TDD workflow (ensure tests pass before and after)
3. Update documentation as needed
4. Review and merge

---

## Appendix: Full Test Function List by Proposed File

### test_ffb_core_physics.cpp
```
test_base_force_modes
test_grip_modulation
test_min_force
test_zero_input
test_grip_low_speed
test_gain_compensation
test_high_gain_stability
test_stress_stability
test_smoothing_step_response
test_time_corrected_smoothing
```

### test_ffb_slip_grip.cpp
```
test_sop_effect
test_combined_grip_loss
test_optimal_slip_buffer_zone
test_progressive_loss_curve
test_grip_floor_clamp
test_grip_threshold_sensitivity
```

### test_ffb_understeer.cpp
```
test_understeer_output_clamp
test_understeer_range_validation
test_understeer_effect_scaling
test_preset_understeer_only_isolation
```

### test_ffb_slope_detection.cpp
```
test_slope_detection_buffer_init
test_slope_sg_derivative
test_slope_grip_at_peak
test_slope_grip_past_peak
test_slope_vs_static_comparison
test_slope_config_persistence
test_slope_latency_characteristics
test_slope_noise_rejection
test_slope_buffer_reset_on_toggle
test_slope_detection_no_boost_when_grip_balanced
test_slope_detection_no_boost_during_oversteer
test_lat_g_boost_works_without_slope_detection
test_slope_detection_default_values_v071
test_slope_current_in_snapshot
test_slope_detection_less_aggressive_v071
test_slope_decay_on_straight
test_slope_alpha_threshold_configurable
test_slope_confidence_gate
test_slope_stability_config_persistence
test_slope_no_understeer_on_straight_v073
test_slope_decay_rate_boundaries
test_slope_alpha_threshold_validation
```

### test_ffb_texture.cpp
```
test_road_texture_teleport
test_scrub_drag_fade
test_lockup_pitch_scaling
test_progressive_lockup
test_abs_frequency_scaling
test_abs_pulse_v060
test_notch_filter_attenuation
test_notch_filter_bandwidth
test_notch_filter_edge_cases
test_static_notch_integration
test_frequency_estimator
test_predictive_lockup_v060
test_rear_lockup_differentiation
test_split_load_caps
test_dynamic_thresholds
test_universal_bottoming
```

### test_ffb_yaw_gyro.cpp
```
test_sop_yaw_kick
test_sop_yaw_kick_direction
test_yaw_kick_threshold
test_yaw_kick_edge_cases
test_yaw_kick_signal_conditioning
test_gyro_damping
test_gyro_stability
test_yaw_accel_smoothing
test_yaw_accel_convergence
test_chassis_inertia_smoothing_convergence
test_kinematic_load_braking
test_kinematic_load_cornering
test_rear_align_effect
test_rear_force_workaround
```

### test_ffb_coordinates.cpp
```
test_coordinate_sop_inversion
test_coordinate_rear_torque_inversion
test_coordinate_scrub_drag_direction
test_coordinate_debug_slip_angle_sign
test_coordinate_all_effects_alignment
test_regression_yaw_slide_feedback
test_regression_no_positive_feedback
test_regression_phase_explosion
test_regression_road_texture_toggle
test_regression_bottoming_switch
test_regression_rear_torque_lpf
```

### test_ffb_config.cpp
```
test_config_safety_clamping
test_config_defaults_v057
test_config_safety_validation_v057
test_legacy_config_migration
test_preset_initialization
test_all_presets_non_negative_speed_gate
test_snapshot_data_integrity
test_snapshot_data_v049
test_zero_effects_leakage
test_missing_telemetry_warnings
test_game_state_logic
test_refactor_abs_pulse
test_refactor_torque_drop
test_refactor_snapshot_sop
test_refactor_units
test_wheel_slip_ratio_helper
test_signal_conditioning_helper
test_unconditional_vert_accel_update
FFBEngineTestAccess::test_unit_sop_lateral
FFBEngineTestAccess::test_unit_gyro_damping
FFBEngineTestAccess::test_unit_abs_pulse
```

### test_ffb_speed_gate.cpp
```
test_stationary_gate
test_idle_smoothing
test_stationary_silence
test_driving_forces_restored
test_smoothstep_helper_function
test_smoothstep_vs_linear
test_smoothstep_edge_cases
test_speed_gate_uses_smoothstep
test_smoothstep_stationary_silence_preserved
```
