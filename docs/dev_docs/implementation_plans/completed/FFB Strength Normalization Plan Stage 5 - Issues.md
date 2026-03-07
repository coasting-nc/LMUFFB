


(issue description not included)

---

### 1. Why was this issue introduced? (Unforeseen vs. Misjudgment)

This issue was caused by a **conceptual flaw in the initial research report**, compounded by a **misjudgment during the implementation of Stage 1**.

**The Flaw in the Research Report:**
The research report heavily pushed for a "Session-Learned Dynamic Normalization" (an envelope peak follower) as the "industry standard." However, it misunderstood how sims like iRacing actually do this. iRacing has an "Auto" button that the user clicks *once* after driving a clean lap to set a **static** max force. 
The research report proposed doing this *continuously in the background*. As you experienced, continuous peak-following in a racing sim is a disaster. If you take a high-G corner, the peak follower raises the ceiling. When you get to a straightaway or a slower corner, the steering feels "limp" because the system is now scaling forces against that massive high-G peak. 

**The Misjudgment in Implementation:**
When we implemented Stage 1, we realized the 400Hz In-Game FFB signal (`genFFBTorque`) was already normalized by the game to a `` range. Because it was already normalized, we bypassed the peak follower for the 400Hz path and gave it a static denominator. 
This created a **split pipeline**:
*   **400Hz Path:** Used a static reference. Result = Stable, consistent weight.
*   **100Hz Path:** Used the volatile, ever-growing peak follower. Result = Limp, inconsistent weight.

### 2. Why can we treat the two signals the same way?

You are absolutely correct: we **can and should** treat them the same way. There is a specific mathematical point where both signals can be converted into **Absolute Newton-Meters (Nm)**. 

Here is how the math works to unify them:

*   **The 100Hz Signal (`mSteeringShaftTorque`):** This is already output by the game in Absolute Nm (e.g., 15 Nm, 25 Nm).
*   **The 400Hz Signal (`genFFBTorque`):** This is output by the game as a percentage ``. The game calculates this by dividing the internal steering rack force by the car's "Nominal Max Steering Torque" (a value baked into the game's physics files).

**The Unification Strategy:**
To treat them identically, we introduce a single user slider: **`m_car_max_torque_nm`** (e.g., 30 Nm). 

1.  **Ingestion:** 
    *   For 100Hz: We take the raw Nm directly.
    *   For 400Hz: We multiply the `` signal by `m_car_max_torque_nm` to convert it *back* into Absolute Nm.
2.  **Addition:** Now that the base steering is in Absolute Nm, we add our custom structural effects (SoP, Gyro, Rear Align), which are *also* calculated in Absolute Nm.
3.  **Final Scaling:** We take this grand total (in Absolute Nm) and map it to the user's physical wheelbase using the Stage 2 logic (`m_target_rim_nm / m_wheelbase_max_nm`).

By doing this, both 100Hz and 400Hz go through the exact same math, respond to the exact same sliders, and output the exact same physical weight at the rim.

### 3. Is the general idea from the 4-stage plan still correct?

**Yes, but with a pivot on Stage 1.**

*   **Stage 2 (Hardware Scaling):** 100% correct. Splitting the hardware max and the target rim torque is the ultimate solution for Direct Drive wheels.
*   **Stage 3 (Tactile Soft-Knee):** 100% correct. Anchoring textures to the static weight of the car prevents high-speed aero from shaking your rig apart.
*   **Stage 4 (Persistence):** 100% correct. Saving the static weight prevents the need for a "warm-up" lap.
*   **Stage 1 (Dynamic Peak Follower):** **This was a failed experiment.** The research report over-engineered a solution to a problem that is much better solved by giving the user a simple `Car Max Torque` slider. 

### Conclusion

There is a bug in the current implementation, but it is a bug born from a flawed architectural premise in the research report. 

Your intuition to unify the signals into a single scale is the exact right move. The implementation plan I provided in the previous message (titled **"Implementation Plan: Unified Absolute Nm Normalization"**) does exactly what you suggested: it rips out the flawed peak follower, introduces `m_car_max_torque_nm`, converts both signals to Absolute Nm, and unifies the pipeline.