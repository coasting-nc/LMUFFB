# Code Review - Iteration 1

## Summary
The proposed code change addresses **Issue #248**, where the steering soft lock felt like a temporary "bump" that disappeared once the wheel stopped moving.

### Evaluation of the Solution:
- **Core Functionality:** The patch correctly identifies that the previous implementation relied too heavily on damping (velocity-based) and had a shallow spring ramp. The new implementation introduces a **Steep Spring Ramp** that reaches 200% of the wheelbase's maximum torque within a tiny steering excess (0.25% at default stiffness). By over-driving the target torque and scaling it to the specific hardware's capability (`m_wheelbase_max_nm`), it ensures the force saturates the wheelbase output regardless of other active effects. This effectively creates the "wall" feel requested by the user.
- **Safety & Side Effects:**
  - **Regressions:** The patch updates multiple test suites (`test_ffb_soft_lock.cpp`, `test_issue_181_soft_lock_weakness.cpp`, etc.) to align with the new high-strength scaling. It maintains the `IsFFBAllowed` safety logic.
  - **Sanitization:** The damping component is now clamped to 50% of the hardware's maximum torque, preventing unstable oscillations or extreme counter-forces while still providing necessary stability.
- **Merge Assessment:**
  - **Blocking (Workflow/Packaging):**
    - Workflow Violation: Missing review documentation (addressed by this file).
    - Repository Pollution: Large log files in root (must be deleted).
  - **Nitpicks:**
    - Use of `std::clamp` in `SteeringUtils.cpp` should include `<algorithm>`.

## Decision
Mostly Correct (Greenlight after nitpicks and workflow fixes).
