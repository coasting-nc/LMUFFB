# Plan Review: Slope Detection Algorithm Fixes (v0.7.1)

## Review Metadata

| Field | Value |
|-------|-------|
| **Plan Document** | `docs/dev_docs/implementation_plans/plan_slope_detection_fixes_v0.7.1.md` |
| **Investigation Report** | `docs/dev_docs/investigations/slope_detection_issues_v0.7.0.md` |
| **Review Date** | 2026-02-02 |
| **Target Version** | 0.7.1 |
| **Reviewer** | Lead Architect (Plan Reviewer) |

---

## Review Summary

| Criterion | Status | Notes |
|-----------|--------|-------|
| **Completeness** | ✅ PASS | All user-reported issues addressed |
| **Safety** | ✅ PASS | Changes are backward-compatible, no risky refactors |
| **Codebase Analysis** | ✅ PASS | Thorough analysis with line numbers and data flow |
| **FFB Effect Impact** | ✅ PASS | All affected effects documented with technical and UX impact |
| **Testability (TDD-Ready)** | ✅ PASS | Test cases include inputs, expected outputs, and assertions |
| **Parameter Synchronization** | ✅ PASS | Checklist provided, no new params (only default changes) |
| **Initialization Order** | ✅ PASS | Correctly identified no cross-header init issues |
| **Boundary Condition Tests** | ⚠️ N/A | Correctly noted as not applicable (no new buffers) |
| **Clarity** | ✅ PASS | Unambiguous implementation steps |

---

## Detailed Review

### 1. Completeness

The plan addresses all issues identified in the investigation report:

| Investigation Issue | Plan Solution | Coverage |
|---------------------|---------------|----------|
| Issue 1: Grip fluctuation at low speeds | New default parameters (sens=0.5, thresh=-0.3, tau=0.04) | ✅ Complete |
| Issue 2: Lateral G Boost interaction | Disable boost when slope detection enabled (Section 1) | ✅ Complete |
| Issue 3: Heavy/Notchy FFB | Parameter adjustments + increased smoothing | ✅ Complete |
| Issue 4.1: Filter Window slider layout | **NOT ADDRESSED** - See Minor Issue #1 | ⚠️ Minor Gap |
| Issue 4.2: Missing tooltip on Filter Window | Added tooltip (Section 3.1) | ✅ Complete |
| Issue 4.3: Missing tooltip on checkbox | Investigation confirmed this is NOT an issue | ✅ N/A |
| Issue 4.4: Latency display location | Maintained in current form | ✅ Complete |
| Issue 5: Grip graph | Correctly uses existing `calc_front_grip` graph | ✅ Complete |
| Issue 6: Live Slope display → Graph | Added slope graph (Section 3.2) | ✅ Complete |

**Minor Issue #1:** The investigation report (Section 4.1) identified that the Filter Window slider doesn't follow the two-column layout pattern and has `NextColumn()` issues. The implementation plan's Section 3.1 adds a tooltip but does **not** address the layout bug itself.

> **Recommendation:** This is a minor cosmetic issue and does NOT warrant rejection. Consider adding a follow-up task for v0.7.2 or handling it as a low-priority fix.

### 2. Safety Assessment

| Change | Risk Level | Mitigation |
|--------|------------|------------|
| Disable Lateral G Boost when slope detection ON | **Low** | Preserves existing behavior when slope detection OFF |
| Default parameter changes | **Low** | Only affects fresh installs/reset; existing configs preserved |
| New snapshot field | **Very Low** | Simple struct addition, no behavior change |
| UI tooltip additions | **Very Low** | Informational only |
| UI warning text | **Very Low** | Informational only |
| New graph buffer | **Low** | Standard pattern already used for other graphs |

**No high-risk changes identified.** All changes are backward-compatible.

### 3. Codebase Analysis Quality

The plan includes:

- ✅ **File locations with line numbers**: All code references include specific line numbers
- ✅ **Current architecture table**: Clear mapping of files → purpose → lines of interest
- ✅ **Impacted functionalities table**: Documents what uses `ctx.avg_grip` and how
- ✅ **Data flow diagram**: Step-by-step flow from telemetry to FFB output (lines 63-81)
- ✅ **Key finding highlighted**: Asymmetry between front/rear grip calculation correctly identified

**Assessment:** Excellent codebase analysis. A developer can understand the context without additional exploration.

### 4. FFB Effect Impact Analysis

The plan correctly categorizes effects:

#### Effects Using Front Grip (`ctx.avg_grip`)
- Understeer Effect: ✅ Documented (no code change needed)
- Slide Texture: ✅ Documented (no code change needed)

#### Effects Using Grip Differential
- Lateral G Boost: ✅ **CRITICAL FIX** clearly marked with technical and UX impact

#### Effects NOT Impacted
- ✅ Correctly lists 8 effects that are unaffected (Steering Shaft Gain, Rear Aligning Torque, Yaw Kick, etc.)

**Assessment:** FFB effect analysis is comprehensive and follows the template requirements.

### 5. Testability (TDD-Ready)

| Test | Purpose | Input Definition | Expected Output | Assertion |
|------|---------|------------------|-----------------|-----------|
| Test 1 | Lat G Boost disabled with slope detection | ✅ Multi-frame telemetry script | ✅ sop_base unchanged | ✅ `abs(diff) < 0.01` |
| Test 2 | Lat G Boost works without slope detection | ✅ Front/rear grip values | ✅ Boosted by ~120% | ✅ `sop_boosted > sop_unboosted * 1.5` |
| Test 3 | New default values | ✅ Fresh FFBEngine | ✅ Specific values | ✅ Direct equality checks |
| Test 4 | Slope snapshot field | ✅ Multi-frame run | ✅ Matches m_slope_current | ✅ `abs(diff) < 0.001` |
| Test 5 | Less aggressive grip response | ✅ Telemetry producing slope -0.5 | ✅ Grip > 0.8 | ✅ Not floored |

**Assessment:** All 5 tests are TDD-ready with specific inputs, expected outputs, and assertions. A developer can write these tests **before** implementation.

**Bonus:** Test 5 includes a detailed calculation block showing old vs new behavior (lines 450-463).

### 6. Parameter Synchronization

The plan correctly notes that **no NEW parameters are being added** - only default values are changing.

The verification checklist (lines 335-345) includes:
- ✅ FFBEngine.h member variable locations
- ✅ Preset struct locations
- ✅ Apply() entries
- ✅ UpdateFromEngine() entries
- ✅ Save/Load entries
- ✅ Validation logic

For the new `slope_current` snapshot field:
- ✅ Documents where to add to FFBSnapshot
- ✅ Documents snapshot population location
- ✅ Documents graph buffer and render locations

**Assessment:** Synchronization checklist is complete and follows the template.

### 7. Initialization Order Analysis

The plan correctly identifies (lines 349-356):
- No circular dependencies introduced
- Changes are default value adjustments (no code flow changes)
- Conditional check in existing function (single file)
- Simple struct field addition

**Assessment:** Correctly analyzed. No initialization order issues.

### 8. Boundary Condition Tests

The plan states (lines 465-467):
> "Not applicable for this change - no new buffer algorithms introduced. The existing slope detection buffer tests from v0.7.0 remain valid."

**Assessment:** Correct. The changes don't introduce new buffer-based algorithms. The existing buffer tests from v0.7.0 cover the algorithm's boundary conditions.

### 9. Clarity of Implementation Steps

The Deliverables Checklist (lines 491-521) provides:
- ✅ File-by-file breakdown with estimated line counts
- ✅ Code snippets for each change with before/after comparisons
- ✅ Verification steps including manual testing
- ✅ Clear expected behaviors for GUI verification

**Assessment:** A developer can implement this plan without ambiguity.

---

## Technical Validation

### Proposed Algorithm Fix (Section 1)

The conditional to disable Lateral G Boost:
```cpp
if (!m_slope_detection_enabled) {
    double grip_delta = ctx.avg_grip - ctx.avg_rear_grip;
    if (grip_delta > 0.0) {
        sop_base *= (1.0 + (grip_delta * m_oversteer_boost * 2.0));
    }
}
```

**Validation:** ✅ Correct approach. This preserves existing behavior for non-slope-detection users while eliminating the oscillation source for slope detection users.

### Proposed Default Values (Section 2)

| Parameter | Old | New | Validation |
|-----------|-----|-----|------------|
| sensitivity | 1.0 | 0.5 | ✅ Aligned with investigation recommendation (0.3-0.5) |
| threshold | -0.1 | -0.3 | ✅ Aligned with investigation recommendation (-0.3 to -0.5) |
| tau | 0.02 | 0.04 | ✅ Aligned with investigation recommendation (0.04-0.05) |

**Validation:** All new defaults fall within the investigation's recommended ranges.

### Appendix Calculation (lines 544-574)

The validation calculations correctly demonstrate that the new defaults provide:
- 40% less grip loss for the same slope value
- 2x longer transition time (smoother feel)

**Validation:** ✅ Math is correct.

---

## Minor Suggestions (Non-Blocking)

1. **Filter Window Layout Fix:** Consider adding this to the scope or creating a follow-up task.

2. **Future Enhancement Documentation:** Section "Why Not Apply Slope Detection to Rear Wheels?" is excellent context. Consider adding this to the Slope_Detection_Guide.md as a technical FAQ item.

3. **Version Reference in Warning Text:** The UI warning (line 306-311) could include a brief tooltip explaining *why* Lateral G Boost is disabled (tooltip on the warning text).

---

## Verdict

**APPROVED** ✅

The implementation plan is comprehensive, technically sound, and ready for implementation. All critical review criteria are satisfied. The single minor gap (Filter Window layout) is cosmetic and does not affect functionality or user experience significantly.

---

## Next Steps

1. Proceed with implementation following TDD approach:
   - Write the 5 unit tests first (verify they fail)
   - Implement the code changes
   - Verify tests pass and no regressions

2. Update documentation as specified

3. Increment version to 0.7.1

4. (Optional) Create follow-up task for Filter Window layout fix

---

*Review completed: 2026-02-02*
