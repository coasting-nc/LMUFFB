---
name: architecting_and_planning
description: Guidelines for creating or updating comprehensive implementation plans
---

# Architecting and Planning

Use this skill BEFORE implementing any complex feature or bugfix to bridge requirements and code.

## 📐 Implementation Plan Structure
Create or update your plan in `docs/dev_docs/plans/plan_[feat/issue].md`.

### 1. Context & Analysis
- **Codebase Analysis**: Thoroughly review impacted files (e.g., `FFBEngine.h`, `Config.cpp`).
- **Data Flow Analysis**: Understand how telemetry or settings reach their destination.
- **FFB Impact**: If FFB is involved, document how effects (Understeer, Road Feel) and the user's "feel" will change.

### 2. Design Rationale (MANDATORY)
Explicitly document the "Why" behind your architecture. Justify choices:
- Why this algorithm over another?
- Why this file for placement?
- How does it maintain reliability and safety?

### 3. Proposed Changes
- **Parameter Synchronization Checklist**: Ensure new settings are added to:
  - FFBEngine member variables
  - `Config.h` (Preset struct)
  - `Apply()` and `UpdateFromEngine()`
  - `Save()` and `Load()` routines.
- **Refactoring Strategy**: Use aliases if extracting types from monolithic headers to maintain compatibility.

### 4. TDD Test Plan
Define tests **BEFORE** code.
- Provide test names and expected (failing) assertions.
- Specify multi-frame data progression for filters or stateful logic.
- Plan for empty, full, and wraparound buffer scenarios.

### 5. Post-Implementation Documentation (CRITICAL)
**The task is NOT finished when tests pass.** It is only finished when the Implementation Plan document accurately reflects the reality of the implementation in the **"Implementation Notes"** section.
- **Success Bias Mitigation**: Do not skip procedural deliverables (docs) just because technical deliverables (code/tests) are done.
- **Mandatory Notes**: Document:
  - Unforeseen issues encountered.
  - Deviations from the original plan.
  - Challenges and recommendations for the future.
