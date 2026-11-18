# FFB strength controls

This guide explains the controls that set how strong the force feedback feels in Le Mans Ultimate. For worked examples, see the [Steering Scale guide](Cornering_Steering_Weight_Guide.md).

## The controls, in the order to set them

| Control | Where | What it does |
| --- | --- | --- |
| **Wheelbase Max Torque** | Presets & General | Your wheel's real maximum torque. Set it once from your wheel's specification; never use it to weaken the force. |
| **Master Gain** | Presets & General | Makes everything stronger or weaker: steering, car-motion cues, vibrations and damping. Start low on a direct-drive wheel and raise it gradually. |
| **Steering Scale** | Presets & General | How strong the steering and the car-motion cues feel compared with vibrations and damping. |
| **Strength Margin** | Presets & General (Advanced calibration) | Lowers steering, car-motion cues and vibrations together while damping stays the same. Use it to make damping relatively stronger. |
| **Min Force** | Presets & General | Lifts the very smallest forces, for wheels with a dead zone around center. Leave it at 0 on direct-drive wheels. |
| **In-Game FFB Gain** | Front Axle | How much of the game's steering force you feel with the In-Game FFB source. |
| **Steering Shaft Gain** and **Car Strength** | Front Axle | The steering level with the Shaft Torque source. A higher Car Strength gives a lighter steering. |
| **Self-adjusting Strength** | Advanced | Lets the app adjust the steering strength by itself during a session, so different cars feel similar. Off by default and not recommended for most drivers. |

## Recommended order

1. Set **Wheelbase Max Torque** to your wheel's specification, and LMU's own FFB Strength to 100%.
2. Choose the steering source: **In-Game FFB** (recommended) or **Shaft Torque**.
3. Set **Master Gain** to a comfortable overall strength.
4. Adjust **Steering Scale** until the steering weight feels right compared with the vibrations.
5. If the force goes flat at its peaks (clipping), raise **Strength Margin** or lower **Steering Scale**.
6. Only then tune the individual effects (Cornering Feel, Cornering Weight, the kicks, Scrub Drag).

## Common problems

| What you feel | Try |
| --- | --- |
| Too heavy everywhere | Lower Master Gain |
| Steering too heavy, vibrations fine | Lower Steering Scale |
| Details vanish at peak load | Raise Strength Margin or lower Steering Scale; check the clipping graph |
| Different weight from car to car | Keep a profile per car class, or try Self-adjusting Strength |
| Nothing changes when you raise Steering Scale (In-Game FFB) | You have reached the game's steering cap; only the car-motion cues get stronger. Raise In-Game FFB Gain or Master Gain instead |

RaceRoom uses Master Gain and its own steering gains; Steering Scale, Strength Margin and Car Strength do not apply there.
