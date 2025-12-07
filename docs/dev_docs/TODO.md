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
- [] If possible, completely remove vJoy dependency as described in docs\dev_docs\investigation_vjoyless_implementation.md
- [] Troubleshooting visualization of FFB and telemetry values as in (future doc) docs\plan_troubleshooting_FFB_visualizations.md
- [ ] **Advanced Filtering**: Implement Bi-Quad filters (Low Pass / High Pass) for better road texture isolation.
- [ ] **Telemetry Analysis**: Add visual graphs to the GUI showing the raw vs. filtered signal.
- [ ] **Telemetry Logging**: Investigate and implement CSV logging. See `docs/dev_docs/telemetry_logging_investigation.md`.

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

### DONE:Update README.txt again and remove INSTALL.txt

README.txt install instructions must be as detailed as the ones in README.md. Note for example that in README.txt we don't indicate a path to download the vJoy SDK dll.

Review INSTALL.txt: does it contain any additional information that is not in README.txt? It seems it is a redundant file, that should be deleted once we include all the information in README.txt.

### DONE: Telemetry trail visualization example

See image docs\bug_reports\telemetry_trail_example.png , which display a typical plot visualization with a telemetry trail of user inputs (accelerator pedal and breaking pedal), with a trailing window of a few seconds. On the right side, there are also histograms of the current (istant) values of the same telemetry values. In any case, this current note from docs\plan_troubleshooting_FFB_visualizations.md is already sufficient to precisely identify the style of these plots: "*Visual Style:* All graphs should be **Rolling Trace Plots** (Oscilloscope style), showing the last 5-10 seconds of data. This helps spot patterns like oscillation or spikes."

## Throubleshooting 4

### More about vJoy

- [x] **Bundling Investigation**: Yes, we can bundle `vJoyInterface.dll` (x64). See `docs/dev_docs/investigation_vjoy_bundling.md`.
- [x] **Version Check**: Implemented startup check for vJoy driver version (warns if < 2.1.8).
- [x] **Licensing**: vJoy is MIT licensed. Added `licenses/vJoy_LICENSE.txt`.


## Throubleshooting 5

### LMU  in-game settings: controls and FFB

See these screenshots to see the available options in LMU:
* docs\bug_reports\LMU_settings_controls.png
* docs\bug_reports\LMU_settings_FFB.png

In particular the relevant setting seems to be "Force Feedback Strenght", that we should set to 0% , so that LMU does not send FFB to the wheel. Update README.md and README.txt to reflect this (instructing on setting this value to 0% in the in-game settings).
We should also probably instruct the user to disable the first toggle, "Force Feedback Effects". Also minimum steering torque should be set to 0%. Pheraphs also "Force Feedback Smoothing" should be set to 0. Also the values in "Advanced Force Feedback" should be set to 0: "Collision Strenght" and "Steering Torque Sensitivity". And also disable the toggle in the final section "Tweaks": "Use Constant Steering Force Effect".

### Smoothing options

For each case in which apply smoothing or other corrections to the FFB formula (eg. cap to values), we should add support to disable/enable each (individually) and, when possible, to configure them within a range of values (eg. from a minimum to a maximum value). For the cap to values, we might allow to increase or decrease the cap value, so that the user can fine tune the FFB to their liking. We might still enforce some hard limits on the ranges of configuration values allowed (to prevent values that will cause clear issues or bugs).


### vJoy bundling

Note that we can consider separately the vJoy installer and the vJoy sdk dll. We could only bundle the sdk dll, and let the user install vJoy through the installer. But we must also check that the installed vJoy version is the expected one, otherwise show a popup with a warning (with a "don't show again" checkbox) and a warning message to the console (this one always active since it is less intrusive). Confirm this is already fully implemented or not.

### vJoy links

Keep this reference for the link to download the vJoy SDk (although we can bundle it with our app): 
https://sourceforge.net/projects/vjoystick/files/Beta%202.x/2.1.9.1-160719/

Keep this reference for the version of vJoy to install:
https://github.com/jshafer817/vJoy/releases/tag/v2.1.9.1


## Throubleshooting 6

### vJoy dll location issue
Address this issue:
"I have moved LMUFFB.exe to the vJoy installation directory (C:\Program Files\vJoy\x64), because the vJoy config/monitor/feeder was complaining if I moved the 'vJoyInterface.dll' elsewhere. LMUFFB seems fine running in here."
We might not want to bundle the vJoyInterface.dll with our app, because of the reported issue. We could either bundle the full vJoy SDK with our app, or if the user has it already installed (with the SDK, note that there are versions of vJoy without the SDK) we might ask the user, during the guided installer (and the guided configurator) to set the path in which the vJoyInterface.dll is located. 
To address this issue, update docs\plan_guided_installer.md and docs\dev_docs\plan_guided_configurator.md .

### wheel binding issues

Address this user reported bug: docs\bug_reports\User bug report 002 VJoy Setup Troubleshooting and Bug.md

Implement "### Recommendation for Developers (Code Fix)". Also add an option in the Gui to enable / disable m_output_ffb_to_vjoy (disabled by default).

Update accordingly this section of the README.md : "4. Steering Axis - Choose ONE method:", and any other relevant section.


### Readme updates (txt and md)

In the install and configuration instructions, consider (if relevant according to latest code changes and findings) adding instruction to the user to disable the LMU in-game FFB forces (so no forces are transmitted from the game to the wheel), using these setting:  "Force Feedback Strenght", that we should set to 0% , so that LMU does not send FFB to the wheel. Update README.md and README.txt to reflect this (instructing on setting this value to 0% in the in-game settings).
We should also probably instruct the user to disable the first toggle, "Force Feedback Effects". Also minimum steering torque should be set to 0%. Pheraphs also "Force Feedback Smoothing" should be set to 0. Also the values in "Advanced Force Feedback" should be set to 0: "Collision Strenght" and "Steering Torque Sensitivity". And also disable the toggle in the final section "Tweaks": "Use Constant Steering Force Effect".

If these suggestions make sense, consider if making them the main instructions, or as secondary suggestions of things to try or for troubleshooting.

## Throubleshooting 7

See docs\bug_reports\User bug report 002 VJoy Setup Troubleshooting and Bug.md , there is an update section from "# Follow up question 1" onward. Please address that and update the README.md and README.txt, and any other relevant document accordingly.

