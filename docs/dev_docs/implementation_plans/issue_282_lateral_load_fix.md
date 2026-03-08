# Implementation Plan - Issue #282: Lateral Load effect is notchy / steep and seems inverted

## 1. Context
The "Lateral Load" effect was introduced to provide Seat-of-the-Pants (SoP) feedback based on tire load transfer. However, user feedback indicates it feel "notchy" (steep high point), and feels "inverted" (pulls into the turn). Additionally, the current GUI slider range is insufficient for some users.

### Design Rationale
- **Notchy/Steep feel**: Caused by dynamic normalization `(FL - FR) / (FL + FR)`. As one tire unloads completely, the ratio hits 1.0 abruptly.
- **Aero-Fade**: The dynamic denominator `(FL + FR)` increases with speed due to downforce, reducing the ratio. The user explicitly asked NOT to fix this, so we will preserve the dynamic component.
- **Inverted feel**: Sign convention fix needed.
- **Slider Range**: Increasing from 200% to 1000% provides more headroom.

## 2. Analysis
Current implementation in `FFBEngine::calculate_sop_lateral`:
```cpp
double total_load = fl_load + fr_load;
double lat_load_norm = (total_load > 1.0) ? (fl_load - fr_load) / total_load : 0.0;
```

Proposed implementation:
```cpp
double lat_load_norm = (fl_load - fr_load) / (fl_load + fr_load + m_fixed_static_load_front + EPSILON_DIV);
lat_load_norm *= 4.0;
```
By adding `m_fixed_static_load_front` to the denominator, we prevent the ratio from plateauing at 1.0 early, making it "less notchy". Since the denominator is now larger, a 4.0x multiplier is used to maintain parity with Lateral G.

### Design Rationale
- **Less Notchy**: Static load addition smooths the curve near tire unloading.
- **Preserve Aero-Fade**: `fl_load + fr_load` is still present, so the signal still attenuates with aero downforce.
- **Sign Fix**: A global `-1.0` inversion is applied to `sop_base` to ensure it resists the turn (LMU +X = Left, Right Turn = Positive Accel, Resisting Force = Negative DirectInput).

## 3. Proposed Changes

### src/Config.h & src/Config.cpp
- Update `m_lat_load_effect` limit and parsing to allow up to 10.0 (1000%).

### src/GuiLayer_Common.cpp
- Increase "Lateral Load" slider max from 2.0f to 10.0f.

### src/FFBEngine.cpp
- Modify `calculate_sop_lateral` to use the "less notchy" formula.
- Apply 4.0x parity multiplier.
- Apply -1.0x inversion to the entire `sop_base` calculation.

## 4. Test Plan
- **Unit Test**: Update `tests/test_issue_213_lateral_load.cpp` to verify the new normalization and directionality.
- **Verification**: Ensure that at 1G lat accel, the Load component resists the turn and has comparable magnitude to Lateral G.

## 5. Implementation Notes
- **Normalization Formula**: Changed to `(FL - FR) / (FL + FR + static_load)` to prevent early saturation (notchiness) while keeping the effect responsive to aerodynamic loading (preserving Aero-Fade).
- **Parity Multiplier**: Increased to 4.0x to compensate for the larger denominator and maintain feel parity with the acceleration-based Lateral G effect.
- **Directionality**: Applied a global `-1.0` inversion to the SoP base force. LMU's coordinate system (+X=Left) means a right turn produces positive acceleration. To produce a resisting force (pulling left), DirectInput requires a negative value.
- **Slider Range**: Successfully increased `m_lat_load_effect` limit to 10.0 (1000%) in `Config.h`, `Config.cpp`, and `GuiLayer_Common.cpp`.
- **Validation**: Updated `tests/test_issue_213_lateral_load.cpp` with the new expected math and verified that both G-force and Load-force pull in the correct resisting direction.

## 6. Post-Implementation Review
### Iteration 1
- **Issues Raised**: The initial implementation (v0.7.150 variant) correctly used fixed static normalization, but this addressed "Aero-Fade" which was explicitly excluded by the user's secondary feedback. The reviewer also noted missing meta-file updates (VERSION, CHANGELOG).
- **Resolution**: Reverted to a hybrid dynamic+static normalization formula `(FL - FR) / (FL + FR + static_load)` which preserves Aero-Fade (the dynamic component is still in the denominator) while fixing notchiness (the static component prevents the ratio from hitting 1.0 abruptly). Increment VERSION to 0.7.153 and updated CHANGELOG_DEV.md.
- **Discrepancies**: None. The reviewer's feedback on missing meta-files was accurate and has been addressed.
