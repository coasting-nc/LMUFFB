# Introduction to LMUFFB

**LMUFFB** (Le Mans Ultimate Force Feedback) is a specialized high-performance application designed to enhance the driving experience in the *Le Mans Ultimate* simulator. Its primary goal is to provide **Force Feedback (FFB)** signals that communicate tire physics—specifically tire grip and loss of traction—more effectively than the game's native output.

This project is a response to the community's need for tools similar to **iRFFB** and **Marvin's AIRA**, which have transformed the FFB landscape for simulators like iRacing.

## Scope & Goal

The core scope of LMUFFB is:
1.  **Telemetry Acquisition**: Reading high-fidelity vehicle physics data from the simulator in real-time.
2.  **FFB Processing**: Applying algorithms to this data to calculate a "synthetic" steering force that emphasizes the "Seat of Pants" (SoP) feel and pneumatic trail effects.
3.  **Signal Output**: Sending this calculated force to the player's steering wheel via a virtual joystick driver (vJoy).

By doing so, LMUFFB allows players to feel when the car is understeering or oversteering through the steering wheel's resistance, a critical feedback loop for driving at the limit.

**Current Status**: The project has migrated from a Python prototype to a **native C++ application** to ensure sub-millisecond latency and consistent 400Hz update rates.
