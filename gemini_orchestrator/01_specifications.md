# Gemini CLI Orchestrator - System Specification

## 1. Overview
The **Gemini CLI Orchestrator** is a Python-based automation tool designed to wrap the Google Gemini CLI. It transforms the conversational agent into a structured, reliable CI/CD-style pipeline for software engineering tasks.

By managing the Gemini CLI as an atomic subprocess, this orchestrator enforces strict **Session Isolation** (preventing context contamination between phases) and provides **Deterministic Control Flow** (replacing fragile manual polling with robust logic).

## 2. Core Philosophy
*   **Pipeline over Chat:** The system treats software development as a series of discrete steps (Plan -> Implement -> Review), not a continuous conversation.
*   **Artifact-Driven Handover:** State is passed between steps exclusively via file paths (e.g., "Review the plan at `docs/plans/feature_A.md`"), not via chat history.
*   **Supervisor Authority:** The Python script is the absolute source of truth for the workflow state. The AI Agent is a "worker" that executes specific tasks on demand.
*   **Strict Role Separation:** Roles are immutable within a task. If the Auditor finds a bug, they *must* reject the task and send it back to the Developer. The Auditor never touches the code.

## 3. Functional Requirements

### 3.1 Process Management
*   **Subprocess Execution:** The system MUST be able to launch `gemini` CLI instances via Python's `subprocess` module.
*   **Headless Operation:** The system SHOULD utilize the CLI's headless mode (if available) or standard I/O piping to interact with the agent without a GUI.
*   **Clean Slate:** Each step in the pipeline MUST run in a fresh OS process or a reset session to ensure zero context leakage.

### 3.2 Workflow Orchestration
The system MUST support the following distinct phases in a linear or looping pipeline:

0.  **Phase Init: Initialization**
    *   **Action:**
        1.  **Remote Safety Check:** Verify (via GitHub API) that the remote `main` branch has **Branch Protection** enabled (specifically "Require Pull Request" and "Block Force Pushes"). If not, warn the user or abort.
        2.  **Worktree Setup:** Create a new feature branch AND check it out into a **dedicated Git Worktree** (e.g., `../.worktrees/task-xyz`).
    *   **Goal:** Ensure physical file-system isolation and verify remote safety nets are in place.

1.  **Phase 0: Analysis Strategy (Dynamic Entry)**
    *   **Modes:**
        *   `--mode bugfix`: Spawns **Investigator** (Internal Focus).
        *   `--mode research`: Spawns **Researcher** (External Focus).
        *   `--mode direct`: Skips Phase 0.
    *   **Phase 0.1: Analysis**
        *   **Investigator:** Uses `codebase_investigator` to diagnose bugs. Output: `diagnostic_report.md`. **(Orchestrator commits artifact)**
        *   **Researcher:** Uses `google_web_search` for theory/feasibility. Output: `research_report.md`. **(Orchestrator commits artifact)**
    *   **Phase 0.2: Lead Analyst (Gatekeeper)**
        *   **Input:** The Report from 0.1.
        *   **Verdict:**
            *   **APPROVE:** Proceed to Architect.
            *   **REJECT:** Loop back to 0.1 with feedback.
            *   **ESCALATE:** Trigger **Researcher** (e.g., if Investigator finds a math issue).
        *   **(Orchestrator commits decision/feedback)**

2.  **Phase A.1: Architect (Planning)**
    *   **Input:** User Request + Any Reports from Phase 0 (Diagnostic and/or Research).
    *   **Goal:** Analyze requirements and produce a Markdown implementation plan.
    *   **Output:** Path to the generated Implementation Plan file.
    *   **(Orchestrator commits plan)**

3.  **Phase A.2: Lead Architect (Plan Review)**
    *   **Input:** The Original Request and the Implementation Plan.
    *   **Goal:** Verify the plan matches the requirements and is technically sound.
    *   **Verdict:**
        *   **APPROVE:** Proceed to Implementation.
        *   **REJECT:** Return to **Phase A.1** with feedback.
    *   **(Orchestrator commits review)**

4.  **Phase B: Developer (Implementation)**
    *   **Input:** Path to the Approved Implementation Plan file.
    *   **Goal:** Write code, run tests, fix errors until tests pass.
    *   **Output:** Git Commit Hash or "Success" status. (Developer performs atomic commits).

5.  **Phase C: Auditor (Code Review)**
    *   **Input:** Path to Implementation Plan file and Git Diff.
    *   **Goal:** Critique changes against project standards.
    *   **Verdict:**
        *   **PASS:** Trigger Phase D.
        *   **FAIL:** Return to **Phase B** (Developer) with the Review Report as new input.
    *   **(Orchestrator commits review report)**

6.  **Phase D: Integration & Delivery**
    *   **Goal:** Ensure the feature branch is up-to-date with `main` and conflict-free.
    *   **Step 1:** Orchestrator attempts to merge `main` into `feature_branch`.
    *   **Step 2 (Conditional):** If conflicts arise, spawn **Integration Specialist**.
        *   **Input:** Conflict Markers + Implementation Plan.
        *   **Action:** Resolve conflicts, run tests to verify integrity.
        *   **Output:** Clean Merge Commit.
    *   **Step 3:** Spawn **Auditor** (Merge Review).
        *   **Input:** The final Merged Codebase.
        *   **Goal:** Verify that the merge (and potential conflict resolution) did not break existing functionality or the new feature.
    *   **Step 4:** Orchestrator pushes branch and creates a Pull Request / Merge Request.

### 3.3 Structured Communication
*   **JSON Enforcement:** The Orchestrator MUST instruct the agent to output key results (status, file paths) in a strict JSON format at the end of its response.
*   **Fuzzy Parsing:** The Orchestrator MUST be able to extract this JSON payload from the raw text output, ignoring conversational filler.

### 3.4 Git Integration
*   **Immediate Branching:** The Orchestrator MUST creates a task-specific branch (e.g., `task/feature-name`) **immediately** upon receiving a request, before any agent interaction occurs. This ensures all generated artifacts (plans, reports) are contained and do not pollute the main branch.
*   **Step-wise Commits:** The Orchestrator MUST perform a git commit after each successful step in the pipeline.
    *   For documentation phases (Investigation, Planning, Review), the Orchestrator commits the generated files.
    *   For the Implementation phase, the Developer agent creates the commits.
    *   This provides a granular "undo" history and crash recovery checkpoints.
*   **Merge & Conflict Management:** 
    *   Before final delivery, the Orchestrator MUST try to merge `main` into the current branch to check for conflicts.
    *   If the merge fails, the **Integration Specialist** agent is spawned to resolve conflicts and commit the fix.
    *   Finally, the Orchestrator creates a **Pull Request** (or local equivalent) rather than pushing directly to `main`.
*   **Safety Rails (Pre-Execution Firewall):**
    *   The Orchestrator MUST implement a **Middleware Layer** that intercepts all tool calls *before* execution.
    *   **Strict Blocking:** Any command attempting to switch context (e.g., `git checkout`, `git switch`) or modify the repository structure (e.g., `git reset`, `git rebase`) outside of the allowed feature branch must be **blocked** immediately. The Agent receives a "Permission Denied" error.
    *   **Allow-list Approach:** Only safe, additive commands (e.g., `git add`, `git commit`, `git status`, `git diff`) are permitted by default.
    *   This ensures it is *technically impossible* for the agent to modify other branches.

### 3.5 Document Management
The system MUST enforce a structured lifecycle for all generated documents:

*   **Active Documents:** Stored in active subfolders during the feature lifecycle.
    *   `docs/dev_docs/research/`
    *   `docs/dev_docs/plans/`
    *   `docs/dev_docs/reviews/`
*   **Archiving:** Upon successful completion (Merge), the Orchestrator MUST move all related artifacts to an archive folder to keep the active directories clean.
    *   `docs/dev_docs/archived/plans/`
    *   `docs/dev_docs/archived/reviews/`
    *   (Optionally prefixed with date/feature-name)

### 3.6 Artifact Interface Definition
The following table defines the exact Inputs and Deliverables for each phase.

| Phase | Agent | Input Artifacts (Provided via Prompt) | Output Deliverables (Must be Created/Updated) | Output JSON Payload |
| :--- | :--- | :--- | :--- | :--- |
| **0.1** | **Investigator** | • User Request String (Bug Report) | • `docs/dev_docs/research/diagnostic_report_[feat].md` | `{"report_path": "..."}` |
| **0.1** | **Researcher** | • User Request String (Feature Idea) | • `docs/dev_docs/research/research_report_[feat].md` | `{"report_path": "..."}` |
| **0.2** | **Lead Analyst** | • Report from Phase 0.1 | • Feedback (if Rejected) | `{"verdict": "APPROVE"}`<br>`{"verdict": "REJECT", "feedback": "..."}`<br>`{"verdict": "ESCALATE"}` |
| **A.1** | **Architect** | • User Request String<br>• Diagnostic/Research Reports (Optional) | • `docs/dev_docs/plans/plan_[feat].md`<br>*(Must include: List of Reference Documents, Detailed Logic, specific Test Scenarios with expected values, List of Documentation to update, List of Expected Deliverables, and a Completion Checklist)* | `{"plan_path": "..."}` |
| **A.2** | **Plan Reviewer** | • User Request String<br>• Plan File Path<br>• Reports from Phase 0 (Optional) | • `docs/dev_docs/reviews/plan_review_[feat]_v[N].md` (Optional, usually just feedback) | `{"verdict": "APPROVE"}`<br>OR<br>`{"verdict": "REJECT", "feedback": "..."}` |
| **B** | **Developer** | • Approved Implementation Plan File Path<br>• Code Review Report Path (Optional/Loop) | • **Source Code Changes**<br>• **Test Files**<br>• **Updated Documentation**<br>• **Updated VERSION & CHANGELOG**<br>• **Verification Report** (Checked-off list) | `{"commit_hash": "...", "status": "success"}` |
| **C** | **Auditor** | • Implementation Plan File Path<br>• **Cumulative Git Diff** (from branch divergence point `git diff main...HEAD`)<br>• Previous Code Review Report (Optional/Loop) | • `docs/dev_docs/reviews/code_review_[feat]_v[N].md` | `{"verdict": "PASS", "review_path": "..."}`<br>OR<br>`{"verdict": "FAIL", "review_path": "..."}` |
| **D** | **Integration Specialist** | • Conflict Markers (Git Output)<br>• Implementation Plan | • **Resolved Conflicts**<br>• **Merge Commit** | `{"status": "success", "commit_hash": "..."}` |

*> **Universal Output:** All agents may optionally include `"backlog_items": ["Idea 1", "Idea 2"]` in their JSON payload. The Orchestrator will automatically capture these.*

### 3.7 Backlog Management
Any agent in the pipeline MUST be able to capture ideas for future enhancements or research topics without derailing the current task.
*   **Mechanism:** Agents can include a `backlog_items` list in their JSON output.
*   **Orchestrator Action:** The Orchestrator extracts these items and appends them to a central `docs/dev_docs/backlog.md` file (or creates it if missing).
*   **Format:** Each item should be a concise description of the potential feature or research topic.

## 4. Non-Functional Requirements
*   **Resilience:** The system MUST handle CLI timeouts, crashes, or malformed JSON outputs gracefully (e.g., by retrying the step or alerting the user).
*   **Observability:** The system MUST log all agent interactions (prompts and raw responses) to a local file for debugging.
*   **Configurability:** Workflow steps and system prompts MUST be defined in configuration files (JSON/YAML) to allow easy tuning without code changes.

## 5. User Interface
*   **CLI Entry Point:** The user runs the tool via `python orchestrator.py --task "description" [--mode {bugfix,research,direct}]`.
*   **Progress Feedback:** The tool displays real-time status of the pipeline (e.g., "[PLANNING]... Done", "[IMPLEMENTING]... In Progress").

## 6. Example Use Case: "Add a Dark Mode Toggle"

### 6.1 The Interaction
**User (Terminal):**
```bash
python orchestrator.py --task "Add a simple Dark Mode toggle to the main settings page. Use a checkbox."
```

**Orchestrator Output:**
```text
[11:00:00] ORCHESTRATOR: Task Received.
[11:00:01] ORCHESTRATOR: Created branch 'feature/dark-mode-toggle'.
[11:00:02] ORCHESTRATOR: Spawning ARCHITECT...
[11:00:45] ARCHITECT: Implementation Plan created at 'docs/plans/dark_mode_plan.md'.
[11:00:46] ORCHESTRATOR: Committing Plan... OK.
[11:00:47] ORCHESTRATOR: Verifying plan... OK. (Review Committed)
[11:00:48] ORCHESTRATOR: Spawning DEVELOPER...
[11:05:20] DEVELOPER: Implementation complete. Tests passed. Commit: 7f3a2b1.
[11:05:22] ORCHESTRATOR: Spawning AUDITOR...
[11:06:10] AUDITOR: Review passed. Report at 'docs/reviews/dark_mode_review.md'.
[11:06:11] ORCHESTRATOR: Committing Review Report... OK.
[11:06:12] ORCHESTRATOR: Pipeline Success! Branch 'feature/dark-mode-toggle' is ready for merge.
```

**User:**
The user can now simply check the branch or run `git merge feature/dark-mode-toggle`.

### 6.2 Deployment & Access
Because the Orchestrator is a standard Python script wrapping a CLI tool, it offers flexible deployment options:

*   **Local Workstation:** Run directly in your terminal (as shown above).
*   **Remote Server / Headless:** Run on a dedicated build server (e.g., an EC2 instance or a home server). You can SSH into it to launch tasks or check logs.
*   **Mobile Interaction (via Wrappers):** Since the input is a text command and output is text status, you can easily wrap this script with a Telegram/Discord bot or a simple web interface (e.g., Streamlit) to trigger tasks and receive updates on your mobile phone while away from your keyboard.

## 7. Iterative Development Strategy
We will adopt an **Iterative "Dogfooding" Approach**:
1.  **Prototype:** Build a minimal `v0.1` Orchestrator.
2.  **Test:** Use this `v0.1` to implement a very simple, real feature in the LMUFFB project.
3.  **Evaluate:** Observe the friction points (e.g., "Did the agent get confused?", "Was the polling too slow?").

## 8. Future Considerations / TODOs (Backlog)

### 8.1 Structured Feedback Channel
*   **Concept:** Every agent response JSON should include an optional `feedback` object (separate from the main result).
*   **Goal:** Allow "Workers" to flag issues up the chain without failing the task.
*   **Use Case:** A Developer notices a vague requirement in the Plan. They implement it anyway (to pass the task) but flag it in `feedback`: "The plan for 'UserAuth' was ambiguous; I assumed JWT." The Lead Architect can then decide to update the documentation later.
*   **Caveat (Honesty vs. Feedback):** The Orchestrator MUST ensure agents do not use the feedback field to hide failures. A `status: success` MUST NOT be accepted if the feedback contains reports of crashes or unresolvable errors.
*   **Note:** This channel is ideal for flagging **Technical Debt** or documentation gaps identified during implementation.
*   **Feedback Triage Workflow:**
    *   If an agent returns non-empty `feedback`, the Orchestrator spawns a **Feedback Triager** (e.g., Lead Analyst).
    *   The Triager reviews the note and decides:
        1.  **Backlog:** Create a new entry in `docs/dev_docs/backlog.md` (e.g., "Refactor UserAuth later").
        2.  **Docs:** Update existing documentation immediately.
        3.  **Ignore:** Deem the feedback trivial.
    *   This ensures "soft warnings" are not lost in the logs but are actively curated into future work.
*   **Feasibility Note:** **Low Complexity.** Requires only a schema update (adding an optional field) and prompt tweaking. Easily integrated into v0.1 or v0.2.

### 8.2 Persistent Agent Memory (`AGENT_MEMORY.md`)
*   **Concept:** A shared, append-only Markdown file that persists across steps (and potentially across tasks).
*   **Goal:** Solve the "Amnesia" problem where Phase B forgets what Phase A discovered about a quirky library.
*   **Workflow:**
    *   Any agent can request to *write* a "Lesson Learned" to Memory.
    *   The Orchestrator *injects* relevant sections of Memory into the context of future agents.
*   **Feasibility Note:** **Medium Complexity.** Requires a "Memory Manager" module to read/write safely to the shared file and logic to decide *what* to inject into the context window to avoid token bloat.

### 8.3 Dynamic Skill Curation
*   **Concept:** Leverage the **Gemini CLI Skills** system (https://geminicli.com/docs/cli/skills/).
*   **Goal:** Turn repeated solutions into reusable tools.
*   **Workflow:**
    *   If the Integration Specialist solves a tricky git conflict, the Orchestrator could prompt: "Should this resolution strategy be saved as a named Skill?"
    *   If yes, the system generates a Skill definition (instructions) for future agents to use, effectively "learning" from its own success.
*   **Feasibility Note:** **High Complexity.** Requires deep integration with the Gemini CLI's Skills API, a mechanism to validate generated skills, and Human-in-the-Loop approval to prevent "hallucinated tools" from entering the system. Best for v1.0+.

### 8.4 Asynchronous Human Review Layer (Human-in-the-Loop)
*   **Concept:** A dedicated Markdown file (e.g., `docs/dev_docs/HUMAN_REVIEW.md`) where a human can asynchronously leave notes, corrections, or guidance for the agents.
*   **Goal:** Provide a non-blocking way for the project owner to steer the system without being present during every step of execution.
*   **Workflow:** (To be determined) Future iterations will determine how agents should ingest these notes (e.g., as part of the Phase Init or context injection).
*   **Feasibility Note:** **Trivial.** Requires no code changes today; only the manual creation and maintenance of the file by the human.

