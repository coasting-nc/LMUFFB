# LMUFFB To-Do List

## Completed Features
- [x] **Core FFB Engine**: C++ implementation of Grip Modulation, SoP, and Texture effects.
- [x] **GUI**: Dear ImGui integration with sliders and toggles.
- [x] **Config Persistence**: Save/Load settings to `config.ini`.
- [x] **Installer**: Inno Setup script for automated installation.
- [x] **Documentation**: Comprehensive guides for Architecture, Customization, and Licensing.

## Immediate Tasks
- [ ] **DirectInput Support**: Replace vJoy with native DirectInput "Constant Force" packets.
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

# Throubleshooting 2

Add a new, comprehensive markdown document discussing and planning how can we make the app more easy to debug and troubleshoot. In particular, for cases in which the FFB is not working as intended (eg. forces too strong, jerking, etc.), for be able to more easily identify the cause of the problem. Consider also adding display widgets that show in the GUI more info and visuals on what the FFB is doing, including having separated visual for each effect or formula, so we can see which one is "exploding" with high values, or oscillating, or having other strange behavior.
This could also be in the form of a "trail" visualization, in that we show the trails of multiple values over time (eg. over the last 5-10 seconds), so we can see if there are any patterns or anomalies.