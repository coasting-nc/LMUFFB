# Code Review - Issue #265: Fix logger unknown car info

## Iteration 1

### Evaluation of the Solution

#### Core Functionality
The patch successfully addresses the core issue through two main mechanisms:
1.  **Early Synchronization**: It introduces `UpdateMetadata` in `FFBEngine`, which is called immediately after telemetry is copied in `main.cpp`. This ensures the engine's internal state is updated from the game's scoring data (which is usually available before physics telemetry).
2.  **Direct Metadata Access**: The auto-start logging block in `main.cpp` is refactored to pull metadata directly from the `g_localData` scoring structures, ensuring the most accurate information is captured at the moment the log file is created.

#### Safety & Side Effects
The patch introduced two significant risks in its initial version:
*   **Critical Logic Regression (Blocking)**: In `FFBEngine::calculate_force`, the `seeded` flag logic was broken, causing the REST API fallback to be triggered every physics frame.
*   **Potential Crash (Blocking)**: In `main.cpp`, the auto-start logging block accessed `vehScoringInfo[idx]` without a bounds check for `idx`.

### Merge Assessment

**Blocking Issues:**
1.  **FFB Engine Regression**: Fixed. `UpdateMetadataInternal` now returns a boolean indicating if a seed occurred.
2.  **Safety Check Missing**: Fixed. An bounds check for `idx` was added in `main.cpp`.

**Final Rating: #Correct#**
