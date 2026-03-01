# Implementation Plan - Issue #206: Scale the strength of Absolute Tactile Textures

## Context
Issue #206 requests a separate slider to scale the strength of "Absolute Tactile Textures". Currently, tactile effects (Road Details, Slide Rumble, Lockup Vibration, etc.) are calculated in absolute Nm and mapped directly to the wheelbase's maximum torque. On some hardware (especially lower-end wheels), these absolute values can be disproportionately strong or weak. A dedicated "Tactile Strength" slider will allow users to scale these effects globally while maintaining their internal absolute Nm logic.

**Design Rationale**: The architectural shift to "Absolute Nm" scaling in v0.7.68 was designed to provide hardware-agnostic haptics. However, hardware differences (motor inertia, internal damping, belt vs direct drive) mean that a "physically accurate" 2Nm pulse may feel different to different users. A global gain stage provides the necessary flexibility for hardware compensation without breaking the underlying physical model.

## Reference Documents
- GitHub Issue #206: [Scale the strength of Absolute Tactile Textures](https://github.com/coasting-nc/LMUFFB/issues/206)
- GitHub Issue #153: Stage 2 Hardware Strength Scaling.
- GitHub Issue #154: Stage 3 Tactile Haptics Normalization.
- GitHub Issue #181: Soft Lock Absolute Nm consistency.
- GitHub Issue #207: Disable Dynamic Normalization by default.

## Codebase Analysis Summary
- **FFBEngine.h/cpp**:
  - `calculate_force()`: Sums structural forces and tactile textures. Currently, tactile textures are summed into `texture_sum_nm` and converted to DirectInput percentage using `wheelbase_max_safe`.
  - Soft Lock is currently included in the `texture_sum_nm` (Absolute Nm) group.

**Design Rationale**: The `calculate_force` function is the core of the signal chain. Introducing the gain stage at the point of summation (before conversion to DI percentage) ensures that the scaling is applied uniformly to all haptic components while they are still in their physical (Nm) representation.

## FFB Effect Impact Analysis
| Effect | Technical Changes | Expected User-Facing Change |
| :--- | :--- | :--- |
| **Tactile Textures** | Multiplied by `m_tactile_gain` before final scaling. Includes: Road Noise, Slide Noise, Spin Rumble, Bottoming Crunch, ABS Pulse, Lockup Rumble. | User can globally increase/decrease the intensity of all haptic "vibrations" using a single slider. |
| **Soft Lock** | **EXCLUDED** from `m_tactile_gain` scaling. | Remains at a consistent absolute Nm level for safety and physical realism. |
| **Structural Forces** | **No Change**. | Steering weight and SoP feel remain unaffected by the tactile gain. |

**Design Rationale**:
- **Tactile Scaling**: Using a single global multiplier rather than per-effect multipliers reduces UI clutter and provides a "master volume" for haptics, which is the most common user request for hardware matching.
- **Soft Lock Exclusion**: Soft Lock is a safety feature representing physical steering rack limits. Scaling it alongside "vibrations" would be dangerous; it must remain anchored to the physical torque limits of the car and wheelbase to prevent hardware damage or unexpected behavior at the steering limit.

## Proposed Changes

### 1. FFBEngine.h
- Add `float m_tactile_gain = 1.0f;` to the `FFBEngine` class.

**Design Rationale**: Storing this as a member of the engine allows the high-frequency FFB thread to access it with zero latency during the calculation loop.

### 2. FFBEngine.cpp
- Update `calculate_force()`:
  - Calculate `tactile_sum_nm` (tactile textures).
  - Apply `m_tactile_gain` to `tactile_sum_nm`.
  - Add `soft_lock_force` after the gain stage.

**Design Rationale**: Summation in `double` precision before conversion to `float` DirectInput percentage minimizes rounding errors and maintains the high dynamic range required for absolute Nm calculations on powerful DD bases.

### 3. Config.h
- Add `float tactile_gain = 1.0f;` to the `Preset` struct.
- Add `Preset& SetTactileGain(float v)` fluent setter.
- Update `Preset::Apply()`, `Preset::UpdateFromEngine()`, and `Preset::Equals()`.

**Design Rationale**: Full integration into the `Preset` system is critical for user experience, allowing different haptic intensities for different car classes (e.g., lower haptics for prototypes vs higher for GT cars).

### 4. Config.cpp
- Update `ParsePresetLine()` and `WritePresetFields()` for persistence.
- Update `Config::Load()` and `Config::Save()` to handle the global parameter.

**Design Rationale**: Persistence ensures that hardware-specific tuning is remembered across application restarts, fulfilling the reliability-focused mission of the "Fixer" role.

### 5. GuiLayer_Common.cpp / Tooltips.h
- Add the "Tactile Strength" slider and its associated tooltip.

**Design Rationale**: Positioning the slider in the "Tactile Textures" section groups it logically with the effects it controls, following standard UI/UX patterns.

## Parameter Synchronization Checklist
- [x] Declaration in `FFBEngine.h`: `float m_tactile_gain`
- [x] Declaration in `Preset` struct: `float tactile_gain`
- [x] Entry in `Preset::Apply()`
- [x] Entry in `Preset::UpdateFromEngine()`
- [x] Entry in `Config::Save()`
- [x] Entry in `Config::Load()`
- [x] Validation logic: `std::clamp(v, 0.0f, 2.0f)`

## Test Plan
- **Baseline Test Verification**: Ensure all existing tests pass.
- **New Unit Test**: `tests/test_issue_206_tactile_scaling.cpp`
  - **Test case 1**: Verify `m_tactile_gain = 0.5` reduces Road/Slide/Lockup by half.
  - **Test case 2**: Verify `m_tactile_gain = 2.0` doubles Road/Slide/Lockup.
  - **Test case 3**: Verify `m_tactile_gain` does NOT affect `soft_lock_force`.
  - **Test case 4**: Verify `m_tactile_gain` does NOT affect structural steering torque.
- **Persistence Test**: Verify `tactile_gain` is correctly saved to and loaded from `config.ini`.

**Design Rationale**: Unit tests specifically targeting the gain stage separation (Tactile vs structural vs Soft Lock) are mandatory to prevent future regressions where safety effects might accidentally be attenuated by user haptic preferences.

## Deliverables
- [x] Modified `src/FFBEngine.h`, `src/FFBEngine.cpp`
- [x] Modified `src/Config.h`, `src/Config.cpp`
- [x] Modified `src/GuiLayer_Common.cpp`
- [x] Modified `src/Tooltips.h`
- [x] Modified `VERSION`, `CHANGELOG_DEV.md`, `USER_CHANGELOG.md`
- [x] New test file `tests/test_issue_206_tactile_scaling.cpp`.
- [x] Implementation Notes (to be added at the end).

## Implementation Notes

### Unforeseen Issues
- **Test Case Clipping**: Initial unit test failed because the `calculate_force` output was clipping at 1.0 (or -1.0) due to high steering excess and low wheelbase max torque. Increased `m_wheelbase_max_nm` to 1000.0f in the test to ensure linear scaling could be verified without saturation.
- **Soft Lock Trigger**: Adjusted steering excess to a more reasonable 1.01f (1% excess) to ensure Soft Lock was active but not dominating the entire signal range in a way that masked texture scaling.

### Plan Deviations
- **None**: The implementation followed the proposed architectural changes exactly.

### Challenges
- **Snapshot Verification**: Realized that `FFBSnapshot` stores the *raw* texture values before the global tactile gain is applied. To verify scaling, the test had to compare the final `total_output` after zeroing out structural forces. This was a cleaner approach than adding more members to the snapshot.

### Recommendations for Future Plans
- When testing scaling of summed signals, always ensure the "base" (structural) components are zeroed or held constant, and verify that the hardware limit (clipping) is not reached during the test.
