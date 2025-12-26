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

## Troubleshooting 25

DONE: remove warnings about missing telemetry data

DONE: add slider for optimal slip angle (and slip ratio)
DONE: list effects affected by grip and load approximation, and list those that are not affected

DONE: add smoothing (and slider) for steering shaft torque
DONE: expose sliders for additional smoothing: yaw kick, gyroscopic damping, Chassis Inertia (Load)
DONE: lockup vibration improvements

add "basic mode" with only main sliders shown, and auto-adjust of settings.

Yaw Kick Smoothing, Gyroscopic Damping Smoothing, Chassis Inertia (Load) Smoothing

understeer effect: experiment to make it work.

update tooltips

test default values after 0-100% normalization of sliders

test if some vibration effects are muted
check lockup vibration effect, feel it before bracking up, enough to prevent it

yaw kick further fixes? smoothing? higher thresholds? non linear transformation? 
experiment with gyro damping to compensate yaw kick

console prints, add timestamp

the game exited from the session, and there were still forces
in particular self align torque and slide vibratio
improve logic of detecting when not driving / not live, and stop ffb

spin vibration might also not be working


we need a button for ..disconnect from game? reset data from dame? signal session finished?
the telemetry persist even after quitting the game (slide texture and rear align torque)

Bracking and lockup effects: implement wider ranges in the sliders

Add more tooltips (many sliders are missing them)

lmuFFB has now so many advance options. This might be confusing for users. Introduce a simplified mode, which shows in the GUI only the most important and intruidtive options, and hide the advanced options. This is similar to the VLC media player, which has a basic mode and an advanced mode for the settings.

add a slider for the yaw kick thresshold to determine at which acceleration or force the yaw kick effect starts to be applied. There is still too much noise in the signal, and it does not actually work when needed (feeling a kick for the rear starting to step out).

Implement "Jardier" wet grip effects.

Implement adaptive (auto) optimal slip angle (and slip rate?)

For all vibration effects, add a slider to select the frequency of vibration (if fixed).
