# Gemini CLI Orchestrator - Architecture & Design

## 1. System Architecture

The system follows a **Controller-Worker** architecture.

```mermaid
graph TD
    User[User Terminal] -->|Starts| Orch[Python Orchestrator]
    Orch -->|Reads| Config[Workflow Config (JSON)]
    Orch -->|Manages| State[State Manager]
    Orch -->|Enforces| Sec[Security Middleware]
    
    Orch -->|Spawns| P0A[P0: Investigator]
    
    subgraph "Execution Pipeline"
         Agent[AI Agent] -->|Requests Tool| Sec
         Sec -->|Validates| OS[Operating System]
         Sec -.->|Blocks| Agent
    end
    
    Orch -->|Spawns| P0B[P0: Researcher]
    Orch -->|Spawns| P0C[P0: Lead Analyst]
    Orch -->|Spawns| P1[P A.1: Architect]
    Orch -->|Spawns| P2[P A.2: Lead Architect]
    Orch -->|Spawns| P3[P B: Developer]
    Orch -->|Spawns| P4[P C: Auditor]
    Orch -->|Spawns| P5[P D: Integration Specialist]
    
    subgraph "Isolation Boundary"
        P0A -- Writes --> Art0[Diagnostic Report]
        P0B -- Writes --> Art0B[Research Report]
        P0C -- Reads --> Art0
        P0C -- Reads --> Art0B
        P0C -- Verdict --> Orch
        P1 -- Reads --> Art0
        P1 -- Reads --> Art0B
        P1 -- Writes --> Art1[Plan Artifact]
        P2 -- Reads --> Art1
        P2 -- Verdict --> Orch
        P3 -- Reads --> Art1
        P3 -- Commits --> Git[Git Repository]
        P4 -- Reads --> Art1
        P4 -- Reads --> Git
        P4 -- Verdict --> Orch
        P5 -- Reads --> Git
        P5 -- Resolves --> Git
    end

    %% Feedback Loops
    Orch -.->|Reject| P0A
    Orch -.->|Escalate| P0B
    Orch -.->|Reject| P1
    Orch -.->|Fail| P3
    Orch -.->|Conflict| P5
```

## 2. Component Design

### 2.1 The `AgentWrapper` Class
This class encapsulates the interface with the Gemini CLI.
*   **Responsibilities:**
    *   Constructing the full shell command.
    *   Managing `stdin`/`stdout` piping.
    *   Injecting the "System Prompt" (e.g., "You are a rigid worker. Output JSON only.").
    *   Handling timeouts and process cleanup.

### 2.2 The `ToolValidator` (Security Middleware)
Sits between the `AgentWrapper` and the OS.
*   **Responsibilities:**
    *   Intercepts every `run_shell_command` request.
    *   Parses the command string (e.g., using `shlex`).
    *   **Policy Enforcement:**
        *   **Blocked:** `git checkout`, `git switch`, `git branch` (creation/deletion), `git push` (except to origin/feature-branch).
        *   **Allowed:** `git status`, `git add`, `git commit`, `git diff`, `ls`, `cat`, `grep`.
    *   Raises `SecurityException` if a violation is detected, preventing execution.

### 2.3 The `WorktreeManager` (Isolation Layer)
Responsible for physical file-system isolation.
*   **Concept:** Instead of running in the user's main directory, each Task runs in a dedicated Git Worktree.
*   **Location:** `../.gemini_worktrees/[task_id]/` (Outside the main repo folder to avoid recursion).
*   **Lifecycle:**
    *   `setup_task(branch_name)`: Runs `git worktree add ...`.
    *   `cleanup_task()`: Runs `git worktree remove ...`.
*   **Benefit:** The Agent cannot accidentally modify uncommitted files in the user's main working copy.

### 2.4 The `PromptBuilder` Module
Responsible for assembling the final prompt string sent to the agent.
*   **Logic:** `Base Prompt` + `Task Context` + `Input Artifacts` + `Output Instructions`.
*   **Example:**
    > "CONTEXT: You are the Developer.
    > INPUT: Read the plan at 'docs/plans/fix_v1.md'.
    > TASK: Implement the code. Run tests.
    > FORMAT: End your response with JSON: { 'status': 'success', ... }"

### 2.5 The `ResponseValidator` Module
Responsible for extracting and validating structured data from the unstructured LLM output.

*   **Extraction Logic (Fuzzy):**
    1.  Look for Markdown code blocks tagged `json`.
    2.  If missing, search for the last outermost `{` and `}` pair.
    3.  `json.loads()` the extracted substring.
*   **Validation Logic (Strict):**
    *   Uses **Pydantic Models** to enforce schemas (e.g., `class InvestigatorResult(BaseModel)`).
    *   Checks for required fields (e.g., `report_path`, `verdict`).
    *   Raises `ValidationError` if the output is malformed.
*   **Error Handling:**
    *   If validation fails, the Orchestrator captures the error message and sends it back to the Agent in a "Retry" prompt, allowing the Agent to self-correct.

### 2.4 The `WorkflowEngine` Class
The main state machine.
*   **State:**
    *   `current_step`: (enum: INVESTIGATE, RESEARCH, ANALYST_REVIEW, PLAN, PLAN_REVIEW, CODE, CODE_REVIEW)
    *   `mode`: (bugfix, research, direct)
    *   `artifacts`: Dictionary of paths.
*   **Transitions (Phase 0):**
    *   `START` -> `INVESTIGATE` (if bugfix)
    *   `START` -> `RESEARCH` (if research)
    *   `START` -> `PLAN` (if direct)
    *   `INVESTIGATE` -> `ANALYST_REVIEW`
    *   `RESEARCH` -> `ANALYST_REVIEW`
    *   `ANALYST_REVIEW` -> `PLAN` (Approve)
    *   `ANALYST_REVIEW` -> `RESEARCH` (Escalate)
    *   `ANALYST_REVIEW` -> `INVESTIGATE/RESEARCH` (Reject/Loop)

## 3. Data Flow

### Phase 0: Analysis (Dynamic)
1.  **Orchestrator** checks `--mode`.
2.  **Orchestrator** creates and switches to branch `task/[id]-[desc]`.
3.  **Orchestrator** spawns **Investigator** (Bug) or **Researcher** (Feature).
4.  **Agent** produces Report.
5.  **Orchestrator** commits the Report.
6.  **Orchestrator** spawns **Lead Analyst**.
    *   *If ESCALATE:* Trigger **Researcher** (if coming from Investigator).
    *   *If APPROVE:* Proceed to Phase A.
    *   **Orchestrator** commits the Verdict/Feedback.

### Phase A: Planning
1.  **Orchestrator** spawns **Architect**.
    *   *Input:* User Request + Any Reports from Phase 0.
2.  **Agent** performs **Codebase Analysis**:
    *   Reviews existing architecture and identifies impacted functionalities.
    *   Traces data flows through affected areas.
    *   Documents which modules/functions will be affected.
3.  **Agent** performs **FFB Effect Impact Analysis** (if FFB-related task):
    *   Identifies all affected FFB effects (understeer, oversteer, lockup, ABS, road texture, etc.).
    *   Documents technical impact (files, functions, data changes).
    *   Documents user-facing impact (FFB feel changes, UI settings, preset adjustments).
4.  **Agent** creates **Parameter Synchronization Checklist** (if adding settings):
    *   Lists declaration locations, Apply/UpdateFromEngine entries, Save/Load entries, validation.
5.  **Agent** performs **Initialization Order Analysis** (if cross-header changes):
    *   Documents circular dependency implications and constructor placement.
6.  **Agent** writes `docs/dev_docs/plans/feature_X.md` including the analysis sections.
7.  **Orchestrator** commits the Plan.
8.  **Orchestrator** spawns **Lead Architect (Plan Reviewer)**.
    *   *Input:* The Plan File.
    *   *Verification:* Plan includes complete codebase analysis, FFB effect impact (if applicable), parameter synchronization (if applicable), and initialization order analysis (if applicable).
9.  **Agent** outputs JSON: `{"verdict": "APPROVE"}` or `{"verdict": "REJECT", "feedback": "..."}`.
    *   *If REJECT:* Loop back to Architect with feedback. Common rejection reasons:
        *   Missing/incomplete codebase analysis.
        *   Missing/incomplete FFB effect impact analysis.
        *   Missing parameter synchronization checklist (when adding settings).
        *   No initialization order analysis (when changes span headers).
        *   Missing boundary condition tests (for buffer algorithms).
        *   Test cases not TDD-ready.
10. **Orchestrator** commits the Review Verdict.

### Phase B: Implementation (TDD)
1.  **Orchestrator** reads Approved Plan.
2.  **Orchestrator** spawns **Developer**.
3.  **Agent** follows TDD:
    *   Writes tests first (based on the Plan's Test Plan section).
    *   Runs new tests to verify they fail (Red Phase).
    *   Implements minimum code to make tests pass (Green Phase).
    *   Runs full test suite to verify no regressions.
4.  **Agent** documents implementation issues:
    *   Appends an "Implementation Notes" section to the Implementation Plan.
    *   Documents unforeseen issues, plan deviations, and challenges encountered.
5.  **Agent** commits changes to git.
6.  **Agent** prints JSON: `{"commit_hash": "abc1234", "tests_passed": true}`.

### Phase C: Review
1.  **Orchestrator** spawns **Auditor**.
    *   *Input:* Plan + Commit Hash.
    *   *Verification:* Check for unintended deletions (code, comments, tests, documentation).
2.  **Agent** writes `docs/dev_docs/reviews/review_X.md`.
3.  **Orchestrator** commits the Review Report.
4.  **Agent** prints JSON: `{"verdict": "PASS"}` or `{"verdict": "FAIL"}`.
    *   *If FAIL:* Loop back to Developer with Review Report.

### Phase D: Integration & Delivery
1.  **Orchestrator** attempts `git merge main` into current branch.
    *   *If Conflict:* **Orchestrator** spawns **Integration Specialist**.
    *   **Agent** resolves conflicts and commits.
2.  **Orchestrator** spawns **Auditor** (Merge Review).
    *   *Goal:* Ensure integrity after merge.
3.  **Orchestrator** pushes branch to remote.
4.  **Orchestrator** creates Pull Request (via API or prints URL).
5.  **Orchestrator** moves artifacts to `docs/dev_docs/archived/`.

## 4. Key Decisions & Trade-offs

*   **No "Memory" by default:** We explicitly choose NOT to pass the chat history. If the Developer needs to know *why* the Architect made a decision, they must read the Plan document. This forces better documentation.
*   **Polling vs. Blocking:** Since we wrap the process, we use **Blocking** calls (waiting for the subprocess to finish) rather than Polling. The Agent script itself is responsible for the "Loop" of running tests until they pass.
*   **Error Handling:** If the Agent fails to output JSON, the Orchestrator will assume failure and ask the user for manual intervention or a retry.


