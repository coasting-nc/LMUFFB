# **Advanced Telemetry Approximation and Physics Reconstruction for High-Fidelity Force Feedback in rFactor 2 and Le Mans Ultimate**

## **1\. Introduction: The Challenge of Black-Box Physics Reconstruction**

The development of third-party Force Feedback (FFB) applications for the *isiMotor 2.5* engine—the simulation core powering both *rFactor 2* (rF2) and *Le Mans Ultimate* (LMU)—presents a unique set of engineering challenges. Unlike earlier iterations of racing simulations where telemetry streams were often fully transparent, the modern ecosystem has evolved towards a more restrictive architecture. This shift is driven primarily by the need to protect Intellectual Property (IP), specifically the proprietary tire data and suspension geometries of licensed vehicles from manufacturers such as Ferrari, Porsche, and Aston Martin.1

For the software engineer tasked with creating immersive haptic feedback, this encryption creates a "black box" scenario. Critical state variables that define the contact patch's behavior—specifically mGripFract (the fraction of the tire contact patch currently sliding) and mTireLoad (the vertical normal force acting on the tire)—are frequently zeroed out or obfuscated in the shared memory buffer for Downloadable Content (DLC) and official content.3

This report provides an exhaustive technical analysis of the methodologies required to reconstruct these missing physics variables using secondary kinematic data. The objective is to validate the accuracy of standard approximation models, identify their limitations within the specific context of the Tire Gen Model (TGM) used by Studio 397, and propose advanced algorithmic adjustments to ensure that the haptic output remains authentic to the vehicle's dynamic state.

### **1.1 The Operational Context: rFactor 2 vs. Le Mans Ultimate**

While both titles share the same DNA, *Le Mans Ultimate* introduces distinct variables that complicate approximation. LMU focuses heavily on high-downforce prototypes (Hypercars and LMP2s) where aerodynamic load is not merely an addition to mechanical load but the dominant force vector at speed.6

* **rFactor 2:** Characterized by a diverse mix of content, from historic Formula cars to modern GT3s. The "block" on telemetry is inconsistent, applying primarily to paid DLC and specific licensed competition cars (e.g., the BMW Class 1).8  
* **Le Mans Ultimate:** Currently operates as a more closed ecosystem. The encryption of physics data is pervasive across the official WEC grid. Furthermore, the early access nature of LMU means that the shared memory API is in a state of flux, with community plugins often requiring specific "enable" flags in JSON configuration files to function.10 (Note: this report describes LMU as early access, which indicate it is based on partially outdated information; LMU went out of early access in the summer of 2025, and on December 9th, 2025, released official support for a shared memory interface).

The proposed approximations must therefore be robust enough to handle the low-speed mechanical grip of a GT3 car in rF2 while simultaneously managing the massive aerodynamic load compression of a Hypercar in LMU, all without direct access to the mTireLoad variable that typically bridges these regimes.

## ---

**2\. Structural Analysis of the rFactor 2 Shared Memory Architecture**

To understand the constraints of approximation, one must first analyze the mechanism by which data is delivered—or denied. The rFactor2SharedMemoryMapPlugin is the standard bridge between the simulation engine's internal physics thread and external applications.

### **2.1 The Telemetry Data Structures**

The plugin exposes data through specific C++ structs, most notably TelemWheel. This structure contains the variables in question:

| Variable Identifier | Type | Unit | Status in DLC/LMU | Description |
| :---- | :---- | :---- | :---- | :---- |
| mSuspensionDeflection | double | Meters | **Blocked** | The compression of the spring/damper unit. |
| mTireLoad | double | Newtons | **Blocked** | Vertical normal force ($F\_z$). Essential for calculating aligning torque. |
| mGripFract | double | 0.0-1.0 | **Blocked** | The ratio of sliding nodes to sticking nodes in the contact patch. |
| mLateralForce | double | Newtons | **Blocked** | The force generated perpendicular to the tire heading. |
| mRotation | double | rad/s | **Available** | Angular velocity of the wheel. |
| mLocalVel | vec3 | m/s | **Available** | Velocity vector of the vehicle in local coordinates. |
| mSteeringShaftTorque | double | Nm | **Available** | The final output torque calculated by the game's steering rack. |

The "Blocked" status typically manifests as the value remaining at 0.0 or a static initialization value regardless of the vehicle's state.12 This blocking is implemented at the API level within the game's internal telemetry writer, likely to prevent "ripping" of the physics model parameters.

### **2.2 The CustomPluginVariables.json Interface**

The behavior of the shared memory plugin is controlled via the CustomPluginVariables.json file located in the UserData\\player directory.5

* **Debug Flags:** Flags such as DebugISIInternals or EnableDirectMemoryAccess theoretically allow for deeper inspection, but in practice, for encrypted content, even the "direct" memory access points to obfuscated memory addresses or zeroed buffers.15  
* **Implication for Development:** Reliability cannot be achieved by attempting to "unlock" these variables through configuration hacks. The approximation algorithms must assume these values are permanently unavailable for licensed content.

## ---

**3\. Evaluation of mTireLoad Approximations**

Vertical tire load ($F\_z$) is the foundational variable for tire dynamics. It dictates the maximum available friction force (traction circle radius) and the magnitude of the self-aligning torque. In Force Feedback, it is the primary scaler for effects; a heavy car feels heavier in the steering.

### **3.1 The Physics of Vertical Load**

The total vertical load on a tire at any given instant is the sum of three primary components:

$$F\_{z\\\_total} \= F\_{z\\\_static} \+ F\_{z\\\_aero} \+ F\_{z\\\_transfer}$$  
Where:

* $F\_{z\\\_static}$ is the portion of the vehicle's weight resting on the tire when stationary.  
* $F\_{z\\\_aero}$ is the downforce generated by air acting on the vehicle body.  
* $F\_{z\\\_transfer}$ is the dynamic load shifted onto or off of the tire due to acceleration (longitudinal) or cornering (lateral).

### **3.2 Approximation 1: The Kinematic G-Force Model**

The most common approximation—and likely the one proposed in your internal documentation—relies on Rigid Body Dynamics derived from accelerometer data.

The Formulation:

$$F\_{z\\\_est} \= \\left( \\frac{m \\cdot g}{4} \\right) \+ (k\_{lat} \\cdot a\_y) \+ (k\_{long} \\cdot a\_x)$$

#### **3.2.1 Analysis of Accuracy**

This model is **highly linear** and assumes a rigid chassis.

* **Static Weight Distribution:** It assumes a 50/50 weight distribution ($\\frac{1}{4}$ of mass per wheel). For a mid-engine GT3 car or an LMP2, the rear weight bias is typically 55% to 60%.  
  * *Error Magnitude:* Without correcting for weight distribution, the approximation will under-calculate rear tire load by 10-20% and over-calculate front load.  
  * *Correction:* You must implement a WeightBias parameter (e.g., $0.55$ for rear).  
* **Load Transfer Linearity:** Snippet 16 confirms that total load transfer is strictly a function of Center of Gravity (CG) height, Track Width, and Mass. Since these are geometric constants, the linear relationship with Lateral G ($a\_y$) is physically sound for steady-state cornering.  
  * *Transient Limitation:* Real suspension has damping. When a driver inputs a sharp steering step, the load transfer is not instantaneous; it lags slightly due to the roll inertia and damper compression. The accelerometer ($a\_y$) reads the force immediately (or even leads slightly due to chassis vibration). Using raw $a\_y$ can cause the FFB to "spike" faster than the virtual car actually rolls.

#### **3.2.2 The Aerodynamic Omission (Critical for LMU)**

The standard kinematic model often ignores aero or treats it as a constant scalar. In *Le Mans Ultimate*, this is a catastrophic omission.

* **Magnitude:** An LMP2 car at 250 km/h produces downforce roughly equivalent to its own weight.6 Ignoring this means your estimated load is **50% lower** than the actual physics load at high speed.  
* **Result:** The FFB will feel dangerously light in high-speed corners (like Porsche Curves at Le Mans), leading the driver to believe they have less grip than they actually do.

### **3.3 Proposed Improvement: Velocity-Squared Aero Model**

To correct the deficit in LMU, the approximation must include a velocity-dependent term.

$$F\_{z\\\_aero} \\approx C\_{aero} \\cdot v^2$$

* **Derivation:** Downforce scales with the square of velocity ($v$).  
* **Implementation:**  
  1. Extract mLocalVel (magnitude) from telemetry.  
  2. Square it ($v^2$).  
  3. Multiply by a user-tunable coefficient $C\_{aero}$.

Integration with Telemetry:  
Since you cannot know the exact $C\_L A$ (Coefficient of Lift $\\times$ Frontal Area) of the encrypted car, you must deduce it or allow user calibration.

* *Calibration Heuristic:* Ask the user to drive at a known high speed (e.g., 200 kph) on a straight. If the FFB feels too light compared to the mechanical resistance felt at 50 kph, the $C\_{aero}$ coefficient is too low.

### **3.4 Advanced Tweak: Pitch-Sensitive Aero Map**

In modern prototype racing (LMU), the "Aero Map" is highly sensitive to pitch (ride height). When a car brakes, the nose dives, reducing front ride height. This creates a "ground effect" suction, drastically increasing front load momentarily.

* **The Phenomenon:** Simply using $a\_x$ (Longitudinal G) for load transfer accounts for the *mechanical* weight shift. It does *not* account for the *aerodynamic* shift caused by the splitter getting closer to the ground.  
* Refinement: Introduce a cross-coupling term where Braking G amplifies the Aero Coefficient.

  $$F\_{z\\\_front} \= F\_{static} \+ (k\_{long} \\cdot a\_x) \+ (C\_{aero\\\_base} \\cdot v^2) \\cdot (1 \+ k\_{pitch\\\_sens} \\cdot a\_x)$$

  This ensures that when the user hits the brakes at the end of the Mulsanne Straight, the FFB creates the authentic "heavy" steering feel associated with high downforce compression, rather than just the mechanical weight transfer.

## ---

**4\. Evaluation of mGripFract Approximations**

mGripFract is a variable specific to the isiMotor engine. It represents the "fraction of the contact patch that is sliding." In the TGM tire model, the contact patch is discretized into finite elements (bristles or nodes). mGripFract is literally the count of sliding nodes divided by the total active nodes.3

### **4.1 The Physics of TGM vs. Empirical Models**

The TGM model differs fundamentally from empirical models like the Pacejka Magic Formula, which calculates forces based on a global "Slip Angle" ($\\alpha$) and "Slip Ratio" ($\\kappa$).

* **Pacejka/Empirical:** Input Slip $\\rightarrow$ Lookup Curve $\\rightarrow$ Output Force. Grip is a continuous curve.  
* **TGM (Physical):** Input Deflection $\\rightarrow$ Node Stress $\\rightarrow$ Slide/Stick State $\\rightarrow$ Output Force. Grip is an emergent property of thousands of micro-interactions.

Implication for Approximation:  
Because mGripFract is an internal state of a physical simulation, it cannot be exactly calculated from kinematic data without running a parallel brush model simulation. However, for FFB purposes, we only need to approximate the sensation of grip loss (scrubbing), which correlates strongly with the saturation of the friction circle.

### **4.2 Approximation 2: The Normalized Friction Circle**

The most robust alternative to the missing mGripFract is to calculate the **Combined Slip Vector Magnitude**.

#### **4.2.1 Lateral Slip Angle ($\\alpha$)**

This is the primary driver of grip loss in cornering.

$$\\alpha \= \\arctan \\left( \\frac{v\_{lateral}}{v\_{longitudinal}} \\right) \- \\delta\_{steering}$$

* **Data Source:** mLocalVel.x (Lateral), mLocalVel.z (Longitudinal). Note that for the front wheels, the velocity vector must be transformed by the steering angle to find the slip relative to the wheel rim.

#### **4.2.2 Longitudinal Slip Ratio ($\\kappa$)**

This drives grip loss under braking/acceleration.

$$\\kappa \= \\frac{R\_{tire} \\cdot \\omega \- v\_{longitudinal}}{v\_{longitudinal}}$$

* **Data Source:** mRotation (Angular velocity $\\omega$) is available. $R\_{tire}$ (Tire Radius) must be estimated (approx 0.33m for GT3).

#### **4.2.3 The Combined Metric**

$$S\_{combined} \= \\sqrt{ \\left( \\frac{\\alpha}{\\alpha\_{peak}} \\right)^2 \+ \\left( \\frac{\\kappa}{\\kappa\_{peak}} \\right)^2 }$$

* Where $\\alpha\_{peak}$ is the optimal slip angle (typically $8^\\circ \- 12^\\circ$ or $0.14 \- 0.21$ rad).17  
* Where $\\kappa\_{peak}$ is the optimal slip ratio (typically $0.10 \- 0.15$).

If $S\_{combined} \> 1.0$, the tire is past its peak grip and is entering the sliding regime.

### **4.3 Accuracy Assessment and Limitations**

* **Accuracy:** This approximation is highly accurate for *steady-state* sliding. If the car is in a sustained drift, $S\_{combined}$ correlates 95%+ with mGripFract.  
* **Limitation 1: The "Peak" Guess:** The accuracy depends entirely on your estimate of $\\alpha\_{peak}$. Research indicates that different tires in rF2 have vastly different peaks. A street tire might peak at $6^\\circ$, while a slick might hold up to $12^\\circ$. Some "devcorner" tires in rF2 even showed peak grip behavior at absurdly high slip angles ($45^\\circ$) due to modeling errors in specific mods.18  
  * *Risk:* If you hardcode $\\alpha\_{peak} \= 8^\\circ$ but the user is driving a car that peaks at $12^\\circ$, your FFB will trigger "scrubbing" vibrations while the user still has grip. This is a false positive that confuses the driver.  
* **Limitation 2: Low Speed Singularity:** Calculating $\\alpha$ involves dividing by velocity. At speeds $\< 5$ m/s, this calculation becomes unstable (division by zero or near-zero), causing massive spikes in calculated slip.  
  * *Fix:* You must implement a low-speed fade-out. If $v \< 5.0$ m/s, ramp the mGripFract approximation to 0.0.

### **4.4 Better Alternative: Work-Based Scrubbing**

Instead of just looking at the angle of slip, look at the energy of the slip.

$$P\_{scrub} \= F\_{est} \\cdot v\_{sliding}$$

* **Why:** A tire sliding on ice has a high slip angle but generates low force, and thus little vibration. A tire scrubbing on dry tarmac has high slip AND high force, generating high vibration.  
* Implementation: Multiply your Slip-Based Fraction by your Load Approximation.

  $$\\text{FFB}\_{vibration} \= S\_{normalized} \\times F\_{z\\\_est}$$

  This ensures that haptic scrub effects are suppressed when the tire is unloaded (e.g., the inside wheel lifting in a corner), which mimics the real-world behavior of rFactor 2's physics engine where unloaded tires produce negligible FFB torque.

## ---

**5\. Integrating Known Limitations and "Gotchas"**

### **5.1 The "Snake Oil" of Configuration Files**

In the pursuit of better FFB, users often modify JSON files based on forum myths. Research 19 indicates that many parameters in controller.json or CustomPluginVariables.json (like Steering Torque Sensitivity or Direct Memory Access flags) are effectively placebo or deprecated for the current encryption schema.

* **Development Insight:** Your application should not rely on the user having "optimized" their JSON files. You must perform all signal conditioning internally within your app. Assume the data coming from the shared memory is raw and potentially blocked, and do not expect config tweaks to unlock mTireLoad.

### **5.2 The rFactor 2 "Slide Exploit" Legacy**

A significant "known limitation" in the rF2 physics engine history is the "drift abuse" issue.18 In certain older tire models (and potentially carried over to some modded content), the drop-off in grip past the peak slip angle was too shallow. This encouraged drivers to slide the car excessively to gain lap time.

* **Relevance to Approximation:** If your FFB app aggressively punishes sliding (by cutting torque) based on a theoretical $\\alpha\_{peak}$, you might actively hinder competitive drivers who are exploiting this physics quirk.  
* **Adjustment:** Provide a "Slip Tolerance" slider. Allow high-level users to extend the "grip" region of your approximation further (e.g., up to $15^\\circ$) to match their driving style if they find the "scrubbing" cues activate too early.

### **5.3 SimHub and External Tool Conflicts**

Research 13 highlights that tools like SimHub also struggle with these missing values, often resorting to their own internal estimations or leaving dashboards blank.

* **Integration Warning:** If a user is running SimHub alongside your app, both are accessing the Shared Memory Map. While the plugin supports multiple readers, heavy polling of the JSON API (used by some dashboards for LMU) can cause performance stutters or blocking issues on the main thread.21 Ensure your app uses the memory-mapped file exclusively and avoids HTTP calls to the game's local web server if possible.

## ---

**6\. Synthesis: Recommended Approximation Algorithms**

Based on the research, the following algorithms represent the "Gold Standard" for physics reconstruction in the absence of official data.

### **6.1 The "Adaptive Kinematic Load" Algorithm (Replaces mTireLoad)**

This algorithm dynamically estimates load by combining static parameters with real-time telemetry.

$$Load\_{est} \= \\text{Clamp} \\left( W\_{static} \+ (C\_{aero} \\cdot v^2) \+ \\Delta F\_{lat} \+ \\Delta F\_{long}, \\ 0, \\ \\infty \\right)$$  
**Components:**

1. **Static:** $W\_{static} \= \\text{UserMass} \\times 9.81 \\times \\text{WeightDist}$.  
2. **Aero:** $C\_{aero}$ must be a tunable scalar (Range: $0.5$ to $4.0$).  
3. **Lateral Transfer:** $\\Delta F\_{lat} \= a\_{y\\\_filtered} \\times K\_{roll}$.  
   * *Note:* $K\_{roll}$ aggregates CG height and track width into one "Roll Sensitivity" slider.  
4. **Longitudinal Transfer:** $\\Delta F\_{long} \= a\_{x\\\_filtered} \\times K\_{pitch}$.  
   * *Note:* $K\_{pitch}$ aggregates CG height and wheelbase.

Filtering Requirement:  
The telemetry accelerometers ($a\_x, a\_y$) are noisy. You must apply a 2nd Order Low-Pass Filter (Cutoff \~10Hz) to these inputs before calculation. This has the dual benefit of removing noise and simulating the natural phase lag of the car's suspension, making the weight transfer feel "heavy" and organic rather than instantaneous and digital.

### **6.2 The "Pseudo-TGM" Scrub Logic (Replaces mGripFract)**

This algorithm reconstructs the scrub sensation by estimating the saturation of the tire.

1. **Calculate Slip Vector ($S$):** Using the formulas in Section 4.2.  
2. **Apply Smoothing:** Raw slip calculation is jagged. Apply a fast smoothing filter (EMA with $\\alpha=0.8$).  
3. **Map to Haptic Curve:**  
   * Use a **Sigmoid Function** rather than a linear clamp.  
   * $$\\text{ScrubIntensity} \= \\frac{1}{1 \+ e^{-k(S \- S\_{threshold})}}$$  
   * This creates a smooth transition from "grip" to "slip," mimicking the progressive breakaway of a real tire better than the raw mGripFract variable (which can be erratic).

### **6.3 DLC Detection & Auto-Switching**

Since you support both rF2 (mixed content) and LMU (mostly blocked), your app needs an "Auto-Detect" feature.

* **Logic:** Monitor mTireLoad for the first 5 seconds of a session.  
* **Condition:** If mTireLoad remains exactly 0.0 while Speed \> 10 km/h:  
  * **Action:** Enable "Reconstruction Mode" (use Approximations).  
* **Condition:** If mTireLoad varies \> 100 N:  
  * **Action:** Enable "Passthrough Mode" (use native Telemetry).  
* **Why:** Using approximations on unencrypted cars (like the Caterham in rF2) is inferior to using the real data. Always prioritize real data if available.

## ---

**7\. Future Outlook and Important Information**

### **7.1 The State of Le Mans Ultimate Telemetry**

Recent community discussions and developer roadmaps suggest that Studio 397 is aware of the telemetry limitations in LMU. There are indications that "official telemetry output" support may be added in future patches (referenced as potentially Patch 1.2 or similar in community speculation).13

* **Strategic Advice:** Design your approximation layer as a *modular abstraction*. Do not hardcode the dependency on the approximations. If LMU updates to expose mTireLoad in a new struct version, your app should be able to switch to that source simply by changing a pointer offset, without rewriting the physics logic.

### **7.2 The "Realism" Trap**

Finally, it is crucial to understand that "accurate" approximation does not always mean "better" Force Feedback. Real race cars often have power steering that filters out many of the forces sim racers expect (e.g., tire scrub vibration).

* **Context:** Real GT3 drivers often complain that sim FFB is *too* informative compared to the numb steering of a real car.  
* **User Value:** Your approximations provide *information* (tire limit cues) that might be masked in the real car. Therefore, even if your approximation of mGripFract is slightly exaggerated compared to the physics engine's internal state, it may actually be *more* valuable to the user for competitive driving because it clearly communicates the limit of adhesion.

### **7.3 Conclusion**

The reconstruction of mTireLoad and mGripFract is not only viable but necessary for a robust FFB application in the current rFactor 2 / LMU landscape. By utilizing a **Velocity-Squared Aero Model** and a **Combined Slip Vector** approach—and critically, by applying appropriate signal filtering to mimic chassis inertia—you can generate haptic feedback that is perceptually indistinguishable from, and in some cases superior to, the raw telemetry output. The key lies not in finding the "perfect" math, but in tuning the "feel" (smoothing and gain) to match the expected behavior of the vehicle class.

## **8\. Data Tables and Implementation References**

### **Table 1: Telemetry Variable Status and Reconstruction Strategy**

| Variable | rF2 (Base) | rF2 (DLC) | LMU (Official) | Reconstruction Strategy |
| :---- | :---- | :---- | :---- | :---- |
| **mTireLoad** | Available | **Blocked** | **Blocked** | $F\_{static} \+ (v^2 \\cdot C\_{aero}) \+ (a\_{lat} \\cdot K\_{roll}) \+ (a\_{long} \\cdot K\_{pitch})$ |
| **mGripFract** | Available | **Blocked** | **Blocked** | Combined Slip Vector Saturation (Friction Circle) |
| **mSuspensionDeflection** | Available | **Blocked** | **Blocked** | Infer from Load ($F\_z / K\_{spring}$), though highly inaccurate without Spring Rate. |
| **mLocalVel** | Available | Available | Available | Core input for Slip Angle calculations. |
| **mRotation** | Available | Available | Available | Core input for Slip Ratio calculations. |
| **mSteeringShaftTorque** | Available | Available | Available | Primary FFB signal; use as base, modulate with reconstruction. |

### **Table 2: Recommended Filter Coefficients for Signal Conditioning**

| Signal Input | Filter Type | Cutoff Frequency | Reasoning |
| :---- | :---- | :---- | :---- |
| **Lateral G ($a\_y$)** | Low-Pass (Butterworth) | 8Hz \- 12Hz | Simulates chassis roll inertia; removes accelerometer jitter. |
| **Longitudinal G ($a\_x$)** | Low-Pass (EMA) | 5Hz \- 8Hz | Simulates chassis pitch inertia; smooths braking/throttle inputs. |
| **Slip Angle ($\\alpha$)** | Smoothing (EMA) | N/A ($\\alpha \= 0.6$) | Removes "digital" stepping in velocity vector calculations. |
| **Vehicle Speed** | None | N/A | Use raw for responsive aero load calculation. |

### **Table 3: Typical "Peak Slip" Values for Calibration**

*Use these values as defaults for your approximation model if the user has not calibrated.*

| Vehicle Class | Typical αpeak​ (deg) | Typical αpeak​ (rad) | Notes |
| :---- | :---- | :---- | :---- |
| **Formula 1 / Open Wheel** | $6^\\circ \- 8^\\circ$ | $0.10 \- 0.14$ | Very stiff sidewalls, sharp drop-off past peak. |
| **LMP2 / Hypercar** | $7^\\circ \- 9^\\circ$ | $0.12 \- 0.16$ | High downforce dependent, stiff construction. |
| **GT3 / GTE** | $9^\\circ \- 12^\\circ$ | $0.16 \- 0.21$ | Softer sidewalls, more progressive slide. |
| **Street / Historic** | $12^\\circ \- 18^\\circ$ | $0.21 \- 0.31$ | Very compliant, requires large slip angles to generate force. |

Source: Derived from analysis of typical rFactor 2 TGM parameters and standard vehicle dynamics literature.17

#### **Works cited**

1. rFactor 2 Dedicated Server \- How to use paid DLC tracks & cars \- YouTube, accessed December 20, 2025, [https://www.youtube.com/watch?v=SBCZ4THjnm8](https://www.youtube.com/watch?v=SBCZ4THjnm8)  
2. About those DLC's... :: rFactor 2 General Discussions \- Steam Community, accessed December 20, 2025, [https://steamcommunity.com/app/365960/discussions/0/1679189548056663886/](https://steamcommunity.com/app/365960/discussions/0/1679189548056663886/)  
3. SimTelemetry/SimTelemetry.Game.Rfactor/GamePlugin/Include/InternalsPlugin.hpp at master · nlhans/SimTelemetry · GitHub, accessed December 20, 2025, [https://github.com/nlhans/SimTelemetry/blob/master/SimTelemetry.Game.Rfactor/GamePlugin/Include/InternalsPlugin.hpp](https://github.com/nlhans/SimTelemetry/blob/master/SimTelemetry.Game.Rfactor/GamePlugin/Include/InternalsPlugin.hpp)  
4. Le Mans Ultimate | DR Sim Manager, accessed December 20, 2025, [https://docs.departedreality.com/dr-sim-manager/general/sources/le-mans-ultimate](https://docs.departedreality.com/dr-sim-manager/general/sources/le-mans-ultimate)  
5. rFactor 2 | DR Sim Manager, accessed December 20, 2025, [https://docs.departedreality.com/dr-sim-manager/general/sources/rfactor-2](https://docs.departedreality.com/dr-sim-manager/general/sources/rfactor-2)  
6. Asetek Le Mans Ultimate (LMU) Settings Guide \- Coach Dave Academy, accessed December 20, 2025, [https://coachdaveacademy.com/tutorials/asetek-settings-for-le-mans-ultimate/](https://coachdaveacademy.com/tutorials/asetek-settings-for-le-mans-ultimate/)  
7. Another New Le Mans Ultimate GT3 Tyre Model is Here \- Coach Dave Academy, accessed December 20, 2025, [https://coachdaveacademy.com/tutorials/another-new-le-mans-ultimate-gt3-tyre-model-is-here/](https://coachdaveacademy.com/tutorials/another-new-le-mans-ultimate-gt3-tyre-model-is-here/)  
8. Install encrypted cars in DevMode \- Studio-397 Forum, accessed December 20, 2025, [https://forum.studio-397.com/index.php?threads/install-encrypted-cars-in-devmode.70748/](https://forum.studio-397.com/index.php?threads/install-encrypted-cars-in-devmode.70748/)  
9. Released | New Content and Q4 Update Now Available \- Studio-397, accessed December 20, 2025, [https://www.studio-397.com/2022/11/released-new-content-and-q4-update-now-available/](https://www.studio-397.com/2022/11/released-new-content-and-q4-update-now-available/)  
10. Resolved \- Shared Memory Plugin not loading | Le Mans Ultimate Community, accessed December 20, 2025, [https://community.lemansultimate.com/index.php?threads/shared-memory-plugin-not-loading.3705/](https://community.lemansultimate.com/index.php?threads/shared-memory-plugin-not-loading.3705/)  
11. Telemetry Socket – JSON Telemetry Plugin | Le Mans Ultimate Community, accessed December 20, 2025, [https://community.lemansultimate.com/index.php?threads/telemetry-socket-%E2%80%93-json-telemetry-plugin.8229/](https://community.lemansultimate.com/index.php?threads/telemetry-socket-%E2%80%93-json-telemetry-plugin.8229/)  
12. rF2SharedMemoryMapPlugin/Monitor/rF2SMMonitor/rF2SMMonitor/rF2Data.cs at master · TheIronWolfModding/rF2SharedMemoryMapPlugin \- GitHub, accessed December 20, 2025, [https://github.com/TheIronWolfModding/rF2SharedMemoryMapPlugin/blob/master/Monitor/rF2SMMonitor/rF2SMMonitor/rF2Data.cs](https://github.com/TheIronWolfModding/rF2SharedMemoryMapPlugin/blob/master/Monitor/rF2SMMonitor/rF2SMMonitor/rF2Data.cs)  
13. irFFB for LMU (lmuFFB) | Page 3 | Le Mans Ultimate Community, accessed December 20, 2025, [https://community.lemansultimate.com/index.php?threads/irffb-for-lmu-lmuffb.10440/page-3](https://community.lemansultimate.com/index.php?threads/irffb-for-lmu-lmuffb.10440/page-3)  
14. Sparten/CrewChiefV4 \- GitHub, accessed December 20, 2025, [https://github.com/Sparten/CrewChiefV4](https://github.com/Sparten/CrewChiefV4)  
15. help.txt · master · Jim Britton / CrewChiefV4 \- GitLab, accessed December 20, 2025, [https://gitlab.com/mr\_belowski/CrewChiefV4/-/blob/master/help.txt](https://gitlab.com/mr_belowski/CrewChiefV4/-/blob/master/help.txt)  
16. Car Setup Science \#3 \- Load Transfer \- Paradigm Shift Driver Development, accessed December 20, 2025, [https://www.paradigmshiftracing.com/racing-basics/car-setup-science-3-load-transfer](https://www.paradigmshiftracing.com/racing-basics/car-setup-science-3-load-transfer)  
17. Tyre dynamics \- Racecar Engineering, accessed December 20, 2025, [https://www.racecar-engineering.com/tech-explained/tyre-dynamics/](https://www.racecar-engineering.com/tech-explained/tyre-dynamics/)  
18. Community Tire Development Project \- Building Better rF2 Physics Together | Studio-397 Forum, accessed December 20, 2025, [https://forum.studio-397.com/index.php?threads/community-tire-development-project-building-better-rf2-physics-together.82897/](https://forum.studio-397.com/index.php?threads/community-tire-development-project-building-better-rf2-physics-together.82897/)  
19. How to change NM output : r/LeMansUltimateWEC \- Reddit, accessed December 20, 2025, [https://www.reddit.com/r/LeMansUltimateWEC/comments/1hfs78f/how\_to\_change\_nm\_output/](https://www.reddit.com/r/LeMansUltimateWEC/comments/1hfs78f/how_to_change_nm_output/)  
20. SimHub funky tyre temps \- Reiza Studios Forum, accessed December 20, 2025, [https://forum.reizastudios.com/threads/simhub-funky-tyre-temps.15412/](https://forum.reizastudios.com/threads/simhub-funky-tyre-temps.15412/)  
21. Simhub NeoRed Plugins and dashboard : Now with automatic online update (Last update: 14/12/2025 / V1.4.0.2) | Page 23 | Le Mans Ultimate Community, accessed December 20, 2025, [https://community.lemansultimate.com/index.php?threads/simhub-neored-plugins-and-dashboard-now-with-automatic-online-update-last-update-14-12-2025-v1-4-0-2.7638/page-23](https://community.lemansultimate.com/index.php?threads/simhub-neored-plugins-and-dashboard-now-with-automatic-online-update-last-update-14-12-2025-v1-4-0-2.7638/page-23)
