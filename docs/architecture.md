# Architecture

LMUFFB follows a modular pipeline architecture designed for real-time processing.

## High-Level Pipeline

```
[ Simulator (LMU) ]
       |
       v (Shared Memory)
       |
[ Telemetry Reader ]  <-- (mmap / ctypes)
       |
       v (Python Data Objects)
       |
[ FFB Engine ]  <-- (Physics Logic / Math)
       |
       v (Normalized Force)
       |
[ Output Driver ]  <-- (vJoy / DirectInput)
       |
       v
[ Physical Wheel ]
```

## Components

### 1. Telemetry Interface (Shared Memory)
The rFactor 2 engine (which powers LMU) exposes telemetry via a Shared Memory Map. We utilize the **rFactor 2 Shared Memory Map Plugin** (by The Iron Wolf) because it is the standard for low-latency access in the rFactor 2 ecosystem.

*   **Mechanism**: The plugin writes a C-struct to a named memory-mapped file (`$rFactor2SMMP_Telemetry$`).
*   **Implementation**: Python's `mmap` module maps this memory into the application's address space. `ctypes` is used to define the C-struct layout (`rF2Telemetry`, `rF2Wheel`, etc.) matching the plugin's header file. This allows for zero-copy-like access to the data variables.

### 2. FFB Engine
The engine (`FFBEngine`) is the "brain" of the application. It runs at the speed of the main loop (typically synchronized with telemetry updates or fixed rate).

*   **Inputs**:
    *   `mSteeringArmForce`: The physics-based aligning torque calculated by the game.
    *   `mGripFract`: The fraction of available grip currently used by the tires.
    *   `mLocalAccel`: Local lateral acceleration (G-force).
*   **Logic**:
    *   **Grip Modulation**: The base game force is multiplied by the grip fraction. As the tire slides (grip fraction drops), the force on the wheel is reduced. This provides a distinct "lightening" of the wheel during understeer.
    *   **SoP Effect**: A lateral G-force component is added to the signal. This mimics the forces felt by the driver's body (Seat of Pants), which are often communicated through the wheel in sim racing to compensate for the lack of physical motion.

### 3. Output Driver
Since Python cannot natively act as a hardware driver, we use **vJoy** (Virtual Joystick).

*   **Implementation**: `pyvjoy` wraps the vJoyInterface.dll.
*   **Operation**: The calculated force (normalized -1.0 to 1.0) is mapped to a vJoy axis (e.g., X-Axis).
*   **Hardware Bridge**: Users typically need a piece of "bridge" software (like vJoyFeeder or SimHub, or the wheel driver itself if it supports it) to interpret this axis position as a Force Feedback signal, or simply map the vJoy axis as the game's steering input (though LMUFFB is FFB-only, so typically this drives a "Constant Force" effect). *Note: A fully robust implementation would send DirectInput FFB packets, which requires more complex C++ interoperability.*
