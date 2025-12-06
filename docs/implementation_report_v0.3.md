# Implementation Report v0.3.0

This document outlines the implementation details for the version 0.3.0 update, focusing on the transition from synthetic "canned" effects to dynamic physics-based signals with correct phase integration.

## 1. Analysis of Requirements

The user requested improvements based on `docs/report_on_ffb_improvements.md`. Key findings from the analysis:

*   **The Math Problem**: Using absolute time (`mElapsedTime`) with dynamic frequencies causes phase discontinuity ("clicks/pops").
    *   **Solution**: Adopt **Phase Accumulation** (`phase += freq * dt`).
*   **Wheel Spin**: The initial proposal to link spin to Engine RPM was rejected due to "noise" concerns on gear/belt wheels.
    *   **Solution**: Link vibration frequency to **Tire Slip Speed** (Car Speed * Slip Ratio). This ensures silence during grip and progressive feedback during slip.
*   **Lockup**: Link vibration to **Car Speed** (scrubbing pitch) and amplitude to **Tire Load**.

## 2. Implementation Details

### A. Phase Integration
Added member variables to `FFBEngine` to track the current phase of each oscillator:
*   `m_lockup_phase`
*   `m_spin_phase`
*   `m_slide_phase`

These accumulate `frequency * delta_time` each frame, ensuring smooth waveform continuity even when frequency modulates rapidly.

### B. Progressive Lockup
*   **Trigger**: Brake input + Slip Ratio < -0.1.
*   **Frequency**: `10Hz + (CarSpeed_ms * 1.5)`. Transitions from low judder at low speed to high-pitch scrub at high speed.
*   **Amplitude**: Scaled by `severity` (slip depth) AND `lockup_gain`.
*   **Waveform**: Sine wave (smoothed).

### C. Traction Loss (Wheel Spin)
*   **Trigger**: Throttle input + Slip Ratio > 0.2.
*   **Torque Drop**: Reduced total FFB gain by up to 60% based on slip severity. This provides the "floating" feeling of a powered slide.
*   **Vibration**:
    *   **Frequency**: Derived from **Slip Speed** (`CarSpeed * SlipRatio`).
    *   Mapping: Low slip speed = Low Hz (Grip fighting). High slip speed = High Hz (Free spinning).
    *   This replaces the "RPM" proposal to ensure the effect is purely tire-dynamics based.

### D. Slide Texture
*   **Trigger**: Lateral Slip Angle > 0.15 rad.
*   **Frequency**: Derived from `LateralGroundVel` (sideways speed).
*   **Waveform**: Changed from Sine to **Sawtooth** approximation (`(phase / 2PI) * 2 - 1`). This provides a sharper "stick-slip" texture more characteristic of rubber sliding on asphalt.
*   **Amplitude**: Modulated by `TireLoad`. Heavily loaded tires vibrate more violently.

## 3. Deviations from Initial Suggestions

*   **RPM Link**: Rejected as per "Follow up questions" analysis. Implemented **Slip Speed** logic instead.
*   **Load Factor**: Simplified load factor normalization to avoid extreme values.

## 4. Verification
*   **Tests**: Updated `tests/test_ffb_engine.cpp` to mock `mDeltaTime` and verify phase accumulation logic does not reset (except wrap-around) or jump discontinuously.
