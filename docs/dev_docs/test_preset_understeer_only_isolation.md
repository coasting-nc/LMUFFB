# Regression Test: "Test: Understeer Only" Preset Isolation

**Test Name:** `test_preset_understeer_only_isolation()`  
**Version:** v0.6.31  
**File:** `tests/test_ffb_engine.cpp`  
**Purpose:** Verify that the "Test: Understeer Only" preset properly isolates the understeer effect

---

## Test Objective

This regression test ensures that the "Test: Understeer Only" preset maintains proper effect isolation. It verifies that:

1. **Primary effect is enabled** - Understeer effect is active with a valid value
2. **All other effects are disabled** - No contamination from SoP, oversteer, yaw kick, gyro, etc.
3. **All textures are disabled** - No lockup, ABS, slide, road, or spin textures
4. **Critical physics parameters are set** - Optimal slip thresholds, native physics mode, speed gate disabled

---

## What It Tests

### ✅ Primary Effect
- `understeer` is in valid range (0.0 - 2.0)

### ✅ Disabled Effects (6 checks)
- `sop = 0.0` - Seat of Pants effect disabled
- `oversteer_boost = 0.0` - Rear grip boost disabled
- `rear_align_effect = 0.0` - Rear aligning torque disabled
- `sop_yaw_gain = 0.0` - Yaw kick counter-steering disabled
- `gyro_gain = 0.0` - Gyroscopic damping disabled
- `scrub_drag_gain = 0.0` - Scrub drag disabled

### ✅ Disabled Textures (5 checks)
- `slide_enabled = false` - Slide texture disabled
- `road_enabled = false` - Road texture disabled
- `spin_enabled = false` - Spin texture disabled
- `lockup_enabled = false` - Lockup vibration disabled
- `abs_pulse_enabled = false` - ABS pulse disabled

### ✅ Physics Parameters (5 checks)
- `optimal_slip_angle = 0.10` - Correct threshold for grip loss
- `optimal_slip_ratio = 0.12` - Correct longitudinal slip threshold
- `base_force_mode = 0` - Native physics mode (required for understeer)
- `speed_gate_lower < 0.0` - Speed gate disabled
- `speed_gate_upper < 0.0` - Speed gate disabled

---

## Total Assertions

**17 assertions** covering all critical parameters for proper understeer isolation.

---

## Why This Test Is Important

### Problem It Prevents

Without this test, future changes to the preset system could accidentally:
1. Re-enable lockup or ABS effects (contaminating the understeer feel with vibrations)
2. Re-enable road texture (adding noise to the steering)
3. Change the optimal slip thresholds (breaking the grip calculation)
4. Enable the speed gate (affecting low-speed testing)
5. Change the base force mode (breaking the understeer effect entirely)

### Historical Context

The preset was originally created with only 7 explicit settings, relying on inherited defaults for ~50 other parameters. This caused:
- Lockup vibrations during braking (contaminated the understeer test)
- Road texture noise (made it hard to isolate understeer feel)
- ABS pulses (interfered with testing)

The v0.6.31 update fixed this by explicitly setting all parameters, and this test ensures they stay correct.

---

## Test Output

### Success:
```
Test: Preset 'Test: Understeer Only' Isolation (v0.6.31)
[PASS] p.understeer > 0.0f && p.understeer <= 2.0f
[PASS] p.sop approx 0.0f
[PASS] p.oversteer_boost approx 0.0f
[PASS] p.rear_align_effect approx 0.0f
[PASS] p.sop_yaw_gain approx 0.0f
[PASS] p.gyro_gain approx 0.0f
[PASS] p.scrub_drag_gain approx 0.0f
[PASS] p.slide_enabled == false
[PASS] p.road_enabled == false
[PASS] p.spin_enabled == false
[PASS] p.lockup_enabled == false
[PASS] p.abs_pulse_enabled == false
[PASS] p.optimal_slip_angle approx 0.10f
[PASS] p.optimal_slip_ratio approx 0.12f
[PASS] p.base_force_mode == 0
[PASS] p.speed_gate_lower < 0.0f
[PASS] p.speed_gate_upper < 0.0f
[PASS] 'Test: Understeer Only' preset properly isolates understeer effect
```

### Failure Example:
If lockup were accidentally re-enabled:
```
Test: Preset 'Test: Understeer Only' Isolation (v0.6.31)
[FAIL] p.lockup_enabled == false
```

---

## Integration

The test is automatically run as part of the FFB Engine test suite:

```cpp
// Understeer Effect Regression Tests (v0.6.28 / v0.6.31)
test_optimal_slip_buffer_zone();
test_progressive_loss_curve();
test_grip_floor_clamp();
test_understeer_output_clamp();
test_understeer_range_validation();
test_understeer_effect_scaling();
test_legacy_config_migration();
test_preset_understeer_only_isolation();  // ← NEW
```

---

## Maintenance

If new effects are added to the FFB system in the future, this test should be updated to verify they are also disabled in the "Test: Understeer Only" preset.

**Example:** If a new "Tire Squeal" effect is added:
```cpp
// Add to test:
ASSERT_TRUE(p.tire_squeal_enabled == false);  // Tire squeal disabled
```
