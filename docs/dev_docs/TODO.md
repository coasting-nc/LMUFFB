# LMUFFB To-Do List

## Completed Features
- [x] **Core FFB Engine**: C++ implementation of Grip Modulation, SoP, and Texture effects.
- [x] **GUI**: Dear ImGui integration with sliders and toggles.
- [x] **Config Persistence**: Save/Load settings to `config.ini`.
- [x] **Installer**: Inno Setup script for automated installation.
- [x] **Documentation**: Comprehensive guides for Architecture, Customization, and Licensing.
- [x] **DirectInput Support**: Replace vJoy with native DirectInput "Constant Force" packets.
- [x] **Troubleshooting Visualizations**: Implemented Real-time "Rolling Trace Plots" for FFB components and Telemetry inputs in the GUI (v0.3.12).
- [x] **Advanced Filtering**: Implemented SoP Smoothing (Low Pass Filter).
- [x] **Telemetry Robustness**: Implemented Sanity Checks for missing Load/Grip/DeltaTime (v0.3.19).

## Immediate Tasks
- [] Guided installer as in docs\plan_guided_installer.md
- [] Add in app guided configurator to as described in Guided configurator in the app
- [] If possible, completely remove vJoy dependency as described in docs\dev_docs\investigation_vjoyless_implementation.md
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

## Throubleshooting 8

1. Review the FFB code again, to check for all the smoothing we apply. I remember there are supposed to be at least 2 different smoothings applied in different parts of the formula / different effects. For each one of such smoothings, there must be a configuration option (also in the GUI) to adjust it (including a neutral value that would effectively disable the smoothing). Currently, the GUI (in Advanced Tuning) allows to to adjust "SoP Smoothing". 

2. Implement this major new feature:
- [] Troubleshooting visualization of FFB and telemetry values as in (future doc) docs\plan_troubleshooting_FFB_visualizations.md

## Throubleshooting 9

In the readmes, for the "bridge" solution, include more detailed instructions as described here: docs\bug_reports\User bug report 002 VJoy Setup Troubleshooting and Bug.md , "# Answer 1" section. I copy them here:

### 1. Download Joystick Gremlin
*   Download it here: [https://whitemagic.github.io/JoystickGremlin/](https://whitemagic.github.io/JoystickGremlin/)
*   Install and run it.

### 2. Map Your Wheel (The Bridge)
1.  In Joystick Gremlin, you will see tabs at the top for your connected devices. Click the tab for your **Moza R9**.
2.  Turn your physical wheel. You should see one of the axes in the list highlight/move (usually "X Axis" or "Rotation X").
3.  Click on that axis in the list.
4.  On the right side, under "Action", choose **"Map to vJoy"**.
5.  **Crucial Step (To fix the spinning bug):**
    *   Select vJoy Device: **1**
    *   Select Axis: **Y** (Do **NOT** select X, as LMUFFB is currently overwriting X for its display, causing the spinning).

### 3. Activate the Bridge
*   In the top menu of Joystick Gremlin, click the **"Activate"** button (the Gamepad icon turns green).
*   *Test:* Now, when you turn your physical Moza wheel, the **Y bar** in the vJoy Monitor (or the Feeder you screenshotted) should move.

### 4. Bind in Game
*   Go into Le Mans Ultimate.
*   Bind **Steering** to that new **vJoy Axis Y**.

Once you do this, the "Bridge" is complete:
**Moza** -> **Joystick Gremlin** -> **vJoy Axis Y** -> **Game**.

This leaves **vJoy Axis X** free for LMUFFB to use for its display without interfering with your steering.



## Throubleshooting 10

Implement the changes described in this doc:
docs\dev_docs\proposed_changes_to_disable_vJoy.md

## Throubleshooting 11
1. I repea what I requested above:
I have done a change to your latest code in main.pp, please review if this is correct: Line 71 (my version): if (vJoyDllLoaded && DynamicVJoy::Get().Enabled()) { // TODO: I have re-added " && DynamicVJoy::Get().Enabled()" make sure this is correct (your version): if (vJoyDllLoaded) {

Verify if this change is correct. Note that I have not pushed the changes, so you must compare your local code with the line I have pasted here.


2.  Also, in your latest changes, I don't see any addition of a new GUI checkbox to "release" vJoy ("Output FFB to vJoy").
Have you merged / confused it with "Monitor FFB on vJoy" checkbox? I think they should remain two separate checkboxes.
Also, every time you enable / disable it, and we acquire or release vJoy, we should print that info on the console.
Please review again docs\dev_docs\proposed_changes_to_disable_vJoy.md


3. In our FFB formula, we have an hardcoded scaling factor of 1000 for the SoP. Instead, expose the scaling factor in the GUI so that it can be adjusted by the user within a range of values.

## Throubleshooting 12

### More on enabling vJoy checkboxes
The difference between vJoy enabled and vJoy monitoring (the two checkboxes) it that monitoring additionally reads the vJoy values and displays them in the telemetry, for debugging purposes. In theory, we could have vJoy enabled without monitoring (but not the other way around I think).

### Casting to int for Plotting
Note that to avoid a compile warning to precision loss I added some casting in src\GuiLayer.cpp, eg:
        ImGui::PlotLines("##Total", plot_total.data.data(), (int)plot_total.data.size(), plot_total.offset, "Total", -1.0f, 1.0f, ImVec2(0, 60));
These int casting are present for all PlotLines calls. Make sure you edit your code with this change, so I don't get any merge issue later.

### Mutex bug

Verify the issues described here and in case fix it: docs\dev_docs\Missing Mutex Lock (Race Condition).md

## Troubleshooting 13

Then implement the changes described in this doc:
docs\dev_docs\decouple_plots_from_gui_refresh.md

Please also add a changelog entry for version 0.3.17 and for this upcoming change (version 0.3.18).

## Troubleshooting 14

Add the following logs / prints to console:
When we release vJoy. When we unselect vJoy as device.


## Troubleshooting 15

Add option "always on top". Possibly also for the console window.

Two missing data found: tire load (avg tyre load) and grip fraction (avg grip fraction). Not exactly missing, more two binary values only (like 0 and 1 only).

Add a way to debug this missing data, see what the game is actually putting out..

Build a log system that logs all the data logged from the game (and also each component that we calculate).
..autosave these data to csv or other file easy to visualize.
could be also enough to inspect it as a txt file
also, we could print stats for each channel: min, max, avg, std dev, mean, 1st and 3rd quartile, etc
Test was done while driving on keyboard.

Investigate what happens to our formula when grip fract and tyre load are missing. What are the default values?

Investigate the user reported issues with the last version (0.4.0):
"I was able to try it on the wheel today, and while the FFB is still not usable, there are some noteworthy changes. 

I first tried the direct input method, where I bound my wheel directly in game after activating vJoy on the x-axis, and the telemetry data now puts out lots of info. The FFB is also different from before, it moves the wheel around **seemingly at random**, while not being connected to the car at all. I am **unable to steer the car** currently, and the car **turns left constantly**. I will add some telemetry info below, the first two are from mapping the wheel directly, without using Joystick Gremlin. 

The picture where telemetry fails to gather certain data is from the attempt using the Y-axis with Joystick Gremlin. I see **no difference in FFB or otherwise between the two methods**, except for the errors in telemetry data. It seems to me like using the alternative method might not be necessary for the future.(...) Anyway, here are the screenshots:"

Good thing is that out final output ffb it not "binary" anymore, but show some actual waveform. There is some clipping occasionally, but that can be tweaked adjusting the gain of the final output or smt like that.

Also other channels might need refinement in how we use them, since values where very low. Either that, or we should change the scale of the graph visualization.

Add feature: multiple preset for the sliders values.

Do test with wheel, vjoy, etc. See what it is actually needed for: our app sending FFB signal, and the game not sending FFB to the wheel.
The signal arriving when we are driving.
Make sure we can manage properly the fact that the user might be driving or in menu, and that the game might also not have been launched. Add a new load / start loop procedure, with buttons in the GUI to retry.
Print more debugging logging info to the console, for each event on device added, acquired, disconnected etc.
Try to acquire the Wheel in direct input in exclusive mode. Try before and after having launched the game.
Reconsider if Joystick Gremlin is necessary.
If useful, compare behavior with using keyboard vs wheel. Binding keyboard to vjoy / j.gremlin to mock a wheel?

Feed the screenshots and issue description to LLM, and see which solutions or experiments they suggest.

print the stats for all the channels to make sure we are using them correctly, at correct scales, and that we are getting real car data (for tire load and grip fract).

TEST TODO: The "Zero Gain" Test (Verify Noise):
Action: Set Master Gain to 0.0 in LMUFFB.
Action: Drive. Does the wheel still move randomly?
Result: If yes, the game is still sending FFB (Double FFB). If no, the random movement comes from LMUFFB calculations.

Consider if we could implement via code a check on whether the uses has active previous shared memory plugins (the one for rF2 and the new one for LMU). Both are not necessary with LMU 1.2. Consider if their presence might interfere with the values we are reading from the official LMU 1.2 shared memory interface.

Consider how we could use additional channels (eg. longitudinal patch velocity / acceleration, rear tires grip, lock up detection, wheelspin detection, etc.)

inprove the customizable sliders in the Gui to allow more options: isolating better each component of the formula, with enable / disable option (disable put a neutral value like 1 in the formulas) for
self aligning torque, lateral g, understeer effect, oversteer effect, tire spin from gas, slide / slip angle, lockup under braking, etc.
more clearly organize each effect / component of the final formula, which its own section in the gui, with option to disable/enable, slider for adjusting main value, smooting factors, caps, etc.

this will help also isolate individual components of the formula in trouble shoootig initial experiments to get a meaningful FFB signal in the wheel.
Do the esperiments isolating 1 component at a time, disabling all the others. Can we feel it? Is it as expected?
See if only adjusting the current sliders produces some usable (not random) FFB signal feel in the wheel.

Ask LLM for priority of things to try next:
settings in LMU, setting FFB force to zero ingame: does it change FFB from the app?

is there something wrong in how we send data to direct input? Since the user described the FFB as random, but in the plot it looked reasonable.
We should add in the troubleshooting visualizations also plots for car behavior:
sterring wheel, gas pedal, brake pedal, speed, acceleration, ..
This will help understand what the car was doing, and explain better what we see in the other channels.

Note that from the telemetry we are also missing wheelspin information. 
It was in rF2 telemetry generated by motec, but not in rF2Data.h
Wheelspin might be derived from the patch velocity and ground velocity.
In LMU we also have mStaticUndeflectedRadius (radius of the tyre) which was missing in rF2. This might allow precise calculation of wheelspin and possibly lockup.

In rF2Data.h :
    double mLateralPatchVel;
    double mLongitudinalPatchVel;
    double mLateralGroundVel;
    double mLongitudinalGroundVel;

In InternalsPlugin.hpp:
    double mRotation;              // radians/sec
    double mLateralPatchVel;       // lateral velocity at contact patch
    double mLongitudinalPatchVel;  // longitudinal velocity at contact patch
    double mLateralGroundVel;      // lateral velocity at contact patch
    double mLongitudinalGroundVel; // longitudinal velocity at contact patch
    mStaticUndeflectedRadius

    Here is the prioritized diagnosis and implementation plan based on your notes, the screenshots, and the codebase analysis.

### Hypotheses on Root Causes

1.  **The "Car Turns Left / Random Steering" Issue (Critical):**
    *   **Hypothesis:** **Feedback Loop via vJoy.**
    *   **Reasoning:** If the user has `Monitor FFB on vJoy` enabled (or if it was enabled previously and vJoy axis is stuck), the App writes the *Calculated Force* to vJoy Axis X. If the user *also* bound the Game's Steering Input to vJoy Axis X, the FFB signal effectively "steers" the car.
    *   **Evidence:** "Moves the wheel around seemingly at random... unable to steer." If the FFB calculates a left force, it moves vJoy X left. The game sees vJoy X left, steers left. Physics generates more aligning torque. FFB increases. Infinite loop.

2.  **The "Binary" / Square Wave Telemetry:**
    *   **Hypothesis:** **Sanity Check Artifacts.**
    *   **Reasoning:** In `FFBEngine.h`, you have logic: `if (avg_load < 1.0 ... ) avg_load = 4000.0;`.
    *   **Evidence:** The screenshots show `Avg Tire Load` jumping exactly between 0 and a high flat value. This confirms the *raw* data from the game is **0.0**, and what you see on the graph is your own fallback logic toggling on/off based on whether the car is moving (`mLocalVel.z > 1.0`).
    *   **Conclusion:** Despite using the new LMU 1.2 interface, the app is reading empty data for the specific car/session the user is running.
TODO: in the plots show the raw value from the game, not our fallback values.

3.  **The "Random" FFB Feel:**
    *   **Hypothesis:** **Noise Amplification.**
    *   **Reasoning:** Since Tire Load is toggling between 0 and 4000 instantly, any effect multiplied by `load_factor` (Slide, Road, Lockup) is being switched on/off violently 400 times a second. This creates a "random" or "noisy" sensation.

---

### Part 1: Prioritized Diagnostic Steps (Try these first)

1.  **Verify Input Binding (Stop the Feedback Loop):**
    *   **Action:** Ask the user to ensure **"Monitor FFB on vJoy"** is **UNCHECKED** in the App.
    *   **Action:** In Game Controls, ensure Steering is bound to their **Physical Wheel**, NOT vJoy (unless they are using Joystick Gremlin on Axis Y).
    *   **Test:** Does the "constant left turn" stop?

2.  **Inspect Raw Data (Bypass Sanity Checks):**
    *   **Action:** We need to see if the game is outputting *anything* other than zero. The current graphs hide this because of the fallback logic.
    *   **Code Change:** Add a "Raw" debug line (see Implementation below).

3.  **Test Different Car Classes:**
    *   **Action:** Ask the user to try a GTE car vs. a Hypercar. LMU 1.2 might populate telemetry differently for different physics models.

4.  **Verify Shared Memory Lock:**
    *   **Action:** Check the console output. Does it say "Shared Memory Lock Initialized"? If the lock isn't working, we might be reading a zeroed-out buffer.

---

### Part 2: Prioritized Implementation List

#### 1. CSV Telemetry Logger (High Priority)
We need to record the raw data to analyze it offline. Screenshots are insufficient for 400Hz data.

*   **What:** A class that writes `timestamp, raw_load, raw_grip, raw_steer, final_ffb` to a `.csv` file.
*   **Why:** To definitively prove if the game is outputting 0.0 or just very low values.

#### 2. "Always On Top" Window Option
*   **What:** Add a toggle to keep the GUI visible over the game (borderless).
*   **Why:** Requested by you/user for easier tuning while driving.

#### 3. Raw vs. Corrected Visualization
*   **What:** In `FFBEngine`, store `raw_tire_load` separately from `avg_load` (which gets the 4000N fallback).
*   **Why:** The current graphs show the fallback value, making it look like "binary" data. We need to see the real input.

#### 4. Safety: Disable vJoy Output by Default
*   **What:** Hardcode `Config::m_output_ffb_to_vjoy = false` and ensure it resets to false on startup unless explicitly saved.
*   **Why:** To prevent the "Car turns left" feedback loop.

---

### Part 3: Proposed Code Changes

#### A. Implement "Always On Top" (GuiLayer.cpp)

Modify `GuiLayer::Init` and `GuiLayer::Render`.

```cpp
// src/GuiLayer.h
static bool m_always_on_top; // Add this static member

// src/GuiLayer.cpp
bool GuiLayer::m_always_on_top = false;

// Inside DrawTuningWindow (add checkbox)
if (ImGui::Checkbox("Always On Top", &m_always_on_top)) {
    SetWindowPos((HWND)g_hwnd, m_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST, 
                 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}
```

#### B. Implement CSV Logger (New Class)

Create `src/TelemetryLogger.h`:

```cpp
#ifndef TELEMETRYLOGGER_H
#define TELEMETRYLOGGER_H

#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>

class TelemetryLogger {
    std::ofstream m_file;
    bool m_recording = false;
    long m_start_time = 0;

public:
    static TelemetryLogger& Get() {
        static TelemetryLogger instance;
        return instance;
    }

    void Start() {
        if (m_recording) return;
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << "lmuffb_log_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S") << ".csv";
        
        m_file.open(ss.str());
        // Header
        m_file << "Time,RawSteer,RawLoadFL,RawLoadFR,RawGripFL,RawGripFR,CalcFFB\n";
        m_recording = true;
        m_start_time = GetTickCount();
    }

    void Stop() {
        if (m_recording) {
            m_file.close();
            m_recording = false;
        }
    }

    void Log(float rawSteer, float loadFL, float loadFR, float gripFL, float gripFR, float outputFFB) {
        if (!m_recording) return;
        float time = (GetTickCount() - m_start_time) / 1000.0f;
        m_file << time << "," << rawSteer << "," << loadFL << "," << loadFR 
               << "," << gripFL << "," << gripFR << "," << outputFFB << "\n";
    }
    
    bool IsRecording() { return m_recording; }
};
#endif
```

**Integration in `main.cpp` (FFBThread):**

```cpp
// Inside the loop, after calculating force:
if (TelemetryLogger::Get().IsRecording()) {
    TelemetryLogger::Get().Log(
        pPlayerTelemetry->mSteeringShaftTorque,
        pPlayerTelemetry->mWheel[0].mTireLoad,
        pPlayerTelemetry->mWheel[1].mTireLoad,
        pPlayerTelemetry->mWheel[0].mGripFract,
        pPlayerTelemetry->mWheel[1].mGripFract,
        (float)force
    );
}
```

**Integration in `GuiLayer.cpp`:**
Add a "Start/Stop Logging" button in the Debug Window.

#### C. Fix "Binary" Visualization (FFBEngine.h)

We need to capture the *raw* values in the snapshot before we apply the fallback logic.

```cpp
// FFBEngine.h

struct FFBSnapshot {
    // ... existing fields ...
    float raw_tire_load; // NEW
    float raw_grip;      // NEW
};

// Inside calculate_force:
double raw_avg_load = (fl.mTireLoad + fr.mTireLoad) / 2.0;
double raw_avg_grip = (fl.mGripFract + fr.mGripFract) / 2.0;

// ... existing sanity check logic ...
double avg_load = raw_avg_load;
if (avg_load < 1.0 && ...) { avg_load = 4000.0; } // Fallback

// ... inside Snapshot creation ...
snap.raw_tire_load = (float)raw_avg_load; // Store RAW
snap.raw_grip = (float)raw_avg_grip;      // Store RAW
snap.tire_load = (float)avg_load;         // Store CORRECTED
```

**Update `GuiLayer.cpp`:**
Add a new plot line for `Raw Load` (maybe in a different color) on top of the `Avg Tire Load` graph to show the difference.

#### D. Refine FFB Formula (Prevent Randomness)

If the data is indeed missing (0.0), the fallback logic toggles between 0 and 4000 based on `mLocalVel.z`. If velocity is noisy near 1.0 m/s, this toggles rapidly.

**Change in `FFBEngine.h`:**
Add hysteresis or a blend to the fallback.

```cpp
// Instead of hard switch:
if (avg_load < 1.0) {
    // If missing, ramp up to 4000 based on speed smoothly
    double speed_factor = (std::min)(1.0, std::abs(data->mLocalVel.z) / 5.0); // Ramp over 0-5 m/s
    avg_load = 4000.0 * speed_factor;
}
```
This prevents the "square wave" on/off switching at low speeds, making the fallback smoother.

### Summary of Next Steps

1.  **Immediate:** Instruct user to uncheck "Monitor FFB on vJoy" and verify game bindings (Fixes steering loop).
2.  **Code:** Implement `TelemetryLogger` and the `Raw` vs `Corrected` data split in `FFBEngine`.
3.  **Test:** Run the logger. If `RawLoad` is truly 0.0 in the CSV, then the LMU 1.2 Shared Memory interface is still not providing data for that car, or we need to check `mExpansion` bytes or other fields in the struct.

# Answer 2
Here is the prioritized diagnosis and implementation plan based on the user report, screenshots, and codebase analysis.

### 1. Diagnosis: Prioritized Things to Try Next

Ask the user (or yourself during testing) to perform these specific steps to isolate the "Random/Left Turn" issue.

1.  **The "Zero Gain" Test (Crucial)**
    *   **Action:** Set **Master Gain** to `0.0` in LMUFFB. Drive.
    *   **Observation:**
        *   *If wheel still pulls left/jerks:* The force is coming from the **Game** (Double FFB) or a hardware centering spring. LMUFFB is innocent.
        *   *If wheel goes limp:* The bad force is definitely coming from LMUFFB calculations.

2.  **The "Raw Input" Check**
    *   **Action:** In LMUFFB, look at the **Telemetry Inspector** graph for **Steering Torque (Nm)** while the car is stationary or moving slowly straight.
    *   **Observation:**
        *   Is it hovering near 0?
        *   Is it stuck at a high value (e.g., -20 Nm)? If so, the *game* is reporting a constant torque, which LMUFFB is simply amplifying.

3.  **The "Effect Isolation" Test**
    *   **Action:** Set **Master Gain** to `1.0`. Set **SoP**, **Understeer**, **Oversteer**, and **All Textures** to `0.0` or `Disabled`.
    *   **Result:** This passes the raw `mSteeringShaftTorque` through. Does it still jerk?
    *   *Hypothesis:* If this feels okay, the "Randomness" comes from the **SoP** or **Texture** calculations reacting to the "Missing" (Zero) tire data.

4.  **Verify Input Binding**
    *   **Action:** Ensure the user hasn't accidentally bound the **Accelerator** or **Clutch** axis to the **Steering** axis in the game. A "constant left turn" often happens when a pedal (0 to 100%) is mapped to steering (-100% to +100%), resulting in a permanent 50% turn or similar offset.

---

### 2. Hypotheses on Root Causes

1.  **The "Fallback" Oscillation (Most Likely for "Random" feel)**
    *   **Issue:** The screenshots show `Avg Tire Load` and `Avg Grip Fract` are **MISSING** (Red text).
    *   **Mechanism:** Your code has a sanity check: `if (load < 1.0) load = 4000.0;`.
    *   **The Bug:** If the telemetry flickers between `0.0` (Missing) and `0.0001` (Noise), the code rapidly toggles between **0N** and **4000N** of load.
    *   **Result:** This acts like a square-wave generator, turning effects on and off 400 times a second. This feels like "random jerking" or "binary" vibration.

2.  **Steering Torque Unit Mismatch**
    *   **Issue:** LMU 1.2 changed `mSteeringArmForce` to `mSteeringShaftTorque`.
    *   **Hypothesis:** The old value was in Newtons (approx +/- 4000). The new value is in Nm (approx +/- 20).
    *   **Code:** You updated the graph scale to +/- 30, but did you update the **Normalization** in `FFBEngine.h`?
    *   *Check:* `double max_force_ref = 20.0;` (In your provided `FFBEngine.h`). This looks correct, *but* if the game outputs 25Nm and you clamp at 20, you clip. If the game outputs raw steering rack force (Newtons) in that field by mistake, you are multiplying it by huge numbers.

3.  **Axis Conflict (The "Left Turn")**
    *   If `Config::m_output_ffb_to_vjoy` is enabled, and the user mapped Game Steering to vJoy Axis X, the FFB signal (which might be negative/left) is being fed back into the steering input.

---

### 3. Implementation Plan (Prioritized)

#### A. Implement "Unbind Device" Button (UX Fix)
Users are confused about whether the device is active.

#### B. Implement Console Stats Logging (Debugging)
We need to see the raw numbers to confirm if they are 0, NaN, or just noisy.

#### C. Fix "Missing Data" Handling
Instead of hard-toggling defaults every frame, implement a "State Latch". If data is missing for > 1 second, switch to fallback and *stay* there until valid data returns for > 1 second.

#### D. Safety: Force Disable vJoy Output
Hardcode `m_output_ffb_to_vjoy = false` in the constructor or `Load` to ensure it doesn't accidentally turn on for users.

---

### 4. Proposed Code Changes

#### Step 1: Add "Unbind Device" to `DirectInputFFB`

**File:** `src/DirectInputFFB.h`
```cpp
// Add to public:
void ReleaseDevice();
```

**File:** `src/DirectInputFFB.cpp`
```cpp
void DirectInputFFB::ReleaseDevice() {
#ifdef _WIN32
    if (m_pEffect) {
        m_pEffect->Stop();
        m_pEffect->Unload();
        m_pEffect->Release();
        m_pEffect = nullptr;
    }
    if (m_pDevice) {
        m_pDevice->Unacquire();
        m_pDevice->Release();
        m_pDevice = nullptr;
    }
    m_active = false;
    m_deviceName = "None";
    std::cout << "[DI] Device released by user." << std::endl;
#endif
}
```

**File:** `src/GuiLayer.cpp` (Inside `DrawTuningWindow`)
```cpp
    // ... inside the Device Selection combo block ...
    if (ImGui::Button("Rescan Devices")) {
        devices = DirectInputFFB::Get().EnumerateDevices();
        selected_device_idx = -1;
    }
    
    // NEW BUTTON HERE
    ImGui::SameLine();
    if (ImGui::Button("Unbind Device")) {
        DirectInputFFB::Get().ReleaseDevice();
        selected_device_idx = -1;
    }
    // ...
```

#### Step 2: Implement Non-Blocking Stats Logging

We will add a simple struct to accumulate stats and print them once per second.

**File:** `FFBEngine.h`

Add this struct inside the class or globally:
```cpp
struct ChannelStats {
    double min = 1e9;
    double max = -1e9;
    double sum = 0.0;
    long count = 0;
    
    void Update(double val) {
        if (val < min) min = val;
        if (val > max) max = val;
        sum += val;
        count++;
    }
    
    void Reset() {
        min = 1e9; max = -1e9; sum = 0.0; count = 0;
    }
    
    double Avg() { return count > 0 ? sum / count : 0.0; }
};
```

Add these members to `FFBEngine` class:
```cpp
    // Logging State
    ChannelStats s_torque;
    ChannelStats s_load;
    ChannelStats s_grip;
    ChannelStats s_lat_g;
    
    std::chrono::steady_clock::time_point last_log_time = std::chrono::steady_clock::now();
```

Update `calculate_force` in `FFBEngine.h`:

```cpp
    double calculate_force(const TelemInfoV01* data) {
        // ... existing setup ...
        
        // 1. Capture Raw Values
        double raw_torque = data->mSteeringShaftTorque;
        double raw_load = (data->mWheel[0].mTireLoad + data->mWheel[1].mTireLoad) / 2.0;
        double raw_grip = (data->mWheel[0].mGripFract + data->mWheel[1].mGripFract) / 2.0;
        double raw_lat_g = data->mLocalAccel.x;

        // 2. Update Stats
        s_torque.Update(raw_torque);
        s_load.Update(raw_load);
        s_grip.Update(raw_grip);
        s_lat_g.Update(raw_lat_g);

        // 3. Periodic Print (Non-blocking check)
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_log_time).count() >= 1) {
            std::cout << "--- TELEMETRY STATS (1s) ---" << std::endl;
            std::cout << "Torque (Nm): Avg=" << s_torque.Avg() << " Min=" << s_torque.min << " Max=" << s_torque.max << std::endl;
            std::cout << "Load (N):    Avg=" << s_load.Avg()   << " Min=" << s_load.min   << " Max=" << s_load.max << std::endl;
            std::cout << "Grip (0-1):  Avg=" << s_grip.Avg()   << " Min=" << s_grip.min   << " Max=" << s_grip.max << std::endl;
            std::cout << "Lat G:       Avg=" << s_lat_g.Avg()  << " Min=" << s_lat_g.min  << " Max=" << s_lat_g.max << std::endl;
            
            // Reset
            s_torque.Reset(); s_load.Reset(); s_grip.Reset(); s_lat_g.Reset();
            last_log_time = now;
        }

        // ... continue with existing calculation logic ...
```

#### Step 3: Improve "Missing Data" Logic (Hysteresis)

Modify the sanity check in `FFBEngine.h` to stop flickering.

```cpp
    // Add member variable
    int m_missing_load_frames = 0;

    // Inside calculate_force
    double avg_load = (fl.mTireLoad + fr.mTireLoad) / 2.0;

    // Hysteresis: Only switch to fallback if missing for > 20 frames (50ms)
    // Only switch back to real data if present for > 20 frames
    if (avg_load < 1.0 && std::abs(data->mLocalVel.z) > 1.0) {
        m_missing_load_frames++;
    } else {
        m_missing_load_frames = std::max(0, m_missing_load_frames - 1);
    }

    // Use fallback if we have been missing data for a while
    if (m_missing_load_frames > 20) {
        avg_load = 4000.0; 
        if (!m_warned_load) {
             std::cout << "[WARNING] Tire Load missing. Using fallback." << std::endl;
             m_warned_load = true;
        }
    }
```

### 5. Additional Debugging for "Left Turn"

In `DirectInputFFB.cpp`, inside `UpdateForce`, add a print if the force is consistently saturated.

```cpp
    // Inside UpdateForce
    if (std::abs(normalizedForce) > 0.99) {
        static int clip_log = 0;
        if (clip_log++ % 400 == 0) {
            std::cout << "[DI] WARNING: Output saturated at " << normalizedForce << ". Possible feedback loop or scaling issue." << std::endl;
        }
    }
```


## Troubleshooting 16

It seems our app sends an "inverted FFB" signal. At least this happens with my T300 wheel. Add a check box option in the GUI to invert the FFB signal.

When a device is acquired, console does not say if Exclusive or background. Please specify it in the console message.
Here is what it says currently:
[DI] Attempting to set Cooperative Level (Exclusive | Background)...
[DI] Acquiring device...
[DI] Device Acquired.
[DI] SUCCESS: Physical Device fully initialized and FFB Effect created.

In the stats printed in the console about the telemetry values: it is not clear if the stats (eg. average) refer to only the last second or to the entire duration of the session. Clarify this. In any case, for min and max value, I want to see the values for the entire duration of the session.

===

Please also investigate this issue and give me an additional section with a plan to fix it:
It seems that master gain should be at 0.5 (instead of 1) to reproduce the same FFB intensity as with original signal . That is, in order for Base torque to be the same as "Total output" when all other influencing factors are neutralized. This might indicate that some coefficient or scale is incorrect (eg. it is double what it should be).

===

ensure we have proper and stable values for the missing data (grip and load)

do the logging totals (on console or on file) when car is moving, that is speed above a certain min speed
and stop loggin when it returns below that
accumulate the values, for min, max, .., only when car is moving

The "always on top" feature of the window (if present) is not customizable as on/off in the GUI. Please add this option. And implement the feature if not already present.
TODO: always on top option not there in the Gui

TODO: before starting LMU, disable rf2 plugin if present, from json file.
This might be an issue.



TODO: it seems the ffb is inverted from our app. Add checkbox to invert ffb


--- TELEMETRY STATS (1s) ---
Torque (Nm): Avg=0.609297 Min=0.609297 Max=0.609297
Load (N):    Avg=0 Min=0 Max=0
Grip (0-1):  Avg=0 Min=0 Max=0
Lat G:       Avg=-1.17966e-06 Min=-1.17966e-06 Max=-1.17966e-06

## Troubleshooting 17
Implement workaounds_and_improvements_ffb
See docs/dev_docs/workaounds_and_improvements_ffb_v0.4.4+.md


investigate which ffb effects can be implemented with current data
eg. info about patch velocity and forces, info on wheel rim and ..rotation, etc.
we also have values for slip..

## Troubleshooting 18

Implement all mitigations to stability risks as discussed and recommended in  docs\dev_docs\grip_calculation_analysis_v0.4.5.md and docs\dev_docs\Stability Risks & Mitigations_v0.4.5.md

## Troubleshooting 19

The troubleshooting graphs are not updated. They do not show the new values used from shared memory, and the new components we calculate.

## Troubleshooting 20

### More on plots and math formulas
#### Done

In formulas md doc, also update the name of variables specifying when they refer to front types only (eg  Grip_avg is only for fronts)

In the plots, we have Calc Front Grip  but we don't have _Calc Rear Grip_
We don't have _Raw Rear Grip_ (we have it only for the front).

Shoudl we have a plot for _avg longitudinal patch vel _? 

Shoudl we have a plot for _long and lat patch vel_ for rear?

We should also add tootlips on the names / titles of plots, with a description of what the componet is / does / or is used for.

We don't have a plot for driver steering input (steering wheel angle). We should add it.


we should combine throttle and brake input plots into a single plot. We should distinguis them with different colors (eg. use red and green colors as it is common for these values).



Consider if other plots might benefit merging, showing multiple values (formula components) on a single plot.
When we have multiple variables in a plot, use different colors in the title text to show which color is assigned to.

Organize the plots in 3 columns instead of 2, to fit more plots vertically in the window, which is already a tower taking all vertical space.

add preset: all effects disabled
makes it easier to then go and enable one single effect
### Other 

We could also have additional plots in which we show the plot for the base value (using default coeafficients) of some components and one with the custom coefficients as set in the main GUi

docs/dev_docs/prompt - Implement Numerical Readouts for Troubleshooting Graphs (Diagnostics).md

docs/dev_docs/Plots with Modular Independent Windows .md

look at the updated math formulas
see which cmponents are affected by new grip and load calc
add present for no effects
add preset only for understeer, only for oversteer, only for self aligning torque
note that SoP might be incorrect (always zero)
currently I am unable to feel oversteer effect
try to isolate that
also understeer one could be better.. need to understand how it supposed to work in terms of feel, an "tune in" to that signal to adjust driving
but currently it does not seem very dynamic..

TODO: there is no logging to files of the telemetry stats
See: docs/dev_docs/design proposal for a High-Performance Asynchronous Telemetry Logger.md
See: docs/dev_docs/log analysis tool design.md
create a logs folder from the program, and save a file there. Save as soon as the driver starts driving and speed above minimum. After speed below that, close a session
and resume it in the same file when speed increases again. One file per execution of the program.
See if in the shared memtmory we have more precise info on when driver in menu, in session but in session menus, and when driving; in case, use those to organize the logs.
Logs should include the values we record from game. And possibly others like car speed, wheel position, throttle, brake pedal, etc.


later would be test on lockup feel (early detection)
and slip from acceleration

about sliding: is it the same as ..oversteer?
distinct effect of sliding at the limit, holding a car at the limit, with a bit of slide?
how should that feel on the wheel?

then there is that issue that I was having of delayed spikes in the wheel:
I would have a spin or significant slide on the wheels, and a 1-2 seconds, or maybe more, after it was done I would get a spike in the wheel, with the wheel moving a few degrees and then stopping and ffb becoming very strong..
as if the tires are overheated and any further use causes this phenomenon..
but just a few seconds after a spin, and once, then no more

other update: the console still does not say if the wheel was acquired exclusively or not
it say "exclusive | .." as if the request to acquire specifies both. We need to edit this.




Reorganize the GUI customizations. Have one section per effect and comonent of the FFB formuls. For instance, add one section for Self Aligning Torque.
Each section for that component / effect should have a main setting / slider, and additional things like smoothing, caps, coefficients, etc.
Group all the SoP settings into one section.

Test the new formulas and plots with LMU (and later rf2)

add a toggle in the gui to select which grip formula to use. If the new approximated one is too inaccurate or ununstable, we want to be able to stick with the other and the fallback mechanisms / values that disable some of the effects (but at least the overall FFB is stable).





Test brake loockup effect. 
Consider both "canned" effects and dynamic, physically determined ones.
Consider which effects are not possible without grip and load (currently missing) and if they can be temporarily replaced with "canned" effects, or approximations, or other workarounds / temporary replacements.

reintroduce support for rF2, so we can test full effects with grip and load there.
keep support for both apps, reuse code. Add conversion of scales etc. when using rF2, because default support should be LMU, and we don't want to mess the code for LMU.
Do a plan of what needs to be changed to support rF2.


for rF2, could have a separate executable, to avoid cluttering the GUi for LMU

can we introduce an equalizer?
like in the moza software?
eg. in order to increase intensity of lower frequency effects, that are related to grip.


open bug in forum for grip and load values always 0

move invert ffb checkbox near the top of gui window (now is at bottom).

when FFB is inverted, also invert the plots for base force and steering arm torque (so that they look identical to the total torque when additional effects are disabled). Consider if also the other plots need inverting.

TEST: ? app not respond to driver setting max force 20 %

TEST: ? if alt tab goes to opp lock of wheel limiter

direct method seems ok
consider removing vjoy and gremlin. Disable warnings. Confirm they are not needed, then remove, from instructions and code.

## Troubleshooting 20

remove vJoy warning popup at startup if dll not present
update readme: remove vJoy, we don't need it
remove vJoy from code
also remove all mentions (in readme) and use (in code) of gremlin.

in main window gui: rename "Use Manual Slip Calc" to "Use Manual Front Slip Ratio Calc"

in FFB Analysis window:
Have both blots
"Calc Front Slip Ratio" and "Game Front Slip Ratio"
"Game Front Slip Ratio" should go into subsection "C. Raw Game Telemetry (Inputs)"
Add a plot for our _manual slip calc_ 
Add a plots both for raw game value for slip, and for _manual slip calc_ , so we can compare them and see if they are identical or at least similar in shape.
compare "Calc Front Slip Ratio" and "Game Front Slip Ratio" to see how accurate the manual calc is.

test lmuFFB with rF2, with the cars that support giving out grip information and other channels that are blocked for DLC cars.
See, for this "open" ffb cars, if our fomulas that approximate grip, load, and other values are accurate enough.

Deep research: AC FFB that make it informative.

Coaching: how to feel each specific effect. Find car, setup, and track combinations (particular corners), that are ideal to feel a particular effect.
Get instructions in what to do while driving to induce a specific car physics dynamic that produces a certain effect on the FFB.
At the same time, use the settings that most highligh a particular effect (isolating it from other effect whenever possible).
Make this guide suitable for Belt driven wheels (eg. T300) and gear driven wheels (eg. G29).

Reorganize the sliders in the main window gui. They should be grouped by component of the FFB formula, and each component should have its own section, with a main slider, and additional settings like smoothing, caps, coefficients, etc.

This would make it more intuitive, and easier to find the right slider to adjust.

Review again the options in the main window, and verify that we have a settings to adjust and isolate each component of the FFB formulas.

Test strange "delayed" spikes in the wheel, after a spin or slide. Which effect is causing them?


we don't have anymore the plot for _avg deflection_ (I think it was removed when we reorganized the plots to group them in more intuitive way)
avg deflection: is it for all 4 tires? Specify in the variable name and / or plot title.


add the settings for which slip angle is ideal for grip. Now it is 0.10, but it could be lower. There is a doc for this. docs\dev_docs\grip_calculation_and_slip_angle_v0.4.12.md

#### Other
Notes on FFB math formulas and plot visualization

"formulas md doc" refers to this document: docs\dev_docs\FFB_formulas.md

Visualiza this component that is in the formula md doc but has no individual plot: **slip angle LPF**, smoothed with exp.moving avg
also, is "slip angle LPF" the slip angle on fronts only? specify in variable names

In plots we indeed have "smoothed slip angle"
but we don't show the actual slip, which is arctan2(Vlat,Vlong). Show also the slip before smoothing.
I notice we have in the plots "**calc slip ratio**": is this it? In any case, we also this must specify for this plot if it is front wheels only.

I note int he formulas that Grip is used for understeer.

About this Math formula: **AccellXlocal**: rename to clearify: is it lateral accel? Is it of whole car, or average of some tires? Which tires? specify in name.

Consider **SoP_base**  (without oversteer component) from the formulas doc: do we show it in a separate plot? If not, we should.

In general, if we have a plot for a formula component (or a raw value) that is based on the front wheels only, we should also have a plot for the rear wheels only version of that component.

In Raw plots, we dont have _LatForceRl and LatForceRR_

Why don't we have a plot for slide of rear tires?

Should we have a slip angle plot for rear tires? (or does it make sense only for turning/steering wheels, that is front wheels?)

_avg longitudinal patch vel _: Should we use that value for brake lockup and slip calculation (or do we do already)?

Investigate bug: "would have **sudden pulls/loss of FFB off centre at random times. Almost like the “reverse FFB” setting was working intermittently**. Obviously had my wheel base settings turned well down. Had a complete lock up on my wheel base fully pulled to the left too (bit like the old days of LMU in game FFB!). Has to restart the wheel base."

