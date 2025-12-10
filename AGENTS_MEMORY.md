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

## 2. Critical Constraints & Math

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

## 4. Recent Architectural Changes (v0.3.x)

### v0.3.20: Documentation Discipline
*   **Lesson:** Every submission **MUST** include updates to `VERSION` and `CHANGELOG.md`. This is now enforced in `AGENTS.md`.

### v0.3.18: Decoupled Plotting
*   Refactored `FFBEngine` to store debug snapshots in `m_debug_buffer`.
*   Updated `GuiLayer` to consume batches, enabling "oscilloscope" style visualization.

### v0.3.17: Thread Safety & vJoy Split
*   **Mutex:** Added `std::lock_guard` in `GuiLayer::DrawDebugWindow` to prevent race conditions when reading shared engine state.
*   **vJoy:** Split functionality into two toggles:
    1.  `m_enable_vjoy`: Acquires/Relinquishes the device.
    2.  `m_output_ffb_to_vjoy`: Writes FFB data to Axis X.
    *   *Purpose:* Allows users to release the vJoy device so external feeders (Joystick Gremlin) can use it, while still keeping the app running.

### v0.3.16: SoP Config
*   Replaced hardcoded `1000.0` scaling for Seat of Pants effect with configurable `m_sop_scale` (exposed in GUI).

### v0.3.14: Dynamic vJoy
*   Implemented a state machine in `main.cpp` to dynamically acquire/release vJoy at runtime based on GUI checkboxes, without needing a restart.

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

