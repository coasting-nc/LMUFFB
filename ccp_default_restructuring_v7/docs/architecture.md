# Architecture

LMUFFB uses a multi-threaded architecture implemented in C++ to ensure minimal latency for the Force Feedback signal while allowing for future GUI expansion.

## High-Level Pipeline

```
[ Simulator (LMU) ]
       |
       v (Shared Memory)
       |
[ Telemetry Reader ]  <-- (Memory Mapped File)
       |
       v (Structs: rF2Telemetry)
       |
[ FFB Engine ]  <-- (FFBThread - 400Hz)
       |
       v (Calculated Force)
       |
[ vJoy Interface ]
       |
       v
[ Physical Wheel ]
```

## Components

### 1. Telemetry Interface (Shared Memory)
The application connects to the **rFactor 2 Shared Memory Map Plugin** (by The Iron Wolf).

*   **Implementation**: Windows API `OpenFileMappingA` and `MapViewOfFile`.
*   **Structs**: Defined in `rF2Data.h`, these mirror the C++ structs used by the plugin, allowing direct access to physics data without parsing overhead.

### 2. Threading Model
The application is split into two primary threads:

*   **FFB Thread (High Priority)**:
    *   Runs at **400Hz** (approx 2.5ms interval) to match the physics update rate of the simulator.
    *   Sole responsibility: Read telemetry -> Calculate Force -> Update vJoy axis.
    *   This isolation ensures that GUI rendering or OS background tasks do not introduce jitter into the FFB signal.
*   **Main/GUI Thread (Low Priority)**:
    *   Runs at **60Hz** (or lower if inactive).
    *   **GuiLayer (`src/GuiLayer.h`)**:
        *   Manages the Win32 Window and DirectX 11 device.
        *   Initializes the Dear ImGui context.
        *   Renders the settings window (`DrawTuningWindow`).
    *   Implements "Lazy Rendering": If `GuiLayer::Render()` reports no activity and the window is not focused, the update rate drops to ~10Hz to save CPU cycles.

### 3. FFB Engine (`FFBEngine.h`)
The core logic is encapsulated in a header-only class to facilitate unit testing.

*   **Inputs**: `SteeringArmForce`, `GripFract` (FL/FR), `LocalAccel` (Lateral G), `VerticalTireDeflection` (Suspension), `SlipAngle`.
*   **Features**:
    *   **Grip Modulation**: Scales force by grip fraction (Understeer feel).
    *   **SoP**: Adds lateral G-force (Oversteer feel).
    *   **Slide Texture**: Injects high-frequency noise when slip angle > threshold.
    *   **Road Texture**: Injects force based on high-frequency suspension velocity changes.
    *   **Min Force**: Boosts small signals to overcome wheel friction.

### 4. Output Driver
*   **vJoy**: The C++ application links against `vJoyInterface.lib` to communicate with the vJoy driver.
*   **Mechanism**: The calculated force (float -1.0 to 1.0) is scaled to the vJoy axis range (1 to 32768) and sent via `SetAxis`.
