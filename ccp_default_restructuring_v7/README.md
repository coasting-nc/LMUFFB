# LMU Force Feedback App (C++ Version)

This is the main C++ implementation of the LMU Force Feedback App, designed for high performance and low latency.

## Features

*   **High Performance Core**: Native C++ Multi-threaded architecture.
    *   **FFB Loop**: Runs at ~400Hz to match game physics.
    *   **GUI Loop**: Runs at ~60Hz with lazy rendering to save CPU.
*   **Real-time Tuning GUI**:
    *   Built with **Dear ImGui** for responsive adjustment of parameters.
    *   Sliders for Master Gain, Understeer (Grip Loss), SoP (Seat of Pants), and Min Force.
    *   Toggles for Texture effects (Slide Rumble, Road Details).
*   **Custom Effects**:
    *   **Grip Modulation**: Feel the wheel lighten as front tires lose grip.
    *   **Texture Synthesis**: Synthetic road noise and slide vibration injection.
*   **Easy Installation**: Inno Setup installer script included to manage dependencies (vJoy, Plugins).

## Architecture

The application reads telemetry from the rFactor 2 engine (Le Mans Ultimate) via Shared Memory and calculates a synthetic Force Feedback signal to send to a vJoy device.

### Prerequisites

*   **vJoy**: Must be installed and configured.
*   **rFactor 2 Shared Memory Map Plugin**: Must be installed in `Le Mans Ultimate/Plugins` and enabled.

## Building

### Windows (Visual Studio / CMake)

1.  **Clone the Repo**: Ensure you have the source code.
2.  **ImGui Setup (Optional but Recommended)**:
    *   To enable the GUI, download [Dear ImGui](https://github.com/ocornut/imgui) and place the source files in `vendor/imgui`.
    *   See `vendor/imgui/README.txt` for exact file requirements.
    *   *If skipped, the app builds in Headless (Console-only) mode.*
3.  **Build**:
    *   Open this folder in Visual Studio.
    *   Configure CMake settings to point `VJOY_SDK_DIR` to your vJoy SDK installation.
    *   Build the `LMUFFB.exe` target.

## Documentation

*   [FFB Customization Guide](docs/ffb_customization.md)
*   [GUI Framework Options](docs/gui_framework_options.md)
*   [DirectInput Implementation Guide](docs/directinput_implementation.md)
*   [Licensing & Redistribution](docs/licensing.md)
*   [Python Prototype & Porting Guides](docs/python_version/)
