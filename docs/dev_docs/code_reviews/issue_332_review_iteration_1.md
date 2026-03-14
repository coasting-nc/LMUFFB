# Code Review - Issue #332 - Iteration 1

This is a high-quality, technically sound patch that directly addresses the regressions reported in Issue #332. The solution demonstrates a deep understanding of the rFactor 2 shared memory behavior and the project's safety mechanisms.

### 1. User's Goal
The objective is to fix Force Feedback (FFB) gain loss during multiplayer join/leave events and prevent the steering wheel from locking at full rotation when the car is in a garage stall.

### 2. Evaluation of the Solution

#### Core Functionality
The patch addresses both parts of the issue effectively:
- **FFB Loss:** In multiplayer, rFactor 2 frequently reorders vehicles when players join or leave, causing the `mControl` state to flicker to `NONE (-1)`. The patch modifies the safety trigger logic to ignore transitions involving `-1`. This prevents the 2-second "Safety Window" (which reduces gain to 30%) from triggering unnecessarily during these transient glitches.
- **Wheel Lock in Pits:** The FFB engine was preserving the "Soft Lock" force even when muted to prevent snaps during Pause. However, in a garage stall, if the steering value is extreme (common during teleports), this caused the wheel to stay pinned. The patch introduces an `inGarage` flag to `calculate_force`, allowing the engine to zero out the soft lock force specifically when in a garage stall, while maintaining it for other states like Pause (satisfying Issue #281).

#### Safety & Side Effects
- **Thread Safety:** The patch correctly uses `g_engine_mutex` when accessing shared state.
- **Regressions:** The logic for ignoring `-1` transitions is well-debounced. While it might theoretically skip a safety window if a genuine AI-to-Player transition happens to pass through a `-1` state frame, this is a preferred trade-off to solve the "unusable" FFB loss reported by users.
- **Mocks & Tests:** The inclusion of `tests/test_issue_332.cpp` is excellent. It covers both scenarios (control glitch and garage soft lock) and ensures no regression for the Pause behavior.

#### Completeness
- The patch correctly updates the function signature in `FFBEngine.h`, the implementation in `FFBEngine.cpp`, and the call site in `main.cpp`.
- **Administrative Omissions (Nitpicks):** The patch fails to include the `VERSION` bump and `CHANGELOG_DEV.md` update, which were explicitly requested in the workflow instructions. It also lacks the `review_iteration_X.md` files required by the "Fixer" protocol. However, from a code quality and functional standpoint, the solution is complete.

### 3. Merge Assessment

- **Blocking:** None. The code logic is robust and safe.
- **Nitpicks:** Missing `VERSION` and `CHANGELOG_DEV.md` updates. These should be added before final release but do not impact the functional correctness of the fix.

### Final Rating: #Mostly Correct#
