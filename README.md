# lmuFFB

A FFB app for LMU, similar to irFFB and Marvin’s iRacing App

Experimental alpha version.



![lmuFFB GUI](docs/screenshots/main_app.png)
![lmuFFB GUI2](docs/screenshots/ffb_analysis.png)


## ⚠️ CRITICAL SAFETY WARNING ⚠️

**BEFORE USING THIS APPLICATION, YOU MUST CONFIGURE YOUR STEERING WHEEL DEVICE DRIVER:**

This is an **experimental early alpha version** of a force feedback application. The FFB formulas are still being refined and **may produce strong force spikes and oscillations** that could be dangerous or damage your equipment.

**Required Safety Steps (DO THIS FIRST):**

1. **BEFORE running LMU and lmuFFB**, open your wheelbase/steering wheel device driver configurator (e.g., Simucube TrueDrive, Fanatec Control Panel, Moza Pit House, etc.)
2. **Reduce the Maximum FFB Strength/Torque to a LOW value:**
   - **For Direct Drive Wheelbases**: Set to **10% or lower** of maximum torque
   - **For Belt/Gear-Driven Wheels**: Set to **20-30%** of maximum strength
   - **This is your primary safety mechanism** - do not skip this step!
3. **Test gradually**: Start with even lower values and increase slowly while monitoring for unexpected behavior
4. **Stay alert**: Be prepared to immediately disable FFB if you experience violent oscillations or unexpected forces

**Why this is critical:**
- The FFB algorithms are under active development and may generate unexpected force spikes
- Unrefined formulas can cause dangerous oscillations, especially on high-torque direct drive systems
- Your safety and equipment protection depend on having a hardware-level force limiter in place

**Do not skip this step.** No software-level safety can replace proper hardware configuration.

## 📥 Download

**[Download the latest release from GitHub](https://github.com/coasting-nc/LMUFFB/releases)**

## ⚠️ Important Notes

### LMU 1.2+ Support (v0.4.0+)

**Full Telemetry Access**: With **Le Mans Ultimate 1.2** (released December 9th, 2024), the game now includes native shared memory support with complete tire telemetry data. **lmuFFB v0.4.0+** fully supports LMU 1.2's new interface, providing access to:
- **Tire Load** - Essential for load-sensitive effects
- **Grip Fraction** - Enables dynamic understeer/oversteer detection
- **Patch Velocities** - Allows physics-based texture generation
- **Steering Shaft Torque** - Direct torque measurement for accurate FFB

**No Plugin Required**: Unlike previous versions, LMU 1.2 has built-in shared memory - no external plugins needed!

### 🧪 Experimental Version - Testing Needed!

This is an **experimental release** with the new LMU 1.2 interface. The FFB formulas may require refinement based on real-world testing.

**Please help us improve lmuFFB:**
1. **Test with caution** - Start with low wheel strength settings (see Safety Warning above)
2. **Experiment with settings** - Try different effect combinations and gains
3. **Share your results** - Post screenshots of the "Troubleshooting Graphs" window and the main app window to the [LMU Forum Thread](https://community.lemansultimate.com/index.php?threads/irffb-for-lmu-lmuffb.10440/)
4. **Report issues** - Let us know what works and what doesn't!

Your testing and feedback is greatly appreciated! 🙏


## Installation & Configuration (LMU 1.2+)

### 1. Prerequisites

*   **vJoy Driver**: Install version **2.1.9.1** (by jshafer817) or compatible. Download from [vJoy releases](https://github.com/jshafer817/vJoy/releases).
    *   *Why vJoy?* The game needs a "dummy" device to bind steering to, so it doesn't try to send its own FFB to your real wheel while lmuFFB is controlling it.
    *   *Tip:* **Disable all vJoy FFB Effects** in the "Configure vJoy" tool, except "Constant Force" (though lmuFFB drives your wheel directly, this prevents vJoy from trying to interfere if you use legacy mode).

### 2. Step-by-Step Setup

**⚠️ STEP 0: Reduce Wheel Strength FIRST (CRITICAL)**
1.  **BEFORE doing anything else**, open your wheel device driver (Simucube TrueDrive, Fanatec Control Panel, Moza Pit House, etc.)
2.  **Reduce Maximum FFB Strength/Torque**:
    *   Direct Drive Wheels: Set to **10% or lower**
    *   Belt/Gear Wheels: Set to **20-30%**
3.  **Save the settings** and keep the driver software open for adjustments

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
        
        Method A vs Method B:
        *   *Pros:* Simplest setup. No vJoy required.
        *   *Cons:* If LMU "locks" the device (Exclusive Mode), LMUFFB might fail to send forces. If this happens, try Method B.
    *   **Method B (vJoy Bridge - Compatibility):** Bind to **vJoy Device (Axis Y)**.
        *   *Requirement:* You MUST use **Joystick Gremlin** (or similar) to map your Physical Wheel to vJoy Axis Y. The "vJoy Demo Feeder" is NOT sufficient for driving.
        *   *Why Axis Y?* LMUFFB uses Axis X for FFB monitoring (if enabled). Using Y prevents conflicts.
5.  **In-Game Force Feedback settings in LMU**:
    *   **FFB Strength**: reduce to **0%** (Off).
    *   **Effects**: Set "Force Feedback Effects" to **Off**.
    *   **Smoothing**: Set to **0** (Raw).
    *   **Advanced**: Set "Collision Strength" and "Steering Torque Sensitivity" to **0%**.
    *   **Tweaks**: Disable "Use Constant Steering Force Effect".

**C. Configure lmuFFB**
1.  Run `LMUFFB.exe`.
2.  **FFB Device**: In the dropdown, select your **Physical Wheel** (e.g., "Simucube 2 Pro", "Fanatec DD1").
    *   *Note:* Do NOT select the vJoy device here. You want lmuFFB to talk to your real wheel.
3.  **Master Gain**: Start low (0.5) and increase.
4.  **SOP Effect**: Start at **0.0**. This effect injects lateral G-force. On High-Torque wheels (DD), this can cause violent oscillation if set too high.
5.  Drive! You should feel force feedback generated by the app.

### Troubleshooting

- **Wheel Jerking / Fighting**: You likely have a "Double FFB" conflict.
    - Ensure in-game Steering is bound to **vJoy**, NOT your real wheel.
-   **Wheel Jerking / Fighting**: You likely have a "Double FFB" conflict.
    -   Ensure in-game Steering is bound to **vJoy**, NOT your real wheel.
    -   Ensure in-game FFB is sending to vJoy.
    -   If the wheel oscillates on straights, reduce **SOP Effect** to 0.0 and increase smoothing.
-   **No Steering (Car won't turn)**:
    -   If you used **Method B (vJoy)**, you need **Joystick Gremlin** running to bridge your wheel to vJoy. The "vJoy Demo Feeder" is for testing only.
-   **Inverted FFB (Force pushes away from center)**:
    -   If the FFB feels "backwards" or "inverted" while driving (wheel pushes away from center instead of pulling toward it), check the **"Invert FFB"** checkbox in the lmuFFB GUI.
    -   This reverses the force direction to match your wheel's expected behavior.
-   **FFB Too Strong / Dangerous Forces**:
    -   **IMMEDIATELY** reduce the maximum FFB strength in your wheel device driver (Simucube TrueDrive, Fanatec Control Panel, Moza Pit House, etc.).
    -   Set to **10% or lower** for direct drive wheels, **20-30%** for belt/gear wheels.
    -   Do this **before** running LMU and lmuFFB again.
    -   Then adjust the "Gain" slider in lmuFFB to fine-tune.
-   **No FFB**: 
    -   Ensure the "FFB Device" in lmuFFB is your real wheel.
    -   Check if the Shared Memory is working (Does "Connected to Shared Memory" appear in the console?).
    -   Verify you're running LMU 1.2 or later (earlier versions don't have native shared memory).
-   **"vJoyInterface.dll not found"**: Ensure the DLL is in the same folder as the executable. You can grab it from `C:\\Program Files\\vJoy\\SDK\\lib\\amd64\\` or download from the [vJoy GitHub](https://github.com/shauleiz/vJoy/tree/master/SDK/lib/amd64/vJoyInterface.dll).
    -   *Alternative:* You can try moving `LMUFFB.exe` directly into `C:\\Program Files\\vJoy\\x64\\` if you have persistent DLL issues.
-   **"Could not open file mapping object"**: Start the game and load a track first. The shared memory only activates when driving.

## Known Issues (v0.4.2+)

### LMU 1.2 Missing Telemetry Data (CRITICAL)

**⚠️ Expected Warnings on Startup:**

When you start lmuFFB with LMU 1.2, you will see console warnings like:
- `[WARNING] Missing Tire Load data detected`
- `[WARNING] Missing Grip Fraction data detected`

**This is expected and NOT a bug in lmuFFB.** This is a **bug in LMU 1.2** - the game is currently returning **zero (0) for all tire load and grip fraction values**, even though the shared memory interface includes these fields.

**Impact:**
- lmuFFB has **automatic fallback logic** that detects this and uses estimated values instead
- FFB will still work, but some effects (like load-sensitive textures and grip-based understeer) will use approximations instead of real data
- You can safely ignore these warnings - they confirm the fallback system is working

**What we need from the community:**
- **Please help us report this to the LMU developers!** 
- We need to file a bug report / feature request asking them to populate these telemetry fields with actual values
- Forum thread: [LMU Forum - lmuFFB](https://community.lemansultimate.com/index.php?threads/irffb-for-lmu-lmuffb.10440/)
- Once LMU fixes this, lmuFFB will automatically use the real data (no code changes needed)

### Other Known Issues
*   **Telemetry Gaps**: Some users report missing telemetry for Dashboard apps (ERS, Temps). lmuFFB has robust fallbacks (Sanity Checks) that prevent dead FFB effects even if the game fails to report data. See [Telemetry Report](docs/dev_docs/telemetry_availability_report.md).


## Feedback & Support

For feedback, questions, or support:
*   **LMU Forum Thread**: [irFFB for LMU (lmuFFB)](https://community.lemansultimate.com/index.php?threads/irffb-for-lmu-lmuffb.10440/)
*   **GitHub Issues**: [Report bugs or request features](https://github.com/coasting-nc/LMUFFB/issues)


## Documentation

*   [The Physics of Feel - Driver's Guide](docs/the_physics_of__feel_-_driver_guide.md) - **Comprehensive guide** explaining how lmuFFB translates telemetry into tactile sensations, with telemetry visualizations
*   [FFB Effects & Customization Guide](docs/ffb_effects.md)
*   [FFB Customization Guide (Legacy)](docs/ffb_customization.md)
*   [Telemetry Data Reference](docs/dev_docs/telemetry_data_reference.md)
*   [vJoy Compatibility Guide](docs/vjoy_compatibility.md)
*   [Comparisons with Other Apps](docs/comparisons.md)
*   [FFB Math Formulas](docs/dev_docs/FFB_formulas.md)



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
1.  Open the Powershell.
2.  Navigate to the repository root.
3.  Run the following commands:
    ```cmd
    'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -SkipAutomaticLocation; cmake --build build --config Release --clean-first
    ```

## Building the Installer (WIP, not yet supported)

To create the `LMUFFB_Setup.exe`:

1.  **Install Inno Setup**: Download from [jrsoftware.org](https://jrsoftware.org/isdl.php).
2.  **Build the Project**: Ensure you have built the `Release` version of `LMUFFB.exe` using Visual Studio.
3.  **Run Compiler**: Open `installer/lmuffb.iss` in Inno Setup Compiler and click **Compile**.
4.  **Output**: The installer will be generated in `installer/Output/`.

---


### Upgrading from v0.4.0 to v0.4.1+

If you're upgrading from **v0.4.0**, please note that **v0.4.1 fixes FFB scaling** to properly use Newton-meter (Nm) torque units as required by the LMU 1.2 API change.

**What this means for you:**
- The FFB forces in v0.4.0 were using incorrect scaling (Force units mixed with Torque data)
- v0.4.1 corrects this to be physically accurate
- **You will likely need to increase your gain settings** by approximately 2-3x compared to v0.4.0

**Recommended steps:**
1. Start with your **Master Gain at 1.0** (instead of 0.5)
2. Gradually increase if needed while monitoring the saturation warnings
3. Re-tune individual effect gains (SoP, Lockup, Spin, etc.) to your preference
4. The forces should now feel more realistic and proportional to actual car behavior

This change ensures consistent FFB strength across different hardware and makes the physics calculations match real-world steering torque values (typically 15-25 Nm for racing cars).


### rFactor 2 Compatibility

**Note**: rFactor 2 is **not supported** in v0.4.0+. For rFactor 2, please use earlier versions of lmuFFB (v0.3.x). See the [rFactor 2 Setup Guide](#rfactor-2-setup-legacy) at the end of this document.


## rFactor 2 Setup (Legacy)

**Note**: rFactor 2 support was removed in v0.4.0. To use lmuFFB with rFactor 2, you must download and use **version 0.3.x** from the [releases page](https://github.com/coasting-nc/LMUFFB/releases).

### Prerequisites for rFactor 2 (v0.3.x only):

1. **rF2 Shared Memory Plugin**: Download `rFactor2SharedMemoryMapPlugin64.dll` from [TheIronWolfModding's GitHub](https://github.com/TheIronWolfModding/rF2SharedMemoryMapPlugin#download)
2. **Installation**: Place the DLL in `rFactor 2/Plugins/` directory
3. **Activation**: Enable the plugin in rFactor 2's game settings: edit [Game Folder]\UserData\player\CustomPluginVariables.JSON , set " Enabled" value to 1, and restart rF2  
4. Follow the same vJoy and wheel configuration steps as described above for LMU

For detailed rFactor 2 setup instructions, refer to the README included with v0.3.x releases.
