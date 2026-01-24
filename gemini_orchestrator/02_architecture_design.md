# Gemini CLI Orchestrator - Architecture & Design

## 1. System Architecture

The system follows a **Controller-Worker** architecture.

```mermaid
graph TD
    User[User Terminal] -->|Starts| Orch[Python Orchestrator]
    Orch -->|Reads| Config[Workflow Config (JSON)]
    Orch -->|Manages| State[State Manager]
    
    Orch -->|Spawns| P1[Process 1: Architect]
    Orch -->|Spawns| P2[Process 2: Developer]
    Orch -->|Spawns| P3[Process 3: Auditor]
    
    subgraph "Isolation Boundary"
        P1 -- Writes --> Art1[Plan Artifact (.md)]
        P2 -- Reads --> Art1
        P2 -- Commits --> Git[Git Repository]
        P3 -- Reads --> Art1
        P3 -- Reads --> Git
    end
    
    P1 -.->|JSON| Orch
    P2 -.->|JSON| Orch
    P3 -.->|JSON| Orch
```

## 2. Component Design

### 2.1 The `AgentWrapper` Class
This class encapsulates the interface with the Gemini CLI.
*   **Responsibilities:**
    *   Constructing the full shell command.
    *   Managing `stdin`/`stdout` piping.
    *   Injecting the "System Prompt" (e.g., "You are a rigid worker. Output JSON only.").
    *   Handling timeouts and process cleanup.

### 2.2 The `PromptBuilder` Module
Responsible for assembling the final prompt string sent to the agent.
*   **Logic:** `Base Prompt` + `Task Context` + `Input Artifacts` + `Output Instructions`.
*   **Example:**
    > "CONTEXT: You are the Developer.
    > INPUT: Read the plan at 'docs/plans/fix_v1.md'.
    > TASK: Implement the code. Run tests.
    > FORMAT: End your response with JSON: { 'status': 'success', ... }"

### 2.3 The `ResponseParser` Module
Responsible for extracting structured data from the unstructured LLM output.
*   **Algorithm:**
    1.  Receive raw stdout string.
    2.  Regex search for the last occurrence of `{...}` structure.
    3.  `json.loads()` the extracted substring.
    4.  Validate against a Pydantic model or schema.

### 2.4 The `WorkflowEngine` Class
The main state machine.
*   **State:**
    *   `current_step`: (enum: PLAN, CODE, REVIEW)
    *   `workspace_root`: Path to repo.
    *   `artifacts`: Dictionary of paths (`{'plan': '...', 'review': '...'}`).
*   **Transitions:**
    *   `PLAN` -> `CODE` (if Plan file exists).
    *   `CODE` -> `REVIEW` (if Git Commit made).
    *   `REVIEW` -> `MERGE` (if verdict is PASS).
    *   `REVIEW` -> `CODE` (if verdict is FAIL, passing feedback loop).

## 3. Data Flow

### Step 1: Planning
1.  **Orchestrator** receives task: "Implement Feature X".
2.  **Orchestrator** creates branch `feature/X`.
3.  **Orchestrator** spawns **Agent (Architect)**.
4.  **Agent** researches and writes `docs/plans/feature_X.md`.
5.  **Agent** prints JSON: `{"plan_path": "docs/plans/feature_X.md"}`.
6.  **Orchestrator** parses JSON, verifies file existence, saves path to state.

### Step 2: Implementation
1.  **Orchestrator** reads `plan_path` from state.
2.  **Orchestrator** spawns **Agent (Developer)**.
    *   *Prompt Injection:* "Read file {plan_path}..."
3.  **Agent** modifies code, runs tests.
4.  **Agent** commits changes to git.
5.  **Agent** prints JSON: `{"commit_hash": "abc1234", "tests_passed": true}`.

### Step 3: Review
1.  **Orchestrator** spawns **Agent (Auditor)**.
    *   *Prompt Injection:* "Review changes in commit {commit_hash} against plan {plan_path}..."
2.  **Agent** writes `docs/reviews/review_X.md`.
3.  **Agent** prints JSON: `{"verdict": "PASS", "report_path": "..."}`.

## 4. Key Decisions & Trade-offs

*   **No "Memory" by default:** We explicitly choose NOT to pass the chat history. If the Developer needs to know *why* the Architect made a decision, they must read the Plan document. This forces better documentation.
*   **Polling vs. Blocking:** Since we wrap the process, we use **Blocking** calls (waiting for the subprocess to finish) rather than Polling. The Agent script itself is responsible for the "Loop" of running tests until they pass.
*   **Error Handling:** If the Agent fails to output JSON, the Orchestrator will assume failure and ask the user for manual intervention or a retry.
