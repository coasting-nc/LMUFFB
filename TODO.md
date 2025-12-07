# LMUFFB To-Do List

## Completed Features
- [x] **Core FFB Engine**: C++ implementation of Grip Modulation, SoP, and Texture effects.
- [x] **GUI**: Dear ImGui integration with sliders and toggles.
- [x] **Config Persistence**: Save/Load settings to `config.ini`.
- [x] **Installer**: Inno Setup script for automated installation.
- [x] **Documentation**: Comprehensive guides for Architecture, Customization, and Licensing.

## Immediate Tasks
- [x] **DirectInput Support**: Replace vJoy with native DirectInput "Constant Force" packets.
- [] Guided installer as in docs\plan_guided_installer.md
- [] Add in app guided configurator to as described in Guided configurator in the app
- [] If possible, completely remove vJoy dependency as described in docs\investigation_vjoyless_implementation.md
- [] Troubleshooting visualization of FFB and telemetry values as in (future doc) docs\plan_troubleshooting_FFB_visualizations.md
- [ ] **Advanced Filtering**: Implement Bi-Quad filters (Low Pass / High Pass) for better road texture isolation.
- [ ] **Telemetry Analysis**: Add visual graphs to the GUI showing the raw vs. filtered signal.
- [ ] **Telemetry Logging**: Investigate and implement CSV logging. See `docs/telemetry_logging_investigation.md`.

## Backlog / Wishlist
- [ ] **Wheel Profiles**: Save/Load settings per car or wheel base.
- [ ] **Auto-Launch**: Option to start automatically with LMU.
- [ ] **Minimize to Tray**: Keep app running in background without taskbar clutter.


## Bug fixes, polish, throubleshooting, and improvements

Make install instruction more clear. Make them consistent between readme.txt and readme.md (eg. which shared memory plugin to install).
Shared memory plugin: this should be correct (please confirm/verify):
* https://github.com/TheIronWolfModding/rF2SharedMemoryMapPlugin#download
* https://www.mediafire.com/file/s6ojcr9zrs6q9ls/rf2_sm_tools_3.7.15.1.zip/file
Make better instructions on installing and enabling the shared memory plugin. Maybe suggest installing CrewChief since it might do that automatically. Note: there is no ingame option in LMU to enable the shared memory plugin (this was maybe in rFactor 2).

Add link to download vJoy (from github). vJoy SDK dll (64 bit) : https://github.com/shauleiz/vJoy/tree/master/SDK/lib/amd64/vJoyInterface.dll

In the instructions for user (in readme) include some of the tips from docs\bug_reports\Wheel Jerking with SOP Slider.md : **Summary Checklist**,  Tip to bind wheel ("You may need to use the "vJoy Feeder" app (comes with vJoy) to move the virtual axis so the game detects it").
Also need to include in the instructions for user (in readme) that LMU must be run in "Borderless" window mode, otherwise crashes happen.

Guided install: give the tip on how to bind the wheel to vJoy, "You may need to use the "vJoy Feeder" app (comes with vJoy) to move the virtual axis so the game detects it".

Address the bug report in docs\bug_reports\Wheel Jerking with SOP Slider.md
Set default settings to the safest possible, eg.: **SOP Effect** at **0.0** and **Master Gain** at **0.5**.

Another user got a crash (it was probably because LMU was not run in borderless window mode).

Another user was having a very strong wheel if gain was above 0 (even at 0.01).It was like having 100% FFB, and he was using a DD wheel with 9 newton meters (Nm) of force.

Todo: we need a markdown document with a plan on how to make a fully guided install (instead of releasing the exe). The install should guide the user through the process of installing vJoy, enabling the shared memory plugin, and binding the wheel to vJoy. Should also display all other instructions that are on the readme.

Consider this: Would it help if we suggest, in the readme, to disable,  in vjoy config,   all effects except constant?

In the pop up that appears if vJoy is not found, put a more descriptive message (eg. version of vJoy needed, put the dll in the same dir as the exe (naming in full the dll), and say if the dll is from vJoy SDK or basic installation). But anyway the real solution to this is the fully guided install.

Do an investigation on this, and save a report to a markdown file: Apparently Marvin's app managed an implementation that does not need vJoy. We currently need vJoy as a "dummy" device so that the game sends its FFB there. Investigate if we can do something similar to Marvin's app, getting rid of vJoy need.

## Throubleshooting 2

### FFB Visualizations

Add a new, comprehensive markdown document (docs\plan_troubleshooting_FFB_visualizations.md) discussing and planning how can we make the app more easy to debug and troubleshoot. In particular, for cases in which the FFB is not working as intended (eg. forces too strong, jerking, etc.), for be able to more easily identify the cause of the problem. Consider also adding display widgets that show in the GUI more info and visuals on what the FFB is doing, including having separated visual for each effect or formula, so we can see which one is "exploding" with high values, or oscillating, or having other strange behavior.
This could also be in the form of a "trail" visualization, in that we show the trails of multiple values over time (eg. over the last 5-10 seconds), so we can see if there are any patterns or anomalies.

In the GUI there should be an option (eg. checkbox or button) to display these additional visualizations for troubleshooting FFB. Each component in our FFB formula should have its own visualization, so that it can be independently visualized and checked.

Consider also adding additional trouble shooting visuals, like status of the shared memory (eg. values we read). We should definitely include also a visual for each telemetry value that we read. It should be visualized like the other trails for the formulas. Also, each visualization must have a checkbox to enable or disable it, to avoid visual clutter.

We should mention in the readme that there are these visualization options, and that they can be enabled or disabled in the GUI. We should also include a screenshot of the GUI with these visualization options enabled.
From these visualizations, we will be able also to spot telemetry values that are absent (eg. always constant or zero).
We should also log more info to the console every time something is not as expected. Eg. if we detect that some telemetry values are always constant or zero, we should log a warning to the console (be carefull not to clutter it and repeat the same warning multiple times, or too often).

Additionally, we could have an app mode to start a "troubleshooting" sesssion (like a "calibration" or something smilar), in that we ask the user to perform a series of actions to help us identify the cause of the problem. These actions could be things like: 
* drive the car in a straight line, 
* do a braking in a straight line,
* drive the car in a corner, 
* do some train braking (braking and turning into a corner)
* accelerate flat out to try indice wheel sping,
* etc.
During this "troubleshooting session", we should log all the telemetry and FFB values to a file (or at least a summary of them). The FFB values should include the individual values of each effect or formula in the FFB. We should also log the FFB values that we are receiving from the game.
We could either have a mechanism for allowing the user to send this data (logged values and / or summary) to us, or have some code that automatically analyzes that data and spots issue, so that the user can copy and paste the issues report / summary into a bug report.

### Clarify how it works the communication of steering inputs among real wheel, vJoy and game

Solve this contradiction:
the readme instructions say that we must bind the ingame control to vJoy,  so that it is a dummy wheel to send the FFB to instead of the real wheel, but if we do that, we can't control the car from the game right? Or would vJoy take the position information from the real wheel and send it to the game? 
In any case, if possible we need to perform additional checks on this (possibly at regular intervals during runtime, as a throubleshooting feature), eg. if vJoy is receiving control inputs from the real wheel.
Should we add an instruction in the readme that says we should bind the real wheel to vJoy?

### Guided configurator in the app
In addition to the guided installer, from the main GUI of the app, should we have a button to run a guided configurator? This is both for initial configuration, and if things mess up and need to be reconfigured.

### Checks for FFB outputs from the game (not the app): "Race condition Double FFB"

See if it is possible to check if the game is sending FFB to the wheel instead of vJoy: eg. from the shared memory, maybe there is info about it. If we detect it, we should print a warning to the console.

# Update README.txt and or README.md

Update README.txt and README.md. The two had conflicting information about the installation process. Make sure they are consistent.

## Throubleshooting 3

### Verify readme, instructions and other docs apply to LMU

This app is primarily for LMU. All information in readme, instructions and other docs must be verified to be valid for LMU. We can mention rF2 only secondarily.
Eg. there seems to be no "None" FFB option in LMU, verify this and update the readme and any other relevant docs.
Add a screenshot of the LMU in-game settings, so we can reference it during development  to confirm what LMU allows.

TODO: update all references to  In-Game FFB to 'None' or 'Smoothing 0' to reflect what LMU actually allows.

TODO: replace "Game" with "LMU" in all references.

### Test Required for removing vJoy dependency
Note: The dependency on vJoy exists primarily to **prevent the game from locking the physical wheel**.
If LMU allows binding an axis *without* acquiring the device for FFB:
1.  Bind Physical Wheel Axis X to Steering in-game.
2.  Set "FFB Type" to **"None"** in game settings.
3.  LMUFFB acquires the wheel via DirectInput.
    *   **Risk:** If the game opens the device in "Exclusive" mode just to read the Axis, LMUFFB might be blocked from writing FFB (or vice versa). DirectInput usually allows "Background" "Non-Exclusive" reads, but FFB writing often requires "Exclusive".
Can LMUFFB send FFB to a device that LMU is actively reading steering from?
    - If yes: vJoy is NOT needed.
    - If no (Access Denied): We need vJoy.

**Experiment for v0.4.0:**
Try "Method 1":
1.  User binds real wheel in game.
2.  User disables FFB in game (Type: None).
3.  LMUFFB attempts to acquire wheel in `DISCL_BACKGROUND | DISCL_EXCLUSIVE` mode.
4.  If it works, vJoy is obsolete.
5.  If it fails (Device Busy), we need vJoy.

### Check and command line log for FFB acquired exclusiverly by LMU

During the loop (or early startup) add a check if the FFB is acquired exclusiverly by LMU. If it is, print a warning to the console.
We could also add a button in the GUI to perform this check: the user would first start LMU and start driving, ensuring that the FFB is working from LMU. Then the user would start LMUFFB and click the button to perform the check. If the FFB is acquired exclusiverly by LMU, print a warning to the console.

### Review docs\plan_troubleshooting_FFB_visualizations.md

Review docs\plan_troubleshooting_FFB_visualizations.md and make sure that in "### A. FFB Component Graph ("The Stack")" we have listed one graph for each component of the FFB, that is, each individual formula or effect that can independently not work as expected and cause FFB issues.

Do a similar review for section "### B. Telemetry Inspector", making sure that we have an invidual visualization for each value we read from game shared memory / telemetry.

Update docs\plan_troubleshooting_FFB_visualizations.md stating clearly that we want the visualization of each value (FFB or telemetry) to be a "trailing" or "trace" live plot visualization that shows the last 5-10 seconds of data, with a sliding window. The plot should be similar to the "telemetry" trace often shown in driving sims with the driver inputs (accelerator pedal, breaking, steering wheel position, etc.).

## Throubleshooting 4

### More about vJoy

Investigate: can we bundle a copy of the vJoy sdk dll (vJoyInterface.dll) with the app exe? What I mean is, can it be a predefined copy (eg. the 64 bit version, amd 64, since we build the app for 64 bit), or does it need to be coming from a user "installation"? 

Todo: do a check at startup that the vJoy installed version is the expected one, otherwise show a popup with a warning (with a "don't show again" checkbox) and a warning message to the console (this one always active since it is less intrusive).

Investigate: considering licensing and copyright aspects, can we legally bundle the vJoy sdk dll with our exe in the installer or redistributable zip? Should we include a license txt specific to vJoy? 

### Update README.txt again

README.txt install instructions must be as detailed as the ones in README.md. Note for example that in README.txt we don't indicate a path to download the vJoy SDK dll.