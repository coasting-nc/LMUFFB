You will have to work on the files downloaded from this repo https://github.com/coasting-nc/LMUFFB and start working on the tasks described below. Therefore, if you haven't done it already, clone this repo https://github.com/coasting-nc/LMUFFB and start working on the tasks described below.

Please initialize this session by following the **Standard Task Workflow** defined in `AGENTS.md`.

1.  **Sync**: Run `git fetch && git reset --hard origin/main` for the LMUFFB repository to ensure you see the latest files.
1.  **Load Memory**: Read `AGENTS_MEMORY.md` from the root dir of the LMUFFB repository to review build workarounds and architectural insights. 
2.  **Load Rules**: Read `AGENTS.md` from the root dir of the LMUFFB repository to confirm instructions. 

Once you have reviewed these documents, please proceed with the following task:

**Task: Fix Intermittent FFB Instability, Refactor Config, and Add Regression Tests**

**Reference Documents:**
*   `docs/bug_reports/Intermittent Reversed FFB Feel.md` (Contains the full analysis and the specific code snippets to implement).
*   `docs/bug_reports/refactor presets declaration with Fluent Builder Pattern .md` (contains the details about the `Config.cpp` preset loading)


**Context:**
We have identified a critical logic error in `FFBEngine.h` where physics state variables (Slip Angle, LPF history) were only being updated conditionally. This caused the "Rear Aligning Torque" effect to toggle on/off randomly based on telemetry health, resulting in violent "reverse FFB" kicks. Additionally, the `Config.cpp` preset loading is currently verbose and hard to read.

**Implementation Requirements:**

1.  **Fix Physics Instability (`FFBEngine.h`)**:
    *   Extract the code from the **"Corrected Code for FFBEngine.h"** section in the reference document.
    *   In `calculate_grip`: Move the `calculate_slip_angle` call **outside** the conditional block so it runs every frame. Add the "CRITICAL LOGIC FIX" comments provided in the reference.
    *   In `calculate_force`: Move the state updates for `m_prev_vert_deflection` (Road Texture) and `m_prev_susp_force` (Bottoming) **outside** their respective `if (enabled)` blocks to the end of the function. They must update unconditionally every frame.

2.  **Refactor Configuration (`src/Config.h` & `src/Config.cpp`)**:
    *   Extract the code from the **"Step 1: Update src/Config.h"** and **"Step 2: Update src/Config.cpp"** sections in the reference document.
    *   Modify the `Preset` struct to use the **Fluent Builder Pattern** (chained setters) and initialize defaults inline.
    *   Rewrite `Config::LoadPresets` to use this new readable syntax.

3.  **Add Tests (`tests/test_ffb_engine.cpp`)**:
    *   Extract the code from the **"Regression Tests"** and **"Stress Test"** sections in the reference document.
    *   Add `test_regression_road_texture_toggle`, `test_regression_bottoming_switch`, `test_regression_rear_torque_lpf`, and `test_stress_stability`.
    *   Update `main()` to call these new tests.

4.  **Documentation**:
    *   Create a new file `docs/dev_docs/bug_analysis_rear_torque_instability.md` using the content provided in the reference document.
    *   Update `AGENTS_MEMORY.md` by appending the "Continuous Physics State" rule found in the reference document.

**Deliverables:**
1.  Modified `FFBEngine.h` (Stable physics).
2.  Modified `src/Config.h` and `src/Config.cpp` (Clean presets).
3.  Modified `tests/test_ffb_engine.cpp` (New tests added and enabled).
4.  New `docs/dev_docs/bug_analysis_rear_torque_instability.md`.
5.  Updated `AGENTS_MEMORY.md`.
6.  **Verification**: Run the tests (`./run_tests` in the build folder) and ensure the new regression tests PASS.
