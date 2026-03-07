# Investigation Report - Issue #282: Lateral Load Effect Magnitude & Aero-Fade

## Problem Statement
The "Lateral Load" Seat-of-the-Pants (SoP) effect is reported as "doing nothing" by users, especially on high-downforce cars like the LMP2.

## Mathematical Analysis

### 1. The Aero-Fade Problem
The current implementation of Normalized Lateral Load Transfer is:
$$\text{LatLoadNorm} = \frac{L_{front} - R_{front}}{L_{front} + R_{front}}$$

Where $L_{front}$ and $R_{front}$ are the loads on the left and right **front** tires.
In a corner, the numerator ($L_{front}-R_{front}$) is dominated by lateral weight transfer:
$$L_{front}-R_{front} \approx \text{AccelX} \cdot \text{Mass} \cdot \frac{h}{w}$$

The denominator ($L_{front}+R_{front}$) is the total **front** axle load:
$$L_{front}+R_{front} = \text{StaticFrontAxleWeight} + \text{AeroFrontDownforce}$$

As speed increases, **AeroFrontDownforce** grows with the square of velocity ($v^2$).
Consequently, while the weight transfer (the signal) remains proportional to Lateral G, the denominator (the normalization factor) grows rapidly. This causes the signal to **shrink** as speed increases.

On an LMP2 car at 300 km/h, the total front load can be 3-4x the static weight, effectively cutting the FFB signal by 75%.

### 2. Relative Magnitude vs Lateral G
The "Lateral G" effect uses raw acceleration: $1.0\text{G} = 1.0$.
The "Lateral Load" effect uses a ratio. For most GT cars, the track-width to CG-height ratio results in a load transfer of ~40-50% at 1G.
$$\text{LatLoadNorm} \approx 0.4 \text{ to } 0.5 \text{ at } 1\text{G}$$

This makes the Lateral Load effect inherently **2x weaker** than the Lateral G effect even at low speeds.

## Normalization Strategy: Fixed vs. Learned

We have decided to use **Fixed Per-Class Normalization Values** instead of learned static load values.

### Fixed Class-Based Normalization
- **Pros:**
    - **Predictability**: FFB feel is consistent from the very first frame. No "learning phase".
    - **Stability**: Not susceptible to telemetry spikes, collisions, or corrupted peaks.
    - **Simplicity**: Lower maintenance and easier to debug/investigate.
    - **Aero-Fade Resistance**: By using a fixed static reference for the denominator, the signal stays proportional to weight transfer regardless of downforce.
- **Cons:**
    - **Less Granular**: Does not account for specific setups (fuel load, ballast).
    - **Maintenance**: Requires maintaining a lookup table of car classes.

### Evaluation
The "Learned" approach was previously disabled by default due to its complexity and tendency to produce inconsistent results (e.g. FFB dropping off after a high-G spike). For a "lean" effect, predictability is more important than accounting for 50kg of fuel difference.

## Proposed Solutions

### 1. Static Normalization (Fixed)
Change the denominator to use the **fixed static front axle weight** for the car class:
$$\text{LatLoadNorm} = \frac{L_{front} - R_{front}}{\text{FixedStaticFrontAxleWeight}}$$

### 2. Magnitude Parity
Apply an internal multiplier (e.g., 2.0x) to bring the load-based signal into the same numerical range as the acceleration-based signal.

### 3. Immediate Fallback
Reduce the hysteresis for kinematic fallback. If telemetry is encrypted (returning exactly 0.0), we should pivot to the estimation immediately.
