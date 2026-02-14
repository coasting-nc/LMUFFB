As a follow up to this implementation plan: docs\dev_docs\implementation_plans\Slope Detection Fixes & Telemetry Enhancements v0.7.35.md
And to this new deep research report: docs\dev_docs\investigations\slope detection advanced features deep research.md

Create an implementation plan for the two features described below:

The Deep Research Report is extremely valuable. It not only validates the mathematical corrections we just designed (Projected Slope & Hold Timer) but also introduces **critical pieces of new information** that fundamentally alter our strategic approach.

Here is the breakdown of the insights and how they impact our plan.
 
### 1. The "Leading Indicator": Pneumatic Trail vs. Lateral G
**Insight:** The report highlights that Lateral G is a *lagging* indicator (force happens after slip). However, **Steering Torque** (derived from Pneumatic Trail) peaks and drops *before* the limit is reached.
> *"Monitoring the derivative of the mSteeringShaftTorque with respect to the steering input allows the FFB system to detect the 'plateau'... before the lateral force peaks."*

**Impact:**
*   **Improvement:** Our current Slope Detection uses `Lateral G / Slip Angle`.
*   **Proposal:** We should add a secondary slope calculation: `Steering Torque / Steering Angle`.
*   **Why:** This would give the user the "warning" of grip loss (Torque Drop) *milliseconds before* the car actually starts sliding (G-Force Drop). Combining these two creates the "Anticipatory" feel mentioned in the report.

### 2. Signal Processing: Slew Rate Limiter vs. Surface Type
**Insight:** The report suggests using a **Slew Rate Limiter** to handle curb strikes, rather than just filtering by Surface Type.
> *"A slew rate limiter... effectively acting as a low-pass filter that only activates during impulsive shocks."*

**Impact:**
*   **Refinement:** My previous plan suggested filtering logs based on `SurfaceType`. The report argues that a Slew Rate Limiter on the *input signal* is a better real-time solution because it handles 3D curbs (geometry) that might not be tagged as "Rumble Strips" in the track data.

---

### Strategic Recommendation

1.  **Torque Slope:** Add the `dTorque/dAngle` calculation as a "Leading Indicator" option for users who want faster reaction times.
2.  **Slew Limiter:** Implement the Slew Rate Limiter on the `Lateral G` input to automatically reject curb spikes without needing complex surface logic.
