# Code Review - Issue #393: Fix additional issues related to the FIR filter

## Date: 2026-03-16
## Iteration: 1

### Evaluation of the Solution:
- **Polyphase Resampler:** Correctly implements decoupled buffer-shifting logic using `m_pending_sample` and `m_needs_shift`. This ensures the filter uses correct historical context for all fractional phases.
- **Holt-Winters Filter:** Fixes the "Buzz" bug by returning smoothed `m_level` instead of `raw_input` on frame boundaries, eliminating the 100Hz sawtooth artifact.
- **Testing:** New regression tests (`test_math_utils_issue_393.cpp`, `test_upsampler_issue_393.cpp`) verify the fixes. Existing tests (`test_upsampler_issue_385.cpp`, `test_ffb_engine.cpp`) updated to align with correct physical behavior.

### Feedback:
- Technical implementation is excellent and resolves the reported bugs.
- Mandatory deliverables like `VERSION` bump, `CHANGELOG_DEV.md` update, and code review logs were missing from the initial assessment.

### Actions Taken:
- Incremented version to 0.7.197.
- Updated `CHANGELOG_DEV.md`.
- Created this review log.

### Rating: Greenlight (Technical implementation approved)
