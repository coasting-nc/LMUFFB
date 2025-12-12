Here is the prompt to instruct the AI coding agent to focus specifically on adding numerical diagnostics to the graphs.

***

Please initialize this session by following the **Standard Task Workflow** defined in `AGENTS.md`.

1.  **Sync**: Run `git fetch && git reset --hard origin/main` (or pull).
2.  **Load Memory**: Read `AGENTS_MEMORY.md`.
3.  **Load Rules**: Read `AGENTS.md`.

Perform the following task:

**Task: Implement Numerical Readouts for Troubleshooting Graphs (Diagnostics)**

**Context:**
We are investigating why certain FFB channels (SoP, Understeer, Road Texture) appear "dead" (flatlined) in the GUI graphs. It is unclear if the values are truly zero (logic bug) or just very small (scaling issue).
To diagnose this, we need **precise numerical readouts** displayed alongside the visual plots.

**Implementation Requirements:**

**1. Update `GuiLayer.cpp` (DrawDebugWindow)**
Modify the plotting logic to calculate and display the **Current Value**, **Minimum**, and **Maximum** for every channel in the "FFB Components" and "Telemetry Inspector" sections.

*   **Helper Function:** Create a helper (e.g., `GetMinMaxAvg`) that iterates through a `RollingBuffer` to find the min/max/current values.
*   **Display Format:** Change the UI layout for each plot to:
    ```text
    [Graph Title] | Val: X.XXX | Min: Y.YY | Max: Z.ZZ
    [PlotLines Drawing]
    ```
*   **Target Channels:** Apply this to **ALL** plots (Total, Base, SoP, Understeer, Road, Slide, Load, Grip, etc.).

**2. Precision:**
*   Use `%.3f` or `%.4f` formatting. We need to see tiny values (e.g., `0.0015`) that might look like zero on a graph.

**Deliverables:**
1.  **Source Code:** Updated `GuiLayer.cpp`.
2.  **Documentation:** Update `CHANGELOG.md` (Added numerical diagnostics to debug window).

**Constraints:**
*   **Performance:** Calculating Min/Max for 4000 points every frame (60Hz) for 20+ graphs might be heavy.
    *   *Optimization:* Only calculate Min/Max every 10th frame, OR just display the **Current Value** (index `offset - 1`) which is instant.
    *   *Decision:* **Display "Current Value" is mandatory and instant.** Min/Max is optional/nice-to-have; if you implement Min/Max, ensure it doesn't stall the GUI thread (maybe scan only a subset or use a cached value).
