# Implementation Plan - Soft Limiter for FFB Clipping

Implement a Soft Limiter (Compressor) in the FFB Engine to prevent "Force Rectification" caused by hard clipping at 100% force.

## Context
Address the "resistance drop" phenomenon when FFB vibrations (like Slide Rumble) clip against the hardware limit. Hard clipping chops off signal peaks while leaving troughs intact, resulting in a net loss of average resisting force. A soft limiter gradually compresses the signal as it approaches the limit, preserving the average force and vibration detail.

### Choice of Issue
This feature was selected based on a critical recommendation in the "Slide Rumble" bug report (`docs/bug_reports/slide rumble issue.md`). The report identifies **Clipping-Induced "Force Rectification"** as a primary cause of the wheel suddenly feeling light and "throwing" into turns when textures activate at high loads. Implementing a "Soft Limiter" or "Compressor" was specifically noted as the ideal future fix for this reliability flaw in the physics engine. While it does not have a formal GitHub issue number, it directly resolves a high-severity behavior reported by users.

## Reference Documents
- Bug Report: `docs/bug_reports/slide rumble issue.md`
- FFB Formulas: `docs/dev_docs/references/FFB_formulas.md`

## Codebase Analysis Summary
- **FFB Calculation**: `FFBEngine::calculate_force` in `src/FFBEngine.cpp` handles the summation and normalization of all force components.
- **Normalization**: `norm_force` is calculated by dividing total sum by `max_torque_ref`.
- **Clipping**: Currently implemented as a hard `clamp(force, -1.0, 1.0)`.
- **Math Utilities**: `src/MathUtils.h` contains shared math helpers.
- **Diagnostics**: `FFBSnapshot` captures state for the GUI.

## FFB Effect Impact Analysis
- **Primary Impact**: All structural forces (Aligning Torque, SoP) and textures (Slide, Road) are subject to the limiter.
- **User Perspective**:
    - Improved consistency: The wheel won't feel "lighter" when textures activate near the limit.
    - More detail: High-frequency vibrations remain palpable even at high cornering loads.
    - Configurable: Users can tune the "Knee" to balance between raw weight and detail preservation.

## Proposed Changes

### 1. `src/MathUtils.h`
- Add `apply_soft_limiter` function using `tanh` for asymptotic compression.

### 2. `src/FFBEngine.h`
- Add member variables to `FFBEngine`:
    - `bool m_soft_limiter_enabled`
    - `float m_soft_limiter_knee`
- Update `FFBSnapshot` to include `clipping_soft` for diagnostics (Ref: Slide Rumble Report).

### 3. `src/Config.h` & `src/Config.cpp`
- Add settings to `Preset` struct.
- **Parameter Synchronization Checklist**:
    - [x] Declaration in `FFBEngine.h`
    - [x] Declaration in `Preset` (Config.h)
    - [x] Entry in `Preset::Apply()`
    - [x] Entry in `Preset::Validate()`
    - [x] Entry in `Preset::UpdateFromEngine()`
    - [x] Entry in `Config::Save()`
    - [x] Entry in `Config::Load()`
    - [x] Entry in `Config::ParsePresetLine()`
    - [x] Entry in `Config::WritePresetFields()`

### 4. `src/FFBEngine.cpp`
- Modify `calculate_force` to apply the limiter before final scaling/clamping (Addressing Force Rectification).
- Populate `clipping_soft` diagnostic.

### 5. `src/GuiLayer_Common.cpp`
- Add UI controls (Checkbox, Slider) for Soft Limiter settings.

### 6. Versioning
- Increment version in `VERSION` and `src/Version.h` by smallest possible increment.
- Update `CHANGELOG_DEV.md`.

## Test Plan (TDD)
1. **`test_soft_limiter_basic_math`**: Verify `apply_soft_limiter` output for values below, at, and above the knee.
2. **`test_soft_limiter_integration`**: Verify `FFBEngine` correctly applies the limiter to the total force.
3. **`test_soft_limiter_clipping_flags`**: Verify that `clipping` (hard) and `clipping_soft` (compression amount) flags are correctly populated in snapshots.
4. **Regression**: Run existing 263+ tests.

## Deliverables
- [x] Modified `src/MathUtils.h`
- [x] Modified `src/FFBEngine.h` and `src/FFBEngine.cpp`
- [x] Modified `src/Config.h` and `src/Config.cpp`
- [x] Modified `src/GuiLayer_Common.cpp`
- [x] New test file `tests/test_ffb_soft_limiter.cpp`
- [x] Updated `VERSION`, `src/Version.h`, and `CHANGELOG_DEV.md`
- [x] Implementation Notes updated in this plan.

## Implementation Notes

### Unforeseen Issues
- **Test Asymmetry**: Discovered that `FFBEngine::calculate_force` required more telemetry fields (speed, load, grip) to be initialized in the mock data than initially expected to prevent side-effects from other features (like speed gating or understeer effects).
- **Floating Point Precision**: The asymptotic behavior check `out_100 < 1.0` required changing to `<= 1.0` due to double-to-float conversions in the engine path.

### Plan Deviations
- **Default Disabled**: Changed default state to `false` (disabled) to ensure that existing users and unit tests see zero change in behavior unless they explicitly enable the feature. This is safer for an experimental physics addition.

### Challenges
- **Rectification Verification**: Proving "reduction in rectification" via unit tests is mathematically complex since it depends on the frequency and waveform of the noise being compressed. Settled for verifying that the average force changes as expected when peaks are compressed vs hard-clipped.

### Recommendations
- **GUI Visualization**: Consider adding a specific color or indicator to the "Clipping" bar in the GUI to show when soft-limiting is active (e.g. orange for soft, red for hard).
