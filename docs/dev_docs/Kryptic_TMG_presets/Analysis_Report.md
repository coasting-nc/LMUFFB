# Analysis of Kryptic TMG's Thrustmaster T300 Presets for LMU

## 1. Introduction
This report analyzes a collection of Force Feedback (FFB) presets created by user **Kryptic TMG** for the **Thrustmaster T300** wheel in Le Mans Ultimate (LMU). The dataset includes specific configurations for 10 GT3 cars and a release video transcript explaining the rationale behind them.

## 2. Philosophy & Approach
Based on the transcript and the preset values, Kryptic's tuning philosophy is centered on **consistency and compensative normalization**.

*   **The Problem:** He observed significant inconsistencies in FFB feel when switching between cars (e.g., the BMW felt 'way heavier' than the Lexus).
*   **The Goal:** To make every car 'stand out and feel good,' specifically targeting the sensation of **weight** and **rear-end detail**.
*   **The Method:** He does not apply a single 'global' style. Instead, he tunes each car individually to compensate for its specific lack of detail or incorrect weight in the base game.
*   **Setup preference:** A critical part of his setup is using **400 degrees of rotation** (both in-wheel and in-game). He describes 900 degrees on the T300 as 'numb' and 'understeery.' This suggests a preference for hyper-responsive, 'pointy' steering over realistic steering ratios (which are typically 540-720° for GT3).

## 3. Comparative Analysis: The Numbers
The following table compares key parameters across the GT3 presets.

| Car | Gain | Max Torque | Shaft Gain | Understeer | Oversteer Boost | SoP | Lockup Gain |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Aston Martin** | 1.20 | 97.4 | 1.99 | 0.64 | 1.81 | 0.15 | 0.98 |
| **BMW M4** | 1.00 | 93.5 | 0.75 | 0.00 | 2.40 | 0.60 | 2.00 |
| **Ferrari 296** | 0.99 | 86.3 | 1.00 | 0.00 | 3.26 | 0.00 | 2.00 |
| **Mustang** | 1.10 | 95.2 | 1.98 | 0.40 | 2.70 | 0.74 | 2.00 |
| **Lamborghini** | 1.24 | 75.8 | 1.98 | 0.61 | 0.21 | 0.80 | 0.13 |
| **Lexus** | 1.00 | 100.0 | 2.00 | 0.00 | 2.52 | 0.29 | 0.37 |
| **McLaren 720S** | 1.20 | 110.1 | 1.99 | 0.64 | 1.26 | 0.15 | 0.98 |
| **Merc AMG** | 1.55 | 100.0 | 1.00 | 1.35 | 2.74 | 0.31 | 0.22 |
| **Porsche 992** | 1.45 | 100.0 | 1.00 | 0.00 | 2.00 | 0.31 | 0.22 |
| **Corvette Z06** | 1.10 | 93.1 | 1.98 | 1.35 | 2.74 | 0.31 | 0.22 |

## 4. Key Differences & Car Characteristics
The wide variance in settings reveals how differently LMU simulates these cars and how Kryptic compensates for it:

### A. Weight Normalization (Gain)
*   **Mercedes & Porsche** require massive gain boosts (1.55 and 1.45) to feel substantial.
*   **Ferrari & BMW** are naturally heavier or more communicative, requiring near-standard gain (~1.0).
*   *Insight:* The base game's dynamic range varies wildly between chassis.

### B. Steering Feel (Shaft Gain)
*   **Group 1 (High Assist):** Aston, Mustang, Lambo, Lexus, McLaren, Corvette all use a **Shaft Gain of ~2.0**. This suggests these cars have 'light' or vague steering racks in-game that need synthetic amplification.
*   **Group 2 (Natural):** BMW, Ferrari, Merc, Porsche use standard gain (~1.0 or lower).

### C. Rear End Detail (Oversteer vs. SoP)
*   **Ferrari 296:** Uses a massive **Oversteer Boost (3.26)** but **0 SoP**. This implies the car's self-aligning torque doesn't communicate slide well, so a synthetic 'boost' effect is maxed out.
*   **Lamborghini:** The opposite approach. Very low boost (0.21) but high SoP (0.80). The car likely communicates well through the rack naturally.
*   **BMW M4:** Balanced approach (2.40 Boost, 0.60 SoP).

### D. Braking Feel (Lockup)
*   Some cars (BMW, Ferrari, Mustang) need a **2.0x multiplier** on lockup effects to be felt.
*   Others (Lambo) need almost no boost (0.13), suggesting they have very harsh/obvious lockup vibrations natively.

## 5. Insights for App Improvement
Based on this analysis, we can draw several conclusions for improving the LMU FFB App:

### 1. Auto-Normalization (The 'Holy Grail')
The heavy manual tuning required here proves that a 'one size fits all' preset is impossible due to car physics variance.
*   **Proposal:** Implement an **Auto-Gain** feature. The app should sample the telemetry torque output over a few laps. If the Mercedes peaks at 4Nm and the Ferrari at 8Nm, the app should automatically adjust the Gain to target the user's wheel max (e.g., 6Nm for a T300).

### 2. Car-Aware Profile Loading
Kryptic manually loads presets. The app should:
*   **Detect the current car** from telemetry.
*   **Automatically load** a saved preset for that car if it exists (e.g., [CarName].ini).
*   *Benefit:* Eliminates the friction of manual switching, which was the user's main pain point.

### 3. Adaptive Effect Boosting
The data shows that **Lockup** and **Oversteer** signals vary in intensity per car.
*   **Proposal:** Analyze the signal-to-noise ratio of specific effects. If the game outputs very faint lockup vibrations for the Mercedes, the app could detect this 'weak signal' and automatically apply a boost multiplier, automating what Kryptic did manually.

### 4. Steering Ratio override
Since users like Cryptic are forcing 400 degrees to fix 'numbness,' the app could offer a **'Sensitivity Compensation'** feature that creates a non-linear steering curve (more sensitive around center) without changing the actual lock, keeping the car drivable but responsive.
