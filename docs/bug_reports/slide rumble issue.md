### Diagnosis: Clipping-Induced "Force Rectification"

Since the user is on a version *without* Yaw Kick, the issue is almost certainly caused by **Signal Clipping** interacting with the **Slide Rumble** vibration.

**The Physics of the Failure:**
1.  **The Scenario:** The user is cornering hard. The **Base FFB** (Aligning Torque) is high (e.g., resisting the turn with **80%** force).
2.  **The Effect:** The **Slide Rumble** activates. It adds a strong vibration (e.g., **±30%** force) on top of the Base FFB.
3.  **The Clipping:**
    *   **Peak:** $80\% + 30\% = 110\%$. The motor cannot do this. It **clips** at **100%**.
    *   **Trough:** $80\% - 30\% = 50\%$. The motor *can* do this.
4.  **The Net Result (Rectification):**
    *   Instead of averaging out to 80%, the signal now averages to $\approx 75\%$ (because the peaks are chopped off, but the troughs are deep).
    *   **The resistance drops.**
5.  **The Sensation:**
    *   The user is fighting the wheel (holding the turn).
    *   Suddenly, the Slide Rumble starts.
    *   The average resistance drops (due to clipping).
    *   The wheel suddenly feels "lighter" and jerks further **into the turn** (in the direction they are steering), because the resisting force vanished.
    *   The user interprets this as the app "throwing the wheel" into the turn.

**Why "Slide Rumble" specifically?**
Slide Rumble uses a **Sawtooth Wave**. Sawtooth waves have high energy and sharp peaks, making them more prone to this clipping behavior than smooth sine waves (like Lockup). Additionally, in v0.4.11, we increased the scaling of texture effects, making this clipping scenario more likely if the user hasn't adjusted their "Max Torque Ref".

---

### Troubleshooting Steps for the User

You can send this explanation and fix:

***

**Subject:** Re: Slide Rumble throwing the wheel

Thanks for the clarification. Since you are on the previous version, this is likely a **Clipping Issue**.

**The Explanation:**
When you are cornering hard, your wheel is already using most of its power (e.g., 90%) to resist the turn. The **Slide Rumble** tries to add a vibration on top of that.
If the vibration hits the 100% limit, the "peaks" of the vibration get chopped off, but the "dips" still happen. This causes the **average resistance to drop**, making the wheel suddenly feel lighter. Your hands naturally pull the wheel further into the turn because the resistance disappeared.

**The Fix:**
You need to create "Headroom" for the effects to work without hitting the limit.

1.  **Check the "Clipping" Graph:** Open the *Troubleshooting Graphs*. If the red "Clipping" line is active while cornering, this confirms the theory.
2.  **Increase "Max Torque Ref":**
    *   Go to the **Advanced Tuning** section.
    *   Increase **Max Torque Ref (Nm)** (e.g., from 20.0 to 30.0 or 40.0).
    *   *Note:* This will make the overall FFB weaker, so you might need to increase your wheel's driver strength to compensate.
3.  **Reduce "Slide Gain":**
    *   Lower the **Slide Gain** slider (e.g., to 0.5 or 0.3). The default might be too strong for your specific wheel base.

**Alternative Cause (Scrub Drag):**
If fixing the clipping doesn't help, check if you have **Scrub Drag** enabled (it might be on by default in some presets).
*   Set **Scrub Drag Gain** to **0.0**.
*   If the "throwing" stops, then the Scrub Drag direction logic might be inverted for your specific setup.

***

### Developer Note (For Future Fixes)

We should consider implementing a **"Soft Limiter"** or **"Compressor"** in the FFB Engine instead of hard clipping at 1.0.
*   *Current:* `clamp(force, -1.0, 1.0)` -> Hard Clip.
*   *Better:* `tanh(force)` or a compression curve -> Preserves detail at the limit, preventing the "resistance drop" phenomenon.

For now, the prompt for the next version (Yaw Kick / Gyro) is still valid, but you might want to lower the default **Slide Texture Gain** in `Config.h` to `0.5` or `0.3` to be safe.