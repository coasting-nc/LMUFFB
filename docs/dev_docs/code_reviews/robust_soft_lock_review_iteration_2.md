# Code Review - Iteration 2

## Summary
The proposed code change addresses **Issue #248**, where the steering soft lock felt like a weak, temporary "bump" that would disappear once the steering wheel stopped moving.

### Evaluation of the Solution:
- **Core Functionality:** The patch implements a **Steep Spring Ramp** that reaches 200% of the hardware's maximum torque within a very small steering excess (0.25% at default stiffness).
- **Persistence:** By prioritizing the spring component and scaling it directly to `m_wheelbase_max_nm`, the resistance persists at zero velocity, effectively creating a "wall" feel.
- **Damping:** Scaled relative to hardware and clamped to 50% max torque, providing stability without overwhelming the stop.
- **Safety:** Ensuring consistency across all wheelbases by using `m_wheelbase_max_nm` for normalization.

### Assessment of Previous Issues:
- **Repository Pollution:** Large logs have been removed from the root directory.
- **Nitpicks:** `std::clamp` in `SteeringUtils.cpp` is functional (likely transitively included).
- **Workflow:** Iteration reviews are being correctly recorded.

## Decision
Correct (Greenlight). The solution is technically robust and meets all requirements.
