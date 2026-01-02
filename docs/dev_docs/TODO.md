# LMUFFB To-Do List


## Troubleshooting 24

DONE: delay from the app due to SoP and slip angle smoothing
DONE: mask the flatspot vibration
DONE:yaw kick cutoffs by speed and force.

User report
"potentially leave the ingame FFB as is and add the SoP effects ontop? (...) for DD users this could be an amazing feature. I tested it with VRS DFP Pro"
if it is not possible to have both FFB coming from game and lmuFFB app, implement it by using the steering shaft torque as a surrogate of the game FFB (which is probably what the game FFB is based on). Try if we can remove the annoying "vibration" from this signal with an added smoothing filter, or other type of filter. This might reduce detail and get rid of the vibration. In fact, the game FFB seems less detailed than the raw steering shaft torque , and the reason might be an added smoothing filter.


loses wheel or connection to game when app not in focus
(make the app always on top? auto reconnect game / rebind wheel when disconnected? other mechanisms to avoid these disconnections?)



improvements to the formulas

---


DONE: remove warnings about missing telemetry data

DONE: add slider for optimal slip angle (and slip ratio)
DONE: list effects affected by grip and load approximation, and list those that are not affected

DONE: add smoothing (and slider) for steering shaft torque
DONE: expose sliders for additional smoothing: yaw kick, gyroscopic damping, Chassis Inertia (Load)
DONE: lockup vibration improvements
DONE: Add more tooltips (many sliders are missing them). update existing tooltips
DONE: add console prints for missing telemetry data, including mLateralForce 
Done: in the GUI, rename "SoP Lateral G" to "lateral G", and "Real Align Torque" to "SoP Self-Aligning Torque" 

### DONE: Signal Processing
This is ready to implement (the latency stuff has been moved to another doc).
* [Report: Signal Processing & Latency Optimization](report_signal_processing_latency.md)

* Static noise / notch filter: add a range or width slider: how many frequency to the left and right of the center frequency should we also suppress.
    * set the default blocked center frequency to 10-12 Hz, which as reported by user Neebula is the correct one to block the baseline vibration. Set the default range accordingly, to block from 10 to 12 Hz. (so center frequency is 11 Hz?) 
    Note: user Neebula reports that the current notch filter set at 10-12 did not seem to  take away any important feedback (no difference found), it just resulted in much smoother ffb ("from the short testing i did i couldnt find a difference except much smoother ffb"). However, some noise was still remaining.
* add a slider for the yaw kick thresshold to determine at which acceleration or force the yaw kick effect starts to be applied. There is still too much noise in the signal, and it does not actually work when needed (feeling a kick for the rear starting to step out).


### DONE: Effect Tuning & Slider Range Expansion
This is ready to implement.
* [Report: Effect Tuning & Slider Range Expansion](report_effect_tuning_slider_ranges.md)

expand the range of some sliders that currently are maxed out or minimized, to give more range to feel the effects when too weak. 
Inlcude: 
* Understeer Effect (what "measure unit" is this value?) Increase the max from 50 to 200.
* Steering Shft Gain: increase max from 100% to 200%.
* ABS Pulse (increase max gain from 2.0 to 10.0)
* Lockup response Gamma: min from 0.1 instead of 0.5
* Lockup strength up to 300%
* brake load cap: up to 10x
* Lockup prediction sensitivity: min down to 10 (from 20). Also add what unit is this.
* Lockup prediction Rear boost: max up to 10x (from 3x)
* Yaw Kick: max at 100% (which is already way too much), down from 200%
* Lateral G Boost (Slide): max up to 400% (from 200%)
* (Bracking and lockup effects: implement wider ranges in the sliders)

* For all vibration effects, add a slider to select the frequency of vibration (if fixed).

* remove "Manual Slip Calc", not needed, all physics info there to get slip info.

DONE: **Frequency Tuning Guide**: Created comprehensive user guide at `docs/user_guides/frequency_tuning_guide.md`:
   - Explains resonance and hardware characteristics
   - Provides starting points for different wheel types (Belt-Driven, Gear-Driven, Direct-Drive)
   - Includes step-by-step tuning methodology
   - Real-world examples and troubleshooting

## Troubleshooting 25

DONE: fix persistent vibrations at low speed. Apply muting and smoothing to rear align torque, all of SoP / oversteer effects, and to base game force.

DONE: Check each slider and setting in the main GUI window, and verify that it is saved to the ini file (both as a "saved preset" and as a saved current configuration).
DONE: Implement auto-save of the settings as current configuration every time the user makes a change to any of the sliders or inputs.
See: docs\dev_docs\autosave_implementation_plan.md
Split into a separate report only the features that we want:

DONE: Auto-Save
auto save last configuration. Is the save config button any longer needed?

"Manual testing current effects, new presets and default values": see section below
test back to back from LMP2 to GT3. Do the new T300 preset for the LMP2 work for the GT3? Try again the understeer effect: in the LMP3 is too sensitive (even 0.84 is too much), in the GT3?
Is the optimal slip angle setting affecting this? Note it should be set to 0.6 for LMP2 and 1.0 for GT3.

yaw kick findings: belos a certain threshold, it is transmitting road texture. Exploit this fact for a separate effect, that would work only below (instead of above) a certain threshold. 
See which component of the current yaw kick effect is causing this. Local rotation / acceleration? What if we use also the other degrees of freedom other than yaw? That is pitch and roll? All combined for a "texture" effect. Or as separate effects.
Review again the distinction betwen yaw kick and self aligning torque: compare the two formulas.

Self aligning torque: rename also the plot. Rename all plots to match the settings name in the main GUI window.

next effects to add:
* longitudinal weight / load changes under bracking and acceleration.
* grip loss effect different calculation, using derivative. Keep exiting method too, but add a drop down to select which method to use.
* option to "invert" mechanism for understerr grip loss: wheel gets heavier instead of lighter. Consider inverting the "weight" feel of also some of the oversteer effects (eg. wheel gets heaverier instead of lighter, and viceversa).

Compare formulas with those used in Marvin's app.
Checklists for completedness for LMU before porting to other sims:
* stable settings, found optimal settings for each car (GT3, LMP2, LMP3, HY, GTE) and wheel type (gear, belt, direct drive, direct drive >20 bit encoder).
* autotune option, ..based on the car. Should adjust to optimize the use of dynamic range.
* basic mode implemented
* all effects sorted out, clarified the optimal window for each to work. Clarified how to "decouple" and multiply them appropriately based on each car forces, weight, and other properties.

Next sims to implement (drop down menu to choose game):
* rf2
* ACE
* AC1
* ACC
* AMS2
* Raceroom
* AMS1
* AC rally?
* PMR?
* Rennsport?
* LFS?
* F1 series?
Make it so that the FFB is standardized, so we can do direct comparisons between sims.

implement fixed based on users reports.
fix: " Slide Rumble, is a just noise and better left off or very very low but its an almost constant effect even driving straight."
fix: "Scrub drag, does not work unless Road details is enabled."
fix: "road texture, no matter how high i set it cant really feel the road at all"
fix: "Bottoming Logic does not work at all. I dropped the car too the floor and tried everything but never anything shows up."
fix: " I am trying to adjust the filters, so I can feel the point at which the car starts to understeer through vibrations in the FFB. I have been experimenting with the slide scrub and understeer values, but I am currently unable to feel the exact point when the (LMP) car starts to understeer through clear vibration or weight changes. There are plenty of vibrations in general, but they seem random or connected to road noise. Any advice?" (user Vito)

in the readmes, troubleshooting section, add an entry for game baseline vibrations, and which settings can be tweaked to reduce/remove them (steering rack smoothing, static notch filter, dynamic notch filter).

check max torque ref: why 100 Nm seems to be ideal? Is some scale off? What it means that it represents the peak torque of the car? 100 Nm is still too much. Which aspects of the car can produce 100 Nm? The steering rack is max about 25 Nm even in indy cars. Also try if we can keep it to a normal value, and adjust other settings, like main gain.

(add more variations of current tests, for a broader , more rdiverse range of settings. Eg. include the current default settings).

additionally, in the ini file, the settings should be saved in the same order as they are displayed in the GUI. This makes it easier to find them in the ini, and to compare the ini with the Gui.


User feedback: "Lateral G, Lateral G Boost, SoP Self-Align and SoP Scale: These settings really dictate the weight of the wheel and how much you can feel the car do what it does, getting a good balance here makes all the difference together with 0.0rads Yawkick." Find a good balance in default settings. See if there are optimal "ratio"s between these settings, so that in auto tune mode, we adjust a single slider and keep these settings in balance.

Yaw Kick: new reports confirm that this need to be split in 2:  (1) A setting used for road or genera FFB details, with low rad/s^2. Also see if other acceleration physics data can be used as a source of road / ffb details. (2) a settings for yaw kick proper, above a certain speed in rad/s^2. This still needs investigation to see if it is implemented properly, if it is actually any useful.

user report: "Yaw kick: I'm not sure what the intention of Yaw Kick is.. from my experience there's no need effects such as a 'kick' for under steer because you already feel it through the ffb. Thats why ffb works best with rad/s = 0 and YawKick around 8Nm which sends full signal through to ffb.
* If disabled, you cannot feel the car hitting a bump eg, drive over a curb you cannot feel the initial impact of suspension compressing. You only get the feeling from the car landing through the base SoP.
* If enabled, but activation threshold on 0.0 rads you feel all ffb as intended(y)
* If enabled, with rads set higher then it feels ffb is missing (like disabled) until rad/s threshold is crossed. The kicker doesnt feel good at all, or perhaps specifically with direct drive ffb."

Slide Rumble: investigate and fix this effect: user report "should not be used, it is constant vibration which makes takes away from all other ffb more important ffb."

investigate: "Road Details: Even after setting a manual input of like 100Nm with a Texture cap of 50x, it does not add anything."

investigate: "Scrub Drag: works if Road details enabled but set to 0. Can scrub be disconnected from road details?"

fix: "Bug: If the client gets disconnected from server (end of practice time) ffb remains active, eg if i was going full speed through corner, that force will remain on the wheel."

renaming each setting to be fully descriptive, backward compatibility, loading values from version 06.24.
Preset and confix fix: saving new sliders values to the ini file, adding app version, 
-> about this, expand prompt about updating the ini format (if it needs to be updated)

Update default values: eg yaw kick default cut oof

preset updates (see subsection below)



### FIxes 

Fix: the game exited from the session, and there were still forces
in particular self align torque and slide vibration
improve logic of detecting when not driving / not live, and stop ffb

### Preset Updates

2. **Preset Updates**: Consider updating built-in presets to showcase new frequency tuning:
   - "High-End DD" preset could use 40Hz ABS for sharper feedback
   - "Belt-Driven" preset could use 15Hz ABS for smoother feel

overhaul the presets:
delete the test and possibly also the guide presets
add a zero latency preset
presets for future: t300, g29, standard DD (<20 bit encoder), high end DD (>20 bit encoder)

* min force: set a value that works wheel for belt and gear driven wheels. The point it to overcome the resistance of the belt/gear for these wheels, to feel the lower forces of the FFB.

### Manual testing current effects, new presets and default values

test and fix current effects (this requires manual testing of the app; only check if we need to implement anything to support such testing):
* understeer effect: experiment to make it work.
* fix: "curbs and road surface almost mute, i'm racing at Sebring and i can hear curbs by ingame sound not wheel.."
* test default values after 0-100% normalization of sliders
* test if some vibration effects are muted
* check lockup vibration effect, feel it before bracking up, enough to prevent it

* experiment with gyro damping to compensate yaw kick
* spin vibration might also not be working
* min force (0.5) on a T300
* yaw kick with activation threshold at 2.0
  * yaw kick further fixes? smoothing? higher thresholds? non linear transformation? 

verify and investigate: LMU 1.2 bug where mLateralForce is zero for rear wheels; see the workaround in use.
* check if the new console warning for missing data triggers

### New Telemetry Effects & Advanced Physics
* [Report: New Telemetry Effects & Advanced Physics](report_new_telemetry_advanced_physics.md)

TODO: separate in multiple reports, eg. one for each major new FFB effect

more telemetry data to be used: 
* High Priority: mLocalRot, mLocalRotAccel. Use Yaw Rate vs Steering Angle to detect oversteer more accurately than Grip Delta
* mLocalAccel.z for braking dive/acceleration squat cues
* "filtered throttle" (if available): use similarly to ABS pulse, to detect when Traction Control kicks in
* mElectricBoostMotorTorque, Hybrid motor torque, Hybrid Haptics: Vibration during deployment/regen (is it useful? or just "immersive"?). From other data: warning when battery full, and risk of rear brakes lockup?
* mLongitudinalForce: currently unused (add weight under braking? remove under acceleration?)
* mSuspForce, mSuspensionDeflection: set bottoming trigger to method B as default, to use mSuspForce instead of mTireLoad. Add use of mSuspensionDeflection. Use mRideHeight for scraping effects.
* mLateralPatchVel: More accurate "scrub" feel
* (other data): feel for "weight of car", eg. make GT3s feel heavier.
* mLongitudinalPatchVel: TC / slip? (if already used, try/experiment to see which sliders value make it felt on T300)
* mRotation, mVerticalTireDeflection, and others: for TC effect? more advanced TC effect, inspired by current brake and ABS effects?
* mTerrainName: Surface FX: Different rumble for Kerbs/Grass/Gravel. 
* mSurfaceType: Faster lookup for Surface FX (?)
* mCamber, mToe?
* mTemperature[3]: Cold Tire Feel: Reduce grip when cold (isn't this already taken into account from our grip estimation?)
* mWear: Wear Feel: Reduce overall gain as tires wear (isn't this already taken into account from our grip estimation?)
* mPressure: Pressure-sensitive handling?
* mBrakePressure: Brake Fade: Judder when overheated
* Suspension Bottoming (Deflection Limit): Triggering a heavy jolt when mSuspensionDeflection or mFront3rdDeflection hits stops (currently uses Ride Height/Force Spike/Load).

* lockup due to turn in under braking (circle of grip): do current lockup prediction cover this? will planned effect cover it? Or do we need some addition to the formulas?
    * additionally, for longitudinal lockup only (no turn in): in real life, there is a feel in the change of deceleration (..."a weird rubbery feel"), when the rear starts to lock up (a similar thing happens when drifting: the point where the tyre goes from when you are just starting to drift, there is this little "hump" that you can feel. But you need a LOAD FORCE for that, YOU CANNOT DO IT WITH VIBRATIONS). Does the current lock up effect cover this?
    * considering that the current lockup effect is a vibration (although with varying amplitude and frequency), should we also add something that is not a "vibration effect" but a change in a continuous force effect? (also "change of deceleration" effect?)
-> the planned feature for dynamic longitudinal weight transfer migth help convey this "rubbery" feeling of locking up as a load force


* See this doc with new effects to be implemented (some already are implemented): "Yaw Kick Smoothing, Gyroscopic Damping Smoothing, Chassis Inertia (Load) Smoothing"
Implement "Jardier" wet grip effects.
Implement adaptive (auto) optimal slip angle (and slip rate?)


### Basic Mode

add "basic mode" with only main sliders shown, and auto-adjust of settings.
Basic Mode: lmuFFB has now so many advance options. This might be confusing for users. Introduce a simplified mode, which shows in the GUI only the most important and intruidtive options, and hide the advanced options. This is similar to the VLC media player, which has a basic mode and an advanced mode for the settings.




### Other stuff

Verify this formula to calculate tyre load when not available. Is it an exact replacement, or an approximation?
$F = K \cdot x + D \cdot v$ (Spring Rate $\times$ Travel + Damper Rate $\times$ Velocity), 

Do we need to update the  Self-Aligning Torque (SAT) calculation to account for caster angle? 
"4.1. The Self-Aligning Torque (SAT) ModelThe primary force a driver feels is the SAT. In a real car, this is generated mechanically by the interaction of the tyre patch and the caster angle. In a simulator, this must be calculated.$$T_{total} = T_{pneumatic} + T_{mechanical}$$"
Mechanical Torque ($T_{mechanical}$): Derived from the caster angle and the lateral force. It always tries to center the wheel.Data: Requires steerAngle (from Shared Memory) and lateral force (Fy, often found in wheelLoad or separate force vectors if available).Pneumatic Torque ($T_{pneumatic}$): Derived from the tyre offset.Data: Requires slipAngle (or wheelSlip proxy) and wheelLoad.
"The "Dead" Feel Problem: ACE users have criticized the FFB for feeling disconnected.7 This usually means the $T_{pneumatic}$ component drops off too abruptly or is masked by excessive damping. A custom app can fix this by applying a non-linear curve (Gamma correction) to the $T_{pneumatic}$ value, boosting the detailed information near the center of the steering range before sending it to the wheel."

"Secondary forces add immersion.

Scrub: When the front tyres slide (Understeer), the steering should vibrate and go light.

Algorithm: if (wheelSlip[Front] > OptimalSlip) { Vibration = (wheelSlip - OptimalSlip) * Gain * sin(t); }

ACE Specifics: Since ACE exposes slipVibrations directly , the custom app can use this engine-derived value as a base and amplify it, rather than calculating it from raw slip ratios, ensuring the vibration matches the audio cues of tyre squeal.   "

TODO: review our Gyroscopic Effects (possible without any added damping?)
"4.3. Gyroscopic Effects
As wheels spin faster, they resist turning. ACE provides wheelAngularSpeed.

Algorithm: DampingForce = GyroConstant * wheelAngularSpeed * SteeringVelocity.

Effect: This adds stability at high speeds (e.g., 250 km/h on the Nürburgring straight) without making the car feel heavy at low speeds (hairpins)."


Yaw Kick update?
"Motion platforms use the acceleration of the slip angle (jerk) to cue the driver that the rear is stepping out"

Slip angle info currently missing in ACE? Possible to calculate it?
" Currently, ACE motion profiles are generic or non-functional for traction loss because the slipAngle field in the shared memory is often returning null or zero in specific car/track combos. This forces motion software vendors to wait for Kunos to patch the export function."

in GM stream (https://www.youtube.com/watch?v=z2pprGlRssw&t=18889s) the "delay" of FFB and disconnect from game physics was there even with SoP smoothing off ("raw"). Only the steering rack force was active. Investigate if there might still be a source of latency / delay / disconnect from game physics. We need manual testing to verify this, from DD users.


---

possible notes for readme:
the current version of the app uses a steering rack force that, in the case of GT3s, corresponds to the game FFB (LMU 1.2). In the case of the LMP2, the ingame FFB (LMU 1.2) adds significant smoothing or damping (this seems to mask a baseline tire vibration) so even the steering rack force alone is significantly more detailed in lmuFFB. In lmuFFB there are some settings to get rid of the baseline tire vibration from the steering rack (satich notch filter, steering torque smoothing). 

lmuFFB is particularly useful for lower end wheels (belt/gear driven, and DDs < 12 Nm), because it enhances details that are difficult to feel or absent otherwise.

---


overhaul the graphs: add new ones for new effects.
reorganize them, so they also take less vertical and horizontal space.



add timestamps to console prints

we need a button for ..disconnect from game? reset data from dame? signal session finished?
the telemetry persist even after quitting the game (slide texture and rear align torque)


Console logs confirm that mLateralForce (Sideways force on the tire), both front and rear, is encrypted by the game. Verify all effects that use it, and make sure there always are appopriate fallbacks. Verify if mLongitudinalForce has the same problem (add console warning, also with gating).
The other suspected missing data was mSuspensionDeflection, but no console print was shown about this in thests. Note: we do have a console warning for mSuspensionDeflection, and mSuspForce.
Notes from InternalsPlugin.hpp:
  double mVerticalTireDeflection;// how much is tire deflected from its (speed-sensitive) radius
  double mSuspensionDeflection;  // meters
  double mRideHeight;            // meters
  double mSuspForce;             // pushrod load in Newtons
  double mBrakePressure;         // currently 0.0-1.0, depending on driver input and brake balance; will convert to true brake pressure (kPa) in future
  double mLateralForce;          // Newtons
  double mLongitudinalForce;     // Newtons
  double mTireLoad;              // Newtons
  
## Implementation Plans
* [Report: Signal Processing & Latency Optimization](report_signal_processing_latency.md)
* [Report: Effect Tuning & Slider Range Expansion](report_effect_tuning_slider_ranges.md)
* [Report: Robustness & Game Integration](report_robustness_game_integration.md)
* [Report: UI/UX Overhaul & Presets](report_ui_ux_overhaul.md)
* [Report: Latency Investigation](report_latency_investigation.md)


## Optimal slip angle in real time: Slope Detection logic 

Here is a miscellaneous of notes taken by me and texts copied and pasted from other reports.

### Peak / Optimal slip angle and slip rate
How to Find the Missing Peak
Domain experts and telemetry tool developers utilize distinct methodologies to bypass this limitation.
6.1 The "Viper" Calculation Method (Real-Time Estimation)
Research snippet  details the C# source code for a SimHub plugin named "Viper.PluginCalcLngWheelSlip." This plugin exists precisely because the native data is insufficient.
The Algorithm:The plugin manually calculates longitudinal slip because the game's wheelSlip output is often a combined vector or unitless.Inputs: WheelAngularSpeed (from Shared Memory), CarSpeed (from Shared Memory).Unknown: TyreRadius ($r$).Challenge: The shared memory does not output the dynamic rolling radius of the tire.Workaround: The plugin likely estimates radius based on the car model or requires user calibration (driving at constant speed to solve for $r = v / \omega$).
Calculation: Once $r$ is estimated, the plugin calculates $\kappa = (\omega r - v) / v$.
Peak Detection: The plugin still does not know the optimal peak. It simply provides the accurate slip ratio. The user must then watch the dashboard, lock the brakes to find the peak (e.g., observing that deceleration is max at 15% slip), and then manually set a "Limit" variable in the plugin settings.

"The MoTeC Histogram Method (Post-Process Analysis)
Professional engineers use data logging to derive the peak. This is the standard workflow for ACC and ACE.

Data Acquisition: Use a tool (like ACC-Motec wrapper) to log SteeringAngle, Speed, G_Lat, and G_Long to a .ld file.

Scatter Plotting: In analysis software (MoTeC i2), generate a scatter plot.

X-Axis: Slip Angle (calculated or raw).

Y-Axis: Lateral G-Force.

Curve Fitting: The data points will form a curve. The top of this curve (the apex) represents the Optimal Peak for that specific setup.

Result: The engineer notes, "For the Porsche 992 GT3 R at Monza, the peak slip is 3.1 degrees."

Application: This value is then manually entered into dashboards or mental notes. It is not read dynamically from the game."

The "Slip Effect" in Force Feedback
FFB is often generated by the physics engine calculating the aligning torque ($M_z$). The aligning torque naturally drops off after the peak slip angle. If players feel this is "mushy" or "missing," it suggests the signal of "passing the peak" is weak.

You described the "Viper" Calculation Method (Real-Time Estimation) for the optimal slip angle and slip ratio. How accurate is this method?

How accurate is the " MoTeC Histogram Method (Post-Process Analysis)"?

Are the optimal values always setup specific?
Can they be calculated in real time by the FFB app?
If the car and setup is fixed, and we have found the optimal peaks with the MoTeC or Viper methods, do these values still change in thr course of a driving session (eg. from turn to turn, over many laps), and with varying conditions (track and air temperature, rain)?


Based on the detailed technical analysis of *Assetto Corsa Evo* (ACE) and standard vehicle dynamics principles, here is the evaluation of the methods and data behavior.

### 1. Accuracy of the "Viper" Calculation Method (Real-Time Estimation)

**Accuracy Rating:** **Moderate to High (Dependent on Calibration)**

The "Viper" method (used in SimHub plugins) calculates the **current** longitudinal slip ratio () in real-time. It does not natively "know" the optimal peak; it simply functions as a high-precision ruler that allows *you* to measure where the peak is.

* **The Radius Problem:** The formula  relies entirely on  (tire rolling radius). Since ACE does not export dynamic tire radius in shared memory, the Viper plugin estimates it (often by assuming zero slip during coasting). If this estimation is off by even 2mm, the calculated slip ratio will be offset (e.g., showing 2% slip when you are actually at 0%).
* **Optimal Detection:** The plugin is highly accurate at reporting *what the tire is doing right now*. However, it cannot tell you if that slip is "optimal" unless you perform a calibration braking zone (locking the wheels) to see at what percentage the G-forces drop off.
* **ACE Specifics:** In ACE, the tire radius changes dynamically with speed (centrifugal expansion) and load (squat). A static radius estimation used by many plugins leads to "drift" in the slip values at high speeds ( km/h), reducing accuracy on straight-line braking.

### 2. Accuracy of the "MoTeC Histogram Method" (Post-Process Analysis)

**Accuracy Rating:** **Very High (The "Gold Standard")**

This is the most accurate method available because it utilizes the physics engine's actual output (G-force vs. Slip Angle) to visualize the tire curve directly.

* **How it works:** You create an "XY Scatter Plot" in MoTeC i2.
* **X-Axis:** Slip Angle (calculated via ).
* **Y-Axis:** Lateral G-Force.


* **The Result:** You will see a clear curve. The top of the "hill" (apex of the curve) is the **exact optimal slip angle** for that car, on that track, with that setup.
* **Noise Filtering:** Unlike real-time apps which struggle with signal noise (bumps, curbs), MoTeC allows you to apply smoothing filters to find the true peak hidden in the data. This provides a definitive "target number" (e.g., 3.4°) that you can then hard-code into your FFB app.

### 3. Are Optimal Values Setup Specific?

**Yes, absolutely.** The "Optimal Peak" is not a fixed number for the chassis; it is a variable resulting from the tire's interaction with the road and load.

* **Aerodynamics (Downforce):** As speed increases, downforce increases vertical load (). Tires typically generate their peak force at *lower* slip angles under higher loads. The optimal angle at 200 km/h (Eau Rouge) will be lower than at 60 km/h (La Source hairpin).
* **Camber:** Increasing negative camber usually shifts the peak slip angle higher, requiring more steering lock to access maximum grip.
* **Tire Pressure:** Drastically changes the contact patch stiffness. An over-inflated tire is stiffer and peaks at a lower slip angle; an under-inflated tire is softer and peaks at a higher angle (feeling "mushy").

### 4. Do Values Change During a Session? (Dynamic Nature)

**Yes.** Even if the setup is fixed, the optimal peak moves constantly due to environmental factors. A static "LUT" (Lookup Table) approach for your FFB app will eventually become inaccurate during a long race.

* **Tire Temperature:** Cold tires are stiff (low peak angle). Hot tires become pliable (higher peak angle). Overheated tires lose friction coefficient (), flattening the peak and making the FFB feel vague.
* **Tire Wear:** As tread wears down, the tire behaves more like a slick (stiffer), potentially lowering the optimal slip angle, but calculating this requires tracking wear percentages which are available in the ACE Shared Memory (`float tyreWear[1]`).
* **Rain/Wet Track:** This causes the most dramatic shift. The friction curve on a wet surface peaks very early and drops off sharply. An FFB app using "Dry" peak values on a "Wet" track will tell the driver to steer too much, causing immediate understeer.

### 5. Can they be calculated in real-time by the FFB app?

**Yes, utilizing a "Slope Detection" Algorithm.**

Since you cannot read a static "Optimal Grip" variable, your FFB app must mathematically "hunt" for the peak in real-time, similar to how an ABS system works.

**Proposed Algorithm for your FFB App:**
Instead of trying to know the static peak value, monitor the **rate of change** of G-force relative to Slip Angle (the derivative ).

1. **Read Data:** `LateralG` and `SlipAngle` from Shared Memory.
2. **Calculate Slope:** 
3. **Logic:**
* **Slope > 0:** More steering is producing more grip. (Grip is increasing).
* **Slope ≈ 0:** You are **AT THE PEAK**. The tire is saturated.
* **Slope < 0:** More steering is producing *less* grip. (You are scrubbing/understeering).


4. **FFB Action:** When `Slope <= 0`, reduce the aligning torque sent to the wheel. This physically lightens the steering wheel, communicating to the user: *"Stop turning, you have exceeded the optimal peak."*

**Recommendation:** For your FFB app, do not try to "hard code" a specific optimal angle (e.g., 3.5°). Instead, implement this **Slope Detection** logic. It automatically adapts to rain, setup changes, and tire wear because it is reacting to the *live* physics result, not a pre-calculated table.


See also:
* docs\dev_docs\slope_detection_implementation_plan.md
* docs\dev_docs\FFB Slope Detection for Grip Estimation.md
* docs\dev_docs\FFB Slope Detection for Grip Estimation2.md