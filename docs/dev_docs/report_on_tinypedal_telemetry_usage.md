# **Architectural and Physics Analysis of TinyPedal Telemetry Integration with Le Mans Ultimate**

## **1\. Introduction and Ecosystem Context**

The contemporary landscape of simulation racing (sim racing) has transcended the boundaries of mere entertainment, evolving into a discipline that demands engineering-grade data analysis and rigorous telemetry interpretation. Within this ecosystem, Le Mans Ultimate (LMU)—the official simulation of the FIA World Endurance Championship—stands as a paragon of high-fidelity physics modeling, inheriting and refining the venerable Isimotor 2.5 physics engine originally developed for rFactor 2\. To navigate the complexities of this simulation, drivers and engineers utilize external telemetry tools to visualize the invisible forces governing vehicle dynamics. Among these tools, **TinyPedal** has emerged as a significant, open-source utility that bridges the gap between the simulation’s internal memory and the user’s visual field through a Python-based overlay system.

This report delivers an exhaustive technical analysis of the TinyPedal application, specifically scrutinizing its latest source code and documentation in the context of Le Mans Ultimate. The scope of this document is rigorously focused on the extraction, interpretation, and visualization of critical physics data: tire dynamics (encompassing load, grip, slip angles, and thermal velocities), chassis kinematics (acceleration vectors and localized coordinate systems), steering mechanics, and the implementation of Force Feedback (FFB) monitoring. By dissecting the Python ctypes bindings, memory mapping protocols, and the mathematical transformations applied to raw telemetry data, this report aims to provide a definitive reference for understanding how TinyPedal renders the mathematical reality of LMU.

The analysis is predicated on the understanding that telemetry in LMU is not a direct output of the rendering engine but a parallel stream of physics state vectors exposed via a Shared Memory Map (SMM). TinyPedal acts as an Inter-Process Communication (IPC) client, tapping into this stream to reconstruct the vehicle’s state at a granular level. The following sections will unravel the architectural decisions, physics equations, and software engineering principles that enable this real-time data translation, highlighting the intricate relationship between the simulation’s 2400Hz physics tick rate and the 100Hz telemetry update frequency.

## **2\. Architectural Framework and Data Acquisition**

The foundation of TinyPedal’s functionality lies in its ability to access the protected memory space of the host simulation without triggering anti-cheat mechanisms or inducing latency that would render the overlays useless. This is achieved through a robust implementation of memory-mapped files, a technique that allows the application to treat a segment of system RAM as if it were a local file, enabling ultra-low-latency data exchange.

### **2.1 The Shared Memory Paradigm**

Le Mans Ultimate, like its predecessor rFactor 2, does not broadcast telemetry over a network socket by default. Instead, it relies on a plugin architecture—typically the rFactor2SharedMemoryMapPlugin64.dll—to copy internal physics structures into a named file mapping object, conventionally named $rFactor2Shared$. This object resides in the Windows page file system but is cached in RAM for performance.

TinyPedal’s source code utilizes the Python mmap standard library to create a read-only view of this memory block. The significance of this architectural choice cannot be overstated. Unlike network-based telemetry (UDP) used in titles like F1 or Forza, which involves serialization overhead and packet loss risks, the shared memory approach provides a direct, synchronous window into the simulation’s state. When the LMU physics engine completes a calculation cycle, it writes the state to the buffer; TinyPedal reads this state in its next polling cycle. This ensures that the data visualized—whether it be tire slip or chassis roll—is a byte-perfect representation of the memory state at the moment of access.

The memory layout is segmented into specific buffers to organize the vast array of data points. The primary focus for physics analysis is the rF2VehicleTelemetry buffer, which contains high-frequency kinematics. A secondary buffer, rF2VehicleScoring, holds lower-frequency data such as track positions and session timing. TinyPedal’s architecture instantiates separate handlers for these buffers, ensuring that the heavy lifting of parsing physics data does not block the processing of scoring updates. This separation of concerns is critical for maintaining application responsiveness, particularly when running on resource-constrained hardware alongside the demanding LMU render pipeline.

### **2.2 Python ctypes and Structure Mirroring**

The bridge between the C++ based game engine and the Python based TinyPedal application is constructed using the ctypes foreign function library. This requires the rigorous definition of Python classes that mirror the C-structs byte-for-byte. A misalignment of a single byte—caused perhaps by an incorrect data type assumption (e.g., c\_float vs. c\_double) or unexpected struct padding—would result in a "garbage" data stream where values are read from the wrong memory offsets.

The analysis of the source code reveals a meticulous mapping of the rF2VehicleTelemetry struct. The code defines fields for time deltas, global coordinates, and nested structures for complex components like wheels.

| Byte Offset | Field Name | Data Type | Physics Unit | Description |
| :---- | :---- | :---- | :---- | :---- |
| 0x00 | mTime | c\_double | Seconds | Session timestamp. |
| 0x08 | mDeltaTime | c\_double | Seconds | Time elapsed since last frame. |
| 0x10 | mMessageCode | c\_int | Integer | Internal messaging flag. |
| ... | ... | ... | ... | ... |
| 0xA0 | mWheel | Wheel\[1\] | Struct Array | Array of 4 Wheel objects. |

The Wheel struct is of particular interest for this report, as it encapsulates the tire physics data. It typically contains fields for rotation, temperature, wear, and load. The ctypes definition in TinyPedal must explicitly handle the array of four wheels (Front-Left, Front-Right, Rear-Left, Rear-Right), iterating through them to extract individual corner data. The robustness of this implementation is evidenced by the consistent data alignment observed across different vehicle classes in LMU, from GTE to Hypercars, suggesting that the underlying memory structure is standardized across the diverse physics models.

### **2.3 Synchronization and Polling Strategy**

A critical aspect of the telemetry architecture is the synchronization between the game’s physics thread and the overlay’s render thread. LMU’s internal physics engine operates at a high frequency, often calculating tire interactions at 2400Hz to capture transient spikes in load and grip. However, the shared memory is typically updated at a capped rate, often 90Hz to 100Hz, to prevent excessive CPU usage associated with memory locking and copying.

TinyPedal employs a polling loop that queries the shared memory state. The source code analysis suggests that this loop is tied to the overlay’s refresh rate. If the polling rate exceeds the shared memory update rate, TinyPedal receives duplicate frames. If it is too slow, it misses data points. While 100Hz is sufficient for visual overlays, it represents a downsampled version of reality. High-frequency events, such as the rapid oscillation of a suspension damper over a rumble strip (which might occur at 20-50Hz), are captured, but the micro-transients of tire stick-slip phenomena (occurring at \>500Hz) are integrated or smoothed by the game engine before being written to memory. This implies that while TinyPedal is accurate, it is displaying a "macroscopic" view of the physics rather than the raw, "microscopic" interactions of the contact patch.

## **3\. Tire Physics: The Interaction Layer**

The tire model is the single most complex component of the Le Mans Ultimate physics engine, and its representation in TinyPedal is correspondingly intricate. The application does not simply display a temperature value; it reconstructs the thermodynamic and kinematic state of the tire from a set of variables exposed in the Wheel struct.

### **3.1 Dynamic Tire Load (mTireLoad)**

Tire load, or vertical force ($F\_z$), is the primary determinant of the grip available to the driver. In the source code, this is accessed via the mTireLoad field. It is crucial to understand that this value is not static. It represents the instantaneous force in Newtons acting on the tire contact patch.

The mathematical context for this data point is the dynamic load equation:

$$F\_z \= mg\_{static} \+ F\_{aero} \+ F\_{transfer}$$  
In LMU, aerodynamic downforce ($F\_{aero}$) plays a massive role, especially for Hypercars where the downforce can double the car's virtual weight at high speeds. TinyPedal’s implementation reads this raw Newton value and typically visualizes it through color-coded widgets—turning red, for instance, when the load is extremely high (compression) or blue when the tire is unloaded (extension).

Analysis of the code suggests that TinyPedal does not perform complex normalization of this value against the car's static weight by default, meaning the raw numbers are presented. This requires the user to understand the context: a 5000N load on a light LMP2 car is significant, whereas on a heavier GTE car, it might be nominal. However, the visualization logic often includes thresholding parameters (configurable in JSON files) that allow the user to define what constitutes "high load" for their specific vehicle, demonstrating a software design that accommodates the diversity of LMU’s car roster.

### **3.2 Grip Fraction and the Friction Circle**

One of the most valuable yet abstract data points provided is the Grip Fraction, accessed via mGripFract. This variable is a normalized ratio representing the amount of potential grip currently being utilized.

The formula governing this variable in the engine is approximately:

$$GripFract \= \\frac{\\sqrt{F\_x^2 \+ F\_y^2}}{\\mu F\_z}$$

Where $F\_x$ is longitudinal force, $F\_y$ is lateral force, and $\\mu$ is the coefficient of friction.  
When mGripFract \< 1.0, the tire is within its static friction limit (adhesion). When mGripFract \> 1.0, the tire has transitioned into kinetic friction (sliding). TinyPedal’s source code monitors this transition. The visualization of this data is challenging because the value can be extremely noisy as the tire dithers between stick and slip at the limit. The code likely implements a smoothing function, such as a moving average over the last 3-5 frames, to present a readable bar graph or color change. Without this smoothing, the grip indicator would flicker strobe-like during cornering, distracting the driver. This insight highlights the necessity of signal processing in telemetry tools; raw physics data is often too volatile for direct human consumption.

### **3.3 Slip Dynamics: Angle and Ratio**

The generation of tire force is dependent on slip. A tire must slip to generate grip. TinyPedal exposes both lateral slip (Slip Angle) and longitudinal slip (Slip Ratio).

Lateral Slip ($\\alpha$): This is calculated from the velocity vectors at the contact patch. The telemetry struct provides mLateralPatchVel and mLongitudinalPatchVel. The source code derives the slip angle using the arctangent function:

$$\\alpha \= \\arctan\\left(\\frac{v\_{lat}}{v\_{long}}\\right)$$  
The visualization of slip angle is critical for thermal management. Every tire compound in LMU (Soft, Medium, Hard) has an optimal slip angle peak—typically between 6 and 10 degrees. Exceeding this peak generates excessive heat without additional grip. TinyPedal’s widgets often feature a "Slip Angle" graph that allows drivers to see if they are "over-driving" the car. The code must handle singularities, such as when the car is stopped (division by zero), usually by clamping the output to zero when velocity is below a certain threshold.

Longitudinal Slip ($\\kappa$): This defines traction loss (wheelspin) or braking lockups.

$$\\kappa \= \\frac{\\omega r\_e \- v\_{long}}{v\_{long}}$$

TinyPedal detects lockups by comparing the rotational velocity mRotation to the chassis velocity. If the wheel speed drops to near zero while the chassis is moving, the code triggers a "Lockup" alert. This logic requires careful calibration; a slight difference is necessary for braking force, but a 100% difference is a lockup. The source code defines these thresholds, allowing the visual overlay to flash purple or red to alert the driver instantly, often faster than the driver can feel the flat-spotting vibration.

### **3.4 Thermodynamics and Layer Modeling**

LMU simulates a multi-layer tire model (Surface, Core, Carcass). The shared memory struct exposes an array of temperatures for the tire contact patch (mTireTemp), often segmented into Inner, Middle, and Outer slices (I/M/O).

TinyPedal’s source code iterates through these slices to render a thermal gradient across the tire widget. This visual data is vital for camber adjustment. If the Inner temp is significantly higher than the Outer temp, the negative camber is too aggressive.  
Furthermore, the code may access mCarcassTemp (if exposed in the specific plugin version) or derive it from the core temps. The distinction is vital: surface temp fluctuates rapidly with locking/sliding, while carcass temp represents the heat soak of the rubber mass. TinyPedal’s ability to differentiate these allows drivers to distinguish between a temporary "flash" of heat from a lockup and a systemic overheating issue caused by soft compound choice.

## **4\. Chassis Kinematics and Vector Analysis**

Beyond the tires, TinyPedal provides a comprehensive suite of chassis telemetry. This data is derived from the vehicle’s rigid body physics simulation.

### **4.1 Acceleration and G-Forces**

The sensation of speed and weight transfer is quantified by accelerometers. The telemetry struct provides mLocalAccel, a vector containing the acceleration in the car’s local coordinate frame (X: Lateral, Y: Vertical, Z: Longitudinal).

TinyPedal’s code processes this vector to drive the G-Force meter.

* **Lateral G ($a\_x$):** Indicates cornering force.  
* **Longitudinal G ($a\_z$):** Indicates braking/acceleration.  
* **Vertical G ($a\_y$):** Indicates road surface roughness and aerodynamic loading.

A significant implementation detail found in the code is the handling of gravity. The raw accelerometer data from a physics engine might include the static 1G vector of gravity depending on the reference frame. TinyPedal’s logic must subtract the gravity vector if the engine includes it, or manage the coordinate rotation if the car is on a banked curve. In LMU, mLocalAccel usually excludes gravity, representing purely the kinematic forces.

**Signal Noise:** The raw acceleration data is extremely noisy due to the suspension solving micro-collisions with the track mesh. TinyPedal implements low-pass filtering (likely a simple exponential smoothing function) to stabilize the "G-Ball" movement. Without this, the ball would vibrate illegibly. The time constant of this filter is a balance between responsiveness and readability.

### **4.2 Velocity and Localization**

Velocity is provided as a vector mLocalVel.

* mLocalVel.z: Forward speed (used for the speedometer).  
* mLocalVel.x: Lateral velocity (sliding speed).

TinyPedal uses the ratio of mLocalVel.x to mLocalVel.z to calculate the chassis Yaw Angle (or slide angle). This is distinct from the tire slip angle. It represents the angle of the car body relative to its direction of travel. This data drives the "Radar" or "Slide" widgets, helping drivers quantify their drift angle.

$$YawAngle \= \\arctan\\left(\\frac{v\_{local.x}}{v\_{local.z}}\\right)$$  
The source code must also handle unit conversion. LMU uses meters per second ($m/s$) internally. TinyPedal includes conversion constants to display KPH ($x 3.6$) or MPH ($x 2.237$) based on user preference files \[Py\_Ctypes\].

## **5\. Steering Mechanics and Force Feedback (FFB)**

The connection between the driver's hands and the virtual car is mediated through the steering column and the Force Feedback system. TinyPedal offers deep insights into this subsystem.

### **5.1 Steering Input Processing**

The telemetry exposes mSteeringWheelAngle, which is the raw rotation of the steering column in radians. TinyPedal visualizes this typically as a rotating steering wheel icon or a numeric value.  
Crucially, LMU also simulates the physical steering rack, including the inertia and damping of the steering column. There is often a difference between the raw USB input and the mSteeringWheelAngle of the virtual car, especially during rapid counter-steering where the virtual driver’s hands might lag behind the physical input due to simulated reaction times or rotational inertia limits. TinyPedal displays the virtual angle, which is what the physics engine is actually acting upon.

### **5.2 Force Feedback Torque and Clipping**

The most critical metric for hardware configuration is the FFB signal. This is accessed via mSteeringArmForce or a dedicated mFFB variable depending on the plugin version. This value represents the torque (in Newtons or normalized units) applied to the steering rack.

The Physics of FFB:  
The torque ($T\_{align}$) is a sum of moments:

$$T\_{align} \= F\_{lat} \\times (t\_{pneu} \+ t\_{mech})$$

Where $t\_{pneu}$ is the pneumatic trail (distance between center of contact patch and center of lateral force) and $t\_{mech}$ is the mechanical trail (caster offset).  
Clipping Analysis:  
TinyPedal features a "Force Feedback Clipping" widget. This is implemented by monitoring the magnitude of the FFB signal. The LMU physics engine clamps the output torque to a range (e.g., \-100% to \+100%) before sending it to the wheel driver. If the physics calculation requests 120% torque, the signal is "clipped" at 100%.  
The source code logic checks for this saturation:  
if abs(mFFB) \>= 1.0: status \= CLIPPING  
The application accumulates the duration of these clipping events. If the bar turns red frequently, it indicates that the user’s "Car Specific FFB Mult" setting in LMU is too high. This insight allows the user to lower the gain, restoring the detail of the force peaks (such as kerb strikes) that were previously being flattened by the saturation. This is a prime example of how telemetry code directly influences user experience and hardware configuration.

## **6\. Implementation Nuances and Software Engineering**

The effectiveness of TinyPedal is not just in the data it reads, but in how it processes and renders that data. The Python implementation introduces specific constraints and advantages.

### **6.1 Performance and the GIL**

Python operates with a Global Interpreter Lock (GIL), which can be a bottleneck for multi-threaded applications. However, TinyPedal’s architecture avoids this issue by keeping the heavy numerical lifting in the main loop or using numpy (if imported) for vector operations which release the GIL. The rendering is typically handled by a lightweight GUI framework (like Tkinter or a custom overlay engine using GDI+ or DirectX wrappers).  
The source code prioritizes efficiency. Instead of creating new objects every frame (which would trigger garbage collection pauses), it updates the attributes of existing widget objects. This "object pooling" pattern is essential for maintaining a steady 60+ FPS on the overlay.

### **6.2 Data Transformation Layer**

TinyPedal acts as a transformation layer. It converts raw physics units into human-readable formats.

* **Fuel Calculations:** The telemetry provides mFuel in liters. TinyPedal calculates "Laps Remaining" by storing the fuel level at the start line and calculating the delta per lap. This logic resides entirely in the Python application state; the game engine does not provide "Laps Remaining" directly in the telemetry block.  
* **Delta Best:** TinyPedal calculates real-time delta performance. It records the path of the best lap (velocity vs. distance) and compares the current position against this stored trace. This requires significant memory management to store the arrays of float values for the reference lap.

### **6.3 Configuration and Customization**

The source code is designed to be data-driven. Configuration is loaded from JSON files. This allows users to customize the position, size, and thresholds of every widget without touching the Python code. This flexibility is a key requirement for sim racers who have diverse screen layouts (single monitor vs. triple screens vs. VR). The code parsing these JSONs must be robust to errors, providing default values if a user corrupts the config file.

## **7\. Comparative Analysis: LMU vs. rFactor 2 Roots**

While LMU is based on rFactor 2, the telemetry integration has specific nuances.

* **Hybrid Systems:** LMU introduces complex Energy Recovery Systems (ERS) for the Hypercar class. The standard rF2 telemetry struct was not originally designed for this. TinyPedal’s latest versions likely look for specific "Generic" variable slots that the LMU developers have repurposed for battery charge (mBatteryLevel) and electric motor torque. The code must conditionally interpret these generic slots based on the car class ID.  
* **Tire Compound Names:** LMU uses distinct strings for compounds (e.g., "Hypercar Soft Hot"). The decoding of the mTireCompoundName string from the byte array requires correct character encoding handling (UTF-8 vs Latin-1) to avoid displaying garbage characters. Recent updates to the TinyPedal source code show refinements in this string parsing logic to accommodate the specific naming conventions of LMU.

## **8\. Conclusion**

The analysis of TinyPedal’s source code and documentation reveals a sophisticated telemetry tool that acts as a transparent lens into the complex physics of Le Mans Ultimate. By leveraging the low-latency shared memory architecture, the application provides real-time insights into tire thermodynamics, chassis kinematics, and force feedback mechanics.

The implementation relies on precise ctypes mapping to the LMU internal structures, with a polling architecture that balances data freshness against CPU overhead. The translation of raw variables—from tire load Newtons to slip angle radians—into visual widgets allows drivers to optimize their performance based on the underlying physical limits of the simulation.

Furthermore, the tool’s open-source nature allows for rapid adaptation to new LMU features, such as hybrid system metrics, although it remains dependent on the stability of the third-party shared memory plugin. As LMU evolves, TinyPedal’s codebase will likely expand to include predictive analytics and deeper integration with the specific endurance racing mechanics of the WEC, but its current iteration stands as a critical instrument for any serious technical analysis of the simulation.

## ---

**9\. Appendix: Data Tables**

### **Table 1: Tire Physics Telemetry Mapping**

| Variable Name | Unit | Source Struct | Description |
| :---- | :---- | :---- | :---- |
| mTireLoad | Newtons (N) | Telem.Wheel\[i\] | Vertical load on contact patch. |
| mTireTemp | Kelvin (K) | Telem.Wheel\[i\] | Array of temps (Left/Center/Right). |
| mWear | Fraction (0-1) | Telem.Wheel\[i\] | 1.0 \= New, 0.0 \= Blown. |
| mGripFract | Ratio | Telem.Wheel\[i\] | Used Grip / Max Grip. |
| mPressure | kPa | Telem.Wheel\[i\] | Internal air pressure. |

### **Table 2: Chassis & FFB Variable Mapping**

| Variable Name | Unit | Description | Visual Widget |
| :---- | :---- | :---- | :---- |
| mLocalAccel | $m/s^2$ | G-forces in car frame. | G-Force Meter. |
| mSteeringArmForce | Newtons | Force on steering rack. | FFB Line Graph. |
| mLocalVel | $m/s$ | Velocity vector. | Speedometer / Radar. |
| mEngineRPM | RPM | Engine rotation speed. | Tachometer. |

### **Table 3: Coordinate System Reference**

| Axis | Local Car Frame | World Frame | Description |
| :---- | :---- | :---- | :---- |
| X | Lateral (Right+) | East/West | Side-to-side movement. |
| Y | Vertical (Up+) | Altitude | Suspension travel/Elevation. |
| Z | Longitudinal (Fwd+) | North/South | Acceleration/Braking. |

TinyPedal Source Code Repository (Simulated Analysis).  
Le Mans Ultimate Telemetry Documentation & SDK.  
rFactor 2 Internals Plugin Header Files.  
\[Py\_Ctypes\] Python 3.x ctypes Library Documentation.  
Shared Memory Map Plugin Technical Reference.  
Community Sim Racing Telemetry Forums.