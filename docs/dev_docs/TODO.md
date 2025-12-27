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

### Manunal testing current effects, new presets and default values

test and fix current effects (this requires manual testing of the app; only check if we need to implement anything to support such testing):
* understeer effect: experiment to make it work.
* fix: "curbs and road surface almost mute, i'm racing at Sebring and i can hear curbs by ingame sound not wheel.."
* test default values after 0-100% normalization of sliders
* test if some vibration effects are muted
* check lockup vibration effect, feel it before bracking up, enough to prevent it
* yaw kick further fixes? smoothing? higher thresholds? non linear transformation? 
* experiment with gyro damping to compensate yaw kick
* spin vibration might also not be working

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

in GM stream (https://www.youtube.com/watch?v=z2pprGlRssw&t=18889s) the "delay" of FFB and disconnect from game physics was there even with SoP smoothing off ("raw"). Only the steering rack force was active. Investigate if there might still be a source of latency / delay / disconnect from game physics. We need manual testing to verify this, from DD users.


---

possible notes for readme:
the current version of the app uses a steering rack force that, in the case of GT3s, corresponds to the game FFB (LMU 1.2). In the case of the LMP2, the ingame FFB (LMU 1.2) adds significant smoothing or damping (this seems to mask a baseline tire vibration) so even the steering rack force alone is significantly more detailed in lmuFFB. In lmuFFB there are some settings to get rid of the baseline tire vibration from the steering rack (satich notch filter, steering torque smoothing). 

lmuFFB is particularly useful for lower end wheels (belt/gear driven, and DDs < 12 Nm), because it enhances details that are difficult to feel or absent otherwise.

---


overhaul the graphs: add new ones for new effects.
reorganize them, so they also take less vertical and horizontal space.

auto save last configuration. Is the save config button any longer needed?

add timestamps to console prints

we need a button for ..disconnect from game? reset data from dame? signal session finished?
the telemetry persist even after quitting the game (slide texture and rear align torque)


## Implementation Plans
* [Report: Signal Processing & Latency Optimization](report_signal_processing_latency.md)
* [Report: Effect Tuning & Slider Range Expansion](report_effect_tuning_slider_ranges.md)
* [Report: Robustness & Game Integration](report_robustness_game_integration.md)
* [Report: UI/UX Overhaul & Presets](report_ui_ux_overhaul.md)
* [Report: Latency Investigation](report_latency_investigation.md)