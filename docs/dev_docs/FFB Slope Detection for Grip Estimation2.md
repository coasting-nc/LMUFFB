# **Algorithmic Estimation of Tire Grip and Optimal Slip Dynamics in Force Feedback Systems: A Comprehensive Analysis of Slope Detection Logic**

## **1\. Introduction: The Haptic Bridge in Virtual Motorsport**

The fundamental challenge of high-fidelity motorsport simulation lies in the translation of complex physical interactions into communicative sensory inputs. In a real-world racing environment, a driver operates within a rich tapestry of sensory data: vestibular cues from g-forces, auditory feedback from tire scrub and engine load, and tactile feedback through the chassis and steering column. In the virtual domain, particularly for fixed-base simulators, the vestibular channel is absent. Consequently, the burden of communicating the vehicle's dynamic state—specifically the limit of adhesion—falls disproportionately on the visual display and, most critically, the haptic interface of the Force Feedback (FFB) steering system.

The "Slope Detection logic" represents a sophisticated class of signal processing algorithms designed to bridge this sensory gap. By analyzing the rate of change—the derivative or slope—of telemetry signals such as tire force, slip angle, and slip ratio, these algorithms attempt to predict and communicate the precise moment of grip loss. The objective is to provide the sim racer with a tactile "early warning system" that mimics the natural drop in steering weight associated with pneumatic trail collapse, thereby enabling the driver to navigate the optimal slip zone with precision.

This report provides an exhaustive technical evaluation of the effectiveness, accuracy, and inherent limitations of Slope Detection logic. It synthesizes theoretical tire mechanics, digital signal processing principles, and empirical data from major simulation platforms (Assetto Corsa, rFactor 2, iRacing, and Automobilista 2\) to determine whether gradient-based estimation is a viable proxy for true grip limits. The analysis explores the bifurcated nature of tire dynamics—distinguishing between the Self-Aligning Torque (SAT) perceived by the driver and the true Lateral Force ($F\_y$) generating the turn—and how algorithmic interpretations of these forces are compromised by signal noise, latency, and environmental non-linearities.

## **2\. Theoretical Foundations: Tire Physics and Haptic Translation**

To evaluate the efficacy of any algorithm attempting to estimate grip, one must first establish the ground truth of how tire forces are generated and subsequently transmitted to the steering rack. The "slope" being detected is not merely a mathematical abstraction but a direct representation of the physical deformation of the tire carcass and the contact patch.

### **2.1 The Mechanics of Slip Angle and Lateral Force**

A pneumatic tire does not generate cornering force by pointing the vehicle in a new direction; rather, it generates force through elastic deformation. As the wheel is steered, the tire carcass twists, and the contact patch distorts. The angle between the direction the wheel is pointing and the direction the contact patch is traveling is defined as the **slip angle** ($\\alpha$).1

The relationship between slip angle and lateral force ($F\_y$) is non-linear and is described by models such as the Pacejka Magic Formula or the brush model. This relationship is foundational to understanding what the Slope Detection logic is attempting to measure. The curve is characterized by three distinct regions, each defined by the gradient (slope) of the force:

1. **The Linear (Elastic) Region:** At low slip angles (typically 0° to 2-3° for racing tires), the tire behaves like a linear spring. The entire contact patch maintains adhesion with the road surface. In this zone, the lateral force increases linearly with slip angle. The slope of this curve, $\\frac{dF\_y}{d\\alpha}$, is constant and is referred to as the **Cornering Stiffness** ($C\_{\\alpha}$).3 For the driver, this is felt as a predictable, linear build-up of steering weight.  
2. **The Transitional Region:** As slip angle increases further, the shear stress at the rear of the contact patch exceeds the coefficient of static friction, and localized sliding begins. The contact patch is now in a mixed state: the front is gripping (adhesion), while the rear is sliding (hysteresis). Crucially, the rate of force accumulation diminishes. The slope $\\frac{dF\_y}{d\\alpha}$ begins to decrease. This "knee" in the curve is the first physical signal that the tire is approaching its limit.4  
3. **The Frictional (Saturation) Region:** Eventually, the entire contact patch enters a sliding state. The lateral force reaches its peak ($F\_{y,max}$) and then plateaus or drops off. At the exact peak of grip, the slope of the force curve is zero ($\\frac{dF\_y}{d\\alpha} \= 0$). Beyond the peak, the slope becomes negative, indicating a loss of grip despite increased steering input.4

Slope Detection logic, in its purest form, monitors the telemetry stream for this transition from a positive slope (grip building) to a zero or negative slope (grip limit reached/exceeded).

### **2.2 The Self-Aligning Torque (SAT) vs. Lateral Force Offset**

A critical distinction must be made between the force the tire generates ($F\_y$) and the torque the driver feels ($M\_z$, or Self-Aligning Torque). This distinction is the single most important factor in analyzing the accuracy of FFB-based grip estimation.

The Self-Aligning Torque is the product of the Lateral Force and the **Pneumatic Trail** ($t\_p$) plus the Mechanical Trail ($t\_m$).6

$$M\_z \= F\_y \\cdot (t\_p \+ t\_m)$$  
The Pneumatic Trail ($t\_p$) is the distance between the geometric center of the contact patch and the centroid of the lateral force distribution.

* **Low Slip:** At low slip angles, the force distribution is roughly triangular, peaking near the rear of the contact patch due to the accumulation of elastic deformation. This creates a large pneumatic trail, resulting in strong aligning torque (heavy steering).  
* **High Slip:** As the rear of the contact patch begins to slide, the area of adhesion shrinks and moves forward. Consequently, the centroid of lateral force migrates toward the center of the wheel. The pneumatic trail ($t\_p$) decreases rapidly.8

**The SAT-Fy Offset Phenomenon:** Because the pneumatic trail collapses *while* the lateral force is still increasing, the Self-Aligning Torque ($M\_z$) peaks and begins to drop **before** the Lateral Force ($F\_y$) reaches its maximum.4

* **Quantitative Insight:** Empirical data from tire testing indicates that for a typical racing slick, the SAT might peak at a slip angle of 4° to 6°, while the Lateral Force peaks at 7° to 10°.1  
* **Implication for Slope Detection:** An algorithm that detects the "slope drop" in the FFB signal is detecting the saturation of the *aligning torque*, not the saturation of the *grip*. This means the logic inherently **underestimates** the optimal slip angle. However, this underestimation is functionally desirable in a racing context because the SAT peak represents the limit of *stability*, whereas the $F\_y$ peak represents the limit of *adhesion* (often on a razor's edge of control).

### **2.3 Longitudinal Dynamics and Slip Ratio**

While lateral dynamics govern cornering, longitudinal dynamics govern braking and acceleration. Here, the variable of interest is the **Slip Ratio** ($\\kappa$), defined as the normalized difference between the angular velocity of the wheel ($\\omega$) and the longitudinal velocity of the vehicle ($v$).2

$$\\kappa \= \\frac{r \\omega \- v}{v}$$  
The friction curve for longitudinal force ($F\_x$) vs. slip ratio follows a similar trajectory to the lateral curve: a linear rise, a peak (typically at 10-20% slip for racing tires), and a decline.12

The "Viper" calculation method referenced in the research 11 focuses on detecting the slope of this longitudinal relationship. By monitoring the ratio of Tyre RPS to Car Speed, the logic attempts to identify the point where an increase in engine RPM (or brake pressure) no longer results in a proportional increase in vehicle acceleration (or deceleration).

* **Mechanism:** The logic continuously calculates the derivative $\\frac{dF\_x}{d\\kappa}$. A high positive value indicates effective traction. A value near zero indicates the optimal slip ratio ($\\kappa\_{opt}$). A negative value indicates excessive wheel spin or lock-up.  
* **Application:** This data is typically fed into tactile transducers (bass shakers) to vibrate the pedals or seat, alerting the driver that they have passed the peak of the longitudinal friction curve.13

## **3\. Algorithmic Implementation of Slope Detection Logic**

To understand the effectiveness of Slope Detection, one must examine how it is implemented in software. The logic is rarely a direct analytical derivative of a perfect function; instead, it is a discrete-time signal processing challenge operating on noisy telemetry data.

### **3.1 The Fundamental Algorithm**

The core operation is numerical differentiation. Let $S\[n\]$ be the signal of interest (e.g., Tire Load, FFB Torque, or Slip Angle) at frame $n$, and $\\Delta t$ be the time step. The instantaneous slope $k$ is approximated as:

$$k\[n\] \\approx \\frac{S\[n\] \- S\[n-1\]}{\\Delta t}$$  
In the context of FFB, the "Slope Detection" logic typically monitors the gradient of the FFB output relative to the steering input or the lateral G-force.

* **Linear Grip Zone:** If the steering angle increases by $\\delta$ and the FFB torque increases by a proportional amount $\\tau$, the ratio $\\tau/\\delta$ (the slope) is constant. The logic passes the raw physics signal to the wheel with linear gain.  
* **Grip Loss Zone:** If the steering angle increases by $\\delta$ but the FFB torque increases by less than the proportional amount (or decreases), the slope $\\tau/\\delta$ drops. The logic detects this deviation from linearity.

Some applications, such as FFBClip for Assetto Corsa, use this detection to manage the dynamic range of the force feedback.15 If the slope approaches zero (indicating the FFB signal is clipping or the tire is saturating), the app dynamically reduces the gain to ensure the "detail" of the drop-off is preserved, rather than being lost to a flat-lined signal.

### **3.2 The "Viper" Method: A Case Study in Longitudinal Estimation**

The "Viper" plugin 11 serves as a prime example of implementing slope detection for longitudinal slip. It addresses the complexity of defining "zero slip" in a dynamic environment.

* **Calibration Phase:** The algorithm requires a baseline. It monitors the wheel speeds during a phase of low lateral and longitudinal acceleration (straight-line cruising) to calculate the effective rolling radius of the tire. This is crucial because tire radius is not constant; it changes with pressure and wear.  
* **Real-Time Computation:** Once calibrated, the logic computes the slip ratio $\\kappa$ in real-time. It compares this against pre-defined thresholds or monitors the rate of change. If $\\frac{d\\omega}{dt}$ spikes without a corresponding $\\frac{dv}{dt}$, the slope of the traction curve has essentially become vertical (infinite slip), triggering the "loss of grip" flag.  
* **Integration:** The result is output as a property (e.g., Viper.LngWheelSlip) which drives visual dashboards (red for lock, blue for spin) or haptic shakers.11

### **3.3 Artificial Enhancement and "Canned" Slopes**

A significant variation of Slope Detection is the *artificial* enforcement of slope characteristics. In simulations where the natural pneumatic trail drop-off is subtle (or masked by heavy mechanical trail/caster), developers or modders use scripts to exaggerate the slope.

* **Assetto Corsa Scripts:** The "Alternative FFB" scripts or custom LUTs (Look-Up Tables) allow users to define a custom curve for the FFB output.18  
* **Mechanism:** The script monitors the slip angle telemetry. It calculates how far the current slip angle is from the theoretical optimal slip angle ($\\alpha\_{opt}$).  
  * If $\\alpha \< \\alpha\_{opt}$: Output \= Physics Force.  
  * If $\\alpha \> \\alpha\_{opt}$: Output \= Physics Force $\\times$ Decay Factor.  
* **Effectiveness:** This artificial slope detection is highly effective for *training* because it creates a distinct, unmistakable tactile cue (the wheel going light) exactly when the tire exceeds the optimal angle. However, it is physically inaccurate, as it imposes a synthetic "cliff" where the real tire might have a broad, forgiving plateau.20

## **4\. Signal Processing Challenges and Limitations**

While the physics of slope detection are sound, the practical implementation in a real-time loop (typically running at 60Hz to 400Hz depending on the sim and telemetry export) is fraught with challenges. The primary adversary is noise.

### **4.1 The Derivative-Noise Dilemma**

Numerical differentiation is an "ill-posed" problem in signal processing. The derivative operator amplifies high-frequency noise.22

* **The Math:** If a signal $x(t)$ is corrupted by noise $\\epsilon(t)$, the derivative is $x'(t) \+ \\epsilon'(t)$. Since noise often fluctuates rapidly (high frequency), its derivative $\\epsilon'(t)$ can be orders of magnitude larger than the derivative of the actual physical signal $x'(t)$.  
* **The Sim Racing Context:** Telemetry data from simulators is discrete and often quantized. A tire load signal might jump from 3000N to 3005N to 2995N due to micro-oscillations in the suspension physics or "jitter" in the data export.24 A raw derivative calculation would interpret these micro-jumps as massive changes in slope, triggering false positives for grip loss.  
* **Consequence:** A naive slope detection algorithm will produce a "jagged" output, causing the FFB to rattle or the bass shakers to buzz incessantly, completely masking the useful signal.

### **4.2 Latency and The Filtering Trade-Off**

To combat noise, filtering is mandatory. However, all causal filters introduce phase shift (latency).26

* **Moving Average/Low-Pass:** These are the simplest filters but introduce significant lag. If a slope detection algorithm uses a 100ms moving average window to smooth the FFB signal, the detection of the "grip drop" will be delayed by \~50ms.  
* **Impact:** In a high-speed slide, 50ms is an eternity. By the time the smoothed slope detection logic realizes the grip is gone and lightens the wheel, the car may have already rotated beyond the point of recovery (the "tank slapper" scenario).28  
* **Savitzky-Golay Filtering:** Research indicates that Savitzky-Golay (SG) filters are superior for this specific application.29 The SG filter fits a low-degree polynomial to the data window via least squares. Unlike a moving average, which flattens peaks, the SG filter preserves the *shape* and *height* of the signal peaks (like the sharp drop in pneumatic trail) while suppressing noise. This makes it the preferred method for accurate slope detection in advanced FFB tools, though it still incurs a computational cost and some latency.30

### **4.3 The "Plateau" Problem and Peak Ambiguity**

The effectiveness of slope detection is heavily dependent on the shape of the tire's friction curve.

* **Peaky Curves (Slicks):** Racing slicks often have a sharp peak followed by a steep drop-off. The slope changes dramatically from positive to negative. Slope detection works exceptionally well here, providing a clear "snap" in the FFB.31  
* **Plateau Curves (Street/Wet Tires):** Street tires and wet weather tires often exhibit a broad "plateau" where grip is relatively constant over a wide range of slip angles.31 In this region, the slope is near zero for a long time.  
* **Failure Mode:** Slope detection logic struggles with plateaus. It detects "zero slope" (peak grip) but cannot easily tell the driver whether they are at the *start* of the plateau (safe) or the *edge* of the plateau (danger). In wet conditions, where the curve flattens significantly, slope detection algorithms tuned for dry tracks often fail to provide any meaningful warning before traction is lost entirely.32

## **5\. Comparative Effectiveness Across Simulators**

The accuracy of Slope Detection is not uniform; it varies significantly based on the underlying physics engine and the data it exposes.

### **5.1 Assetto Corsa (AC) and Assetto Corsa Competizione (ACC)**

* **Data Availability:** AC provides high transparency, exposing static tire curve data (LUTs) in files like tyres.ini.33 This allows apps like "Sidekick" to compare real-time telemetry against the *known* theoretical peak slope. This creates a "perfect" reference model, making slope detection highly accurate relative to the game's internal math.  
* **ACC Physics:** ACC utilizes a 5-point tire model that is dynamic, affected by pressure, flex, and carcass heating.35 The "slope" here is emergent and variable.  
* **FFB Implementation:** Kunos (the developer) implements a "dynamic damping" and "enhanced understeer" effect. The slope detection is effectively built into the FFB engine itself. When the front tires scrub, the engine artificially reduces the SAT force to ensure the driver feels the understeer, even if the physical SAT drop-off would be subtle.15 This makes slope detection *highly effective* for the user, even if it involves some "canned" enhancement.

### **5.2 rFactor 2 (rF2) and Le Mans Ultimate (LMU)**

* **Physical Model:** rF2 uses a complex physical brush model (TGM files) where the contact patch is simulated as independent bristles.35  
* **Signal Characteristics:** The FFB output is pure steering rack force, updated at high frequency (up to 720Hz). This signal is physically accurate but extremely noisy due to the simulation of individual bristle interactions and road surface details.  
* **Slope Detection:** Detecting the "grip slope" in rF2 is challenging due to the "flat spot" phenomenon. If a tire is flat-spotted, it generates a periodic vibration spike.39 A simple derivative-based algorithm will interpret this vibration as rapid changes in grip slope, potentially causing the FFB to clip or oscillate. Effective slope detection here requires advanced filtering to separate the "macro" grip trend from the "micro" surface/vibration noise.

### **5.3 iRacing**

* **Philosophy:** iRacing adheres to a strict "steering column torque only" philosophy. It does not add artificial effects to enhance the feeling of grip loss.39  
* **Limitation:** On cars with high caster or power steering, the natural drop in pneumatic trail (the slope change) can be very subtle. Unlike AC, there is no "enhanced understeer" variable to amplify this.  
* **Effectiveness:** External apps (like "Iris" or "Viper") have limited ability to inject slope-based effects because iRacing's API is more restrictive regarding FFB modification. The slope detection logic is therefore less effective as a real-time haptic aid in iRacing compared to other titles, often necessitating reliance on visual tire noise or volume cues.37

## **6\. Quantitative Accuracy Assessment**

How accurately does the logic estimate the key performance parameters?

### **Table 1: Accuracy of Slope Detection Logic for Dynamic Parameters**

| Parameter | Effectiveness | Accuracy | Primary Challenge |
| :---- | :---- | :---- | :---- |
| **Grip Level ($F\_{max}$)** | **Low** | **Low** | **Load Sensitivity:** Slope detection identifies the *shape* of the curve, not the absolute amplitude. It cannot tell if the peak is at 1000N or 5000N without knowing the vertical load ($F\_z$), which varies constantly with aero and weight transfer.42 |
| **Optimal Slip Angle ($\\alpha\_{opt}$)** | **High** | **Biased (Safe)** | **SAT-Fy Offset:** The logic detects the SAT peak, which occurs 2-4 degrees *before* the true lateral force peak. It consistently underestimates $\\alpha\_{opt}$. This is "inaccurate" physically but "effective" for stability.4 |
| **Optimal Slip Ratio ($\\kappa\_{opt}$)** | **Moderate** | **Moderate** | **Radius Dynamics:** Accurate calculation requires knowing the exact tire radius, which changes with speed (growth) and pressure. Errors in radius calculation lead to offset slip ratio estimations.11 |

### **6.1 The Impact of Environmental Variables**

The accuracy of slope detection is further compromised by environmental factors that shift the tire curves:

* **Camber Thrust:** Negative camber increases lateral thrust and shifts the peak of the curve. A slope detection algorithm calibrated for 0° camber will be inaccurate for a car running \-3.5° camber, as the "slope" profile changes asymmetrically (turning left vs. right).44  
* **Temperature & Wear:** As tires heat up or wear, the cornering stiffness ($C\_{\\alpha}$) changes. The slope of the linear region decreases (the tire becomes "spongier"). A static threshold algorithm (e.g., "trigger rumble at slope \< 0.5") will trigger earlier and earlier as the tire degrades, potentially giving the driver false feedback that they are losing grip when they are actually just experiencing thermal softening.33

## **7\. Advanced Solutions: Beyond Simple Derivatives**

Given the limitations of simple derivative-based slope detection, the industry and community are moving toward more robust estimation methods.

### **7.1 Kalman Filtering**

The Extended Kalman Filter (EKF) represents the state-of-the-art for vehicle state estimation.47

* **Mechanism:** Instead of just trusting the noisy sensor (telemetry), the EKF uses a predictive model of the vehicle dynamics. It predicts what the slip angle *should* be, compares it to the measurement, and updates the estimate based on a weighted trust factor (covariance).  
* **Advantage:** EKF can provide a smooth, noise-free estimate of the slip slope without the phase lag of a low-pass filter. It can also estimate "hidden" states like true friction coefficient ($\\mu$) even when the signal is noisy.49  
* **Challenge:** Implementing an EKF requires a fairly accurate model of the vehicle (mass, wheelbase, tire stiffness). Generic FFB apps often lack this specific data for every car in a simulation, making EKF difficult to generalize.

### **7.2 Neural Networks and Machine Learning**

Recent research suggests using Neural Networks to infer grip limits.48

* **Approach:** A neural network can be trained on datasets of telemetry to recognize the complex, non-linear patterns that precede grip loss—patterns that a simple "slope \< threshold" logic might miss.  
* **Potential:** These models could account for the interplay of temperature, pressure, and aero load to provide a highly accurate "Percent Grip Remaining" metric, far surpassing the accuracy of simple slope detection.51

## **8\. Conclusion**

Slope Detection logic is a vital, albeit imperfect, tool in the sim racing ecosystem. Its **effectiveness** is high in regards to providing immersive, actionable feedback: it successfully translating the abstract mathematics of tire saturation into the intuitive sensation of "steering lightness" that drivers rely on to find the limit. By detecting the collapse of pneumatic trail (the SAT peak), it provides a built-in safety margin that encourages stable, consistent driving.

However, its **accuracy** as a measurement tool is fundamentally limited. It systematically underestimates the true optimal slip angle due to the SAT-Fy offset, effectively guiding drivers to the limit of *feel* rather than the limit of *force*. Furthermore, its reliance on numerical differentiation makes it highly susceptible to signal noise, necessitating filtering that introduces detrimental latency.

For the professional peer or developer, the key takeaway is that Slope Detection should not be viewed as a standalone solution for grip estimation. It must be integrated with adaptive filtering (like Savitzky-Golay) and context-aware logic that accounts for load sensitivity and environmental conditions. The future of high-fidelity FFB lies not in better derivative calculations, but in model-based estimation (Kalman Filters) and data-driven approaches (Neural Networks) that can interpret the noisy language of tire physics with greater nuance than a simple slope gradient.

#### **Works cited**

1. Tyre Slip Angle – Geometry Explained Suspension Secrets, accessed December 27, 2025, [https://suspensionsecrets.co.uk/tyre-slip-angle/](https://suspensionsecrets.co.uk/tyre-slip-angle/)  
2. Tyre dynamics \- Racecar Engineering, accessed December 27, 2025, [https://www.racecar-engineering.com/tech-explained/tyre-dynamics/](https://www.racecar-engineering.com/tech-explained/tyre-dynamics/)  
3. The tire modeling fundamentals – Part 1 \- Automotive Papers, accessed December 27, 2025, [https://automotivepapers.com/2023/01/28/the-tire-modeling-fundamentals-part-1/](https://automotivepapers.com/2023/01/28/the-tire-modeling-fundamentals-part-1/)  
4. The Absolute Guide to Racing Tires \- Part 1 \- Lateral Force \- Racing Car Dynamics, accessed December 27, 2025, [http://racingcardynamics.com/racing-tires-lateral-force/](http://racingcardynamics.com/racing-tires-lateral-force/)  
5. The Absolute Guide to Racing Tires \- Part 1 \- Lateral Force \- Racing Car Dynamics, accessed December 27, 2025, [https://racingcardynamics.com/racing-tires-lateral-force/](https://racingcardynamics.com/racing-tires-lateral-force/)  
6. Pneumatic trail vs mechanical trail \- TenTenths Motorsport Forum, accessed December 27, 2025, [https://tentenths.com/forum/showthread.php?goto=newpost\&t=93916](https://tentenths.com/forum/showthread.php?goto=newpost&t=93916)  
7. Self Aligning Torque | DrRacing's Blog \- WordPress.com, accessed December 27, 2025, [https://drracing.wordpress.com/2013/10/27/self-aligning-torque/](https://drracing.wordpress.com/2013/10/27/self-aligning-torque/)  
8. Self-Aligning Torque Explained: Understanding Tire Dynamics \- SimpleTire, accessed December 27, 2025, [https://simpletire.com/learn/tire-news-information/self-aligning-torque](https://simpletire.com/learn/tire-news-information/self-aligning-torque)  
9. FMR: Yet another rFactor mod (also, pneumatic trail) | Nerdgasm \- WordPress.com, accessed December 27, 2025, [https://needlesslyobscure.wordpress.com/2010/07/06/fmr-yet-another-rfactor-mod-also-pneumatic-trail/](https://needlesslyobscure.wordpress.com/2010/07/06/fmr-yet-another-rfactor-mod-also-pneumatic-trail/)  
10. How to interpret ffb? : r/simracing \- Reddit, accessed December 27, 2025, [https://www.reddit.com/r/simracing/comments/52vmli/how\_to\_interpret\_ffb/](https://www.reddit.com/r/simracing/comments/52vmli/how_to_interpret_ffb/)  
11. viper4gh/SimHub-Plugin-CalcLngWheelSlip: Plugin for SimHub. It calculates longitudinal wheel slip by the relationship between Tyre RPS and Car Speed and provides the result as new properties. \- GitHub, accessed December 27, 2025, [https://github.com/viper4gh/SimHub-Plugin-CalcLngWheelSlip](https://github.com/viper4gh/SimHub-Plugin-CalcLngWheelSlip)  
12. Tires \- Vehicle Physics Pro, accessed December 27, 2025, [https://vehiclephysics.com/blocks/tires/](https://vehiclephysics.com/blocks/tires/)  
13. Simhub \- bass shakers Wheelslip | Page 3 \- Reiza Studios Forum, accessed December 27, 2025, [https://forum.reizastudios.com/threads/simhub-bass-shakers-wheelslip.30366/page-3](https://forum.reizastudios.com/threads/simhub-bass-shakers-wheelslip.30366/page-3)  
14. Racingrig – Tactile Feedback – Michis F-16 Simulator, accessed December 27, 2025, [https://f16simulator.net/wp/en/2020/05/18/racingrig-tactile-feedback/](https://f16simulator.net/wp/en/2020/05/18/racingrig-tactile-feedback/)  
15. This is how the FFB works--and how to Set it up : r/assettocorsa \- Reddit, accessed December 27, 2025, [https://www.reddit.com/r/assettocorsa/comments/5l64g8/this\_is\_how\_the\_ffb\_worksand\_how\_to\_set\_it\_up/](https://www.reddit.com/r/assettocorsa/comments/5l64g8/this_is_how_the_ffb_worksand_how_to_set_it_up/)  
16. Assetto Corsa (PC) \- Fanatec Recommended Settings, accessed December 27, 2025, [https://forum.fanatec.com/topic/517-assetto-corsa-pc-fanatec-recommended-settings/page/2/](https://forum.fanatec.com/topic/517-assetto-corsa-pc-fanatec-recommended-settings/page/2/)  
17. Determining Steered Angle from Telemetry | Page 2 \- Reiza Studios Forum, accessed December 27, 2025, [https://forum.reizastudios.com/threads/determining-steered-angle-from-telemetry.30439/page-2](https://forum.reizastudios.com/threads/determining-steered-angle-from-telemetry.30439/page-2)  
18. I just bought my first wheel and I don't really enjoy it. I want to, any tips? : r/simracing \- Reddit, accessed December 27, 2025, [https://www.reddit.com/r/simracing/comments/18p4sh4/i\_just\_bought\_my\_first\_wheel\_and\_i\_dont\_really/](https://www.reddit.com/r/simracing/comments/18p4sh4/i_just_bought_my_first_wheel_and_i_dont_really/)  
19. Why is no one talking about the new 'Alternative FFB' script in the latest CSP??? : r/assettocorsa \- Reddit, accessed December 27, 2025, [https://www.reddit.com/r/assettocorsa/comments/1jl25ey/why\_is\_no\_one\_talking\_about\_the\_new\_alternative/](https://www.reddit.com/r/assettocorsa/comments/1jl25ey/why_is_no_one_talking_about_the_new_alternative/)  
20. PSA: If you're using a Logitech wheel and struggling to get to grips with this sim, generate a custom FFB look up table and drop it into the settings. the difference is drastic. : r/ACCompetizione \- Reddit, accessed December 27, 2025, [https://www.reddit.com/r/ACCompetizione/comments/1cdn9r7/psa\_if\_youre\_using\_a\_logitech\_wheel\_and/](https://www.reddit.com/r/ACCompetizione/comments/1cdn9r7/psa_if_youre_using_a_logitech_wheel_and/)  
21. Automobilista 2 Custom Force Feedback \- Overview & Recommendations | Page 255, accessed December 27, 2025, [https://forum.reizastudios.com/threads/automobilista-2-custom-force-feedback-overview-recommendations.11135/page-255](https://forum.reizastudios.com/threads/automobilista-2-custom-force-feedback-overview-recommendations.11135/page-255)  
22. Numerical differentiation of noisy data: A unifying multi-objective optimization framework, accessed December 27, 2025, [https://pmc.ncbi.nlm.nih.gov/articles/PMC7899139/](https://pmc.ncbi.nlm.nih.gov/articles/PMC7899139/)  
23. On the Signal-to-Noise Ratio for Numerical Differentiation \- arXiv, accessed December 27, 2025, [https://arxiv.org/html/2501.14906v1](https://arxiv.org/html/2501.14906v1)  
24. How to curve fit noisy data in real-time with minimal lag \- General Guidance \- Arduino Forum, accessed December 27, 2025, [https://forum.arduino.cc/t/how-to-curve-fit-noisy-data-in-real-time-with-minimal-lag/655764](https://forum.arduino.cc/t/how-to-curve-fit-noisy-data-in-real-time-with-minimal-lag/655764)  
25. Read data in real time :: Sim Racing Telemetry General Discussions \- Steam Community, accessed December 27, 2025, [https://steamcommunity.com/app/845210/discussions/0/4666237625659148039/](https://steamcommunity.com/app/845210/discussions/0/4666237625659148039/)  
26. Real-time numerical differentiation of sampled data using adaptive input and state estimation, accessed December 27, 2025, [https://dsbaero.engin.umich.edu/wp-content/uploads/sites/441/2024/04/Real-time-numerical-differentiation-of-sampled-data-using-adaptive-input-and-state-estimation.pdf](https://dsbaero.engin.umich.edu/wp-content/uploads/sites/441/2024/04/Real-time-numerical-differentiation-of-sampled-data-using-adaptive-input-and-state-estimation.pdf)  
27. Compute the time derivative of a noisy digital signal? \- Signal Processing Stack Exchange, accessed December 27, 2025, [https://dsp.stackexchange.com/questions/27420/compute-the-time-derivative-of-a-noisy-digital-signal](https://dsp.stackexchange.com/questions/27420/compute-the-time-derivative-of-a-noisy-digital-signal)  
28. HANDWHEEL FORCE FEEDBACK WITH LANEKEEPING ASSISTANCE: COMBINED DYNAMICS, STABILITY AND BOUNDING, accessed December 27, 2025, [https://ddl.stanford.edu/sites/g/files/sbiybj25996/files/media/file/2006\_thesis\_switkes\_handwheel\_force\_feedback\_with\_lanekeeping\_assistance\_0.pdf](https://ddl.stanford.edu/sites/g/files/sbiybj25996/files/media/file/2006_thesis_switkes_handwheel_force_feedback_with_lanekeeping_assistance_0.pdf)  
29. Intro to Signal Smoothing Filters \- Seeq Knowledge Base, accessed December 27, 2025, [https://support.seeq.com/kb/R58/cloud/intro-to-signal-smoothing-filters](https://support.seeq.com/kb/R58/cloud/intro-to-signal-smoothing-filters)  
30. What are the advantages and disadvantages to the various smoothing functions available in LabChart? | ADInstruments, accessed December 27, 2025, [https://www.adinstruments.com/support/knowledge-base/what-are-advantages-and-disadvantages-various-smoothing-functions-available](https://www.adinstruments.com/support/knowledge-base/what-are-advantages-and-disadvantages-various-smoothing-functions-available)  
31. Racing Slicks vs Street Tires on Track: How Big is the Difference? \- YouTube, accessed December 27, 2025, [https://www.youtube.com/watch?v=1fWc4Usi-f4](https://www.youtube.com/watch?v=1fWc4Usi-f4)  
32. Slip angles in the wet \- The Technical Forum \- Autosport Forums, accessed December 27, 2025, [https://forums.autosport.com/topic/210086-slip-angles-in-the-wet/](https://forums.autosport.com/topic/210086-slip-angles-in-the-wet/)  
33. Actual tire grip based on wear and temp | Kunos Simulazioni \- Official Forum \- Assetto Corsa, accessed December 27, 2025, [https://www.assettocorsa.net/forum/index.php?threads/actual-tire-grip-based-on-wear-and-temp.16909/](https://www.assettocorsa.net/forum/index.php?threads/actual-tire-grip-based-on-wear-and-temp.16909/)  
34. How to adjust camber "correct"? | Kunos Simulazioni \- Official Forum \- Assetto Corsa, accessed December 27, 2025, [https://www.assettocorsa.net/forum/index.php?threads/how-to-adjust-camber-correct.25688/](https://www.assettocorsa.net/forum/index.php?threads/how-to-adjust-camber-correct.25688/)  
35. Tyre Stiffness :: Assetto Corsa Competizione General Discussions \- Steam Community, accessed December 27, 2025, [https://steamcommunity.com/app/805550/discussions/0/3491891042512704931/?ctp=2](https://steamcommunity.com/app/805550/discussions/0/3491891042512704931/?ctp=2)  
36. PHYSICS \- Tyres\! oh... it's raining... | Page 8 | Kunos Simulazioni \- Assetto Corsa, accessed December 27, 2025, [https://www.assettocorsa.net/forum/index.php?threads/tyres-oh-its-raining.50739/page-8](https://www.assettocorsa.net/forum/index.php?threads/tyres-oh-its-raining.50739/page-8)  
37. FFB \- No tire grip feel \- or am I missing something? | Le Mans Ultimate Community, accessed December 27, 2025, [https://community.lemansultimate.com/index.php?threads/ffb-no-tire-grip-feel-or-am-i-missing-something.9374/](https://community.lemansultimate.com/index.php?threads/ffb-no-tire-grip-feel-or-am-i-missing-something.9374/)  
38. rF2 FFB system and philosophy | Page 3 \- Studio-397 Forum, accessed December 27, 2025, [https://forum.studio-397.com/index.php?threads/rf2-ffb-system-and-philosophy.67691/page-3](https://forum.studio-397.com/index.php?threads/rf2-ffb-system-and-philosophy.67691/page-3)  
39. is iRacing really the one doing it right ? (serious discussion) : r/simracing \- Reddit, accessed December 27, 2025, [https://www.reddit.com/r/simracing/comments/1p5p2un/is\_iracing\_really\_the\_one\_doing\_it\_right\_serious/](https://www.reddit.com/r/simracing/comments/1p5p2un/is_iracing_really_the_one_doing_it_right_serious/)  
40. Mr Deap's sim racing introduction guide \- Steam Community, accessed December 27, 2025, [https://steamcommunity.com/sharedfiles/filedetails/?id=912315488](https://steamcommunity.com/sharedfiles/filedetails/?id=912315488)  
41. Marvin's Awesome iRacing App (MAIRA) – Introduction and first impressions \- Simracing-PC, accessed December 27, 2025, [https://simracing-pc.de/en/2025/03/25/marvins-awesome-iracing-app-maira-introduction-and-first-impressions/](https://simracing-pc.de/en/2025/03/25/marvins-awesome-iracing-app-maira-introduction-and-first-impressions/)  
42. Racing car behaviour \- Understanding the Racing Tyre | Kunos Simulazioni \- Official Forum, accessed December 27, 2025, [https://www.assettocorsa.net/forum/index.php?threads/racing-car-behaviour-understanding-the-racing-tyre.36933/](https://www.assettocorsa.net/forum/index.php?threads/racing-car-behaviour-understanding-the-racing-tyre.36933/)  
43. Programming Vehicles in Games \- Wassimulator.com, accessed December 27, 2025, [https://wassimulator.com/blog/programming/programming\_vehicles\_in\_games.html](https://wassimulator.com/blog/programming/programming_vehicles_in_games.html)  
44. Camber – Geometry Explained \- Suspension Secrets, accessed December 27, 2025, [https://suspensionsecrets.co.uk/camber/](https://suspensionsecrets.co.uk/camber/)  
45. Camber in Cornering : r/FSAE \- Reddit, accessed December 27, 2025, [https://www.reddit.com/r/FSAE/comments/13h4dnq/camber\_in\_cornering/](https://www.reddit.com/r/FSAE/comments/13h4dnq/camber_in_cornering/)  
46. How much does tyre wear affect lap times? : r/ACCompetizione \- Reddit, accessed December 27, 2025, [https://www.reddit.com/r/ACCompetizione/comments/qjqy1b/how\_much\_does\_tyre\_wear\_affect\_lap\_times/](https://www.reddit.com/r/ACCompetizione/comments/qjqy1b/how_much_does_tyre_wear_affect_lap_times/)  
47. Extended Kalman filter based state estimation of formula student autonomous racing \- SPIE Digital Library, accessed December 27, 2025, [https://www.spiedigitallibrary.org/proceedings/Download?urlId=10.1117%2F12.3007015](https://www.spiedigitallibrary.org/proceedings/Download?urlId=10.1117/12.3007015)  
48. An Unscented Kalman Filter-Informed Neural Network for Vehicle Sideslip Angle Estimation \- IEEE Xplore, accessed December 27, 2025, [https://ieeexplore.ieee.org/iel7/25/10683799/10502278.pdf](https://ieeexplore.ieee.org/iel7/25/10683799/10502278.pdf)  
49. (PDF) Development and validation of a Kalman filter-based model for vehicle slip angle estimation \- ResearchGate, accessed December 27, 2025, [https://www.researchgate.net/publication/262864048\_Development\_and\_validation\_of\_a\_Kalman\_filter-based\_model\_for\_vehicle\_slip\_angle\_estimation](https://www.researchgate.net/publication/262864048_Development_and_validation_of_a_Kalman_filter-based_model_for_vehicle_slip_angle_estimation)  
50. Vehicle sideslip angle estimation using Kalman filters: modelling and validation \- Sheffield Hallam University Research Archive, accessed December 27, 2025, [https://shura.shu.ac.uk/22810/1/IFIT2018\_042\_final\_v2.pdf](https://shura.shu.ac.uk/22810/1/IFIT2018_042_final_v2.pdf)  
51. Tire lateral force estimation and grip potential identification using Neural Networks, Extended Kalman Filter, and Recursive Lea \- Coventry University, accessed December 27, 2025, [https://pure.coventry.ac.uk/ws/files/11039499/tire\_force\_est\_NN\_MA\_SK\_revision\_DRAFT\_cur.pdf](https://pure.coventry.ac.uk/ws/files/11039499/tire_force_est_NN_MA_SK_revision_DRAFT_cur.pdf)  
52. Comparison of Performance of Predicting the Wear Amount of Tire Tread Depending on Sensing Information \- MDPI, accessed December 27, 2025, [https://www.mdpi.com/1424-8220/23/1/459](https://www.mdpi.com/1424-8220/23/1/459)

# Followu up questions

What is the difference between a Savitzky-Golay Filtering and a Low Pass Filter?

## Answer

**Mode 2: Simple Q&A**

The fundamental difference lies in **how** they separate "signal" from "noise," which drastically changes how the Force Feedback (FFB) feels during a slide.

Think of a **Low Pass Filter** as "blurring" the data to hide the rough edges, while a **Savitzky-Golay Filter** attempts to "trace" the data with a curved ruler to find the true shape.

Here is the technical breakdown of why this matters for Slope Detection in sim racing:

### 1. The Low Pass Filter (LPF)

* **How it works:** It attenuates (reduces) all signals above a certain frequency (e.g., 10Hz). It assumes that rapid changes are "noise" and slow changes are "physics."
* ** The Problem:** A sudden loss of grip (the "snap" of oversteer) is a **high-frequency event**. It happens instantly. An LPF sees this sharp drop in force as "noise" and smooths it out.
* **Result in FFB:** The sharp "cliff" of grip loss is eroded into a gentle hill. You feel the slide **late** and **vaguely**, making it harder to catch the car.



### 2. The Savitzky-Golay Filter (SG)

* **How it works:** It does not simply cut frequencies. Instead, it takes a small window of data points (e.g., the last 10 samples) and mathematically fits a polynomial curve (like a parabola) to them. It then uses the slope of *that perfect curve* as the output.


* **The Advantage:** It preserves the **height and width of peaks**. If the tire force drops sharply, the polynomial fit bends to match that drop rather than averaging it away.


* **Result in FFB:** It retains the distinct "kick" in the steering wheel when the tires let go, while still ignoring the random "static" or jitter from the telemetry signal.

### Summary Comparison Table

| Feature | Low Pass Filter (Moving Average) | Savitzky-Golay Filter |
| --- | --- | --- |
| **Method** | Averages adjacent points (Blurring) | Fits a curve to points (Tracing) |
| **Peak Handling** | Flattens peaks (Reduces FFB detail) | **Preserves peaks** (Retains FFB detail) |
| **Grip Loss Feel** | Vague, "mushy" transition | Distinct, sharp transition |
| **Latency** | Low to Moderate (Phase Shift) | Moderate (Requires window buffer) 

 |
| **Best For** | General noise reduction (vibration) | **Slope/Derivative Detection** |

**Recommendation:** For your FFB slope detection algorithm, **Savitzky-Golay** is the superior choice. Although it is computationally heavier and introduces slight latency (due to needing a window of past data to fit the curve), preserving the *shape* of the traction loss curve is critical for providing the driver with actionable information.

## Question

Which latency can I expect, in ms , from the Savitzky-Golay Filter? Is the latency adjustable?

## Answer

**Mode 2: Simple Q&A**

You can expect a latency ranging from **16ms to 100ms** in a typical SimHub environment, depending entirely on how aggressively you tune the filter.

Yes, the latency is **fully adjustable**. It is directly controlled by the **Window Size** (the number of data points the filter analyzes at once) and the update frequency of your software.

Here is the technical breakdown of how to calculate and adjust it:

### 1. How to Calculate Expected Latency

The Savitzky-Golay filter is "non-causal," meaning it technically needs future data to calculate the current "perfect" curve. In real-time applications like SimHub, you simulate this by buffering incoming data. The output is effectively delayed by half the window size.

The formula for latency is:


Assuming SimHub is running at a standard **60Hz** (approx. 16.6ms per sample):

| Window Size (Samples) | Smoothing Effect | Estimated Latency | Suitability for FFB |
| --- | --- | --- | --- |
| **5** | Light Smoothing | **~33 ms** | **Excellent** (Imperceptible delay) |
| **9** | Moderate Smoothing | **~66 ms** | **Good** (Standard baseline) |
| **15** | Heavy Smoothing | **~116 ms** | **Poor** (Noticeable lag in catching slides) |
| **25** | Very Heavy | **~200 ms** | **Unusable** (Disconnects driver from car) |

### 2. How to Adjust the Latency

You adjust the latency by changing the **Window Size** parameter in your algorithm.

* **To Reduce Latency:** Lower the window size (e.g., reduce from 15 to 7). This makes the FFB respond faster to grip loss but allows more signal noise (vibrations/grain) to pass through.
* **To Increase Smoothness:** Increase the window size. This eliminates "jagged" FFB spikes but delays the "drop" sensation when the tires lose grip.

**Important Note for SimHub Users:**
The "Time Between Samples" depends on your specific SimHub setup.

* **SimHub (Licensed):** Typically updates at **60Hz** (16ms per sample).
* **SimHub (Free Version):** Custom Serial devices are limited to **10Hz** (100ms per sample). If you are using the free version, a Savitzky-Golay filter will introduce massive latency (e.g., a window of 5 would result in 200ms delay), making it unsuitable for real-time FFB.

## Question

Assume I am not using SimHub, but reading telemetry from the game (Assetto Corsa Evo or Le Mans Ultimate) at 400 Hz. Which latency can I expect from  Savitzky-Golay filter  depending on the number of data points / samples (Window Size)?

Regarding the  Kalman Filtering, I have some data from those two games telemetry that can be used for this. I think I have mass and wheelbase, but I am not sure if I have  tire stiffness (I suspect not). Can I still calculate the Kalman Filtering? Can I estimate  tire stiffness to calculate the Kalman Filtering? 

## Answer

**Mode 2: Simple Q&A**

### 1. Savitzky-Golay Latency at 400 Hz

At a telemetry update rate of **400 Hz**, the time between samples () is **2.5 ms**. Because the data density is much higher, you can use significantly larger window sizes to achieve smoother signals while maintaining lower latency than is possible at 60 Hz.

Using the group delay formula for real-time applications (), here is the expected latency for various Window Sizes ():

| Window Size () | Latency (ms) | Filtering Characteristic |
| --- | --- | --- |
| **9 samples** | **10.0 ms** | Very Low Latency. Minimal smoothing, still captures some noise. |
| **15 samples** | **17.5 ms** | **Sweet Spot.** Excellent noise rejection with imperceptible delay. |
| **25 samples** | **30.0 ms** | Strong smoothing. Good for very noisy signals (e.g., rFactor 2/LMU raw force). |
| **41 samples** | **50.0 ms** | Noticeable delay. Only recommended for indicators (like tire wear), not FFB. |

**Key Takeaway:** At 400 Hz, a window of **15 to 25 samples** is ideal. It provides a latency of 17–30ms, which is faster than a single frame of video at 60 FPS (16.6ms), making the delay effectively imperceptible to the driver while providing a very smooth derivative for the Slope Detection logic.

### 2. Kalman Filtering with Unknown Tire Stiffness

**Can you calculate it without known stiffness?**
Strictly speaking, a standard Kalman Filter (KF) **cannot** function correctly if the tire cornering stiffness () is completely unknown, as the filter uses this parameter to predict how the vehicle *should* react to steering inputs. However, you **can** still solve this problem using advanced estimation techniques.

**How to Estimate Tire Stiffness ():**
Since you have the vehicle mass () and wheelbase (), you can implement a **Dual Estimation** strategy. This involves running two algorithms simultaneously or using a specialized filter that treats stiffness as a variable to be found, rather than a constant.

#### Method A: Recursive Least Squares (RLS) (Recommended)

This is the industry-standard method for real-time parameter estimation.

1. **Concept:** You assume the tire behaves linearly at low slip angles (e.g., straight-line driving or gentle cornering).
2. **The Formula:** The algorithm monitors the relationship between Lateral Acceleration () and the Steering Angle ().
*  (Lateral Force is approx. mass times lateral acceleration).
* At low slip, .


3. **Process:** The RLS algorithm continuously solves for  whenever the car is driving straight or turning gently. It "learns" the stiffness of the tire.
4. **Integration:** Once RLS estimates the stiffness (e.g., 150,000 N/rad), it feeds this value into your main Kalman Filter to estimate the Slip Angle during aggressive cornering.

#### Method B: Joint Extended Kalman Filter (Joint EKF)

This method includes the tire stiffness as a "state" in the state vector () alongside Yaw Rate and Sideslip.

* **State Vector:**  (Sideslip, Yaw Rate, Stiffness).
* **Mechanism:** The filter observes the error between the *predicted* yaw rate and the *measured* yaw rate (from telemetry). It attributes slow, consistent errors to a wrong stiffness value and updates  automatically.
* **Pros:** It adapts dynamically to tire wear and rain (as stiffness drops).
* **Cons:** It is computationally expensive and difficult to tune. If the filter is not tuned perfectly, the stiffness estimate can oscillate wildy, causing FFB glitches.

**Recommendation:**
Start with **Method A (RLS)**. It is computationally lighter and more stable. Use the telemetry from *Le Mans Ultimate* (which has high-quality tire physics) to "train" your stiffness estimator on the straightaways and gentle corners, then lock that value to calculate the slip angle limits during the heavy braking and cornering zones.

