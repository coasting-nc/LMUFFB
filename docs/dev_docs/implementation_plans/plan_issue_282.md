# Implementation Plan - Issue #282: Lateral Load effect is doing nothing

**Issue:** #282 - "Lateral Load effect is doing nothing"

## Context
The "Lateral Load" effect (Issue #213) uses physical tire load transfer to drive Seat-of-the-Pants (SoP) feedback. Users report that it is imperceptible, especially on cars with encrypted telemetry (like LMP2) where tire load must be estimated.

## Analysis
The "doing nothing" sensation is caused by:
1. **Aero-Fade**: Normalizing by dynamic total load $(L_{front}+R_{front})$ causes the signal to shrink as aerodynamic downforce increases at high speeds.
2. **Magnitude Disparity**: The load-based signal is mathematically smaller (~0.4 peak) than the acceleration-based signal (1.0 peak) for the same gain.
3. **Onset Delay**: The 20-frame hysteresis for kinematic fallback creates a 50ms "dead zone" of zero feedback when starting a session or after jumps.

## Design Rationale (MANDATORY)
- **Fixed Static Normalization**: Using a fixed class-based static front axle weight as the denominator ensures the "lean" feel represents weight transfer percentage, making it independent of speed and aero. Fixed values are preferred over learned values for predictability and stability.
- **2.0x Gain Multiplier**: Internal scaling to bring the effect into parity with Lateral G, ensuring both are tangible at similar user settings.
- **Aggressive Fallback**: Reducing the hysteresis to 5 frames (12.5ms) and adding immediate zero-load detection at speed ensures near-instant feedback for encrypted cars.
- **Diagnostic Transparency**: Adding plots to the log analyzer allows for quantitative verification of weight transfer dynamics.
- **Caching**: Caching vehicle class and fixed load values improves performance by avoiding frame-by-frame string parsing.

## Proposed Steps

### 1. Investigation & Log Analyzer Enhancements
- Update `SessionInfo` in `src/AsyncLogger.h` to include `lat_load_effect` and `sop_scale`.
- Populate these in `src/main.cpp` when starting a log.
- Add `plot_lateral_diagnostic` to `tools/lmuffb_log_analyzer/plots.py`.
- Document findings in `docs/dev_docs/investigations/issue_282_lateral_load_analysis.md`.

### 2. Physics & Magnitude Optimization (`src/FFBEngine.cpp`)
- Modify `calculate_sop_lateral` to normalize `lat_load_norm` using a fixed class-based static axle load.
- Apply a 2.0x internal multiplier to the `m_lat_load_effect` contribution in `sop_base`.
- *Loop*: Implement, Build, Test, Commit, Review.

### 3. Fallback Reliability (`src/FFBEngine.cpp`)
- Reduce `MISSING_LOAD_WARN_THRESHOLD` to 5.
- Add immediate fallback trigger if speed is high (> 10 m/s) and tire load is exactly 0.0.
- Rename variables to `*_front` for clarity.
- *Loop*: Implement, Build, Test, Commit, Review.

### 4. Final Verification & Documentation
- Update `tests/test_issue_213_lateral_load.cpp` to account for new normalization and scaling.
- Create `tests/test_issue_282_magnitude.cpp` to specifically validate aero-fade resistance and magnitude parity.
- Update `VERSION` and `CHANGELOG_DEV.md`.
- Run full regression suite.

## Implementation Notes
- **Initial Analysis**: Confirmed that total dynamic load was indeed suppressing the signal at high speeds in LMP2 cars.
- **Review 1 Address**: Addressed Python diagnostic math inconsistency (missing 2.0x boost). Verified that `SPEED_HIGH_THRESHOLD` was already defined.
- **Review 2 Address**: Optimized performance by caching vehicle class and fixed load values in `FFBEngine` members (`m_current_class_enum`, `m_fixed_static_axle_load_front`).
- **Final Results**: Unit tests confirm that the Lateral Load signal now maintains its magnitude at 300 km/h and reaches numerical parity with the Lateral G signal. Kinematic fallback now activates within 12.5ms (or immediately if speeding).
