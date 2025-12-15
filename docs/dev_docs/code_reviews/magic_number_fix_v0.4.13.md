# Magic Number Fix Summary - v0.4.13

**Date:** 2025-12-14  
**Issue:** Magic number in synthetic mode deadzone check  
**Status:** ✅ **RESOLVED**

---

## Problem

The synthetic mode deadzone threshold (`0.5` Nm) was hardcoded as a magic number in `FFBEngine.h`:

```cpp
// BEFORE (Line 511)
if (std::abs(game_force) > 0.5) {
```

While documented in comments, this violated the codebase's practice of extracting physics constants to named constants for maintainability.

---

## Solution

Extracted the magic number to a named constant in the Physics Constants section of `FFBEngine.h`.

### Changes Made

**1. Added Constant Definition (Lines 272-277):**
```cpp
// Synthetic Mode Deadzone Threshold (v0.4.13)
// Prevents sign flickering at steering center when using Synthetic (Constant) base force mode.
// Value: 0.5 Nm - If abs(game_force) < threshold, base input is set to 0.0.
// This creates a small deadzone around center to avoid rapid direction changes
// when the steering shaft torque oscillates near zero.
static constexpr double SYNTHETIC_MODE_DEADZONE_NM = 0.5; // Nm
```

**2. Updated Code to Use Constant (Line 517):**
```cpp
// AFTER
if (std::abs(game_force) > SYNTHETIC_MODE_DEADZONE_NM) {
```

---

## Benefits

1. **Maintainability:** Single source of truth for the deadzone value
2. **Clarity:** Self-documenting code with descriptive constant name
3. **Consistency:** Follows established pattern for physics constants
4. **Documentation:** Comprehensive comment explains purpose and rationale

---

## Impact

- **Files Modified:** 1 (`FFBEngine.h`)
- **Lines Changed:** 2 (1 addition, 1 modification)
- **Breaking Changes:** None
- **Test Changes:** None required (tests reference behavior, not implementation)

---

## Verification

The constant is now properly defined alongside other physics constants:
- `MIN_SLIP_ANGLE_VELOCITY` (v0.4.9)
- `REAR_TIRE_STIFFNESS_COEFFICIENT` (v0.4.10)
- `MAX_REAR_LATERAL_FORCE` (v0.4.10)
- `REAR_ALIGN_TORQUE_COEFFICIENT` (v0.4.11)
- `SYNTHETIC_MODE_DEADZONE_NM` (v0.4.13) ✅ **NEW**

---

**Status:** ✅ **Complete - Ready for Commit**
