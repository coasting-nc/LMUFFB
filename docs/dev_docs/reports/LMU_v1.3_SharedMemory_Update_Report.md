# LMU v1.3 Shared Memory Update Report

## 1. Introduction
On March 31, 2026, Le Mans Ultimate (LMU) was updated to version 1.3. This update included a significant expansion of the Shared Memory Interface used by the Studio 397 Plugin SDK. This report details the changes to `InternalsPlugin.hpp` and `SharedMemoryInterface.hpp`, their impact on the `lmuFFB` project, and recommendations for future updates.

---

## 2. Detailed Changes

### 2.1 New Enumerations
Two new enum classes were introduced to provide structured vehicle metadata, replacing the need for fragile string parsing in many cases.

| Enum Name | Description |
|---|---|
| `IP_VehicleClass` | Identifies the car category (Hypercar, LMP2, LMP2_ELMS, LMP3, GTE, GT3, PaceCar). |
| `IP_VehicleChampionship` | Identifies the specific season/championship year (WEC 2023-2026, ELMS 2025-2026). |

### 2.2 `TelemWheelV01` Expansion
New fields were added to the per-wheel telemetry structure, primarily focusing on tire thermal behavior and compound identification.

| New Field | Type | Description |
|---|---|---|
| `mTireCarcassTemperature` | `double` | Average carcass temperature (Kelvin). |
| `mTireInnerLayerTemperature[3]` | `double` | Average innermost layer rubber temperature (Kelvin). |
| `mOptimalTemp` | `float_t` | The ideal operating temperature for the current tire. |
| `mCompoundIndex` | `uint8_t` | Index of the tire compound within the brand. |
| `mCompoundType` | `uint8_t` | Type classification of the compound. |

### 2.3 `TelemInfoV01` Expansion (Significant)
The main telemetry structure received the largest update, including Hybrid/Electric powertrain data, live drivetrain settings, and metadata.

| New Field | Type | Description |
|---|---|---|
| **Hybrid / Electric** | | |
| `mBatteryChargeFraction` | `double` | Current battery charge (0.0 to 1.0). |
| `mRegen` | `float` | Current regeneration power (kW). |
| `mSoC` | `float` | State of Charge. |
| `mVirtualEnergy` | `float` | Virtual energy remaining (for specific regulations). |
| **Drivetrain Settings** | | |
| `mABSActive`, `mTCActive` | `bool` | Real-time activation flags for safety systems. |
| `mTC`, `mTCMax` | `uint8_t` | Current Traction Control setting and its range. |
| `mTCSlip`, `mTCSlipMax` | `uint8_t` | TC Slip Target setting and range. |
| `mTCCut`, `mTCCutMax` | `uint8_t` | TC Power Cut setting and range. |
| `mABS`, `mABSMax` | `uint8_t` | Current ABS setting and range. |
| `mMotorMap`, `mMotorMapMax`| `uint8_t` | Engine mapping setting and range. |
| `mMigration`, `mMigrationMax`| `uint8_t` | Brake migration setting and range. |
| `mFrontAntiSway`, `mRearAntiSway`| `uint8_t` | Live anti-roll bar settings (if adjustable). |
| **Metadata** | | |
| `mVehicleModel[30]` | `char` | Cleaned vehicle model name string. |
| `mVehicleClass` | `IP_VehicleClass`| Categorical vehicle class. |
| `mVehicleChampionship` | `IP_VehicleChampionship`| Categorical championship ID. |

### 2.4 `SharedMemoryInterface.hpp` Changes
- **`SharedMemoryGeneric`**: Added `float FFBTorque`, which likely exposes the game's calculated force before hardware processing.
- **`SharedMemoryPathData`**: Added `char pluginsFolder[MAX_PATH]`.

---

## 3. Impact Analysis for lmuFFB

### 3.1 Memory Layout & Alignment
The addition of fields in the middle of `TelemInfoV01` and `TelemWheelV01` has shifted the memory offsets for all subsequent fields (like the `mWheel[4]` array at the end of `TelemInfoV01`).
- **Status**: Since `lmuFFB` compiles against these headers and uses `sizeof` for copying, the project is internally consistent.
- **Action Required**: A full recompile is mandatory to ensure the app matches the new layout provided by the LMU v1.3 executable.

### 3.2 Vehicle Identification (`VehicleUtils`)
The current `lmuFFB` implementation relies on complex string searching (`"ORECA"`, `"499P"`, etc.) to determine car classes and seed default loads.
- **Status**: The new `mVehicleClass` and `mVehicleModel` fields provide a much cleaner and more reliable way to identify cars.
- **Risk**: Legacy string parsing might still be needed for backward compatibility with older logs or if the new fields are not populated for all modded/older content.

### 3.3 Drivetrain Feedback
Previously, `lmuFFB` had to guess if ABS or TC was active based on slip ratios.
- **Status**: The new `mABSActive` and `mTCActive` flags allow for bit-perfect tactile pulses when these systems intervene.

### 3.4 Hybrid Effects
The inclusion of `mRegen` and `mBatteryChargeFraction` opens the door for new haptic effects, such as a "regen whine" or haptic cues when the battery is depleted (Power Derating).

---

## 4. Implementation Recommendations

### Phase 1: Stability (Immediate)
1. **Full Build**: Perform a clean rebuild of the project to align with the new struct offsets.
2. **Regression Testing**: Run the existing test suite to ensure that the core physics (derived from unchanged fields like `mLocalAccel` or `mUnfilteredSteering`) remain accurate.

### Phase 2: Optimization (Short-term)
1. **Refactor `VehicleUtils`**: Update `ParseVehicleClass` to prioritize the `mVehicleClass` enum. Use the string-based search only as a fallback for `ParsedVehicleClass::UNKNOWN`.
2. **Improve Branding**: Use `mVehicleModel` to improve the "Car Brand" detection, reducing the reliance on `mVehicleName` which often contains team/livery names.

### Phase 3: New Features (Medium-term)
1. **Telemetry UI**: Add the new drivetrain settings (TC Level, ABS Level, Motor Map) to the "System Health" or a new "Car Info" panel in the GUI.
2. **Haptic Enhancement**:
   - Trigger a specialized vibration pulse when `mABSActive` or `mTCActive` is true.
   - Implement "Regen Resistance" haptics using the `mRegen` field.

---

## 5. Conclusion
The LMU v1.3 update is a significant leap forward for the plugin ecosystem. By providing structured data for vehicle classes and powertrain states, it allows `lmuFFB` to become more robust and haptically rich. While the memory layout changes require an immediate recompile, the long-term benefits for car identification and system-integrated feedback are substantial.

**Report Generated:** March 31, 2026
**Version Compatibility:** LMU v1.3+ / lmuFFB v0.7.274+
