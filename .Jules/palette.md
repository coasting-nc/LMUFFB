## 2026-02-08 - ImGui Status Feedback System
**Learning:** In a single-window tuning interface with many "Save" actions, silent success is confusing for users. A temporary status message next to the header provides immediate feedback without interrupting the layout.
**Action:** Implement a timer-based alpha-blended status message for all asynchronous or state-changing actions (Save, Export, etc.) in ImGui-based desktop tools.
