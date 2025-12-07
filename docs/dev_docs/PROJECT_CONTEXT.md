# Project Context: LMUFFB (Le Mans Ultimate Force Feedback)

## 1. Goal
The objective of this project is to build a high-performance **Force Feedback (FFB) Application** for the racing simulator *Le Mans Ultimate (LMU)*. 
This tool is inspired by **iRFFB** (for iRacing) and **Marvin's AIRA**. It aims to solve the problem of "numb" or "generic" force feedback by calculating synthetic forces derived directly from physics telemetry (tire grip, suspension load, lateral Gs) rather than relying solely on the game's steering rack output.

## 2. Architecture Overview
The project has evolved from a Python prototype to a **native C++ application** to ensure ultra-low latency (critical for FFB).

### Pipeline
1.  **Telemetry Source**: The app reads telemetry from the **rFactor 2 Engine** (which powers LMU) via a memory-mapped file created by the *rFactor 2 Shared Memory Map Plugin*.
2.  **Processing (The Engine)**: A high-priority thread (400Hz) calculates forces based on:
    *   **Grip Modulation**: Reduces force as front grip is lost (Understeer).
    *   **SoP (Seat of Pants)**: Adds lateral G-forces to simulate chassis yaw (Oversteer).
    *   **Texture Synthesis**: Injects high-frequency vibrations for sliding and road bumps.
3.  **Output**: The app sends the calculated force signal to a **vJoy** (Virtual Joystick) device. The user binds the game controls to this virtual device, allowing the app to control the physical wheel via feedback loops or bridge software.

## 3. Current State
*   **Version**: C++ Port (Main).
*   **GUI**: Implemented using **Dear ImGui** (DX11). Allows real-time tuning of Gain, Smoothing, SoP, etc.
*   **Persistence**: Saves settings to `config.ini`.
*   **Installer**: Inno Setup script provided.
*   **Status**: Alpha. Functional loop, but vJoy dependency makes setup complex for users.

## 4. Key Resources
*   **iRFFB**: [GitHub Repository](https://github.com/nlp80/irFFB) - The primary inspiration.
*   **Marvin's AIRA**: [GitHub Repository](https://github.com/mherbold/MarvinsAIRA) - Inspiration for "Texture" and "Detail" effects.
*   **rF2 Shared Memory Plugin**: [TheIronWolfModding](https://github.com/TheIronWolfModding/rF2SharedMemoryMapPlugin) - The interface used to get data.
*   **vJoy**: [jshafer817 Fork](https://github.com/jshafer817/vJoy) - The required driver for Windows 10/11.

## 5. Technology Stack
*   **Language**: C++ (C++17 standard).
*   **Build System**: CMake (3.10+).
*   **GUI Library**: Dear ImGui (MIT License).
*   **OS**: Windows 10 / 11.
*   **Dependencies**: vJoy SDK (`vJoyInterface.lib`).

## 6. Roadmap / Next Steps
The immediate focus for future sessions is improving the **Output Stage**.
1.  **DirectInput Support**: Move away from vJoy. Implement `IDirectInputDevice8` to send "Constant Force" packets directly to the user's physical wheel. This drastically simplifies setup (no virtual driver needed).
2.  **Telemetry Analysis**: Implement more advanced filtering (High-Pass / Low-Pass) to isolate specific suspension frequencies for better "Road Texture".
3.  **Wheel Profiles**: Save/Load settings per car or per wheel base.
