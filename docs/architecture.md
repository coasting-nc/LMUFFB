#Architecture (v0.4.1+)

LMUFFB uses a multi-threaded architecture implemented in C++ to ensure minimal latency for the Force Feedback signal while allowing for responsive GUI interaction.

## Design Choices

The app follows best practices for real-time signal generation. Recent updates (v0.3.0+) transitioned from "canned" effects to physics-based haptics using Phase Integration for smooth, dynamic oscillators.

## High-Level Pipeline

```
[ Simulator (LMU 1.2) ]
       |
       v (Native Shared Memory)
       |
[ Telemetry Reader ]  <-- (Memory Mapped File: $LMU_Data$)
       |
       v (Structs: TelemInfoV01, TelemWheelV01)
       |
[ FFB Engine ]  <-- (FFBThread - 400Hz)
       | (Sanity Checks, Hysteresis, Normalization)
       v (Calculated Torque: -1.0 to 1.0)
       |
[ DirectInput FFB ]  <-- (or vJoy for compatibility)
       |
       v
[ Physical Wheel ]
```

## Components

### 1. Telemetry Interface (LMU 1.2 Native Shared Memory)

**No Plugin Required**: LMU 1.2+ includes built-in shared memory support.

*   **Implementation**: Windows API `OpenFileMappingA` and `MapViewOfFile`.
*   **Memory Map Name**: `$LMU_Data$` (player-specific telemetry)
*   **Structs**: Defined in `src/lmu_sm_interface/InternalsPlugin.hpp`, provided by Studio 397.
*   **Locking**: Uses `SharedMemoryLock` class to prevent data corruption during reads.
*   **Player Indexing**: Scans the 104-slot vehicle array to find the player's car via `VehicleScoringInfoV01::mIsPlayer`.

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

*   **Sanity Layer (v0.3.19+)**: Incoming telemetry is validated against physical rules with hysteresis filtering (v0.4.1+). Invalid states trigger fallbacks to prevent effects from cutting out.
*   **Inputs (LMU 1.2 API)**:
    *   `mSteeringShaftTorque` (Nm) - Primary FFB source
    *   `mTireLoad` (N) - Vertical tire load
    *   `mGripFract` (0-1) - Front/Rear grip usage
    *   `mLocalAccel.x` (m/s²) - Lateral G-force
    *   `mLateralPatchVel` (m/s) - Contact patch lateral velocity
    *   `mLongitudinalPatchVel` (m/s) - For slip ratio calculation
    *   `mVerticalTireDeflection` (m) - Suspension travel
*   **Features**:
    *   **Grip Modulation**: Scales torque by grip fraction (Understeer feel).
    *   **SoP (Seat of Pants)**: Adds lateral G-force (Oversteer feel).
    *   **Dynamic Textures**: Lockup, Spin, Slide, Road, Bottoming - all frequency-modulated.
    *   **Hysteresis (v0.4.1)**: 20-frame stability filter for missing telemetry data.
    *   **Diagnostic Logging (v0.4.1)**: Non-blocking 1Hz stats output.
    *   **Min Force**: Boosts small signals to overcome wheel friction.
*   **New Telemetry Mappings (v0.7.0)**:
    *   **Axle 3rd Deflection**: Front3rdDeflection/Rear3rdDeflection (0-1) added to per-wheel suspension deflection (0-2 total range).
    *   **Suspension Force Enhancements**: RideHeight, Drag, Downforce contributions to mSuspForce calculations.
    *   **Tire Condition Effects**: Wear reduces mGripFract, Flat tires reduce mTireLoad.
    *   **GUI Surface**: Axle contributions displayed in telemetry inspector with separate visualization.

### 4. Output Driver

**Primary: DirectInput (v0.2.0+)**
*   **Implementation**: `DirectInputFFB` class using Windows DirectInput 8 API.
*   **Device Selection**: User selects physical wheel from GUI dropdown.
*   **Effect Type**: Constant Force effect with continuous parameter updates.
*   **Unbinding (v0.4.1)**: GUI button to release device without closing app.
*   **Saturation Warnings (v0.4.1)**: Rate-limited console alerts when output exceeds 99%.

**Fallback: vJoy (Legacy/Compatibility Mode)**
*   **Use Case**: When DirectInput device is locked by the game (Exclusive Mode conflict).
*   **Mechanism**: Links against `vJoyInterface.lib` to communicate with vJoy driver.
*   **Scaling**: Calculated torque (-1.0 to 1.0) scaled to vJoy axis range (1 to 32768).
