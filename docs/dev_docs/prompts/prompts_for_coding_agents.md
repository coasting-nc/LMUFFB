
Please initialize this session by following the **Standard Task Workflow** defined in `AGENTS.md`.

1.  **Sync**: Run `git fetch && git reset --hard origin/main` (or pull) to ensure you see the latest files.
2.  **Load Memory**: Read `AGENTS_MEMORY.md` to review build workarounds and architectural insights.
3.  **Load Rules**: Read `AGENTS.md` to confirm constraints.

Perform the following task:

**Task: verify issues and implement fixes**

Carefully read the document "docs\dev_docs\grip_calculation_analysis_v0.4.5.md", verify all issues described and, if confirmed, implement the recommended fixes.

===

Clone this repo https://github.com/coasting-nc/LMUFFB and start working on the tasks described below.

Please initialize this session by following the **Standard Task Workflow** defined in `AGENTS.md`.

1.  **Sync**: Run `git fetch && git reset --hard origin/main` (or pull) to ensure you see the latest files.
2.  **Load Memory**: Read `AGENTS_MEMORY.md` to review build workarounds and architectural insights.
3.  **Load Rules**: Read `AGENTS.md` to confirm constraints.
4.  
Once you have reviewed these documents, please proceed with the following task:

**Task: (...)**


**Context:**
(...)

**Implementation Requirements:**
(...)

**Deliverables:**
(...)



===

Here is the prompt to instruct the AI coding agent.

***

Please initialize this session by following the **Standard Task Workflow** defined in `AGENTS.md`.

1.  **Sync**: Run `git fetch && git reset --hard origin/main` (or pull) to ensure you see the latest files.
2.  **Load Memory**: Read `AGENTS_MEMORY.md` to review build workarounds and architectural insights.
3.  **Load Rules**: Read `AGENTS.md` to confirm constraints.

Perform the following task:

**Task: Implement Physics Workarounds, Advanced Slip Calculations, and New FFB Effects (v0.4.5)**

**Context:**
Le Mans Ultimate (LMU) version 1.2 currently returns 0.0 for `mTireLoad` and `mGripFract` in the shared memory, breaking several FFB effects. We need to implement physics-based approximations to restore this functionality. Additionally, we want to leverage available data (Patch Velocity, Wheel Rotation, Ride Height) to create new effects and more robust slip calculations.
Refer to **`docs/dev_docs/workaounds_and_improvements_ffb_v0.4.4+.md`** for the detailed physics analysis and formulas required for this task.

**Implementation Requirements:**

1.  **Physics Approximations (Workarounds):**
    *   **Approximate `mTireLoad`:** Create a calculated value `calc_load`.
        *   Formula: `mSuspForce` + (Estimated Unsprung Mass ~300N).
        *   *Optional Enhancement:* Add `(mFrontDownforce / 2)` if available in `TelemInfoV01`.
    *   **Approximate `mGripFract`:** Create a calculated value `calc_grip`.
        *   Logic: Derive from Slip Angle. If `abs(SlipAngle) > Optimal (0.15 rad)`, reduce grip.
        *   Formula: `1.0 - max(0.0, (abs(calculated_slip_angle) - 0.15) * falloff_factor)`.

2.  **Advanced Slip & Lockup Calculation:**
    *   Implement a **Manual Slip Ratio** calculation to detect Lockups/Spin even if game telemetry is broken.
    *   **Formula:** `Ratio = (V_wheel - V_car) / V_car`.
        *   `V_wheel`: `mRotation` (rad/s) * `Radius` (m).
        *   `Radius`: Convert `mStaticUndeflectedRadius` (unsigned char, cm) to meters (`val / 100.0`).
        *   `V_car`: `mLocalVel.z`.
    *   **GUI Option:** Add a toggle/dropdown to select the source for Lockup/Spin effects: **"Game Data (mSlipRatio)"** vs **"Calculated (Wheel Speed)"**.

3.  **Universal Bottoming Logic:**
    *   Implement two detection methods for the Bottoming effect:
        *   **Method A (Scraping):** Trigger if `mRideHeight` < Threshold (e.g., 0.002m).
        *   **Method B (Suspension Force):** Trigger if derivative of `mSuspForce` exceeds a threshold (Spike detection).
    *   **GUI Option:** Add a setting to choose between "Chassis Scraping" or "Suspension Bump Stops".

4.  **New & Improved Effects:**
    *   **Scrub Drag:** New effect. If `mLateralPatchVel` is high, add a constant force *opposing* the slide direction to simulate rubber dragging resistance.
    *   **Alternative Understeer:** Use the `calc_grip` (from Step 1) to drive the Understeer effect if the raw `mGripFract` is detected as 0.

5.  **GUI & Visualization (`GuiLayer.cpp`):**
    *   Add sliders/checkboxes for all new options (Calculation Methods, Scrub Drag Gain).
    *   Update **Troubleshooting Graphs**: Add traces for `Calc Load`, `Calc Grip`, `Calc Slip Ratio`, and `Ride Height`.

6.  **Safety & Robustness:**
    *   Add sanity checks for all new inputs (`mSuspForce`, `mRideHeight`, `mStaticUndeflectedRadius`).
    *   Print a "One-time" console warning if these new inputs are missing/zero.

**Deliverables:**
1.  **Source Code:** Updated `FFBEngine.h`, `GuiLayer.cpp`, `Config.h`, `Config.cpp`.
2.  **Tests:** Update `tests/test_ffb_engine.cpp` to verify:
    *   Manual Slip Ratio calculation (math correctness).
    *   Radius unit conversion (cm to m).
    *   Bottoming logic triggers (Ride height vs Force).
3.  **Documentation:**
    *   Update `docs/dev_docs/FFB_formulas.md` with the new approximation formulas.
    *   Update `docs/ffb_effects.md` describing the new "Scrub Drag" and "Universal Bottoming".
    *   Update `docs/dev_docs/telemetry_data_reference.md` marking the new fields (`mSuspForce`, `mRideHeight`, etc.) as "Used".
    *   Update `CHANGELOG.md` (v0.4.5).

**Constraints:**
*   **Thread Safety:** Ensure `Config` changes from GUI do not race with `FFBEngine` reads (use the existing mutex).
*   **Performance:** Do not allocate memory inside `calculate_force`.
*   **Units:** Pay strict attention to `mStaticUndeflectedRadius` being in **centimeters** (integer). Cast to double before division.

===


**Constraints:**
(...)

===

Please initialize this session by following the **Standard Task Workflow** defined in `AGENTS.md`.

1.  **Sync**: Run `git fetch && git reset --hard origin/main` (or pull) to ensure you see the latest files.
2.  **Load Memory**: Read `AGENTS_MEMORY.md` to review build workarounds and architectural insights.
3.  **Load Rules**: Read `AGENTS.md` to confirm constraints.

Perform the following task:

Read document docs\dev_docs\FFB App Issues And Debugging v0.4.2.md and implement the requested fixes for the described issues.

**Task: (...)**

**Context:**
(...)

**Implementation Requirements:**
(...)

**Deliverables:**
(...)

**Constraints:**
(...)



=====


Clone this repo https://github.com/coasting-nc/LMUFFB and start working on the tasks described below.

Please initialize this session by following the **Standard Task Workflow** defined in `AGENTS.md`.

1.  **Sync**: Run `git fetch && git reset --hard origin/main` (or pull) to ensure you see the latest files.
2.  **Load Memory**: Read `AGENTS_MEMORY.md` to review build workarounds and architectural insights.
3.  **Load Rules**: Read `AGENTS.md` to confirm constraints.

Once you have reviewed these documents, please proceed with the following task:

**Task: (...)**

**Reference Documents:**
(...)

**Context:**
(...)

**Implementation Requirements:**
(...)

**Deliverables:**
(...)


=====

Clone this repo https://github.com/coasting-nc/LMUFFB and start working on the tasks described below.

Please initialize this session by following the **Standard Task Workflow** defined in `AGENTS.md`.

1.  **Sync**: Run `git fetch && git reset --hard origin/main` (or pull) to ensure you see the latest files.
2.  **Load Memory**: Read `AGENTS_MEMORY.md` to review build workarounds and architectural insights.
3.  **Load Rules**: Read `AGENTS.md` to confirm constraints.

Perform the following task:

**Task: Implement Rear Physics Workarounds, Fix GUI Scaling, and Tune Defaults (v0.4.10)**

**Reference Documents:**
*   `docs\dev_docs\Rear Physics Workarounds & GUI Scaling (v0.4.10).md` (Contains detailed formulas and logic).

**Context:**
Analysis of the current version reveals three critical issues:
1.  **Dead Rear Effects:** The "Rear Align Torque" effect is flat because LMU 1.2 reports 0.0 for `mLateralForce` (just like it does for Tire Load). We must approximate this force using our calculated slip angles and loads.
2.  **Invisible Data (Scaling Bug):** The Physics Engine now outputs Torque (Nm, range ~0-20), but the GUI plots are still scaled for Force (Newtons, range ±1000). This makes effects like SoP, Understeer Cut, and Road Texture appear as flat lines even when active.
3.  **Usability:** The default SoP Scale (5.0) is too weak for the new Nm math, and the GUI slider for it has an incorrect minimum value (100.0), preventing proper tuning.

**Implementation Requirements:**

**1. Physics Engine Updates (`FFBEngine.h`)**
*   **New Helper:** `approximate_rear_load()` (Implement same logic as front: `mSuspForce` + 300N).
*   **New Calculation:** `calc_rear_lat_force`.
    *   **Formula:** `RearSlipAngle (Raw) * CalcRearLoad * 15.0` (Stiffness constant).
    *   *Note:* Use `m_grip_diag.rear_slip_angle` (calculated in v0.4.7) and the new `calc_rear_load`.
    *   **Safety:** Clamp the result to **±6000.0 N** to prevent explosions if slip angle spikes.
*   **Fix Rear Align Torque:**
    *   Replace the dead `data->mWheel[i].mLateralForce` input with `calc_rear_lat_force` in the `rear_torque` calculation.
*   **Update Snapshot:**
    *   Populate `ffb_rear_torque` with the new calculated value.
    *   Add `calc_rear_load` to the snapshot struct (we only had front before) and populate it.

**2. Tuning & Defaults (`Config.cpp` / `FFBEngine.h`)**
*   **Boost SoP Scale:** Change default `m_sop_scale` from **5.0** to **20.0**.
    *   *Reason:* 1G * 0.15 Gain * 20 Scale = 3.0 Nm. This provides a feelable baseline.
*   **Update Presets:** Update the "Default" preset in `Config.cpp` to use `sop_scale = 20.0`.

**3. GUI Updates (`GuiLayer.cpp`)**
*   **Fix Slider Bug:** Change `SoP Scale` slider range from `100.0f, 5000.0f` to **`0.0f, 200.0f`**.
*   **Fix Plot Scales (CRITICAL):**
    *   Change `min_scale`/`max_scale` for **ALL** FFB Component plots to **-20.0f, 20.0f**.
    *   *Affected Plots:* Base Torque, SoP, Oversteer Boost, Rear Align Torque, Scrub Drag, Understeer Cut, Road Texture, Slide Texture, Lockup/Spin/Bottoming.
    *   *Exception:* `Total Output` should remain -1.0 to 1.0.
*   **Update Plots:**
    *   **Header B (Internal Physics):** Change `Calc Front Load` to `Calc Load (Front/Rear)`.
        *   Plot Front Load in **Cyan**.
        *   Plot Rear Load in **Magenta** (on top, transparent background).
        *   *Tooltip:* "Cyan: Front, Magenta: Rear".
    *   **Header C (Raw Telemetry):** Change `Raw Rear Lat Force` to `Calc Rear Lat Force` (since Raw is dead, show our workaround).

**Deliverables:**
1.  **Code:** Updated `FFBEngine.h`, `Config.cpp`, `GuiLayer.cpp`.
2.  **Docs:** Update `CHANGELOG.md` (v0.4.10).
3.  **Tests:** Update `tests/test_ffb_engine.cpp`.
    *   Add a test case verifying `rear_torque` is non-zero when `mLateralForce` input is 0 but `RearSlipAngle` is non-zero.

**Constraints:**
*   **Safety:** Ensure the calculated rear lateral force is clamped.
*   **Visuals:** Ensure the multi-line plot for Load uses distinct colors and resets cursor position correctly.


# Prompt template to fix git repo cloning issues

Please clone this repo https://github.com/coasting-nc/LMUFFB and start working on the tasks described below.

**⚠️ CRITICAL ENVIRONMENT SETUP (Perform these steps first):**
1.  **Clone to Temp:** Clone the repository into a temporary folder (e.g., `git clone https://github.com/coasting-nc/LMUFFB temp_repo`).
2.  **Flatten:** Move all contents from `temp_repo` into the current root directory (`.`).
3.  **Detach (Crucial):** **DELETE the `.git` folder** that came with the clone (`rm -rf .git`).
    *   *Reason:* I do not have write access to the remote. You must treat these as local files only. This prevents "Submodule" errors (mode 160000) and prevents you from attempting `git push`.
4.  **Verify:** Ensure the files (like `src/`, `CMakeLists.txt`) are in the root directory and that no `.git` folder exists.

**Standard Task Workflow:**
*   Load Memory: Read `AGENTS_MEMORY.md`.
*   Load Rules: Read `AGENTS.md`.

Once you have reviewed these documents and performed the setup, please proceed with the following task:

**Task: (...)**

**Reference Documents:**
(...)

**Context:**
(...)

**Implementation Requirements:**
(...)

**Deliverables:**
(...)

### Git / Large Diff Issue
*   **Issue:** `git status`, `git fetch`, or other commands may fail with "The diff size is unusually large" if the repository state is significantly different or if build artifacts are not ignored.
*   **Workaround:** Rely on `read_file`, `overwrite_file`, and `replace_with_git_merge_diff` directly. Do not depend on bash commands for verification if this error occurs. Ensure `.gitignore` covers all build directories (e.g., `tests/build/`).


# Prompt
You will have to work on the files downloaded from this repo https://github.com/coasting-nc/LMUFFB and start working on the tasks described below. Therefore, if you haven't done it already, clone this repo https://github.com/coasting-nc/LMUFFB and start working on the tasks described below.

Please initialize this session by following the **Standard Task Workflow** defined in `AGENTS.md`.

Please clone this repo https://github.com/coasting-nc/LMUFFB and start working on the tasks described below.

**⚠️ CRITICAL ENVIRONMENT SETUP (Perform these steps first):**

1.  **Clone to Temp:** Clone the repository into a temporary folder (e.g., `git clone https://github.com/coasting-nc/LMUFFB temp_repo`).
2.  **Flatten:** Copy the contents from `temp_repo` to the current directory. Use `cp -r temp_repo/* .` (This is safer than moving).
3.  **Detach:** Delete the `.git` folder (`rm -rf .git`) and the temp folder (`rm -rf temp_repo`).
    *   *Reason:* I do not have write access to the remote. You must treat these as local files only.
4.  **Error Handling (Crucial):**
    *   If **ANY** of the above steps fail (e.g., git failure, permission denied, file not found), you **MUST** immediately append the specific error message to `AGENTS_MEMORY.md` under a header `## Initialization Error`.
    *   If you cannot write to the file, print the error clearly in the chat.

**Standard Task Workflow:**
*   Load Memory: Read `AGENTS_MEMORY.md`.
*   Load Rules: Read `AGENTS.md`.

Once you have reviewed these documents and performed the setup, please proceed with the following task:

**Task: (...)**

**Reference Documents:**
(...)

**Context:**
(...)

**Implementation Requirements:**
(...)

**Deliverables:**
(...)

### Git / Large Diff Issue
*   **Issue:** `git status`, `git fetch`, or other commands may fail with "The diff size is unusually large" if the repository state is significantly different or if build artifacts are not ignored.
*   **Workaround:** Rely on `read_file`, `overwrite_file`, and `replace_with_git_merge_diff` directly. Do not depend on bash commands for verification if this error occurs. Ensure `.gitignore` covers all build directories (e.g., `tests/build/`).


#### Git & Repo Management

##### Submodule Trap
*   **Issue:** Cloning a repo inside an already initialized repo (even if empty) can lead to nested submodules or detached git states.
*   **Fix:** Ensure the root directory is correctly initialized or cloned into. If working in a provided sandbox with `.git`, configure the remote and fetch rather than cloning into a subdirectory.

##### File Operations
*   **Lesson:** When moving files from a nested repo to root, ensure hidden files (like `.git`) are handled correctly or that the root `.git` is properly synced.
*   **Tooling:** `replace_with_git_merge_diff` requires exact context matching. If files are modified or desynchronized, `overwrite_file_with_block` is safer.




# Prompt

You will have to work on the files downloaded from this repo https://github.com/coasting-nc/LMUFFB and start working on the tasks described below. Therefore, if you haven't done it already, clone this repo https://github.com/coasting-nc/LMUFFB and start working on the tasks described below.

Please initialize this session by following the **Standard Task Workflow** defined in `AGENTS.md`.

1.  **Sync**: Run `git fetch && git reset --hard origin/main` for the LMUFFB repository to ensure you see the latest files.
2.  **Load Memory**: Read `AGENTS_MEMORY.md` from the root dir of the LMUFFB repository to review build workarounds and architectural insights. 
3.  **Load Rules**: Read `AGENTS.md` from the root dir of the LMUFFB repository to confirm instructions. 

Once you have reviewed these documents, please proceed with the following task:

**Task: (...)**

**Reference Documents:**
(...)

**Context:**
(...)

**Implementation Requirements:**
(...)

**Deliverables:**
(...)

**Check-list for completion:**
(...)

### Git / Large Diff Issue
*   **Issue:** `git status`, `git fetch`, or other commands may fail with "The diff size is unusually large" if the repository state is significantly different or if build artifacts are not ignored.
*   **Workaround:** Rely on `read_file`, `overwrite_file`, and `replace_with_git_merge_diff` directly. Do not depend on bash commands for verification if this error occurs. Ensure `.gitignore` covers all build directories (e.g., `tests/build/`).


#### Git & Repo Management

##### Submodule Trap
*   **Issue:** Cloning a repo inside an already initialized repo (even if empty) can lead to nested submodules or detached git states.
*   **Fix:** Ensure the root directory is correctly initialized or cloned into. If working in a provided sandbox with `.git`, configure the remote and fetch rather than cloning into a subdirectory.

##### File Operations
*   **Lesson:** When moving files from a nested repo to root, ensure hidden files (like `.git`) are handled correctly or that the root `.git` is properly synced.
*   **Tooling:** `replace_with_git_merge_diff` requires exact context matching. If files are modified or desynchronized, `overwrite_file_with_block` is safer.

