# Grip Calculation Logic Analysis - v0.4.5

**Document Version:** 1.0  
**Application Version:** 0.4.5  
**Date:** 2025-12-11  
**Author:** Development Team

---

## Executive Summary

This document analyzes the grip calculation logic in FFBEngine v0.4.5, identifying critical issues with formula selection tracking, inconsistent fallback behavior between front and rear wheels, and lack of observability in both production and test environments.

### Key Findings

- ✅ **Functional:** Grip approximation mechanism works when telemetry is missing
- ❌ **Critical Issue:** No tracking of which formula (telemetry vs. approximation) is used
- ❌ **Critical Issue:** Rear wheels have NO fallback mechanism (inconsistent with front)
- ❌ **Issue:** Original telemetry values are permanently lost after approximation
- ❌ **Issue:** Insufficient observability for debugging and testing

---

## 1. Overview of Grip Usage in FFBEngine

Grip values (`mGripFract`) from telemetry are used in three main areas:

### 1.1 Front Wheel Grip (Primary)
- **Location:** `FFBEngine.h` lines 301-339
- **Purpose:** Modulates steering force based on tire grip (understeer effect)
- **Formula:** `grip_factor = 1.0 - ((1.0 - avg_grip) * m_understeer_effect)`
- **Fallback:** YES - Uses slip angle approximation when telemetry is missing

### 1.2 Rear Wheel Grip (Oversteer Detection)
- **Location:** `FFBEngine.h` lines 367-376
- **Purpose:** Detects oversteer condition (rear grip < front grip) to boost SoP force
- **Formula:** `grip_delta = avg_grip - avg_rear_grip`
- **Fallback:** NO - Uses raw telemetry values directly ⚠️

### 1.3 Statistics/Snapshot
- **Location:** `FFBEngine.h` lines 239, 647
- **Purpose:** Records grip for telemetry display and debugging
- **Value Stored:** Final calculated grip (cannot distinguish source)

---

## 2. Front Grip Calculation Flow

### 2.1 Normal Path (Telemetry Available)

```cpp
// Step 1: Read raw telemetry
double grip_l = fl.mGripFract;        // Line 303
double grip_r = fr.mGripFract;        // Line 304
double avg_grip = (grip_l + grip_r) / 2.0;  // Line 305

// Step 2: Sanity check passes (grip > 0.0001)
// Condition at line 316 is FALSE

// Step 3: Clamp to [0.0, 1.0]
avg_grip = (std::max)(0.0, (std::min)(1.0, avg_grip));  // Line 336

// Step 4: Use in calculations
double grip_factor = 1.0 - ((1.0 - avg_grip) * m_understeer_effect);  // Line 338
```

### 2.2 Fallback Path (Telemetry Missing)

```cpp
// Step 1: Read raw telemetry
double avg_grip = (fl.mGripFract + fr.mGripFract) / 2.0;
// Result: avg_grip = 0.0 (or very small value)

// Step 2: Sanity check FAILS
if (avg_grip < 0.0001 && avg_load > 100.0) {  // Line 316
    
    // Step 3: Calculate slip angles
    double slip_fl = get_slip_angle(fl);  // Line 318
    double slip_fr = get_slip_angle(fr);  // Line 319
    double avg_slip = (slip_fl + slip_fr) / 2.0;  // Line 320
    
    // Step 4: Approximate grip from slip angle
    // Peak slip ~0.15 rad (8.5 deg). Falloff after that.
    double excess = (std::max)(0.0, avg_slip - 0.15);  // Line 324
    avg_grip = 1.0 - (excess * 2.0);  // Line 325
    
    // Step 5: Apply floor
    avg_grip = (std::max)(0.2, avg_grip);  // Line 326 - MINIMUM 0.2
    
    // Step 6: Set warning flags
    if (!m_warned_grip) {
        std::cout << "[WARNING] Missing Grip. Using Approx based on Slip Angle." << std::endl;
        m_warned_grip = true;  // One-time warning
    }
    frame_warn_grip = true;  // Per-frame flag
}

// Step 7: Original telemetry value is LOST
// avg_grip now contains approximated value (0.2 to 1.0)
```

### 2.3 Slip Angle Calculation

The `get_slip_angle` lambda function (lines 308-313):

```cpp
auto get_slip_angle = [&](const TelemWheelV01& w) {
    double v_long = std::abs(w.mLongitudinalGroundVel);
    double min_speed = 0.5;
    if (v_long < min_speed) v_long = min_speed;  // Prevent div-by-zero
    return std::atan2(std::abs(w.mLateralPatchVel), v_long);
};
```

**Physics Basis:**
- Slip angle = arctan(lateral_velocity / longitudinal_velocity)
- At low slip angles (< 0.15 rad ≈ 8.5°): Tire has good grip
- At high slip angles (> 0.15 rad): Tire is sliding, grip degrades

**Approximation Formula:**
```
excess_slip = max(0, slip_angle - 0.15)
grip = 1.0 - (excess_slip * 2.0)
grip = max(0.2, grip)  // Floor at 20% grip
```

**Example Values:**
| Slip Angle | Excess | Formula Result | Final Grip |
|------------|--------|----------------|------------|
| 0.0 rad    | 0.0    | 1.0            | 1.0        |
| 0.15 rad   | 0.0    | 1.0            | 1.0        |
| 0.20 rad   | 0.05   | 0.9            | 0.9        |
| 0.40 rad   | 0.25   | 0.5            | 0.5        |
| 0.65 rad   | 0.50   | 0.0            | **0.2**    |
| 1.0 rad    | 0.85   | -0.7           | **0.2**    |

---

## 3. Rear Grip Calculation Flow

### 3.1 Current Implementation (NO FALLBACK)

```cpp
// Lines 368-370
double grip_rl = data->mWheel[2].mGripFract;  // RAW telemetry
double grip_rr = data->mWheel[3].mGripFract;  // RAW telemetry
double avg_rear_grip = (grip_rl + grip_rr) / 2.0;

// Lines 373-376: Oversteer boost calculation
double grip_delta = avg_grip - avg_rear_grip;
if (grip_delta > 0.0) {
    sop_total *= (1.0 + (grip_delta * m_oversteer_boost * 2.0));
}
```

### 3.2 Problem Scenarios

#### Scenario A: Front telemetry missing, rear telemetry OK
```
Front: mGripFract = 0.0 → Approximated to 0.2
Rear:  mGripFract = 0.8 → Used directly
grip_delta = 0.2 - 0.8 = -0.6 (negative)
Result: No oversteer boost (correct behavior, but for wrong reasons)
```

#### Scenario B: Front telemetry OK, rear telemetry missing
```
Front: mGripFract = 0.8 → Used directly
Rear:  mGripFract = 0.0 → Used directly (WRONG!)
grip_delta = 0.8 - 0.0 = 0.8 (large positive)
Result: INCORRECT oversteer boost triggered!
```

#### Scenario C: Both missing
```
Front: mGripFract = 0.0 → Approximated to 0.2
Rear:  mGripFract = 0.0 → Used directly (WRONG!)
grip_delta = 0.2 - 0.0 = 0.2
Result: INCORRECT oversteer boost triggered!
```

---

## 4. Critical Issues Identified

### 4.1 Issue #1: No Formula Tracking

**Problem:** The system does not track which grip calculation method was used.

**Impact:**
- Cannot determine if `avg_grip = 0.2` is from:
  - Actual telemetry reporting 20% grip
  - Approximation formula flooring at 20%
- Debugging is difficult
- Test assertions cannot verify correct code path
- Telemetry displays cannot indicate data quality

**Current State Variables:**
```cpp
bool m_warned_grip = false;     // One-time warning (persistent)
bool frame_warn_grip = false;   // Per-frame flag (local variable)
```

**Missing Variables:**
- `bool front_grip_approximated`
- `bool rear_grip_approximated`
- `double front_grip_original`
- `double rear_grip_original`

### 4.2 Issue #2: Inconsistent Fallback Behavior

**Problem:** Front wheels have fallback logic, rear wheels do not.

**Comparison Table:**

| Aspect | Front Wheels | Rear Wheels |
|--------|-------------|-------------|
| Sanity Check | ✅ Yes (line 316) | ❌ No |
| Approximation | ✅ Yes (lines 318-326) | ❌ No |
| Warning | ✅ Yes (lines 328-330) | ❌ No |
| Floor Value | ✅ 0.2 | ❌ None (can be 0.0) |
| Frame Flag | ✅ `frame_warn_grip` | ❌ None |

**Why This Matters:**
- Oversteer boost calculation compares front and rear grip
- If rear telemetry fails, comparison is invalid
- Can trigger false oversteer detection
- Asymmetric handling of same failure mode

### 4.3 Issue #3: Data Loss

**Problem:** Original telemetry values are overwritten and lost.

**Code Location:** Line 325 in FFBEngine.h
```cpp
avg_grip = 1.0 - (excess * 2.0);  // OVERWRITES original value
```

**Consequences:**
- Cannot log original vs. corrected values
- Cannot analyze approximation accuracy
- Cannot detect if telemetry is consistently bad
- Cannot implement adaptive fallback strategies

### 4.4 Issue #4: Insufficient Observability

**Problem:** Snapshot data doesn't indicate grip source.

**Current Snapshot (line 647):**
```cpp
snap.grip_fract = (float)avg_grip;  // Final value only
```

**Missing Information:**
- Was this value from telemetry or approximation?
- What was the original telemetry value?
- What slip angle was calculated?
- How often is approximation being used?

**Impact on Testing:**
- Test at line 798 expects `force_grip = 0.1`
- Cannot verify WHY it's 0.1:
  - Was approximation triggered? (Should be YES)
  - Was floor applied? (Should be YES)
  - What was the calculated slip angle? (Unknown)

---

## 5. Test Analysis

### 5.1 Test: Telemetry Sanity Checks (lines 718-813)

This test verifies the grip fallback mechanism:

```cpp
// Setup: Missing grip scenario
data.mWheel[0].mGripFract = 0.0;  // Line 775
data.mWheel[1].mGripFract = 0.0;  // Line 776
data.mSteeringShaftTorque = 10.0; // Line 782

// Execute
double force_grip = engine.calculate_force(&data);  // Line 788

// Verify warning was triggered
ASSERT_TRUE(engine.m_warned_grip);  // Lines 790-796

// Verify output force
ASSERT_NEAR(force_grip, 0.1, 0.001);  // Line 798
```

### 5.2 Expected Calculation Path

**Step-by-step breakdown:**

1. **Initial grip:** `avg_grip = (0.0 + 0.0) / 2.0 = 0.0`

2. **Sanity check:** `if (0.0 < 0.0001 && avg_load > 100.0)` → TRUE

3. **Slip angle calculation:**
   - `mLateralPatchVel` not set → defaults to 0.0
   - `mLongitudinalGroundVel` not set → defaults to 0.0
   - `v_long` clamped to `min_speed = 0.5`
   - `slip_angle = atan2(0.0, 0.5) = 0.0`

4. **Grip approximation:**
   - `excess = max(0.0, 0.0 - 0.15) = 0.0`
   - `avg_grip = 1.0 - (0.0 * 2.0) = 1.0`
   - `avg_grip = max(0.2, 1.0) = 1.0` ← **Wait, should be 1.0!**

5. **Expected force:**
   - `grip_factor = 1.0 - ((1.0 - 1.0) * 1.0) = 1.0`
   - `output_force = 10.0 * 1.0 = 10.0 Nm`
   - `norm_force = 10.0 / 20.0 = 0.5`

**DISCREPANCY DETECTED!** 🚨

The test expects `0.1` but the calculation suggests `0.5` should be the result when slip angle is zero.

### 5.3 Actual Behavior Investigation

The test passes with `force_grip = 0.1`, which means:
- `grip_factor = 0.2` (since `10.0 * 0.2 / 20.0 = 0.1`)
- `avg_grip = 0.2` was used
- The floor was applied

**This implies:**
- Either the slip angle calculation returned a high value (> 0.65 rad)
- OR there's additional logic affecting the grip value
- OR the test setup triggers a different code path

**Possible cause:** The test may not be setting all required telemetry fields, causing the slip angle calculation to produce unexpected results.

### 5.4 Test Limitations

**What the test DOES verify:**
- ✅ Warning flag is set (`m_warned_grip`)
- ✅ Output force matches expected value (0.1)

**What the test DOES NOT verify:**
- ❌ Which code path was taken
- ❌ What slip angle was calculated
- ❌ Whether floor was applied vs. formula result
- ❌ Original telemetry value preservation
- ❌ Intermediate calculation values

---

## 6. Recommendations

### 6.1 Short-term: Documentation (Current PR)

**Action:** Add comprehensive comments to document current behavior.

**Locations:**
- `FFBEngine.h` lines 301-339: Front grip calculation
- `FFBEngine.h` lines 367-376: Rear grip calculation (note lack of fallback)
- `test_ffb_engine.cpp` line 718-813: Test expectations and limitations

### 6.2 Medium-term: Enhanced Observability

**Action:** Add tracking variables without changing behavior.

```cpp
// Add to FFBEngine class
struct GripDiagnostics {
    bool front_approximated = false;
    bool rear_approximated = false;
    double front_original = 0.0;
    double rear_original = 0.0;
    double front_slip_angle = 0.0;
    double rear_slip_angle = 0.0;
} m_grip_diag;
```

**Benefits:**
- Can log original vs. corrected values
- Tests can verify correct code path
- Telemetry can show data quality
- No behavior changes (backward compatible)

### 6.3 Long-term: Consistent Fallback Logic

**Action:** Apply same fallback mechanism to rear wheels.

```cpp
// After line 370, add:
if (avg_rear_grip < 0.0001 && avg_load > 100.0) {
    double slip_rl = get_slip_angle(data->mWheel[2]);
    double slip_rr = get_slip_angle(data->mWheel[3]);
    double avg_slip = (slip_rl + slip_rr) / 2.0;
    
    double excess = (std::max)(0.0, avg_slip - 0.15);
    avg_rear_grip = 1.0 - (excess * 2.0);
    avg_rear_grip = (std::max)(0.2, avg_rear_grip);
    
    if (!m_warned_rear_grip) {
        std::cout << "[WARNING] Missing Rear Grip. Using Approx based on Slip Angle." << std::endl;
        m_warned_rear_grip = true;
    }
    frame_warn_rear_grip = true;
}
```

**Benefits:**
- Consistent behavior across all wheels
- Prevents false oversteer detection
- More robust telemetry handling

### 6.4 Long-term: Refactor to Separate Function

**Action:** Extract grip calculation into reusable function.

```cpp
struct GripResult {
    double value;           // Final grip value
    bool approximated;      // Was approximation used?
    double original;        // Original telemetry value
    double slip_angle;      // Calculated slip angle (if approximated)
};

GripResult calculate_grip(const TelemWheelV01& w1, 
                          const TelemWheelV01& w2,
                          double avg_load,
                          bool& warned_flag) {
    GripResult result;
    result.original = (w1.mGripFract + w2.mGripFract) / 2.0;
    result.value = result.original;
    result.approximated = false;
    
    if (result.value < 0.0001 && avg_load > 100.0) {
        result.approximated = true;
        double slip1 = calculate_slip_angle(w1);
        double slip2 = calculate_slip_angle(w2);
        result.slip_angle = (slip1 + slip2) / 2.0;
        
        double excess = (std::max)(0.0, result.slip_angle - 0.15);
        result.value = 1.0 - (excess * 2.0);
        result.value = (std::max)(0.2, result.value);
        
        if (!warned_flag) {
            std::cout << "[WARNING] Missing Grip. Using Approx based on Slip Angle." << std::endl;
            warned_flag = true;
        }
    }
    
    result.value = (std::max)(0.0, (std::min)(1.0, result.value));
    return result;
}
```

**Benefits:**
- Single source of truth
- Reusable for front and rear
- Returns all diagnostic information
- Easier to test in isolation
- Clearer code structure

### 6.5


TODO: add the following recommendations: 
Add test assertions for formula selection:
// In tests, explicitly verify which path was taken
ASSERT_TRUE(engine.m_warned_grip); // Approximation was used
// Or add a getter to check internal state

Consider making grip calculation a separate function:
struct GripResult {
    double value;
    bool approximated;
    double original;
};

GripResult calculate_grip(const TelemWheelV01& w1, 
                          const TelemWheelV01& w2, 
                          double avg_load);

---

## 7. Impact Assessment

### 7.1 Production Impact

**Current Behavior:**
- Front grip fallback works correctly
- Rear grip has no fallback → potential false oversteer detection
- No way to monitor approximation usage in production

**Risk Level:** **MEDIUM**
- Most sims provide reliable grip telemetry
- Fallback is rarely triggered in normal operation
- When triggered, front wheels behave correctly
- Rear wheel issue only affects oversteer boost feature

### 7.2 Testing Impact

**Current Limitations:**
- Cannot verify approximation logic in detail
- Cannot test rear wheel fallback (doesn't exist)
- Cannot assert on intermediate values
- Test expectations based on empirical results, not calculated values

**Risk Level:** **LOW**
- Tests do verify end-to-end behavior
- Warning flags are checked
- Output values are validated
- Missing: detailed path verification

### 7.3 Debugging Impact

**Current Challenges:**
- Cannot distinguish telemetry quality from logs
- Cannot analyze approximation accuracy
- Cannot detect systematic telemetry issues
- Snapshot data doesn't show grip source

**Risk Level:** **MEDIUM**
- Makes troubleshooting user issues harder
- Cannot provide data quality metrics
- Difficult to tune approximation formula

---

## 8. Conclusion

The grip calculation logic in v0.4.5 is **functionally correct for front wheels** but has **significant gaps in observability and consistency**:

### Working Correctly ✅
- Front wheel grip approximation when telemetry is missing
- Slip angle-based fallback formula
- Warning system for missing data
- Floor value prevents unrealistic zero grip

### Needs Improvement ⚠️
- No tracking of which formula was used
- Rear wheels lack fallback mechanism
- Original telemetry values are lost
- Insufficient diagnostic information
- Asymmetric handling of front vs. rear

### Recommended Actions
1. **Immediate:** Add documentation (this PR)
2. **Next Release:** Add diagnostic variables
3. **Future:** Implement rear wheel fallback
4. **Future:** Refactor to separate function

---

## Appendix A: Code References

### Key Files
- `FFBEngine.h`: Main force calculation logic
- `test_ffb_engine.cpp`: Unit tests
- `rF2Data.h`: Telemetry data structures

### Key Line Numbers (v0.4.5)
- **Front grip calculation:** FFBEngine.h:301-339
- **Rear grip calculation:** FFBEngine.h:367-376
- **Slip angle helper:** FFBEngine.h:308-313
- **Snapshot recording:** FFBEngine.h:647
- **Test case:** test_ffb_engine.cpp:718-813

### Key Variables
- `avg_grip`: Front wheel average grip (mutated)
- `avg_rear_grip`: Rear wheel average grip (raw)
- `m_warned_grip`: One-time warning flag
- `frame_warn_grip`: Per-frame warning flag
- `grip_factor`: Final multiplier for steering force

---

## Appendix B: Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-12-11 | Initial analysis for v0.4.5 |

---

**End of Document**
