# Gemini CLI Automated Workflow

This document outlines how the Gemini CLI (with Jules) automates the project's contribution standards. It adapts the human-focused workflow from `how_to_contribute_to_the_project.md` into a fully autonomous process.

## 🤖 Automated Workflow Steps (Session Isolated) (V. 2.0)

To prevent context contamination (e.g., the Reviewer being biased by the Implementer's chat history), this workflow uses **Git branches** as the only state transfer mechanism. Each step is a fresh "Session" or "Task".

In other words: To achieve strict "Session Isolation"—where the "Reviewer" has no knowledge of the "Implementer's" struggle, only the results—we use Git as the handover mechanism and distinct Jules Tasks as the isolated workers.

### Step 1: Research & Planning (Session A)
*   **Agent**: Local CLI or Jules Task #1.
*   **Action**: 
    1.  Investigate requirements (`codebase_investigator`, `google_web_search`).
    2.  Create a detailed plan in `docs/dev_docs/plans/[feature_name].md`.
    3.  **Handover**: Commit and Push the plan to a new feature branch.
    *   `git checkout -b feature/[name]`
    *   `git add docs/dev_docs/plans/...`
    *   `git commit -m "docs: add implementation plan"`
    *   `git push`

### Step 2: Implementation (Session B - Fresh Context)
*   **Agent**: Jules Task #2 (Triggered via `start_new_jules_task`).
*   **Prompt**: "Implement the plan found in `docs/dev_docs/plans/[feature_name].md`. Run tests to verify."
*   **Isolation**: This agent sees only the Plan file, not the research history.
*   **Action**:
    1.  Agent reads the plan and implements code.
    2.  Agent runs builds and tests.
    3.  **Handover**: Agent commits and pushes changes to the feature branch.

### Step 3: Code Review (Session C - Fresh Context)
*   **Agent**: Jules Task #3 (Triggered as a new task).
*   **Prompt**: "Review the changes in branch feature/[name] against the plan in `docs/dev_docs/plans/[feature_name].md`. Produce a markdown review."
*   **Isolation**: This agent sees only the final code. It acts as an unbiased auditor.
*   **Action**:
    1.  Agent analyzes the diff.
    2.  Agent writes `docs/dev_docs/reviews/review_[feature_name].md`.
    3.  **Handover**: Agent commits and pushes the review.

### Step 4: Finalization (Session A or D)
*   **Agent**: Local CLI.
*   **Action**:
    1.  Pull the review and implementation.
    2.  Address any critical feedback from the review.
    3.  Merge to main/master (or create PR).

---
*Note: This workflow ensures that every automated contribution remains traceable, documented, and verified to the same (or higher) standard as manual contributions.*

---

## 🤖 Automated Workflow Steps (V. 1)

### Step 1: Research & Discovery
*   **Action**: Use `codebase_investigator` to map dependencies and `google_web_search` for external physics/technical research.
*   **Deliverable**: A research report (if required) stored in `docs/dev_docs/research/`.

### Step 2: Implementation Plan
*   **Action**: Analyze the requirement against the current codebase and generate a structured plan.
*   **Deliverable**: A new `.md` file in `docs/dev_docs/plans/` containing:
    *   **Motivation**: Why the change is being made.
    *   **Architecture**: High-level structural changes.
    *   **Test Plan**: Specific automated tests to be added (Unit/Integration).
    *   **Documentation**: List of documents requiring updates.

### Step 3: Implementation & Agent Loop
*   **Action**: 
    1.  Apply code changes using `replace` and `write_file`.
    2.  Execute build commands from `build_commands.txt`.
    3.  Run tests (`run_combined_tests.exe`) and capture output.
    4.  **Fix & Iterate**: If tests fail, analyze logs, adjust code, and rebuild until 100% pass.
    5.  Update `VERSION` and `CHANGELOG.md`.

### Step 4: Automated Code Review
*   **Action**: Self-review the changes against project conventions (DirectInput, ImGui, Physics performance).
*   **Deliverable**: A `code_review_[feature].md` file documenting the analysis and any self-corrections made during the process.

### Step 5: Finalization & Commit
*   **Action**: 
    1.  Stage all implementation artifacts (the Plan, the Review, and the Code).
    2.  Propose a commit message that reflects the "Why" and links to the generated documents.
    3.  Awaiting user confirmation to finalize the commit.

---

Adapted Workflow for Gemini CLI (v 0, directly from the chat)

  Step 0: Deep Research (Adapted)
   * Original: Use Gemini Deep Research manually.
   * My Automation: I can use google_web_search and web_fetch to gather external info, and codebase_investigator to deeply
     analyze the existing architecture. I can then compile these findings into the required Markdown report.
       * Note: For extremely complex physics queries, I might still ask you to provide a specific paper or resource, but I can
         do the heavy lifting of synthesizing it.

  Step 1: Implementation Plan
   * Original: Generate a plan using an LLM with full context.
   * My Automation: I will perform the analysis and write_file to create the implementation plan in Markdown, covering:
       * Motivation/Overview.
       * Proposed Changes (with snippets).
       * Test Plan: Description of new tests (automating the request to include this).
       * Doc Updates: Identifying which docs need updates.
       * Context: I don't need FULL_PROJECT_CONTEXT.md explicitly pasted; I can read the repo directly.

  Step 2: Implementation & Verification
   * Original: Agent writes code, compiles, runs tests, fixes errors.
   * My Automation:
       1. I will write/edit the code (write_file, replace).
       2. I will run the build commands from build_commands.txt (run_shell_command).
       3. I will run the tests and parse the output.
       4. If tests fail, I will analyze the logs, fix the code, and retry (the "Agent Loop").
       5. I will update VERSION and CHANGELOG.md.

  Step 3: Code Review
   * Original: generate a review, fix issues.
   * My Automation:
       * I can generate a code_review_v1.md file by analyzing the git diff against the project standards.
       * I can then read my own review and apply the fixes immediately.

  Step 4: Commit
   * Original: Commit all artifacts (Plan, Review, Code).
   * My Automation: I will stage all generated markdown files (Plan, Review) and code changes, then generate a commit message
     for your approval.