# AI Memory & Knowledge Base

**To the AI (Jules):**
This file represents your "Long-Term Memory". It contains lessons learned from previous sessions.
*   **READ** this before starting any task to avoid repeating past mistakes.
*   **WRITE** to this if you discover a new workaround, a build quirk, or an architectural insight that will help future instances of yourself.

---

## 🔧 Environment & Command Workarounds

### Git Synchronization
*   **Issue**: `git pull` often fails because the environment is read-only or lacks credentials for the upstream repo.
*   **Solution**: Use `git fetch` followed by `git reset --hard origin/main` (or specific branch) if strictly necessary, but generally, just work with the files present in the VM. Do not attempt to push.

### CMake & Build
*   **Constraint**: The Linux VM cannot build the `LMUFFB.exe` target (Windows only).
*   **Working Command**: Always target the tests directory explicitly:
    ```bash
    mkdir -p build_tests && cd build_tests
    cmake ../tests
    cmake --build .
    ```

---

## 🧠 Architectural Insights

### Telemetry Data
*   **Observation**: LMU telemetry is sometimes inconsistent.
*   **Strategy**: When adding new effects based on telemetry, always check if the value is non-zero in `tests/test_ffb_engine.cpp` before assuming it works in the game.

### Thread Safety
*   **Critical**: The GUI thread (Consumer) and FFB thread (Producer) share data.
*   **Rule**: Never access `FFBEngine` members directly from `GuiLayer` without locking `g_engine_mutex`, OR use the `GetDebugBatch()` snapshot system which handles locking internally.