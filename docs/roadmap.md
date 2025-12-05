# Roadmap & Future Development

To evolve LMUFFB from a prototype to a daily-driver application, the following steps are recommended:

## Short Term
*   **GUI Implementation**: Add a Graphical User Interface (using Tkinter, PyQt, or DearPyGui) to allow users to adjust:
    *   Overall Gain.
    *   Grip Modulation Factor (how much grip loss lightens the wheel).
    *   SoP Factor (how much lateral G is added).
    *   Smoothing/Filtering.
*   **Config Persistence**: Save/Load user settings to a JSON/INI file.

## Medium Term
*   **DirectInput FFB Support**: Move beyond vJoy "Axis" mapping. Implement proper DirectInput "Constant Force" packet sending. This allows the app to coexist better with the game (game handles buttons/shifters, app handles FFB).
*   **Advanced Filters**: Implement High-Pass filters to isolate road texture from the telemetry (if available via suspension velocity/accel) and boost it (Marvin's AIRA style).

## Long Term (Performance)
*   **C++ Rewrite**: Port the core `main.py` loop and `FFBEngine` to C++.
    *   Use raw Windows APIs for Shared Memory and DirectInput.
    *   Minimize latency to sub-millisecond levels.
*   **Wheel-Specific Modes**: Add specific protocols for popular bases (Fanatec, Simucube, Logitech) to display data on wheel screens (RPM LEDs) using the telemetry data.
