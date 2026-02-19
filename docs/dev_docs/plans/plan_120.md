# Implementation Plan - Fix Tire Load Normalization (#120)

**Context**:
Address remaining hardcoded tire load constants that cause incorrect FFB behavior (especially artificial bottoming) in high-downforce vehicle classes like Hypercars and LMP2.

**Codebase Analysis Summary**:
- `VehicleUtils.cpp`: `GetDefaultLoadForClass` had a mismatch for `LMP2_UNSPECIFIED` (7500N vs expected 8000N).
- `FFBEngine.cpp`: `calculate_suspension_bottoming` used a hardcoded 8000N threshold, which is lower than the peak load of Hypercars (9500N), causing false bottoming triggers.
- `FFBEngine.cpp`: `update_static_load_reference` used a hardcoded 4000N fallback.

**FFB Effect Impact Analysis**:
- **Suspension Bottoming**: No longer triggers falsely on straights for Hypercars. Threshold now scales dynamically with car class (1.6x peak load).
- **Dynamic Weight**: More accurate baseline for cars where the static load hasn't been learned yet, as it now uses class-aware fallbacks and resets on car change.

**Proposed Changes**:
1. Update `GetDefaultLoadForClass` in `src/VehicleUtils.cpp` to return 8000.0 for `LMP2_UNSPECIFIED`.
2. Define `NOINLINE` macro in `src/FFBEngine.h` for cross-platform support and replace `__declspec(noinline)` usage.
3. Replace hardcoded `8000.0` in `FFBEngine::calculate_suspension_bottoming` with `m_auto_peak_load * 1.6`.
4. Replace hardcoded `4000.0` fallback in `FFBEngine::update_static_load_reference` with `m_auto_peak_load * 0.5`.
5. Update `FFBEngine::InitializeLoadReference` to reset `m_static_front_load` to the class-aware fallback when car class changes.
6. Increment version to `0.7.60` in `VERSION`.

**Test Plan**:
- Add `test_lmp2_unspecified_load` to verify 8000N seeding.
- Add `test_hypercar_bottoming_threshold` to verify dynamic bottoming threshold (no trigger at 10000N for Hypercar).
- Add `test_static_load_fallback_class_aware` to verify reset and fallback logic.
- Update existing tests in `test_ffb_coverage_refactor.cpp` and `test_ffb_load_normalization.cpp` that relied on old hardcoded values.

**Implementation Notes**:
- **Unforeseen Issues**: Encountered phase cancellation in bottoming test due to specific `dt` and 50Hz frequency; resolved by changing `dt`.
- **Plan Deviations**: Added `m_static_front_load` reset in `InitializeLoadReference` to ensure tests passing when switching car classes.
- **Challenges**: Linux build failed initially due to `__declspec` which was fixed by introducing `NOINLINE` macro.
- **Recommendations**: Always use class-aware multipliers instead of Newton constants for any load-dependent effects.
