# lmuFFB

A FFB app for LMU, similar to irFFB and Marvin’s iRacing App

Experimental alpha version.

![lmuFFB GUI](docs/screenshots/app.png)


## Installation & Configuration  

### 1. Prerequisites
*   **vJoy Driver**: Install version **2.1.9.1** (by jshafer817) or compatible. Download from [vJoy releases](https://github.com/jshafer817/vJoy/releases).
    *   *Why vJoy?* The game needs a "dummy" device to bind steering to, so it doesn't try to send its own FFB to your real wheel while lmuFFB is controlling it.
    *   *Tip:* **Disable all vJoy FFB Effects** in the "Configure vJoy" tool, except "Constant Force" (though lmuFFB drives your wheel directly, this prevents vJoy from trying to interfere if you use legacy mode).
*   **rF2 Shared Memory Plugin**: Download `rFactor2SharedMemoryMapPlugin64.dll` from [TheIronWolfModding](https://github.com/TheIronWolfModding/rF2SharedMemoryMapPlugin#download) or use the version bundled with [CrewChief](https://thecrewchief.org/).
    *   **Installation**: Copy `rFactor2SharedMemoryMapPlugin64.dll` to your `Le Mans Ultimate/Plugins/` folder.
    *   *Note:* LMU does not have an in-game menu to enable plugins like rFactor 2 did. It is usually enabled by default if the file is present.

### 2. Step-by-Step Setup

**A. Configure vJoy**
1.  Open **Configure vJoy**.
2.  Set up **Device 1** with at least **X Axis** enabled.
3.  Click Apply.

**B. Configure Le Mans Ultimate (LMU)**
1.  Start LMU.
2.  Go to **Settings > Graphics**:
    *   Set **Display Mode** to **Borderless**. (Prevents crashes/minimizing).
3.  Go to **Controls / Bindings**.
    *   *Screenshot:* ![LMU Controls](docs/screenshots/lmu_controls.png) *(To be added)*
4.  **Steering Axis**: 
    *   **Method A (Direct - Recommended):** Bind to your **Physical Wheel**.
        
        **Step-by-Step Setup:**
        1.  **Clean Slate:** Close Joystick Gremlin and vJoy Feeder if running.
        2.  **Game Setup:**
            *   Start Le Mans Ultimate.
            *   Bind **Steering** directly to your **Physical Wheel** (e.g., Simucube, Fanatec, Moza, Logitech).
            *   **Important:** Set In-Game FFB Strength to **0%** (or "Off").
        3.  **App Setup:**
            *   Start LMUFFB.
            *   **CRITICAL STEP:** In the LMUFFB window, look for the **"FFB Device"** dropdown menu.
            *   Click it and select your **Physical Wheel** from the list.
            *   *Note:* You **must** do this manually. If the dropdown says "Select Device...", the app is calculating forces but sending them nowhere.
        4.  **Verify:**
            *   Check the console for errors. If you select your wheel and **do not** see a red error like `[DI] Failed to acquire`, then it is connected!
            *   Drive the car. You should feel the physics-based FFB.
        
        **Troubleshooting - No FFB:**
        *   **Check Console Messages:** While driving, look for `[DI Warning] Device unavailable` repeating in the console.
            *   **YES, I see the warning:** The game has 'Locked' your wheel in Exclusive Mode. You cannot use the Direct Method. You must use Method B (vJoy Bridge).
            *   **NO, console is clean:** The game might be overwriting the signal. Try **Alt-Tabbing** out of the game. If FFB suddenly kicks in when the game is in the background, it confirms the game is interfering. Use Method B.
        *   **Try Adjusting Settings:** If you feel no FFB, try tweaking these values in lmuFFB:
            *   **Master Gain:** Increase from 0.5 to 1.0 or higher.
            *   **SOP (Seat of Pants):** Increase from 0.0 to 0.3 (you should feel lateral forces in corners).
            *   **Understeer Effect:** Ensure it's at 1.0 (default).
        
        *   *Pros:* Simplest setup. No vJoy required.
        *   *Cons:* If LMU "locks" the device (Exclusive Mode), LMUFFB might fail to send forces. If this happens, try Method B.
    *   **Method B (vJoy Bridge - Compatibility):** Bind to **vJoy Device (Axis Y)**.
        *   *Requirement:* You MUST use **Joystick Gremlin** (or similar) to map your Physical Wheel to vJoy Axis Y. The "vJoy Demo Feeder" is NOT sufficient for driving.
        *   *Why Axis Y?* LMUFFB uses Axis X for FFB monitoring (if enabled). Using Y prevents conflicts.
5.  **Force Feedback**:
    *   **Type**: Set to "None" (if available) or reduce **FFB Strength** to **0%** / **Off**.
        *   *Note:* LMU may not have a "None" option; reducing strength to 0 is the workaround.
    *   **Effects**: Set "Force Feedback Effects" to **Off**.
    *   **Smoothing**: Set to **0** (Raw).
    *   **Advanced**: Set "Collision Strength" and "Steering Torque Sensitivity" to **0%**.
    *   **Tweaks**: Disable "Use Constant Steering Force Effect".

**C. Configure lmuFFB (The App)**
1.  Run `LMUFFB.exe`.
2.  **FFB Device**: In the dropdown, select your **Physical Wheel** (e.g., "Simucube 2 Pro", "Fanatec DD1").
    *   *Note:* Do NOT select the vJoy device here. You want lmuFFB to talk to your real wheel.
3.  **Master Gain**: Start low (0.5) and increase.
4.  **SOP Effect**: Start at **0.0**. This effect injects lateral G-force. On High-Torque wheels (DD), this can cause violent oscillation if set too high.
5.  Drive! You should feel force feedback generated by the app.

### Troubleshooting

- **Wheel Jerking / Fighting**: You likely have a "Double FFB" conflict.
    - Ensure in-game Steering is bound to **vJoy**, NOT your real wheel.
    - Ensure in-game FFB is sending to vJoy.
    - If the wheel oscillates on straights, reduce **SOP Effect** to 0.0 and increase smoothing.
- **No Steering (Car won't turn)**:
    - If you used **Method B (vJoy)**, you need **Joystick Gremlin** running to bridge your wheel to vJoy. The "vJoy Demo Feeder" is for testing only.
- **No FFB**: 
    - Ensure the "FFB Device" in lmuFFB is your real wheel.
    - Check if the Shared Memory Plugin is working (Does "Connected to Shared Memory" appear in the console?).
- **"vJoyInterface.dll not found"**: Ensure the DLL is in the same folder as the executable. You can grab it from `C:\Program Files\vJoy\SDK\lib\amd64\` or download from the [vJoy GitHub](https://github.com/shauleiz/vJoy/tree/master/SDK/lib/amd64/vJoyInterface.dll).
    - *Alternative:* You can try moving `LMUFFB.exe` directly into `C:\Program Files\vJoy\x64\` if you have persistent DLL issues.
- **"Could not open file mapping object"**: Start the game and load a track first.

## Known Issues (v0.3.3)
*   **Telemetry Gaps**: Some users report missing telemetry for Dashboard apps (ERS, Temps). lmuFFB has robust fallbacks, but if `mGripFract` is missing (Tire Temps broken), the Understeer effect may be static. See [Telemetry Report](docs/dev_docs/telemetry_availability_report.md).


## Feedback & Support

For feedback, questions, or support:
*   **LMU Forum Thread**: [irFFB for LMU (lmuFFB)](https://community.lemansultimate.com/index.php?threads/irffb-for-lmu-lmuffb.10440/)
*   **GitHub Issues**: [Report bugs or request features](https://github.com/coasting-nc/LMUFFB/issues)


## Documentation

*   [The Physics of Feel - Driver's Guide](docs/the_physics_of__feel_-_driver_guide.md) - **Comprehensive guide** explaining how lmuFFB translates telemetry into tactile sensations, with telemetry visualizations
*   [FFB Effects & Customization Guide](docs/ffb_effects.md)
*   [FFB Customization Guide (Legacy)](docs/ffb_customization.md)
*   [GUI Framework Options](docs/gui_framework_options.md)
*   [DirectInput Implementation Guide](docs/dev_docs/directinput_implementation.md)
*   [Telemetry Data Reference](docs/dev_docs/telemetry_data_reference.md)
*   [vJoy Compatibility Guide](docs/vjoy_compatibility.md)
*   [Comparisons with Other Apps](docs/comparisons.md)



## Architecture

The application reads telemetry from the rFactor 2 engine (Le Mans Ultimate) via Shared Memory and calculates a synthetic Force Feedback signal to send to DirectInput.

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
    *   **Dynamic Oversteer**: Counter-steering force suggestion based on rear axle alignment torque (v0.2.2).
    *   **Progressive Lockup**: Feel the edge of tire braking limit before full lock (v0.2.2).
    *   **Traction Loss**: Feel the rear "float" and spin up under power (v0.2.2).
*   **Easy Installation**: Inno Setup installer script included to manage dependencies (vJoy, Plugins).


## Building (for developers)

### Prerequisites for all methods
*   **Compiler**: MSVC (Visual Studio 2022 Build Tools) or generic C++ compiler.
*   **Build System**: CMake (3.10+).
*   **vJoy**:
    *   **Users**: Install the **vJoy Driver**. We recommend version **2.1.9.1** (by jshafer817) for Windows 10/11 compatibility. See [vJoy Compatibility Guide](docs/vjoy_compatibility.md) for download links and details.
    *   **Developers**: The vJoy Installer typically installs the **SDK** (headers and libraries) to `C:\Program Files\vJoy\SDK`. lmuFFB links against this SDK.
*   **Dear ImGui (Optional)**: Download from [GitHub](https://github.com/ocornut/imgui) and place in `vendor/imgui` to enable the GUI.

### Option A: Visual Studio 2022 (IDE)
1.  Open Visual Studio.
2.  Select "Open a local folder" and choose the repo root.
3.  Visual Studio will auto-detect `CMakeLists.txt`.
4.  Open `CMakeSettings.json` (or Project Settings) to verify the variable `VJOY_SDK_DIR` points to your vJoy SDK location (Default: `C:/Program Files/vJoy/SDK`).
5.  Select **Build > Build All**.

### Option B: Visual Studio Code
1.  Install **VS Code**.
2.  Install extensions: **C/C++** (Microsoft) and **CMake Tools** (Microsoft).
3.  Open the repo folder in VS Code.
4.  When prompted to configure CMake, select your installed compiler kit (e.g., *Visual Studio Community 2022 Release - x86_amd64*).
5.  Open `.vscode/settings.json` (or create it) to set the vJoy path:
    ```json
    {
        "cmake.configureSettings": {
            "VJOY_SDK_DIR": "C:/Path/To/vJoy/SDK"
        }
    }
    ```
6.  Click **Build** in the bottom status bar.

### Option C: Command Line (Windows)
1.  Open the **Developer Command Prompt for VS 2022**.
2.  Navigate to the repository root.
3.  Run the following commands:
    ```cmd
    mkdir build
    cd build
    cmake -G "NMake Makefiles" -DVJOY_SDK_DIR="C:/Path/To/vJoy/SDK" ..
    nmake
    ```
    *Alternatively, use `cmake --build .` instead of `nmake`.*


## Building the Installer

To create the `LMUFFB_Setup.exe`:

1.  **Install Inno Setup**: Download from [jrsoftware.org](https://jrsoftware.org/isdl.php).
2.  **Build the Project**: Ensure you have built the `Release` version of `LMUFFB.exe` using Visual Studio.
3.  **Run Compiler**: Open `installer/lmuffb.iss` in Inno Setup Compiler and click **Compile**.
4.  **Output**: The installer will be generated in `installer/Output/`.

