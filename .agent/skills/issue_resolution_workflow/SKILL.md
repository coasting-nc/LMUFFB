---
name: issue_resolution_workflow
description: Systematic process for resolving bugs and issues in the codebase (Fixer role)
---

# Issue Resolution Workflow (Fixer)

Use this skill when tasked with fixing bugs or resolving specific issues. Focus on systematic execution and autonomous progress.

## 🛠️ The Fixer Process

### 🔍 1. Triage & Select
- Identify **ONE** open issue.
- Focus on exactly one issue to ensure isolation of concerns.
- Document the issue number and title in your implementation plan.

### 📐 2. Architect (Plan)
- ALWAYS create or update an implementation plan in `docs/dev_docs/plans/`.
- Reuse existing plans if present; explain deletions or modifications.
- Include mandatory **Design Rationale** blocks to justify your decisions.

### 🔧 3. Develop & Iterate (The Loop)
1. **Implement**: Follow TDD principles (write/update test first, verify failure).
2. **Build & Test**: Run `cmake --build build`. Fix all errors before review.
3. **Commit**: Save intermediate work (e.g., `git commit -am "WIP: Iteration N"`).
4. **Independent Code Review**: Use the code review tool.
   - Save output to `docs/dev_docs/code_reviews/review_iteration_X.md`.
   - If it fails, address feedback and loop back to step 1.
   - If it passes (Greenlight), break the loop.

### 📝 4. Document & Finalize
- Update the implementation plan with final notes on deviations and issues.
- Prepare a PR/Submission description highlighting safety impact and Linux verification.

## ⚠️ Key Constraints
- **Autonomous Progress**: Do not stop for confirmation; loop autonomously until the code is "Greenlighted".
- **Linux/Windows Hybrid**: Build and test using Linux commands with project mocks.
- **Reliability Standards**: Use `std::isfinite` and `std::clamp` to sanitize inputs and outputs.
- **Progress Preservation**: Commit BEFORE potentially destructive operations (e.g., `git reset`).
