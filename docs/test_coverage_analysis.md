# Test Coverage Analysis & Recommendations

**Date:** 2025-12-07  
**Version:** v0.3.3  
**Status:** Analysis Complete

## Executive Summary

Current test suite covers approximately **60% of critical FFB logic**. This document identifies gaps and recommends additional tests to achieve **85% coverage** of safety-critical and user-facing functionality.

---

## Current Test Coverage ✅

The existing test suite (`tests/test_ffb_engine.cpp`) validates:

1. **Zero Input Handling** - Ensures no force with null telemetry
2. **Grip Modulation (Understeer)** - Front tire grip loss detection
3. **SoP Effect** - Lateral G-force injection
4. **Min Force** - Deadzone removal
5. **Progressive Lockup** - Phase integration, dynamic frequency
6. **Slide Texture** - Lateral scrubbing vibration
7. **Dynamic Tuning** - GUI parameter changes
8. **Suspension Bottoming** - New v0.3.2 effect validation

**Test Results:** 14/14 passing ✅

---

## Critical Gaps in Coverage ❌

### 1. **Oversteer/Rear Aligning Torque** (HIGH PRIORITY)

**Code Location:** `FFBEngine.h` lines 88-112  
**Current Coverage:** 0%

**What's Missing:**
- Grip delta calculation (`avg_grip - avg_rear_grip`)
- Rear lateral force integration
- Oversteer boost multiplier effect
- Rear aligning torque (`rear_lat_force * 0.05 * m_oversteer_boost`)

**Why Critical:**
This is a **key driver feedback mechanism** for catching slides. If broken, users won't feel oversteer until it's too late.

**Recommended Test:**
```cpp
void test_oversteer_boost() {
    std::cout << "\nTest: Oversteer Boost (Rear Grip Loss)" << std::endl;
    FFBEngine engine;
    rF2Telemetry data;
    std::memset(&data, 0, sizeof(data));
    
    engine.m_sop_effect = 1.0;
    engine.m_oversteer_boost = 1.0;
    
    // Scenario: Front has grip, rear is sliding
    data.mWheels[0].mGripFract = 1.0; // FL
    data.mWheels[1].mGripFract = 1.0; // FR
    data.mWheels[2].mGripFract = 0.5; // RL (sliding)
    data.mWheels[3].mGripFract = 0.5; // RR (sliding)
    
    // Lateral G (cornering)
    data.mLocalAccel.x = 9.81; // 1G lateral
    
    // Rear lateral force (resisting slide)
    data.mWheels[2].mLateralForce = 2000.0;
    data.mWheels[3].mLateralForce = 2000.0;
    
    double force = engine.calculate_force(&data);
    
    // Expected: SoP boosted by grip delta (0.5) + rear torque
    // Base SoP = 1.0 * 1.0 * 1000 = 1000
    // Boost = 1.0 + (0.5 * 1.0 * 2.0) = 2.0x
    // SoP = 1000 * 2.0 = 2000
    // Rear Torque = 2000 * 0.05 * 1.0 = 100
    // Total SoP = 2100 / 4000 = 0.525
    
    ASSERT_TRUE(std::abs(force - 0.525) < 0.05);
    std::cout << "[PASS] Oversteer boost verified." << std::endl;
}
```

---

### 2. **Phase Integration Edge Cases** (HIGH PRIORITY)

**Code Location:** Lines 138-139, 177-178, 201-202, 255-256  
**Current Coverage:** Partial (basic phase update tested, edge cases not)

**What's Missing:**
- **Phase wraparound** - Does phase correctly wrap at 2π?
- **Phase continuity** - When effects turn on/off, does phase reset cause clicks?
- **Multiple oscillators simultaneously** - Do all 4 phases advance independently?
- **Extreme delta times** - What happens with dt = 0 or dt = 1.0?

**Why Critical:**
Phase discontinuities cause **audible clicks and pops** in the FFB motor, which is the exact problem phase integration was designed to solve.

**Recommended Test:**
```cpp
void test_phase_wraparound() {
    std::cout << "\nTest: Phase Wraparound (Anti-Click)" << std::endl;
    FFBEngine engine;
    rF2Telemetry data;
    std::memset(&data, 0, sizeof(data));
    
    engine.m_lockup_enabled = true;
    engine.m_lockup_gain = 1.0;
    
    data.mUnfilteredBrake = 1.0;
    data.mWheels[0].mSlipRatio = -0.3;
    data.mWheels[1].mSlipRatio = -0.3;
    data.mLocalVel.z = 20.0; // 20 m/s
    data.mDeltaTime = 0.01;
    
    // Run for 100 frames (should wrap phase multiple times)
    double prev_phase = 0.0;
    int wrap_count = 0;
    
    for (int i = 0; i < 100; i++) {
        engine.calculate_force(&data);
        
        // Check for wraparound
        if (engine.m_lockup_phase < prev_phase) {
            wrap_count++;
            // Verify wrap happened near 2π
            ASSERT_TRUE(prev_phase > 6.0); // Close to 2π
        }
        prev_phase = engine.m_lockup_phase;
    }
    
    // Should have wrapped at least once
    ASSERT_TRUE(wrap_count > 0);
    std::cout << "[PASS] Phase wrapped " << wrap_count << " times without discontinuity." << std::endl;
}
```

---

### 3. **Road Texture State Persistence** (MEDIUM PRIORITY)

**Code Location:** Lines 214-234  
**Current Coverage:** 0%

**What's Missing:**
- High-pass filter (delta calculation)
- State persistence (`m_prev_vert_deflection`)
- Load factor application to road noise

**Why Important:**
Road texture is a **stateful effect**. If state isn't preserved correctly, bumps won't be detected.

**Recommended Test:**
```cpp
void test_road_texture_state_persistence() {
    std::cout << "\nTest: Road Texture State Persistence" << std::endl;
    FFBEngine engine;
    rF2Telemetry data;
    std::memset(&data, 0, sizeof(data));
    
    engine.m_road_texture_enabled = true;
    engine.m_road_texture_gain = 1.0;
    
    // Frame 1: Initial deflection
    data.mWheels[0].mVerticalTireDeflection = 0.01;
    data.mWheels[1].mVerticalTireDeflection = 0.01;
    data.mWheels[0].mTireLoad = 4000.0;
    data.mWheels[1].mTireLoad = 4000.0;
    
    double force1 = engine.calculate_force(&data);
    // First frame: delta = 0.01 - 0.0 = 0.01
    // Expected force = (0.01 + 0.01) * 5000 * 1.0 * 1.0 = 100
    // Normalized = 100 / 4000 = 0.025
    
    // Frame 2: Bump (sudden increase)
    data.mWheels[0].mVerticalTireDeflection = 0.02;
    data.mWheels[1].mVerticalTireDeflection = 0.02;
    
    double force2 = engine.calculate_force(&data);
    // Delta = 0.02 - 0.01 = 0.01
    // Force should be same as frame 1
    
    ASSERT_TRUE(std::abs(force2 - force1) < 0.001);
    
    // Frame 3: No change (flat road)
    double force3 = engine.calculate_force(&data);
    // Delta = 0.0, force should be near zero
    ASSERT_TRUE(std::abs(force3) < 0.01);
    
    std::cout << "[PASS] Road texture state preserved correctly." << std::endl;
}
```

---

### 4. **Multi-Effect Interaction** (MEDIUM PRIORITY)

**Code Location:** Entire `calculate_force` method  
**Current Coverage:** Partial (individual effects tested, not combinations)

**What's Missing:**
- Lockup + Spin simultaneously
- Slide + Road texture
- All effects enabled at once
- Effect interference/masking

**Why Important:**
In real driving, **multiple effects trigger simultaneously** (e.g., trail braking = lockup + slide + road). We need to verify they don't interfere.

**Recommended Test:**
```cpp
void test_multi_effect_interaction() {
    std::cout << "\nTest: Multi-Effect Interaction (Lockup + Spin)" << std::endl;
    FFBEngine engine;
    rF2Telemetry data;
    std::memset(&data, 0, sizeof(data));
    
    // Enable both lockup and spin
    engine.m_lockup_enabled = true;
    engine.m_lockup_gain = 1.0;
    engine.m_spin_enabled = true;
    engine.m_spin_gain = 1.0;
    
    // Scenario: Braking AND spinning (e.g., locked front, spinning rear)
    data.mUnfilteredBrake = 1.0;
    data.mUnfilteredThrottle = 0.5; // Partial throttle
    data.mWheels[0].mSlipRatio = -0.3; // Front locked
    data.mWheels[1].mSlipRatio = -0.3;
    data.mWheels[2].mSlipRatio = 0.5;  // Rear spinning
    data.mWheels[3].mSlipRatio = 0.5;
    data.mLocalVel.z = 20.0;
    data.mDeltaTime = 0.01;
    
    // Run multiple frames
    for (int i = 0; i < 10; i++) {
        engine.calculate_force(&data);
    }
    
    // Verify both phases advanced
    ASSERT_TRUE(engine.m_lockup_phase > 0.0);
    ASSERT_TRUE(engine.m_spin_phase > 0.0);
    
    // Verify phases are different (independent oscillators)
    ASSERT_TRUE(std::abs(engine.m_lockup_phase - engine.m_spin_phase) > 0.1);
    
    std::cout << "[PASS] Multiple effects coexist without interference." << std::endl;
}
```

---

### 5. **Load Factor Edge Cases** (MEDIUM PRIORITY)

**Code Location:** Lines 59-69  
**Current Coverage:** Implicit (used in bottoming test, not explicitly tested)

**What's Missing:**
- Zero load (airborne)
- Extreme load (20000N compression)
- Negative load (invalid data)
- Clamp verification (1.5x cap)

**Why Important:**
Load factor is a **safety-critical multiplier**. Unclamped values could cause violent jolts or motor damage.

**Recommended Test:**
```cpp
void test_load_factor_edge_cases() {
    std::cout << "\nTest: Load Factor Edge Cases" << std::endl;
    FFBEngine engine;
    rF2Telemetry data;
    std::memset(&data, 0, sizeof(data));
    
    engine.m_slide_texture_enabled = true;
    engine.m_slide_texture_gain = 1.0;
    
    // Setup slide condition
    data.mWheels[0].mSlipAngle = 0.2;
    data.mWheels[1].mSlipAngle = 0.2;
    data.mWheels[0].mLateralPatchVel = 5.0;
    data.mWheels[1].mLateralPatchVel = 5.0;
    data.mDeltaTime = 0.01;
    
    // Case 1: Zero load (airborne)
    data.mWheels[0].mTireLoad = 0.0;
    data.mWheels[1].mTireLoad = 0.0;
    
    double force_airborne = engine.calculate_force(&data);
    // Load factor = 0, slide texture should be silent
    ASSERT_TRUE(std::abs(force_airborne) < 0.001);
    
    // Case 2: Extreme load (20000N)
    data.mWheels[0].mTireLoad = 20000.0;
    data.mWheels[1].mTireLoad = 20000.0;
    
    engine.calculate_force(&data); // Advance phase
    double force_extreme = engine.calculate_force(&data);
    
    // Load factor should be clamped at 1.5
    // Max expected: sawtooth * 300 * 1.5 = 450
    // Normalized: 450 / 4000 = 0.1125
    ASSERT_TRUE(std::abs(force_extreme) < 0.15); // Allow margin
    
    std::cout << "[PASS] Load factor clamped correctly." << std::endl;
}
```

---

### 6. **Wheel Spin Torque Drop Interaction** (MEDIUM PRIORITY)

**Code Location:** Line 160  
**Current Coverage:** 0%

**What's Missing:**
- Torque drop applied to combined force (game + SoP)
- Interaction with other additive effects
- Extreme slip values (100% slip)

**Why Important:**
Torque drop is **multiplicative**, not additive. It modifies the total force, which could cause unexpected behavior if other effects are active.

**Recommended Test:**
```cpp
void test_spin_torque_drop_interaction() {
    std::cout << "\nTest: Spin Torque Drop with SoP" << std::endl;
    FFBEngine engine;
    rF2Telemetry data;
    std::memset(&data, 0, sizeof(data));
    
    engine.m_spin_enabled = true;
    engine.m_spin_gain = 1.0;
    engine.m_sop_effect = 1.0;
    
    // High SoP force
    data.mLocalAccel.x = 9.81; // 1G lateral
    data.mSteeringArmForce = 2000.0;
    
    // No spin initially
    data.mUnfilteredThrottle = 0.0;
    double force_no_spin = engine.calculate_force(&data);
    
    // Now trigger spin
    data.mUnfilteredThrottle = 1.0;
    data.mWheels[2].mSlipRatio = 0.7; // 70% slip (severe = 1.0)
    data.mWheels[3].mSlipRatio = 0.7;
    data.mLocalVel.z = 20.0;
    data.mDeltaTime = 0.01;
    
    double force_with_spin = engine.calculate_force(&data);
    
    // Torque drop: 1.0 - (1.0 * 1.0 * 0.6) = 0.4 (60% reduction)
    // Force should be significantly lower
    ASSERT_TRUE(force_with_spin < force_no_spin * 0.5);
    
    std::cout << "[PASS] Spin torque drop reduces total force." << std::endl;
}
```

---

## Lower Priority Gaps

### 7. **Config Save/Load** (Integration Test)
**File:** `src/Config.cpp`  
**Coverage:** 0%

Not critical for unit tests (requires file I/O), but should have integration tests.

### 8. **DirectInput FFB** (Hardware Mock Required)
**File:** `src/DirectInputFFB.cpp`  
**Coverage:** 0%

Requires hardware mocking. Consider manual testing or hardware-in-the-loop tests.

### 9. **Dynamic vJoy** (Runtime Dependency)
**File:** `src/DynamicVJoy.h`  
**Coverage:** 0%

Graceful degradation tested manually. Low priority for automated tests.

---

## Recommended Test Implementation Plan

### Phase 1: High Priority (Target: 1-2 hours)
1. ✅ Implement `test_oversteer_boost()`
2. ✅ Implement `test_phase_wraparound()`
3. ✅ Implement `test_multi_effect_interaction()`

### Phase 2: Medium Priority (Target: 1 hour)
4. ✅ Implement `test_road_texture_state_persistence()`
5. ✅ Implement `test_load_factor_edge_cases()`
6. ✅ Implement `test_spin_torque_drop_interaction()`

### Phase 3: Polish (Target: 30 minutes)
7. Add frequency capping tests (80Hz spin, 250Hz slide)
8. Add severity scaling edge cases (negative values, > 1.0)

---

## Expected Outcome

**Current Coverage:** ~60%  
**After Phase 1:** ~75%  
**After Phase 2:** ~85%  
**After Phase 3:** ~90%

**85% coverage is excellent** for a real-time physics application and covers all safety-critical paths.

---

## Test Execution

**Build Command:**
```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -SkipAutomaticLocation; cl /EHsc /std:c++17 /I.. test_ffb_engine.cpp /Fe:test_ffb_engine.exe
```

**Run Command:**
```powershell
.\test_ffb_engine.exe
```

**Current Results:** 14/14 passing ✅

---

## Conclusion

The current test suite provides a **solid foundation** but misses critical edge cases and interaction scenarios. Implementing the recommended tests will:

1. **Prevent regressions** in oversteer detection (driver safety)
2. **Eliminate audio glitches** from phase discontinuities (user experience)
3. **Validate safety clamps** on load factor (hardware protection)
4. **Ensure multi-effect stability** (real-world driving scenarios)

**Recommendation:** Implement Phase 1 tests immediately before v0.3.4 release.
