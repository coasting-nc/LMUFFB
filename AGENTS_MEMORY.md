# Agent Knowledge Base

This document records technical constraints, architectural patterns, and environmental quirks discovered during development. Future agents should consult this to avoid repeating past analyses.

## 1. Environment & Build

### Linux Sandbox Constraints
The development environment is Linux-based, but the application is a Windows application relying on DirectX and DirectInput.
*   **Full Compilation:** Not possible in this environment. The `main.cpp` and `GuiLayer.cpp` depend on `<d3d11.h>`, `<dinput.h>`, and `<windows.h>`, which are unavailable in the Linux container.
*   **Test Compilation:** Unit tests **CAN** be built and run because `tests/test_ffb_engine.cpp` only links against the physics engine (`FFBEngine.h`), which uses standard C++ math libraries and simple structs.

### Verified Build Commands (Tests)
To verify logic changes in the physics engine, use the following sequence:

```bash
mkdir -p tests/build
cd tests/build
cmake ..
make
./run_tests
```

**Note:** The root `CMakeLists.txt` is designed for Windows (MSVC). The `tests/CMakeLists.txt` is the one relevant for verification in this environment.

### Git / Large Diff Issue
*   **Issue:** `git status`, `git fetch`, or other commands may fail with "The diff size is unusually large" if the repository state is significantly different or if build artifacts are not ignored.
*   **Workaround:** Rely on `read_file`, `overwrite_file`, and `replace_with_git_merge_diff` directly. Do not depend on bash commands for verification if this error occurs. Ensure `.gitignore` covers all build directories (e.g., `tests/build/`).

## 2. Critical Constraints & Math

### Coordinate Systems (rFactor 2 vs DirectInput)
*   **rFactor 2 / LMU:** Left-handed. +X = Left.
*   **DirectInput:** +Force = Right.
*   **Rule:** Lateral forces from the game (+X) must be INVERTED (negated) to produce the correct DirectInput force (Left).
*   **Common Pitfall:** Using `abs()` on lateral velocity destroys directional information needed for counter-steering logic. Always preserve the sign until the final force calculation.

### Phase Accumulation (Anti-Glitch)
To generate vibration effects (Lockup, Spin, Road Texture) without audio-like clicking or popping artifacts:
*   **Pattern:** Never calculate `sin(time * freq)`.
*   **Correct Approach:** Use an accumulator `m_phase += freq * dt * TWO_PI`.
*   **Why:** Frequency changes dynamically based on car speed. If you use absolute time, a sudden frequency change causes a discontinuity in the sine wave phase, resulting in a "pop". Integrating delta-phase ensures the wave is continuous.

### Producer-Consumer Visualization
To avoid "aliasing" (square-wave look) in the GUI graphs:
*   **Physics Rate:** 400Hz.
*   **GUI Rate:** 60Hz.
*   **Problem:** Sampling the physics value once per GUI frame misses high-frequency spikes and vibrations.
*   **Solution:** `FFBEngine` acts as a **Producer**, pushing *every* sample (400Hz) into a thread-safe `std::vector<FFBSnapshot>`. `GuiLayer` acts as a **Consumer**, grabbing the entire batch every frame and plotting all points.
*   **Mechanism:** `m_debug_mutex` protects the swap of the buffer.

## 3. Workarounds

### Git Syncing
*   **Issue:** `git pull` often hangs or fails in this environment due to credential prompts or history mismatches.
*   **Workaround:** Use the following sequence to force a sync with the remote state:
    ```bash
    git fetch && git reset --hard origin/main
    ```

### ImGui Warnings
*   **Issue:** `ImGui::PlotLines` expects `int` for the count, but `std::vector::size()` returns `size_t`.
*   **Fix:** Always cast the size: `(int)plot_data.size()`.

## 4. Recent Architectural Changes (v0.3.x - v0.4.x)

### v0.4.20: Coordinate System Stability
*   **Lesson:** Fixed positive feedback loops in Scrub Drag and Yaw Kick by inverting their logic. Stability tests must verify DIRECTION (Negative/Positive) not just magnitude.

### v0.4.19: Coordinate System Overhaul
*   **Lesson:** Verified that rFactor 2 uses +X=Left. All lateral inputs (SoP, Rear Torque, Scrub Drag) must be inverted to produce negative (Left) force for DirectInput.

### v0.4.18: Smoothing
*   **Lesson:** Yaw Acceleration is noisy (derivative of velocity). Must be smoothed (LPF) before use in FFB to avoid feedback loops with vibration effects.

### v0.3.20: Documentation Discipline
*   **Lesson:** Every submission **MUST** include updates to `VERSION` and `CHANGELOG.md`. This is now enforced in `AGENTS.md`.

## 5. Documentation Maintenance

### Documentation Scanning Process
When making changes to the codebase, you **must** follow this documentation update process:

1.  **Scan All Documentation**: Use `find_by_name` with pattern `*.md` to discover all markdown files in the project.
2.  **Identify Relevant Files**: Review file names and paths to determine which documents might be affected by your changes.
3.  **Read Before Updating**: Always read the current content of documentation files before updating them to understand context and maintain consistency.
4.  **Update Comprehensively**: Don't stop at the first document - your changes may affect multiple files across different directories.

### Documentation Categories
*   **User Documentation** (`docs/`): End-user guides, feature descriptions, troubleshooting
*   **Developer Documentation** (`docs/dev_docs/`): Technical specs, formulas, architecture, investigations
*   **Root Documentation**: `README.md`, `CHANGELOG.md`, `AGENTS.md`, `AGENTS_MEMORY.md`

### Common Documentation Update Patterns
*   **New FFB Effect**: Update `docs/ffb_effects.md`, `docs/the_physics_of__feel_-_driver_guide.md`, `docs/dev_docs/FFB_formulas.md`, and `README.md`
*   **LMU 1.2 Changes**: Update `docs/dev_docs/new_ffb_features_enabled_by_lmu_1.2.md` and `README.md`
*   **Architecture Changes**: Update `docs/architecture.md` and potentially `AGENTS.md` if it affects development workflow
*   **Bug Fixes**: Update `CHANGELOG.md` and consider updating `docs/dev_docs/TODO.md`

### Documentation Anti-Patterns
*   ❌ **Don't** assume only one document needs updating
*   ❌ **Don't** skip reading existing documentation before editing
*   ❌ **Don't** forget to update user-facing docs when adding features
*   ❌ **Don't** leave outdated information in documentation after making changes

### Keeping Documentation Knowledge Current (CRITICAL)
**Pattern: Review Docs After Git Sync**

After performing `git fetch` or `git pull`, you **must** review what documentation has changed to stay current with the project:

*   **Why This Matters**: 
    *   Documentation changes reflect evolving architecture, new features, API updates, and critical fixes
    *   Outdated knowledge leads to incorrect implementations and breaking changes
    *   The project evolves between sessions - you must catch up before making changes

*   **How to Check for Changes**:
    ```bash
    # See which markdown files changed since last session
    git diff --name-only HEAD@{1} HEAD -- '*.md'
    ```

*   **What to Read**:
    *   **Always read** any files shown by the diff command
    *   **Priority files** if they changed:
        *   `docs/dev_docs/telemetry_data_reference.md` - API units and field names (source of truth)
        *   `docs/dev_docs/FFB_formulas.md` - Scaling constants and physics equations
        *   `docs/architecture.md` - System components and design patterns
        *   `README.md` - User features and setup instructions
        *   `CHANGELOG.md` - What changed and when
        *   `AGENTS_MEMORY.md` - Lessons from previous sessions

*   **Example**: If `telemetry_data_reference.md` was updated to document the Force→Torque unit change in LMU 1.2, you must read it to understand that `mSteeringShaftTorque` is in Newton-meters, not Newtons. Without this knowledge, you might use incorrect scaling factors.

**Action Item**: Make reviewing changed documentation the **second step** of every session (right after reading AGENTS_MEMORY.md).

## 6. Grip Calculation Logic (v0.4.6)

See: docs\dev_docs\avg_load_issue.md

### Fallback Mechanism
*   **Behavior**: When telemetry grip (`mGripFract`) is 0.0 but load is present, the engine approximates grip from slip angle.
*   **Front vs Rear**: As of v0.4.6, this logic applies to BOTH front and rear wheels.
*   **Constraint**: The fallback triggers if `avg_grip < 0.0001` AND `avg_load > 100.0`.
    *   *Gotcha*: `avg_load` is currently calculated from **Front Wheels Only**. This means rear fallback depends on front loading. This works for most cases (grounded car) but requires care in synthetic tests (must set front load even when testing rear behavior).

### Diagnostics
*   **Struct**: `GripDiagnostics m_grip_diag` tracks whether approximation was used and the original values.
*   **Why**: Original telemetry values are overwritten by the fallback logic. To debug or display "raw" data, use `m_grip_diag.original` instead of the modified variables.

## 7. Continuous Physics State (Anti-Glitch)

### Continuous Physics State (Anti-Glitch)
*   **Rule:** Never make the calculation of physics state variables (like Slip Angle, RPM smoothing, or LPFs) conditional on telemetry health or other flags.
*   **Why:** 
    1.  **Filters:** Low Pass Filters (LPF) rely on a continuous stream of `dt` updates. If you stop calling them, their internal state becomes stale. When you call them again, they produce a spike.
    2.  **Downstream Dependencies:** A variable calculated in a "Fallback" block (like `slip_angle` in `calculate_grip`) might be used by a completely different effect later (like `Rear Aligning Torque`).
*   **Incident:** See `docs/dev_docs/bug_analysis_rear_torque_instability.md`. We caused violent wheel kicks by only calculating Slip Angle when Grip was missing.


## 8. Git & Repo Management

### Submodule Trap
*   **Issue:** Cloning a repo inside an already initialized repo (even if empty) can lead to nested submodules or detached git states.
*   **Fix:** Ensure the root directory is correctly initialized or cloned into. If working in a provided sandbox with `.git`, configure the remote and fetch rather than cloning into a subdirectory.

### File Operations
*   **Lesson:** When moving files from a nested repo to root, ensure hidden files (like `.git`) are handled correctly or that the root `.git` is properly synced.
*   **Tooling:** `replace_with_git_merge_diff` requires exact context matching. If files are modified or desynchronized, `overwrite_file_with_block` is safer.

## 9. Repository Handling (Read-Only Mode)
*   **No Git Push:** You do not have write access to the remote repository. Never attempt `git push`.
*   **Delivery:** Your final output is the modified files (which the user will download as a ZIP), not a git commit.
