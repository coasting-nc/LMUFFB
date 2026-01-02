# Regression Test: Non-Negative Speed Gate Values in All Presets

**Test Name:** `test_all_presets_non_negative_speed_gate()`  
**Version:** v0.6.32  
**File:** `tests/test_ffb_engine.cpp`  
**Purpose:** Prevent negative speed gate values in presets that cause confusing GUI display

---

## Test Objective

This regression test ensures that **all presets** maintain valid, non-negative speed gate values. It prevents the issue where negative values (intended to "disable" the gate) are displayed as confusing negative km/h values in the GUI.

---

## What It Tests

For **every preset** in the system, the test verifies:

1. **`speed_gate_lower >= 0.0`** - Lower threshold is non-negative
2. **`speed_gate_upper >= 0.0`** - Upper threshold is non-negative
3. **`speed_gate_upper >= speed_gate_lower`** - Upper threshold is not less than lower (sanity check)

---

## Why This Test Is Important

### The Problem It Prevents

**Issue:** In v0.6.32, the "Test: Understeer Only" preset used negative speed gate values:
```cpp
.SetSpeedGate(-10.0f, -5.0f)  // ❌ WRONG
```

**Result in GUI:**
- **Mute Below**: -36.0 km/h (confusing!)
- **Full Above**: -18.0 km/h (confusing!)

The negative values were intended to "disable" the speed gate, but:
- Values are stored in **m/s** internally
- GUI converts to **km/h** by multiplying by 3.6
- Result: -10.0 m/s × 3.6 = **-36.0 km/h** ❌

### The Correct Approach

Use **0.0** for both thresholds to disable the speed gate:
```cpp
.SetSpeedGate(0.0f, 0.0f)  // ✅ CORRECT
```

**Result in GUI:**
- **Mute Below**: 0.0 km/h ✅
- **Full Above**: 0.0 km/h ✅

---

## Test Coverage

The test checks **all presets** including:
1. Default (T300)
2. T300
3. Test: Game Base FFB Only
4. Test: SoP Only
5. Test: Understeer Only
6. Test: Textures Only
7. Test: Rear Align Torque Only
8. Test: SoP Base Only
9. Test: Slide Texture Only
10. Any future presets added to the system

---

## Test Output

### Success (All Presets Valid):
```
Test: All Presets Have Non-Negative Speed Gate Values (v0.6.32)
[PASS] All 9 presets have valid non-negative speed gate values
```

### Failure (Negative Values Found):
```
Test: All Presets Have Non-Negative Speed Gate Values (v0.6.32)
[FAIL] Preset 'Test: Understeer Only' has negative speed_gate_lower: -10 m/s (-36 km/h)
[FAIL] Preset 'Test: Understeer Only' has negative speed_gate_upper: -5 m/s (-18 km/h)
[FAIL] One or more presets have invalid speed gate values
```

### Failure (Invalid Range):
```
Test: All Presets Have Non-Negative Speed Gate Values (v0.6.32)
[FAIL] Preset 'Custom Preset' has speed_gate_upper < speed_gate_lower: 1.0 < 5.0
[FAIL] One or more presets have invalid speed gate values
```

---

## Integration

The test runs automatically as part of the FFB Engine test suite:

```cpp
// Understeer Effect Regression Tests (v0.6.28 / v0.6.31 / v0.6.32)
test_optimal_slip_buffer_zone();
test_progressive_loss_curve();
test_grip_floor_clamp();
test_understeer_output_clamp();
test_understeer_range_validation();
test_understeer_effect_scaling();
test_legacy_config_migration();
test_preset_understeer_only_isolation();
test_all_presets_non_negative_speed_gate();  // ← NEW (v0.6.32)
```

---

## Maintenance

### When Adding New Presets

This test automatically validates **all presets** loaded by `Config::LoadPresets()`. When adding a new preset:

1. ✅ **Use non-negative values**: `.SetSpeedGate(0.0f, 5.0f)` or `.SetSpeedGate(0.0f, 0.0f)`
2. ❌ **Never use negative values**: `.SetSpeedGate(-10.0f, -5.0f)`
3. ✅ **Ensure upper >= lower**: The test will catch inverted ranges

### Speed Gate Semantics

- **`0.0, 0.0`**: Effectively disables the speed gate (no range)
- **`1.0, 5.0`**: Fades vibrations from 1.0 m/s (3.6 km/h) to 5.0 m/s (18 km/h)
- **Negative values**: ❌ **INVALID** - Will fail this test

---

## Historical Context

**v0.6.32 Bug Fix:**
- The "Test: Understeer Only" preset was using negative values to "disable" the speed gate
- This caused confusing negative km/h displays in the GUI
- Fixed by changing to `0.0, 0.0` and adding this regression test
- See: `docs/dev_docs/preset_review_understeer_only.md`

---

## Related Tests

- `test_preset_understeer_only_isolation()` - Verifies the specific "Test: Understeer Only" preset configuration
- `test_speed_gate_custom_thresholds()` - Verifies speed gate physics behavior with custom thresholds
