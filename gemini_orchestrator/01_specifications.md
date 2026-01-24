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

0.  **Phase 0: Researcher (Deep Research) - Optional**
    *   **Trigger:** Activated via user flag (e.g., `--research`).
    *   **Input:** High-level user request.
    *   **Goal:** Perform deep internet research and codebase analysis to understand complex domains or requirements.
    *   **Output:** Path to Research Report (Markdown).

1.  **Phase A.1: Architect (Planning)**
    *   **Input:** High-level user request (and optional Research Report from Phase 0).
    *   **Goal:** Analyze requirements and produce a Markdown implementation plan.
    *   **Output:** Path to the generated Implementation Plan file.

2.  **Phase A.2: Lead Architect (Plan Review)**
    *   **Input:** The Original Request and the Implementation Plan.
    *   **Goal:** Verify the plan matches the requirements and is technically sound.
    *   **Verdict:**
        *   **APPROVE:** Proceed to Implementation.
        *   **REJECT:** Return to **Phase A.1** with feedback.

3.  **Phase B: Developer (Implementation)**
    *   **Input:** Path to the Approved Implementation Plan file.
    *   **Goal:** Write code, run tests, fix errors until tests pass.
    *   **Constraint:** Must work on a specific Git branch.
    *   **Output:** Git Commit Hash or "Success" status.

4.  **Phase C: Auditor (Code Review)**
    *   **Input:** Path to Implementation Plan file and Git Diff.
    *   **Goal:** Critique changes against project standards.
    *   **Verdict:**
        *   **PASS:** Trigger Merge.
        *   **FAIL:** Return to **Phase B** (Developer) with the Review Report as new input.

### 3.3 Structured Communication
*   **JSON Enforcement:** The Orchestrator MUST instruct the agent to output key results (status, file paths) in a strict JSON format at the end of its response.
*   **Fuzzy Parsing:** The Orchestrator MUST be able to extract this JSON payload from the raw text output, ignoring conversational filler.

### 3.4 Git Integration
*   The Orchestrator MUST handle git operations that are "above" the agent's pay grade, such as:
    *   Creating feature branches before the Developer starts.
    *   Merging branches after the Auditor approves.
    *   Resetting/Cleaning the workspace between runs.

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
| **0** | **Researcher** | • User Request String | • `docs/dev_docs/research/report_[feat].md` | `{"report_path": "..."}` |
| **A.1** | **Architect** | • User Request String<br>• Research Report Path (Optional) | • `docs/dev_docs/plans/plan_[feat].md`<br>*(Must include: Detailed Logic, specific Test Scenarios with expected values, List of Documentation to update, List of Expected Deliverables, and a Completion Checklist)* | `{"plan_path": "..."}` |
| **A.2** | **Plan Reviewer** | • User Request String<br>• Plan File Path<br>• Research Report Path (Optional) | • `docs/dev_docs/reviews/plan_review_[feat]_v[N].md` (Optional, usually just feedback) | `{"verdict": "APPROVE"}`<br>OR<br>`{"verdict": "REJECT", "feedback": "..."}` |
| **B** | **Developer** | • Approved Implementation Plan File Path<br>• Code Review Report Path (Optional/Loop) | • **Source Code Changes**<br>• **Test Files**<br>• **Updated Documentation**<br>• **Updated VERSION & CHANGELOG**<br>• **Verification Report** (Checked-off list) | `{"commit_hash": "...", "status": "success"}` |
| **C** | **Auditor** | • Implementation Plan File Path<br>• Git Diff (via `git diff master...HEAD`)<br>• Previous Code Review Report (Optional/Loop) | • `docs/dev_docs/reviews/code_review_[feat]_v[N].md` | `{"verdict": "PASS", "review_path": "..."}`<br>OR<br>`{"verdict": "FAIL", "review_path": "..."}` |

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
*   **CLI Entry Point:** The user runs the tool via `python orchestrator.py --task "description" [--research]`.
*   **Progress Feedback:** The tool displays real-time status of the pipeline (e.g., "[PLANNING]... Done", "[IMPLEMENTING]... In Progress").

## 6. Example Use Case: "Add a Dark Mode Toggle"

### 6.1 The Interaction
**User (Terminal):**
```bash
python orchestrator.py --task "Add a simple Dark Mode toggle to the main settings page. Use a checkbox."
```

**Orchestrator Output:**
```text
[11:00:00] ORCHESTRATOR: Task Received. Spawning ARCHITECT...
[11:00:45] ARCHITECT: Implementation Plan created at 'docs/plans/dark_mode_plan.md'.
[11:00:46] ORCHESTRATOR: Verifying plan... OK.
[11:00:46] ORCHESTRATOR: Creating branch 'feature/dark-mode-toggle'.
[11:00:48] ORCHESTRATOR: Spawning DEVELOPER...
[11:05:20] DEVELOPER: Implementation complete. Tests passed. Commit: 7f3a2b1.
[11:05:22] ORCHESTRATOR: Spawning AUDITOR...
[11:06:10] AUDITOR: Review passed. Report at 'docs/reviews/dark_mode_review.md'.
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
4.  **Refine:** Update the Orchestrator code based on this real-world usage before adding complex features like "Retry Logic" or "Git Integration".

