# New FFB Features Enabled by LMU 1.2

> **⚠️ API Source of Truth**  
> All field names, types, and units referenced in this document are defined in **`src/lmu_sm_interface/InternalsPlugin.hpp`**.  
> This is the official Studio 397 API specification for LMU 1.2 shared memory.

Based on a detailed review of `src/lmu_sm_interface/InternalsPlugin.hpp`, there are several **new or previously inaccessible data points** that open the door for significant new FFB features.

Here is a breakdown of the new possibilities, ranked by value to the driving experience.

### 1. Hybrid System Haptics (The "Killer Feature" for LMU)
**Data Source:** `mElectricBoostMotorTorque`, `mElectricBoostMotorState`, `mElectricBoostMotorRPM`
**Context:** LMU focuses on Hypercars (LMDh/LMH) which rely heavily on hybrid deployment.
**New Effect: "Hybrid Whine / Pulse"**
*   **Concept:** Simulate the high-frequency vibration of the electric motor through the steering column when the hybrid system deploys or regenerates.
*   **Logic:**
    *   If `mElectricBoostMotorState == 2` (Propulsion): Inject a high-frequency, low-amplitude sine wave (e.g., 150Hz) scaled by `mElectricBoostMotorTorque`.
    *   If `mElectricBoostMotorState == 3` (Regeneration): Inject a "gritty" texture (e.g., 80Hz sawtooth) to signal braking regen.
*   **Why:** This gives the driver tactile confirmation of hybrid strategy without looking at the dash.

### 2. Surface-Specific Textures (Terrain FX)
**Data Source:** `mSurfaceType` (unsigned char), `mTerrainName` (char array)
**Context:** Previously, we relied on suspension deflection for all road noise. Now we know *what* we are driving on.
**New Effect: "Surface Rumble"**
*   **Logic:** Use a `switch` statement on `mSurfaceType`:
    *   **5 (Rumblestrip):** Boost the existing `Road Texture` gain by 2x.
    *   **2 (Grass) / 3 (Dirt) / 4 (Gravel):** Inject a low-frequency "wobble" (5-10Hz) to simulate uneven ground and reduce the Master Gain (simulating low grip).
    *   **1 (Wet):** Slightly reduce high-frequency "Slide Texture" friction to simulate hydroplaning risk.

### 3. Aerodynamic Weighting
**Data Source:** `mFrontDownforce`, `mRearDownforce`
**Context:** Hypercars generate massive downforce.
**New Effect: "Aero Stiffening"**
*   **Concept:** Increase the "Min Force" or "Damping" sensation as downforce increases.
*   **Logic:** `TotalGain = BaseGain * (1.0 + (mFrontDownforce / ReferenceDownforce) * AeroFactor)`.
*   **Why:** Helps center the wheel at high speeds (Mulsanne Straight) preventing oscillation, while keeping it light in slow hairpins.

### 4. Mechanical Damage Feedback
**Data Source:** `mDentSeverity[8]`, `mDetached`, `mLastImpactMagnitude`
**Context:** Endurance racing involves contact.
**New Effect: "Damage Wobble"**
*   **Concept:** If the front suspension/bodywork is damaged, the wheel should not rotate smoothly.
*   **Logic:** If `mDentSeverity[0]` (Front Left) or `[1]` (Front Right) > 0:
    *   Inject a sine wave linked to `mWheelRotation` (Wheel Speed).
    *   This simulates a bent rim or unbalanced tire.

### 5. Third Spring (Heave) Bottoming
**Data Source:** `mFront3rdDeflection`, `mRear3rdDeflection`
**Context:** Modern prototypes use a "Third Element" (Heave spring) to manage aerodynamic loads.
**Enhancement to: "Bottoming Effect"**
*   **Current Logic:** Uses `mTireLoad`.
*   **New Logic:** Combine `mTireLoad` with `mFront3rdDeflection`.
*   **Why:** You might hit the bump stops on the heave spring (aerodynamic bottoming) *before* the tires overload. This provides a distinct "thud" when the car scrapes the ground due to downforce, distinct from hitting a curb.

### 6. Brake Temperature Fade
**Data Source:** `mBrakeTemp`
**Context:** Carbon brakes have a specific operating window.
**New Effect: "Brake Judder / Fade"**
*   **Logic:**
    *   If `mBrakeTemp` > Optimal (e.g., 800°C): Introduce a low-frequency "Judder" (10Hz) during braking to simulate warped discs or overheating material.
    *   Reduce the "Lockup" vibration amplitude (simulating the loss of bite).

---

### Summary of Recommended Code Changes

To implement these, you would add the following to `FFBEngine.h`:

1.  **New Settings:**
    *   `bool m_hybrid_pulse_enabled`
    *   `bool m_surface_fx_enabled`
    *   `float m_aero_weight_gain`

2.  **New Logic in `calculate_force`:**

```cpp
// Example Hybrid Logic
if (m_hybrid_pulse_enabled && data->mElectricBoostMotorState == 2) {
    // Map Torque to Amplitude
    double hybrid_amp = (data->mElectricBoostMotorTorque / 200.0) * m_hybrid_gain;
    // High frequency whine
    total_force += std::sin(time * 150.0 * TWO_PI) * hybrid_amp;
}

// Example Surface Logic
if (m_surface_fx_enabled) {
    unsigned char surface = data->mWheel[0].mSurfaceType; // Check FL wheel
    if (surface == 5) { // Rumble Strip
        // Boost existing road texture
        road_noise *= 2.5; 
    } else if (surface == 2 || surface == 4) { // Grass/Gravel
        // Add chaotic wobble
        total_force += std::sin(time * 10.0 * TWO_PI) * 0.3;
    }
}
```

**Conclusion:**
The LMU 1.2 interface is a goldmine. While fixing the basic Tire Load/Grip is the priority, adding **Hybrid Haptics** and **Surface FX** would make your app significantly more immersive than the standard game FFB.