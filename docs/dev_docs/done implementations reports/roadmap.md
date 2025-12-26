# Roadmap & Future Development

To evolve LMUFFB from a prototype to a daily-driver application, the following steps are recommended:

## Completed Features (C++ Port)
*   [x] **Native C++ Port**: Migrated from Python to C++ for performance.
*   [x] **FFB Engine**: Implemented Grip Modulation, SoP, Min Force.
*   [x] **Texture Effects**: Implemented Slide Texture (noise) and Road Texture (suspension delta).
*   [x] **Architecture**: Threaded design (FFB 400Hz / Main 60Hz).
*   [x] **Testing**: Comprehensive C++ Unit Test suite.

## Short Term
*   [x] **GUI Implementation**: Added support for **Dear ImGui**.
    *   Logic for Sliders and Toggles implemented in `src/GuiLayer.cpp`.
    *   Developer instructions in `vendor/imgui/README.txt`.
*   [x] **Installer Support**: Added Inno Setup script (`installer/lmuffb.iss`) handling vJoy checks and Plugin installation.
*   [ ] **Config Persistence**: Save/Load user settings to an `.ini` or `.json` file.

## Medium Term
*   **DirectInput FFB Support**: Documentation guide created (`docs/dev_docs/directinput_implementation.md`). Implementation pending.

## Long Term (Performance)
*   **Wheel-Specific Modes**: Add specific protocols for popular bases (Fanatec, Simucube, Logitech) to display data on wheel screens (RPM LEDs) using the telemetry data.
