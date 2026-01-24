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

### Task 1.3: Single-Step Orchestration
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
*   `architect_prompt.txt`: Instructions for planning.
*   `developer_prompt.txt`: Instructions for coding (includes JSON schema).
*   `auditor_prompt.txt`: Instructions for reviewing.

### Task 2.3: The Pipeline Loop
*   Update `orchestrator.py` to chain the steps:
    1.  `Plan = run_step(Architect)`
    2.  `Result = run_step(Developer, input=Plan)`
    3.  `Verdict = run_step(Auditor, input=Result)`

## Phase 3: Robustness (Day 3)
**Goal:** Error handling and Git integration.

### Task 3.1: Git Helper
*   Create `src/git_utils.py`.
*   Implement `create_branch`, `get_diff`, `commit_exists`.

### Task 3.2: Retry Logic
*   Wrap `run_gemini_command` in a retry loop.
*   If JSON parsing fails, feed the error back to the agent: "You failed to output JSON. Please try again."

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
1.  Creates a plan.
2.  Creates a branch.
3.  Implements the code.
4.  Reviews the code.
5.  Stops and asks me to Merge.
