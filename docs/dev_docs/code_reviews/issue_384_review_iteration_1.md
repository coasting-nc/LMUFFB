**Analysis and Reasoning:**

1.  **User's Goal:** The objective is to prevent violent steering wheel jolts and permanent "NaN infection" of internal filters caused by invalid telemetry data during game session transitions.

2.  **Evaluation of the Solution:**
    *   **Core Functionality:** The patch implements a comprehensive "Defense in Depth" strategy as outlined in the issue and implementation plan.
        *   **Hardware Sanitization:** Added a check in `DirectInputFFB::UpdateForce` to catch non-finite values before they are cast to integers, which prevents the massive negative magnitude bug.
        *   **Core Telemetry Sanitization:** Early rejection of frames where critical chassis data (steering, rotation, torque) is NaN.
        *   **Auxiliary Sanitization:** Non-critical channels (throttle, brake, wheel loads) are sanitized to `0.0` in a working copy. This is particularly clever as it prevents filter corruption while allowing the engine to trigger existing fallbacks (like `approximate_load`) instead of cutting FFB entirely.
        *   **Context Switch Safety:** Changed the filter reset logic to trigger whenever the "allowed" state changes in *either* direction (`m_was_allowed != allowed`).
        *   **Filter Integrity:** The code was correctly updated to ensure that the upsamplers/filters are fed the sanitized `m_working_info` instead of the raw `data->` pointer.
    *   **Safety & Side Effects:** The use of `std::isfinite` is correct. The logic avoids global FFB loss for minor telemetry glitches by using the "Sanitize and Fallback" approach for auxiliary data. No regressions or security vulnerabilities (like hardcoded secrets) are introduced.
    *   **Completeness:** The patch is exceptionally complete. It includes the source code changes, a new suite of unit tests verifying all aspects of the fix (core rejection, auxiliary sanitization, bidirectional reset), updated documentation (Implementation Plan, GitHub Issue mirror), and standard project maintenance (VERSION, CHANGELOG_DEV.md).

3.  **Merge Assessment (Blocking vs. Non-Blocking):**
    *   **Blocking:** None. The solution is functional, safe, and highly maintainable.
    *   **Nitpicks:** None. The code quality is excellent and follows the project's reliability standards.

### Final Rating: #Correct#
