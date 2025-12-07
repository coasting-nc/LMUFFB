# **Technical Analysis of Shared Memory Telemetry in Le Mans Ultimate: Integration Strategies for Tire and Steering Data**

## **Executive Summary**

The transition from the established rFactor 2 (rF2) ecosystem to the new Le Mans Ultimate (LMU) platform has introduced significant complexity for telemetry application developers. While LMU utilizes the foundational ISIMotor 2.5 architecture, substantial modifications to the physics engine—specifically regarding tire modeling and input processing for Hypercar and GTE classes—have disrupted legacy data extraction methods. This has resulted in a pervasive issue where standard C++ applications utilizing the legacy rFactor 2 Shared Memory Map Plugin receive null or zero-value readings for critical telemetry channels, most notably tire load (mTireLoad), contact patch velocity (mPatchVel), and steering input (mUnfilteredSteering).

This report provides an exhaustive technical analysis of the Le Mans Ultimate shared memory ecosystem. It dissects the architectural divergences between rF2 and LMU that cause these data dropouts and evaluates the efficacy of community-developed solutions, specifically the utilization of Direct Memory Access (DMA) via forked plugin libraries. Furthermore, this document offers a detailed comparative analysis of established telemetry clients—Crew Chief, Second Monitor, and SimHub—to deconstruct their implementation strategies. The findings presented herein serve as a definitive guide for C++ developers seeking to engineer robust, high-fidelity telemetry integrations for Le Mans Ultimate, ensuring access to the full spectrum of vehicle dynamics data required for advanced simulation analysis.

## **1\. Architectural Foundations of Shared Memory in ISIMotor Engines**

To fully comprehend the mechanics of data extraction in Le Mans Ultimate, it is necessary to first examine the underlying architecture of the shared memory system inherited from rFactor 2\. This system forms the bedrock upon which all third-party telemetry tools operate, and its limitations are the primary source of the "zero value" phenomenon currently experienced by developers.

### **1.1 The Philosophy of the Internal Plugin Interface**

The ISIMotor engine, developed by Image Space Incorporated and subsequently refined by Studio 397 for rFactor 2 and Le Mans Ultimate, is designed with a modular architecture that supports "Internals Plugins." These are Dynamic Link Libraries (DLLs) written in C++ that are loaded directly into the game's address space at runtime.

Unlike external telemetry APIs common in other simulators (such as the UDP streams used by the F1 series or Forza Motorsport), the ISIMotor interface allows code to run *synchronously* with the physics engine. This offers a distinct advantage: access to high-frequency data (up to 400Hz) with zero latency. However, it also imposes a strict dependency on the game's internal memory structures. The plugin functions by subscribing to specific game events—such as UpdateTelemetry, UpdateScoring, and UpdateGraphics—during which the game engine passes a pointer to an internal data structure containing the current simulation state.1

### **1.2 The Mechanism of the Shared Memory Map Plugin**

The "rFactor 2 Shared Memory Map Plugin," originally architected by The Iron Wolf, serves as a bridge between this internal, pointer-based game environment and external applications. Its primary function is to democratize access to the internal data by copying it from the game's private memory heap into a **Memory Mapped File**—a segment of system RAM backed by the system paging file, which can be accessed by multiple processes simultaneously.

The process follows a strict sequence:

1. **Initialization:** Upon game launch, the plugin creates a named file mapping object (e.g., $rFactor2SMMP\_Telemetry$) using the Windows API CreateFileMapping.  
2. **Data Marshaling:** During every physics tick (typically every 2.5ms to 10ms depending on configuration), the game calls the plugin's UpdateTelemetry method.  
3. **Buffer Population:** The plugin performs a memcpy operation, transferring data from the game's internal TelemInfoV01 struct into the shared memory buffer.1  
4. **Synchronization:** To prevent external clients from reading data while it is being written (a "torn frame"), the plugin increments a version counter (mVersionUpdateBegin) before writing and another (mVersionUpdateEnd) after writing.

### **1.3 The Structural Definition of Telemetry Data**

The data within the shared memory buffer is organized into a rigid C-style structure, rF2Telemetry. For a client application to successfully interpret the byte stream, it must overlay an identical structure definition onto the memory view.

**Table 1: Core Components of the rF2Telemetry Structure**

| Data Segment | Offset (Approx.) | Type | Description |
| :---- | :---- | :---- | :---- |
| **Header** | 0x00 | uint32 | Versioning and synchronization flags used for concurrency control. |
| **Vehicle State** | Variable | double | Position, velocity, acceleration, and orientation vectors. |
| **Input State** | Variable | double | Unfiltered throttle, brake, clutch, and steering inputs. |
| **Tire Physics** | Variable | double | Arrays containing data for FL, FR, RL, RR tires: Load, Temperature, Wear, Grip. |
| **Damage** | Variable | double | Bodywork and mechanical damage states (often derived). |

The integrity of this data transfer relies entirely on the validity of the pointers provided by the game engine. If the game engine passes a pointer to a deprecated or uninitialized memory region for a specific variable, the plugin will dutifully copy zeros or garbage data into the shared buffer. This architectural vulnerability is the precise failure point observed in Le Mans Ultimate.

## **2\. Le Mans Ultimate: The Divergence and Data Loss**

While Le Mans Ultimate shares its DNA with rFactor 2, it represents a distinct fork in the engine's development, particularly regarding the physics of the tire model and the input handling for modern diverse hardware. These changes have broken the implicit contract between the game engine and the legacy shared memory plugin, leading to the zero-value readings for tire load and steering.

### **2.1 The Disconnection of Tire Physics Data**

The user's query highlights a specific loss of mTireLoad (vertical load in Newtons) and mPatchVel (contact patch velocity). In rFactor 2, these values were populated directly by the engine into the TelemInfoV01 struct passed to plugins.

In Le Mans Ultimate, the introduction of the new Hypercar and GTE tire models—which likely involve more complex thermodynamic and deformation calculations—appears to have shifted where this data resides in memory. When the legacy rFactor2SharedMemoryMapPlugin64.dll attempts to read these values using the standard SDK methods, the API returns null.

Why the Values are Zero:  
The standard plugin relies on the game's GetTelemetry() API function. In LMU, for certain car classes (specifically Hypercars and GTEs), the internal wiring of this function for mTireLoad is incomplete or points to a legacy tire object that is no longer updated by the physics thread. Consequently, the value remains at its initialization state: 0.0.3  
This is not a bug in the user's C++ code. The shared memory buffer is correctly mapped, and the structure is correctly aligned, but the source data being piped into that buffer is empty. This is confirmed by the behavior of other clients; unmodified versions of SimHub and Crew Chief also fail to display this data when running solely on the standard rF2 plugin.4

### **2.2 The Steering Data Void**

Similarly, steering data (mUnfilteredSteering or mSteering) is often reported as static or zero. This issue is tied to changes in how LMU handles DirectInput and Force Feedback. The game engine's internal telemetry structure, which previously mirrored the raw input from the steering wheel, now often fails to update this specific field in the standard export.

This is critical for applications that calculate self-aligning torque or analyze driver inputs. The standard plugin, expecting the game to push this data, receives nothing. To resolve this, a different approach—one that pulls data rather than waiting for it to be pushed—is required. This "pull" mechanism is known as Direct Memory Access (DMA).6

## **3\. The Solution: Direct Memory Access and Community Forks**

To bridge the gap between the broken API and the active physics memory, the sim racing development community has engineered a solution that bypasses the standard API entirely. This solution involves a specialized fork of the shared memory plugin and a specific configuration protocol.

### **3.1 The Role of Direct Memory Access (DMA)**

Direct Memory Access, in the context of this plugin, refers to the technique of scanning the game process's RAM to locate the *actual* memory addresses where physics variables are stored, rather than relying on the addresses provided by the SDK.

The plugin contains "signatures"—unique patterns of bytes that identify specific functions or data structures within the Le Mans Ultimate.exe binary. During initialization, the plugin scans the game's memory to find these signatures. Once located, it calculates the offsets to the live variables (e.g., the real-time tire load variable in the new tire model) and reads them directly.1

### **3.2 The tembob64 Fork: LMU\_SharedMemoryMapPlugin64.dll**

Research identifies a specific fork of the plugin maintained by GitHub user tembob64 (Temur Bobokhidze) as the industry standard for LMU integration. This fork, often distributed as LMU\_SharedMemoryMapPlugin64.dll, includes updated memory signatures for the latest LMU builds.9

**Key Enhancements in the LMU Fork:**

1. **Updated Signatures:** It contains the specific memory offsets required to find tire load, contact patch velocity, and brake temperatures in the current LMU build.  
2. **Hybrid System Support:** It maps new data points relevant to LMU, such as electric motor state (mElectricBoostMotorState) and battery charge, which are non-existent in the standard rF2 structure.11  
3. **Steering Fix:** It implements a workaround to read steering inputs directly from the hardware abstraction layer if the physics engine fails to report them.

### **3.3 Configuration Strategy: CustomPluginVariables.JSON**

The presence of the DLL alone is insufficient. The plugin must be explicitly configured to use DMA. This is controlled via the CustomPluginVariables.JSON file located in the user's UserData\\player directory.

**Table 2: Essential Configuration Parameters for LMU Telemetry**

| Parameter | Recommended Value | Technical Function |
| :---- | :---- | :---- |
| **Enabled** | 1 | Loads the DLL into the game process memory space. |
| **EnableDirectMemoryAccess** | 1 | **CRITICAL FIX:** Instructs the plugin to ignore the standard API return values for specific fields (like Tire Load) and instead read from the discovered memory addresses. |
| **EnableHWControlInput** | 1 | **CRITICAL FIX:** Forces the plugin to read steering, throttle, and brake inputs from the raw input layer, resolving the zero-steering issue. |
| **UnsubscribedBuffersMask** | 160 | A bitmask used to disable updates for specific buffers (e.g., Scoring or Rules) to save CPU cycles if only telemetry is needed. |

6

If EnableDirectMemoryAccess is set to 0 (the default), the plugin reverts to the standard behavior, and the C++ app will continue to read zero values for tire load.

## **4\. Comparative Analysis of Known Clients**

Analyzing how established clients implement LMU support provides a blueprint for successful C++ integration.

### **4.1 Crew Chief: The Consumer Model**

Repository: mrbelowski/CrewChiefV4 13  
Implementation Style: C\# Managed Wrapper  
Crew Chief is a comprehensive race engineer application that relies heavily on shared memory. Its source code, specifically RF2GameStateMapper.cs, reveals that it does not implement its own memory scanning logic for LMU. Instead, it relies on the user (or its own installer) to place the correct plugin DLL into the game directory.

**Integration Logic:**

* Crew Chief maps the standard buffer name $rFactor2SMMP\_Telemetry$.  
* It assumes the data within that buffer is correct.  
* **The Crucial Insight:** Crew Chief works with LMU only when the CustomPluginVariables.JSON is correctly configured. Users frequently report "Crew Chief not working" issues that are resolved solely by editing this JSON file to enable the plugin.15 This confirms that the logic for fixing the data lies entirely within the plugin configuration, not the client code.  
* **Version Management:** Crew Chief often auto-updates the rFactor2SharedMemoryMapPlugin64.dll. In LMU, this can be problematic if it overwrites the tembob64 LMU-specific version with a standard rF2 version. Advanced users often disable auto-updates or manually restore the LMU-compatible DLL.

### **4.2 Second Monitor: The Telemetry Aggregator**

Repository: Winzarten/SecondMonitor 1  
Implementation Style: C\# / WPF  
Second Monitor acts as a telemetry viewer and timing screen. Like Crew Chief, it uses a C\# connector (RFactor2Connector.cs) to map the shared memory file.

**Integration Logic:**

* It utilizes the rF2SMMonitor C\# sample code provided by The Iron Wolf as its foundation.  
* **Data Validity:** Snippets suggest that Second Monitor users also face the "zero value" issue in LMU unless they manually update the plugin infrastructure. The application does not natively support the new hybrid parameters unless the underlying struct definition is updated to match the LMU-specific plugin's extended output.18  
* **Dependency:** It has a strict dependency on the rFactor2SharedMemoryMapPlugin. If the plugin fails to load (due to missing runtimes like VC++ 2013/2015), the app receives no data.

### **4.3 SimHub and Tiny Pedal: The Power Users**

Repositories: SimHub (Closed Source Core, Open Plugins), s-victor/TinyPedal 19  
Implementation Style: Hybrid (Standard \+ DMA)  
SimHub represents the most advanced integration tier. Community plugins like **NeoRed** and **Redadeg** have pushed the boundaries of what is possible with LMU telemetry.

**Integration Logic:**

* **Dual Plugin Strategy:** Unlike Crew Chief, SimHub setups often use *both* rFactor2SharedMemoryMapPlugin64.dll AND LMU\_SharedMemoryMapPlugin64.dll. The former provides standard telemetry, while the latter (configured with DMA) fills in the gaps for tire temps, loads, and hybrid data.21  
* **Tiny Pedal's Visualization:** Tiny Pedal, an open-source overlay tool, renders tire contact patches in real-time. This requires valid mGripFract and mTireLoad data. The documentation for Tiny Pedal explicitly mandates the installation of the shared memory plugin and the modification of CustomPluginVariables.JSON to enable the plugin.19  
* **Troubleshooting Insight:** The Tiny Pedal community notes that full-screen mode in LMU can prevent overlays from rendering, but more importantly, they highlight that *without* the DMA flag enabled, their tire widgets show "cold" or "static" tires, confirming the link between DMA and data validity.24

## **5\. Technical Implementation Guide for C++ Developers**

Based on the research, the following step-by-step guide details the implementation required to fix the zero-value read issue in a C++ application.

### **5.1 Step 1: Plugin Deployment**

The standard plugin distributed with rFactor 2 tools is insufficient. You must source the LMU-specific fork.

1. **Download:** Acquire the latest LMU\_SharedMemoryMapPlugin64.dll from the tembob64 GitHub repository releases.  
2. **Install:** Copy the DLL to \\steamapps\\common\\Le Mans Ultimate\\Plugins.  
   * *Warning:* Ensure the directory is named Plugins (plural). If it does not exist, create it.  
3. **Dependencies:** Ensure the Visual C++ Redistributables (2013 and 2015-2019) are installed on the target machine, as the plugin depends on these runtimes.19

### **5.2 Step 2: Configuration of the DMA Hook**

This is the single most critical step. The C++ app will read zeros unless this is configured.

1. Navigate to \\steamapps\\common\\Le Mans Ultimate\\UserData\\player.  
2. Open or create CustomPluginVariables.JSON.  
3. Insert or update the following block:

JSON

{  
  "LMU\_SharedMemoryMapPlugin64.dll": {  
    "Enabled": 1,  
    "EnableDirectMemoryAccess": 1,  
    "EnableHWControlInput": 1,  
    "DebugISIInternals": 0,  
    "DebugOutputLevel": 0,  
    "DebugOutputSource": 1,  
    "UnsubscribedBuffersMask": 0  
  }  
}

* **EnableDirectMemoryAccess: 1**: Activates the memory scanner for mTireLoad and mPatchVel.  
* **EnableHWControlInput: 1**: Activates the raw input reader for mUnfilteredSteering.

### **5.3 Step 3: C++ Code Adaptation**

The C++ code must map the shared memory file. While the mapping name typically remains $rFactor2SMMP\_Telemetry$, the LMU plugin might optionally map to Global\\$rFactor2SMMP\_Telemetry$ depending on the server environment.

**Robust Mapping Logic:**

C++

\#**include** \<windows.h\>  
\#**include** \<iostream\>  
\#**include** "rF2Data.h" // Assuming this contains the struct definition

class LMUTelemetryReader {  
private:  
    HANDLE hMapFile;  
    rF2Telemetry\* pTelemetry;

public:  
    LMUTelemetryReader() : hMapFile(NULL), pTelemetry(NULL) {}

    bool Connect() {  
        // Try Local Namespace first  
        hMapFile \= OpenFileMapping(FILE\_MAP\_READ, FALSE, "$rFactor2SMMP\_Telemetry$");  
          
        // If failed, try Global Namespace (often needed for Dedicated Server contexts)  
        if (hMapFile \== NULL) {  
            hMapFile \= OpenFileMapping(FILE\_MAP\_READ, FALSE, "Global\\\\$rFactor2SMMP\_Telemetry$");  
        }

        if (hMapFile \== NULL) {  
            std::cerr \<\< "Error: Could not open file mapping. Is the plugin loaded and enabled in JSON?" \<\< std::endl;  
            return false;  
        }

        pTelemetry \= (rF2Telemetry\*)MapViewOfFile(hMapFile, FILE\_MAP\_READ, 0, 0, 0);  
          
        if (pTelemetry \== NULL) {  
            std::cerr \<\< "Error: Could not map view of file." \<\< std::endl;  
            CloseHandle(hMapFile);  
            return false;  
        }

        return true;  
    }

    void ReadLoop() {  
        if (\!pTelemetry) return;

        while (true) {  
            // Synchronization Check to avoid Torn Frames  
            unsigned int versionBegin \= pTelemetry-\>mVersionUpdateBegin;  
              
            // Memory Fence (Compiler specific) to prevent read reordering  
            std::atomic\_thread\_fence(std::memory\_order\_acquire);

            // Read the data  
            double tireLoadFL \= pTelemetry-\>mTireLoad;  
            double steering \= pTelemetry-\>mUnfilteredSteering;  
            double patchVelFL \= pTelemetry-\>mPatchVel; // If mapped in custom struct

            // Memory Fence  
            std::atomic\_thread\_fence(std::memory\_order\_acquire);

            unsigned int versionEnd \= pTelemetry-\>mVersionUpdateEnd;

            // Validate consistency  
            if (versionBegin \== versionEnd && versionBegin\!= 0) {  
                // Valid Data Frame  
                printf("Tire Load FL: %.2f N | Steering: %.2f\\n", tireLoadFL, steering);  
            }  
              
            Sleep(10); // Poll rate  
        }  
    }  
};

Data Structure Considerations:  
The user mentioned patch velocity. In standard rF2 structs, this might not be explicitly named mPatchVel. It is often derived or mapped into the mExtended buffer. However, the tembob64 plugin maps specific LMU data. The user should verify if mTireLoad and other missing metrics are mapped into the standard rF2Telemetry slots (repurposing them) or if they need to read the rF2Extended buffer. The SimHub integration suggests they are mapped into the standard slots to maintain compatibility with existing dashboards.22

## **6\. Future Proofing and Risks**

### **6.1 The Fragility of Memory Scanning**

The solution relies on finding specific byte patterns (signatures) in the game's executable. When Studio 397 releases a game update (e.g., a new patch or DLC), these memory addresses often shift. This breaks the DMA scanner, causing the plugin to fail or revert to zero values.

**Mitigation:** The C++ application cannot fix this on its own. The user must maintain a process for updating the LMU\_SharedMemoryMapPlugin64.dll whenever the game updates. The tembob64 repository is the primary source for these updates. The application should ideally check the plugin version or hash to warn the user if an outdated plugin is detected.

### **6.2 The Dangers of the REST API**

Research uncovered an alternative method of data access via the game's HTTP REST API (port 6397). However, this is strongly discouraged for real-time telemetry. Reports indicate that polling endpoints like /rest/garage/UIScreen/TireManagement can corrupt game state, causing flickering menus and CPU spikes.25 The shared memory approach, while complex to configure, is the only method that guarantees passive, safe data extraction.

## **7\. Conclusion**

The "zero value" readings for tire load and steering in the user's C++ application are not a failure of the reading code, but a systemic failure of the standard rFactor 2 API within the Le Mans Ultimate environment. The standard API pointers for these specific physics variables are disconnected in the LMU engine build.

To rectify this, the integration strategy must shift from a passive reliance on the standard plugin to an active deployment of the **Direct Memory Access (DMA)** capable LMU\_SharedMemoryMapPlugin64.dll. By configuring this plugin with EnableDirectMemoryAccess and EnableHWControlInput set to 1 in the CustomPluginVariables.JSON file, the plugin will bypass the broken API, scan the memory for the live data structures, and populate the shared memory buffer with valid floating-point values. This approach, validated by the architectures of SimHub, Crew Chief, and Tiny Pedal, represents the only viable path for high-fidelity telemetry in Le Mans Ultimate.

#### **Works cited**

1. rF2SharedMemoryMapPlugin/Source/rFactor2SharedMemoryMap.cpp at master \- GitHub, accessed December 7, 2025, [https://github.com/TheIronWolfModding/rF2SharedMemoryMapPlugin/blob/master/Source/rFactor2SharedMemoryMap.cpp](https://github.com/TheIronWolfModding/rF2SharedMemoryMapPlugin/blob/master/Source/rFactor2SharedMemoryMap.cpp)  
2. TheIronWolfModding/rF2SharedMemoryMapPlugin: rFactor 2 Internals Shared Memory Map Plugin \- GitHub, accessed December 7, 2025, [https://github.com/TheIronWolfModding/rF2SharedMemoryMapPlugin](https://github.com/TheIronWolfModding/rF2SharedMemoryMapPlugin)  
3. Info Required \- \[Physics\] Shared memory bugged | Le Mans Ultimate Community, accessed December 7, 2025, [https://community.lemansultimate.com/index.php?threads/physics-shared-memory-bugged.4109/](https://community.lemansultimate.com/index.php?threads/physics-shared-memory-bugged.4109/)  
4. LMU Shared Memory wrong | Le Mans Ultimate Community, accessed December 7, 2025, [https://community.lemansultimate.com/index.php?threads/lmu-shared-memory-wrong.7456/](https://community.lemansultimate.com/index.php?threads/lmu-shared-memory-wrong.7456/)  
5. THE shared memory | Le Mans Ultimate Community, accessed December 7, 2025, [https://community.lemansultimate.com/index.php?threads/the-shared-memory.10812/](https://community.lemansultimate.com/index.php?threads/the-shared-memory.10812/)  
6. Le Mans Ultimate | DR Sim Manager, accessed December 7, 2025, [https://docs.departedreality.com/dr-sim-manager/general/sources/le-mans-ultimate](https://docs.departedreality.com/dr-sim-manager/general/sources/le-mans-ultimate)  
7. Telemetry not working in LMU – Game detected by SimPro Manager V2.1.1 but no data on GT Neo / Alpha Evo \[@Simagic\] \- Reddit, accessed December 7, 2025, [https://www.reddit.com/r/Simagic/comments/1lj2kq8/telemetry\_not\_working\_in\_lmu\_game\_detected\_by/](https://www.reddit.com/r/Simagic/comments/1lj2kq8/telemetry_not_working_in_lmu_game_detected_by/)  
8. rFactor 2 | DR Sim Manager, accessed December 7, 2025, [https://docs.departedreality.com/dr-sim-manager/general/sources/rfactor-2](https://docs.departedreality.com/dr-sim-manager/general/sources/rfactor-2)  
9. Temur Bobokhidze | Le Mans Ultimate Community, accessed December 7, 2025, [https://community.lemansultimate.com/index.php?members/temur-bobokhidze.2222/](https://community.lemansultimate.com/index.php?members/temur-bobokhidze.2222/)  
10. Releases · tembob64/LMU\_SharedMemoryMapPlugin \- GitHub, accessed December 7, 2025, [https://github.com/tembob64/LMU\_SharedMemoryMapPlugin/releases](https://github.com/tembob64/LMU_SharedMemoryMapPlugin/releases)  
11. Add missing parameters to telemetry for plugins | Le Mans Ultimate Community, accessed December 7, 2025, [https://community.lemansultimate.com/index.php?threads/add-missing-parameters-to-telemetry-for-plugins.66/](https://community.lemansultimate.com/index.php?threads/add-missing-parameters-to-telemetry-for-plugins.66/)  
12. Download here: SimHub Dashboards | Le Mans Ultimate Community, accessed December 7, 2025, [https://community.lemansultimate.com/index.php?threads/download-here-simhub-dashboards.646/](https://community.lemansultimate.com/index.php?threads/download-here-simhub-dashboards.646/)  
13. \[REL\] \- Crew Chief v4.5 with rFactor 2 support | Studio-397 Forum, accessed December 7, 2025, [https://forum.studio-397.com/index.php?threads/crew-chief-v4-5-with-rfactor-2-support.54421/](https://forum.studio-397.com/index.php?threads/crew-chief-v4-5-with-rfactor-2-support.54421/)  
14. mrbelowski/CrewChiefV4 \- GitHub, accessed December 7, 2025, [https://github.com/mrbelowski/CrewChiefV4](https://github.com/mrbelowski/CrewChiefV4)  
15. Crew chief not working anymore : r/LeMansUltimateWEC \- Reddit, accessed December 7, 2025, [https://www.reddit.com/r/LeMansUltimateWEC/comments/1hcuxdj/crew\_chief\_not\_working\_anymore/](https://www.reddit.com/r/LeMansUltimateWEC/comments/1hcuxdj/crew_chief_not_working_anymore/)  
16. Can't get CrewChief working. : r/LeMansUltimateWEC \- Reddit, accessed December 7, 2025, [https://www.reddit.com/r/LeMansUltimateWEC/comments/1jcb5wi/cant\_get\_crewchief\_working/](https://www.reddit.com/r/LeMansUltimateWEC/comments/1jcb5wi/cant_get_crewchief_working/)  
17. Telemetry \- SecondMonitor (Timing & status App) \- KW Studios Forum, accessed December 7, 2025, [https://forum.kw-studios.com/index.php?threads/secondmonitor-timing-status-app.9587/](https://forum.kw-studios.com/index.php?threads/secondmonitor-timing-status-app.9587/)  
18. Upvote missing parameters to telemetry for plugins feature request\! /|\\ Simhub NeoRed Plugins and dashboard (Last update: 26/09/2025 / V1.1.0.2) | Page 41 | Le Mans Ultimate Community, accessed December 7, 2025, [https://community.lemansultimate.com/index.php?threads/upvote-missing-parameters-to-telemetry-for-plugins-feature-request-simhub-neored-plugins-and-dashboard-last-update-26-09-2025-v1-1-0-2.7638/page-41](https://community.lemansultimate.com/index.php?threads/upvote-missing-parameters-to-telemetry-for-plugins-feature-request-simhub-neored-plugins-and-dashboard-last-update-26-09-2025-v1-1-0-2.7638/page-41)  
19. TinyPedal/TinyPedal: Free and Open Source telemetry overlay application for racing simulation \- GitHub, accessed December 7, 2025, [https://github.com/TinyPedal/TinyPedal](https://github.com/TinyPedal/TinyPedal)  
20. Download here: SimHub Dashboards | Page 63 | Le Mans Ultimate Community, accessed December 7, 2025, [https://community.lemansultimate.com/index.php?threads/download-here-simhub-dashboards.646/page-63](https://community.lemansultimate.com/index.php?threads/download-here-simhub-dashboards.646/page-63)  
21. Download Here : Simhub NeoRed Plugins (1.2.5.5 \- 14/09/2025) / "NeoSuperDash" and "NeoLiveBoard" dashboard, accessed December 7, 2025, [https://community.lemansultimate.com/index.php?threads/download-here-simhub-neored-plugins-1-2-5-5-14-09-2025-neosuperdash-and-neoliveboard-dashboard.7638/post-62172](https://community.lemansultimate.com/index.php?threads/download-here-simhub-neored-plugins-1-2-5-5-14-09-2025-neosuperdash-and-neoliveboard-dashboard.7638/post-62172)  
22. Download here: SimHub Dashboards | Page 58 | Le Mans Ultimate Community, accessed December 7, 2025, [https://community.lemansultimate.com/index.php?threads/download-here-simhub-dashboards.646/page-58](https://community.lemansultimate.com/index.php?threads/download-here-simhub-dashboards.646/page-58)  
23. TinyPedal \- open source overlay for rF2 (Pacenotes,Radar,FFB,Deltabest,Relative,Fuel Calculator) | Studio-397 Forum, accessed December 7, 2025, [https://forum.studio-397.com/index.php?threads/tinypedal-open-source-overlay-for-rf2-pacenotes-radar-ffb-deltabest-relative-fuel-calculator.71557/](https://forum.studio-397.com/index.php?threads/tinypedal-open-source-overlay-for-rf2-pacenotes-radar-ffb-deltabest-relative-fuel-calculator.71557/)  
24. Problem with Tinypedal after last LMU update | Le Mans Ultimate Community, accessed December 7, 2025, [https://community.lemansultimate.com/index.php?threads/problem-with-tinypedal-after-last-lmu-update.6215/](https://community.lemansultimate.com/index.php?threads/problem-with-tinypedal-after-last-lmu-update.6215/)  
25. Known Issue \- \[Plugins\] Accessing Specific APIs Causes Data Corruption, High CPU Utilization | Le Mans Ultimate Community, accessed December 7, 2025, [https://community.lemansultimate.com/index.php?threads/plugins-accessing-specific-apis-causes-data-corruption-high-cpu-utilization.10719/](https://community.lemansultimate.com/index.php?threads/plugins-accessing-specific-apis-causes-data-corruption-high-cpu-utilization.10719/)