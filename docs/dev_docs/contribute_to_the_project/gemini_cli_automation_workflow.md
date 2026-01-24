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

## 🎮 The Orchestrator Pattern

To ensure high-quality automation without manual overhead, the primary Gemini CLI agent acts as the **Orchestrator (Project Manager)**.

### How Orchestration is Automated:
1.  **State Management**: The Orchestrator uses the local environment to manage branches and verify that artifacts (Plans, Reviews) are correctly produced before triggering the next phase.
2.  **Delegated Execution**: The Orchestrator does not perform complex implementation or review logic itself. Instead, it uses the `start_new_jules_task` tool to spawn fresh, isolated "worker" agents for Phase 2 and 3.
3.  **Handoff Protocols**: 
    *   The Orchestrator provides the worker agent with a strict "Statement of Work" (e.g., "Implement ONLY the changes defined in plan X").
    *   The worker agent signals completion by committing its changes to Git.
    *   The Orchestrator verifies the commit before initiating the next isolated worker.

### Why this is safer:
By acting as the Orchestrator, the primary CLI agent maintains the high-level project goal while ensuring that the "Implementer" and "Reviewer" sessions remain completely ignorant of each other's chat histories, preventing "context leakage" or biased reviews.

## 🚀 Level 3: Full Automation via Polling Supervisor

To overcome the "wait bottleneck" (where the CLI waits for a human to confirm a Jules task is done), the Orchestrator implements an **Active Polling Loop**.

**The Process:**
1.  **Launch**: Orchestrator triggers `start_new_jules_task` for Phase 2 (Implementation).
2.  **Monitor**: Orchestrator enters a loop, executing `git fetch origin [branch_name]` every 60 seconds.
3.  **Detect**: When the commit history changes (indicating Jules has pushed the implementation), the Orchestrator breaks the loop.
4.  **Verify & Proceed**: Orchestrator immediately pulls the changes, runs local verification, and if successful, triggers Phase 3 (Review).

*Requirement*: The terminal session must remain open during the polling phase.

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
    5.  Awaiting user confirmation to finalize the commit.

---

## 🤖 Claude Code: A Modern Alternative for This Workflow

Based on an analysis of **Claude Code** (Anthropic's CLI tool), the orchestration with separation workflow described in this document can be accomplished more natively using its specific features.

### 1. Solving the "Context Isolation" Bottleneck
*   **The Goal:** Distinct "sessions" (Planner, Implementer, Reviewer) that share *no* chat history, only artifacts (files/git branches).
*   **Claude Code Feature:** **"Multi-Claude Workflows" & Sub-Agents.**
    *   Claude Code explicitly supports spawning **sub-agents** for specific tasks to manage context.
    *   It can utilize **Git Worktrees** to run multiple instances in parallel on different branches without conflict.
    *   This means the "Implementer" agent can be spawned in a fresh process, unaware of the "Planner's" research, effectively enforcing the isolation you want without needing the complex `start_new_jules_task` scaffolding.

### 2. Solving the "Wait/Polling" Bottleneck
*   **The Goal:** The Orchestrator monitors progress without the user manually checking or keeping a terminal window hostage.
*   **Claude Code Feature:** **Programmatic Tool Calling & Headless Mode.**
    *   Unlike the standard REPL (Read-Eval-Print Loop), Claude Code can act as a **programmatic orchestrator**. It can write and execute Python scripts to handle the control flow (loops, conditionals, waiting for git commits).
    *   It has a **Headless Mode** designed for CI/CD and automation. This allows the orchestrator to run in the background or as a dedicated process that "wakes up" when a sub-agent pushes a commit, solving the "active polling loop" issue elegantly.

### 3. Solving the "Role Definition" Bottleneck
*   **The Goal:** Strict roles (e.g., "Reviewer" must only critique, not code).
*   **Claude Code Feature:** **`CLAUDE.md` & Skills.**
    *   Claude Code uses a configuration file (`CLAUDE.md`) to define project-specific commands and context.
    *   You can define specific **Skills** or instructions for each "role" that are loaded dynamically. For example, the "Reviewer" agent could be initialized with a specific instruction set that disables file-writing tools and only allows reading and commenting, enforcing the role at the system level.

### Summary Comparison

| Requirement | Gemini CLI + Jules Bottleneck | Claude Code Solution |
| :--- | :--- | :--- |
| **Isolation** | Relies on starting new, separate tasks (`start_new_jules_task`) which can be disjointed. | **Native Sub-agents & Worktrees:** Can manage parallel, isolated contexts natively. |
| **Orchestration** | "Active Polling Loop" (a simple generic loop) blocks the terminal. | **Programmatic Orchestration:** Can run scripts/code to manage the workflow logic reliably. |
| **Automation** | Requires user confirmation for many steps; "Wait bottleneck". | **Headless Mode:** Can run autonomously for long-duration tasks without user input. |
| **Context** | Large context can get "polluted" over a long session. | **Automatic Context Management:** actively compacts history and uses sub-agents to keep context clean. |

### Conclusion
The architecture designed in this document—specifically the **Orchestrator Pattern** with **Session Isolation**—is practically the native design philosophy of Claude Code. Moving to that tool would allow for the implementation of this workflow "out of the box" rather than requiring manual polling loops.

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

---

## 🔮 Gemini CLI Roadmap vs. Desired Features (Jan 2026)

Based on a review of the Gemini CLI roadmap and current capabilities, here is how it stacks up against the "Orchestration with Separation" requirements:

### 1. Multi-Agent Workflows / Parallelism
*   **Gemini CLI Status:** **Not natively available** in the same seamless way as Claude Code.
*   **Current State:** You can technically run multiple instances of Gemini CLI in different terminal tabs, but there is no built-in "Multi-Gemini" orchestration where one agent spawns and manages others in parallel native processes (Worktrees) to solve a single task cooperatively.
*   **Workarounds:** You can use `run_shell_command` to execute `gemini` commands in the background, but this is manual plumbing rather than a first-class feature.

### 2. Sub-Agents (Dynamic & Isolated)
*   **Gemini CLI Status:** **Experimental / In-Development.**
*   **Current State:** There is an open feature request and active development for a `SubAgent` class to allow LLM-driven tool orchestration and custom system prompts.
*   **Jules Integration:** The "Jules" extension (`start_new_jules_task`) is effectively a specialized sub-agent for coding tasks, but it is a specific implementation rather than a generic "spawn a sub-agent for X" capability.
*   **Roadmap:** "Background Agents" are on the roadmap to enable long-running, autonomous tasks.

### 3. Programmatic Orchestrator
*   **Gemini CLI Status:** **Limited / Different Philosophy.**
*   **Current State:** Gemini CLI focuses on a **ReAct loop** (Reason & Act) where the model iterates on a thought process. It does not currently expose a Python-like scripting interface *to the model itself* to write control flow logic (loops, conditionals) for its own tools.
*   **Comparison:** Claude Code allows writing Python code to orchestrate tools (e.g., `for file in files: process(file)`). Gemini CLI typically relies on the model deciding to call a tool one by one, or you (the user) writing a bash script that calls Gemini CLI.

### 4. Headless Mode
*   **Gemini CLI Status:** **Fully Supported.**
*   **Current State:** Gemini CLI has a robust **Headless Mode** designed for CI/CD and automation. You can pipe prompts into it (`echo "prompt" | gemini ...`) and get JSON output.
*   **Utility:** This matches the need for the "Polling Supervisor" to run without a GUI, but the *logic* of the supervisor (the polling loop itself) currently has to be written by the user in a shell script, whereas Claude Code aims to let the agent write that logic itself.

### Summary
While Gemini CLI is powerful and open-source, the specific **"Orchestrator with Separation"** architecture designed in this document—where one agent programmatically manages others with strict context barriers—is currently **easier to implement "out of the box" with Claude Code**. Gemini CLI is moving in that direction with "Background Agents," but the programmatic control flow is a key differentiator for Claude Code right now.

---

## 🐍 The "Wrapper" Solution: External Python Orchestrator

An alternative to waiting for native features is to use an **external Python Orchestrator** (a script that calls the Gemini CLI via subprocess). This approach resolves several key bottlenecks by treating the CLI as an atomic tool rather than a persistent chat partner.

### ✅ Problems Solved

1.  **Session Isolation (The "Context Contamination" Problem)**
    *   **Solved:** By calling `subprocess.run(["gemini", "prompt", ...])` for each step, you force a fresh process every time. The "Reviewer" invocation literally cannot see the "Implementer" chat history because it's a completely new OS process.
    *   **Mechanism:** The Python script holds the "State" (branch names, file paths) and passes *only* the necessary artifacts (e.g., "Read `plan.md` and `code.cpp`") to the agent via the prompt.

2.  **The "Wait/Polling" Bottleneck**
    *   **Solved:** Python can reliably manage the control flow. Instead of a fragile shell loop, the script can:
        *   Trigger the implementation task.
        *   Enter a `while` loop checking `git log` or `git status` every 30 seconds.
        *   Parse the output reliably.
        *   Automatically trigger the Review step the moment the commit lands.

3.  **Programmatic Orchestration**
    *   **Solved:** You don't need the Agent to be the orchestrator (risking hallucinations). Logic is defined in Python: `if review_passed: merge() else: kick_back_to_implementer()`. This is deterministic and safer.

### ⚠️ Remaining Limitations

1.  **Tool Restriction (Role Enforcement)**
    *   **Issue:** You cannot easily "disable" tools for a specific invocation. Even if prompted "You are a Reviewer, do not write code," the agent *still has* the `write_file` tool available.
    *   **Mitigation:** Requires strong system prompts in the script.

2.  **Context Loading "Friction"**
    *   **Issue:** Since every step is a fresh process, the agent has zero context. The Python script must explicitly feed it everything or trust it to find it, which may increase token usage.

3.  **Error Recovery**
    *   **Issue:** If the CLI crashes or returns malformed output in headless mode, the Python script needs robust exception handling to retry or alert the user.

### Verdict
The Python Orchestrator is a viable bridge. It effectively achieves **Isolation** and **Automation** with the current Gemini CLI by treating it as an API rather than a chat partner.

### Writing a sophisticated Python orchestrator (using LangChain or just robust Python logic)

Writing a sophisticated Python orchestrator (using LangChain or just robust Python logic) that treats the Gemini CLI as a "black box function" is likely the definitive solution for your requirements.

Here is why this approach works so well and how it addresses the remaining gaps:

  1. The "Agentic Pipeline" Architecture
  By shifting the logic out of the prompt and into Python code, you turn a "Chat" into a "Pipeline".

   * The CLI acts as the "Worker": It has the tools (edit files, run tests, Git). It doesn't need to know why it's doing task B
     after task A; it just executes.
   * Python acts as the "Manager": It holds the state. It knows that "The Plan is in docs/plan_X.md" and "The Code is in       
     src/main.cpp".

  2. Solving the Context/Linking Problem
  Your idea to pass file paths instead of conversation history is the key to efficient isolation.

   * Step 1 (Architect):
       * Input: "Analyze the crash in log.txt."
       * Action: Agent runs, researches, and writes docs/plans/fix_crash_v1.md.
       * Output (Structured):
   1         {
   2           "status": "success",
   3           "artifact_path": "docs/plans/fix_crash_v1.md",
   4           "next_recommended_step": "implementation"
   5         }
   * The Python Orchestrator: Parses this JSON. It doesn't care about the content of the plan, only the path.
   * Step 2 (Developer):
       * Orchestrator Prompt: "You are a Developer. Read the plan at docs/plans/fix_crash_v1.md. Implement it. Output a JSON   
         summary when done."
       * Context: The agent starts fresh. It reads the file (using its read_file tool). It has zero "memory" of the Architect's
         web searches or internal monologue, ensuring pure implementation based only on the documentation.

  3. Why Wrapping the CLI is better than using the Raw API (for now)
  You might ask, "Why not just use LangChain with the Gemini API directly?"

   * The CLI has "Batteries Included": The Gemini CLI already has a pre-configured toolset (replace, run_shell_command,        
     google_web_search, codebase_investigator).
   * Effort Saver: If you use the raw API, you have to re-write all those tools and their permission/sandboxing logic yourself.
   * Workflow: Wrapping the CLI lets you leverage the agent's ability to navigate the filesystem and run shell commands, which 
     is complex to implement reliably from scratch.

  4. Implementation Details (The "Sophisticated" Part)

  To make this robust, your Python Orchestrator needs to enforce Strict Interfaces:

   1. Input Injection:
       * Use the --headless flag (if available) or simply pipe stdin to gemini.
       * Construct prompts that explicitly demand JSON output at the end:
          > "...When you are finished, output the result in this JSON format: {"files_created": [], "status": "..."}. Do not   
  output any other text after the JSON."

   2. Output Parsing (The Hard Part):
       * LLMs love to talk before/after the JSON. Your Python script should use a "fuzzy parser" (e.g., a regex that finds the 
         first { and last }) to extract the JSON payload from the CLI's stdout.

   3. State Management:
       * The Python script should maintain a workflow_state.json object that tracks:
           * current_branch
           * plan_path
           * review_path
           * test_results_path

  Conclusion
  You are essentially building a CI/CD pipeline for Intelligence.

   * Jenkins/GitHub Actions orchestrates Build Tools (Compiler, Linter).
   * Your Python Script orchestrates Cognitive Tools (Planner, Coder, Reviewer).

  This approach gives you the Context Isolation of separate processes, the Control of a programming language, and the Power of 
  the Gemini CLI's toolset. It is the most robust way to achieve your goal today.


## Subagents in Gemini CLI

### Resources

* https://github.com/google-gemini/gemini-cli/issues/3132
* https://medium.com/google-cloud/advanced-gemini-cli-part-3-isolated-agents-b9dbab70eeff
* https://aipositive.substack.com/p/how-i-turned-gemini-cli-into-a-multi