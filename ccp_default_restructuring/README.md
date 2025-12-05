# LMU Force Feedback App (C++ Version)

This is the main C++ implementation of the LMU Force Feedback App, designed for high performance and low latency.

## Architecture

The application reads telemetry from the rFactor 2 engine (Le Mans Ultimate) via Shared Memory and calculates a synthetic Force Feedback signal to send to a vJoy device.

### Prerequisites

*   **vJoy**: Must be installed and configured.
*   **rFactor 2 Shared Memory Map Plugin**: Must be installed in `Le Mans Ultimate/Plugins` and enabled.

## Building

### Windows (Visual Studio / CMake)

1.  Open this folder in Visual Studio.
2.  Configure CMake settings to point `VJOY_SDK_DIR` to your vJoy SDK installation.
3.  Build.

## Documentation

*   [FFB Customization Guide](docs/ffb_customization.md)
*   [GUI Framework Options](docs/gui_framework_options.md)
*   [Python Prototype & Porting Guides](docs/python_version/)
