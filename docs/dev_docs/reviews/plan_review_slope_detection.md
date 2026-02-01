# Plan Review: Slope Detection Algorithm for Dynamic Grip Estimation

**Plan Path:** `docs/dev_docs/plans/plan_slope_detection.md`
**Review Date:** 2026-02-01
**Reviewer:** Lead Architect (Plan Reviewer)

---

## Executive Summary

| Verdict | **✅ APPROVE** |
|---------|----------------|
| **Plan Quality** | Excellent |
| **Completeness** | High |
| **TDD-Readiness** | Good |
| **Risk Level** | Low (Feature disabled by default) |

The implementation plan for the Slope Detection Algorithm is **well-structured, comprehensive, and ready for implementation**. It demonstrates a thorough understanding of the codebase, provides detailed technical specifications, and includes adequate safeguards for backward compatibility. Minor suggestions for improvement are noted below but do not warrant rejection.

---

## Review Criteria Assessment

### 1. ✅ Completeness

| Requirement | Status | Notes |
|-------------|--------|-------|
| Context/Problem Statement | ✅ Complete | Clear explanation of the problem (static thresholds) and proposed solution (dynamic derivative-based detection) |
| Reference Documents | ✅ Complete | All 4 source documents properly referenced |
| Codebase Analysis | ✅ Complete | Detailed analysis of `calculate_grip()`, `FFBEngine.h`, and `Config.h` |
| FFB Effect Impact Analysis | ✅ Complete | Comprehensive table of affected/unaffected effects |
| Implementation Details | ✅ Complete | Code snippets with line numbers for all changes |
| Test Plan | ✅ Complete | 8 unit tests with inputs, outputs, and assertions |
| Deliverables Checklist | ✅ Complete | Itemized checklist with line count estimates |
| Documentation Updates | ✅ Complete | CHANGELOG, VERSION, README mentioned |

**Assessment:** The plan covers all requirements from `architect_prompt.md` and `01_specifications.md`.

---

### 2. ✅ Codebase Analysis Quality

The plan demonstrates **strong codebase analysis**:

| Aspect | Evidence |
|--------|----------|
| **Impacted modules identified** | `FFBEngine.h`, `Config.h`, `Config.cpp`, `GuiLayer.cpp` clearly listed |
| **Functions/classes identified** | `calculate_grip()` (lines 593-678), `GripResult` struct, `FFBCalculationContext`, `Preset` struct |
| **Data flow documented** | Clear flow diagram: Telemetry → Grip Calculation → Effect Calculations → Output |
| **Existing functionality impact** | Section 2.4 clearly shows the branching logic: new code path for slope detection *alongside* preserved static threshold logic |

**Key Insight Documented:** The plan correctly identifies that slope detection replaces **steps 2c/2d/2e** of the grip calculation fallback path (line 78), not the entire function.

---

### 3. ✅ FFB Effect Impact Analysis

The plan includes an **exemplary FFB effect impact analysis** (Section: "FFB Effects Impact Analysis"):

| Category | Effects Listed | Technical Impact | User Impact |
|----------|----------------|------------------|-------------|
| **Front Grip Consumers** | Understeer Effect, Slide Texture | ✅ Functions and line numbers provided | ✅ "Smoother and more progressive" feel described |
| **Rear Grip Consumers** | Lateral G Boost (Oversteer) | ✅ Differential calculation explained | ✅ "Better oversteer detection in mixed conditions" |
| **Slip Angle Derived** | Rear Aligning Torque | ✅ Noted as unaffected | ✅ Counter-steering cues remain accurate |
| **Unaffected Effects** | Steering Shaft, Yaw Kick, Gyro, Lockup, Road Texture, ABS, etc. | ✅ Clear list with rationale | ✅ No changes expected |

**User Experience Summary Table (Section 5):**
- Clear before/after comparison (static vs. slope detection)
- Explicitly notes what will change and what will NOT change

**Assessment:** This section meets and exceeds the requirements in `architect_prompt.md` (Step 2: FFB Effect Impact Analysis).

---

### 4. ✅ Testability (TDD-Ready)

The plan includes **8 unit tests** with sufficient detail for TDD:

| Test | Purpose | Inputs | Expected Outputs | Assertions |
|------|---------|--------|------------------|------------|
| `test_slope_detection_buffer_init` | Buffer initialization | Fresh engine | `count == 0, index == 0, current == 0.0` | ✅ Clear |
| `test_slope_sg_derivative` | SG derivative accuracy | Linear ramp, window=9, dt=0.01 | `derivative ≈ 10.0 ±1.0` | ✅ Clear |
| `test_slope_grip_at_peak` | Zero slope = full grip | Constant G=1.2, constant slip | `output > 0.95` | ✅ Clear |
| `test_slope_grip_past_peak` | Negative slope = grip loss | Decreasing G, increasing slip | `0.2 < output < 0.9` | ✅ Clear |
| `test_slope_vs_static_comparison` | Both methods detect loss | Same telemetry, 12% slip | Both detect grip loss | ✅ Clear |
| `test_slope_config_persistence` | Save/load cycle | Non-default values | Loaded == saved | ✅ Clear |
| `test_slope_latency_characteristics` | Buffer fill timing | window=15, dt=0.0025 | `fill_count == window_size` | ✅ Clear |
| `test_slope_noise_rejection` | SG filter rejects noise | Constant G + noise | `|slope| < 1.0` | ✅ Clear |

**Assessment:** Tests are described to a level where a developer can write them BEFORE implementation (TDD Red Phase).

---

### 5. ✅ Clarity & Implementability

| Aspect | Assessment |
|--------|------------|
| **Code locations** | ✅ Specific line numbers provided (e.g., "line 312", "~line 1100") |
| **Code snippets** | ✅ C++ pseudocode provided for all major changes |
| **Naming conventions** | ✅ Consistent prefix `m_slope_*` and `slope_*` |
| **Configuration parameters** | ✅ Table with types, defaults, and descriptions |
| **GUI layout** | ✅ ImGui tree structure with conditional visibility |
| **Validation logic** | ✅ Range clamps and odd-number enforcement documented |
| **Backward compatibility** | ✅ Feature OFF by default, static logic preserved |

**Assessment:** A developer can implement this plan without additional clarification.

---

### 6. ✅ Safety & Risk Assessment

| Risk Area | Mitigation | Assessment |
|-----------|------------|------------|
| **Breaking existing behavior** | `slope_detection_enabled = false` by default | ✅ Low risk |
| **Complete FFB loss** | Grip floor of 0.2 documented (line 588) | ✅ Safe |
| **Config corruption (old files)** | Missing keys use defaults, no migration needed | ✅ Safe |
| **Buffer overflow** | `SLOPE_BUFFER_MAX = 41`, window clamped to [5, 41] | ✅ Safe |
| **Division by zero** | Not applicable (dt from telemetry, window guaranteed ≥5) | ✅ Safe |

---

### 7. User Settings & Presets Impact

| Aspect | Status | Notes |
|--------|--------|-------|
| **New settings defined** | ✅ Yes | 5 new parameters in Preset struct |
| **GUI section designed** | ✅ Yes | "Slope Detection (Experimental)" tree node |
| **Migration logic needed** | ✅ No | Defaults apply to missing keys |
| **Preset updates required** | ✅ Yes | All presets get new settings (disabled by default) |
| **Settings interaction documented** | ✅ Yes | "Optimal Slip Angle is IGNORED when enabled" (tooltip) |

**Assessment:** The "User Settings & Presets Impact" section fully addresses requirements from `auditor_prompt.md`.

---

## Minor Suggestions (Non-Blocking)

The following are **optional improvements** that do not affect the approval status:

### 1. ✅ ~~Savitzky-Golay Coefficient Source~~ (RESOLVED)

~~The plan mentions "pre-computed coefficients for quadratic polynomial fit" but does not specify:~~
- ~~How coefficients are computed (runtime vs. compile-time)~~
- ~~Reference for SG coefficient formulas~~

**Resolution (2026-02-01):** A comprehensive Savitzky-Golay Coefficients Deep Research Report has been created at `docs/dev_docs/plans/savitzky-golay coefficients deep research report.md`  and `docs/dev_docs/plans/savitzky-golay coefficients deep research report(2).md`. The implementation plan has been updated to include:
- The closed-form coefficient generation formula: `w_k = k / (S_2 × h)` where `S_2 = M(M+1)(2M+1)/3`
- Full C++ implementation of `calculate_sg_derivative()` with algorithm comments
- Boundary handling strategy (shrinking symmetric window)
- Reference to the deep research report

### 2. Edge Case: Very Low Speed

The plan notes that Slope Detection is active when `mGripFract` is zero (same condition as static threshold fallback). Consider explicitly documenting behavior at:
- Standstill (speed = 0)
- Very low speeds (< 5 km/h)

The existing code likely already handles this via speed gate, but a note would improve clarity.

### 3. Diagnostic Logging

Consider adding a debug log message when slope detection mode is activated/deactivated, to help users troubleshoot configuration issues.

### 4. Optional: Experimental Preset

The plan mentions "An experimental preset could be added" but does not include it in the deliverables checklist. Consider:
- Adding one experimental preset for testing purposes
- Or explicitly noting this is deferred to a future version

---

## Compliance Matrix

| Document Requirement | Compliance |
|----------------------|------------|
| `01_specifications.md` §3.6 - Artifact Interface | ✅ Plan includes all required sections |
| `02_architecture_design.md` §3 - Data Flow | ✅ Codebase analysis traces data flow |
| `architect_prompt.md` Step 1 - Codebase Analysis | ✅ Impacted modules, functions, and data flows documented |
| `architect_prompt.md` Step 2 - FFB Effect Impact | ✅ Affected effects listed with technical and user-facing impact |
| `architect_prompt.md` Step 3 - User Settings Impact | ✅ New settings, migration, and preset impact documented |
| `architect_prompt.md` Step 4 - TDD-Ready Test Plan | ✅ 8 tests with inputs, outputs, and assertions |
| `plan_reviewer_prompt.md` - Reject Reasons Check | ✅ None of the common rejection reasons apply |

---

## Conclusion

The **Slope Detection Algorithm Implementation Plan** is **approved for implementation**. The document is thorough, technically sound, and provides sufficient detail for a developer to follow TDD practices. The feature's conservative default (disabled) and preservation of the static threshold logic ensure low risk to existing users.

**Next Step:** Proceed to **Phase B: Developer (Implementation)**.

---

```json
{
  "verdict": "APPROVE",
  "review_path": "docs/dev_docs/reviews/plan_review_slope_detection.md",
  "feedback": "Excellent implementation plan. Comprehensive codebase and FFB effect analysis, TDD-ready test plan, and clear implementation details. Minor suggestions for SG coefficient documentation and edge case handling are optional. Ready for development."
}
```
