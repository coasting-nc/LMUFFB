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
