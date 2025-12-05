# Customization of Tire Grip FFB

One of the primary advantages of external FFB applications like LMUFFB is the ability to tailor the force feedback sensation to the driver's preference, often exceeding the configuration options available in the game itself.

Drawing inspiration from **iRFFB** and **Marvin's AIRA**, LMUFFB allows for granular control over how tire physics are translated into steering forces.

## Core Customization Features

The following features should be exposed to the user (e.g., via sliders 0-100%):

### 1. Tire Grip Effect (Understeer)
*   **Description**: Modulates the strength of the force feedback based on the tire's available grip.
*   **Behavior**:
    *   **100%**: Maximal effect. The steering wheel becomes significantly lighter as the front tires lose grip (understeer). This provides a clear "cliff" sensation when pushing too hard.
    *   **0%**: Linear force. The steering force follows the game's physics torque purely, which may remain heavy even when sliding (depending on the car's geometry).
*   **Implementation**: `OutputForce = GameForce * (1.0 - (1.0 - GripFraction) * SliderValue)`

### 2. Seat of Pants (SoP) / Oversteer
*   **Description**: Simulates the lateral G-forces acting on the driver's body by injecting lateral force into the steering wheel. This helps the driver feel the rear end stepping out (yaw) before they see it visually.
*   **Behavior**:
    *   **100%**: Strong yaw cues. Useful for catching oversteer early.
    *   **0%**: Pure steering rack forces only.
*   **Implementation**: `TotalForce += (LateralAccel / 9.81) * ScalingFactor * SliderValue`

### 3. Slide Texture (Detail Booster)
*   **Description**: Adds a synthetic vibration or "scrubbing" texture when the tires are sliding laterally.
*   **Inspiration**: Marvin's AIRA "Slip Effect" or "Detail Booster".
*   **Behavior**:
    *   Generates high-frequency noise (e.g., sine wave or white noise) modulated by the slip angle or slide velocity.
    *   Helps distinguish between "gripping" turning and "sliding" scrubbing.
*   **Implementation**: `if (SlipAngle > Threshold) TotalForce += NoiseGenerator() * SliderValue`

### 4. Road Texture
*   **Description**: Amplifies high-frequency vertical suspension movements to enhance the feeling of curbs, bumps, and road surface details.
*   **Behavior**:
    *   Useful for belt-driven or gear-driven wheels that might dampen these subtle signals. Direct Drive users might lower this.
*   **Implementation**: High-pass filter on `mSuspensionDeflection` or `mVerticalTireDeflection`.

## User Interface Design Proposal

A potential GUI configuration panel would look like this:

| Setting | Type | Range | Description |
| :--- | :--- | :--- | :--- |
| **Master Gain** | Slider | 0 - 200% | Overall strength of the output signal. |
| **Smoothing** | Slider | 0 - 50 | Low-pass filter to reduce graininess/noise. |
| **Understeer Feel** | Slider | 0 - 100% | How much the wheel lightens during grip loss. |
| **SoP Effect** | Slider | 0 - 100% | Strength of lateral G-force injection. |
| **Slide Rumble** | Toggle | On/Off | Enable synthetic scrubbing vibration. |
| **Min Force** | Slider | 0 - 20% | Minimum force to overcome wheel friction (deadzone removal). |

## Philosophy: "Augmentation" vs "Reconstruction"

*   **Reconstruction (iRFFB Style)**: Takes the raw low-rate signal and attempts to predict/smooth it to a higher rate. LMUFFB naturally runs at high rates (400Hz) thanks to Shared Memory, so this is less critical than in iRacing (60Hz).
*   **Augmentation (MAIRA Style)**: LMUFFB leans towards this philosophy. We have a good base signal, and we want to *add* information (SoP, Slide Texture) that might be subtle or missing in the native feed.
