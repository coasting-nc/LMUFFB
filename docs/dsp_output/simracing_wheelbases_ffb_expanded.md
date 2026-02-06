# Sim Racing Wheelbases – FFB Specifications (Expanded)

Reference table of wheelbase specifications relevant to force feedback processing.

| Wheelbase | Drive | Peak_Torque_Nm | Estimated_Slew_Nm_per_ms | Encoder_Type | Encoder_Resolution | USB_FFB_Rate_Hz | iRacing_360Hz | Telemetry_API | FFB_Processing_Notes |
|-----------|-------|----------------|--------------------------|--------------|--------------------|-----------------|---------------|---------------|----------------------|
| Conspit ARES 12 | Direct Drive | 12 | ~9.5 | Optical | ~23-bit | 1000Hz | Yes | Shared Memory | Crisp detail |
| Conspit Platinum 18 | Direct Drive | 18 | ~8 | Optical | ~23-bit | 1000Hz | Yes | Shared Memory | Higher torque |
| Fanatec ClubSport DD | Direct Drive | 12 | ~5 | Hall/Magnetic | ~20-21-bit* | 1000Hz | Yes | FullForce API | Mid torque class |
| Fanatec ClubSport DD+ | Direct Drive | 15 | ~6 | Hall/Magnetic | ~20-21-bit* | 1000Hz | Yes | FullForce API | Enhanced dynamics |
| Fanatec CSL DD | Direct Drive | 5-8 | ~4-5 | Hall/Magnetic | ~20-21-bit* | 1000Hz | Some | FullForce API | Entry DD |
| Fanatec DD+ | Direct Drive | 15 | 6 | Magnetic | Undisclosed | 1000Hz | Yes | FullForce API | Improved torque |
| Fanatec GT DD Pro | Direct Drive | 8 | ~5 | Hall/Magnetic | ~20-21-bit* | 1000Hz | Some | FullForce API | Mid-range torque |
| Fanatec Podium DD1 | Direct Drive | 20 | ~7 | Likely Optical | ~22-bit* | 1000Hz | Yes | FullForce API | Higher torque |
| Fanatec Podium DD2 | Direct Drive | 24-25 | ~8 | Likely Optical | ~22-bit* | 1000Hz | Yes | FullForce API | Flagship DD |
| Logitech G29/G920 | Gear | 2.1-2.2 | Low | Gear + Motor | Very low | 1000Hz | No | DirectInput | Basic FFB |
| Logitech G923 | Gear | 2.3 | Low | Gear + Motor | Very low | 1000Hz | No | DirectInput+TrueForce | TrueForce extension |
| Logitech RS50 | Direct Drive | 8 | Mid | Motor/Sensor* | ~20-21-bit* | 1000Hz | No* | G Hub + TrueForce | Direct Drive w/ TrueForce |
| Moza R12 | Direct Drive | 12 | 5 | Magnetic | 21-bit | 1000Hz | Yes | Moza SDK/UDP | High-frequency noise potential |
| Moza R16 | Direct Drive | 16 | 6 | Magnetic | 21-bit | 1000Hz | Yes | Moza SDK | Filtering recommended |
| Moza R21 | Direct Drive | 21 | ~7 | Magnetic | ~21-bit* | 1000Hz | Yes | Moza SDK | Rich mid-high torque |
| Moza R9 | Direct Drive | 9 | 4 | Magnetic | 21-bit | 1000Hz | Partial | Moza SDK | Needs damping |
| Sim-Lab DDX26 | Direct Drive | 26 | 8 | Optical Absolute | 24-bit | 1000Hz+ | Yes | RaceDirector | Extremely clean output |
| Sim-Lab DDX39 | Direct Drive | 39 | 10 | Optical Absolute | 24-bit | 1000Hz+ | Yes | RaceDirector | Industrial servo behavior |
| Simagic Alpha | Direct Drive | 15 | ? | Absolute | ~18-bit equiv (~262k ppr) | 40kHz internal/1000Hz | Partial | SimPro Manager | 3rd Gen filter |
| Simagic Alpha EVO | Direct Drive | 12 | ? | Absolute | 21-bit | 20kHz internal/1000Hz | Not documented | SimPro Manager | Zero cogging tech |
| Simagic Alpha EVO Pro | Direct Drive | 18 | ? | Absolute | 21-bit | 20kHz internal/1000Hz | Not documented | SimPro Manager | Higher torque EVO |
| Simagic Alpha EVO Sport | Direct Drive | 9 | ? | Absolute | 21-bit | 20kHz internal/1000Hz | Not documented | SimPro Manager | Low inertia design |
| Simagic Alpha Mini | Direct Drive | 10 | ? | Absolute | ~18-bit equiv (~262k ppr) | 40kHz internal/1000Hz | Partial | SimPro Manager | Wireless/data high internal rate |
| Simagic Alpha U | Direct Drive | 23 | ? | Absolute | ~18-bit equiv (~262k ppr) | 40kHz internal/1000Hz | Partial | SimPro Manager | Top torque class |
| Simucube 2 Pro | Direct Drive | 25 | ~8 | Optical Absolute | ~22-24-bit | 1000Hz | Yes | Simucube API / Shared Memory | Torque reconstruction |
| Simucube 2 Sport | Direct Drive | 17 | ~6-7 | Optical Absolute | ~22-bit | 1000Hz | Yes | Simucube API / Shared Memory | Balanced detail |
| Simucube 2 Ultimate | Direct Drive | 32 | ~9.5 | Optical Absolute | ~22-24-bit | 1000Hz | Yes | Simucube API / Shared Memory | Premium output |
| Simucube 3 Pro | Direct Drive | 23 | ~8-9 | Optical Absolute | ~23-bit | 1000Hz | Yes | Simucube API / Shared Memory | Next-gen encoding |
| Simucube 3 Ultimate | Direct Drive | 35 | ~10 | Optical Absolute | ~23-bit | 1000Hz | Yes | Simucube API / Shared Memory | High torque/bandwidth |
| Thrustmaster Belt/Hybrid | Hybrid | 3-6 | Lower | Belt & Sensor | Low | 1000Hz | No | DirectInput | Lower transient detail |
| Thrustmaster T248/TX/TS-P | Hybrid/Belt | 3.5-6.5 | Low-Mid | Belt + Sensor | Low | 1000Hz | No | DirectInput | Belt compliance |
| Thrustmaster T300 RS | Hybrid/Belt | 3.9 | Low | Belt + Sensor | Low | 1000Hz | No | DirectInput | Belt compliance |
| Thrustmaster T598 | Direct Drive | 5-10 | Mid | Hall/Mag* | ~20-21-bit* | 1000Hz | Limited | DirectInput | Axial flux smooth |
| Thrustmaster T818 | Direct Drive | 10 | Mid | Magnetic/Hall* | ~20-21-bit* | 1000Hz | Partial | DirectInput | Entry DD |
| VRS DFP15 | Direct Drive | 15 | 7 | Incremental Optical | ~21-bit @10kHz | 1000Hz | Yes | USB/ Config / Shared Memory | Very raw minimal smoothing |
| VRS DFP20 | Direct Drive | 20 | High | Optical | ~22-bit | 1000Hz | Yes | USB/ Config / Shared Memory | Higher torque VRS Pro |
| VRS uDFP20 | Direct Drive | 6-20 | Mid | Optical | ~22-bit | 1000Hz | Partial | USB/ Config / Shared Memory | Tunable torque |

---

*Converted from `simracing_wheelbases_ffb_expanded.xml` (Excel XML format)*
