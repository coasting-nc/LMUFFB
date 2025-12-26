# Rear Load Approximation Note (v0.4.39)

**Date**: 2025-12-20  
**Status**: Low Priority Enhancement  
**Severity**: Minor

---

## Issue Description

The Rear Aligning Torque calculation (lines 738-743 in `FFBEngine.h`) uses `approximate_rear_load()` which relies on `mSuspForce`:

```cpp
double calc_load_rl = approximate_rear_load(data->mWheel[2]);
double calc_load_rr = approximate_rear_load(data->mWheel[3]);
double avg_rear_load = (calc_load_rl + calc_load_rr) / 2.0;
```

The `approximate_rear_load()` function is defined as:

```cpp
double approximate_rear_load(const TelemWheelV01& w) {
    return w.mSuspForce + 300.0;
}
```

### Potential Problem

If `mSuspForce` is **also blocked** for rear wheels on encrypted content (similar to how `mTireLoad` is blocked), this approximation will return only 300N (unsprung mass), making the Rear Aligning Torque effect very weak.

---

## Current Status

### Why This is Low Priority

1. **Empirical Evidence**: Testing on encrypted LMU content (Hypercars, DLC) shows that `mSuspForce` is **typically available** even when `mTireLoad` is blocked.

2. **Front Axle is Critical**: The front axle load calculation (lines 585-598) already has the Kinematic Model fallback. Since front load is used for:
   - Grip calculation
   - Understeer effect
   - Slide texture amplitude
   
   The most critical FFB components are already protected.

3. **Rear Torque is Supplementary**: The Rear Aligning Torque is a **supplementary cue** for oversteer feel. Even if it's weak, the primary FFB (base force, SoP, understeer) will still function correctly.

### Observed Behavior

On encrypted content tested so far:
- `mTireLoad` = 0.0 (blocked) ❌
- `mSuspForce` = Valid (not blocked) ✓

This suggests the game engine blocks tire sensors but leaves suspension sensors active, likely because suspension data is needed for visual suspension animation.

---

## Potential Solution (Future Enhancement)

If empirical testing reveals that `mSuspForce` is **also blocked** for rear wheels on some encrypted content, the fix would be straightforward:

### Proposed Code Change

```cpp
// Step 1: Calculate Rear Loads
// Check if SuspForce is blocked (similar to front axle logic)
double calc_load_rl, calc_load_rr;

if (data->mWheel[2].mSuspForce > MIN_VALID_SUSP_FORCE) {
    // SuspForce available -> Use existing approximation
    calc_load_rl = approximate_rear_load(data->mWheel[2]);
    calc_load_rr = approximate_rear_load(data->mWheel[3]);
} else {
    // SuspForce blocked -> Use Kinematic Model
    calc_load_rl = calculate_kinematic_load(data, 2); // Rear Left
    calc_load_rr = calculate_kinematic_load(data, 3); // Rear Right
}

double avg_rear_load = (calc_load_rl + calc_load_rr) / 2.0;
```

### Estimated Effort

- **Code Changes**: 5-10 lines
- **Testing**: Verify on encrypted content
- **Risk**: Very low (same pattern as front axle)

---

## Recommendation

**Action**: Monitor user feedback from encrypted content (LMU Hypercars, DLC).

**Trigger for Implementation**:
- User reports weak rear-end feel on encrypted cars
- Telemetry logs show `mSuspForce ≈ 0.0` for rear wheels
- Rear Aligning Torque effect is significantly weaker than on non-encrypted cars

**Priority**: Low (implement in v0.4.40 or later if needed)

---

## Testing Strategy

If implementing this enhancement:

1. **Add Test Case**:
   ```cpp
   void test_rear_load_kinematic_fallback() {
       // Setup: Block both mTireLoad AND mSuspForce for rear wheels
       data.mWheel[2].mTireLoad = 0.0;
       data.mWheel[2].mSuspForce = 0.0;
       data.mWheel[3].mTireLoad = 0.0;
       data.mWheel[3].mSuspForce = 0.0;
       
       // Verify rear load uses Kinematic Model
       // Verify Rear Aligning Torque is non-zero
   }
   ```

2. **Manual Testing**:
   - Load encrypted LMU content (Hypercar)
   - Induce oversteer
   - Verify counter-steering cue is present
   - Compare feel to non-encrypted car

---

## Related Files

- **Implementation**: `FFBEngine.h` lines 738-743
- **Kinematic Model**: `FFBEngine.h` lines 448-479
- **Front Axle Logic**: `FFBEngine.h` lines 585-598
- **Code Review**: `docs/dev_docs/code_reviews/code_review_v0.4.39.md`

---

**Document Version**: 1.0  
**Last Updated**: 2025-12-20  
**Status**: Monitoring
