# Bug Report: Suspension Bottoming Directional Logic

**Date:** 2025-05-23
**Status:** Resolved (v0.3.3)
**Component:** `FFBEngine.h` (Suspension Bottoming Effect)

## Description
The "Suspension Bottoming" effect, introduced in v0.3.2, is intended to provide a heavy haptic impulse when the suspension travels to its limit (detected via high Tire Load). 

However, the current implementation applies this force as a **DC Offset** (constant addition) whose direction is determined by the *current* total force. 

```cpp
double sign = (total_force > 0) ? 1.0 : -1.0; 
total_force += bump_force * sign;
```

## The Issue
When driving on a straight line, `total_force` is near zero (e.g., oscillating between -0.001 and 0.001 due to noise or minor steering adjustments). 
If the car hits a bump or compression (e.g., Eau Rouge compression) while the wheel is centered:
1.  `total_force` might be slightly negative (e.g., -0.0001).
2.  The logic calculates `sign` as `-1.0`.
3.  The `bump_force` (which can be large, e.g., 20% of max force) is added in the negative direction.
4.  **Result:** The wheel violently jerks to the LEFT (or RIGHT) depending on the micro-state of the FFB at that instant.

This feels unnatural. A vertical compression should result in a "heavy" feeling or a vibration/jolt that doesn't necessarily pull the wheel to one side unless the geometry dictates it (bump steer). In a straight line, it should probably be a high-frequency vibration (crunch) or simply increase the resistance to movement (damping), not an active turning force.

## Proposed Fix
Instead of a DC offset, use a synthesized waveform that feels like a mechanical limit hit, regardless of steering direction.

**Option A: High-Frequency Pulse (Crunch)**
Inject a 50-60Hz sine wave burst. This feels like a "thud" or "shudder" rather than a pull.
```cpp
// 50Hz Crunch
double crunch = std::sin(m_bottoming_phase) * bump_force;
total_force += crunch;
```

**Option B: Resistance (Friction/Damping)**
Increase the damping coefficient momentarily, making the wheel feel "heavy/stuck". (Harder to implement in the current "Force-only" architecture without Damping support).

**Recommendation:** Implement Option A (Pulse/Crunch) in the next release.


## Corrected Code Implementation

You need to modify `FFBEngine.h` to add the phase state variable and update the calculation logic.

**Step 1: Add the state variable**
Add `m_bottoming_phase` to the class member variables (alongside `m_lockup_phase`, etc.).

**Step 2: Update the logic**
Replace the DC offset logic with the oscillator logic.

Here is the corrected file segment for `FFBEngine.h`:

```cpp
class FFBEngine {
public:
    // ... [Existing Variables] ...

    // Phase Accumulators
    double m_lockup_phase = 0.0;
    double m_spin_phase = 0.0;
    double m_slide_phase = 0.0;
    double m_bottoming_phase = 0.0; // <--- ADD THIS

    double calculate_force(const rF2Telemetry* data) {
        // ... [Existing Code] ...

        // --- 5. Suspension Bottoming (High Load Impulse) ---
        if (m_bottoming_enabled) {
            // Detect sudden high load spikes which indicate bottoming out
            double max_load = (std::max)(fl.mTireLoad, fr.mTireLoad);
            
            // Threshold: 8000N is a heavy hit
            const double BOTTOM_THRESHOLD = 8000.0;
            
            if (max_load > BOTTOM_THRESHOLD) {
                double excess = max_load - BOTTOM_THRESHOLD;
                
                // Non-linear response (Square root softens the initial onset)
                double bump_magnitude = std::sqrt(excess) * m_bottoming_gain * 0.5;
                
                // FIX: Use a 50Hz "Crunch" oscillation instead of directional DC offset
                double freq = 50.0; 
                
                // Phase Integration
                m_bottoming_phase += freq * dt * TWO_PI;
                if (m_bottoming_phase > TWO_PI) m_bottoming_phase -= TWO_PI;

                // Generate vibration (Sine wave)
                // This creates a heavy shudder regardless of steering direction
                double crunch = std::sin(m_bottoming_phase) * bump_magnitude;
                
                total_force += crunch;
            }
        }

        // ... [Rest of Code] ...
    }
};
```

## Recommendation
Apply this fix immediately. The current implementation causes erratic behavior that could be misinterpreted by users as a hardware fault or "clipping" issue. The fix aligns with the "Phase Integration" architecture introduced in v0.3.0.