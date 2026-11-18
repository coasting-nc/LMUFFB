# FFB troubleshooting guide

Use this guide when you already have a profile you mostly like, but one thing feels wrong. Find what you feel, then see which controls to try. To build a profile from scratch, use the [step-by-step tuning guide](step_by_step_tuning_guide.md).

This is a first version, so treat its advice as what to try first. Your wheel, car and taste may need something different. Control names are the Le Mans Ultimate ones; see [RaceRoom](#raceroom) for differences.

## How to use this guide

1. **Save a copy of your profile first.** Use Duplicate, so you can always go back.
2. **Change one control at a time.** Suggested step sizes are in [the control reference](#control-reference).
3. **Test the same way each time:** the same car, track and corner, with your wheel's own settings unchanged.
4. **If it gets worse or nothing changes, put it back** before you try the next control.
5. **Lower your wheel's strength** before trying anything that could make forces stronger, especially on a Direct Drive wheel.
6. **Avoid pushing one effect very high.** It tends to dull the others, so several small changes usually beat one big one.

Where a tip mentions Dalking, it comes from his setup video. He uses an 18 Nm wheel and likes a heavy feel, so go by the direction of the change and not the exact number.

## Quick finder

| What you feel | Go to |
| --- | --- |
| Too heavy, too light, tiring, differs by car | [Overall strength](#overall-strength) |
| Vague, numb, muddled, delayed | [Detail and response](#detail-and-response) |
| False understeer, no warning before the front goes | [Front grip](#front-grip) |
| Heavy under braking, a spike or dip into a corner | [Braking and turn-in](#braking-and-turn-in) |
| Too little or too much load through the corner | [Mid-corner load](#mid-corner-load) |
| Rear catches you out, kicks too strong, wheel fights you | [Rear of the car](#rear-of-the-car) |
| Wheel pulled around on kerbs, or kerbs numb | [Kerbs and bumps](#kerbs-and-bumps) |
| Wobble, shaking, grain, notchiness | [Wobble, shaking and grain](#wobble-shaking-and-grain) |
| Buzzing, too much or too little vibration | [Vibrations](#vibrations) |
| Jolts, pulling to one side, no force at all | [Jolts and no force](#jolts-and-no-force) |
| A slider does nothing | [A control seems to do nothing](#a-control-seems-to-do-nothing) |

## Overall strength

| What you feel | Try first | Then | Good to know |
| --- | --- | --- | --- |
| Too heavy everywhere; tiring | Lower **Master Gain** | Lower **Steering Scale** for steering/cues relative to vibrations | Master Gain also lowers damping. Strength Margin leaves damping unchanged |
| Too light everywhere | Check wheel torque/source and raise **Master Gain** gradually | Review **Steering Scale** and **Strength Margin** | The game's steering force is capped; raising Steering Scale past that cap only strengthens car-motion cues. Watch clipping |
| Everything, including vibrations, too strong | Lower **Master Gain** | | This changes everything at once |
| Heavy mid-corner, but too light at first turn-in | Lower **Cornering Feel** | Raise **Steering Shaft Gain** | Dalking also lowers **Strength Margin** and **Steering Scale** together. We haven't confirmed that this helps, so test it |
| Details vanish at peak load; a flat "ceiling" | Raise **Strength Margin** or lower **Steering Scale** | Lower the added effects, then **Master Gain** | This is clipping: the force has hit its limit. Check the clipping graph |
| Steering is strong but the added effects are faint | Lower **Steering Shaft Gain** | Raise the effect you miss | |
| Added effects drown out the steering | Raise **Steering Shaft Gain** or **In-Game FFB Gain** | Lower the strongest effect | |
| With Shaft Torque, everything is weak or washy | Try different **Car Strength** values | **Steering Scale** | Dalking found 25 too washy and used about 55-65. Lower values normally feel stronger, so test it on your wheel |
| Different weight from car to car | Keep a profile per car or class | **Master Gain** per car | A known limit: in our tests, a GT3 needed 63% where an LMP2 needed 85% |
| Strength feels wrong whatever you set | Check your wheel's own software | | A strength limit there changes everything here |

## Detail and response

| What you feel | Try first | Then | Good to know |
| --- | --- | --- | --- |
| Vague or numb overall | Lower **Cornering Feel** if it is high | Lower **Center Feel**; lower **Global Vibrations Strength** | One very high setting can dull the others |
| Muddled, too much going on | Lower the setting you raised last | **Rotation Kick**, then the other kicks | One kick can hide another |
| Feedback feels late | Set **Steering Smoothing** low | **Cornering Feel Smoothing**, **Brake Feel Smoothing** and **Road Smoothing** at 0 | Smoothing controls show the delay they add. Also reduce damper and filters in your wheel's software |
| Rubbery; slow to respond | Lower damper or inertia in your wheel's software | **Smart Hands-off Protection** and **Hands-off Smoothing** to minimum | Dalking keeps both at minimum, since they add delay |
| No feel around center on straights | Raise **Center Feel** | Raise **Min Force** only if your wheel has a real deadzone | Too much can hide Cornering Feel near center |
| Center feels notchy or artificial | Lower **Center Feel** | Lower **Min Force** | |
| The overall feel is not what you want | Compare **Shaft Torque** with **In-Game FFB** | | Dalking prefers Shaft Torque. Try both, since they feel different |

## Front grip

| What you feel | Try first | Then | Good to know |
| --- | --- | --- | --- |
| Wheel goes light too early; feels like understeer that isn't there | Lower **Understeer Effect** | Lower **Brake Feel** if it happens under braking | |
| No warning before the front washes out; you turn too much and wear out the front tires | Raise **Understeer Effect** | Raise **Cornering Feel** a little | Dalking stays at 7-15% and warns against going lower |
| Understeer shows late, then suddenly | **Understeer Curve** below 1 | Raise **Understeer Effect** | This effect is still being improved |
| Very different understeer in another class | Keep a separate profile per class | | This effect can feel different between car classes |
| Wheel goes light under hard braking in a straight line | Lower **Understeer Effect** | Check whether you are locking up | |
| Understeer Effect changes nothing | Turn off **Game Force Only** | | |

## Braking and turn-in

Brake Feel and Cornering Feel work as a pair here, because the weight hands over from one to the other as you release the brake and turn. Tune them together using one whole corner.

| What you feel | Try first | Then | Good to know |
| --- | --- | --- | --- |
| Very heavy when braking and starting to turn | Lower **Brake Feel** | Lower **Cornering Feel** | They add up on turn-in, especially at slow corners. Dalking uses 5-6%, where the default is 17% |
| Spike of weight at turn-in | Lower **Brake Feel** | **Brake Feel Smoothing** up slightly | Smoothing adds delay |
| Weight drops as you release the brake | Raise **Cornering Feel** | Lower **Brake Feel** or **Understeer Effect** | |
| Hard to rotate the car on entry; wheel feels stuck | Lower **Brake Feel** | Lower **Cornering Feel** | |
| Nothing changes when braking | Raise **Brake Feel** | Try it while turning | You feel it mostly with some steering applied, not in a dead straight line |
| Wheel feels too light while accelerating out | Set **Wheelspin Force Reduction** to **0%** in LMU | Check **Brake Feel** separately | Since 0.7.914, Spin Strength changes rumble only. Older versions coupled rumble and lightening |
| Weight comes on in a notchy way | **Brake Feel Curve** | **Brake Feel Smoothing** | Changing the curve also changes the strength, so recheck it |

## Mid-corner load

| What you feel | Try first | Then | Good to know |
| --- | --- | --- | --- |
| Not enough feeling of load in the corner; cold tires feel vague | Raise **Cornering Feel** | Add **Cornering Weight** | This was Dalking's biggest improvement. He used 100-110%, and up to 130% at some tracks |
| Too much weight in long corners | Lower **Cornering Feel** | Lower **Cornering Weight**, then **Steering Scale** | |
| Feel goes muddy with high Cornering Feel | Lower **Cornering Feel** | | |
| Load feels notchy | **Cornering Weight Curve** | **Cornering Feel Smoothing** up slightly | Smoothing adds delay |
| Weight jumps when changing direction | Lower **Cornering Feel** | **Cornering Feel Smoothing** up slightly | |

## Rear of the car

| What you feel | Try first | Then | Good to know |
| --- | --- | --- | --- |
| Rear goes on throttle before you feel it | Raise **Power Kick: Gain** | Lower **Threshold** or **TC Slip Target** | Lower values react earlier. Adjust together with Self-Aligning Torque, because you feel both at once |
| Rear goes on fast corner entry before you feel it | Raise **Braking Kick: Gain** | Lower **Threshold** | |
| Wheel doesn't help you catch a slide | Raise **SoP Self-Aligning Torque** | | Dalking uses about 120% and calls it massive |
| Wheel fights you through normal corners | Lower **SoP Self-Aligning Torque** | | |
| Kicks too violent | Lower that kick's **Gain** | **Snap** to 0, then raise **Threshold** | |
| Kicks fire in ordinary driving or on bumps | Raise that kick's **Threshold** | Lower **Rotation Kick** | |
| Kicks feel dull or late | **Kick Response** low | **Snap** up slightly, **Curve** below 1 | |
| A kick never happens | Lower **Threshold** a little, as well as raising **Gain** | Try with TC lower | With TC stopping the slide, there may be nothing to feel |
| You feel weight during a slide but not its direction | **SoP Self-Aligning Torque** | **Cornering Feel Boost (Slide)** | Dalking felt no difference from the boost |
| Steering goes light when the wheels spin | LMU: set **Wheelspin Force Reduction** to **0%** | For 0.7.913 or older, turn off **Spin Vibration** instead | Since 0.7.914 the controls are independent. A migrated reduction can remain active with vibration off |

## Kerbs and bumps

| What you feel | Try first | Then | Good to know |
| --- | --- | --- | --- |
| Wheel pulled left and right on kerbs, or while braking on them | Raise **Kerb Calming** | Lower **SoP Self-Aligning Torque** | Dalking uses 35-40%. A higher Self-Aligning Torque needs more calming |
| Still pulled with Kerb Calming high | Lower **Steering Shaft Gain** or **Cornering Weight** | Compare Shaft Torque with In-Game FFB | Kerb Calming doesn't remove every kerb force |
| Kerbs feel numb or missing | Lower **Kerb Calming** | Raise **Road Gain** | You still want to know they are there |
| Rear cue disappears over kerbs | Lower **Kerb Calming** | | |
| Harsh hit on big kerbs | Lower **Road Gain** | Lower **Vibration Load Limit**; turn **Bottoming** off | |
| Too many small bumps | Lower **Road Gain** | **Global Vibrations Strength** down, **Road Smoothing** up | Smoothing delays the feel |
| Hits when the car bottoms out are too strong | Turn off **Bottoming Effect** | | This effect is still being improved, so leave it off for now |

## Wobble, shaking and grain

| What you feel | Try first | Then | Good to know |
| --- | --- | --- | --- |
| Wheel wobble at speed | Lower **Cornering Feel** | Raise **Braking Kick: Threshold**; raise **TC Slip Target** | Dalking or a friend saw wobble from each of these |
| Wobble began after lowering a threshold | Put that threshold back | | |
| Wobble on a soft car over bumps | Lower or turn off **Road Details** | | |
| Oscillation when you let go of the wheel | Raise **Smart Hands-off Protection** in small steps | Add damper in your wheel's software | Adds some delay. Keep your hands on the wheel, and don't test hands-off at speed |
| Wheel shakes while stopped or at very low speed | Raise **Stationary Damping** | | |
| Grainy feel | Add a little damper in your wheel's software | **Hands-off Smoothing** up if Hands-off is in use | Dalking went from 15 down to about 2 on his wheel's damper with this app |
| Grain only with a certain effect on | Lower that effect | **Global Vibrations Strength** | |

## Vibrations

| What you feel | Try first | Then | Good to know |
| --- | --- | --- | --- |
| Every grain of sand comes through | Lower **Global Vibrations Strength** | Lower **Road Gain** | Dalking uses 50% |
| Vibrations overpower the steering feel | Lower **Global Vibrations Strength** | Lower **Lockup Strength**, **Spin Strength** | |
| Buzz all through hard braking | Lower **Lockup Strength**, or raise **Start Slip %** | Turn it off if you use pedal haptics | A known limit, and it is being improved |
| Lock-up vibration too strong | Lower **Lockup Strength** | Lower **Rear Boost** | **Global Vibrations Strength** doesn't change this one |
| Rear lock-ups much stronger than front | Lower **Rear Boost** | | |
| No lock-up vibration | Turn on **Lockup Vibration** | Raise its strength, and check that you really lock up | Cars with ABS rarely lock |
| ABS vibration missing | Turn on **ABS Pulse** | Raise **Pulse Gain** | Off by default. It only runs while ABS is working |
| Spin rumble annoying | Turn off **Spin Vibration** | | |
| No road feel | Turn on **Road Details** at about 5% | | |
| Vibration too strong under high downforce | Lower **Vibration Load Limit** | | Only affects Road Details and Slide Rumble |
| Pitch feels wrong | **Vibration Pitch**, **Spin Pitch**, **Slide Pitch** | | Wheels differ in what they can reproduce |

## Jolts and no force

| What you feel | Try first | Then | Good to know |
| --- | --- | --- | --- |
| Jolt when pausing or returning to the menu | Lower your wheel's strength for now | Report it, with a log | A known issue that we are looking into. No setting is known to fix it yet |
| Wheel pulls steadily to one side | Lower **Rotation Kick** | Check your wheel's center calibration; try the default profile | Report it, with a log |
| No force at all | Check **FFB Device** and the status line at the top of the app | Check **Master Gain** | |
| Force goes the wrong way | Toggle **Invert FFB Signal** while parked | | Confirm at low speed before driving |
| Feels like two forces fighting | LMU with Shaft Torque: turn in-game FFB off. RaceRoom: set FFB Strength to 0% | | The game may be driving the wheel as well |
| Force stops during a race | Check the status line | Check whether another program took the wheel | |

## A control seems to do nothing

| Control | Why |
| --- | --- |
| Missing settings or panels | Settings unsupported by the selected game are hidden. An empty tab explains that no settings apply. Temporarily unavailable controls can still be faded |
| Car Strength | Only used with Shaft Torque |
| Understeer Effect, Brake Feel | Turn off Game Force Only |
| Brake Feel | You feel it mostly with some steering applied |
| Cornering Feel Boost (Slide) | Needs Cornering Feel above 0, and Grip Limit Detection off |
| Kerb Calming | Needs SoP Self-Aligning Torque above 0 |
| Vibration Load Limit | Only affects Road Details and Slide Rumble |
| Global Vibrations Strength | Doesn't change Lock-up or ABS |
| Kicks and vibrations | They only act when the event happens: no wheel spin, no Spin Vibration. TC can prevent it |
| Soft Lock | Temporarily unavailable |

## Control reference

Use these step sizes to start with. Default means the Default LMU profile.

| Control | Tab | Default | Step | What you feel |
| --- | --- | --- | --- | --- |
| Master Gain | Presets & General | 85% | 5% | Everything |
| Steering Scale | Presets & General | 3.6 Nm | 0.5 Nm | Steering weight and added effects |
| Strength Margin | Presets & General | 100% | 5% | Steering, effects and vibrations; higher is weaker |
| Min Force | Presets & General | 0 | 0.005 | Very small forces, near center |
| Stationary Damping | Presets & General | 12% | 2% | Resistance when stopped |
| Smart Hands-off Protection | Presets & General | 0% | 2% | Resistance to oscillation |
| Center Feel Strength | Presets & General | 2.0x | 0.25x | Force near center |
| Steering Shaft Gain | Front Axle | 99% | 5% | Steering force, with Shaft Torque |
| In-Game FFB Gain | Front Axle | 67% | 5% | Steering force, with In-Game FFB |
| Car Strength | Front Axle | 22.5 Nm | 5 Nm | Steering and added effects, with Shaft Torque; higher is weaker |
| Understeer Effect | Front Axle | 25% | 1-2% | Lightening as the front loses grip |
| Cornering Feel | Cornering | 167% | 10% | Weight from cornering |
| Cornering Weight | Cornering | 0% | 25% | A little more corner weight |
| SoP Self-Aligning Torque | Rear Axle | 50% | 10% | Counter-steer as the rear slides |
| Kerb Calming | Rear Axle | 7% | 5% | Calmer wheel on kerbs |
| Rotation Kick / Activation Threshold | Rear Axle | 10% / 1.02 | 5% / 0.1 | Early cue that the car is rotating |
| Power Kick: Gain / Threshold / TC Slip Target | Power Oversteer | 20% / 0.33 / 10% | 5% / 0.05 / 1% | Cue as the rear goes under power |
| Brake Feel | Braking | 17% | 1% | Weight under braking; lighter under power |
| Braking Kick: Gain / Threshold | Braking | 5% / 1.0 | 5% / 0.1 | Cue as the rear goes light on entry |
| Lockup Strength | Braking | 2.0 (200%) | 0.1 | Lock-up vibration |
| ABS Pulse: Pulse Gain | Braking | Off; 0.17 | 0.05 | ABS vibration |
| Global Vibrations Strength | Vibrations | 94% | 10% | Road, Slide, Spin and Bottoming |
| Vibration Load Limit | Vibrations | 1.59x | 0.1x | Limits Road and Slide |
| Road Gain | Vibrations | Off; 5% | 1% | Road and kerb texture |
| Spin Strength | Vibrations | 40% | 5% | Spin rumble and lighter steering |
| Slide Gain | Vibrations | Off; 5% | 1% | Sliding texture |

**Leave these alone unless you know why you are changing them:** the Grip tab, where one change can shift many effects at once, and the Safety tab, which protects you and your wheel.

## RaceRoom

RaceRoom works differently, so some advice changes:

- **In-game setting:** keep FFB Strength at 0% in RaceRoom's controls.
- **Strength:** use **Master Gain** and the steering gains. Steering Scale, Strength Margin and Car Strength don't apply there, and are hidden.
- **Values don't carry over:** the same slider can need a very different value. Default RaceRoom uses Cornering Feel 4% and Brake Feel 100%, so start from that profile.
- **Same ideas:** the same effects cause the same kinds of feel, such as Brake Feel and Cornering Feel adding up on turn-in. So use the tables above to decide which control to try.
- **Differs by car:** with the same settings, turn-in weight was fine in the Tatuus F4, a bit heavy in a BMW GT3 and very heavy in an Audi. A profile per class helps.

## If nothing here helps

1. Go back to your saved copy, or to the default profile.
2. If the problem remains on the default profile, turn off effects until it stops. The `Test: ...` profiles can help.
3. Report it with what you felt, the car, track and wheel, your profile and a log (START LOGGING, then reproduce the problem).
