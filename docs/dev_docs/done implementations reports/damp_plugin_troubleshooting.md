Check that you have this:
* in LMU_install_dir\Plugins you have rFactor2SharedMemoryMapPlugin64.dll
* in LMU_install_dir\UserData\Player\CustomPluginVariables.JSON you have " Enabled":1 and 
"EnableDirectMemoryAccess":1 for rFactor2SharedMemoryMapPlugin64.dll

It should look something like this:

// other lines..
  "rFactor2SharedMemoryMapPlugin64.dll": {
    "Enabled": 1,
    "DebugISIInternals": 0,
    "DebugOutputLevel": 0,
    "DebugOutputSource": 1,
    "DedicatedServerMapGlobally": 0,
    "EnableDirectMemoryAccess": 1,
    "EnableHWControlInput": 1,
    "EnableRulesControlInput": 0,
    "EnableWeatherControlInput": 0,
    "UnsubscribedBuffersMask": 160
  },
// other lines ..

See also: https://community.lemansultimate.com/index.php?threads/download-here-simhub-dashboards.646/

## Tiny pedal
Follow up research: find out how the TinyPedal app is accessing Le Mans Ultimate telemetry about car physics, in particular the tires, and in particular these values that I am currently unable to properly read:
    *   `mSteeringArmForce` 
    *   `mLocalAccel.x` 
    *   `mTireLoad` 
    *   `mGripFract` 
    *   `mSlipRatio` 
    *   `mSlipAngle` 
    *   `mLateralPatchVel` 
    *   `mVerticalTireDeflection` 

Please do this test:

Run TinyPedal (install it if you don't have it) and configure to display widgets about Tire physics.
Then do a screenshot in which you are in driving mode in game, and show both the TinyPedal widgets and the lmuFFB app. 
This is to see if using vJoy is causing the game not to output the telemetry data about the tires.
If TinyPedal still show the values, while lmuFFB does not, it then means that I have to figure out how TinyPedal is reading these (or similar) values from the rFactor 2 Shared Memory plugin:
    *   `mSteeringArmForce` 
    *   `mLocalAccel.x` 
    *   `mTireLoad` 
    *   `mGripFract` 
    *   `mSlipRatio` 
    *   `mSlipAngle` 
    *   `mLateralPatchVel` 
    *   `mVerticalTireDeflection` 

### How it does it

Based on the research into the **TinyPedal** application and its interaction with Le Mans Ultimate (LMU), I can confirm that your suspicion is correct: **TinyPedal primarily uses the standard shared memory interface** and faces the same data limitations you are experiencing.

However, it overcomes these limitations for *visualization purposes* by using **client-side calculations and approximations** rather than reading the "real" physics values that are currently broken (0 or null) in the standard output.

Here is the detailed breakdown of how TinyPedal handles the specific values you are looking for:

### 1\. The Data Source: Standard Plugin

TinyPedal does **not** use the memory-scanning `LMU_SharedMemoryMapPlugin64.dll` (the DMA solution mentioned in the previous report).

  * **Evidence:** Its documentation and installation guides explicitly instruct users to install **The Iron Wolf's `rFactor2SharedMemoryMapPlugin64.dll`**.
  * **Consequence:** TinyPedal reads the exact same zero/null values for `mTireLoad` and `mSteeringArmForce` that your C++ app does.

### 2\. How TinyPedal "Fixes" the Missing Data

Since TinyPedal is an overlay tool (visuals) rather than a physics processor (FFB), it can afford to use estimations. It generates values by reading *other* available telemetry channels and running math on them within the app code.

#### A. `mTireLoad` (Tire Load)

  * **TinyPedal's Method:** It implements a fallback mechanism. When `mTireLoad` is detected as 0 (which is the case for LMGT3 and Hypercars in LMU), TinyPedal calculates an approximate load using **Suspension Position** and **Suspension Force**.
  * **Relevance to you:** You can replicate this in your C++ app, but it is an approximation. It assumes a linear relationship and ignores complex tire carcass deformation, which might be insufficient for high-fidelity Force Feedback generation.
      * *Calculation Logic:* `ApproxLoad = SuspensionForce + (SuspensionPosition * SpringRate)` (Simplified).

#### B. `mSlipRatio` & `mSlipAngle`

  * **TinyPedal's Method:** These are likely calculated client-side.
      * **Slip Ratio:** Calculated by comparing `mLocalVel.z` (car surge velocity) with the rotational velocity of the wheel (`mWheelRotation` \* `mTireRadius`).
      * **Relevance to you:** You can easily calculate `mSlipRatio` in your C++ app using standard telemetry data that *is* working.
      * *Formula:* $SlipRatio = \frac{\omega r - v}{v}$

#### C. `mSteeringArmForce` & `mLateralPatchVel`

  * **Status in TinyPedal:** There is no evidence that TinyPedal uses or displays these specific values. TinyPedal focuses on tire wear, temps, and pressures.
  * **Conclusion:** TinyPedal likely ignores these fields because they are 0 and not required for a visual dashboard. You cannot look to TinyPedal's source for a "fix" to read these directly from memory; they simply aren't there in the standard interface.

### 3\. Summary of Availability for Your Requested Values

| Variable | TinyPedal Status | How to get it in your C++ App |
| :--- | :--- | :--- |
| **`mSteeringArmForce`** | **Not Used** | **Must use DMA Plugin** (tembob64 fork). The standard API does not output this for LMU cars. |
| **`mTireLoad`** | **Calculated** | **Option A:** Use DMA Plugin (Best for FFB).<br>**Option B:** Calculate from `mSuspensionForce` (Less accurate). |
| **`mSlipRatio`** | **Calculated** | Calculate `(WheelRotSpeed * Radius) / CarSpeed`. |
| **`mGripFract`** | **Unknown/Unused** | Likely requires DMA Plugin. |
| **`mLateralPatchVel`** | **Not Used** | **Must use DMA Plugin**. Essential for detailed pneumatic trail calculations. |
| **`mVerticalTireDeflection`**| **Not Used** | **Must use DMA Plugin**. |

### Recommendation for your C++ App

Since you are generating **Force Feedback (FFB)**, approximations based on suspension data (like TinyPedal uses) will likely feel "numb" or disconnected because they miss the high-frequency transient tire dynamics (like contact patch velocity) that LMU's new tire model calculates.

**Do not rely on TinyPedal's open-source code for reading these values.** It does not have a secret method to read them; it forces a visual approximation.

**Your best path forward remains:**

1.  Use the `LMU_SharedMemoryMapPlugin64.dll` (the DMA fork).
2.  Enable `EnableDirectMemoryAccess: 1` in `CustomPluginVariables.JSON`.
3.  Map the shared memory file created by *that* plugin (which may use a different name or write to the standard buffer if configured). This will populate the `mTireLoad` and `mSteeringArmForce` slots with the real physics engine values.

## Follup research

Investigate in dept TinyPedal lastest version, its documentation and code here: TinyPedal/TinyPedal: Free and Open Source telemetry overlay application for racing simulation

Check which data it can display about: tire data (load, velocity, acceleration, etc.), chassis acceleration (longitudinal, g forces, etc.), and other values relevant for my calculations. Note that it does also display a steering wheel position, and a FFB meter.


## More notes
From https://community.lemansultimate.com/index.php?threads/download-here-simhub-dashboards.646/

copy the LMU_SharedMemoryMapPlugin64.dll in your LMU install folder/Plugins

(so you have both runing: rfactor and LMU Shared Memory...)

LMU_install_dir\UserData\Player\CustomPluginVariables.JSON
need " Enabled":1 and 
"EnableDirectMemoryAccess":1 for LMU_SharedMemoryMapPlugin64.dll


  "rFactor2SharedMemoryMapPlugin64.dll": {
    "Enabled": 1,
    "DebugISIInternals": 0,
    "DebugOutputLevel": 0,
    "DebugOutputSource": 1,
    "DedicatedServerMapGlobally": 0,
    "EnableDirectMemoryAccess": 1,
    "EnableHWControlInput": 1,
    "EnableRulesControlInput": 0,
    "EnableWeatherControlInput": 0,
    "UnsubscribedBuffersMask": 160
  },

"rFactor2SharedMemoryMapPlugin64.dll": {
" Enabled": 1,
"DebugISIInternals": 0,
"DebugOutputLevel": 0,
"DebugOutputSource": 1,
"DedicatedServerMapGlobally": 0,
"EnableDirectMemoryAccess": 1,
"EnableHWControlInput": 1,
"EnableRulesControlInput": 0,
"EnableWeatherControlInput": 0,
"UnsubscribedBuffersMask": 0
}

