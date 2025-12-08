lmuFFB - Le Mans Ultimate Force Feedback
========================================
Version: 0.3.7

See README.md for full documentation with images and links.


===============================================================================
!!!                    CRITICAL SAFETY WARNING                             !!!
===============================================================================

BEFORE USING THIS APPLICATION, YOU MUST CONFIGURE YOUR STEERING WHEEL DEVICE 
DRIVER:

This is an EXPERIMENTAL EARLY ALPHA VERSION of a force feedback application. 
The FFB formulas are still being refined and MAY PRODUCE STRONG FORCE SPIKES 
AND OSCILLATIONS that could be dangerous or damage your equipment.

REQUIRED SAFETY STEPS:

1. Open your wheelbase/steering wheel device driver configurator 
   (e.g., Simucube TrueDrive, Fanatec Control Panel, Moza Pit House, etc.)

2. Set the Maximum Strength/Torque to a LOW value:
   - For Direct Drive Wheelbases: Set to 10% OR LOWER of maximum torque
   - For Belt/Gear-Driven Wheels: Set to 20-30% of maximum strength

3. Test gradually: Start with even lower values and increase slowly while 
   monitoring for unexpected behavior

4. Stay alert: Be prepared to immediately disable FFB if you experience 
   violent oscillations or unexpected forces

WHY THIS IS CRITICAL:
- The FFB algorithms are under active development and may generate unexpected 
  force spikes
- Unrefined formulas can cause dangerous oscillations, especially on 
  high-torque direct drive systems
- Your safety and equipment protection depend on having a hardware-level 
  force limiter in place

DO NOT SKIP THIS STEP. No software-level safety can replace proper hardware 
configuration.

===============================================================================


IMPORTANT NOTES
---------------

CURRENT LMU LIMITATIONS (v0.3.20):

Limited Telemetry Access:
Due to limitations in Le Mans Ultimate versions up to 1.1, lmuFFB can currently 
only read LATERAL G ACCELERATION from the game (used for the Seat of Pants effect). 
Other FFB effects that rely on tire data (such as Tire Load, Grip Fraction, etc.) 
are NOT AVAILABLE because LMU does not yet output this telemetry.

LMU 1.2 Update Coming:
With the upcoming release of LMU 1.2 (on December 9th, 2025), the game will officially support a new 
shared memory format that is expected to include tire telemetry data. This will 
unlock the full range of FFB effects. However, lmuFFB will require an update to 
support this new format - the current version (0.3.20) does not yet support 
LMU 1.2's shared memory.

rFactor 2 Compatibility:
lmuFFB may work with rFactor 2 out of the box using the same installation 
instructions, as both games share the same underlying engine and telemetry system. 
However, rFactor 2 support is not officially tested or guaranteed.


PREREQUISITES
-------------

1. rF2 Shared Memory Plugin (rFactor2SharedMemoryMapPlugin64.dll)
   Download from:
   - https://github.com/TheIronWolfModding/rF2SharedMemoryMapPlugin#download
   - Or bundled with CrewChief: https://thecrewchief.org/
   
   Installation Steps:
   - Place the rFactor2SharedMemoryMapPlugin64.dll file into "Le Mans Ultimate/Plugins/" 
     directory
   - NOTE: The "Plugins" folder may not exist by default - create it manually if needed
   - TIP: If you've installed apps like CrewChief, this plugin might already be present. 
     Check before downloading.
   
   Activation:
   - LMU requires manual activation: Open "Le Mans Ultimate\UserData\player\
     CustomPluginVariables.JSON" and set the " Enabled" field to 1 for the plugin entry
   - IMPORTANT: Restart LMU completely after making this change
   - If the plugin entry is missing: Install the Visual C++ 2013 (VC12) runtime from 
     your game's "Support\Runtimes" folder, then restart LMU to auto-generate the entry

2. vJoy Driver (Version 2.1.9.1 recommended)
   Download: https://github.com/jshafer817/vJoy/releases
   
   Why vJoy? LMU needs a "dummy" device to bind steering to, so it doesn't 
   try to send its own FFB to your real wheel while lmuFFB is controlling it.
   
   Configuration:
   - Open "Configure vJoy" tool
   - Set up Device 1 with at least X Axis enabled
   - Disable all vJoy FFB Effects EXCEPT "Constant Force"
   - Click Apply

3. vJoyInterface.dll (Required for lmuFFB to run)
   This DLL MUST be in the same folder as LMUFFB.exe
   
   Download from:
   - C:\Program Files\vJoy\SDK\lib\amd64\ (if you installed vJoy SDK)
   - https://github.com/shauleiz/vJoy/tree/master/SDK/lib/amd64/vJoyInterface.dll


STEP-BY-STEP SETUP
------------------

A. Configure Le Mans Ultimate (LMU)
   1. Start LMU
   2. Go to Settings > Graphics:
      - Set Display Mode to "Borderless" (Prevents crashes/minimizing)
   
   3. Go to Controls / Bindings
   4. Steering Axis - Choose ONE method:
      
      METHOD A (Direct - Recommended):
      Step-by-Step Setup:
      1. Clean Slate: Close Joystick Gremlin and vJoy Feeder if running
      2. Game Setup:
         - Start Le Mans Ultimate
         - Bind Steering directly to your Physical Wheel 
           (e.g., Simucube, Fanatec, Moza, Logitech)
         - IMPORTANT: Set In-Game FFB Strength to 0% (or "Off")
      3. App Setup:
         - Start LMUFFB
         - CRITICAL STEP: In the LMUFFB window, find the "FFB Device" dropdown
         - Click it and select your Physical Wheel from the list
         - Note: You MUST do this manually. If it says "Select Device...", 
           the app is calculating forces but sending them nowhere
      4. Verify:
         - Check the console for errors. If you select your wheel and do NOT 
           see a red error like "[DI] Failed to acquire", then it is connected!
         - Drive the car. You should feel the physics-based FFB
      
      Troubleshooting - No FFB:
      - Check Console Messages: While driving, look for "[DI Warning] Device 
        unavailable" repeating in the console
        * YES, I see the warning: The game has 'Locked' your wheel in Exclusive 
          Mode. You cannot use the Direct Method. You must use Method B (vJoy Bridge)
        * NO, console is clean: The game might be overwriting the signal. Try 
          Alt-Tabbing out of the game. If FFB suddenly kicks in when the game 
          is in the background, it confirms the game is interfering. Use Method B
      - Try Adjusting Settings: If you feel no FFB, try tweaking these values:
        * Master Gain: Increase from 0.5 to 1.0 or higher
        * SOP (Seat of Pants): Increase from 0.0 to 0.3 (feel lateral forces)
        * Understeer Effect: Ensure it's at 1.0 (default)
      
      Pros: Simplest setup. No vJoy required
      Cons: If wheel has no FFB, try Method B
      
      METHOD B (vJoy Bridge - Compatibility):
      - Bind to "vJoy Device (Axis Y)"
      - IMPORTANT: You MUST use "Joystick Gremlin" to map your wheel to vJoy Axis Y.
      - Why? Guarantees separation of FFB and Input.
   
   5. Force Feedback Settings:
      - Type: Set to "None" (if available) or reduce FFB Strength to 0% / Off
      - Note: LMU may not have a "None" option; reducing strength to 0 is 
        the workaround.
      - Effects: Set "Force Feedback Effects" to Off
      - Smoothing: Set to 0 (Raw) for fastest telemetry updates
      - Advanced: Set Collision Strength and Torque Sensitivity to 0%
      - Tweaks: Disable "Use Constant Steering Force Effect"

B. Configure lmuFFB (The App)
   1. Run LMUFFB.exe
   2. FFB Device: Select your Physical Wheel (e.g., "Simucube 2 Pro", "Fanatec DD1")
      - Do NOT select the vJoy device here
   3. Master Gain: Start low (0.5) and increase gradually
   4. SOP Effect: Start at 0.0
      - This effect injects lateral G-force
      - On High-Torque wheels (DD), this can cause violent oscillation if too high
   5. Drive! You should feel force feedback generated by the app


TROUBLESHOOTING
---------------

Wheel Jerking / Fighting:
  - You likely have a "Double FFB" conflict
  - Ensure LMU Steering is bound to vJoy, NOT your real wheel
  - Ensure LMU FFB is sending to vJoy (or disabled/strength at 0)
  - If the wheel oscillates on straights, reduce SOP Effect to 0.0

No Steering (Car won't turn):
  - If using Method B (vJoy), you need Joystick Gremlin running
  - Ensure it maps your wheel to vJoy Axis Y
  - The "vJoy Demo Feeder" is for testing only, not driving

No FFB:
  - Ensure "FFB Device" in lmuFFB is your real wheel
  - Check if Shared Memory Plugin is working (console should show "Connected")
  - Verify the plugin DLL is in Le Mans Ultimate/Plugins/ folder

"vJoyInterface.dll not found":
  - Copy vJoyInterface.dll to the same folder as LMUFFB.exe
  - Get it from: C:\Program Files\vJoy\SDK\lib\amd64\
  - Or download: https://github.com/shauleiz/vJoy/tree/master/SDK/lib/amd64/vJoyInterface.dll

"Could not open file mapping object":
  - Start LMU and load a track first
  - The Shared Memory Plugin only activates when driving


KNOWN ISSUES (v0.3.7)
---------------------
- Telemetry Gaps: Some users report missing telemetry for Dashboard apps 
  (ERS, Temps). lmuFFB has robust fallbacks, but if Tire Temperature data 
  is broken, the Understeer effect may be static.


FEEDBACK & SUPPORT
------------------
For feedback, questions, or support:
- LMU Forum Thread: 
  https://community.lemansultimate.com/index.php?threads/irffb-for-lmu-lmuffb.10440/
- GitHub Issues: 
  https://github.com/coasting-nc/LMUFFB/issues


For full documentation, advanced settings, and developer information, 
see README.md or visit: https://github.com/coasting-nc/LMUFFB
