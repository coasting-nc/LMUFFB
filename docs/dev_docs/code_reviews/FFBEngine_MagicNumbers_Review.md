# Code Review: Magic Numbers Refactoring — FFBEngine.cpp / FFBEngine.h

**Version:** 0.7.107  
**Date:** 2026-02-28  
**Reviewer:** Antigravity (AI Code Review Agent)  
**Scope:** Staged changes in `src/FFBEngine.cpp` and `src/FFBEngine.h`  
**Objective:** Replace all readability-magic-numbers Clang-Tidy warnings with named `static constexpr` constants.

> [!NOTE]
> This document was updated after initial review to include (§6) the 11 additional tests added to `test_ffb_magic_numbers.cpp` and (§3.3–3.5) the three minor naming issues that were subsequently fixed in the codebase.

---

## 1. Summary

The refactoring successfully extracts **over 60 literal magic numbers** from `FFBEngine.cpp` into named `static constexpr` constants declared in the `private:` section of `FFBEngine` in `FFBEngine.h`. The strategy is sound, systematic, and results in a highly readable code body. All examined constant values correctly match their original magic number counterparts. The CI check (`readability-magic-numbers`) has been re-enabled in `build_commands.txt` and the GitHub Actions workflow.

**Overall verdict: ✅ APPROVED with minor notes**

---

## 2. Constant Values: Accuracy Check

Each new constant was cross-checked against the original value in the diff. The table below covers every constant introduced by this refactoring:

### 2.1 — Physics and Signal Processing Constants

| Constant | Value | Original Literal | Match? |
|---|---|---|---|
| `SAFETY_SLEW_NORMAL` | `1000.0f` | `1000.0` | ✅ |
| `SAFETY_SLEW_RESTRICTED` | `100.0f` | `100.0` | ✅ |
| `IDLE_SPEED_MIN_M_S` | `3.0f` | `3.0` | ✅ |
| `IDLE_BLEND_FACTOR` | `0.1f` | `0.1` | ✅ |
| `MIN_TAU_S` | `0.0001` | `0.0001` (×3 uses) | ✅ |
| `ALPHA_MIN` | `0.001` | `0.001` | ✅ |
| `ALPHA_MAX` | `1.0` | `1.0` | ✅ |
| `HPF_TIME_CONSTANT_S` | `0.1` | `0.1` | ✅ |
| `ZERO_CROSSING_EPSILON` | `0.05` | `0.05` | ✅ |
| `MIN_FREQ_PERIOD` | `0.005` | `0.005` | ✅ |
| `MAX_FREQ_PERIOD` | `1.0` | `1.0` | ✅ |
| `DUAL_DIVISOR` | `2.0` | `2.0` (many uses) | ✅ |
| `DEBUG_FREQ_SMOOTHING` | `0.9` | `0.9` (complement `0.1` derived as `1.0 - DEBUG_FREQ_SMOOTHING`) | ✅ |
| `UNIT_CM_TO_M` | `100.0` | `100.0` | ✅ |
| `RADIUS_FALLBACK_MIN_M` | `0.1` | `0.1` | ✅ |
| `RADIUS_FALLBACK_DEFAULT_M` | `0.33` | `0.33` | ✅ |
| `MIN_NOTCH_WIDTH_HZ` | `0.1` | `0.1` | ✅ |
| `GRAVITY_MS2` | `9.81` | `9.81` | ✅ |
| `EPSILON_DIV` | `1e-9` | `1e-9` | ✅ |
| `TORQUE_ROLL_AVG_TAU` | `1.0` | `1.0` | ✅ |
| `TORQUE_SPIKE_RATIO` | `3.0` | `3.0` | ✅ |
| `TORQUE_SPIKE_MIN_NM` | `15.0` | `15.0` | ✅ |
| `LAT_G_CLEAN_LIMIT` | `8.0` | `8.0` | ✅ |
| `TORQUE_SLEW_CLEAN_LIMIT` | `1000.0` | `1000.0` | ✅ |
| `SESSION_PEAK_DECAY_RATE` | `0.005f` | `0.005` | ✅ |
| `PEAK_TORQUE_FLOOR` | `1.0f` | `1.0` | ✅ |
| `PEAK_TORQUE_CEILING` | `100.0f` | `100.0` | ✅ |
| `STRUCT_MULT_SMOOTHING_TAU` | `0.25f` | `0.25` | ✅ |
| `DT_EPSILON` | `0.000001` | `0.000001` | ✅ |
| `DEFAULT_DT` | `0.0025` | `0.0025` | ✅ |
| `SPEED_EPSILON` | `1.0` | `1.0` | ✅ |
| `SPEED_HIGH_THRESHOLD` | `10.0` | `10.0` | ✅ |
| `G_FORCE_THRESHOLD` | `3.0` | `3.0` | ✅ |
| `MISSING_TELEMETRY_WARN_THRESHOLD` | `50` | `50` (×4 uses) | ✅ |
| `MISSING_LOAD_WARN_THRESHOLD` | `20` | `20` | ✅ |
| `VEHICLE_NAME_CHECK_IDX` | `10` | `10` | ✅ |
| `STR_MAX_64` | `63` | `63` | ✅ |
| `LOAD_SAFETY_FLOOR` | `3000.0` | `3000.0` | ✅ |
| `LOAD_DECAY_RATE` | `100.0` | `100.0` | ✅ |
| `COMPRESSION_KNEE_THRESHOLD` | `1.5` | `1.5` | ✅ |
| `COMPRESSION_KNEE_WIDTH` | `0.5` | `0.5` | ✅ |
| `COMPRESSION_RATIO` | `4.0` | `4.0` | ✅ |
| `TACTILE_EMA_TAU` | `0.1` | `0.1` | ✅ |
| `USER_CAP_MAX` | `10.0` | `10.0` | ✅ |
| `DYNAMIC_WEIGHT_MIN` | `0.5` | `0.5` | ✅ |
| `DYNAMIC_WEIGHT_MAX` | `2.0` | `2.0` | ✅ |
| `CLIPPING_THRESHOLD` | `0.99` | `0.99`/`0.99f` (2 uses) | ✅ |
| `FFB_EPSILON` | `0.0001` | `0.0001` | ✅ |
| `SOFT_LOCK_MUTE_THRESHOLD_NM` | `0.1` | `0.1` | ✅ |
| `DEBUG_BUFFER_CAP` | `100` | `100` | ✅ |

### 2.2 — SoP, Gyro, and Lateral Effect Constants

| Constant | Value | Original Literal | Match? |
|---|---|---|---|
| `G_LIMIT_5G` | `5.0` | implicit (was `49.05 / 9.81 = 5G`) | ✅ Corrected (see §3.1) |
| `SMOOTHNESS_LIMIT_0999` | `0.999` | `0.999` | ✅ |
| `SOP_SMOOTHING_MAX_TAU` | `0.1` | `0.1` | ✅ |
| `MIN_LFM_ALPHA` | `0.001` | `0.001` | ✅ |
| `OVERSTEER_BOOST_MULT` | `2.0` | `2.0` | ✅ |
| `MIN_YAW_KICK_SPEED_MS` | `5.0` | `5.0` | ✅ |
| `DEFAULT_STEERING_RANGE_RAD` | `9.4247` | `9.4247` | ✅ |

### 2.3 — Effect Synthesis Constants

| Constant | Value | Original Literal | Match? |
|---|---|---|---|
| `ABS_PULSE_MAGNITUDE_SCALER` | `2.0` | `2.0` | ✅ |
| `LOCKUP_ACCEL_MARGIN` | `2.0` | `2.0` | ✅ |
| `MIN_SLIP_WINDOW` | `0.01` | `0.01` | ✅ (renamed from `MIN_SLOPE_WINDOW_S`, see §3.5) |
| `LOW_PRESSURE_LOCKUP_THRESHOLD` | `0.1` | `0.1` | ✅ |
| `LOW_PRESSURE_LOCKUP_FIX` | `0.5` | `0.5` | ✅ |
| `LOCKUP_BASE_FREQ` | `10.0` | `10.0` | ✅ |
| `LOCKUP_FREQ_SPEED_MULT` | `1.5` | `1.5` | ✅ |
| `SPIN_THROTTLE_THRESHOLD` | `0.05` | `0.05` | ✅ |
| `SPIN_SLIP_THRESHOLD` | `0.2` | `0.2` | ✅ |
| `SPIN_SEVERITY_RANGE` | `0.5` | `0.5` | ✅ |
| `SPIN_TORQUE_DROP_FACTOR` | `0.6` | `0.6` | ✅ |
| `SPIN_BASE_FREQ` | `10.0` | `10.0` | ✅ |
| `SPIN_FREQ_SLIP_MULT` | `2.5` | `2.5` | ✅ |
| `SPIN_MAX_FREQ` | `80.0` | `80.0` | ✅ |
| `SLIDE_VEL_THRESHOLD` | `1.5` | `1.5` | ✅ |
| `SLIDE_BASE_FREQ` | `10.0` | `10.0` | ✅ |
| `SLIDE_FREQ_VEL_MULT` | `5.0` | `5.0` | ✅ |
| `SLIDE_MAX_FREQ` | `250.0` | `250.0` | ✅ |
| `SAWTOOTH_SCALE` | `2.0` | `2.0` | ✅ |
| `SAWTOOTH_OFFSET` | `1.0` | `1.0` | ✅ |
| `SCRUB_VEL_THRESHOLD` | `0.001` | `0.001` | ✅ |
| `SCRUB_FADE_RANGE` | `0.5` | `0.5` | ✅ |
| `DEFLECTION_DELTA_LIMIT` | `0.01` | `0.01` | ✅ |
| `DEFLECTION_ACTIVE_THRESHOLD` | `1e-6` | `0.000001` | ✅ |
| `ROAD_TEXTURE_SPEED_THRESHOLD` | `5.0` | `5.0` | ✅ |
| `DEFLECTION_NM_SCALE` | `50.0` | `50.0` | ✅ |
| `ACCEL_ROAD_TEXTURE_SCALE` | `0.05` | `0.05` | ✅ |
| `BOTTOMING_RH_THRESHOLD_M` | `0.002` | `0.002` | ✅ |
| `BOTTOMING_IMPULSE_THRESHOLD_N_S` | `100000.0` | `100000.0` | ✅ |
| `BOTTOMING_IMPULSE_RANGE_N_S` | `200000.0` | `200000.0` | ✅ |
| `BOTTOMING_LOAD_MULT` | `2.5` | `2.5` | ✅ |
| `BOTTOMING_INTENSITY_SCALE` | `0.05` | `0.05` | ✅ |
| `BOTTOMING_FREQ_HZ` | `50.0` | `50.0` | ✅ |
| `PERCENT_TO_DECIMAL` | `100.0` | `100.0` | ✅ |

### 2.4 — Default Values for Member Initialization

| Constant | Value | Original Member Initializer | Match? |
|---|---|---|---|
| `DEFAULT_CALC_DT` | `0.0025` | `0.0025` | ✅ |
| `DEFAULT_TEXTURE_LOAD_CAP` | `1.5f` | `1.5f` | ✅ |
| `DEFAULT_BRAKE_LOAD_CAP` | `1.5f` | `1.5f` | ✅ |
| `DEFAULT_LOCKUP_START_PCT` | `5.0f` | `5.0f` | ✅ |
| `DEFAULT_LOCKUP_FULL_PCT` | `15.0f` | `15.0f` | ✅ |
| `DEFAULT_LOCKUP_REAR_BOOST` | `1.5f` | `1.5f` | ✅ |
| `DEFAULT_LOCKUP_GAMMA` | `2.0f` | `2.0f` | ✅ |
| `DEFAULT_LOCKUP_PREDICTION_SENS` | `50.0f` | `50.0f` | ✅ |
| `DEFAULT_SOFT_LOCK_STIFFNESS` | `20.0f` | `20.0f` | ✅ |
| `DEFAULT_SOFT_LOCK_DAMPING` | `0.5f` | `0.5f` | ✅ |
| `DEFAULT_NOTCH_Q` | `2.0f` | `2.0f` | ✅ |
| `DEFAULT_STATIC_NOTCH_FREQ` | `11.0f` | `11.0f` | ✅ |
| `DEFAULT_STATIC_NOTCH_WIDTH` | `2.0f` | `2.0f` | ✅ |
| `DEFAULT_YAW_KICK_THRESHOLD` | `0.2f` | `0.2f` | ✅ |
| `DEFAULT_SPEED_GATE_UPPER_M_S` | `5.0f` | `5.0f` | ✅ |
| `DEFAULT_ROAD_FALLBACK_SCALE` | `0.05f` | `0.05f` | ✅ |
| `DEFAULT_SLOPE_SG_WINDOW` | `15` | `15` | ✅ |
| `DEFAULT_SLOPE_SENSITIVITY` | `0.5f` | `0.5f` | ✅ |
| `DEFAULT_SLOPE_SMOOTHING_TAU` | `0.04f` | `0.04f` | ✅ |
| `DEFAULT_SLOPE_ALPHA_THRESHOLD` | `0.02f` | `0.02f` | ✅ |
| `DEFAULT_SLOPE_DECAY_RATE` | `5.0f` | `5.0f` | ✅ |
| `DEFAULT_SLOPE_CONFIDENCE_MAX_RATE` | `0.10f` | `0.10f` | ✅ |
| `DEFAULT_SLOPE_MIN_THRESHOLD` | `-0.3f` | `-0.3f` | ✅ |
| `DEFAULT_SLOPE_MAX_THRESHOLD` | `-2.0f` | `-2.0f` | ✅ |
| `DEFAULT_SLOPE_G_SLEW_LIMIT` | `50.0f` | `50.0f` | ✅ |
| `DEFAULT_SLOPE_TORQUE_SENSITIVITY` | `0.5f` | `0.5f` | ✅ |
| `DEFAULT_APPROX_MASS_KG` | `1100.0f` | `1100.0f` | ✅ |
| `DEFAULT_APPROX_AERO_COEFF` | `2.0f` | `2.0f` | ✅ |
| `DEFAULT_APPROX_WEIGHT_BIAS` | `0.55f` | `0.55f` | ✅ |
| `DEFAULT_APPROX_ROLL_STIFFNESS` | `0.6f` | `0.6f` | ✅ |
| `DEFAULT_ABS_FREQ_HZ` | `20.0f` | `20.0f` | ✅ |
| `STR_BUF_64` | `64` | `64` | ✅ |
| `DEFAULT_AUTO_PEAK_LOAD` | `4500.0` | `4500.0` | ✅ |
| `DEFAULT_SESSION_PEAK_TORQUE` | `25.0` | `25.0` | ✅ |

---

## 3. Findings and Issues

### 3.1 ✅ Notable Improvement: G-Clamp Rewrite

**Location:** `calculate_sop_lateral()`, `FFBEngine.cpp` line ~726

**Original:**
```cpp
double raw_g = (std::max)(-49.05, (std::min)(49.05, data->mLocalAccel.x));
double lat_g = (raw_g / 9.81);
```

**Refactored:**
```cpp
double raw_g = (std::max)(-G_LIMIT_5G * GRAVITY_MS2, (std::min)(G_LIMIT_5G * GRAVITY_MS2, data->mLocalAccel.x));
double lat_g = (raw_g / GRAVITY_MS2);
```

**Verdict:** ✅ Excellent. The literal `49.05` was an implicit `5G * 9.81` product. By decomposing it into `G_LIMIT_5G * GRAVITY_MS2`, the intent is now self-documenting. This is the best kind of magic-number replacement.

---

### 3.2 ✅ Consistent Pattern: `DUAL_DIVISOR = 2.0`

**Location:** throughout `FFBEngine.cpp` (15+ uses)

`DUAL_DIVISOR` is used everywhere a two-element average is computed: `(a + b) / DUAL_DIVISOR`. The naming accurately describes the operation. All usages are consistent.

---

### 3.3 ✅ RESOLVED: `DT_EPSILON` Semantic Reuse

**Status: Fixed in follow-up commit.**

**Original issue:** `DT_EPSILON` (named for delta-time, in seconds) was also used to check near-zero deflection values (in meters) at lines 323 and 359.

**Fix applied:** Added `DEFLECTION_NEAR_ZERO_M = 1e-6` to the `FFBEngine` private constants section and replaced both off-domain usages:

```diff
-    if (avg_susp_def < DT_EPSILON && std::abs(data->mLocalVel.z) > SPEED_HIGH_THRESHOLD) {
+    if (avg_susp_def < DEFLECTION_NEAR_ZERO_M && std::abs(data->mLocalVel.z) > SPEED_HIGH_THRESHOLD) {

-    if (avg_vert_def < DT_EPSILON && std::abs(data->mLocalVel.z) > SPEED_HIGH_THRESHOLD) {
+    if (avg_vert_def < DEFLECTION_NEAR_ZERO_M && std::abs(data->mLocalVel.z) > SPEED_HIGH_THRESHOLD) {
```

`DT_EPSILON` now exclusively refers to time (used only at L216 for the `ctx.dt` sanity check). `DEFLECTION_NEAR_ZERO_M` correctly communicates the physical domain (meters of suspension/tire deflection).

---

### 3.4 ✅ RESOLVED: `SPEED_EPSILON` Used for Lateral Force Threshold

**Status: Fixed in follow-up commit.**

**Original issue:** `SPEED_EPSILON = 1.0` (a speed in m/s) was reused to check lateral force magnitude in Newtons at lines 335 and 347.

**Fix applied:** Added `MIN_VALID_LAT_FORCE_N = 1.0` to the `FFBEngine` private constants section:

```diff
-    if (avg_lat_force_front < SPEED_EPSILON && std::abs(data->mLocalAccel.x) > G_FORCE_THRESHOLD) {
+    if (avg_lat_force_front < MIN_VALID_LAT_FORCE_N && std::abs(data->mLocalAccel.x) > G_FORCE_THRESHOLD) {

-    if (avg_lat_force_rear < SPEED_EPSILON && std::abs(data->mLocalAccel.x) > G_FORCE_THRESHOLD) {
+    if (avg_lat_force_rear < MIN_VALID_LAT_FORCE_N && std::abs(data->mLocalAccel.x) > G_FORCE_THRESHOLD) {
```

`SPEED_EPSILON` now exclusively governs speed-domain hysteresis. `MIN_VALID_LAT_FORCE_N` makes the Newton unit explicit in the name.

---

### 3.5 ✅ RESOLVED: `MIN_SLOPE_WINDOW_S` Misleading Name

**Status: Fixed in follow-up commit.**

**Original issue:** `MIN_SLOPE_WINDOW_S` implied a time window in seconds, but was used as a minimum slip ratio denominator (dimensionless) in `calculate_lockup_vibration()`.

**Fix applied:** Renamed throughout (header + usage site):

```diff
-    static constexpr double MIN_SLOPE_WINDOW_S = 0.01;
+    static constexpr double MIN_SLIP_WINDOW = 0.01;

-    if (window < MIN_SLOPE_WINDOW_S) window = MIN_SLOPE_WINDOW_S;
+    if (window < MIN_SLIP_WINDOW) window = MIN_SLIP_WINDOW;
```

The new name is dimensionless and domain-correct: it guards the minimum width of the slip ratio interpolation window.

---

### 3.6 ⚠️ Minor Issue: Unused Constants

The following constants are declared in the `private:` section of `FFBEngine` but do not appear to be used in `FFBEngine.cpp`:

| Constant | Value | Note |
|---|---|---|
| `PEAK_TORQUE_DECAY` | `0.005f` | Possibly confused with `SESSION_PEAK_DECAY_RATE` (same value) |
| `GAIN_SMOOTHING_TAU` | `0.25f` | Possibly future use |
| `HALF_PERIOD_MULT` | `0.5` | May belong to `GripLoadEstimation.cpp` |
| `ACCEL_EPSILON` | `1.0` | Not referenced in `FFBEngine.cpp` |
| `PHASE_WRAP_LIMIT` | `50.0` | Phase wrapping not in this file |
| `GAIN_REDUCTION_MAX` | `50.0` | Not referenced in `FFBEngine.cpp` |

**Recommendation:** Verify if these are used in `GripLoadEstimation.cpp` or other files. If not, remove them to avoid clutter and potential `bugprone-unused-local-non-trivial-variable` warnings.

---

### 3.7 ⚠️ Minor Structure Risk: `STR_BUF_64` and `DEFAULT_CALC_DT` Duplicated

**Location:** `FFBEngine.h`

Both `STR_BUF_64 = 64` and `DEFAULT_CALC_DT = 0.0025` are declared at two scopes (file-scope and private class member) for a valid reason: `FFBCalculationContext` is defined before the `FFBEngine` class body and needs these values for its default member initializers.

**No behavioral bug** — both values are identical. **Recommendation:** Add an explanatory comment to both declarations documenting the intentional duplication.

---

### 3.8 ✅ Correct: One Remaining Literal `100.0f` in Snapshot

**Location:** `FFBEngine.cpp`, line 635 (not changed by this refactoring)

```cpp
snap.tire_radius = (float)fl.mStaticUndeflectedRadius / 100.0f;
```

`UNIT_CM_TO_M` exists as a `double` but this is in a `float` context. Left intentionally unchanged. Not a bug.

---

## 4. Naming Quality Assessment

Overall naming quality is **high**. Key observations:

| Category | Assessment |
|---|---|
| Physical units in names (`_M_S`, `_NM`, `_HZ`, `_N_S`) | ✅ Excellent — units are explicit |
| `DEFAULT_` prefix for member defaults | ✅ Clear distinction from algorithm constants |
| `MIN_` / `MAX_` prefixes | ✅ Consistently applied |
| `EPSILON` suffix | ✅ Appropriate for near-zero thresholds |
| `DUAL_DIVISOR` | ✅ Correct and concise |
| `PERCENT_TO_DECIMAL` | ✅ Self-explanatory |
| `SAWTOOTH_SCALE` / `SAWTOOTH_OFFSET` | ✅ Correctly describes waveform parameters |
| `COMPRESSION_KNEE_THRESHOLD` / `WIDTH` | ✅ Matches DSP/audio compression terminology |
| `BOTTOMING_IMPULSE_THRESHOLD_N_S` | ✅ Force-rate units are explicit |
| `DEFLECTION_NEAR_ZERO_M` *(new)* | ✅ Units explicit, semantic domain correct |
| `MIN_VALID_LAT_FORCE_N` *(new)* | ✅ Units explicit (Newtons) |
| `MIN_SLIP_WINDOW` *(renamed)* | ✅ Dimensionless, context-correct |

All three §3.3–3.5 naming concerns are now resolved.

---

## 5. Code Coverage Analysis

### 5.1 Current Coverage for `FFBEngine.cpp` (at initial review)

| Metric | Value |
|---|---|
| **Line Coverage** | **99.8%** (only L61 uncovered) |
| **Branch Coverage** | **81.8%** |
| **Function Coverage** | **100.0%** (14/14) |

### 5.2 Branch Gaps Relevant to the Refactoring

| Uncovered Branch (approx.) | Refactored Constant | Status after §6 |
|---|---|---|
| L216 | `DT_EPSILON` / `DEFAULT_DT` | ✅ Covered by `test_mn_invalid_delta_time_fallback` |
| L289-290 | `MISSING_LOAD_WARN_THRESHOLD` | ✅ Covered by two tests (susp-force and kinematic paths) |
| L298, L314, L326, L338 | `MISSING_TELEMETRY_WARN_THRESHOLD` | ✅ Covered by four tests (one each) |
| L430 | `MIN_SLIP_WINDOW` *(renamed)* | ✅ Exercised via lockup spin test |
| L438 | `ABS_PULSE_MAGNITUDE_SCALER` | ✅ Covered by `test_mn_abs_pulse_magnitude_scaler` |
| L926-930 | `SPIN_*` constants | ✅ Covered by `test_mn_spin_detection_torque_drop` |
| L996 | `SLIDE_VEL_THRESHOLD` | ✅ Covered by `test_mn_slide_texture_velocity_threshold` |
| L1043 | `BOTTOMING_FREQ_HZ` | ✅ Covered by `test_mn_bottoming_ride_height_threshold` |

---

## 6. Additional Tests Added: `test_ffb_magic_numbers.cpp`

**File:** `tests/test_ffb_magic_numbers.cpp`  
**Test count:** 11 test cases, all in categories `CorePhysics` and `Texture`  
**Result:** ✅ 374/374 test cases pass, 1505 assertions, 0 failures

These tests were written to directly exercise the 7 branch-coverage gaps identified in the review. Each test validates that the refactored constant has its correct value at runtime, not just at compile time.

### Test 1 — `DT_EPSILON` / `DEFAULT_DT` (L216)

**Test:** `test_mn_invalid_delta_time_fallback`  
**What it covers:** Passes `mDeltaTime = 0.0` to `calculate_force()`.  
**Assertions:**
- Output force is finite (no NaN/Inf from division by zero)
- `m_warned_dt` is set after first bad-dt call
- `m_warned_dt` is NOT re-set on subsequent valid-dt calls (one-shot latch)

### Tests 2a/2b — `MISSING_LOAD_WARN_THRESHOLD` (L289–290), both sub-branches

**Tests:** `test_mn_missing_load_fallback_susp_force`, `test_mn_missing_load_fallback_kinematic`  
**What they cover:** 25 consecutive frames with `mTireLoad = 0` at speed. Verifies both load-fallback paths:
- (a) `mSuspForce > 10N` → `approximate_load()` path
- (b) `mSuspForce = 0` → kinematic estimation path

**Assertions:** `m_warned_load` is set; output remains finite in both cases.

### Tests 3a–3d — `MISSING_TELEMETRY_WARN_THRESHOLD` (L298, L314, L326, L338)

**Tests:** `test_mn_missing_susp_force_warning`, `test_mn_missing_susp_deflection_warning`, `test_mn_missing_lat_force_front_warning`, `test_mn_missing_lat_force_rear_warning`  
**What they cover:** 55 consecutive frames each with one telemetry field zeroed.  
**Assertions:** Each corresponding `m_warned_*` flag is false before 50 frames and true after 55 frames.

### Test 4 — `BOTTOMING_RH_THRESHOLD_M` / `BOTTOMING_FREQ_HZ`

**Test:** `test_mn_bottoming_ride_height_threshold`  
**What it covers:** Method 0 (ride height) suspension bottoming.  
**Assertions:**
- `mRideHeight = 0.002` (exactly at threshold) → force ≈ 0 (not triggered)
- `mRideHeight = 0.001` (below threshold) → `|force| > 0.0001` (triggered)

This directly validates that `BOTTOMING_RH_THRESHOLD_M = 0.002` is the correct boundary.

### Test 5 — `SPIN_SLIP_THRESHOLD`, `SPIN_TORQUE_DROP_FACTOR`, `SPIN_SEVERITY_RANGE`

**Test:** `test_mn_spin_detection_torque_drop`  
**What it covers:** Rear wheels with `patch_vel / ground_vel = 0.6` (> threshold 0.2).  
**Computation verified:**
```
severity = (0.6 - 0.2) / 0.5 = 0.8
gain_reduction = 1.0 - (0.8 × 1.0 × 0.6) = 0.52
```
**Assertions:** `gain_reduction_factor ≈ 0.52 ± 0.01` (validates `SPIN_TORQUE_DROP_FACTOR = 0.6` numerically).

### Test 6 — `SLIDE_VEL_THRESHOLD` (1.5 m/s)

**Test:** `test_mn_slide_texture_velocity_threshold`  
**What it covers:** Two sub-cases called via `FFBEngineTestAccess::CallCalculateSlideTexture()`.  
**Assertions:**
- `mLateralPatchVel = 1.0` (< 1.5) → `slide_noise == 0`
- `mLateralPatchVel = 3.0` (> 1.5) → `m_slide_phase > 0` (effect activated)

### Test 7 — `ABS_PULSE_MAGNITUDE_SCALER` (2.0)

**Test:** `test_mn_abs_pulse_magnitude_scaler`  
**What it covers:** ABS pulse generation with `abs_gain = 1.0`, `speed_gate = 1.0`, phase near π/2.  
**Assertions:**
- Force is non-zero (ABS triggered)
- `|force| ≤ 2.001` (bounded by `abs_gain × ABS_PULSE_MAGNITUDE_SCALER`)
- `|force| > 1.70` (near-peak, confirming the scaler produces 2× not less)

---

## 7. CI / Build System Changes

### 7.1 `build_commands.txt`

The `-readability-magic-numbers` suppression has been **removed** from the Clang-Tidy invocation. This correctly re-enables the check to guard against regressions.

### 7.2 `clang_tidy_suppressions_backlog.md`

Updated to reflect "PARTIALLY RESOLVED in 0.7.107 (Batch 1: Core Physics)". This is accurate — other files such as `Config.cpp` and `GripLoadEstimation.cpp` still contain magic numbers for future batches.

> [!NOTE]
> The `cmake --build build_ninja` command using Clang-Tidy fails on `vendor/imgui` files. This is a **pre-existing issue with third-party vendor code** (imgui uses MSVC-specific `offsetof` and `reinterpret_cast` patterns that are not valid constexpr under Clang-Tidy). It is unrelated to this PR and does not affect the MSVC build or test suite.

---

## 8. Documentation Updates

- **`CHANGELOG_DEV.md`:** Entry for `0.7.107` is clear and accurate. Correctly characterizes scope as "Batch 1 (Core Physics)".
- **`USER_CHANGELOG.md`:** User-facing entry is appropriate — explains the internal improvement without implying any change in FFB feel.
- **`clang_tidy_suppressions_backlog.md`:** "PARTIALLY RESOLVED" wording is accurate and honest.

---

## 9. Summary of Findings

| ID | Severity | Category | Finding | Status |
|---|---|---|---|---|
| 3.1 | ✅ Improvement | Naming | G-clamp decomposed via `G_LIMIT_5G * GRAVITY_MS2` — excellent | — |
| 3.2 | ✅ OK | Naming | `DUAL_DIVISOR` consistent across all 15+ usages | — |
| 3.3 | ✅ Resolved | Naming | `DT_EPSILON` → `DEFLECTION_NEAR_ZERO_M` for deflection checks | Fixed |
| 3.4 | ✅ Resolved | Naming | `SPEED_EPSILON` → `MIN_VALID_LAT_FORCE_N` for lateral force checks | Fixed |
| 3.5 | ✅ Resolved | Naming | `MIN_SLOPE_WINDOW_S` → `MIN_SLIP_WINDOW` (dimensionless) | Fixed |
| 3.6 | ⚠️ Minor | Unused Code | 4–6 constants declared but unused in `FFBEngine.cpp` — verify in other files | Open |
| 3.7 | ⚠️ Minor | Structure | `STR_BUF_64` and `DEFAULT_CALC_DT` duplicated (by design) — add comment | Open |
| 3.8 | ℹ️ Info | Completeness | One remaining `100.0f` literal in snapshot (out of scope) | Open |
| 6 | ✅ Done | Testing | 11 tests added in `test_ffb_magic_numbers.cpp`, all 374 tests pass | Done |

**No critical or high-severity issues were found. All constant values are correct. Three naming issues resolved.**

---

## 10. Verdict

| Dimension | Status |
|---|---|
| Constant values correct | ✅ All 60+ verified against original literals |
| Naming quality | ✅ All three flagged issues resolved (`DEFLECTION_NEAR_ZERO_M`, `MIN_VALID_LAT_FORCE_N`, `MIN_SLIP_WINDOW`) |
| Structural integrity | ✅ Pure refactoring, zero logic changes |
| CI re-enabled | ✅ `readability-magic-numbers` check active again |
| Unused constants | ⚠️ 4–6 declared but not used in `FFBEngine.cpp` — verify in other files |
| Duplicate globals | ⚠️ `STR_BUF_64` and `DEFAULT_CALC_DT` duplicated — by design, add explanatory comment |
| Branch coverage | ✅ 11 new tests in `test_ffb_magic_numbers.cpp`; all 7 flagged branches now exercised |
| Test suite | ✅ 374/374 cases pass, 1505 assertions, 0 failures |

**Recommendation: APPROVE. All naming concerns addressed. No regressions introduced.**

---

*Review generated by Antigravity (AI Code Review Agent) — 2026-02-28*  
*Updated after follow-up fixes to §3.3, §3.4, §3.5 and addition of §6 (test suite).*  
*Diff file: `%TEMP%\magic_numbers_diff.txt`*
