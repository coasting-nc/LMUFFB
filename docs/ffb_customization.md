# Customization of Tire Grip FFB

One of the primary advantages of external FFB applications like LMUFFB is the ability to tailor the force feedback sensation to the driver's preference.

The C++ version of LMUFFB implements the following customizable effects:

## 1. Understeer Effect (Grip Modulation)
*   **Description**: Modulates the strength of the force feedback based on the tire's available grip.
*   **Implementation**: `OutputForce = GameForce * (1.0 - (1.0 - Grip) * SliderValue)`
*   **Tuning**:
    *   **100% (1.0)**: Maximal lightness when understeering.
    *   **0% (0.0)**: Force follows game physics purely (heavy even when sliding).

## 2. Seat of Pants (SoP) / Oversteer
*   **Description**: Simulates the lateral G-forces acting on the driver's body by injecting lateral force into the steering wheel.
*   **Implementation**: `TotalForce += (LateralAccel / 9.81) * ScalingFactor * SliderValue`
*   **Tuning**: Higher values help catch oversteer earlier by feeling the "weight" of the car shifting.

## 3. Slide Texture
*   **Description**: Adds a synthetic vibration or "scrubbing" texture when the tires are sliding laterally.
*   **Implementation**: Injects high-frequency noise (Sine wave or Random) when `SlipAngle > Threshold` or `GripFract < Threshold`.
*   **Tuning**:
    *   **Gain**: Amplitude of the vibration.
    *   **Toggle**: On/Off.

## 4. Road Texture
*   **Description**: Amplifies high-frequency vertical suspension movements to enhance the feeling of curbs, bumps, and road surface details.
*   **Implementation**: Uses a high-pass filter (delta of `mVerticalTireDeflection`) to detect bumps.
*   **Tuning**:
    *   **Gain**: Strength of the amplification. Useful for damping-heavy wheels.

## 5. Min Force
*   **Description**: Boosts small force signals to overcome the internal friction/deadzone of mechanical wheels (Gears/Belts).
*   **Implementation**: If force is non-zero but below threshold, set it to threshold (preserving sign).

## 6. Preset Management & Sharing
LMUFFB allows you to save and share your configurations easily.

### Saving Custom Presets
1. Adjust the sliders to your preference.
2. Enter a name in the text box next to **"Save New"**.
3. Click **"Save New"**. Your preset is now saved to `config.ini` and will appear in the **"Load Preset"** dropdown.

### Exporting for Sharing
1. Select the preset you want to share from the **"Load Preset"** dropdown.
2. Click **"Export Selected..."**.
3. Choose a location to save the `.ini` file. This file contains all your settings and can be sent to other users.

### Importing a Shared Preset
1. Click **"Import Preset..."**.
2. Select the `.ini` file you received.
3. LMUFFB will automatically add it to your user presets. If a preset with the same name exists, it will be renamed (e.g., "FastGT3 (1)").
4. The imported preset is automatically saved to your `config.ini` for future use.
