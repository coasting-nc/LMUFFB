# GUI Framework Options for LMUFFB (C++ Version)

To transform LMUFFB from a console application to a user-friendly tool, a Graphical User Interface (GUI) is required. This document evaluates the options for implementing the GUI in C++.

## Priorities
1.  **Maintainability**: Code should be easy to read and update.
2.  **Robustness**: Stability is critical for a real-time FFB app.
3.  **Ease of Implementation**: Fast iteration time.

## Option 1: Dear ImGui (Recommended)
**Dear ImGui** is an immediate-mode GUI library designed for real-time applications (game engines, tools).

*   **Pros**:
    *   **Extremely Fast Development**: UI is defined in code logic. Adding a slider is one line of code: `ImGui::SliderFloat("Gain", &gain, 0.0f, 1.0f);`.
    *   **Performance**: Designed for high frame rates (ideal for visualizing telemetry graphs).
    *   **Lightweight**: Tiny footprint, no DLL hell, compiles into the executable.
    *   **Modern Look**: Can be styled easily (Docking, Dark Mode).
*   **Cons**:
    *   **Non-Standard Look**: Does not look like a native Windows app (looks like a game tool).
    *   **CPU Usage**: Redraws every frame (though can be optimized with "lazy" rendering).
*   **Suitability**: **High**. Perfect for a "tuner" app where sliders and graphs are the main focus.

## Option 2: Qt (Widgets or Quick)
**Qt** is a comprehensive cross-platform application framework.

*   **Pros**:
    *   **Professional Look**: Native styling or custom skins.
    *   **Rich Features**: Extensive library (networking, threading, persistence).
    *   **Visual Designer**: Qt Designer allows drag-and-drop UI creation.
*   **Cons**:
    *   **Bloat**: Requires shipping large DLLs (Qt5Core.dll, Qt5Gui.dll, etc.).
    *   **Licensing**: LGPL/Commercial constraints.
    *   **Complexity**: Meta-Object Compiler (MOC), signals/slots paradigm adds build complexity.
*   **Suitability**: **Medium**. Overkill for a simple FFB tuner, but good if the app grows into a complex suite.

## Option 3: wxWidgets
**wxWidgets** is a C++ library that lets developers create applications for Windows, macOS, Linux using native widgets.

*   **Pros**:
    *   **Native Look**: Uses Win32 API under the hood on Windows.
    *   **Stable**: Mature and widely used.
*   **Cons**:
    *   **Old-School API**: Event tables and macros can feel dated compared to modern C++.
    *   **Verbose**: Defining layouts in code is verbose.
*   **Suitability**: **Low**. Harder to make "cool" custom widgets (like force bars) compared to ImGui.

## Option 4: Native Win32 API / MFC
Directly using `CreateWindow` or Microsoft Foundation Classes.

*   **Pros**:
    *   **Zero Dependencies**: No external libraries needed.
    *   **Tiny Size**.
*   **Cons**:
    *   **Painful Development**: Creating layouts, handling resizing, and custom drawing graphs is extremely tedious.
    *   **Unmaintainable**: Boilerplate code explodes quickly.
*   **Suitability**: **Very Low**. Not recommended for rapid development.

## Recommendation: Dear ImGui

**Reasoning**:
*   The primary use case is **tuning parameters** (sliders) and **visualizing data** (telemetry graphs). ImGui excels at exactly this.
*   It introduces minimal build complexity (just add the .cpp files to the project).
*   It is the standard for sim racing tools (e.g., used in many overlays, SimHub plugins, etc.).
*   Integration with the existing `main.cpp` loop is straightforward:
    ```cpp
    // In Main Loop
    ImGui_ImplDX11_NewFrame();
    ImGui::Begin("LMUFFB Settings");
    ImGui::SliderFloat("Gain", &engine.m_gain, 0.0f, 2.0f);
    ImGui::End();
    ImGui::Render();
    ```
