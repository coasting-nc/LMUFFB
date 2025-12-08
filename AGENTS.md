# LMUFFB - AI Developer Guide

This document provides context, constraints, and tool instructions for AI assistants (Jules) working on the LMUFFB C++ Force Feedback Driver.

## 🌍 Environment & Constraints

*   **Target OS**: Windows 10/11 (DirectX 11, DirectInput, Win32 API).
*   **Jules Environment**: Ubuntu Linux.
*   **Implication**: You **cannot** build the full `LMUFFB.exe` or run the GUI in the Jules VM.
*   **Strategy**: You **can** build and run the **Unit Tests** (`tests/`) as they are pure C++ logic isolated from Windows headers. Always verify logic changes via these tests.

---

## 🛠️ Developer Tools (Jules Compatible)

### 1. Run Logic Tests (Cross-Platform)
Use this to verify physics math, phase integration, and safety clamps.
*   **Context**: The test suite mocks the telemetry input and verifies `FFBEngine` output.
*   **Commands**:
    ```bash
    mkdir -p build_tests
    cd build_tests
    cmake ../tests
    cmake --build .
    ./run_tests
    ```

### 2. Context Generator
*   **Script**: `python scripts/create_context.py`
*   **Usage**: Run this if you need to consolidate the codebase into a single file for analysis, though usually, you should read specific files directly.

---

## 🏗️ Architecture & Patterns

### 1. The Core Loop (400Hz)
*   **Component**: `FFBEngine` (Header-only: `FFBEngine.h`).
*   **Constraint**: Runs on a high-priority thread. **No memory allocation** (heap) allowed inside `calculate_force`.
*   **Math Rule (Critical)**: Use **Phase Accumulation** for vibrations.
    *   ❌ *Wrong*: `sin(time * frequency)` (Causes clicks when freq changes).
    *   ✅ *Right*: `phase += frequency * dt; output = sin(phase);`
*   **Safety**: All physics inputs involving `mTireLoad` must be clamped (e.g., `std::min(1.5, load_factor)`) to prevent hardware damage during physics glitches.

### 2. The GUI Loop (60Hz)
*   **Component**: `src/GuiLayer.cpp` (ImGui).
*   **Pattern**: **Producer-Consumer**.
    *   *Producer (FFB Thread)*: Pushes `FFBSnapshot` structs into `m_debug_buffer` every tick.
    *   *Consumer (GUI Thread)*: Calls `GetDebugBatch()` to swap the buffer and render *all* ticks since the last frame.
    *   *Constraint*: Never read `FFBEngine` state directly for plots; always use the snapshot batch to avoid aliasing.

### 3. Hardware Interface
*   **Component**: `src/DirectInputFFB.cpp`.
*   **Pattern**: Sends "Constant Force" updates.
*   **Optimization**: Includes a check `if (magnitude == m_last_force) return;` to minimize driver overhead.

---

## 📂 Key Documentation References

*   **Formulas**: `docs/dev_docs/FFB_formulas.md` (The math behind the code).
*   **Telemetry**: `docs/dev_docs/telemetry_data_reference.md` (Available inputs).
*   **Structs**: `rF2Data.h` (Memory layout - **Must match rFactor 2 plugin exactly**).

---

## 📝 Code Generation Guidelines

1.  **Adding New Effects**:
    *   Add a boolean toggle and gain float to `FFBEngine` class.
    *   Add a phase accumulator variable if it oscillates.
    *   Implement logic in `calculate_force`.
    *   Add UI controls in `GuiLayer::DrawTuningWindow`.
    *   Add visualization data to `FFBSnapshot` struct.

2.  **Modifying Config**:
    *   Update `src/Config.h` (declaration).
    *   Update `src/Config.cpp` (Save/Load logic).
    *   **Default to Safe**: New features should default to `false` or `0.0` if they generate force.

3.  **Thread Safety**:
    *   Access to `FFBEngine` settings from the GUI thread must be protected by `std::lock_guard<std::mutex> lock(g_engine_mutex);`.

## 🚫 Common Pitfalls
*   **Do not** use `mElapsedTime` for sine waves (see Math Rule).
*   **Do not** remove the `vJoyInterface.dll` dynamic loading logic (the app must run even if vJoy is missing).
*   **Do not** change the struct packing in `rF2Data.h` (it breaks shared memory reading).