# Wheel strength, Self-adjusting Strength and vibrations

## 1. Your wheel and your preference

Two settings describe your setup:

1. **Wheelbase Max Torque (Nm):** your wheel's real maximum (for example 5 Nm for an entry-level wheel, 25 Nm for a high-end direct-drive wheel). Set it once to your wheel's specification.
2. **Steering Scale (Nm):** how strong you want the steering to feel. Lower it for a lighter wheel in long races; raise it for a heavier feel.

Because the app knows your wheel's real strength, vibrations such as road texture, ABS and slide rumble feel the same whatever Steering Scale you choose. Steering Scale changes the steering weight, not the vibrations.

### Setting it up

* Look up your wheelbase's maximum torque (for example Fanatec CSL DD 8 Nm, Simagic Alpha Mini 10 Nm, Simucube 2 Pro 25 Nm) and set **Wheelbase Max Torque** to that value. You rarely need to change it again.
* Adjust **Steering Scale** to taste.

### Older profiles

Profiles from older versions are converted automatically when you load them. If an old profile used an unrealistic value, the app starts from a safe point (15 Nm wheel, 10 Nm steering). The built-in profiles already use realistic values.

## 2. Self-adjusting Strength

Different cars produce very different steering forces: a heavy GT3 car feels nothing like a light prototype.

**Self-adjusting Strength** (Advanced tab, off by default) adjusts the steering strength by itself during a session, so different cars feel similar at your chosen Steering Scale.

* **Leaving the pits:** in a new session it needs a few seconds of hard cornering or braking to settle; usually it is settled within the first corners of your out-lap.
* **Stable feel:** once settled it changes slowly, so the steering does not fluctuate.
* **Crashes and kerbs:** big hits such as a wall or a hard bottoming are ignored, so they do not upset the strength.

We do not recommend it for most drivers: after a very big hit the force can feel lower for a while. With it off, the steering strength comes from Car Strength (Shaft Torque source) or your wheel (In-Game FFB source).

## 3. Vibrations at speed

* **Self-adjusting Tyre Feel** keeps road, slide and lock-up vibrations equally strong on every car class.
* The app learns each car's weight while you drive slowly in the pit lane, and remembers it, so vibrations feel right from the first corner next time.
* At very high speed in cars with a lot of downforce, vibrations are gently limited, so road detail stays clear without shaking the wheel out of your hands.
