# Understeer Investigation Report (Final)

## Executive Summary
The "Understeer Effect" issues (lightness/signal loss) are caused by a misunderstanding of the `optimal_slip_angle` setting in the context of the Fallback Estimator. While **0.06 rad (3.4°)** is a physically accurate peak slip angle for LMP2/Hypercars, using it as the **cutoff threshold** for the FFB effect causes the system to punish the driver for merely reaching the limit of adhesion, rather than exceeding it.
## User reports
Below are the reports by two users about an issue with the understeer effect. These report were the reason for the present investgation and report document.

### User 1:
Findings from more testing: the Understeer Effect seems to be working. In fact, with the LMP2 it is even too sensitive and makes the wheel too light. I had to set the understeer effect slider to 0.84, and even then it was too strong.

### User 2:
With regards to the understeer effect I have to say that it is not working for me (Fanatec CLS DD). I tried the "test understeer only" preset and if I set the understeer effect to anything from 1 to 200 I can't feel anything, no FFB. Only below 1 there is some weight in the FFB when turning. When I turn more than I should and the front tires lose grip, I expect the wheel to go light, but that is not the case. The wheel stays just as heavy. So I cannot feel the point of the front tires losing grip. I tried GT3 and LMP2, same result.

## The "Physics vs. Algorithm" Conflict

### 1. The Physical Reality
User 1 is correct: LMP2 and Hypercars have very stiff tires with peak grip occurring around **0.06 - 0.08 radians (3.4° - 4.5°)**.

### 2. The Algorithmic Flaw
The application uses this value not as the "Peak", but as the **Penalty Start Line**.
*   **Rule:** `If Current_Slip > Optimal_Slip, Reduce_Force`.
*   **Result:** If `Optimal_Slip` is set to 0.06:
    *   The moment the driver reaches optimal grip (0.06), the FFB begins to cut.
    *   This creates a "Hole" in the FFB exactly where the steering should feel heaviest (maximum load/alignment torque).
    *   *Symptom:* "The wheel makes the steering too light" (User 1).

### 3. Why the Default must be Higher
To provide useful Force Feedback, the "Understeer Effect" should only reduce force when the user **exceeds** the optimal slip angle significantly (i.e., when they are actually scrubbing/wasting grip), not when they are utilizing it.
*   **Proposed Default:** **0.10 radians (5.7°)**.
*   **Logic:** This creates a "buffer zone" (0.06 to 0.10) where the driver can lean on the tire and feel full weight. The force only drops when the slip becomes excessive (true understeer), creating a *dynamic* feeling of loss as the limit is exceeded.

## Detailed Failure Cases

### Case A: The "False Understeer" (User 1)
*   **Car:** LMP2 (Stiff).
*   **Setting:** `Optimal = 0.06`.
*   **Action:** Driver takes Porsche Curves at the limit (Slip = 0.065).
*   **Algorithm:** "Slip (0.065) > Optimal (0.06). Grip reduced."
*   **Result:** Steering goes light.
*   **Driver Perception:** "I lost grip!" (False).
*   **Reality:** The car is gripping perfectly; the FFB logic is too aggressive.

### Case B: The "Signal Collapse" (User 2)
*   **Car:** GT3.
*   **Setting:** `Optimal = 0.06` (T300 Preset).
*   **Action:** Driver pushes hard (Slip = 0.09 / 5°).
*   **Algorithm:** "Slip (0.09) is 150% of Optimal (0.06). HUGE PENALTY."
*   **Grip Calc:** Drops to near limit (0.2).
*   **Effect Multiplier:** User sets `Understeer Gain = 2.0`.
*   **Math:** `Force = Base * (1.0 - (0.8 * 2.0)) = Base * -0.6`.
*   **Result:** Force clamped to 0. Total FFB loss.

## Recommendations

### 1. Renaming / Re-tooltiping
The parameter name `m_optimal_slip_angle` is scientifically accurate but practically misleading for tuning.
*   **Concept:** It acts as an **"Understeer Tolerance"** or **"Punishment Threshold"**.
*   **New Tooltip Definition:** "The slip angle limit above which the force begins to drop. Set this **higher** than the physical peak (e.g., 0.10 for LMP2) to allow driving at the limit without force loss."

### 2. Update Configuration Defaults
*   **T300/Default Presets:** Change `optimal_slip_angle` from **0.06** to **0.10**.
*   **Rationale:** 0.10 provides a safe buffer. It is high enough that "Peak Grip" (0.06) feels fully weighted, but low enough that a "Slide" (0.12+) will still cause a noticeable drop in tension. This restores the dynamic communication of the tire limit.

### 3. Refine the Drop-Off Curve
The current penalty curve `1.0 / (1.0 + Excess * 2.0)` is effectively a "Cliff".
*   **Issue:** Once the threshold is crossed, grip plummets too fast, causing the "On/Off" feeling reported by User 2.
*   **Solution:** Change formula to `1.0 / (1.0 + Excess)`.
*   **Benefit:** This creates a progressive fade-out of force as the slide worsens, allowing the driver to feel the *approach* of the limit and catch the slide, rather than incorrectly feeling that the slide has already happened.

### 4. Range Safety
*   **Slider Cap:** Limit `understeer_effect` to **2.0** (or 200%).
*   **Fallback Safety:** clamp the effect internally so that a single calculation cannot invert the force.

## Automated Regression Tests

The following tests should be added to `tests/test_ffb_engine.cpp` to verify the fix and prevent regression:

**Test 1: `test_optimal_slip_buffer_zone`**
*   **Goal:** Verify that driving at the physical tire peak (0.06) does NOT trigger force reduction when using the new default (0.10).
*   **Setup:**
    *   `m_optimal_slip_angle` = 0.10
    *   `m_understeer_effect` = 1.0
    *   Simulate Telemetry: `LateralVelocity` consistent with 0.06 rad slip.
*   **Expect:** `GripFactor` should be 1.0 (or > 0.99). Force should equal Base Torque.
*   **Why:** Ensures User 1's issue (lightness in corners) is solved.

**Test 2: `test_progressive_loss_dynamic`**
*   **Goal:** Verify force drops smoothly as slip exceeds the threshold, not instantly.
*   **Setup:**
    *   `m_optimal_slip_angle` = 0.10
    *   Step `Slip` from 0.08 -> 0.10 -> 0.12 -> 0.14.
*   **Expect:**
    *   At 0.08: No Drop.
    *   At 0.10: Minimal Drop.
    *   At 0.12: Moderate Drop.
    *   At 0.14: Significant Drop.
*   **Why:** Ensures dynamic feel is preserved and the "Cliff" is removed.

**Test 3: `test_understeer_clipping_safety`**
*   **Goal:** Verify that maximum settings do not cause math errors or zero-force on minor slips.
*   **Setup:**
    *   `m_optimal_slip_angle` = 0.10
    *   `m_understeer_effect` = 2.0 (Max)
    *   `Slip` = 0.11 (Minor overshoot).
*   **Expect:** Force should be reduced but POSITIVE and perceptible (> 0.0).
*   **Why:** Ensures User 2's issue (total signal loss) is protected against.
