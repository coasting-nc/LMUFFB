# Code Review - Issue #303: Safety Fixes for FFB Spikes

## Summary
This is a comprehensive and high-quality patch that addresses the user's request for enhanced FFB safety mechanisms and diagnostic logging. It follows the "Fixer" persona's strict workflow, including detailed documentation and a robust implementation plan.

### Analysis and Reasoning:

1.  **User's Goal:** The objective is to implement safety features that detect and mitigate Force Feedback (FFB) spikes/jolts caused by game stutters or state transitions, while improving diagnostic logging to help users and developers understand these events.

2.  **Evaluation of the Solution:**
    *   **Core Functionality:**
        *   **Safety Window:** Successfully implements a 2-second "Safety Mode" triggered by untrusted states.
        *   **Mitigation:** During the safety window, the patch correctly applies a 50% gain reduction, a tighter slew rate limit (200 units/s), and an additional 100ms EMA smoothing pass.
        *   **Detection:** Implements robust detection for lost frames (via telemetry timestamps), control transitions (`mControl`), high-slew spikes (sustained for 5 frames), and "Full Tock" (wheel pinned at lock with high force).
        *   **Logging:** Adds clear, throttled log entries for all safety events, including FFB muting/unmuting and soft lock engagement/influence.
        *   **User Guide:** Provides a new Markdown guide specifically requested by the user to help them interpret the logs.
    *   **Safety & Side Effects:**
        *   The patch adheres to thread-safety requirements by using `g_engine_mutex` when accessing the engine from the FFB thread.
        *   The changes are well-contained within the FFB pipeline and do not modify unrelated core logic.
        *   Sanitization (clamping and finite checks) is maintained.
    *   **Completeness:** The patch includes updates to `FFBEngine`, `main.cpp`, `SteeringUtils`, unit tests, versioning, and documentation. All requested features from the issue description and interaction history are present.

3.  **Merge Assessment:**
    *   **Blocking:** None. The logic is sound, the tests are comprehensive, and the code is maintainable.
    *   **Nitpicks:** None. The use of a static variable in `main.cpp` for frame timing is acceptable for this architecture as it resides within a singleton-like thread context.

### Final Rating: #Correct#
