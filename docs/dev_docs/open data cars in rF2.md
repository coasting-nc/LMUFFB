These cars should have fully transparent telemetry visible from shared memory:

* Chevrolet Corvette C6.R GT2 (2009): An ISI-era release. Physics files are accessible, and mTireLoad is fully visible in telemetry.
* Nissan GT500 (2013) & 370Z: While technically Super GT, the GT500 uses high-downforce physics that output full load data.
* Chevrolet Camaro GT3 (2012): An early GT3 implementation by ISI. Useful for comparing "Legacy" GT3 physics with the modern, encrypted GT3 DLC.
* Dallara DW12 (2014)
* Formula Renault 3.5 (2014): One of the most highly regarded physics models in rFactor 2.
* Marussia MR01 (F1 2012)
* Historic F1 (Brabham BT20, BT44B, March 761, McLaren M23)
* McLaren	MP4/8
* McLaren	MP4/13
* AC Cars	427 SC (Cobra)
* Howston	G4 / G6 (Fictional)
* ? Howston Dissenter 1974
* ? Formula ISI 2012
* ? Renault Megane Trophy 2013
* Panoz	AIV Roadster
* Renault	Clio Cup / Megane	
* ? Riley MkXX Daytona Prototype
  
# Other data that might be blocked for DLC cars

* mSuspensionDeflection
* mLateralForce
* mTireLoad
* mGripFract

# Workaournds for missing data in DLC cars

## Operational Consequences for Telemetry Analysis**

The segregation of data access has created a difficult environment for serious simulation users, necessitating specific workarounds depending on the vehicle type.

### **10.1 Setup Engineering: The "Blind" Suspension Tune**

For encrypted cars (e.g., **Porsche 911 GT3 Cup**), the lack of mTireLoad cripples standard suspension analysis.

* **The Issue:** Engineers cannot create Load vs. Velocity histograms. This means they cannot definitively separate "track roughness" (high frequency, low load) from "driver inputs" (low frequency, high load).  
* **The Workaround:** Engineers must use Lateral Acceleration ($G\_{lat}$) and Vertical Acceleration ($G\_{vert}$) as imperfect proxies. However, $G\_{vert}$ is noisy and does not account for steady-state aerodynamic load, making ride-height optimization for cars like the **Vanwall LMH** significantly harder.33

### **10.2 Driver Coaching: The Missing Slip Angle Metric**

For unencrypted cars like the **Formula Renault 3.5**, a coach can overlay SlipAngle against mGripFract. This visualizes exactly how well the driver is exploiting the tire's peak.

* **The Issue:** On encrypted cars (e.g., **Formula E**), this channel is flat-lined.  
* **The Workaround:** Coaches must rely on "visual" slip (comparing steering angle to yaw rate). This is less precise and makes it harder to teach the nuanced "micro-slip" required for maximum pace in *rFactor 2*.

### **10.3 Third-Party Tool Compatibility**

Popular telemetry tools handle this missing data differently:

* **MoTeC i2 Pro:** Will show empty channels for Load/Wear on encrypted cars. Users typically see flat lines at 0\.  
* **SimHub:** Dashboards attempting to show "Tire Load" (often visualized as a color-changing tire icon) will fail to update or will remain static gray on DLC cars.  
* **Crew Chief:** The race engineer spotter app can still read tire *temperatures* and *pressures* (which are visible HUD elements), but cannot give detailed feedback on tire *life* remaining if the mWear channel is also encrypted (which is common in the newest DLC).35
