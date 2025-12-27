# Query and plan

## Query 

How effective and accurate is the Slope Detection logic for a FFB app, in order to estimate the grip level, optimal slip angle, and optimal slip ratio for the FFB signal? What are the challenges and limitations?

## Plan

(1) Research the concept of "Slope Detection logic" in the context of Sim Racing Force Feedback (FFB) applications to understand how it analyzes the relationship between Self Aligning Torque (SAT) and tire slip.
(2) Investigate the theoretical method of using the derivative (slope) of the Force vs. Slip curve to identify the peak friction point, corresponding to the Optimal Slip Angle and Optimal Slip Ratio.
(3) Assess the accuracy of slope-based estimation in determining the actual Grip Level compared to ground-truth telemetry data from simulation engines.
(4) Analyze the effectiveness of this logic in communicating the limit of adhesion to the driver, specifically looking for how well it translates the drop in pneumatic trail into perceptible FFB cues.
(5) Identify signal processing challenges, such as the impact of high-frequency noise on derivative calculations and the trade-offs between smoothing filters and input latency.
(6) Explore limitations caused by different tire models (e.g., peaky vs. plateau friction curves) and suspension geometries (e.g., caster angles) that may decouple the SAT peak from the lateral force peak.
(7) Examine how external variables like road texture, curbs, and rapid load transfer affect the reliability of the slope detection algorithm in isolating tire physics from environmental forces.

# Report

# **Title: Exhaustive Analysis of Slope Detection Logic in Force Feedback Systems: Efficacy, Accuracy, and Limitations in Grip and Slip Estimation**

## **1\. Introduction: The Haptic Interface as a Deterministic Communication Channel**

In the realm of high-fidelity vehicle simulation and operator training, the Force Feedback (FFB) mechanism serves as the primary, and often sole, haptic communication channel between the physics engine and the human operator. Unlike the physical world, where a driver integrates vestibular cues (g-forces), proprioceptive feedback (chassis vibration), and visual flow to assess vehicle state, the operator of a static simulator relies disproportionately on the torque signals transmitted through the steering column. Consequently, the fidelity of this signal—its ability to encode complex tire dynamics into perceivable torque variations—is the limiting factor in simulation accuracy. Central to this encoding process is the concept of "Slope Detection," a logic structure that interprets the rate of change, or derivative, of tire forces relative to slip quantities. This report provides an exhaustive technical analysis of Slope Detection logic, evaluating its effectiveness in estimating grip levels, determining optimal slip angles, and identifying optimal slip ratios, while strictly delineating the substantial signal processing challenges and physical limitations inherent in current architectures.

The fundamental premise of simulation haptics is the translation of calculated mathematical moments—specifically the Self-Aligning Torque (SAT)—into a voltage signal driving a motor. However, the raw output of a tire model is rarely sufficient for intuitive control. The relationship between tire forces and slip is non-linear, and the critical information regarding the "limit of adhesion" is contained not in the absolute magnitude of the force, but in the gradient of the force curve. Slope Detection logic, therefore, acts as a derivative-based interpretive layer. It monitors the slope of the SAT curve ($\\frac{dM\_z}{d\\alpha}$) or the friction-slip curve ($\\frac{d\\mu}{d\\kappa}$) to provide the driver with a tactile "early warning" system. When this slope transitions from positive to zero (peak) and then to negative (drop-off), it signals the saturation of the contact patch.

The accuracy of this logic is paramount. If the FFB system inaccurately renders the slope—whether due to latency in the signal processing chain, aliasing from road texture noise, or fundamental inaccuracies in the underlying tire model—the driver's perception of the vehicle's limit is distorted. This report synthesizes data from tire dynamics research, signal processing theory, and specific simulator implementations (e.g., Assetto Corsa, iRacing, rFactor 2\) to construct a comprehensive assessment of the state-of-the-art in haptic slope detection. It explores how modern direct-drive hardware and advanced reconstruction filters attempt to mitigate the inherent noise-latency trade-off, and why the "optimal" slip angle communicated by FFB often diverges from the true physical optimum of the tire.

## **2\. Theoretical Framework: Tire Dynamics and the Mechanics of Slope**

To evaluate the accuracy of Slope Detection logic, one must first establish the physical ground truth: the generation of forces and moments within the tire contact patch. The "slope" being detected is a direct manifestation of the changing pressure distribution and adhesion status of the tire tread elements as they traverse the contact patch.

### **2.1 The Genesis of Lateral Force and the Linear Region**

The interaction between the tire and the road surface is governed by two primary mechanisms: adhesion, arising from intermolecular bonding between the rubber and the aggregate, and hysteresis, the energy loss due to viscoelastic deformation of the rubber.1 In the linear region of operation—typically at low slip angles ($\\alpha \< 2^\\circ$)—the entire contact patch remains in a state of static adhesion. Here, the lateral force ($F\_y$) generated is directly proportional to the slip angle.

$$F\_y \= C\_\\alpha \\cdot \\alpha$$  
The constant of proportionality, $C\_\\alpha$, is the Cornering Stiffness. In this region, the "slope" of the force curve is constant and positive. For a simulation FFB algorithm, this is the "build-up" phase. The logic detects a consistent rise in torque $\\frac{dF\_y}{d\\alpha} \\approx \\text{constant}$, which serves as the baseline for the driver's sense of "weight" in the steering. This linear relationship is crucial for the "Slip-Slope" friction estimation methods used in autonomous racing, where the steepness of this initial slope is used to predict the maximum available friction ($\\mu\_{max}$) before the limit is even reached.2 If the slope is steep, the available grip is high (e.g., dry asphalt); if the slope is shallow, the grip is low (e.g., ice or gravel). FFB systems effectively communicate this by changing the "spring rate" or stiffness feeling of the wheel near the center.4

### **2.2 Pneumatic Trail and the Self-Aligning Torque (SAT)**

The critical divergence between "Force" and "Torque"—and the phenomenon that makes Slope Detection possible—occurs as the slip angle increases into the transitional range. As the tire deforms, the lateral force is not distributed evenly across the contact patch. The leading edge of the tire, encountering fresh road, has high adhesion. The trailing edge, having already deformed, is more likely to slide. Consequently, the centroid of the lateral force shifts rearward, behind the geometric center of the wheel.

The distance between the wheel center and this force centroid is the **Pneumatic Trail** ($t\_p$). The driver perceives the Self-Aligning Torque ($M\_z$), which is the product of the lateral force and the total trail (pneumatic trail $t\_p$ \+ mechanical trail $t\_m$):

$$M\_z \= F\_y(\\alpha) \\cdot (t\_p(\\alpha) \+ t\_m)$$  
This equation reveals the mechanism of the tactile "drop-off." As the slip angle increases, the region of sliding within the contact patch propagates from the rear toward the front. This causes the force centroid to move forward, reducing the pneumatic trail ($t\_p$). Eventually, at high slip angles, $t\_p$ can shrink to zero or even become negative.

### **2.3 The Derivative as a Haptic Cue**

Slope Detection logic monitors the derivative of this torque function.

* **Phase 1 (Linear):** Both $F\_y$ and $t\_p$ are stable. $M\_z$ rises linearly. Slope is positive.  
* **Phase 2 (Peak):** The tire approaches saturation. $F\_y$ is still increasing (though the rate is slowing), but $t\_p$ is decreasing rapidly. The product $M\_z$ reaches a maximum. At this precise moment, the slope is zero ($\\frac{dM\_z}{d\\alpha} \= 0$).  
* **Phase 3 (Drop-off):** As the driver pushes further toward the absolute grip limit of the tire, the collapse of $t\_p$ dominates the equation. $M\_z$ decreases, resulting in a negative slope ($\\frac{dM\_z}{d\\alpha} \< 0$).

This negative slope is the "signal" that the FFB system communicates to the driver. It manifests as the steering wheel going "light" in the hands. The accuracy of Slope Detection logic depends entirely on how faithfully this physical phenomenon is modeled in the physics engine and how cleanly it is transmitted through the signal processing chain. In simulators like Assetto Corsa or rFactor 2, which utilize complex physical tire models (e.g., Pacejka Magic Formula or brush models), this drop-off is an emergent property of the math.5 In less sophisticated systems, it may be a "canned effect" triggered by a simple threshold, which significantly degrades the accuracy of the cue.

## **3\. Effectiveness in Grip Level Estimation**

The primary utility of Slope Detection logic in FFB is the estimation of the available grip level ($\\mu\_{max}$). The effectiveness of this estimation is high in terms of relative trend detection but faces limitations in absolute quantification due to external variables like load and temperature.

### **3.1 The "Slip-Slope" Correlation**

Research into autonomous vehicle dynamics confirms that the slope of the initial linear region of the friction-slip curve is a robust predictor of the surface friction coefficient. This "Slip-Slope" approach relies on the observation that the stiffness of the tire-road interaction is fundamentally linked to the adhesion limit.2 In FFB applications, this translates to the "weight" of the steering. When a sim racer transitions from a high-grip surface (rubbered-in tarmac) to a low-grip surface (green track or rain), the Cornering Stiffness ($C\_\\alpha$) drops. The FFB logic reflects this by lowering the torque gradient—the wheel requires less effort to turn for the same slip angle.

This passive form of slope detection is highly effective. It allows the driver to intuitively "feel" the grip level before reaching the limit. The logic is continuous and analog, requiring no discrete "events" to trigger. However, its accuracy is contingent on the update rate of the physics engine. A 60 Hz signal (common in older consoles or non-pro simulators) may alias this slope, making subtle changes in grip (e.g., patchy rain) indistinguishable from signal noise.8

### **3.2 The SAT Drop-off as a Limit Indicator**

The SAT drop-off (the transition to negative slope) is the most effective tool for estimating the *limit* of grip. By peaking *before* the lateral force limit, the SAT curve provides a safety margin. Research indicates that for a typical racing tire, the SAT peak might occur at $3^\\circ-4^\\circ$ of slip, while the lateral force peak occurs at $6^\\circ-8^\\circ$.4

This offset makes the logic an effective "early warning" system. If the driver reacts to the zero-slope point (the peak weight), they are safely within the recoverable envelope of the tire. However, this also implies a limitation: relying solely on the SAT peak causes the driver to underestimate the total available grip. To extract maximum performance, the driver must learn to ignore the "warning" of the lightening wheel and push into the negative slope region. FFB systems that artificially enhance this drop-off (e.g., the "Enhance Understeer" effect in Assetto Corsa or Forza Motorsport) effectively increase the *perceptibility* of the limit but decrease the *accuracy* of the grip estimation, as they exaggerate the physical signal.10

### **3.3 Limitations: Load and Temperature Sensitivity**

A significant challenge for Slope Detection logic is isolating the friction coefficient from other variables that affect compliance. The slope of the SAT curve is heavily dependent on Vertical Load ($F\_z$).

* **Load Sensitivity:** As downforce or weight transfer increases the load on a tire, the contact patch grows, and the cornering stiffness increases. This steepens the slope of the SAT curve.13  
* **The Ambiguity:** A driver feeling a "heavier" wheel (steeper slope) might interpret it as "more grip" (higher $\\mu$), when it is actually just "more load" (higher $F\_z$). Conversely, a lightening wheel might be interpreted as loss of grip, when it is actually just the front unloading over a crest.

Sophisticated FFB logic (e.g., rFactor 2's RealRoad) models the thermodynamic state of the tire, where overheating reduces the stiffness and flattens the slope. However, simplified models may use static lookup tables that fail to account for these dynamic variables, leading to "canned" slope behaviors that do not accurately reflect the changing grip conditions.15

## **4\. Accuracy in Optimal Slip Angle Estimation**

Determining the "Optimal Slip Angle"—the specific angle $\\alpha\_{opt}$ at which Lateral Force $F\_y$ is maximized—is perhaps the most complex task for FFB Slope Detection logic due to the inherent physical disconnect between the torque peak and the force peak.

### **4.1 The Offset Problem**

The fundamental limitation of using steering torque to find the optimal slip angle is that the two peaks do not coincide. As established, the SAT peak ($M\_{z,max}$) occurs at a lower slip angle than the Lateral Force peak ($F\_{y,max}$).5 This creates a "blind zone" for the driver.

* **Zone A (Linear):** Torque increases with slip. Feedback is positive and intuitive.  
* **Zone B (The Offset):** Torque is decreasing (negative slope), but Grip is still increasing. This is the counter-intuitive zone where the driver must push *against* the feedback's suggestion that the limit has been reached.  
* **Zone C (Post-Limit):** Both Torque and Grip are decreasing. The car is sliding.

If a Slope Detection algorithm is programmed to simply "maximize torque," it will guide the driver to Zone A/B boundary, resulting in under-driving. The driver will feel the car is "on rails" but will be slower than the theoretical limit. Accurate sim racing requires the driver to operate in Zone B. The "accuracy" of the FFB here is defined by how well it communicates the *rate of decline* in Zone B. A sharp drop-off (high negative slope) indicates a "peaky" tire where the limit is abrupt; a shallow drop-off indicates a forgiving tire.

### **4.2 Algorithmic "Fixes" and Their Trade-offs**

Developers have attempted to address this offset through various logic adjustments.

* **Understeer Enhancement:** Logic such as "FFB Understeer" in Forza or "Enhance Understeer" in Assetto Corsa artificially manipulates the FFB curve. When the physics engine detects that the slip angle has exceeded the SAT peak but not the Force peak, it may flatten the curve or artificially drop the torque to signal the driver. While this helps novice drivers detect the limit, it distorts the "Slope" information, preventing expert drivers from feeling the subtle residual align torque that exists in Zone B.11  
* **Gyroscopic Stabilization:** In Assetto Corsa’s Custom Shaders Patch (CSP), a "Gyro" implementation adds a torque vector based on the wheel's rotation speed and the suspension geometry (caster). This physical force naturally dampens the wheel's oscillation but also modifies the perceived slope of the SAT drop-off. By acting as a dynamic damper, it can make the transition into the slide feel more progressive and controllable, allowing the driver to hold the car in Zone B more effectively.18

## **5\. Optimal Slip Ratio and Longitudinal Dynamics**

While Slope Detection is highly effective for lateral dynamics (steering), its application to longitudinal dynamics (acceleration and braking) via the steering wheel is fraught with physical and implementation challenges.

### **5.1 The Lack of a Torque Vector**

The primary limitation is mechanical: the steering wheel rotates around the steering axis, while longitudinal slip ($\\kappa$) generates forces in the rolling direction. There is no direct kinematic link that causes wheelspin or lock-up to generate a primary torque around the steering column, unlike the strong link for cornering forces.17  
FFB systems must therefore rely on secondary effects or "fake" cues to communicate longitudinal slip slope:

* **Scrub Radius Effects:** If the vehicle has a non-zero scrub radius, the differential in longitudinal forces (e.g., one wheel gripping, one slipping) creates a yaw moment around the kingpin. The FFB logic can detect this "differential slope" and transmit it as a tug on the wheel.21  
* **Vibration Injection:** Most simulators use a threshold-based logic rather than a true slope detection for longitudinal slip. When $\\kappa \> \\kappa\_{opt}$, the system injects a high-frequency vibration (e.g., 50-100Hz) to simulate the "judder" of a locking tire or the "chatter" of wheelspin. This is a binary or stepped cue, not a continuous derivative, and thus has low accuracy for estimating the *optimal* ratio—it only effectively signals when the optimum has been *exceeded*.23

### **5.2 Telemetry-Based Haptics (The Bass Shaker Solution)**

Recognizing the limitations of the steering wheel for longitudinal slip, the sim racing community and developers have turned to supplementary haptics. Software like SimHub utilizes telemetry data to perform true Slope Detection on the longitudinal slip.

* **Logic:** The software monitors the slip ratio $\\kappa \= \\frac{WheelSpeed \- CarSpeed}{CarSpeed}$. It maps the vibration amplitude to this ratio.  
* **Accuracy:** Unlike the steering wheel, which is bandwidth-limited and mechanically decoupled, bass shakers can output a vibration frequency directly proportional to the slip speed. This provides a highly accurate, continuous gradient of information. The "slope" here is perceived as an increase in vibration intensity. By tuning the "gamma" and "threshold" of this response, drivers can create a tactile curve that peaks exactly at $\\kappa\_{opt}$, providing a far more accurate estimation tool than the steering wheel ever could.25

## **6\. Challenges: Signal Processing, Noise, and Latency**

The theoretical efficacy of Slope Detection is constantly at war with the realities of digital signal processing. Calculating the derivative of a signal ($\\frac{dM\_z}{dt}$) is mathematically simple but practically hazardous in a real-time control loop due to noise amplification.

### **6.1 The Derivative Noise Problem**

Differentiation acts as a high-pass filter. High-frequency noise—whether from quantization errors in the physics engine, road texture (macrotexture), or track bumps—has a very steep slope, even if its amplitude is low. When an algorithm attempts to calculate the "Grip Slope" (a low-frequency trend) from the raw FFB signal, it is often swamped by the "Noise Slope" of the road texture.

* **Road Texture Masking:** Research on pavement macrotexture indicates that road surfaces generate significant high-frequency noise.28 In a simulator, if the "Road Effects" gain is set too high, the constant chatter of the texture creates a "jagged" SAT curve. The driver cannot feel the subtle drop-off of the pneumatic trail because it is buried under the high-amplitude noise of the bumps. This leads to the phenomenon of "aliasing," where a bump is misinterpreted as a loss of grip, or a loss of grip is masked by a bump.9

### **6.2 The Latency vs. Filtering Trade-off**

To mitigate noise, FFB systems apply filters. This introduces the most critical challenge in sim racing: Latency.

* **Low-Pass Filters:** A standard moving average or low-pass filter smooths the noise but introduces a phase delay. If the filter delays the signal by 20ms, the driver receives the "loss of grip" information 20ms late. At 200 km/h, this delay can be the difference between catching a slide and crashing.8  
* **Nyquist Limitations:** As noted in technical analyses of iRacing's FFB, the physics engine may run at 360 Hz, meaning the maximum discernible frequency is 180 Hz (Nyquist limit). Any slope changes occurring faster than this (e.g., instantaneous snap oversteer) are aliased or lost. Heavy filtering further lowers this effective bandwidth.8

### **6.3 Advanced Reconstruction Filters (Simucube)**

To solve the Noise-Latency dilemma, high-end hardware manufacturers like Granite Devices (Simucube) have developed "Reconstruction Filters." Unlike standard low-pass filters that simply attenuate high frequencies, reconstruction filters likely use predictive algorithms (e.g., spline interpolation or Kalman-like estimation) to "guess" the intended curve between the discrete data points delivered by the sim.31

* **Slope Preservation:** These filters are designed specifically to preserve the *rate of change* (slope) of the signal while discarding the quantization noise. This allows the SAT drop-off to remain sharp and distinct (perceptible) without the graininess of the raw signal.  
* **Slew Rate Limits:** These drivers also allow users to set a "Slew Rate Limit" (Nm/ms). While intended to prevent violent spikes, setting this too low effectively caps the maximum slope the wheel can reproduce. This artificially flattens the grip drop-off, making the car feel numb and making optimal slip angle estimation impossible. For maximum accuracy, the slew rate must be uncapped, placing the burden of smoothing on the reconstruction filter.8

## **7\. Implementation Case Studies**

The varying approaches to Slope Detection are evident in the architectures of major simulators and middleware.

### **7.1 iRacing and irFFB: The "SOP" Calculation**

iRacing's native FFB is often criticized for being "too pure"—it outputs only the steering column torque derived from the physics, without adding artificial effects to compensate for the lack of seat-of-pants feel. This led to the development of **irFFB**, a middleware utility.

* **SOP (Seat of Pants) Effect:** irFFB calculates a synthetic force based on the vehicle's lateral acceleration and yaw rate. It essentially mixes the "Rear Slip Slope" into the steering torque. This allows the driver to feel the rear tires losing grip (oversteer) through the steering wheel, a cue that is physically impossible to feel through the steering column in a real car (where it is felt through the chassis).  
* **Slope/Gamma Setting:** irFFB includes a "Slope" setting that acts as a gamma correction curve. By making the response non-linear, it amplifies the small forces near the center (steepening the initial slope) and compresses the high forces. This artificially enhances the sensation of the SAT drop-off, making the "lightening" of the wheel more dramatic and easier to detect for users with lower-torque wheels.9

### **7.2 Assetto Corsa: Gyro and Physical Trail**

Assetto Corsa is renowned for its specific handling of the pneumatic trail.

* **Physical Trail Modeling:** AC's tire model explicitly calculates the trail reduction, resulting in a very pronounced drop-off.  
* **Gyro Implementation:** The "More Physically Accurate Gyro" in CSP 18 uses the physics of the spinning wheel mass to generate torque. This acts as a high-frequency filter that naturally creates a "smooth but heavy" feel. It resists rapid changes in direction, which helps stabilize the FFB signal against noise, allowing the "true" slope of the grip loss to shine through without the interference of oscillation.

### **7.3 SimHub: The Telemetry Approach**

SimHub avoids the steering torque limitations entirely by using telemetry data to drive bass shakers.

* **Logic:** It uses the WheelSlip parameter directly.  
* **Effectiveness:** This is the most "accurate" slope detection because it is pure math—it does not pass through a steering rack model, a motor driver, or a gear train. It is a direct visualization (via vibration) of the slip ratio. The "slope" is determined by the Response Curve configured by the user, allowing for a perfectly customized "Optimal Slip" notification system.25

## **8\. Data Summary: Metrics of Slope Detection**

The following table summarizes the effectiveness of Slope Detection across the different domains analyzed:

| Dynamic Metric | Primary FFB Mechanism | Slope Signal | Accuracy | Primary Limitation |
| :---- | :---- | :---- | :---- | :---- |
| **Grip Level ($\\mu$)** | Cornering Stiffness ($C\_\\alpha$) | Stiffness of force build-up (Linear Region Slope) | **High** (Relative) | Load ($F\_z$) sensitivity can mask friction changes. |
| **Optimal Slip Angle** | SAT Drop-off | Transition from Positive to Negative Slope ($\\frac{dM\_z}{d\\alpha} \< 0$) | **Moderate** | **Offset Error:** SAT peaks before Grip peaks. Driver must "drive past the peak." |
| **Optimal Slip Ratio** | Vibration / Scrub | Threshold Trigger or Artificial Noise | **Low** (Steering) | No mechanical torque link. Relies on "canned" effects or external haptics. |
| **Limit of Adhesion** | SAT Collapse | Rapid Negative Slope | **High** | Signal Latency and Road Texture Noise (Aliasing). |

## **9\. Conclusion**

The "Slope Detection" logic in FFB systems is a sophisticated, albeit imperfect, heuristic for estimating vehicle states. It is not a direct measurement of grip, but rather a measurement of the *symptoms* of grip mechanics—specifically the migration of the pneumatic trail and the reduction in cornering stiffness.

The accuracy of this logic is highest when estimating the **Limit of Adhesion**. The physical phenomenon of SAT drop-off provides a reliable, high-contrast signal that warns the driver of impending grip loss. However, its accuracy in estimating the **Optimal Slip Angle** is structurally compromised by the physical offset between the aligning torque peak and the lateral force peak. Simulation drivers must learn to interpret the "negative slope" not as a failure of grip, but as the zone of maximum performance—a counter-intuitive skill that separates elite sim racers from novices.

The greatest challenges to this logic remain **Signal-to-Noise Ratio** and **Latency**. The derivative nature of slope detection makes it inherently sensitive to high-frequency road texture noise. The future of high-fidelity simulation lies in the divergence of these signals: using "Reconstruction Filters" and "Gyro" stabilization to purify the low-frequency steering torque (SAT) for handling precision, while offloading the high-frequency texture and slip vibration to multimodal haptic systems (active pedals, chassis transducers). This separation ensures that the critical "Slope" of the grip limit is never masked by the noise of the road, providing the operator with a deterministic and accurate interface for vehicle control.

#### **Works cited**

1. The Absolute Guide to Racing Tires \- Part 1 \- Lateral Force, accessed December 27, 2025, [https://racingcardynamics.com/racing-tires-lateral-force/](https://racingcardynamics.com/racing-tires-lateral-force/)  
2. Online Slip Detection and Friction Coefficient Estimation for Autonomous Racing \- arXiv, accessed December 27, 2025, [https://arxiv.org/html/2509.15423v2](https://arxiv.org/html/2509.15423v2)  
3. Online Slip Detection and Friction Coefficient Estimation for Autonomous Racing \- arXiv, accessed December 27, 2025, [https://arxiv.org/html/2509.15423v1](https://arxiv.org/html/2509.15423v1)  
4. Tyre Slip Angle – Geometry Explained Suspension Secrets, accessed December 27, 2025, [https://suspensionsecrets.co.uk/tyre-slip-angle/](https://suspensionsecrets.co.uk/tyre-slip-angle/)  
5. Tire Characteristics Sensitivity Study \- Chalmers Publication Library, accessed December 27, 2025, [https://publications.lib.chalmers.se/records/fulltext/162882.pdf](https://publications.lib.chalmers.se/records/fulltext/162882.pdf)  
6. Tech Explained: Steering Forces \- Racecar Engineering, accessed December 27, 2025, [https://www.racecar-engineering.com/tech-explained/tech-explained-steering-forces/](https://www.racecar-engineering.com/tech-explained/tech-explained-steering-forces/)  
7. On-Board Road Friction Estimation Technique for Autonomous Driving Vehicle-Following Maneuvers \- MDPI, accessed December 27, 2025, [https://www.mdpi.com/2076-3417/11/5/2197](https://www.mdpi.com/2076-3417/11/5/2197)  
8. Force Feedback Settings \- Why you are crashing... : r/iRacing \- Reddit, accessed December 27, 2025, [https://www.reddit.com/r/iRacing/comments/1kfe7fe/force\_feedback\_settings\_why\_you\_are\_crashing/](https://www.reddit.com/r/iRacing/comments/1kfe7fe/force_feedback_settings_why_you_are_crashing/)  
9. iRacing FFB Configuration \- Byte Insight, accessed December 27, 2025, [https://byteinsight.co.uk/2023/04/iracing-wheel-configuration-options/](https://byteinsight.co.uk/2023/04/iracing-wheel-configuration-options/)  
10. Enhanced understeer effect on or off? : r/assettocorsa \- Reddit, accessed December 27, 2025, [https://www.reddit.com/r/assettocorsa/comments/4g3flx/enhanced\_understeer\_effect\_on\_or\_off/](https://www.reddit.com/r/assettocorsa/comments/4g3flx/enhanced_understeer_effect_on_or_off/)  
11. FH4: Wheel Setup and Tuning \- Forza Support, accessed December 27, 2025, [https://support.forzamotorsport.net/hc/en-us/articles/360002007867-FH4-Wheel-Setup-and-Tuning](https://support.forzamotorsport.net/hc/en-us/articles/360002007867-FH4-Wheel-Setup-and-Tuning)  
12. Change the "enhance understeer effect" to an assist | Kunos Simulazioni \- Official Forum, accessed December 27, 2025, [https://www.assettocorsa.net/forum/index.php?threads/change-the-enhance-understeer-effect-to-an-assist.31339/](https://www.assettocorsa.net/forum/index.php?threads/change-the-enhance-understeer-effect-to-an-assist.31339/)  
13. Vehicle Setup and Kinematics | Q\&A Series \- OptimumG, accessed December 27, 2025, [https://optimumg.com/vehicle-setup-and-kinematics-qa-series/](https://optimumg.com/vehicle-setup-and-kinematics-qa-series/)  
14. Self Aligning Torque | DrRacing's Blog \- WordPress.com, accessed December 27, 2025, [https://drracing.wordpress.com/2013/10/27/self-aligning-torque/](https://drracing.wordpress.com/2013/10/27/self-aligning-torque/)  
15. Mr Deap's sim racing introduction guide \- Steam Community, accessed December 27, 2025, [https://steamcommunity.com/sharedfiles/filedetails/?l=french\&id=912315488](https://steamcommunity.com/sharedfiles/filedetails/?l=french&id=912315488)  
16. Physics Modding | PDF | Euclidean Vector | Matrix (Mathematics) \- Scribd, accessed December 27, 2025, [https://www.scribd.com/document/813097469/Physics-Modding](https://www.scribd.com/document/813097469/Physics-Modding)  
17. Tyre dynamics \- Racecar Engineering, accessed December 27, 2025, [https://www.racecar-engineering.com/tech-explained/tyre-dynamics/](https://www.racecar-engineering.com/tech-explained/tyre-dynamics/)  
18. Logitech pro wheel, Force feedback, graphics, and chatgpt4 in assetto corsa (experiment), accessed December 27, 2025, [https://www.reddit.com/r/assettocorsa/comments/1jsf5za/logitech\_pro\_wheel\_force\_feedback\_graphics\_and/](https://www.reddit.com/r/assettocorsa/comments/1jsf5za/logitech_pro_wheel_force_feedback_graphics_and/)  
19. WSC Legends 60's Pack | THRacing, accessed December 27, 2025, [https://thracing.de/wp-content/uploads/2025/01/2025-01-WSC-Legends-60s-Pack-v1.2.1.pdf](https://thracing.de/wp-content/uploads/2025/01/2025-01-WSC-Legends-60s-Pack-v1.2.1.pdf)  
20. Self aligning torque \- Wikipedia, accessed December 27, 2025, [https://en.wikipedia.org/wiki/Self\_aligning\_torque](https://en.wikipedia.org/wiki/Self_aligning_torque)  
21. Is there a formula to calculate Slip Angle for each tire? : r/FSAE \- Reddit, accessed December 27, 2025, [https://www.reddit.com/r/FSAE/comments/uz99vh/is\_there\_a\_formula\_to\_calculate\_slip\_angle\_for/](https://www.reddit.com/r/FSAE/comments/uz99vh/is_there_a_formula_to_calculate_slip_angle_for/)  
22. An investigatory study into improving vehicle control by the use of direct real time slip angle sensing \- Sign in, accessed December 27, 2025, [https://pure.coventry.ac.uk/ws/portalfiles/portal/42231101/Sriskantha2016.pdf](https://pure.coventry.ac.uk/ws/portalfiles/portal/42231101/Sriskantha2016.pdf)  
23. WRC 9 FFB......My Deep Dive to Decipher What is Going On : r/simrally \- Reddit, accessed December 27, 2025, [https://www.reddit.com/r/simrally/comments/miqolh/wrc\_9\_ffbmy\_deep\_dive\_to\_decipher\_what\_is\_going\_on/](https://www.reddit.com/r/simrally/comments/miqolh/wrc_9_ffbmy_deep_dive_to_decipher_what_is_going_on/)  
24. This is how the FFB works--and how to Set it up : r/assettocorsa \- Reddit, accessed December 27, 2025, [https://www.reddit.com/r/assettocorsa/comments/5l64g8/this\_is\_how\_the\_ffb\_worksand\_how\_to\_set\_it\_up/](https://www.reddit.com/r/assettocorsa/comments/5l64g8/this_is_how_the_ffb_worksand_how_to_set_it_up/)  
25. ShakeIt V3 Effects configuration · SHWotever/SimHub Wiki · GitHub, accessed December 27, 2025, [https://github.com/SHWotever/SimHub/wiki/ShakeIt-V3-Effects-configuration](https://github.com/SHWotever/SimHub/wiki/ShakeIt-V3-Effects-configuration)  
26. SimHub ShakeIt Slip/Grip haptic discussion \- blekenbleu, accessed December 27, 2025, [https://blekenbleu.github.io/pedals/ShakeIt/SG.htm](https://blekenbleu.github.io/pedals/ShakeIt/SG.htm)  
27. Telemetry Outputs Overview | DR Sim Manager, accessed December 27, 2025, [https://docs.departedreality.com/dr-sim-manager/development/telemetry-outputs-overview](https://docs.departedreality.com/dr-sim-manager/development/telemetry-outputs-overview)  
28. Evolution of Pavement Friction and Macrotexture After Asphalt Overlay \- Connect NCDOT, accessed December 27, 2025, [https://connect.ncdot.gov/projects/research/RNAProjDocs/2020-11%20FinalReport.pdf](https://connect.ncdot.gov/projects/research/RNAProjDocs/2020-11%20FinalReport.pdf)  
29. Influencing Parameters on Tire–Pavement Interaction Noise: Review, Experiments, and Design Considerations \- MDPI, accessed December 27, 2025, [https://www.mdpi.com/2411-9660/2/4/38](https://www.mdpi.com/2411-9660/2/4/38)  
30. Logitech G27: "Damping", "Filter", "Minimum Force",... What do they all mean?? \- Reddit, accessed December 27, 2025, [https://www.reddit.com/r/simracing/comments/2rinrw/logitech\_g27\_damping\_filter\_minimum\_force\_what\_do/](https://www.reddit.com/r/simracing/comments/2rinrw/logitech_g27_damping_filter_minimum_force_what_do/)  
31. Reconstruction Filter V2 testing \- Simucube 2 \- Granite Devices Community, accessed December 27, 2025, [https://community.granitedevices.com/t/reconstruction-filter-v2-testing/11612](https://community.granitedevices.com/t/reconstruction-filter-v2-testing/11612)  
32. Simucube 2 Effects, accessed December 27, 2025, [https://docs.simucube.com/TunerSoftware/wheelbases/wheelbaseeffects.html](https://docs.simucube.com/TunerSoftware/wheelbases/wheelbaseeffects.html)  
33. Simucube 2 \- Sport / Pro / Ultimate User Guide, accessed December 27, 2025, [https://simucube.com/app/uploads/2022/11/Simucube\_2\_User\_Guide.pdf](https://simucube.com/app/uploads/2022/11/Simucube_2_User_Guide.pdf)  
34. With Calculations of Delay | PDF | Transport \- Scribd, accessed December 27, 2025, [https://www.scribd.com/document/769242033/WITH-CALCULATIONS-OF-DELAY](https://www.scribd.com/document/769242033/WITH-CALCULATIONS-OF-DELAY)  
35. Suspended for “Tanking”?\!?\!?\! (Details in comments) : r/iRacing \- Reddit, accessed December 27, 2025, [https://www.reddit.com/r/iRacing/comments/n1yf5q/suspended\_for\_tanking\_details\_in\_comments/](https://www.reddit.com/r/iRacing/comments/n1yf5q/suspended_for_tanking_details_in_comments/)