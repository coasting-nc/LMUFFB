# LMUFFB To-Do List


## Troubleshooting 24

DONE: delay from the app due to SoP and slip angle smoothing
DONE: mask the flatspot vibration
DONE:yaw kick cutoffs by speed and force.

User report
"potentially leave the ingame FFB as is and add the SoP effects ontop? (...) for DD users this could be an amazing feature. I tested it with VRS DFP Pro"
if it is not possible to have both FFB coming from game and lmuFFB app, implement it by using the steering shaft torque as a surrogate of the game FFB (which is probably what the game FFB is based on). Try if we can remove the annoying "vibration" from this signal with an added smoothing filter, or other type of filter. This might reduce detail and get rid of the vibration. In fact, the game FFB seems less detailed than the raw steering shaft torque , and the reason might be an added smoothing filter.

decouple the scale of sliders from main gain and ref torque.
reorganize UI: understeer and oversterer groups of widgets , collapsible
within oversteer, nester collapsible group called SoP. First widget Rear align torque (research to confirm proper name), then yaw kick, gyro damping, and others.

loses wheel or connection to game when app not in focus
(make the app always on top? auto reconnect game / rebind wheel when disconnected? other mechanisms to avoid these disconnections?)

yaw kick further fixes? smoothing? higher thresholds? non linear transformation? 

improvements to the formulas

remove warnings about missing telemetry data

add slider for optimal slip angle (and slip ratio)

---

GUI

GUI reorganization

# General
* Master gain
* Max Torque ref
* min force
* Load Cap (add tooltip and clarify if actually general or just related to SoP)(or to tyre load, all 4 or 2)

# Understeer and front tyres
* Steering Shaft Gain
* Understeer (Grip): rename to clarify: its front tyres grip..
## Advanced / experimental 
* Base Force Mode

# Oversteer and Rear Tyres
* Oversteer Boost

## SoP
* Rear Align Torque (Rename?
* Yaw Kick
* Gyroscopic Damping
* Lateral G
## -> advanced (collapsible)
* SoP smoothing
* Sop Scale


# Grip and slip angle estimation
* Slip Angle Smoothing

# Advanced
# Signal filtering
* (current content)
