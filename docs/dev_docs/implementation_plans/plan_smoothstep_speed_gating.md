# Implementation Plan: Smoothstep Speed Gating

**Status: COMPLETED (2026-02-03) - Version 0.7.2**


## Context

This plan describes the implementation of **Smoothstep Speed Gating**, a signal processing improvement adopted from Marvin's AIRA. The change replaces the current linear speed gate interpolation with a smooth S-curve (Hermite interpolation) for more natural FFB transitions at low speeds.

### Problem Statement

The existing speed gate uses linear interpolation:
```cpp
ctx.speed_gate = (car_speed - lower) / (upper - lower);
```

This creates:
- Perceivable "kink" at the lower threshold (sudden start)
- Another kink at the upper threshold (abrupt full engagement)
- Users report jarring FFB changes when accelerating from pit lane

### Proposed Solution

Apply the **smoothstep** function `t² × (3 - 2t)` to create an S-curve transition with zero derivative at both endpoints.

---

## Reference Documents

1. **Source Implementation:**
   - `docs/dev_docs/tech_from_other_apps/Marvin's AIRA Refactored FFB_Effects_Technical_Report.md` - Section "Smoothstep Speed Gating"

2. **Comparison Report:**
   - `docs/dev_docs/investigations/aira_vs_lmuffb_comparison_report.md`

---

## Codebase Analysis Summary

### Current Implementation

**Location:** `FFBEngine.h` lines 1036-1040

```cpp
// Current Implementation (Linear)
double speed_gate_range = (double)m_speed_gate_upper - (double)m_speed_gate_lower;
if (speed_gate_range < 0.1) speed_gate_range = 0.1;
ctx.speed_gate = (ctx.car_speed - (double)m_speed_gate_lower) / speed_gate_range;
ctx.speed_gate = (std::max)(0.0, (std::min)(1.0, ctx.speed_gate));
```

### Setting Declarations

| Setting | FFBEngine.h | Config.h Preset | Default |
|---------|-------------|-----------------|---------|
| `m_speed_gate_lower` | Line 306 | Line 96 | 1.0 m/s (3.6 km/h) |
| `m_speed_gate_upper` | Line 307 | Line 97 | 5.0 m/s (18 km/h) |

**No new settings required.** This change only modifies the interpolation curve.

---

## FFB Effects Impact Analysis

### Effects Using `ctx.speed_gate`

| Effect | Location | How Gate is Applied | User Experience Change |
|--------|----------|---------------------|------------------------|
| **Understeer** | FFBEngine.h:1075 | `output_force *= ctx.speed_gate` | Smoother FFB fade-in when leaving pits |
| **SoP Lateral** | FFBEngine.h:1347 | `ctx.sop_base_force *= ctx.speed_gate` | More natural lateral feeling onset |
| **Rear Torque** | FFBEngine.h:1348 | `ctx.rear_torque *= ctx.speed_gate` | Gradual rear-end perception activation |
| **Yaw Kick** | FFBEngine.h:1349 | `ctx.yaw_force *= ctx.speed_gate` | Gentler oversteer snap-back at low speeds |
| **ABS Pulse** | FFBEngine.h:1382 | `* ctx.speed_gate` | Smoother ABS feedback onset |
| **Lockup Vibration** | FFBEngine.h:1457 | `* ctx.speed_gate` | Less jarring brake lockup feel |
| **Road Texture** | FFBEngine.h:1541 | `ctx.road_noise *= ctx.speed_gate` | Gradual road feel onset |
| **Bottoming** | FFBEngine.h:1579 | `* ctx.speed_gate` | Smoother curb/bump engagement |

### Expected FFB Feel Changes

| Speed Range | Current (Linear) | New (Smoothstep) |
|-------------|------------------|------------------|
| 0 - 1 m/s | 0% (flat) | 0% (flat) |
| 1 - 2 m/s | 25% | 15.6% (gentler start) |
| 2 - 3 m/s | 50% | 50% (same) |
| 3 - 4 m/s | 75% | 84.4% (earlier full) |
| 4 - 5 m/s | 100% | 100% |
| 5+ m/s | 100% | 100% |

**Key Difference:** The S-curve has zero slope at both endpoints, eliminating the perceivable "kink" when effects activate or reach full strength.

---

## Proposed Changes

### Change 1: Add Smoothstep Helper Function

**File:** `FFBEngine.h`  
**Location:** After line 790 (helper functions section, near `calculate_sg_derivative`)

```cpp
// Helper: Smoothstep interpolation - v0.7.2
// Returns smooth S-curve interpolation from 0 to 1
// Uses Hermite polynomial: t² × (3 - 2t)
// Zero derivative at both endpoints for seamless transitions
inline double smoothstep(double edge0, double edge1, double x) {
    double range = edge1 - edge0;
    if (range < 0.0001) return (x < edge0) ? 0.0 : 1.0;
    
    double t = (x - edge0) / range;
    t = (std::max)(0.0, (std::min)(1.0, t));
    return t * t * (3.0 - 2.0 * t);
}
```

### Change 2: Modify Speed Gate Calculation

**File:** `FFBEngine.h`  
**Location:** Lines 1036-1040 (inside `calculate_force()`)

```cpp
// BEFORE: Linear interpolation
double speed_gate_range = (double)m_speed_gate_upper - (double)m_speed_gate_lower;
if (speed_gate_range < 0.1) speed_gate_range = 0.1;
ctx.speed_gate = (ctx.car_speed - (double)m_speed_gate_lower) / speed_gate_range;
ctx.speed_gate = (std::max)(0.0, (std::min)(1.0, ctx.speed_gate));

// AFTER: Smoothstep interpolation
ctx.speed_gate = smoothstep(
    (double)m_speed_gate_lower, 
    (double)m_speed_gate_upper, 
    ctx.car_speed
);
```

---

## Test Plan (TDD-Ready)

### Test 1: `test_smoothstep_helper_function`

**Purpose:** Verify smoothstep returns mathematically correct values

**Test Script:**
```cpp
static void test_smoothstep_helper_function() {
    std::cout << "\nTest: Smoothstep Helper Function (v0.7.2)" << std::endl;
    FFBEngine engine;
    
    // At lower edge: t=0 → result=0
    double at_lower = engine.smoothstep(1.0, 5.0, 1.0);
    ASSERT_NEAR(at_lower, 0.0, 0.001);
    
    // At upper edge: t=1 → result=1
    double at_upper = engine.smoothstep(1.0, 5.0, 5.0);
    ASSERT_NEAR(at_upper, 1.0, 0.001);
    
    // At midpoint: t=0.5 → result=0.5 (symmetric)
    double at_mid = engine.smoothstep(1.0, 5.0, 3.0);
    ASSERT_NEAR(at_mid, 0.5, 0.001);
    
    // At 25%: t=0.25 → result=0.15625 (t²(3-2t) = 0.0625 * 2.5)
    double at_25 = engine.smoothstep(1.0, 5.0, 2.0);
    ASSERT_NEAR(at_25, 0.15625, 0.001);
    
    // At 75%: t=0.75 → result=0.84375 (t²(3-2t) = 0.5625 * 1.5)
    double at_75 = engine.smoothstep(1.0, 5.0, 4.0);
    ASSERT_NEAR(at_75, 0.84375, 0.001);
}
```

### Test 2: `test_smoothstep_vs_linear`

**Purpose:** Verify smoothstep produces different values than linear at non-midpoint

**Test Script:**
```cpp
static void test_smoothstep_vs_linear() {
    std::cout << "\nTest: Smoothstep vs Linear Comparison (v0.7.2)" << std::endl;
    FFBEngine engine;
    
    // At t=0.25, linear=0.25, smoothstep=0.15625
    double smooth_25 = engine.smoothstep(1.0, 5.0, 2.0);
    ASSERT_TRUE(smooth_25 < 0.25);  // Below linear
    
    // At t=0.75, linear=0.75, smoothstep=0.84375
    double smooth_75 = engine.smoothstep(1.0, 5.0, 4.0);
    ASSERT_TRUE(smooth_75 > 0.75);  // Above linear
    
    // This asymmetry is the key benefit - faster fade-in at end, slower at start
    g_tests_passed++;
}
```

### Test 3: `test_smoothstep_edge_cases`

**Purpose:** Test boundary conditions

**Test Script:**
```cpp
static void test_smoothstep_edge_cases() {
    std::cout << "\nTest: Smoothstep Edge Cases (v0.7.2)" << std::endl;
    FFBEngine engine;
    
    // Below lower threshold → 0
    double below = engine.smoothstep(1.0, 5.0, 0.0);
    ASSERT_NEAR(below, 0.0, 0.001);
    
    // Above upper threshold → 1
    double above = engine.smoothstep(1.0, 5.0, 10.0);
    ASSERT_NEAR(above, 1.0, 0.001);
    
    // Negative speed (reverse) → 0
    double negative = engine.smoothstep(1.0, 5.0, -5.0);
    ASSERT_NEAR(negative, 0.0, 0.001);
    
    // Zero range (edge0 == edge1) → safe behavior
    double zero_range = engine.smoothstep(3.0, 3.0, 3.0);
    ASSERT_TRUE(zero_range == 0.0 || zero_range == 1.0);
    
    // Very small range → no crash
    double tiny_range = engine.smoothstep(1.0, 1.0001, 1.00005);
    ASSERT_TRUE(tiny_range >= 0.0 && tiny_range <= 1.0);
}
```

### Test 4: `test_speed_gate_uses_smoothstep`

**Purpose:** End-to-end verification that calculate_force applies smoothstep

**Multi-Frame Telemetry Script:**
| Speed (m/s) | Linear Gate | Smoothstep Gate | Expected Behavior |
|-------------|-------------|-----------------|-------------------|
| 0.0 | 0.0 | 0.0 | Identical |
| 2.0 | 0.25 | 0.156 | Smoothstep lower |
| 3.0 | 0.50 | 0.50 | Identical (midpoint) |
| 4.0 | 0.75 | 0.844 | Smoothstep higher |
| 5.0+ | 1.0 | 1.0 | Identical |

**Test Script:**
```cpp
static void test_speed_gate_uses_smoothstep() {
    std::cout << "\nTest: Speed Gate Uses Smoothstep (v0.7.2)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_speed_gate_lower = 1.0f;
    engine.m_speed_gate_upper = 5.0f;
    
    // Enable road texture as probe
    engine.m_road_texture_enabled = true;
    engine.m_road_texture_gain = 1.0f;
    engine.m_max_torque_ref = 20.0f;
    
    // Test at 25% speed (2.0 m/s)
    TelemInfoV01 data_25 = CreateBasicTestTelemetry(2.0);
    data_25.mWheel[0].mVerticalTireDeflection = 0.002;
    data_25.mWheel[1].mVerticalTireDeflection = 0.002;
    engine.calculate_force(&data_25);
    
    // Test at 50% speed (3.0 m/s) 
    TelemInfoV01 data_50 = CreateBasicTestTelemetry(3.0);
    data_50.mWheel[0].mVerticalTireDeflection = 0.002;
    data_50.mWheel[1].mVerticalTireDeflection = 0.002;
    
    // Reset deflection state
    engine.m_prev_vert_deflection[0] = 0.0;
    engine.m_prev_vert_deflection[1] = 0.0;
    double force_50 = engine.calculate_force(&data_50);
    
    // Reset for 25% test
    engine.m_prev_vert_deflection[0] = 0.0;
    engine.m_prev_vert_deflection[1] = 0.0;
    double force_25 = engine.calculate_force(&data_25);
    
    // At 25%, smoothstep=0.156, at 50%, smoothstep=0.5
    // Ratio should be approximately 0.156/0.5 = 0.3125
    // With linear it would be 0.25/0.5 = 0.5
    double ratio = std::abs(force_25 / force_50);
    ASSERT_TRUE(ratio < 0.4);  // Confirms curve is not linear
}
```

### Test 5: `test_smoothstep_stationary_silence_preserved`

**Purpose:** Regression test - ensure stationary silence still works

**Test Script:**
```cpp
static void test_smoothstep_stationary_silence_preserved() {
    std::cout << "\nTest: Smoothstep Stationary Silence (v0.7.2)" << std::endl;
    FFBEngine engine;
    InitializeEngine(engine);
    engine.m_speed_gate_lower = 1.0f;
    engine.m_speed_gate_upper = 5.0f;
    
    TelemInfoV01 data = CreateBasicTestTelemetry(0.0);
    data.mSteeringShaftTorque = 10.0;  // Heavy input
    data.mLocalAccel.x = 5.0;           // Lateral G
    
    double force = engine.calculate_force(&data);
    
    // Speed gate at 0 m/s should still be 0.0
    ASSERT_NEAR(force, 0.0, 0.001);
}
```

---

## Deliverables

### Code Changes

- [ ] Add `smoothstep()` helper function to FFBEngine.h
- [ ] Modify speed gate calculation in `calculate_force()` to use smoothstep

### Tests

- [ ] test_smoothstep_helper_function
- [ ] test_smoothstep_vs_linear
- [ ] test_smoothstep_edge_cases
- [ ] test_speed_gate_uses_smoothstep
- [ ] test_smoothstep_stationary_silence_preserved

### Documentation

- [ ] Update CHANGELOG.md with v0.7.2 entry
- [ ] Add note in user documentation about smoother low-speed FFB

---

## Implementation Notes

*This section should be filled in by the developer during implementation.*

### Unforeseen Issues
*   **None checked.** Implementation of the Hermite math and integration into the physics engine proceeded without valid technical errors.

### Plan Deviations
*   **Version Numbering:** The plan originally targeted version `0.8.0`. This was changed to `0.7.2` during the release phase to adhere to strict versioning protocols (smallest possible increment).
*   **Test Suite Count:** The final test count (575) differed from the initial estimate (580) due to a baseline miscalculation, though all 5 new tests were successfully implemented and verified.

### Challenges Encountered
*   **Test Count Verification:** There was a brief confusion regarding the total number of passing tests vs the expected count. This required manual verification of the logs to confirm that the new tests were indeed running and passing, rather than relying solely on the total count.

### Recommendations for Future Plans
*   **Baseline Referencing:** Future plans should specify "Baseline Count + N New Tests" rather than a hard number for total tests, as the baseline can shift due to other parallel work.

---

## Document History

| Version | Date | Author | Notes |
|---------|------|--------|-------|
| 1.0 | 2026-02-03 | Antigravity | Initial plan (split from combined plan) |
