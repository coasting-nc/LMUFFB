# ImGui Dependency

This project requires the **Dear ImGui** library for the Graphical User Interface.

## Instructions for Developers

1.  Download the **Dear ImGui** source code (version 1.90+ recommended) from [GitHub](https://github.com/ocornut/imgui).
2.  Extract the contents into this directory (`vendor/imgui`).
3.  Ensure the following files are present in the root of `vendor/imgui`:
    *   `imgui.h`
    *   `imgui.cpp`
    *   `imgui_draw.cpp`
    *   `imgui_widgets.cpp`
    *   `imgui_tables.cpp`
    *   `imgui_demo.cpp`
4.  You also need the **backends** (DirectX 11 and Win32). Copy them from `backends/` in the ImGui zip to `vendor/imgui/backends/`:
    *   `imgui_impl_dx11.h` / `.cpp`
    *   `imgui_impl_win32.h` / `.cpp`

## CMake Configuration

The `CMakeLists.txt` is configured to look for these files. If they are missing, the GUI functionality will not be compiled, and the app will run in "Headless" mode (Console only).
