# Gemini CLI Orchestrator - Implementation Plan

## 0. Iterative Development Strategy (Dogfooding)
This project will not be built in a single "Big Bang". Instead, we will build a minimal viable version and immediately use it to perform real work.

**The Loop:**
1.  **Build v0.1:** A simple script that hardcodes the prompts and subprocess calls.
2.  **Live Test:** Attempt to implement a trivial feature (e.g., "Add a timestamp to the log file") using *only* the Orchestrator.
3.  **Analysis:** Review the logs. Did the agent output JSON correctly? Did the file get created?
4.  **Refactor:** Fix the Orchestrator code based on these findings.
5.  **Repeat:** Move to v0.2 with state management, then test a slightly harder feature.

---

## Phase 1: Prototype (Day 1)
**Goal:** Prove the concept of wrapping the Gemini CLI and extracting JSON.

### Task 1.1: The `AgentWrapper`
*   Create `src/wrapper.py`.
*   Implement `run_gemini_command(prompt: str) -> str`.
*   Use `subprocess.run` to call `gemini`.
*   **Test:** Call the CLI to say "hello" and ensure we capture stdout.

### Task 1.2: JSON Parsing
*   Create `src/parser.py`.
*   Implement `extract_json(raw_text: str) -> dict`.
*   **Test:** Feed it dummy text with embedded JSON and verify extraction.

### Task 1.4: Response Validation (Schemas)
*   Create `src/schemas.py`.
*   Define Pydantic models for each agent output:
    *   `InvestigatorResult(report_path: str)`
    *   `GatekeeperResult(verdict: Literal["APPROVE",...])`
    *   `PlanResult(plan_path: str)`
    *   `DeveloperResult(commit_hash: str)`
    *   `ReviewResult(verdict: str)`
*   Implement `validate_response(raw_text: str, schema: Type[BaseModel]) -> BaseModel`.

### Task 1.5: Single-Step Orchestration
*   Create `orchestrator_v0.py`.
*   Hardcode a simple task: "Create a file named `test.txt` with content 'Hello World' and return JSON."
*   Verify the file is created and the JSON confirms it.

## Phase 2: Core Logic (Day 2)
**Goal:** Implement the State Machine and distinct roles.

### Task 2.1: State Manager
*   Create `src/state.py` using a simple Data Class or Dictionary.
*   Functions to save/load state to `workflow_state.json`.

### Task 2.2: Prompt Templates
*   Create `templates/` directory.
*   `investigator_prompt.txt`: Instructions for bug diagnosis.
*   `researcher_prompt.txt`: Instructions for feature research.
*   `analyst_gatekeeper_prompt.txt`: Instructions for reviewing reports/escalating.
*   `architect_prompt.txt`: Instructions for planning. **MUST include:**
    *   **Codebase Analysis:** Architect must review current codebase, identify impacted functionalities, and trace data flows.
    *   **FFB Effect Impact Analysis:** For FFB-related tasks, document affected effects (understeer, oversteer, lockup, etc.) from both technical (files, functions, data changes) and user (FFB feel, UI settings, presets) perspectives.
    *   **User Settings & Presets Impact:** Migration logic considerations.
    *   **Parameter Synchronization Checklist:** For new settings, document declaration locations, Apply/UpdateFromEngine entries, Save/Load entries.
    *   **Initialization Order Analysis:** For cross-header changes, document circular dependencies and constructor placement.
    *   **Boundary Condition Tests:** For buffer-based algorithms, include empty/partial/full/wraparound tests.
*   `plan_reviewer_prompt.txt`: Instructions for validating the plan. **MUST verify:**
    *   Codebase analysis section is complete.
    *   FFB effect impact is documented (if applicable).
    *   Parameter synchronization checklist is complete (if adding settings).
    *   Initialization order analysis is included (if cross-header changes).
    *   Boundary condition tests are included (for buffer algorithms).
    *   Test plan is TDD-ready.
*   `developer_prompt.txt`: Instructions for TDD-based coding (includes JSON schema). **MUST include:**
    *   TDD cycle (Red-Green phases).
    *   Append "Implementation Notes" section to plan documenting unforeseen issues and challenges.
*   `auditor_prompt.txt`: Instructions for reviewing code. **MUST verify:**
    *   Code correctness, style, tests, and safety.
    *   No unintended deletions of code, comments, tests, or documentation.

### Task 2.4: Integration Specialist
*   Create `integration_specialist_prompt.txt`: Instructions for resolving git merge conflicts.
*   "You are an expert at resolving git conflicts. Keep the feature changes unless they break core logic."

### Task 2.5: The Pipeline Loop
*   Update `orchestrator.py` to chain the steps:
    -1. **Initialization:** `create_branch("task/...")`
    0.  **Phase 0 (Dynamic):**
        *   If `bugfix`: `Report = run_step(Investigator)` -> `git_commit(Report)`
        *   If `research` or `escalated`: `Report = run_step(Researcher)` -> `git_commit(Report)`
        *   `Verdict = run_step(Gatekeeper, input=Report)` -> `git_commit(Verdict)` -> Loop/Escalate/Approve.
    1.  `Plan = run_step(Architect, input=Reports)` -> `git_commit(Plan)`
    2.  `PlanVerdict = run_step(PlanReviewer, input=Plan)` -> `git_commit(PlanVerdict)` -> Loop if Rejected.
    3.  `Result = run_step(Developer, input=Plan)` (Developer follows TDD: tests first, verify fail, implement, verify pass)
    4.  `CodeVerdict = run_step(Auditor, input=Result)` -> `git_commit(CodeVerdict)` -> Loop if Failed.
    5.  **Phase D:**
        *   `git merge main`
        *   If conflict: `run_step(IntegrationSpecialist)` -> Commit.
        *   `git push` -> Create PR.

## Phase 3: Robustness (Day 3)
**Goal:** Error handling and Git integration.

### Task 3.1: Git Helper
*   Create `src/git_utils.py`.
*   Implement `create_branch`, `get_diff`, `commit_exists`.

### Task 3.2: Document Archiver
*   Create `src/archiver.py`.
*   Implement `archive_artifacts(feature_name: str)`.
*   Ensure it moves files from `docs/dev_docs/plans/` to `docs/dev_docs/archived/plans/`.

### Task 3.4: Remote Safety (GitHub Integration)
*   Create `src/github_utils.py`.
*   Implement `verify_branch_protection(branch="main")`.
*   Use `subprocess` to call `gh api repos/:owner/:repo/branches/main/protection` and check for `required_pull_request_reviews`.
*   If `gh` CLI is not installed or the check fails, prompt the user to manually verify or skip.

## Phase 4: Polish & CLI (Day 4)
**Goal:** Make it usable for the end user.

### Task 4.1: CLI Arguments
*   Use `argparse` to accept `--task`, `--resume`, `--verbose`.

### Task 4.2: Logging
*   Implement a logger that writes `logs/session_[timestamp].log`.

## Definition of Done
The system is ready when I can run:
`python orchestrator.py --task "Add a new 'About' page"`
...and it autonomously:
1.  Creates a branch immediately.
2.  Creates a plan (and commits it).
3.  Implements the code (and commits it).
4.  Reviews the code (and commits the report).
5.  Creates a Pull Request / Merge Request.
