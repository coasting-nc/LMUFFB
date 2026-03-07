
# Implementation Plan - Linux Port (GLFW + OpenGL)

## Context
The goal is to make the LMUFFB application compatible with Linux (specifically Ubuntu) to enable Continuous Integration (CI) and improve code quality via cross-platform compilation. While the core Force Feedback (DirectInput) and Telemetry (Shared Memory) features are Windows-specific, the GUI and Physics Engine can run on Linux. This plan details replacing the DirectX 11 backend with GLFW/OpenGL for Linux builds and mocking the Windows-specific hardware layers.

## Reference Documents
*   **User Request:** Port GUI to Linux using GLFW + OpenGL.
*   **Provided Context:** `CMakeLists.txt`, `tests/CMakeLists.txt`, `src/GuiLayer.cpp`, `src/main.cpp`.

## Codebase Analysis Summary

### Current Architecture
*   **Build System:** CMake-based. `src/CMakeLists.txt` currently hardcodes Windows libraries (`d3d11`, `dinput8`) and sources (`imgui_impl_dx11`).
*   **Entry Point (`src/main.cpp`):** Initializes `DirectInputFFB`, `GameConnector`, and `GuiLayer`. Uses `std::this_thread::sleep_for` for timing.
*   **GUI Layer (`src/GuiLayer.cpp`):** Monolithic file containing:
    *   ImGui Widget Logic (`DrawTuningWindow`, `DrawDebugWindow`).
    *   DirectX 11 Initialization (`CreateDeviceD3D`).
    *   Win32 Message Handling (`WndProc`).
    *   Windows-specific Screenshot logic (`BitBlt`, `PrintWindow`).
*   **Hardware Layer (`src/DirectInputFFB.cpp`):** Uses DirectInput API.
*   **Telemetry Layer (`src/GameConnector.cpp`):** Uses Windows Shared Memory APIs.
*   **Tests:** `tests/CMakeLists.txt` defines `run_combined_tests`. It conditionally includes Windows-specific sources.

### Impacted Functionalities
1.  **Build System:** `CMakeLists.txt` needs conditional logic for Linux dependencies (GLFW/OpenGL).
2.  **GUI Initialization:** `GuiLayer` must be refactored to separate the *Backend* (DX11 vs GLFW) from the *Frontend* (ImGui widgets).
3.  **Hardware Interfaces:** `DirectInputFFB`, `GameConnector`, and `DynamicVJoy` must be mocked on Linux.
4.  **Screenshots:** The `SaveCompositeScreenshot` function in `GuiLayer.cpp` uses GDI/BitBlt and will be disabled/mocked on Linux for this iteration.

## FFB Effect Impact Analysis
*   **No Impact on Logic:** The core physics logic in `FFBEngine.cpp` is platform-agnostic and will remain identical.
*   **User Perspective (Linux):** Users can launch the app, adjust sliders, and view the "Tuning" and "Debug" windows.
    *   **FFB:** No force generation (Mocked).
    *   **Telemetry:** No data connection (Mocked).
    *   **Screenshots:** Disabled on Linux initially.

## Proposed Changes

### 1. Build System (`CMakeLists.txt`)
*   Modify root `CMakeLists.txt`:
    *   Add `if(WIN32)` block for DirectX, DirectInput, and `imgui_impl_dx11`/`win32`.
    *   Add `else()` block for Linux:
        *   `find_package(glfw3 REQUIRED)`
        *   `find_package(OpenGL REQUIRED)`
        *   Add `imgui_impl_glfw.cpp` and `imgui_impl_opengl3.cpp` to `IMGUI_SOURCES`.
        *   Link `glfw` and `OpenGL::GL`.
*   Modify `tests/CMakeLists.txt`:
    *   Ensure `linux_mock` directory is handled if strictly required, or rely on `#ifdef` mocks in source.

### 2. GUI Layer Refactoring
Split `src/GuiLayer.cpp` to separate concerns.

*   **`src/GuiLayer_Common.cpp`:**
    *   **Content:** `DrawTuningWindow`, `DrawDebugWindow`, `SetupGUIStyle`.
    *   **Action:** Extract these functions from `GuiLayer.cpp`.
*   **`src/GuiLayer_Win32.cpp`:**
    *   **Content:** `Init`, `Render`, `Shutdown` (DX11 implementation), `WndProc`, `CreateDeviceD3D`, `SaveCompositeScreenshot` (Windows implementation).
    *   **Action:** Move platform-specific code here.
*   **`src/GuiLayer_Linux.cpp`:**
    *   **Content:** `Init`, `Render`, `Shutdown` (GLFW/OpenGL implementation).
    *   **Logic:**
        *   `Init`: `glfwInit`, `glfwCreateWindow`, `ImGui_ImplGlfw_InitForOpenGL`, `ImGui_ImplOpenGL3_Init`.
        *   `Render`: `glfwPollEvents`, `ImGui_ImplOpenGL3_NewFrame`, `ImGui_ImplGlfw_NewFrame`, `glClear`, `ImGui_ImplOpenGL3_RenderDrawData`, `glfwSwapBuffers`.
        *   `Shutdown`: Cleanup GLFW/ImGui.
        *   `GetWindowHandle`: Returns `GLFWwindow*` cast to `void*`.
        *   `SaveCompositeScreenshot`: Empty stub (no-op) for now.

### 3. Hardware Mocking (Source Level)
Use preprocessor guards to mock Windows dependencies inline, keeping file structure simple.

*   **`src/DirectInputFFB.cpp`:**
    *   Wrap Windows headers and logic in `#ifdef _WIN32`.
    *   **Linux:** `Initialize` returns `true`. `SelectDevice` returns `true`. `UpdateForce` is no-op.
*   **`src/GameConnector.cpp`:**
    *   Wrap Windows headers and logic in `#ifdef _WIN32`.
    *   **Linux:** `TryConnect` returns `false`. `CopyTelemetry` returns `false`.
*   **`src/DynamicVJoy.h`:**
    *   Wrap `LoadLibrary` and Windows types in `#ifdef _WIN32`.
    *   **Linux:** `Load` returns `false`.

### 4. Main Entry Point (`src/main.cpp`)
*   Wrap `timeBeginPeriod(1)` in `#ifdef _WIN32`.
*   Update `DirectInputFFB::Get().Initialize(...)` call. On Linux, the `void*` returned by `GuiLayer::GetWindowHandle()` will be ignored by the mocked `Initialize`.

### 5. Version Increment
*   Increment version in `src/Version.h` (e.g., `0.7.17` -> `0.7.18`).

## Parameter Synchronization Checklist
*   N/A - No new user settings.

## Test Plan (TDD-Ready)

### 1. Compilation Verification (Linux)
*   **Test:** `Build Linux`
*   **Command:** `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --clean-first`
*   **Expectation:** Build succeeds. No errors about missing `d3d11.h` or `dinput.h`.

### 2. Unit Test Verification (Linux)
*   **Test:** `Run Combined Tests`
*   **Command:** `./build/tests/run_combined_tests`
*   **Expectation:** All physics tests (CorePhysics, SlipGrip, etc.) pass. Windows-specific tests (GUI Interaction, Screenshot) should be skipped/excluded by the build configuration.

### 3. GUI Launch Verification (Linux)
*   **Test:** `Launch App`
*   **Action:** Run `./LMUFFB`
*   **Expectation:**
    *   Window opens.
    *   ImGui renders "Tuning" and "Debug" windows.
    *   Status shows "Connecting to LMU..." (Yellow).
    *   Console logs "[DI] Mock Initialized".



## Deliverables
*   [ ] `src/GuiLayer_Common.cpp` (New file).
*   [ ] `src/GuiLayer_Win32.cpp` (New file).
*   [ ] `src/GuiLayer_Linux.cpp` (New file).
*   [ ] Updated `src/GuiLayer.h` (Cleaned up).
*   [ ] Updated `CMakeLists.txt` (Platform logic).
*   [ ] Updated `src/DirectInputFFB.cpp` (Mocks).
*   [ ] Updated `src/GameConnector.cpp` (Mocks).
*   [ ] Updated `src/DynamicVJoy.h` (Mocks).
*   [ ] Updated `src/main.cpp` (Platform guards).
*   [ ] Updated `src/Version.h`.


*   [ ] **Implementation Notes:** Update plan with any library version conflicts or specific GLFW setup hurdles encountered.
