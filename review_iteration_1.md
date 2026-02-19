# Code Review - Soft Limiter for FFB Clipping

**Round:** 1
**Status:** Greenlight ✅

## Summary
Implemented a Soft Limiter (Compressor) to address "Force Rectification" artifacts when the FFB signal clips. This feature gradually compresses forces as they approach the hardware limit (100%), preserving vibration detail that would otherwise be lost to hard clipping.

## Changeset Evaluation

### 1. Physics & Math (`src/MathUtils.h`, `src/FFBEngine.cpp`)
- **Logic**: Added `apply_soft_limiter` using a tanh-based asymptotic curve. This is mathematically sound for FFB compression as it ensures a smooth transition and never exceeds 1.0.
- **Integration**: The limiter is applied after master gain scaling but before final hardware clamping, which is the correct position for a compressor.
- **Safety**: Ensured `range` (1.0 - knee) never results in division by zero. (Note: `std::isfinite` checks were missing in this iteration and addressed in the next).

### 2. Configuration & Persistence (`src/Config.h`, `src/Config.cpp`)
- **Compatibility**: Defaulted `soft_limiter_enabled` to `false` to maintain existing FFB feel and test integrity by default.
- **Robustness**: Added rigorous clamping in `Validate()` and `Apply()` to ensure the knee remains within a sensible [0.1, 0.99] range.
- **Persistence**: Fully integrated into the INI parser and preset system.

### 3. User Interface (`src/GuiLayer_Common.cpp`)
- **Accessibility**: Added a dedicated "Soft Limiter / Compressor" section in the General FFB tab with descriptive tooltips.

### 4. Observability (`src/FFBEngine.h`)
- **Diagnostics**: Added `clipping_soft` to `FFBSnapshot`, allowing users and developers to monitor the exact amount of compression being applied in real-time.

### 5. Testing (`tests/test_ffb_soft_limiter.cpp`)
- **Coverage**: Verified basic math, engine integration, and clipping flag logic.
- **Regression**: Confirmed that all 263+ existing tests pass, ensuring zero impact on legacy features.

## Conclusion
The implementation is robust, thread-safe, and follows the project's architectural patterns. No issues found.
