FFB-HUB - Telemetry-Based Force Feedback App for Le Mans Ultimate and RaceRoom
==============================================================================

DOWNLOAD
--------

https://github.com/coasting-nc/LMUFFB/releases


INSTALLATION & CONFIGURATION
---------------------------------------

For both LMU and RaceRoom:

1. Extract the zip to a folder where you can write files, such as a folder in Downloads. Do not use Program Files.
2. Start FFBHUB.exe
3. Select your wheel under FFB Device.
4. Choose LMU or RaceRoom using the Game selector at the top of the app.
5. Run the game in Borderless Mode or VR (you can run the game before or after the app, it does not make any difference). 
6. In RaceRoom, turn off FFB.
7. For LMU, turn off “Use Constant Steering Force Effect” (to prevent the game from trying to take control of the wheel and send FFB), and set FFB Strength to 100% (the maximum) which allows the app to properly scale the wheel strength when using **In-Game FFB** option as the "Torque Source" in FFB-HUB.

When not in VR, make sure the game **Display Mode** is **Borderless** (Prevents crashes/minimizing).

Help us improve the app
------------------------

1. Report issues - For bugs and technical issues, opening a GitHub issue is MUCH BETTER than 
   posting on the forum. 
   GitHub issues: https://github.com/coasting-nc/LMUFFB/issues

2. For LMU, post screenshots, thoughts and settings to the LMU Forum Thread:
   https://community.lemansultimate.com/index.php?threads/lmuffb-app.10440/



TROUBLESHOOTING (LMU)
----------------------

I am getting excessive oscillations while on straights
 - Try setting "Smarts Hands-off Protection" to 10% - 20 % or more (it should 
 work on GT3s, but might not for LMP2s and Hypercars)
 - Increase at at least 1.0 the "Threshold" slider for "Rotation Kick" and "Power Kick" (if they are enabled).

I am getting too much vibrations while braking
 - Reduce "Lockup Strength" of the "Lockup Vibration" effect (eg. down to 20%).
 - Increase Steering Smoothing ("Front Axle" tab) to 8 ms.
 - Also adjust the other settings of the "Lockup Vibration" effect (but you might need different setting by car class):
   - Curve (increase this to make the effect start weaker and more progressively)
   - Start slip (increase this to delay the start of the effect, requiring more tire slip to trigger)
   - Full slip (increase this to make the effect reach its maximum strength later)

Unable to save settings or config.ini errors:
  - This usually happens if you placed the FFB-HUB app or folder in a location 
    without write permissions (e.g., inside C:\Program Files)
  - Solution: Keep the entire FFB-HUB app folder to a location where the app can have 
  full write access, such as a subfolder in your Downloads folder
  
Inverted FFB (Force pushes away from center):
  - If the FFB feels "backwards" or "inverted" while driving (wheel pushes 
    away from center instead of pulling toward it), check the "Invert FFB" 
    checkbox in the FFB-HUB app GUI
  - This reverses the force direction to match your wheel's expected behavior

No FFB:
  - Ensure the "FFB Device" in the FFB-HUB app is your real wheel
  - Check if Connected to Le Mans Ultimate" appear in the GUI
  - Verify the FFB strength in your device driver is not too low
  - Increase Master Gain or reduce Car Strength
  - Check the FFB-HUB app console for errors

FFB too weak:
  - Increase Master Gain
  - Or reduce Car Strength (this amplifies the signal by setting a lower reference point for maximum torque)

Strange pull in some scenarios:
  - If you feel a strange pull in specific driving situations, try reducing the Rear Align Torque
  
The app has too many options and it's confusing:
  - A future version will include a "Basic Mode" with auto adjustment and defaults for most settings.
  - We need to first determine which settings work best for most users and wheel types (Direct Drive vs Belt/Gear).
  - Please leave feedback on the forum thread telling us what settings work best for you:
    https://community.lemansultimate.com/index.php?threads/lmuffb-app.10440/

Any other issue:
  - PREFERRED: Open a GitHub Issue: https://github.com/coasting-nc/LMUFFB/issues
    This is the best way to ensure your issue is tracked and addressed. 
    Our AI agents periodically scan and attempt to fix GitHub issues.
  - Alternatively, post in the thread in the LMU forum, but be aware that 
    forum posts are harder to track:
    https://community.lemansultimate.com/index.php?threads/lmuffb-app.10440/

CLARIFICATION ABOUT THE INTERACTION BETWEEN IN-GAME FFB (from LMU) AND the FFB from the App
-------------------------------------------------------------------------------------------

The app takes exclusive control of the FFB sent to your wheelbase, which means that it is the only program 
sending force feedback to your wheelbase. 
This is required by DirectInput, and it is the reason why the app displays "Game FFB is blocked."

However, among the many telemetry channels that it uses, the FFB-HUB app includes a 
specific telemetry channel called "In-Game FFB", which allows you to still 
include the LMU original FFB signal within the final FFB that the app produces.

How it Works:

The "In-Game FFB" stream is the FFB produced by LMU. It is mostly based on the 
Steering Rack Torque (which is why in the app you can select it as one of the 
two possible steering torque sources), plus any additional effect and 
processing that LMU might be doing on it behind the scenes (eg. adding 
damping, smoothing, vibrotactile effects, etc.).

This stream is impacted by any setting that you might change from the game FFB Settings (see below).

You can control the impact of the In-Game FFB on the final FFB produced by 
the FFB-HUB app in several ways:

- You can enable or disable it: by selecting "In-Game FFB" as your Steer Torque 
  Source in the app, it will be combined with all other telemetry-based forces 
  processed by the FFB-HUB app to produce the final FFB you feel.

- Game Settings: Any changes you make to LMU's internal FFB settings 
  (strength, smoothing, vibrotactile effects, etc.) should directly affect 
  what the FFB-HUB app receives through this channel.

- In App Gain slider: The "In-Game FFB Gain" slider in the FFB-HUB app acts as an 
  additional multiplier to change the strength of this specific channel.

In-Game FFB vs. Shaft Torque (Legacy):

In the App ("Front Axle" tab), you have two options for your base steering torque ("Steer Torque Source"):

1. In-Game FFB:
   - Pros: The most detailed choice. It includes LMU's own processing 
     (e.g., smoothing) and special effects (e.g., vibrations).
   - Cons: If you don't like the game built-in processing.

2. Shaft Torque (Legacy):
   - Pros: A "pure" source of steering rack torque information without 
     game-side processing.
   - Cons: Less detailed, and may require gain adjustments via the 
     "Steering Shaft Gain" slider, because it might have a very different 
     baseline strength compared to In-Game FFB.


FAQ
---

Q: Does the app constitute an unfair advantage ("cheating")?

A: No. Multiple wheelbase manufacturers already provide or are implementing 
telemetry-based force feedback (including VNM, Simucube, and Simagic). 
By the same logic, those would constitute an "unfair advantage" too.

Additionally, bassshakers/transducers, active pedals, motion platforms, and 
other simracing equipment also give similar additional telemetry-based 
information (tire lockup, wheelspin, car behavior, etc.), and are not 
considered an unfair advantage either.

The app allows everybody to have the option of having additional 
telemetry-based feedback information, not just the owners of expensive 
equipment or particular wheelbases.


Q: Does the app have an option to remove the feeling of flatspots vibrations 
from FFB? Is that an unfair advantage?

A: No. The app had a "Notch Filter" option that is also present in Simucube 
wheelbase software (and possibly other wheelbases), so that is a legitimate 
feature. The feature was supposed to reduce specific user-defined frequencies 
from the FFB signal, like in the Simucube software. It was improperly named 
"Flatspot Suppression", but it could attenuate any frequency, like in other 
wheelbase software. Early testing suggested it was not actually doing anything 
(it wasn't working). The feature has in any case been removed in recent 
versions of the app, since it was a legacy effect that was not working and was 
not really necessary for the app.


Q: How is the app getting the telemetry data from the game?

A: The app is using the official LMU Shared Memory Interface that was 
introduced with LMU 1.2 in December 2025.


UPCOMING FEATURES
-----------------

* Basic mode with only main sliders and auto adjustments of more advanced 
  settings
* Wet Weather Haptics (Hydro-Grain): vibration cues telling when the tires 
  are on dry or wet surface, and if there is grip or not. This tries to 
  replicate some the particular wet track grip and dynamics that can be felt in 
  real life cars.
* Support for other Sims (AC Evo, RaceRoom, AC, ACC, rF2).


FEEDBACK & SUPPORT
------------------

For feedback, questions, or support:
- GitHub Issues (RECOMMENDED for bugs): 
  https://github.com/coasting-nc/LMUFFB/issues
  GitHub issues are easier to track and are periodically tackled by AI development agents.
- LMU Forum Thread for the FFB-HUB app: 
  https://community.lemansultimate.com/index.php?threads/lmuffb-app.10440/
  Best for general discussion and sharing settings.


For full documentation, advanced settings, and tuning tips, 
see README.md or visit:
- FFB Tuning Recommendations: https://github.com/coasting-nc/LMUFFB/blob/main/docs/user_guides/FFB%20Tuning%20Recommendations.md
- Main Project Page: https://github.com/coasting-nc/LMUFFB



VIDEOS, REVIEWS, AND TUTORIALS
------------------------------

For the LMU version:

* UKOG: https://www.youtube.com/watch?v=4OVys-06StM
* Gamer Muscle: https://www.youtube.com/watch?v=CQJRmhMaoPU
* Step-by-step guide by Dalking on how to tune the app: https://www.youtube.com/watch?v=qAvCqoPBkys

To find and share app profiles, see:
* Dalking LMUFFB / FFB-HUB profile, developed for a 18 Nm VNM wheelbase, from his discord: https://discord.gg/xfwJwUa
* UKOG discord, which has a section with user profiles and discussions for specific wheelbases: https://discord.gg/ukog
* Gamer Muscle Patreon (free): from which you can download GM profiles for GT3s, LMP2, Hypercars, and a specific profile for lower Nm wheelbases (4 Nm): https://www.patreon.com/posts/lmuffb-app-gamer-155277185


===============================================================================
RFACTOR 2 COMPATIBILITY
===============================================================================

Note: rFactor 2 is NOT SUPPORTED in v0.4.0+. For rFactor 2, please use 
earlier versions of lmuFFB (v0.3.x). See the releases page for v0.3.x 
downloads:
https://github.com/coasting-nc/LMUFFB/releases

rF2 will be supported again in a future version, in particular to compare 
our grip approximation algorithm with the real values returned by the game 
for non DLC cars.

===============================================================================
SUPPORT FOR OTHER SIMS
===============================================================================
 
Support for AC, ACC, ACE is planned for a future release. See https://github.com/coasting-nc/LMUFFB/issues/30

LMU PROFILE UPGRADES
--------------------
When upgrading from an older version, keep legacy_lmu_profiles.toml next to
the executable. It lets FFB-HUB preserve a selected or car-bound DD (Yaw),
T300 (Yaw), or V0.7.66 Raw profile as a user copy. Any edits in your current
settings are kept. Fresh installations do not use these retired profiles.
