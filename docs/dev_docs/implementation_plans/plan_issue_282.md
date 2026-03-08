# Implementation Plan - Issue 282: Lateral Load effect is notchy / steep and seems inverted

Implement mathematical transformations (Cubic, Quadratic, Hermite) to the lateral load effect to eliminate "notchiness" at the limits, fix its perceived inversion by changing its sign, and decouple it from the oversteer-boosted SoP sub-formula.

## Issue Reference
[Issue #282](https://github.com/coasting-nc/LMUFFB/issues/282) - Lateral Load effect is notchy / steep and seems inverted

## Design Rationale
- **Smoothing (Anti-Notch):** The "notchiness" at 100% load transfer is caused by a linear slope hitting a mathematical ceiling. Applying S-curve transformations (Cubic, Quadratic, Hermite) reduces the derivative to zero at the extremes, providing a progressive "feel" as the tire approaches its limit.
- **Sign Inversion:** User feedback indicates the current lateral load direction feels inverted relative to expectations. Changing the sign will align it with the desired physical sensation.
- **Decoupling from Boost:** Currently, lateral load is part of `sop_base` which is multiplied by the `oversteer_boost`. Moving it to an "independent addendum" ensures it provides a consistent, un-boosted reference of weight transfer, which is more useful for tactile feedback of chassis roll and avoids interference with oversteer effects.

## Proposed Changes

### 1. FFBEngine Logic Updates (`src/FFBEngine.h`, `src/FFBEngine.cpp`)
- Add `LatLoadTransform` enum class to define the transformation types: `LINEAR`, `CUBIC`, `QUADRATIC`, `HERMITE`.
- Add `m_lat_load_transform` member variable to `FFBEngine`.
- Update `FFBCalculationContext` to include `lat_load_force`.
- Modify `calculate_sop_lateral`:
    - Calculate `lat_load_norm = (total_load > 1.0) ? (fr_load - fl_load) / total_load : 0.0;` (Inverted sign: `fr - fl` instead of `fl - fr`).
    - Clamp `lat_load_norm` to `[-1.0, 1.0]`.
    - Apply selected transformation:
        - **Cubic:** $f(x) = 1.5x - 0.5x^3$
        - **Quadratic:** $f(x) = 2x - x|x|$
        - **Hermite:** $f(x) = x \cdot (1 + |x| - x^2)$
    - Smooth the transformed value.
    - Calculate `ctx.lat_load_force = (m_sop_load_smoothed * (double)m_lat_load_effect) * (double)m_sop_scale;`.
    - Remove `m_sop_load_smoothed` from `sop_base` calculation.
- Modify `calculate_force`:
    - Add `ctx.lat_load_force` to `structural_sum`.
- **Verification:** Use `read_file` on `src/FFBEngine.h` and `src/FFBEngine.cpp` to confirm implementation.

### 2. Configuration & Persistence (`src/Config.h`, `src/Config.cpp`)
- Add `lat_load_transform` to `Preset` struct.
- Update `Preset::Apply` and `Preset::UpdateFromEngine`.
- Update `Config::Save` and `Config::Load` to handle the new setting (stored as an integer).
- Update `WritePresetFields` and `ParsePresetLine`.
- **Verification:** Use `read_file` on `src/Config.h` and `src/Config.cpp` to confirm implementation.

### 3. GUI Updates (`src/GuiLayer_Common.cpp`)
- Add a combo box (dropdown) for "Lateral Load Transform" below the "Lateral Load" slider.
- Options: "Linear (Raw)", "Cubic (Smooth)", "Quadratic (Broad)", "Hermite (Locked Center)".
- **Verification:** Use `read_file` on `src/GuiLayer_Common.cpp` to confirm implementation.

### 4. Versioning & Changelog
- Increment `VERSION` to `0.7.154`.
- Update `CHANGELOG_DEV.md`.
- **Verification:** Use `read_file` on `VERSION` and `CHANGELOG_DEV.md`.

## Parameter Synchronization Checklist
- [x] Declaration in `FFBEngine.h`: `LatLoadTransform m_lat_load_transform = LatLoadTransform::LINEAR;`
- [x] Declaration in `Preset` (Config.h): `int lat_load_transform = 0;`
- [x] `Preset::Apply()`: `engine.m_lat_load_transform = static_cast<LatLoadTransform>(lat_load_transform);`
- [x] `Preset::UpdateFromEngine()`: `lat_load_transform = static_cast<int>(engine.m_lat_load_transform);`
- [x] `Config::Save()`: `file << "lat_load_transform=" << static_cast<int>(engine.m_lat_load_transform) << "\n";`
- [x] `Config::Load()`: `else if (key == "lat_load_transform") engine.m_lat_load_transform = static_cast<LatLoadTransform>(std::stoi(value));`
- [x] Validation: Clamp int value to `[0, 3]` during load.

## Test Plan
- **New Unit Test File: `tests/test_issue_282_lateral_load_fix.cpp`**
    - `test_issue_282_transformations`: Verify all 4 transformation types at `x=0, 0.5, 1.0` and symmetry.
    - `test_issue_282_sign_inversion`: Verify `lat_load_force` pulls in the opposite direction to `sop_base_force` for same-turn telemetry.
    - `test_issue_282_decoupling`: Verify `oversteer_boost` does not scale `lat_load_force`.
- **Verification:** Use `ls tests/test_issue_282_lateral_load_fix.cpp` to confirm creation.
- **Full Suite:** Run `./build/tests/run_combined_tests` to ensure no regressions.

## Deliverables
- [ ] Modified `src/FFBEngine.h`, `src/FFBEngine.cpp`
- [ ] Modified `src/Config.h`, `src/Config.cpp`
- [ ] Modified `src/GuiLayer_Common.cpp`
- [ ] Updated `VERSION`
- [ ] Updated `CHANGELOG_DEV.md`
- [ ] New test file `tests/test_issue_282_lateral_load_fix.cpp`
- [ ] Implementation Plan with final notes.
- [ ] Complete pre-commit steps to ensure proper testing, verification, review, and reflection are done.

## Implementation Notes

### Iteration 1
- Initial implementation of the requested transformations (Cubic, Quadratic, Hermite) in `FFBEngine.cpp`.
- Fixed the sign inversion and decoupled lateral load from the oversteer-boosted `sop_base`.
- Added GUI dropdown and Config persistence.
- Added comprehensive unit tests and updated existing ones.
- **Review Feedback**: Technical implementation was solid, but identified missing procedural steps (VERSION, CHANGELOG, and code review records). Also suggested moving formulas to `MathUtils.h` and adding thread safety to GUI updates.

### Iteration 2
- Refactored transformation formulas into `src/MathUtils.h`.
- Added `std::lock_guard<std::recursive_mutex> lock(g_engine_mutex)` to GUI update logic for thread safety.
- Updated `VERSION` to `0.7.154`.
- Updated `CHANGELOG_DEV.md`.
- Added documentation of code reviews in `docs/dev_docs/code_reviews/`.
- **Review Feedback**: "Final Rating: #Correct#". All requirements met.

### Unforeseen Issues
- Existing tests for Issue #213 failed initially due to the architectural change (splitting SoP G-force from Load-force) and the sign inversion. These tests were updated to reflect the new expected behavior.

### Challenges
- Ensuring the mathematical transformations reached exactly 1.0 at x=1.0 while maintaining zero slope was key. The selected formulas ($1.5x - 0.5x^3$ for Cubic, $2x - x|x|$ for Quadratic, and $x(1 + |x| - x^2)$ for Hermite) satisfy these conditions perfectly.

### Recommendations
- The independent `lat_load_force` addendum could potentially benefit from its own scale factor in the future if users find it too dominant compared to G-based SoP, though `m_lat_load_effect` already provides this control.
