# Steering Scale

Steering Scale sets how strong the steering and the car-motion cues feel compared with vibrations and damping. The value is shown in **Nm**. Your saved profiles and button bindings keep working.

Use **Master Gain** to make everything stronger or weaker. Set **Wheelbase Max Torque** to your wheel's real maximum; do not raise it to weaken the force.

## What it changes

Steering Scale changes the steering itself and the car-motion cues: Cornering Feel, Cornering Weight, the rear counter-steering cue (SoP Self-Aligning Torque), the kicks and Scrub Drag. It does not change road, slide, spin, bottoming, ABS or lock-up vibrations, or the damping. Master Gain scales everything together.

**Strength Margin** lowers steering, car-motion cues and vibrations together, while damping stays where it is. Its default is **100%**. Use it to make damping stronger compared with the other forces; it is not the same as lowering Master Gain.

## With the In-Game FFB source

The game's own steering force cannot be raised beyond the game's strength before Master Gain. Where that cap is reached depends on Steering Scale, In-Game Gain and Strength Margin. For example, with a 12 Nm wheel, In-Game Gain 100% and Strength Margin 0%, a Steering Scale of 12 Nm reaches the cap. With Strength Margin 100%, a Steering Scale of 12, 18 and 24 Nm gives 50%, 75% and 100% of the game's strength.

Past the cap, raising Steering Scale strengthens only the car-motion cues; the steering itself does not get heavier. Added effects, Min Force and the final mix can still raise the force or clip.

Keep LMU's own FFB Strength at the recommended 100% and turn off the game's own wheel output separately. LMU Strength 0% removes the steering information the app needs; it is not a volume control for the app.

## With the Shaft Torque source

The cap above does not apply. Steering Scale, Steering Shaft Gain and Car Strength set the steering level, and high settings can clip. After changing source, compare the same car and corners and raise Master Gain gradually.

## Practical adjustments

| What you want | First adjustment |
| --- | --- |
| Less force everywhere | Lower Master Gain |
| Lighter steering and car-motion cues, same vibrations | Lower Steering Scale |
| More damping compared with steering, cues and vibrations | Raise Strength Margin and check the balance |
| More of one slide or braking cue | Raise that effect, rather than Steering Scale past the cap |
| More detail at the peaks | Check for clipping and lower the strongest settings; a clipped force cannot get its peaks back |

Save a copy of your profile before tuning. Change one setting at a time, with the same wheel settings, car and test corner. RaceRoom does not use this LMU setting.
