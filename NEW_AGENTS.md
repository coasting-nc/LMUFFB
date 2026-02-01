# LMUFFB - AI Developer Guide


## ✅ Standard Task Workflow (SOP)

**Perform these steps for EVERY task to ensure quality and consistency.**



### Commands to build and run tests

Update app version, compile main app, compile all tests (including windows tests), all in one single command:
`& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -SkipAutomaticLocation; cmake -S . -B build; cmake --build build --config Release --clean-first`

Run all tests that had already been compiled:
`.\build\tests\Release\run_combined_tests.exe`

### 🧪 Test-Driven Development
*   **Requirement**: You **must** add or update C++ unit tests for every logic change or new feature.
*   **Location**: Add test cases to files under `tests/`.
*   **Verification**: You **must** compile and run the tests to prove your code works.

    *   *Command (Windows - PowerShell) to update app version, compile main app, compile all tests (including windows tests), all in one single command*:
        ```powershell
        & 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -SkipAutomaticLocation; cmake -S . -B build; cmake --build build --config Release --clean-first
        ```
    *   Run all tests that had already been compiled:
        ```powershell
        .\build\tests\Release\run_combined_tests.exe
        ```

### 1. 🧠 Consult Memory
*   **Action**: Read `AGENTS_MEMORY.md`.
*   **Why**: It contains workarounds (like Git fixes) and architectural lessons learned from previous sessions.

### 2. Context

    *   **Read Updated Docs**: For each changed documentation file, read its current content to understand the updates.
    *   **Why**: Documentation changes often reflect new features, API changes, architecture updates, or critical fixes. You must stay current with the project's evolving knowledge base.
    *   **Priority Files**: Pay special attention to changes in:
        *   `README.md` - User-facing features and setup
        *   `CHANGELOG_DEV.md` - Recent changes and version history
        *   `docs/dev_docs/references/Reference - telemetry_data_reference.md` - API source of truth
        *   `docs/dev_docs/FFB_formulas.md` - Physics and scaling constants
        *   `docs/architecture.md` - System design and components
        *   `AGENTS_MEMORY.md` - Previous session learnings
*   **Context**: If you need to refresh your understanding of the full codebase, run `python scripts/create_context.py`.



### 4. 📚 Documentation Updates
*   **Requirement**: You **must** scan and update ALL relevant documentation to reflect your changes.
*   **Process**:
    1.  **Scan Documentation**: Use `find_by_name` to list all `.md` files in the project.
    2.  **Read Relevant Docs**: Review the documentation files that are likely affected by your changes.
    3.  **Determine Relevance**: Identify which documents need updates based on your changes.
    4.  **Update Documents**: Modify all relevant documentation to maintain consistency.
*   **Common Documentation Targets**:
    *   **Math/Physics Changes** → Update `docs/dev_docs/FFB_formulas.md`
    *   **New FFB Effects** → Update `docs/ffb_effects.md` AND `docs/the_physics_of__feel_-_driver_guide.md`
    *   **Telemetry Usage** → Update `docs/dev_docs/references/Reference - telemetry_data_reference.md`
    *   **GUI Changes** → Update `README.md` (text descriptions)
    *   **Architecture Changes** → Update `docs/architecture.md`
    *   **New Features** → Update `README.md`, `docs/introduction.md`, and relevant feature docs
    *   **Bug Fixes** → Consider updating `docs/dev_docs/TODO.md` to mark items as complete
    *   **LMU 1.2 Features** → Update `docs/dev_docs/new_ffb_features_enabled_by_lmu_1.2.md`
    *   **Configuration Changes** → Update `docs/ffb_customization.md`
*   **Documentation Directories**:
    *   `docs/` - User-facing documentation
    *   `docs/dev_docs/` - Developer and technical documentation
    *   `docs/bug_reports/` - Bug reports and troubleshooting
    *   Root `.md` files - `README.md`, `CHANGELOG_DEV.md`, `AGENTS.md`, `AGENTS_MEMORY.md`
*   **Critical**: Do NOT assume only one document needs updating. Your changes may affect multiple documents.

### 5. 📦 Versioning & Changelog
*   **Update Version**: Increment the number in the `VERSION` file (root directory).
    *   *Patch (0.0.X)*: Bug fixes, tweaks, refactoring.
    *   *Minor (0.X.0)*: New features, new effects.
    *   You must also update `src\Version.h`.
*   **Update Changelog**: Add a concise entry to `CHANGELOG_DEV.md` under the new version number.

### 6. 🧠 Update Memory (Critical)
*   **Action**: If you encountered a build error, a command failure, or learned something new about the code structure, append it to `AGENTS_MEMORY.md`.
*   **Goal**: Help the *next* AI session avoid the same problem.

### 7. 📤 Delivery
*   **🚫 NO GIT STAGING**: You are strictly forbidden from running `git add`, `git commit`, or `git reset`.
    *   **Rule**: Modify files on disk ONLY.
    *   **Reason**: The user must review and stage changes manually.
    *   **No Exceptions**: Do not stage files even if you created them.
*   **Do Not Push**: Do not run `git push`.

*   **MANDATORY CHECKLIST**:
    *   [ ] **Documentation Scanned**: Did you scan all `.md` files and identify relevant docs?
    *   [ ] **Documentation Updated**: Did you update ALL relevant documentation (not just one file)?
    *   [ ] **Version Bumped**: Did you increment the number in `VERSION`?
    *   [ ] **Changelog Updated**: Did you add a section in `CHANGELOG_DEV.md`?
    *   [ ] **Tests Passed**: Did you verify with `run_tests`?

---

## 🌍 Environment & Constraints

*   **Target OS**: Windows 10/11.

---

## 🏗️ Architecture & Patterns

### 1. The Core Loop (400Hz)
*   **Component**: `FFBEngine` (Header-only: `FFBEngine.h`).
*   **Constraint**: Runs on a high-priority thread. **No memory allocation** (heap) allowed inside `calculate_force`.
*   **Math Rule (Critical)**: Use **Phase Accumulation** for vibrations.
    *   ❌ *Wrong*: `sin(time * frequency)` (Causes clicks when freq changes).
    *   ✅ *Right*: `phase += frequency * dt; output = sin(phase);`
*   **Safety**: All physics inputs involving `mTireLoad` must be clamped (e.g., `std::min(1.5, load_factor)`) to prevent hardware damage.

### 2. The GUI Loop (60Hz)
*   **Component**: `src/GuiLayer.cpp` (ImGui).
*   **Pattern**: **Producer-Consumer**.
    *   *Producer (FFB Thread)*: Pushes `FFBSnapshot` structs into `m_debug_buffer` every tick.
    *   *Consumer (GUI Thread)*: Calls `GetDebugBatch()` to swap the buffer and render *all* ticks since the last frame.
    *   *Constraint*: Never read `FFBEngine` state directly for plots; always use the snapshot batch to avoid aliasing.

### 3. Hardware Interface
*   **Component**: `src/DirectInputFFB.cpp`.
*   **Pattern**: Sends "Constant Force" updates.
*   **Optimization**: Includes a check `if (magnitude == m_last_force) return;` to minimize driver overhead.

---

## 📂 Key Documentation References

*   **Formulas**: `docs/dev_docs/FFB_formulas.md` (The math behind the code).
*   **Telemetry**: `docs/dev_docs/references/Reference - telemetry_data_reference.md` (Available inputs).

---

## 📝 Code Generation Guidelines

1.  **Adding New Effects**:
    *   Add a boolean toggle and gain float to `FFBEngine` class.
    *   Add a phase accumulator variable (`double m_effect_phase`) if it oscillates.
    *   Implement logic in `calculate_force`.
    *   Add UI controls in `GuiLayer::DrawTuningWindow`.
    *   Add visualization data to `FFBSnapshot` struct.

2.  **Modifying Config**:
    *   Update `src/Config.h` (declaration).
    *   Update `src/Config.cpp` (Save/Load logic).
    *   **Default to Safe**: New features should default to `false` or `0.0`.

3.  **Thread Safety**:
    *   Access to `FFBEngine` settings from the GUI thread must be protected by `std::lock_guard<std::mutex> lock(g_engine_mutex);`.

## 🚫 Common Pitfalls
*   **Do not** use `mElapsedTime` for sine waves (see Math Rule).