# LMU Force Feedback App (LMUFFB)

## Introduction

LMUFFB is a custom Force Feedback (FFB) application for **Le Mans Ultimate (LMU)**. It is designed to provide an enhanced or alternative FFB signal compared to the native game output, with a specific focus on communicating **tire grip** and **loss of traction** to the driver.

This project is inspired by similar tools for other simulators, such as **iRFFB** (for iRacing) and **Marvin's AIRA**. It leverages the telemetry data exposed by the game engine (rFactor 2 engine) to calculate synthetic forces in real-time.

## Architecture

The application consists of three main components:

1.  **Telemetry Reader (`LMUFFB/shared_memory.py`)**:
    -   Accesses the simulator's telemetry via Shared Memory.
    -   LMU is built on the rFactor 2 engine, so it uses the same shared memory mechanism.
    -   We use the standard **rFactor 2 Shared Memory Map Plugin** interface (by The Iron Wolf). This is preferred over DAMPlugin for real-time applications because it is designed for low-latency access rather than logging.
    -   Implemented using Python's `ctypes` and `mmap` to map the C++ structures exposed by the plugin.

2.  **FFB Engine (`LMUFFB/ffb_engine.py`)**:
    -   The core logic center. It takes telemetry data (slip angle, tire load, grip fraction, lateral G) and calculates a force value.
    -   The current algorithm modulates the game's native `SteeringArmForce` based on the `GripFract` (grip fraction) telemetry. When grip is lost, the force is reduced ("lightened") to signal understeer.
    -   It also adds a "Seat of Pants" (SoP) effect derived from lateral acceleration (`LocalAccel`), which simulates the G-forces acting on the driver's body (often felt as a yaw force on the wheel in sim racing).

3.  **FFB Output (`LMUFFB/ffb_output.py`)**:
    -   Sends the calculated force to the physical steering wheel.
    -   *Current State*: This module is a Mock implementation. In a production Windows environment, this would interface with **vJoy** (Virtual Joystick) or DirectInput to send the force signal.

## Comparison with Other FFB Apps

### vs iRFFB & Marvin's AIRA (iRacing)

*   **Telemetry Source**: iRFFB reads from the iRacing API, which provides telemetry at 360Hz (via memory mapping similar to what we do). LMU's shared memory typically updates at physics rate (400Hz) or a divisor of it.
*   **FFB Philosophy**:
    *   **iRFFB**: Typically offers "Reconstruction" (smoothing 60Hz) or "360Hz" mode. It often calculates aligning torque using a rack force model or blends it with SoP effects.
    *   **Marvin's AIRA**: Focuses on "Detail Augmentation". It splits effects (Road, Curb, Slip) and allows boosting specific frequencies to enhance tactile feedback. It uses advanced filtering to find hidden details in the telemetry.
    *   **LMUFFB** (this app): Takes a macro-level approach similar to iRFFB's "360Hz" mode but relies on the rFactor 2 engine's `SteeringArmForce`. Its unique feature is modulating this force using the explicit `GripFract` (Grip Fraction) telemetry to signal understeer, and adding "Seat of Pants" (Lateral G) forces.
*   **Understeer/Oversteer**:
    *   iRFFB often estimates understeer via Slip Angle vs. Torque curves.
    *   LMUFFB uses the direct `GripFract` variable provided by the rFactor 2 engine.

**See [docs/comparisons.md](docs/comparisons.md) for a detailed breakdown, including missing features.**

### vs TinyPedal (rFactor 2 / LMU)

*   **Similarities**: Both apps rely on the same **rFactor 2 Shared Memory Map Plugin** to access data. The method of reading `mmap` files in Python is identical.
*   **Differences**: TinyPedal is primarily an **Overlay** application (displaying data), whereas LMUFFB is a **Control** application (calculating and sending forces). TinyPedal does not write back to the wheel.

## Findings & Challenges

1.  **Shared Memory Access**: Accessing LMU telemetry is identical to rFactor 2. The key is ensuring the `rFactor2SharedMemoryMapPlugin64.dll` is correctly installed in the `Plugins` folder and enabled in `CustomPluginVariables.json`.
2.  **DAMPlugin vs Shared Memory Map**: The initial request mentioned DAMPlugin. Research confirmed that while DAMPlugin is excellent for Motec logging, the **Shared Memory Map Plugin** (by The Iron Wolf) is the standard for real-time apps like CrewChief, TinyPedal, and SimHub. We chose the latter for better compatibility and performance.
3.  **Mock Environment**: Developing this in a Linux/Container environment required mocking the Windows-specific `mmap` behavior and lack of physical joystick drivers. The architecture isolates `ffb_output.py` to make swapping this for a real vJoy implementation straightforward.

## User Guide

### Requirements

1.  **Le Mans Ultimate** (or rFactor 2).
2.  **rFactor 2 Shared Memory Map Plugin**:
    -   Download from [The Iron Wolf's GitHub](https://github.com/TheIronWolfModding/rF2SharedMemoryMapPlugin).
    -   Place `rFactor2SharedMemoryMapPlugin64.dll` in `Le Mans Ultimate\Plugins\`.
    -   Enable it in `Le Mans Ultimate\UserData\player\CustomPluginVariables.JSON`:
        ```json
        "rFactor2SharedMemoryMapPlugin64.dll":{
          " Enabled":1
        }
        ```
3.  **vJoy** (for final FFB output):
    -   Install vJoy drivers.
    -   Configure a vJoy device.
4.  **Python 3.10+**.

### Installation & Running

1.  Clone this repository.
2.  Install dependencies (if any added later, currently standard lib).
3.  Run the app:
    ```bash
    python LMUFFB/main.py
    ```
4.  The app will wait for the simulator to start (it looks for the shared memory buffer).

### Troubleshooting

If the app is not working as expected, check the following:

*   **"Shared memory file ... not found. Waiting for game..."**:
    *   Ensure the **rFactor 2 Shared Memory Map Plugin** is installed in the correct `Plugins` folder.
    *   Verify `CustomPluginVariables.JSON` has the plugin enabled (`" Enabled": 1`).
    *   Start the game and drive a car onto the track. The memory buffer is only created when the physics engine initializes.
*   **vJoy Errors / Mock Mode**:
    *   If you see "Falling back to Mock", ensure **vJoy** drivers are installed and a device is configured in `vJoyConf`.
    *   Ensure `pyvjoy` is installed (`pip install pyvjoy`).
*   **Permissions**:
    *   In rare cases, you may need to run the script as **Administrator** to access the shared memory created by the game.

## Building for Distribution

To distribute LMUFFB to end-users without requiring them to install Python, you can compile it into a standalone executable.

1.  **Install PyInstaller**:
    ```bash
    pip install pyinstaller
    ```

2.  **Build the Executable**:
    Run the following command from the repository root:
    ```bash
    pyinstaller --onefile --name LMUFFB LMUFFB/main.py
    ```

3.  **Locate the Output**:
    The compiled `LMUFFB.exe` will be located in the `dist/` folder. You can distribute this single file.

*Note: Ensure you install `pyvjoy` in your Python environment before building, so PyInstaller can bundle it.*

## Developer Guide

### Project Structure

```
LMUFFB/
├── ffb_engine.py       # FFB Calculation Logic
├── ffb_output.py       # Output Interface (Mock/vJoy)
├── main.py             # Main Loop
├── shared_memory.py    # ctypes structures & mmap reader
└── tests.py            # Unit tests
```

### Running Tests

Unit tests mock the shared memory file to verify logic without the game running.

```bash
export PYTHONPATH=$PYTHONPATH:.
python3 LMUFFB/tests.py
```

## Documentation

For more detailed information, please refer to the `docs/` folder:
*   [Introduction](docs/introduction.md)
*   [Architecture](docs/architecture.md)
*   [Comparisons & Missing Features](docs/comparisons.md)
*   [Performance Analysis (Python vs C++)](docs/performance_analysis.md)
*   [Roadmap & Future Development](docs/roadmap.md)
