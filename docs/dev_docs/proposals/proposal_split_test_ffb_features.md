# Proposal: Split `test_ffb_features.cpp` Into Focused Test Files

**Status:** In Progress
**Target Version:** 0.7.6+
**Date Created:** 2026-02-04
**Complexity:** Medium  
**Estimated Effort:** 4-6 hours

---

## Executive Summary

The `test_ffb_features.cpp` file currently contains **1,543 lines** and **31 test functions**, making it the largest test file in the suite after the v0.7.5 refactoring. This proposal outlines a strategy to split it into 4-5 focused test files based on functional groupings.

---

## Current State Analysis

### File Statistics

| Metric | Value |
|--------|-------|
| **Total Lines** | 1,543 |
| **Total Bytes** | 58,582 |
| **Test Functions** | 31 |
| **Average Lines/Test** | ~50 |
| **Current Category** | "Road Effects & Features" |

### Test Function Inventory

The file currently contains tests for the following features:

**Regression Tests (2):**
1. `test_regression_road_texture_toggle` - Road texture toggle spike fix
2. `test_regression_bottoming_switch` - Bottoming method switch spike fix

**Road Texture & Suspension (6):**
3. `test_road_texture_teleport` - Delta clamp for teleport scenarios
4. `test_suspension_bottoming` - Suspension bottoming effect
5. `test_road_texture_state_persistence` - State persistence
6. `test_universal_bottoming` - Both bottoming methods
7. `test_unconditional_vert_accel_update` - Vertical acceleration update
8. `test_scrub_drag_fade` - Scrub drag fade-in

**Speed Gating (4):**
9. `test_stationary_gate` - Speed gate at stationary
10. `test_idle_smoothing` - Automatic idle smoothing
11. `test_stationary_silence` - Base torque & SoP gating
12. `test_driving_forces_restored` - Forces restored when driving

**Lockup & Braking (4):**
13. `test_progressive_lockup` - Progressive lockup effect
14. `test_predictive_lockup_v060` - Predictive lockup (v0.6.0)
15. `test_abs_pulse_v060` - ABS pulse detection (v0.6.0)
16. `test_rear_lockup_differentiation` - Rear lockup fix

**Slide Texture (1):**
17. `test_slide_texture` - Front & rear slide texture

**Multi-Effect & Integration (3):**
18. `test_dynamic_tuning` - GUI simulation
19. `test_oversteer_boost` - Lateral G boost (slide)
20. `test_phase_wraparound` - Phase wraparound
21. `test_multi_effect_interaction` - Multi-effect interaction

**Load & Thresholds (2):**
22. `test_split_load_caps` - Split load caps
23. `test_dynamic_thresholds` - Dynamic thresholds

**Notch Filter (4):**
24. `test_notch_filter_attenuation` - Notch filter attenuation
25. `test_frequency_estimator` - Frequency estimator
26. `test_static_notch_integration` - Static notch integration
27. `test_notch_filter_bandwidth` - Notch filter bandwidth
28. `test_notch_filter_edge_cases` - Notch filter edge cases

**Refactoring Tests (3):**
29. `test_spin_torque_drop_interaction` - Spin torque drop
30. `test_refactor_abs_pulse` - ABS pulse refactor
31. `test_refactor_torque_drop` - Torque drop refactor

---

## Proposed Split Strategy

### Option A: 4-File Split (Recommended)

Split into 4 focused files based on functional domains:

#### File 1: `test_ffb_road_texture.cpp` (~400 lines)
**Purpose:** Road texture, suspension bottoming, and scrub drag effects

**Tests (8):**
- `test_regression_road_texture_toggle`
- `test_road_texture_teleport`
- `test_suspension_bottoming`
- `test_road_texture_state_persistence`
- `test_universal_bottoming`
- `test_unconditional_vert_accel_update`
- `test_scrub_drag_fade`
- `test_regression_bottoming_switch`

**Runner:** `void Run_RoadTexture()`

**Tags:** `[Texture]`, `[Physics]`, `[Regression]`

---

#### File 2: `test_ffb_lockup_braking.cpp` (~450 lines)
**Purpose:** Lockup, ABS, and braking-related haptic effects

**Tests (7):**
- `test_progressive_lockup`
- `test_predictive_lockup_v060`
- `test_abs_pulse_v060`
- `test_rear_lockup_differentiation`
- `test_refactor_abs_pulse`
- `test_refactor_torque_drop`
- `test_spin_torque_drop_interaction`

**Runner:** `void Run_LockupBraking()`

**Tags:** `[Texture]`, `[Physics]`, `[Regression]`

---

#### File 3: `test_ffb_notch_filter.cpp` (~350 lines)
**Purpose:** Notch filter system and frequency estimation

**Tests (4):**
- `test_notch_filter_attenuation`
- `test_frequency_estimator`
- `test_static_notch_integration`
- `test_notch_filter_bandwidth`
- `test_notch_filter_edge_cases`

**Runner:** `void Run_NotchFilter()`

**Tags:** `[Math]`, `[Smoothing]`, `[Edge]`

---

#### File 4: `test_ffb_integration.cpp` (~350 lines)
**Purpose:** Multi-effect interactions, speed gating, and integration tests

**Tests (12):**
- `test_stationary_gate`
- `test_idle_smoothing`
- `test_stationary_silence`
- `test_driving_forces_restored`
- `test_slide_texture`
- `test_dynamic_tuning`
- `test_oversteer_boost`
- `test_phase_wraparound`
- `test_multi_effect_interaction`
- `test_split_load_caps`
- `test_dynamic_thresholds`

**Runner:** `void Run_Integration()`

**Tags:** `[Integration]`, `[Physics]`, `[Config]`

---

### Option B: 5-File Split (More Granular)

Alternative approach with finer granularity:

1. **`test_ffb_road_texture.cpp`** (6 tests, ~300 lines) - Road texture only
2. **`test_ffb_suspension.cpp`** (4 tests, ~250 lines) - Suspension & bottoming
3. **`test_ffb_lockup_braking.cpp`** (7 tests, ~450 lines) - Same as Option A
4. **`test_ffb_notch_filter.cpp`** (5 tests, ~350 lines) - Same as Option A
5. **`test_ffb_integration.cpp`** (9 tests, ~300 lines) - Integration tests only

**Pros:** More focused files, easier to navigate  
**Cons:** More files to manage, potential over-fragmentation

---

## Recommended Approach: Option A (4-File Split)

### Rationale

1. **Balanced File Sizes:** 300-450 lines each (manageable)
2. **Clear Functional Boundaries:** Each file has a distinct purpose
3. **Logical Grouping:** Related tests stay together
4. **Not Too Fragmented:** Avoids excessive file count
5. **Maintainability:** Easy to find and modify tests

---

## Implementation Plan

### Phase 1: Preparation

1. **Capture Baseline:**
   ```powershell
   .\build\tests\Release\run_combined_tests.exe > baseline_pre_split.log
   ```
   - Record exact test count (currently 591)
   - Save test output for comparison

2. **Create Test Name Inventory:**
   - Extract all test function names from `test_ffb_features.cpp`
   - Create checklist to verify no tests are lost

### Phase 2: Create New Files

Create 4 new test files following the established pattern:

**Template for each file:**
```cpp
#include "test_ffb_common.h"

namespace FFBEngineTests {

static void test_example() {
    std::cout << "\nTest: Example [Texture][Physics]" << std::endl;
    // ... test implementation
}

// ... more tests

void Run_<Category>() {
    std::cout << "\n=== <Category> Tests ===" << std::endl;
    test_example();
    // ... call all tests
}

} // namespace FFBEngineTests
```

**Files to create:**
1. `tests/test_ffb_road_texture.cpp`
2. `tests/test_ffb_lockup_braking.cpp`
3. `tests/test_ffb_notch_filter.cpp`
4. `tests/test_ffb_integration.cpp`

### Phase 3: Migrate Tests

For each new file:

1. **Copy test functions** from `test_ffb_features.cpp`
2. **Verify completeness** - ensure entire function body is copied
3. **Add tags** to test output messages
4. **Create runner function** that calls all tests
5. **Build and verify** after each file

### Phase 4: Update Build System

**Update `tests/CMakeLists.txt`:**
```cmake
set(TEST_SOURCES 
    main_test_runner.cpp 
    test_ffb_common.cpp
    test_ffb_core_physics.cpp
    test_ffb_slope_detection.cpp
    test_ffb_understeer.cpp
    test_ffb_smoothstep.cpp
    test_ffb_yaw_gyro.cpp
    test_ffb_coordinates.cpp
    test_ffb_road_texture.cpp      # NEW
    test_ffb_lockup_braking.cpp    # NEW
    test_ffb_notch_filter.cpp      # NEW
    test_ffb_integration.cpp       # NEW
    test_ffb_config.cpp
    test_ffb_slip_grip.cpp
    test_ffb_internal.cpp
    # ... rest of files
)
```

**Update `tests/test_ffb_common.h`:**
```cpp
// Add new runner declarations
void Run_RoadTexture();
void Run_LockupBraking();
void Run_NotchFilter();
void Run_Integration();
```

**Update `tests/test_ffb_common.cpp`:**
```cpp
void Run() {
    std::cout << "\n--- FFBEngine Regression Suite ---" << std::endl;
    
    Run_CorePhysics();
    Run_SlopeDetection();
    Run_Understeer();
    Run_SpeedGate();
    Run_YawGyro();
    Run_Coordinates();
    Run_RoadTexture();        // NEW
    Run_LockupBraking();      // NEW
    Run_NotchFilter();        // NEW
    Run_Integration();        // NEW
    Run_Config();
    Run_SlipGrip();
    Run_Internal();
    
    // ... summary
}
```

### Phase 5: Delete Original File

After verification:
```powershell
# Verify test count matches
.\build\tests\Release\run_combined_tests.exe

# If 591 tests pass, delete original
Remove-Item tests\test_ffb_features.cpp
```

### Phase 6: Verification

**Build Test:**
```powershell
cmake --build build --config Release --clean-first
```

**Test Count Verification:**
```powershell
.\build\tests\Release\run_combined_tests.exe
# Expected: TOTAL PASSED : 591
```

**Comparison:**
```powershell
# Compare with baseline
diff baseline_pre_split.log current_output.log
```

---

## Version Increment

**Version Change:** Current → Current + 0.0.1

Example: If current is 0.7.5, increment to 0.7.6

**Justification:** Infrastructure change only, no FFB logic changes

**Files to Update:**
- `VERSION`
- `src/Version.h`
- `CHANGELOG_DEV.md`

---

## Deliverables

### Code Changes
- [x] Create `tests/test_ffb_road_texture.cpp` (Completed v0.7.7)
- [x] Create `tests/test_ffb_lockup_braking.cpp` (Completed v0.7.6)
- [ ] Create `tests/test_ffb_notch_filter.cpp`
- [ ] Create `tests/test_ffb_integration.cpp`
- [x] Update `tests/CMakeLists.txt` (Partial)
- [x] Update `tests/test_ffb_common.h` (Partial)
- [x] Update `tests/test_ffb_common.cpp` (Partial)
- [x] Delete `tests/test_ffb_features.cpp` (In Progress - Reduced size)
- [x] Update `VERSION` (v0.7.6)
- [x] Update `src/Version.h` (v0.7.6)

### Testing
- [ ] Verify build succeeds
- [ ] Verify 591 tests pass
- [ ] Verify no tests lost or duplicated

### Documentation
- [ ] Update `CHANGELOG_DEV.md`
- [ ] Create implementation notes in this document
- [ ] Move this proposal to `completed` folder when done

---

## Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Missing tests after split | Low | High | Compare test counts, use checklist |
| Build failures | Low | Medium | Incremental migration, frequent builds |
| Duplicate test names | Very Low | Low | Use `static` keyword, unique names |
| Test isolation issues | Very Low | Low | Shared infrastructure already proven |

---

## Benefits

### Immediate Benefits

1. **Improved Navigation:** Easier to find specific tests
2. **Faster Iteration:** Smaller files compile faster
3. **Better Organization:** Clear functional boundaries
4. **Easier Code Review:** Smaller diffs per category

### Long-Term Benefits

1. **Scalability:** Room for growth in each category
2. **Maintainability:** Focused files are easier to maintain
3. **Onboarding:** New developers can understand structure faster
4. **Testing Efficiency:** Tag-based filtering more effective

---

## Alternative: Keep As-Is

### When to Consider

Keep `test_ffb_features.cpp` as-is if:

1. **File is stable** - No new tests being added frequently
2. **Team preference** - Team prefers fewer files
3. **Low priority** - Other refactoring work is more important
4. **Tag system sufficient** - Tag filtering provides enough granularity

### Current Assessment

**Recommendation:** **Proceed with split**

**Reasoning:**
- File is 2.6x larger than average (1543 vs ~600 lines)
- Contains diverse functionality (not cohesive)
- Tag system would benefit from finer file granularity
- Follows pattern established in v0.7.5 refactoring

---

## Comparison with v0.7.5 Refactoring

### Similarities

- Pure refactoring (no logic changes)
- Test count preservation critical
- Incremental migration approach
- Same shared infrastructure pattern

### Differences

| Aspect | v0.7.5 | This Proposal |
|--------|--------|---------------|
| **Source File** | 7,263 lines | 1,543 lines |
| **Target Files** | 9 files | 4 files |
| **Complexity** | High | Medium |
| **Estimated Time** | 8-12 hours | 4-6 hours |
| **Risk Level** | Medium | Low |

**Advantage:** Smaller scope makes this refactoring lower risk and faster to execute.

---

## Success Criteria

### Must Have
- ✅ All 591 tests pass
- ✅ Build succeeds with no errors
- ✅ No test logic modified
- ✅ Test count matches baseline exactly

### Should Have
- ✅ File sizes balanced (300-450 lines each)
- ✅ Clear functional grouping
- ✅ Tags added to test output
- ✅ Documentation updated

### Nice to Have
- ✅ Improved test execution time (via tag filtering)
- ✅ Better code review experience
- ✅ Easier navigation in IDE

---

## Timeline Estimate

| Phase | Estimated Time |
|-------|----------------|
| Preparation | 30 minutes |
| Create new files | 2 hours |
| Migrate tests | 1.5 hours |
| Update build system | 30 minutes |
| Verification | 30 minutes |
| Documentation | 30 minutes |
| **Total** | **5-6 hours** |

**Note:** Assumes familiarity with codebase and v0.7.5 refactoring process.

---

## Future Considerations

### After This Split

Once `test_ffb_features.cpp` is split, the test suite structure will be:

**Core Test Files (14):**
1. `test_ffb_common.cpp` (shared infrastructure)
2. `test_ffb_core_physics.cpp` (622 lines)
3. `test_ffb_slip_grip.cpp` (827 lines)
4. `test_ffb_understeer.cpp` (249 lines)
5. `test_ffb_slope_detection.cpp` (637 lines)
6. `test_ffb_road_texture.cpp` (~400 lines) **NEW**
7. `test_ffb_lockup_braking.cpp` (~450 lines) **NEW**
8. `test_ffb_notch_filter.cpp` (~350 lines) **NEW**
9. `test_ffb_integration.cpp` (~350 lines) **NEW**
10. `test_ffb_yaw_gyro.cpp` (764 lines)
11. `test_ffb_coordinates.cpp` (635 lines)
12. `test_ffb_config.cpp` (443 lines)
13. `test_ffb_smoothstep.cpp` (131 lines)
14. `test_ffb_internal.cpp` (505 lines)

**Average File Size:** ~480 lines (down from ~540 with features.cpp)

**Largest File:** `test_ffb_slip_grip.cpp` (827 lines)

### Next Refactoring Candidate

If further splitting is desired, consider:
- **`test_ffb_slip_grip.cpp`** (827 lines) - Could split into grip calculation vs. kinematic load

---

## Appendix: Test Function Mapping

### Detailed Mapping to New Files

**test_ffb_road_texture.cpp:**
```
test_regression_road_texture_toggle()
test_road_texture_teleport()
test_suspension_bottoming()
test_road_texture_state_persistence()
test_universal_bottoming()
test_unconditional_vert_accel_update()
test_scrub_drag_fade()
test_regression_bottoming_switch()
```

**test_ffb_lockup_braking.cpp:**
```
test_progressive_lockup()
test_predictive_lockup_v060()
test_abs_pulse_v060()
test_rear_lockup_differentiation()
test_refactor_abs_pulse()
test_refactor_torque_drop()
test_spin_torque_drop_interaction()
```

**test_ffb_notch_filter.cpp:**
```
test_notch_filter_attenuation()
test_frequency_estimator()
test_static_notch_integration()
test_notch_filter_bandwidth()
test_notch_filter_edge_cases()
```

**test_ffb_integration.cpp:**
```
test_stationary_gate()
test_idle_smoothing()
test_stationary_silence()
test_driving_forces_restored()
test_slide_texture()
test_dynamic_tuning()
test_oversteer_boost()
test_phase_wraparound()
test_multi_effect_interaction()
test_split_load_caps()
test_dynamic_thresholds()
```

---

## Document History

| Version | Date | Author | Notes |
|---------|------|--------|-------|
| 1.0 | 2026-02-04 | Gemini | Initial proposal |

---

**Status:** Awaiting approval  
**Priority:** Low-Medium (infrastructure improvement)  
**Effort:** Medium (5-6 hours)  
**Risk:** Low (proven refactoring pattern)
