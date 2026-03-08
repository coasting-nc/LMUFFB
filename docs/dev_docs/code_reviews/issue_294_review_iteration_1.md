**Review Iteration 1 - Issue #294**

**Summary of Feedback:**
1. **Missing GUI Updates (BLOCKING):** Claimed that `src/GuiLayer_Common.cpp` needs updates to match renamed members (`s_front_load`, etc.).
2. **Missing Version & Changelog (BLOCKING):** VERSION and CHANGELOG updates were missing.
3. **Semantic Inconsistency:** `calculate_grip` parameter rename to `avg_front_load` was misleading for a shared function.
4. **Stray Files:** Internal log files were included in the diff.

**My Response and Actions:**
1. **GUI Updates:** I performed a thorough search and confirmed that `src/GuiLayer_Common.cpp` does NOT access the renamed members (`s_front_load`, `s_front_grip`, `s_torque`) directly. It uses `FFBSnapshot` fields which already had consistent naming. The build succeeded without any changes to this file, proving it was not blocking.
2. **Version & Changelog:** Updated `VERSION` to `0.7.151` and added a detailed entry to `CHANGELOG_DEV.md`.
3. **Semantic Inconsistency:** Renamed the `calculate_grip` parameter from `avg_front_load` to `reference_load` to maintain axle-agnostic semantics for this utility function.
4. **Stray Files:** Deleted all generated `.log` files and `test_normalization.ini`.

**Final Status:** All issues addressed. Build and tests passed.
