================================================================================
                    LMUFFB - Le Mans Ultimate Force Feedback
                               C++ Version 1.0.0
================================================================================

Welcome to LMUFFB! This application provides high-performance force feedback
for Le Mans Ultimate (rFactor 2) using a virtual joystick device.

QUICK START:
1. Make sure vJoy is installed (see INSTALL.txt)
2. Start Le Mans Ultimate
3. Run LMUFFB.exe
4. Adjust the force feedback settings in the GUI window

FEATURES:
- High-performance FFB loop (~400Hz) for realistic force feedback
- Real-time GUI tuning for all parameters
- Grip modulation (feel tire grip loss)
- Texture effects (road details and slide rumble)
- Customizable force output scaling

SYSTEM REQUIREMENTS:
- Windows 10 or Windows 11 (64-bit or 32-bit)
- vJoy driver 2.1.9.1 or compatible (see INSTALL.txt)
- Le Mans Ultimate game installed
- rF2 Shared Memory Map Plugin installed

DOCUMENTATION:
- README.md - Full technical documentation
- INSTALL.txt - Step-by-step installation guide
- docs/ folder - Detailed guides on compatibility and customization

TROUBLESHOOTING:
If you experience issues:
1. Ensure vJoy is properly installed (vJoy Control Panel shows your device)
2. Verify Le Mans Ultimate is running before launching LMUFFB
3. Check that the rF2 Shared Memory Plugin is enabled in the game
4. Make sure vJoyInterface.dll is in the same folder as LMUFFB.exe

For more help, see the full README.md or visit the GitHub repository.

CONFIGURATION:

*   **In-Game (LMU)**:
    *   **Controls**: Bind **Steering** to the **vJoy Device Axis** (usually X-Axis).
    *   **FFB**: You can technically turn off in-game FFB since it's going to the vJoy device (which has no motors), but leaving it on is fine as it drives the calculation LMUFFB reads. Ideally, set "FFB Smoothing" in-game to 0 (raw) so LMUFFB gets the cleanest data.
*   **vJoy**:
    *   Configure one device with at least **one axis (X)**.
    *   Ensure "Enable Force Feedback" is checked in vJoy configuration if available (though LMUFFB talks to your physical wheel via the bridge or directly).
*   **LMUFFB**:
    *   Select your **Physical Wheel** if a device selector is implemented (currently it drives vJoy, which requires a "feeder" or mapping software like vJoyFeeder or SimHub to bridge back to your physical wheel if not using DirectInput mode). *Note: The current C++ version outputs to vJoy. You need a bridge (like vJoy's own feeder or binding vJoy as the game input) to close the loop.*

================================================================================
Version: 1.0.0
License: See LICENSE file
Repository: https://github.com/coasting-nc/LMUFFB
================================================================================
